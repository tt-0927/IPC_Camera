/**
 * @FilePath     : classroom_gateway.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 17:04:19
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 17:10:12
 * @Description  : 课堂分析网关实现
 */

#include "classroom_gateway.h"

#include <memory>
#include <set>

#include "cJSON.h"
#include "classroom_config.h"
#include "classroom_config_adapter.h"
#include "classroom_stats_sink.h"
#include "control_manage.h"
#include "dlog.h"
#include "raw_model_handle.h"
#include "rk_model_handle.h"
#include "tvsdk_event.h"
#include "event_linkage.h"

namespace Classroom
{
namespace
{
/**
 * @brief   : 「这个模型键我已经报过一次了」的去重表
 * @return   {bool} true 表示本次是第一次，调用方应当打日志
 */
bool firstBackendNotice(const std::string &strKey)
{
    static std::set<std::string> s_setNoticed;
    return s_setNoticed.insert(strKey).second;
}
}

namespace
{
/* "当前活着的网关"：构造时登记、析构时摘除。 */
CClassroomGateway *s_pAliveGateway = nullptr;
}

CClassroomGateway::CClassroomGateway()
{
    /* 构造即装配。装配失败只把本对象留成"未就绪"，不向创建方（CAlgoStreamDeal）返回错误：
       这条创建链上还挂着整机其余算法能力的启动，课堂起不来不该把它们一起带崩 */
    init();
    s_pAliveGateway = this;
}

CClassroomGateway::~CClassroomGateway()
{
    /* 先摘指针再拆引擎：否则拆除过程中走配置下发会摸到半拆的引擎 */
    if (s_pAliveGateway == this)
    {
        s_pAliveGateway = nullptr;
    }
    deinit();
}

CClassroomGateway *CClassroomGateway::instance()
{
    return s_pAliveGateway;
}

bool CClassroomGateway::init()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (m_spEngine)
    {
        dlog_warn("课堂分析网关: 重复装配，已忽略（现有引擎继续在跑）");
        return true;
    }

    const std::string strPath = CClassroomConfig::defaultPath();

    /* 声明顺序见 classroom_gateway.h：管线必须在引擎之前声明，
       这样无论走成功路径（deinit 显式释放）还是这里的失败回滚（出作用域逆序析构），
       都是引擎先亡、管线后亡 —— 引擎的节拍线程调用的正是管线 */
    std::unique_ptr<CPerceivePipeline> spPerceive;
    std::unique_ptr<CClassroomEngine> spEngine = std::make_unique<CClassroomEngine>();

    spPerceive = std::make_unique<CPerceivePipeline>(spEngine.get());

    /* 加载器按 backend 分派。写在这里而不是塞进 CRkModelHandle::create 内部：
       「哪些后端已经有实现」是装配期的事实，适配器类不该知道自己是不是唯一的那个适配器 */
    spEngine->setLoader([](const CModelDesc_S &stDesc) -> ModelPtr {
        if (CLASSROOM_BACKEND_YOLOV5 == stDesc.strBackend ||
            CLASSROOM_BACKEND_YOLOV8 == stDesc.strBackend ||
            CLASSROOM_BACKEND_YOLOV8_POINT == stDesc.strBackend)
        {
            return CRkModelHandle::create(stDesc);
        }

        /* raw = 自己前后处理的模型（FastPose 出关键点、表情出分类向量）：
           张量进、浮点向量出，语义解释留给各自的调用方（见 raw_model_handle.h 文件头） */
        if (CLASSROOM_BACKEND_RAW == stDesc.strBackend)
        {
            return CRawModelHandle::create(stDesc);
        }

        /* 走到这里只可能是配置写了一个不存在的后端名。这里**不能**顺手挑一个适配器凑数：
           后端选错不会崩，只会按错的张量布局解析输出，表现为「能跑、框乱飞」，
           比一次可观测的加载失败难查一个量级。如实返回 nullptr，池子记 LOAD_FAILED */
        if (firstBackendNotice(stDesc.strKey))
        {
            dlog_warn("课堂分析网关: 模型 %s 的后端名 %s 不是已实现的后端，按 LOAD_FAILED 上报"
                      "（请修正下发配置里的 models[].backend）",
                      stDesc.strKey.c_str(), stDesc.strBackend.c_str());
        }
        return ModelPtr();
    });

    /* 感知回调与它的读数出口（P1）。两个 lambda 都捕获裸指针：
       管线由本网关持有且比引擎活得久，引擎的节拍线程在 deinit() 里已经 join */
    CPerceivePipeline *pPerceive = spPerceive.get();
    spEngine->setPerceiveFn([pPerceive](const FramePtr &spPixel, const ConfigPtr &spConf,
                                        const FeatureFramePtr &spOut) {
        return pPerceive->run(spPixel, spConf, spOut);
    });
    spEngine->setPerceiveStatFn([pPerceive]() { return pPerceive->statText(); });
    /* 考勤段取比对结论的第三条线（引擎→感知，方向与读数那条相同）。
       管线内部那张表按跟踪 ID 索引且只在节拍线程被碰，这里只是把它的取走动作转过去 */
    spEngine->setFaceMatchTakeFn([pPerceive](int32_t nTrackId, CFaceMatch_S &rstOut) {
        return pPerceive->takeFaceMatch(nTrackId, rstOut);
    });

    /* 即时事件告警通道：207/209 的班级级 BEGIN
       一出来就经 TVSDK 的 AI 目标告警推给平台，平台据此主动查询教师机的讲台状态 */
    CClassroomEngine *pEngineRaw = spEngine.get();
    spEngine->setEventAlarmFn([pEngineRaw](const CBehaviorEvent_S &stEvent, int64_t llWallMs) -> bool {
        std::unique_ptr<NET_AlarmAiObjectInfo_S> pInfo(new NET_AlarmAiObjectInfo_S());
        if (!Classroom::fillClassEventAlarm(stEvent, llWallMs, *pInfo))
        {
            return false;
        }
        /* 本地联动开关：打开后同一条事件也交给事件联动通道（声光/邮件/录像），
           与 TVSDK 推送互相独立——没有客户端时联动仍然生效 */
        if (pEngineRaw)
        {
            const ConfigPtr spSnap = pEngineRaw->config();
            if (spSnap && spSnap->bLinkageEnable)
            {
                EventTriggerContext_S stCtx;
                stCtx.enEventType = Event::Type_E::REAL_ALARM;
                stCtx.llTimestamp = llWallMs;
                stCtx.nObjectType = static_cast<int>(stEvent.enBehavior);
                CEventLinkage::instance()->handleEvent(stCtx);
            }
        }
        if (ControlManage::instance()->tvsdk_get_client_count() <= 0)
        {
            return false;
        }
        return ControlManage::instance()->tvsdk_push_alarm(
                   static_cast<int>(pInfo->uAlarmType), pInfo.get(),
                   static_cast<int>(sizeof(*pInfo))) >= 0;
    });

    /* 平台统计推送 */
    m_spStatsSink = std::make_unique<CTvSdkStatsSink>();
    spEngine->setStatsSink(m_spStatsSink.get());

    std::string strErr;
    if (!spEngine->init(strPath, strErr))
    {
        /* 只降级课堂功能 */
        m_spStatsSink.reset();
        dlog_error("课堂分析网关: 引擎装配失败(%s)，课堂功能关闭", strErr.c_str());
        return false;
    }

    m_spPerceive = std::move(spPerceive);
    m_spEngine = std::move(spEngine);
    dlog_info("课堂分析网关: 就绪，配置文件=%s", strPath.c_str());
    dlog_info("课堂分析网关 状态(装配后): %s", m_spEngine->statusJson().c_str());
    return true;
}

void CClassroomGateway::deinit()
{
    /* 持锁销毁是安全的，且必须是这个顺序 */
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        return;
    }

    /* 退出前落一次全量读数：长跑结束时，这一行就是验收依据（不必额外抓现场） */
    dlog_info("课堂分析网关 状态(停止前): %s", m_spEngine->statusJson().c_str());

    m_spEngine->deinit();
    m_spEngine.reset();
    /* 管线必须在引擎之后放 */
    m_spPerceive.reset();
    /* 统计出口同理：引擎的节拍线程每拍都会问 ready()，必须在引擎 join 之后才回收 */
    m_spStatsSink.reset();
    dlog_info("课堂分析网关: 已停止");
}

void CClassroomGateway::recvMediaData(MediaData_S stMediaData)
{
    /* 只认视频 */
    if (MediaDataType_E::VIDEO_DATA != stMediaData.enType)
    {
        return;
    }

    /* 取流侧（CStreamHandler）已经为这一帧拷了一份私有内存，函数返回后那份拷贝就归我们说了算，
       但它的宿主 stMediaData 是栈上的——所以这里只把指针转交给总线做第二次定长拷贝，
       绝不把这个 shared_ptr 存下来 */
    const int nWidth = stMediaData.stMediaParam.nVideoWidth;
    const int nHeight = stMediaData.stMediaParam.nVideoHeight;

    /* 参数不齐直接丢：总线里那道尺寸闸只会给出同一个「丢」字，
       但把"上游没给宽高"这种驱动/配置问题混进普通丢帧计数里，现场就查不出来了 */
    if (nullptr == stMediaData.pData.get() || nWidth <= 0 || nHeight <= 0 || stMediaData.nSize <= 0)
    {
        return;
    }

    onFrame(stMediaData.pData.get(), stMediaData.nSize, nWidth, nHeight, stMediaData.u64PTS);
}

bool CClassroomGateway::isReady() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return (nullptr != m_spEngine) && m_spEngine->isRunning();
}

bool CClassroomGateway::onFrame(const void *pucNv12, int64_t llLen, int nWidth, int nHeight, uint64_t u64PtsUs)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        return false;
    }
    return m_spEngine->onFrame(pucNv12, llLen, nWidth, nHeight, u64PtsUs);
}

bool CClassroomGateway::onConfigText(const std::string &strJson, std::string &rstrResp)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        rstrResp = "课堂分析未运行";
        return false;
    }

    std::string strErr;
    if (!m_spEngine->reloadConfigFromText(strJson, strErr))
    {
        rstrResp = "拒绝: " + strErr;
        dlog_error("课堂分析网关: 文本配置被拒绝: %s", strErr.c_str());
        return false;
    }

    const ConfigPtr spSnap = m_spEngine->config();
    rstrResp = "接受, 版本 " + std::to_string(spSnap ? spSnap->llVersion : 0);
    dlog_info("课堂分析网关: 文本配置已生效，%s", rstrResp.c_str());
    return true;
}

bool CClassroomGateway::onConfigFile(const std::string &strPath, std::string &rstrResp)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        rstrResp = "课堂分析未运行";
        return false;
    }

    std::string strErr;
    if (!m_spEngine->reloadConfigFromFile(strPath, strErr))
    {
        rstrResp = "拒绝: " + strErr;
        dlog_error("课堂分析网关: 配置文件 %s 被拒绝: %s", strPath.c_str(), strErr.c_str());
        return false;
    }

    const ConfigPtr spSnap = m_spEngine->config();
    rstrResp = "接受, 版本 " + std::to_string(spSnap ? spSnap->llVersion : 0);
    dlog_info("课堂分析网关: 已重载 %s，%s", strPath.c_str(), rstrResp.c_str());
    return true;
}

std::string CClassroomGateway::statusJson() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        /* 不返回空串：调用方（日志/后续通道）拿到的必须始终是一份可解析的 JSON */
        return "{\"running\":false,\"reason\":\"classroom engine not initialized\"}";
    }
    return m_spEngine->statusJson();
}

void CClassroomGateway::dumpStatus(const char *pReason)
{
    dlog_info("课堂分析网关 状态(%s): %s",
              (nullptr != pReason) ? pReason : "手动", statusJson().c_str());
}

/* 回执统一走 JSON：`fill_common()` 只在 `Json::init(data)` 成功时才挂 `Data`，
   纯文本回执会被整个丢掉 ⇒ 网页只看得到 `Return`，看不到版本号与拒绝原因。失败也带 error，前端可直接显示。 */
std::string configReply(bool bOk, int64_t llVersion, bool bPersisted, const std::string &strErr)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (nullptr == pRoot)
    {
        return std::string("{}");
    }
    cJSON_AddBoolToObject(pRoot, "ok", bOk ? 1 : 0);
    if (bOk)
    {
        cJSON_AddNumberToObject(pRoot, "version", static_cast<double>(llVersion));
        cJSON_AddBoolToObject(pRoot, "persisted", bPersisted ? 1 : 0);
    }
    if (!strErr.empty())
    {
        cJSON_AddStringToObject(pRoot, "error", strErr.c_str());
    }
    char *pszOut = cJSON_PrintUnformatted(pRoot);
    std::string strOut = (nullptr != pszOut) ? pszOut : "{}";
    if (nullptr != pszOut)
    {
        cJSON_free(pszOut);
    }
    cJSON_Delete(pRoot);
    return strOut;
}

bool CClassroomGateway::readConfigPage(const char *pszFeature, std::string &rstrResp)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        rstrResp = configReply(false, 0, false, "课堂分析未运行");
        return false;
    }

    ConfigPage_E enFeature = ConfigPage_E::PAGE_NUM;
    if (!parseConfigPage(pszFeature, enFeature))
    {
        rstrResp = configReply(false, 0, false,
                               std::string("未知功能页: ") + ((nullptr != pszFeature) ? pszFeature : ""));
        dlog_warn("课堂分析网关: 配置取用：功能页不认识: %s", (nullptr != pszFeature) ? pszFeature : "(null)");
        return false;
    }

    /* 取原文而不是把快照序列化回 JSON：序列化器要和 parseIntoJson 维护同一份键名，
       那是两份必然漂移的口径（classroom_config.h 的 lastConfigText 注释里写明了代价） */
    rstrResp = Classroom::readConfigPage(m_spEngine->configText(), enFeature);
    return true;
}

bool CClassroomGateway::configPageSet(const char *pszFeature, const std::string &strPartial,
                                     std::string &rstrResp)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_spEngine)
    {
        rstrResp = configReply(false, 0, false, "课堂分析未运行");
        return false;
    }

    ConfigPage_E enFeature = ConfigPage_E::PAGE_NUM;
    if (!parseConfigPage(pszFeature, enFeature))
    {
        rstrResp = configReply(false, 0, false,
                               std::string("未知功能页: ") + ((nullptr != pszFeature) ? pszFeature : ""));
        dlog_warn("课堂分析网关: 配置下发：功能页不认识: %s", (nullptr != pszFeature) ? pszFeature : "(null)");
        return false;
    }

    /* 合入的是整份原文（只动本页那几个键），再走与 onConfigText 同一条 reloadFromText：
       同一套校验、同一条换入路径，不开"配置专用"的第二条写入路径 */
    std::string strMerged;
    std::string strErr;
    if (!Classroom::mergeConfigPage(m_spEngine->configText(), enFeature,
                                   strPartial, strMerged, strErr))
    {
        /* 合并失败：旧快照继续生效，绝不出现「半套新参数」 */
        rstrResp = configReply(false, 0, false, strErr);
        dlog_error("课堂分析网关: 页面 %s 配置被拒绝: %s", configPageTitle(enFeature), strErr.c_str());
        return false;
    }

    return applyMergedText(strMerged, configPageTitle(enFeature), rstrResp);
}

/* 换入 + 落盘 + 回执：单页 Set 与区域表 Set 共用这一条路径。
   ★ 落盘是本轮补的：`reloadFromText` 只换内存快照，此前**没有任何写文件路径**，
   症状是"网页保存成功、重启回旧值"。
   写失败**不回滚内存里的新配置**（它已经生效了），但必须在回执里说清 persisted=false，
   否则网页会把"这次保存不会保留"当成"保存成功"。 */
bool CClassroomGateway::applyMergedText(const std::string &strMerged, const char *pszTitle,
                                        std::string &rstrResp)
{
    std::string strErr;
    if (!m_spEngine->reloadConfigFromText(strMerged, strErr))
    {
        rstrResp = configReply(false, 0, false, strErr);
        dlog_error("课堂分析网关: %s 配置被拒绝: %s", pszTitle, strErr.c_str());
        return false;
    }

    std::string strSaveErr;
    const bool bPersisted = CClassroomConfig::saveTextToFile(CClassroomConfig::defaultPath(),
                                                             strMerged, strSaveErr);
    if (!bPersisted)
    {
        dlog_error("课堂分析网关: %s 已生效但落盘失败: %s", pszTitle, strSaveErr.c_str());
    }

    const ConfigPtr spSnap = m_spEngine->config();
    const int64_t llVersion = (spSnap ? spSnap->llVersion : 0);
    rstrResp = configReply(true, llVersion, bPersisted, bPersisted ? std::string() : strSaveErr);
    dlog_info("课堂分析网关: %s 配置已生效，版本 %lld，落盘=%d",
              pszTitle, static_cast<long long>(llVersion), bPersisted ? 1 : 0);
    return true;
}

CClassroomEngine *CClassroomGateway::engine() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_spEngine.get();
}
} /* namespace Classroom end */


bool classroomConfigPageGet(const char *pszFeature, std::string &rstrResp)
{
    Classroom::CClassroomGateway *pGw = Classroom::CClassroomGateway::instance();
    if (nullptr == pGw)
    {
        rstrResp = "{\"ok\":false,\"error\":\"课堂分析网关未装配\"}";
        return false;
    }
    return pGw->readConfigPage(pszFeature, rstrResp);
}

bool classroomConfigPageSet(const char *pszFeature, const std::string &strPartial,
                          std::string &rstrResp)
{
    Classroom::CClassroomGateway *pGw = Classroom::CClassroomGateway::instance();
    if (nullptr == pGw)
    {
        rstrResp = "{\"ok\":false,\"error\":\"课堂分析网关未装配\"}";
        return false;
    }
    return pGw->configPageSet(pszFeature, strPartial, rstrResp);
}

std::string classroomConfigReport()
{
    Classroom::CClassroomGateway *pGw = Classroom::CClassroomGateway::instance();
    if (nullptr == pGw)
    {
        /* 与网关 statusJson() 的未就绪回执同一口径：调用方拿到的必须始终可解析 */
        return "{\"running\":false,\"reason\":\"classroom gateway not initialized\"}";
    }
    /* 课程报告页要的是"实到/应到 + 行为时间轴 + 专注度"，不是运维诊断计数。
       引擎的 reportJson() 只读三份无损快照（考勤账本 / 会话累计 / 最近一条群体样本），
       不消费事件环——不能用 buildReport()，那会把下一份上报的事件吃掉 */
    Classroom::CClassroomEngine *pEngine = pGw->engine();
    if (nullptr == pEngine)
    {
        return "{\"running\":false,\"reason\":\"classroom engine not initialized\"}";
    }
    return pEngine->reportJson();
}
