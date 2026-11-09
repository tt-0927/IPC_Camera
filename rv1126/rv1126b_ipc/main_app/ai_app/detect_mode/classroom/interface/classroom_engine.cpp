/**
 * @FilePath     : classroom_engine.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-17 22:18:07
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 12:05:00
 * @Description  : 课堂分析引擎实现（装配 / 节拍 / 预热 / 轮转 / 守护 / 观测）
 */

#include "classroom_engine.h"

#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>

#include "classroom_clock.h"
#include "classroom_fmt.h"
#include "crop_geom.h" 
#include "classroom_warm_policy.h"
#include "dlog.h"
#include "face_gallery.h"
#include "face_gallery_db.h"
#include "stats_feed.h"
#include "tvsdk_event.h"

namespace Classroom
{
namespace
{
/* 三态 → 场景层的整数读数（1 成立 / 0 不成立 / -1 没测） */
int32_t triToInt(TriState_E enState)
{
    if (TriState_E::TS_TRUE == enState)
    {
        return 1;
    }
    return (TriState_E::TS_FALSE == enState) ? 0 : -1;
}

/* 心跳 bin= 列的构建号：编译期注入，空格换 '-'（心跳按空白切列，
   值里带空格会撑破读数脚本）。只算一次，进程内恒定 */
const char *binBuildColumn()
{
    static const std::string strBuild = [] {
        std::string str = CLASSROOM_BIN_BUILD;
        std::replace(str.begin(), str.end(), ' ', '-');
        return str;
    }();
    return strBuild.c_str();
}

/* 配置文件的内容指纹 */
uint64_t configFileHash(const char *pszPath)
{
    FILE *pFile = fopen(pszPath, "rb");
    if (nullptr == pFile)
    {
        return 0;
    }
    uint64_t u64Hash = 0xCBF29CE484222325ULL;
    uint8_t aucBuf[4096];
    size_t uRead = 0;
    while ((uRead = fread(aucBuf, 1, sizeof(aucBuf), pFile)) > 0)
    {
        for (size_t i = 0; i < uRead; ++i)
        {
            u64Hash ^= aucBuf[i];
            u64Hash *= 0x100000001B3ULL;
        }
    }
    fclose(pFile);
    return u64Hash;
}

/* 时段阈值的搬运：配置块（rules.group）与群体层入参是两个类型，中间隔这一层而不是让   group_stats 去 include 配置层——那会把整套 JSON 依赖拖进每一个用群体层的单测，
   而这里要的只是三个整数 */
CGroupPeriodGate_S groupPeriodGateOf(const CConfigSnapshot_S &stSnap)
{
    CGroupPeriodGate_S stGate;
    stGate.nListenMin  = stSnap.stGroup.nListenMinPersons;
    stGate.nReadingMin = stSnap.stGroup.nReadingMinPersons;
    stGate.nDiscussMin = stSnap.stGroup.nDiscussMinPersons;
    return stGate;
}

/* 黑板角色区域 → 源帧归一化（0~1）外接框（给 CBoardWatcher 取景用） */
bool boardRectNormSrc(const CConfigSnapshot_S &stSnap, CRectf_S &rstNorm)
{
    double dfMinX = 0.0;
    double dfMinY = 0.0;
    double dfMaxX = 0.0;
    double dfMaxY = 0.0;
    bool bKnown = false;

    /* Web 口径换算到 0~1：像素口径除参考分辨率，0~8191 口径除那个惯用的满量程。
       参考分辨率配成 0 是配置错误，validate 已拦（region_ref.width 为 0 是硬错误），
       这里再挡一次除法——除零在这里不崩，只会给出 inf，然后被钳成一个占满整幅的框 */
    const double dfRefX = (static_cast<double>(stSnap.nRegionRefWidth) > 0.0)
                              ? static_cast<double>(stSnap.nRegionRefWidth) : 1.0;
    const double dfRefY = (static_cast<double>(stSnap.nRegionRefHeight) > 0.0)
                              ? static_cast<double>(stSnap.nRegionRefHeight) : 1.0;
    const bool bNorm = (CoordSpace_E::COORD_SPACE_NORM_8192 == stSnap.enRegionSpace);

    for (const CRegionRoleRef_S &stRole : stSnap.vecRegionRoles)
    {
        if (RegionRole_E::REGION_ROLE_BOARD != stRole.enRole)
        {
            continue;
        }
        for (const int32_t nId : stRole.vecRegionIds)
        {
            for (const CRegionDesc_S &stDesc : stSnap.vecRegions)
            {
                if (stDesc.nId != nId)
                {
                    continue;
                }
                CRectf_S stBox = stDesc.stRectWeb;
                if (RegionShape_E::REGION_POLY == stDesc.enShape)
                {
                    stBox = CRectf_S();
                    bool bFirst = true;
                    for (const CPointf_S &stPt : stDesc.vecPointsWeb)
                    {
                        if (bFirst)
                        {
                            stBox = CRectf_S{ stPt.fX, stPt.fY, stPt.fX, stPt.fY };
                            bFirst = false;
                            continue;
                        }
                        stBox.fX1 = (stPt.fX < stBox.fX1) ? stPt.fX : stBox.fX1;
                        stBox.fY1 = (stPt.fY < stBox.fY1) ? stPt.fY : stBox.fY1;
                        stBox.fX2 = (stPt.fX > stBox.fX2) ? stPt.fX : stBox.fX2;
                        stBox.fY2 = (stPt.fY > stBox.fY2) ? stPt.fY : stBox.fY2;
                    }
                }
                if (stBox.area() <= 0.0f)
                {
                    continue;   /* 退化框（画成一个点或一条线）不参与外接：它会把框拉偏而不加信息 */
                }
                const double fX1 = stBox.fX1 / (bNorm ? CLASSROOM_REGION_NORM_MAX : dfRefX);
                const double fY1 = stBox.fY1 / (bNorm ? CLASSROOM_REGION_NORM_MAX : dfRefY);
                const double fX2 = stBox.fX2 / (bNorm ? CLASSROOM_REGION_NORM_MAX : dfRefX);
                const double fY2 = stBox.fY2 / (bNorm ? CLASSROOM_REGION_NORM_MAX : dfRefY);
                if (!bKnown)
                {
                    dfMinX = fX1; dfMinY = fY1; dfMaxX = fX2; dfMaxY = fY2;
                    bKnown = true;
                    continue;
                }
                dfMinX = (fX1 < dfMinX) ? fX1 : dfMinX;
                dfMinY = (fY1 < dfMinY) ? fY1 : dfMinY;
                dfMaxX = (fX2 > dfMaxX) ? fX2 : dfMaxX;
                dfMaxY = (fY2 > dfMaxY) ? fY2 : dfMaxY;
            }
        }
    }
    if (!bKnown)
    {
        return false;
    }
    rstNorm.fX1 = static_cast<float>(dfMinX);
    rstNorm.fY1 = static_cast<float>(dfMinY);
    rstNorm.fX2 = static_cast<float>(dfMaxX);
    rstNorm.fY2 = static_cast<float>(dfMaxY);
    return true;
}


/* 降采样系数：先降频、再卸模型、最后才关功能，顺序不可颠倒 */
int32_t stretchOfLevel(GuardLevel_E enLevel)
{
    switch (enLevel)
    {
        case GuardLevel_E::GL_SOFT:      return 150;
        case GuardLevel_E::GL_HARD:      return 300;
        case GuardLevel_E::GL_EMERGENCY: return 100;  /* 此时根本不做感知，系数无意义 */
        default:                         return 100;
    }
}

/* 水位回落到该倍数以上、且连续这么多次采样，才允许往下降一档 */
#define GUARD_RECOVER_RATIO (1.50)
#define GUARD_RECOVER_SAMPLES (3)

/* 预热的三个时间常量（重试周期、复议窗口、复核保留期）连同它们各自的理由，一起搬进了
   classroom_warm_policy.h：边界值与照它行事的判据得分在同一处，主机用例才钉得住
   "改了配置立刻再问一次""缺件不刷屏""轮转有出口"这三条 */

/* 单个模型的 RGA 输入缓冲块数：至少 2 块，规避实测到的「同指针+同尺寸输入被后端缓存」 */
#define CROP_BUF_NUM (2)

void setThreadName(const char *pName)
{
    pthread_setname_np(pthread_self(), pName);
}

/* statusJson / 心跳行的拼装原语在 common/classroom_fmt.h：
   搬去那里是因为判据必须住在主机闸门链得到的层，引擎链 rknn 进不了闸 */

const char *levelName(GuardLevel_E enLevel)
{
    switch (enLevel)
    {
        case GuardLevel_E::GL_SOFT:      return "soft";
        case GuardLevel_E::GL_HARD:      return "hard";
        case GuardLevel_E::GL_EMERGENCY: return "emergency";
        default:                         return "normal";
    }
}

const char *stateName(BackendState_E enState)
{
    switch (enState)
    {
        case BackendState_E::READY:         return "ready";
        case BackendState_E::MODEL_MISSING: return "model_missing";
        case BackendState_E::OUT_OF_BUDGET: return "out_of_budget";
        case BackendState_E::LOAD_FAILED:   return "load_failed";
        case BackendState_E::UNLOADED:      return "unloaded";
        case BackendState_E::GROUP_BUSY:    return "group_busy";
        case BackendState_E::CAPTURE_MISS:  return "capture_miss";
        case BackendState_E::BACKEND_BUSY:  return "backend_busy";
        default:                            return "disabled";
    }
}

const char *groupName(ModelGroup_E enGroup)
{
    switch (enGroup)
    {
        case ModelGroup_E::GROUP_BASE: return "base";
        case ModelGroup_E::GROUP_POSE: return "pose";
        case ModelGroup_E::GROUP_FACE: return "face";
        default:                       return "other";
    }
}
}

CClassroomEngine::~CClassroomEngine()
{
    deinit();
}

void CClassroomEngine::setLoader(ModelLoaderFn fnLoader)
{
    m_fnLoader = std::move(fnLoader);
}

void CClassroomEngine::setPerceiveFn(CPerceiveFn fnPerceive)
{
    m_fnPerceive = std::move(fnPerceive);
}

void CClassroomEngine::setPerceiveStatFn(CPerceiveStatFn fnStat)
{
    m_fnPerceiveStat = std::move(fnStat);
}

void CClassroomEngine::setFaceMatchTakeFn(CFaceMatchTakeFn fnTake)
{
    m_fnFaceTake = std::move(fnTake);
}

void CClassroomEngine::applyGallery(const CConfigSnapshot_S &stSnap)
{
    const CGalleryConf_S &stGallery = stSnap.stGallery;

    /* 关掉与"没有名单"是两件事，但落到这里同一个形态（空底库）：区别在日志里要说清楚，
       否则现场只看得到"名单一直空" */
    std::unique_ptr<IFaceGallery> spNew;
    if (!stGallery.bEnable)
    {
        dlog_info("classroom engine: 花名册按配置关闭，考勤不会给任何人点名");
    }
    else if ("sqlite" == stGallery.strSource)
    {
        /* 表名先过 galleryTableOk 再交给实现方：公共仓把它直接拼进 SQL（见其注释），
           这道闸是那条路径上唯一的入口校验。失败原因只报长度不报内容——
           表名可能是人名，日志是全世界可读的 */
        if (!galleryTableOk(stGallery.strTable))
        {
            dlog_error("classroom engine: gallery.table 被拒 (%zu 字节)，未建 sqlite 底库",
                       stGallery.strTable.size());
        }
        else
        {
            std::unique_ptr<CSqliteFaceGallery> spSqlite(new (std::nothrow) CSqliteFaceGallery());
            if (spSqlite && spSqlite->setSource(stGallery.strTable, stGallery.nReloadSec,
                                                stGallery.nMaxEnroll))
            {
                spNew = std::move(spSqlite);
            }
        }
    }
    else if ("file" == stGallery.strSource)
    {
        std::unique_ptr<CFileFaceGallery> spFile(new (std::nothrow) CFileFaceGallery());
        if (spFile)
        {
            /* setSource 不读文件（读在第一次 poll 里）：这条路不能在这里碰磁盘，
               applySnapshot 之后紧跟着的就是感知，解析压在这一拍会把节拍吃掉 */
            spFile->setSource(stGallery.strPath, stGallery.nReloadSec, stGallery.nMaxEnroll);
            spNew = std::move(spFile);
        }
    }
    else
    {
        /* "none" 是显式答案，其它值是配错 */
        if ("none" == stGallery.strSource)
        {
            dlog_info("classroom engine: gallery source=none，考勤无姓名可用");
        }
        else
        {
            dlog_error("classroom engine: gallery.source 未知，未建底库");
        }
    }

    if (!spNew)
    {
        /* 兜底一具空底库而不是留 null：faceGallery() 的契约是"永不为空"，
           管线与账本因此不必在每个调用点各判一次空（判漏一处就是崩在节拍线程上）。
           CNullFaceGallery 什么也不做，正是"问它要身份它就说没有"那个语义 */
        spNew.reset(new (std::nothrow) CNullFaceGallery());
        if (!spNew)
        {
            /* 连一具空壳都分配不出来：这一版配置干脆不动现有的库（继续用上一份名单），
               而不是把 null 装上去——那等于在这里把上面那条契约破掉，
               而破契约的地方永远不是读它的人先发现 */
            dlog_error("classroom engine: 花名册内存分配失败，沿用旧花名册");
            return;
        }
    }

    const bool bChanged = (nullptr == m_spGallery ||
                           0 != std::strcmp(m_spGallery->name(), spNew->name()));
    m_spGallery = std::move(spNew);
    /* 账本拿同一个指针：它只用 available()/enrollNum() 两处，但必须是同一个实现方，
       否则会出现"感知按 A 库比对、账本按 B 库判断要不要命名" */
    m_stAttend.setGallery(m_spGallery.get());

    if (bChanged)
    {
        dlog_info("classroom engine: 底库来源已生效: %s reload=%ds max_enroll=%d",
                  m_spGallery->name(), stGallery.nReloadSec, stGallery.nMaxEnroll);
    }

}

void CClassroomEngine::setUploadFn(CUploadFn fnUpload)
{
    m_fnUpload = std::move(fnUpload);
}

void CClassroomEngine::setEventAlarmFn(CEventAlarmFn fnEventAlarm)
{
    m_fnEventAlarm = std::move(fnEventAlarm);
}

void CClassroomEngine::setStatsSink(IStatsSink *pStatsSink)
{
    m_stStatsHub.setSink(pStatsSink);
}

CWarmNeed_S CClassroomEngine::warmNeedOf() const
{
    CWarmNeed_S stNeed;
    stNeed.llPhoneWarmUntilMs = m_llPhoneWarmUntilMs;
    stNeed.llBoardLastActMs = m_llBoardLastActMs;
    stNeed.enLevel = m_enLevel;
    stNeed.bNeedRotate = m_bNeedRotate;
    stNeed.enHeavyTurn = m_enHeavyTurn;
    return stNeed;
}

uint32_t CClassroomEngine::effectiveGroupMask(const CConfigSnapshot_S &stSnap, int64_t llNowMs) const
{
    /* 判据全在 classroom_warm_policy 里（那张行为→组的表、两份事件触发的闩、守护档位、轮转排他），
       这里只把本拍那几个成员打包成入参递过去。不清 m_mapWarmState：见 warmGroups 里那条理由 */
    return warmEffectiveGroupMask(stSnap, llNowMs, warmNeedOf());
}

int64_t CClassroomEngine::sliceOfGroup(ModelGroup_E enGroup) const
{
    const CModelPoolParam_S stParam = m_stPool.param();
    return (ModelGroup_E::GROUP_FACE == enGroup) ? stParam.llSliceFaceMs : stParam.llSlicePoseMs;
}

bool CClassroomEngine::init(const std::string &strConfigPath, std::string &rstrErr)
{
    if (m_bRunning)
    {
        rstrErr = "engine already running";
        return false;
    }

    /* ---- 1. 配置：文件在但校验失败时仍用默认值起线，缺件与坏件都必须可观测 ---- */
    const std::string strPath = strConfigPath.empty() ? CClassroomConfig::defaultPath() : strConfigPath;
    m_stConfig.loadDefaults(strPath);

    if (0 == access(strPath.c_str(), F_OK))
    {
        std::string strErr;
        if (!m_stConfig.reloadFromFile(strPath, strErr))
        {
            /* 现场最常见形态：Web 下发写坏了一个字段。默认值继续跑 + 一条 ERROR，
               比拒绝启动整机功能更强，但坏件信息绝不能只落在返回值里 */
            dlog_error("classroom engine: 配置 %s 被拒，按出厂缺省运行: %s",
                       strPath.c_str(), strErr.c_str());
        }
    }
    else
    {
        dlog_info("classroom engine: %s 无配置文件，按默认值运行", strPath.c_str());
    }

    /* 看护基线：不预置的话第一拍会把同一份内容再载一次（版本凭空 +1，
       心跳里的 config_version 就没法用来回答"现场到底改过没有"） */
    m_strConfigPath = strPath;
    m_llLastConfigCheckMs = 0;
    m_bConfigFilePresent = false;
    m_llConfigMTimeSec = 0;
    m_llConfigSize = 0;
    m_u64ConfigHash = 0;

    struct stat stFileInfo;
    if (0 == ::stat(strPath.c_str(), &stFileInfo))
    {
        m_bConfigFilePresent = true;
        m_llConfigMTimeSec = static_cast<int64_t>(stFileInfo.st_mtime);
        /* 基线指纹一起取：不取的话第一次轮询会看到"指纹不同"而凭空重载一次 */
        m_u64ConfigHash = configFileHash(strPath.c_str());
        m_llConfigSize = static_cast<int64_t>(stFileInfo.st_size);
    }

    ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap)
    {
        rstrErr = "config snapshot unavailable";
        return false;
    }

    m_llBootMs = classroomNowMs();
    m_stStat = CEngineStat_S();
    m_stStat.bRunning = true;
    m_stStat.llConfigVersion = spSnap->llVersion;

    /* ---- 2. 帧总线：固定占用，失败即整体失败（没有像素就没有一切）---- */
    if (!m_stBus.init(spSnap->nSrcWidth, spSnap->nSrcHeight, spSnap->nCanvasWidth,
                      spSnap->nCanvasHeight, spSnap->enMirror, spSnap->nRotate))
    {
        rstrErr = "frame bus init failed, src " + std::to_string(spSnap->nSrcWidth) + "x" +
                  std::to_string(spSnap->nSrcHeight);
        stopModules();
        return false;
    }

    /* ---- 3. 特征总线 ---- */
    if (!m_stFeatureBus.init(0, spSnap->nMaxPersons))
    {
        rstrErr = "feature bus init failed, persons " + std::to_string(spSnap->nMaxPersons);
        stopModules();
        return false;
    }

    /* ---- 3.5 群体聚合中心（P3）。失败只让报表段不可用，不带走感知与事件 ----
       理由：这一块的价值是"给平台一份占比"，它起不来时平台看到的是"这一段没有数据"，
       而如果把整个课堂功能一起拒掉，代价是连事件与人数都没有了 —— 那是把降级做成故障 */
    m_vecRowInput.resize(CLASSROOM_MAX_PERSONS);
    /* 平台统计读账本记录的复用表（每 5s 一次左中离课统计）：与行输入同批定容，运行期不分配 */
    m_vecAttendScratch.reserve(CLASSROOM_MAX_PERSONS);
    /* 热力折样的临时表与行输入同批定容：seatStep 逐行覆写它，运行期不再分配 */
    m_vecSeatObs.resize(CLASSROOM_MAX_PERSONS);
    /* 座位先验的输入表与热力那张同容量、同下标（seatStep 里两遍循环都按 m_vecRowInput 的行序走），
       因此必须在同一处定容——一张定了容一张没定，症状是"人多的那一拍少喂几行"而不是崩 */
    m_vecPriorIn.resize(CLASSROOM_MAX_PERSONS);
    /* 轨迹折样的临时表同批定容（同一处纪律：少一行就是"人多的那一拍少记几个人"，且不报错） */
    m_vecStuObs.resize(CLASSROOM_MAX_PERSONS);
    CDataCenterParam_S stDcParam;
    stDcParam.llReportIntervalMs =
        static_cast<int64_t>(spSnap->stEgress.nReportIntervalSec) * 1000;
    if (!m_stData.init(stDcParam))
    {
        dlog_error("classroom engine: 聚合中心初始化失败，停用指标上报 "
                   "(感知与事件照常运行)");
    }
    /* 事件合并器（P0-2）挂在 data.center 之前：固定容量，init 不会失败。
       挂在这一层而不是规则层，是因为「合并成一条」改变的是**上报口径**——
       规则环里的原始事件还要留给日志与统计对照 */
    m_stMerger.init(CEventMergerParam_S());

    /* ---- 3.6 断网滞留落盘。与聚合中心同一档：起不来只让"补传"这一项能力消失 ----
       目录不可建/不可写的机器上，报表照发、断网期间照丢，但状态里必须有这一条：
       现场问"断网那半小时的数据呢"，答案要么在这行日志里，要么根本没有 */
    std::string strSpoolErr;
    if (!spoolApply(*spSnap, strSpoolErr))
    {
        dlog_error("classroom engine: 报表滞留环不可用，补传关闭: %s",
                   strSpoolErr.c_str());
    }
    m_stStat.bUploadWired = static_cast<bool>(m_fnUpload);
    /* 心跳落点在首拍之前定下来。不放到 metrics() 里"用的时候再说"：第一拍只有 10s，
       而落点要不要建、建成了什么样子，都得在那之前进一次启动日志 */
    metricApply(*spSnap);

    /* ---- 4. 模型池 + 调度器 ---- */
    m_stPool.setParam(spSnap->stPool);
    if (m_fnLoader)
    {
        m_stPool.setLoader(m_fnLoader);
    }

    int32_t nRegistered = 0;
    for (const auto &stDesc : spSnap->vecModels)
    {
        if (!m_stPool.registerModel(stDesc))
        {
            dlog_error("classroom engine: 注册模型 %s 失败", stDesc.strKey.c_str());
            continue;
        }
        ++nRegistered;
    }

    if (!m_stPool.start())
    {
        rstrErr = "model pool start failed";
        stopModules();
        return false;
    }

    m_spSched = std::make_unique<CInferScheduler>(&m_stPool);
    if (!m_spSched->start())
    {
        rstrErr = "infer scheduler start failed";
        stopModules();
        return false;
    }

    dlog_info("classroom engine: 已注册 %d 个模型, loader=%s, perceive=%s, upload=%s evt_alarm=%s spool=%s",
              nRegistered, m_fnLoader ? "injected" : "NONE(model state will be LOAD_FAILED)",
              m_fnPerceive ? "injected" : "absent(no feature frame)",
              m_fnUpload ? "injected" : "NONE(report goes to log only, not spooled)",
              m_fnEventAlarm ? "injected" : "NONE(207/209 BEGIN not pushed)",
              m_stSpool.isReady() ? "ready" : "UNAVAILABLE(no retransmit)");

    /* ---- 5. 规则引擎装配：采样器必须在节拍线程起来之前注册完 ----
       运行期不增删：判定路径拿的是裸指针数组，
       边跑边改这张表就等于让节拍线程读一个正在被改的 vector */
    IBehaviorSampler *apSampler[] = { &m_stStudent, &m_stTeacher };
    for (size_t i = 0; i < sizeof(apSampler) / sizeof(apSampler[0]); ++i)
    {
        if (!m_stRule.addSampler(apSampler[i]))
        {
            rstrErr = "rule sampler register failed: " + std::string(apSampler[i]->name());
            stopModules();
            return false;
        }
    }
    /* 装配收口：从这一刻起「这一行开着却没人判它」才有答案。此刻多半还没有第一份快照
       （快照在节拍里换入），那也不要紧——收口只是划一条"问完了"的线，
       真正的话由第一次 applyConfig 说 */
    m_stRule.verifySamplerAssembly();

    /* ---- 6. 节拍线程：启动即创建、退出才销毁（§4.9）---- */
    m_bRunning = true;
    m_llAppliedVersion = -1;
    m_enLevel = GuardLevel_E::GL_NORMAL;
    m_nRecoverStreak = 0;
    m_llLastWarmMs = 0;
    m_llLastGuardMs = 0;
    m_llLastMetricMs = 0;
    m_llLastHeadBeatMs = 0;
    m_llHeavyUntilMs = 0;
    m_llBoardLastActMs = 0;
    m_enHeavyTurn = ModelGroup_E::GROUP_POSE;
    m_bNeedRotate = false;
    m_u32PixelSeqHandled = 0;
    m_bGeomStale = false;
    m_mapHeld.clear();
    m_mapWarmState.clear();
    m_stRule.reset(m_llBootMs);   /* 上一轮的在场名单、门表状态与事件环都不带进这一轮 */
    /* 故意的「与任何目标都不相等」，逼第一拍完整评估一次（否则会漏掉启动即需要的组） */
    m_u32WantedMask = 0xFFFFFFFFu;
    m_u32ModelMask = 0xFFFFFFFFu;

    const CMemInfo_S stBase = classroomQueryMem();
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_stStat.llRssBaseKb = stBase.llVmRssKb;
        m_stStat.llRssMaxKb = stBase.llVmRssKb;
        m_stStat.llAvailMinKb = stBase.llMemAvailableKb;
        m_stStat.llLastRssKb = stBase.llVmRssKb;
        m_stStat.llLastAvailKb = stBase.llMemAvailableKb;
        m_stStat.llLastMmzKb = stBase.llMmzUsedKb;
        m_stStat.u32ThreadNumMax = static_cast<uint32_t>((stBase.llThreadNum > 0) ? stBase.llThreadNum : 0);
    }

    m_thTick = std::thread(&CClassroomEngine::tickLoop, this);

    dlog_info("classroom engine 已启动: version=%lld enable=%d rss=%lldKB avail=%lldKB mmz=%lldKB threads=%lld",
              static_cast<long long>(spSnap->llVersion), spSnap->bEnable ? 1 : 0,
              static_cast<long long>(stBase.llVmRssKb), static_cast<long long>(stBase.llMemAvailableKb),
              static_cast<long long>(stBase.llMmzUsedKb), static_cast<long long>(stBase.llThreadNum));
    rstrErr.clear();
    return true;
}

void CClassroomEngine::deinit()
{
    stopModules();

    std::lock_guard<std::mutex> stLock(m_mtxStat);
    m_stStat.bRunning = false;
}

void CClassroomEngine::stopModules()
{
    /* 幂等：deinit 与 init 失败回滚共用一条路径，析构再调一次也不能崩 */
    {
        std::lock_guard<std::mutex> stLock(m_mtxTick);
        m_bRunning = false;
    }
    m_cvTick.notify_all();
    if (m_thTick.joinable())
    {
        m_thTick.join();
    }

    /* 释放顺序与申请顺序严格相反：先断生产（调度器/池），再回收资源（租约/裁剪/总线）。
       反过来的话 worker 可能在池已停之后还去 acquire，白等一轮还会留下半截状态 */
    /* OCR 那条线程排在调度器之前摘 */
    m_stOcrPP.deinit();
    m_strOcrDictApplied.clear();

    if (m_spSched)
    {
        m_spSched->stop();
        m_spSched.reset();
    }

    m_mapHeld.clear();          /* 先放租约，否则 nUsers>0 会挡住 unloadAll */
    m_mapWarmState.clear();
    m_stPool.unloadAll();
    m_stPool.stop();

    {
        std::lock_guard<std::mutex> stLock(m_mtxCrop);
        m_mapCrop.clear();
    }

    m_stFeatureBus.deinit();
    m_stBus.deinit();

    /* P3 两块跟着停：聚合中心的环是 200KB 级常驻（480 样本 + 256 事件），
       不起节拍线程却把内存留在手里，等于「关了功能但账没还」。
       滞留环只关 fd、不动文件：积压是掉电之外唯一还能救回来的数据，
       删它必须是显式动作（clear），不能跟着一次 deinit 顺手发生 */
    m_stData.deinit();
    m_stMerger.reset();
    m_stSpool.deinit();
    m_stSpoolParam = CSpoolParam_S();
    m_bSpoolApplied = false;
    m_bUploadBroken = false;

    m_u32WantedMask = 0;
    m_llAppliedVersion = -1;
    m_bNeedRotate = false;
    m_llHeavyUntilMs = 0;
    {
        /* 节流基线清零：否则重新 init 后第一帧会被上一轮运行的时刻吃掉。
           该成员由取流线程在同一把锁下写，这里也必须持锁——32 位上 8 字节变量会撕裂读写 */
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_llLastFrameAcceptMs = 0;
    }

    /* 看护状态一起复位：重新 init 时按新路径重新取基线，不能拿上一次的 mtime 判"没改过" */
    m_strConfigPath.clear();
    m_llLastConfigCheckMs = 0;
    m_bConfigFilePresent = false;
    m_llConfigMTimeSec = 0;
    m_llConfigSize = 0;
}

bool CClassroomEngine::isRunning() const
{
    std::lock_guard<std::mutex> stLock(m_mtxTick);
    return m_bRunning;
}

bool CClassroomEngine::onFrame(const void *pNv12, int64_t nLen, int nW, int nH, uint64_t u64PtsUs)
{
    ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap || !spSnap->bEnable)
    {
        /* 总开关关闭时连 memcpy 都不做" */
        return false;
    }

    const int64_t llNowMs = classroomNowMs();

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64FramesIn;

        if (nW != spSnap->nSrcWidth || nH != spSnap->nSrcHeight)
        {
            /* 总线只按「长度不小于一帧」判，来帧更大时会静默截拷，画面右下缺一块比丢帧糟得多 */
            ++m_stStat.u64FramesRejected;
            if (1 == m_stStat.u64FramesRejected || 0 == m_stStat.u64FramesRejected % 250)
            {
                dlog_error("classroom engine: 帧 %dx%d != 配置 %dx%d，丢弃 (rejected=%llu)",
                           nW, nH, spSnap->nSrcWidth, spSnap->nSrcHeight,
                           static_cast<unsigned long long>(m_stStat.u64FramesRejected));
            }
            return false;
        }

        /* 节流而非丢 */
        if (m_llLastFrameAcceptMs > 0 &&
            llNowMs - m_llLastFrameAcceptMs < CLASSROOM_FRAME_MIN_INTERVAL_MS)
        {
            ++m_stStat.u64FramesThrottled;
            return false;
        }
        m_llLastFrameAcceptMs = llNowMs;
    }

    ++m_u64FrameSeqGen;
    return m_stBus.publish(pNv12, nLen, u64PtsUs, static_cast<uint32_t>(m_u64FrameSeqGen));
}

bool CClassroomEngine::reloadConfigFromFile(const std::string &strPath, std::string &rstrErr)
{
    return m_stConfig.reloadFromFile(strPath, rstrErr);
}

bool CClassroomEngine::reloadConfigFromText(const std::string &strJson, std::string &rstrErr)
{
    return m_stConfig.reloadFromText(strJson, "push", rstrErr);
}

ConfigPtr CClassroomEngine::config() const
{
    return m_stConfig.current();
}

std::string CClassroomEngine::configText() const
{
    return m_stConfig.lastConfigText();
}

CEngineStat_S CClassroomEngine::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtxStat);
    CEngineStat_S stOut = m_stStat;
    /* 运行时长是派生量，没有"最后一次写入的值"这种东西，只能在读数时算。
       它此前从未被写：statusJson 的 up_ms 恒为 0，而运维看到 0 的第一反应是"进程刚重启"，
       偏偏同一条心跳的 up= 是真数——两行读数互相打脸时，人倾向于相信那个看着像 bug 的 */
    stOut.llUpMs = classroomNowMs() - m_llBootMs;
    return stOut;
}

CropUnitPtr CClassroomEngine::cropUnitOf(const std::string &strKey) const
{
    std::lock_guard<std::mutex> stLock(m_mtxCrop);

    auto it = m_mapCrop.find(strKey);
    return (it == m_mapCrop.end()) ? CropUnitPtr() : it->second;
}

void CClassroomEngine::tickLoop()
{
    setThreadName("ClsTick");

    std::unique_lock<std::mutex> stLock(m_mtxTick);
    while (m_bRunning)
    {
        m_cvTick.wait_for(stLock, std::chrono::milliseconds(CLASSROOM_TICK_PERIOD_MS),
                          [this] { return !m_bRunning; });
        if (!m_bRunning)
        {
            break;
        }

        stLock.unlock();
        tickOnce(classroomNowMs());   /* 一拍里可能加载模型（百 ms 级），绝不持锁做 */
        stLock.lock();
    }
}

void CClassroomEngine::tickOnce(int64_t llNowMs)
{
    watchConfigFile(llNowMs);

    ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64Ticks;
    }

    if (spSnap->llVersion != m_llAppliedVersion)
    {
        applySnapshot(spSnap, llNowMs);
    }

    guardSample(llNowMs);
    rotateHeavyIfDue(llNowMs, *spSnap);
    warmGroups(*spSnap, llNowMs);

    perceiveStep(spSnap, llNowMs);

    /* 规则判定排在感知之后：同一拍里就能拿到刚发布的那张特征帧，省一整拍的事件延迟 */
    ruleStep(llNowMs);

    /* 板书排在判定之后 */
    boardStep(llNowMs);

    /* 考勤喂观测排在群体采样之前：本拍的到课人数要进同一份报表，
       错开一拍的话"下课那份报表"永远少最后一个人的在场增量 */
    attendStep(llNowMs);

    /* 群体采样紧跟判定：本拍刚生效/刚结束的状态要进这一条样本，
       错开一拍的话四小时的课里每个指标都系统性晚 5s，且时长积分会漏掉头尾各一段 */
    dataStep(llNowMs);
    /* 平台统计出包紧跟在采样之后 */
    statsStep(llNowMs);
    tryReport(llNowMs);

    metrics(llNowMs);
}

void CClassroomEngine::perceiveStep(const ConfigPtr &spSnap, int64_t llNowMs)
{
    if (!m_fnPerceive || m_enLevel >= GuardLevel_E::GL_EMERGENCY)
    {
        /* P1 之前没有检测器、应急档不让吃内存：这两种情况下本拍就是不产出。
           这里刻意不伪造特征帧 —— 空帧会被下游读成「这屏 0 人」，
           而规则引擎有自己的断档判定，它才是「没证据」这件事的正确出口 */
        return;
    }

    const int32_t nIntervalMs = spSnap->stSample.nHeadIntervalMs * stretchOfLevel(m_enLevel) / 100;
    if (!beatDue(m_llLastHeadBeatMs, llNowMs, nIntervalMs))
    {
        return;
    }

    const FramePtr spPixel = m_stBus.latest();
    if (!spPixel || spPixel->u32Seq == m_u32PixelSeqHandled)
    {
        return;
    }
    m_u32PixelSeqHandled = spPixel->u32Seq;

    FeatureFramePtr spOut = m_stFeatureBus.acquire();
    if (!spOut)
    {
        return;
    }

    spOut->stGeom = spPixel->stGeom;
    spOut->u64PtsUs = spPixel->u64PtsUs;
    spOut->u32PixelSeq = spPixel->u32Seq;
    spOut->llMonotonicMs = llNowMs;
    spOut->bPoseResident = groupHeld(ModelGroup_E::GROUP_POSE);
    spOut->bFaceResident = groupHeld(ModelGroup_E::GROUP_FACE);
    /* 上一拍规则层记下的「谁在等姿态证据」随帧进感知层（P0-1）：ruleStep 排在本步之后，
       所以表里的永远是上一拍的结论——与 panActivePrev / phoneRecheckIds 同一条
       「晚一拍换无并发」的账。抄进帧而不是让感知层回读规则引擎，是因为感知层
       只该看见输入（帧 + 配置），不该反手摸判定层；这也让回放复现挑选结果成为可能 */
    {
        int32_t nUrgNum = 0;
        const CPoseUrgencyHint_S *pstUrg = m_stRule.poseUrgencyTable(nUrgNum);
        spOut->setPoseUrgency(pstUrg, nUrgNum);
    }

    bool bOk = false;
    try
    {
        bOk = m_fnPerceive(spPixel, spSnap, spOut);
    }
    catch (const std::exception &stEx)
    {
        /* 感知层是算法代码的地盘，异常必须在边界挡住：节拍线程一死，整个子系统会变成
           "帧还在进、结果永不出"的静默僵死，比崩掉更难发现 */
        dlog_error("classroom engine: perceive 抛异常 %s", stEx.what());
        bOk = false;
    }

    if (bOk)
    {
        m_stFeatureBus.publish(spOut);   /* 句柄被 publish 置空，槽位转为对外可见 */
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64PerceiveRounds;
    }
    else
    {
        /* spOut 在此析构，槽位原样回到总线，未发布的内容对下游不可见 */
        /* 基础组不在板 = 这一拍是关档边沿上那唯一一帧在途，不是稳态故障。
           问的是模型池，必须在拿统计锁之前问完 */
        const bool bEdge = !groupHeld(ModelGroup_E::GROUP_BASE);
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64PerceiveFail;
        if (bEdge)
        {
            ++m_stStat.u64PerceiveFailEdge;
        }
    }
}

void CClassroomEngine::ruleStep(int64_t llNowMs)
{
    if (m_enLevel >= GuardLevel_E::GL_EMERGENCY)
    {
        /* 应急档不喂样也不取数：已生效的事件在 applyGuardLevel 里一次性收过尾了，
           这里再喂等于让降级现场继续产新事件——而那些事件没人上报，只会堆满环再丢最旧 */
        return;
    }

    /* 每拍必调，无新帧也调：断档收尾靠的就是这个无条件调用（见 CRuleEngine::run） */
    m_stRule.run(m_stFeatureBus.latest(), llNowMs);

    /* 主体翻转的"封口"要覆盖两种时序：①翻转当拍收尾的 END 由规则引擎打标；
       ②证据先断、门自己收的 END 已经在合并窗口里挂着——那一条由这里补标 */
    int32_t nFlipNum = 0;
    const int32_t *panFlipIds = m_stRule.flippedIds(nFlipNum);
    for (int32_t i = 0; i < nFlipNum; ++i)
    {
        m_stMerger.sealPending(panFlipIds[i]);
    }

    CBehaviorEvent_S stEvent;
    while (m_stRule.pollEvent(stEvent))
    {
        reportEvent(stEvent);
    }

    /* 合并器的窗口到点放行也靠这个无条件调用：END 挂起后如果再没有新事件进来，
       没有这一拍一次的 tick 它就一直挂着（挂起表被占满，后面的事件被挤出去） */
    m_stMerger.tick(llNowMs);
    drainMerger();

    /* 板书第二条触发源：把「当前教师是否站在黑板区域」喂给采集时机层 */
    const int32_t nTeaId = m_stRule.subjects().teacherId();
    CSubjectFact_S stTeaFact;
    const bool bTeaInBoard = (nTeaId >= 0 && m_stRule.subjects().factOf(nTeaId, stTeaFact)) &&
                             stTeaFact.bInBoard;
    m_stBoard.notifyTeacherStay(bTeaInBoard, llNowMs);
}

void CClassroomEngine::reportEvent(const CBehaviorEvent_S &stEvent)
{
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RuleEvents;
    }

    /* 平台统计的段取**合并前**的原始边沿 */
    CStatsEventObs_S stSegObs;
    stSegObs.bSegment = true;
    m_stStatsHub.onEvent(stEvent, stSegObs, classroomNowMs(), reportWallMs());

    /* 先进合并器（P0-2）：翻转接手产生的「END(旧ID)+BEGIN(新ID)」在这里并成一段，
       被吞掉的事件不再出现在下游（聚合环、平台报文、emitEvent 的日志），
       但会在合并器自己的日志里留一行 swallow——原始产量与上报产量的差就是它 */
    m_stMerger.feed(stEvent);
    drainMerger();
}

void CClassroomEngine::drainMerger()
{
    CBehaviorEvent_S stOut;
    while (m_stMerger.pollEvent(stOut))
    {
        emitEvent(stOut);
    }
}

bool CClassroomEngine::eventLogAdmit(int64_t llNowMs, uint64_t &ru64Suppressed)
{
    ru64Suppressed = 0;
    if (m_llEvtLogWinMs <= 0 || llNowMs - m_llEvtLogWinMs >= CLASSROOM_EVT_LOG_WIN_MS)
    {
        /* 翻页：把上一窗的抑制量交给调用方补摘要，然后开新一窗 */
        ru64Suppressed = m_u64EvtLogHeld;
        m_llEvtLogWinMs = llNowMs;
        m_u32EvtLogWin = 0;
        m_u64EvtLogHeld = 0;
        return true;
    }
    if (m_u32EvtLogWin < CLASSROOM_EVT_LOG_MAX)
    {
        ++m_u32EvtLogWin;
        return true;
    }
    ++m_u64EvtLogHeld;
    return false;
}

void CClassroomEngine::deliverEventPush(const CBehaviorEvent_S &stEvent)
{
    /* 事件通道二选一（setEventAlarmFn 的注释里有理由）。告警通道优先：
       它是用户口径里的正式通道，装上之后不再往 JSON 流里发第二份 */
    if (m_fnEventAlarm)
    {
        /* 只有 207/209 的班级级 BEGIN 承载「让平台去问教师机」的语义；
           其余事件（101~106、210、逐人行的旧口径）只进周期报表，不过这里不算"漏推"，
           所以不记 skip——把每一条学生事件都记成一次跳过，这个计数就没法读了 */
        if (!isImmediateClassEvent(stEvent))
        {
            return;
        }

        /* 墙钟档口由这里取（与 deliverReport 同一口径）：通道只认秒级偏移，
           但结构体里的 llTimestampMs 必须是墙钟，不能在通道那边各取一次 */
        const int64_t llWallMs = reportWallMs();
        ++m_u64EvtAlarmAttempt;

        bool bOk = false;
        try
        {
            bOk = m_fnEventAlarm(stEvent, llWallMs);
        }
        catch (const std::exception &stEx)
        {
            dlog_warn("classroom event alarm: 异常 %s", stEx.what());
        }
        catch (...)
        {
            dlog_warn("classroom event alarm: 未知异常");
        }

        if (bOk)
        {
            ++m_u64EvtAlarmOk;
        }
        else
        {
            ++m_u64EvtAlarmFail;
            /* 只在 BEGIN 打一行：207/209 的开事件本来就不密，这行是
               「平台为什么没来问教师机」在设备侧能留下的唯一直接证据 */
            dlog_warn("classroom event alarm: %s 未送达（无客户端/服务不可用），全集仍进周期报表",
                      behaviorName(stEvent.enBehavior));
        }
        return;
    }

    if (!m_fnUpload)
    {
        /* 没接通道：周期报表照旧带这条事件，这里不出第二份账 */
        ++m_u64EvtPushSkip;
        return;
    }
    /* 成对取墙钟与单调钟（与 deliverReport 同一口径）：报文里的时刻是
       「单调读数 → 墙钟」的换算，两个读数之间隔多久，这份报文就偏移多久 */
    const std::string strPayload =
        eventPushJson(stEvent, reportWallMs(), classroomNowMs());
    ++m_u64EvtPush;
    try
    {
        m_fnUpload(SPOOL_TYPE_EVENT, strPayload);
    }
    catch (const std::exception &stEx)
    {
        dlog_warn("classroom event push: 异常 %s", stEx.what());
    }
    catch (...)
    {
        dlog_warn("classroom event push: 未知异常");
    }
}

void CClassroomEngine::emitEvent(const CBehaviorEvent_S &stEvent)
{
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RuleEvents;
    }

    /* P3 的群体统计与上报在这一点接管（聚合、去重、断网滞留都归它）。
       眼下先一条一行落日志：这既是 P2 的上机验收手段（事件对不对、时长准不准，
       只看这一行就能判），也是聚合层出问题时唯一的对照物——所以别嫌它啰嗦。
       但不能让它无限冲量：抖动风暴时这一行能把日志环灌满，窗口内超出
       配额的只计数，翻页补一条抑制摘要——明细在滞留环与周期报表里都有 */
    uint64_t u64Suppressed = 0;
    if (eventLogAdmit(stEvent.llMs, u64Suppressed))
    {
        if (u64Suppressed > 0)
        {
            dlog_info("classroom event: %llu 条事件行在最近 %ds 窗口内被压制",
                      static_cast<unsigned long long>(u64Suppressed),
                      static_cast<int>(CLASSROOM_EVT_LOG_WIN_MS / 1000));
        }
        dlog_info("classroom event: %s %s 目标=%d 座位=%d 已持续=%lldms 开机毫秒=%lld 流水=%u 头框=[%.0f,%.0f,%.0f,%.0f]",
                  stEvent.bBegin ? "开始" : "结束",
                  behaviorCnName(stEvent.enBehavior),
                  stEvent.nPersonId, stEvent.nSeatId,
                  static_cast<long long>(stEvent.llHeldMs),
                  static_cast<long long>(stEvent.llMs),
                  stEvent.u32FeatureSeq,
                  stEvent.stHeadBox.fX1, stEvent.stHeadBox.fY1,
                  stEvent.stHeadBox.fX2, stEvent.stHeadBox.fY2);
    }

    /* 板书采集的触发就挂在这条结束边上（教师面向黑板 → 转回面向学生） */
    if (!stEvent.bBegin && Behavior_E::TEA_BOARD_WRITE == stEvent.enBehavior)
    {
        m_stBoard.notifyEpisodeEnd(stEvent.llMs);
    }

    /* 转进聚合中心：环满覆盖最旧并计数，所以这里**不需要**判"放得下放不下"。
       上面那行日志保留 —— 它是聚合层出错时唯一的对照物 */
    CBehaviorEvent_S stStored = stEvent;
    stStored.u64EventSeq = m_stData.pushEvent(stEvent);

    /* 平台计数（0x6200）走合并后的事件：一次持续行为只计一次，跟踪 ID 翻转被并掉的
       END+BEGIN 不再各计一遍——「规避相同累加次数」在这一层落实，平台无需自己去重计数 */
    CStatsEventObs_S stCntObs;
    stCntObs.bCount = true;
    m_stStatsHub.onEvent(stStored, stCntObs);

    /* 即时事件通道：事件发生那一拍就发 kind=event 单事件报文，
       不等最长 60s 的周期报表。周期报表仍是权威全集（断网滞留/重传语义不变），
       两边同带 ev_seq 只用于对账；去重按通道各算——报表流内先到为准，告警是触发器、
       不因报表已有同号而被抑制。没进环（u64EventSeq==0）的那份没法对账，照发 */
    deliverEventPush(stStored);
}

void CClassroomEngine::attendStep(int64_t llNowMs)
{
    const ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap || !spSnap->stAttend.bEnable || m_enLevel >= GuardLevel_E::GL_EMERGENCY)
    {
        return;
    }

    /* 到点重读底库。放在取帧之前是有意的  */
    if (m_spGallery->poll(llNowMs))
    {
        dlog_info("classroom engine: 底库已重载, enroll=%d", m_spGallery->enrollNum());
    }
    const ConstFeatureFramePtr spFrame = m_stFeatureBus.latest();
    if (!spFrame || 0 == spFrame->u32Seq || spFrame->u32Seq == m_u32AttendSeq)
    {
        return;    /* 同一张帧喂两次不会改任何数（增量是 0） */
    }
    if (llNowMs - spFrame->llMonotonicMs > CLASSROOM_FEATURE_TTL_MS)
    {
        /* 过期帧不喂：取流断了半小时再回来，中间那半小时既不该记成"全班缺席"也不该记成
           "全班在场"——账本上这段时间就是没有观测，到课时长因此少半小时，那是真话 */
        return;
    }
    m_u32AttendSeq = spFrame->u32Seq;

    if (!m_bAttendOpen)
    {
        /* 开一节新账。应到传 -1 = 不用"本节"这一级覆盖，让账本回落到配置里的 expected_num：
           课表级的应到要等课表通道落地才有来源，现在没有就是没有 */
        m_stAttend.beginSession(-1);
        m_bAttendOpen = true;
    }

    const int32_t nCap = static_cast<int32_t>(spFrame->vecPersons.size());
    const int32_t nRowNum = (spFrame->nPersonNum < nCap) ? spFrame->nPersonNum : nCap;
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPersonFeature_S &stFeat = spFrame->vecPersons[static_cast<size_t>(nRow)];

        /* 「考勤检测区域」（attendance.region_ids，空 = 全画面） */
        bool bAttendRegionCfgErr = false;
        if (!m_stRule.regions().inAnyRegion(spSnap->stAttend.vecRegionIds, 0.5f,
                                            stFeat.stHeadBox, bAttendRegionCfgErr))
        {
            if (bAttendRegionCfgErr)
            {
                /* 区域配错（引用了不存在的块）时**不记账也不静默**：整份配置在校验期
                   就会被拒，能走到这里说明是运行期换了表。计数一次就够，别每拍刷 */
                if (0 == m_u64AttendRegionCfgErr.load(std::memory_order_relaxed))
                {
                    dlog_warn("classroom engine: 考勤区域引用不可用, 暂时不按区域过滤");
                }
                m_u64AttendRegionCfgErr.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                m_stAttendRegionSkip.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
        }

        CAttendObs_S stObs;
        stObs.nTrackId = stFeat.nPersonId;
        /* 人头上限 */
        stObs.nFramePersons = nRowNum;
        /* 座位号有两个候选来源，而它们是**两套编号**，所以这里只能选一套、不能"有就用没有就用别的" */
        stObs.nSeatId = m_stPrior.param().bEnable ? m_stPrior.seatIdOf(stFeat.nPersonId)
                                                  : stFeat.nSeatId;
        /* 位置证据：与座位热力/轨迹同一个换算出口 dstToNormSrc，不在这里另乘缩放系数 */
        {
            const CPointf_S stCenter = stFeat.stHeadBox.center();
            CPointf_S stNorm;
            if (stFeat.stHeadBox.area() > 0.0f && spFrame->stGeom.dstToNormSrc(stCenter, stNorm))
            {
                stObs.fNormX = stNorm.fX;
                stObs.fNormY = stNorm.fY;
                const int nDstH = spFrame->stGeom.dstHeight();
                stObs.fHeadNorm = (nDstH > 0)
                                      ? (stFeat.stHeadBox.height() / static_cast<float>(nDstH))
                                      : -1.0f;
            }
        }
        /* 时刻取**帧的产出时刻**而不是本拍时刻：两次感知之间可能隔着一整拍的降级耗时，
           用拍时刻会让每条增量抖到拍长那么大，而到课时长正是这些增量累出来的 */
        stObs.llFrameMs = spFrame->llMonotonicMs;
        if (stFeat.faceUsable(spFrame->llMonotonicMs, CLASSROOM_FEATURE_TTL_MS))
        {
            stObs.nFaceW = static_cast<int32_t>(stFeat.stFaceBox.width());
            stObs.nFaceH = static_cast<int32_t>(stFeat.stFaceBox.height());
        }
        /* 比对结论按跟踪 ID 取，一次一份：取走即清是账本那两道票门的前提（见 CFaceMatchTakeFn）。
           取不到就让 bMatchTried 留在 false——那是"这拍没比过"，与"比过而没命中"
           （true + bHit=false）在账本里是两条不同的计数：前者查排程与底库，后者查门限。
           没注入取函数（未接感知）时这里恒为 false，读起来与"G_FACE 空窗"同一种可见的缺失 */
        if (stObs.nTrackId >= 0 && m_fnFaceTake)
        {
            stObs.bMatchTried = m_fnFaceTake(stObs.nTrackId, stObs.stMatch);
        }
        m_stAttend.observe(stObs);
    }
}

void CClassroomEngine::dataStep(int64_t llNowMs)
{
    const ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap || m_enLevel >= GuardLevel_E::GL_EMERGENCY || !m_stData.isReady())
    {
        /* 聚合中心没建起来（参数非法）时不采样：宁可报表段一直是"没有样本"这条可见的缺失，
           也不要拿默认容量悄悄跑一份"看起来正常"的账本 */
        return;
    }
    if (!beatDue(m_llLastSampleMs, llNowMs, spSnap->stSample.nGroupIntervalMs))
    {
        return;
    }

    const ConstFeatureFramePtr spFrame = m_stFeatureBus.latest();
    if (!spFrame)
    {
        return;    /* 一条特征帧都没有 = 没有证据。此时不采"0 人"样本，那是把"没看见"报成"没人" */
    }
    if (llNowMs - spFrame->llMonotonicMs > CLASSROOM_FEATURE_TTL_MS)
    {
        /* 总线只负责"取到最新那张"，新鲜度是消费者的事（latest() 里没有期限，也不该有）。
           行内的骨骼/人脸证据自己有 TTL，唯独"这一屏有 N 个人"这一项不会过期 ——
           所以不挡这一条的话：总开关一关、或者取流断半小时，账本会拿着最后一帧的
           人数继续积 在场时长，报表上就是一节"坐满人却没人被看着"的课。
           停下来留下的这段时间由聚合中心的断档计数（gap）自己记账，不在这里再造一个数 */
        return;
    }

    const int32_t nCap = static_cast<int32_t>(spFrame->vecPersons.size());
    const int32_t nRowNum = (spFrame->nPersonNum < nCap) ? spFrame->nPersonNum : nCap;
    if (nRowNum <= 0)
    {
        /* 空场也折一次（0 行）：不记秒数，但那笔"多久没看到就算离场"的账要照走，
           否则课间十分钟足以让上一节所有人的格子僵在池子里（见 CSeatHeat::fold） */
        seatStep(spFrame, 0, spSnap->stSample.nGroupIntervalMs, spSnap->llVersion);
        CGroupMetric_S stEmpty;
        if (buildGroupMetric(*spFrame, nullptr, 0, llNowMs, groupPeriodGateOf(*spSnap), stEmpty))
        {
            m_stData.pushSample(stEmpty);    /* 空场是一条有效样本（平台要看到 0） */
            /* 时段层同样要吃这一条。空场不喂的后果是**下一段会盖住这段没人时间**：
               状态机没被告知"现在什么都不是"，于是把最后一次承认的讲授一路延长，
               空教室在报表上就成了一段 40 分钟的知识教授（本轮设备上读 fed=0 抓到的） */
            sceneStep(stEmpty, spSnap, llNowMs);
            statsFeedStep(spSnap, stEmpty, spFrame, 0, llNowMs);
        }
        return;
    }

    /* 逐行取"当前生效了哪些行为"。分母（有没有证据）在这里定，比率在 group_stats 里算：
       两侧读同一个**帧上自带的**节拍（poseTtlMs），不会出现"感知层说骨骼过期、这里又说有证据"。
       不许在这里拿配置重拼一遍：配置里的 max_pose_persons 是希望值，
       感知层真正跑出的产能写在帧上，读配置就等于替它猜 */
    const int32_t nPoseTtlMs = spFrame->poseTtlMs();
    const int64_t llFrameMs = spFrame->llMonotonicMs;

    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPersonFeature_S &stRow = spFrame->vecPersons[nRow];
        CRowStatInput_S &stIn = m_vecRowInput[static_cast<size_t>(nRow)];
        stIn = CRowStatInput_S();

        stIn.nPersonId = stRow.nPersonId;

        /* 主体、区域、位置三组读数都在 fillRowSubject 里定：它只认仲裁器的事实，
           不认帧行的 enSubject（那一路恒为 STUDENT）。挪出引擎不是为了好看——
           dataStep 要整台设备才跑得起来，而这条选错证据来源的坑恰好是"日志一切正常、
           报表里教师永远是 0"，只有纯函数那一层能被主机用例钉住 */
        CSubjectFact_S stFact;
        fillRowSubject(stIn, m_stRule.subjects().factOf(stRow.nPersonId, stFact) ? &stFact : nullptr,
                       stRow.stHeadBox, spFrame->stGeom, spSnap->stScene.fGroundShiftHeads);
        stIn.bPoseEvidence = stRow.poseUsable(llFrameMs, nPoseTtlMs);
        stIn.bFaceEvidence = stRow.faceUsable(llFrameMs, CLASSROOM_FEATURE_TTL_MS);

        /* 列序一律走 behaviorAtColumn：这里不重抄 101~112 / 201~213，
           枚举以后加行为时这段循环自动跟上，不会出现"门表有、位图没有"的半更新 */
        for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
        {
            Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
            if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
            {
                continue;
            }
            const int nStuBit = studentMaskBit(enBehavior);
            const int nTeaBit = teacherMaskBit(enBehavior);
            /* "判过"与"生效"分开记：时段门的分母要的是前者。放在 isActive 之前，
               因为没生效的那些恰恰是分母最需要知道"判过没有"的部分 */
            if (nStuBit >= 0 && m_stRule.isKnown(stRow.nPersonId, enBehavior))
            {
                stIn.u32StuKnownMask |= (1u << nStuBit);
            }
            if (!m_stRule.isActive(stRow.nPersonId, enBehavior))
            {
                continue;
            }
            if (nStuBit >= 0)
            {
                stIn.u32StuMask |= (1u << nStuBit);
            }
            else if (nTeaBit >= 0)
            {
                stIn.u32TeaMask |= (1u << nTeaBit);
            }
        }
    }

    CGroupMetric_S stMetric;
    if (buildGroupMetric(*spFrame, m_vecRowInput.data(), nRowNum, llNowMs,
                         groupPeriodGateOf(*spSnap), stMetric))
    {
        m_stData.pushSample(stMetric);
    }
    /* 热力折样排在群体指标之后：它读的就是这一批 m_vecRowInput，同一拍、同一个步长，
       两边算出的"这一拍代表 5 秒"才是同一个 5 秒 */
    seatStep(spFrame, nRowNum, spSnap->stSample.nGroupIntervalMs, spSnap->llVersion);
    stuTrackStep(nRowNum, llNowMs);
    sceneStep(stMetric, spSnap, llNowMs);
    statsFeedStep(spSnap, stMetric, spFrame, nRowNum, llNowMs);
}

void CClassroomEngine::statsFeedStep(const ConfigPtr &spSnap, const CGroupMetric_S &stMetric,
                                     const ConstFeatureFramePtr &spFrame, int32_t nRowNum,
                                     int64_t llNowMs)
{
    CStatsSample_S stIn;
    stIn.llMonoMs = llNowMs;
    stIn.llWallMs = reportWallMs();
    stIn.pstMetric = &stMetric;

    /* 人群构造与"谁算离课"是统计层的口径：引擎只递行表与特征帧 */
    CStatsPerson_S aPersons[CLASSROOM_MAX_PERSONS];
    const int32_t nPersons = buildStatsPersons(
        m_vecRowInput.data(), nRowNum,
        spFrame ? spFrame->vecPersons.data() : nullptr,
        spFrame ? static_cast<int32_t>(spFrame->vecPersons.size()) : 0,
        aPersons, CLASSROOM_MAX_PERSONS);
    stIn.pstPersons = aPersons;
    stIn.nPersonNum = nPersons;

    stIn.bAttendEnable = (spSnap && spSnap->stAttend.bEnable);
    if (spSnap && stIn.bAttendEnable)
    {
        stIn.nPresentNum = m_stAttend.presentNum();
        stIn.nNamedNum = m_stAttend.namedNum();
        stIn.nExpectedNum = m_stAttend.expectedNum();
        stIn.nAbsentNum = m_stAttend.absentNum();

        const int64_t llLeaveMs =
            static_cast<int64_t>(spSnap->stStats.nLeaveAbsentSec) * 1000;
        m_vecAttendScratch.clear();
        const int32_t nRecNum = m_stAttend.records(m_vecAttendScratch);
        stIn.nLeftMid = countLeftMid(m_vecAttendScratch.data(), nRecNum, llNowMs, llLeaveMs,
                                     m_stData.session().llLastPresentMs);
    }
    stIn.bPatrolKnown = (spSnap && spSnap->behaviorOn(Behavior_E::TEA_PATROL));

    m_stStatsHub.onSample(stIn);
}

void CClassroomEngine::statsStep(int64_t llNowMs)
{
    /* 墙钟每拍取一次（clock_gettime 量级）：统计头里的 llWallMs 与段的换算必须同源，
       不能让状态机自己再取一次时钟——主机用例也就无从复现 */
    m_stStatsHub.tick(llNowMs, reportWallMs());
}

void CClassroomEngine::sceneStep(const CGroupMetric_S &stMetric, const ConfigPtr &spSnap, int64_t llNowMs)
{
    if (!spSnap)
    {
        return;
    }
    /* 板书边沿的游标先推进，与开不开无关：关着的这段时间里攒下的边沿，
       重新打开那一刻会凭空算成"刚写过板" */
    const uint64_t u64EdgeNow = m_stBoard.stat().u64Episode;
    const bool bBoardEdge = (u64EdgeNow > m_u64BoardEdgeSeen);
    m_u64BoardEdgeSeen = u64EdgeNow;

    /* 场景时段排在群体样本之后：它的全部输入就是刚算出来的这一条（生效人数、分母、
       三态时段），自己去门表再捞一遍就会读到同一拍的另一半，两边差一拍在段边界上看得见。
       **空场样本同样走这里**：漏掉空场等于让上一段一路延长到下次有人为止 */
    if (!spSnap->stScene.bEnable)
    {
        return;
    }
    CSceneFact_S stFact;
    stFact.llNowMs = llNowMs;
    stFact.nEvidenceNum = stMetric.nStuEvidence;
    stFact.bTeacherPresent = (stMetric.nTeacherNum > 0);
    stFact.bTeachingOn = (m_stRule.activeNum(Behavior_E::TEA_TEACHING) > 0);
    stFact.bInteractOn = (m_stRule.activeNum(Behavior_E::TEA_INTERACT) > 0);
    stFact.bBoardEdge = bBoardEdge;
    stFact.nRaiseNum = m_stRule.activeNum(Behavior_E::STU_RAISE_HAND);
    stFact.nHeadDownNum = m_stRule.activeNum(Behavior_E::STU_HEAD_DOWN);
    stFact.nDiscussNum = m_stRule.activeNum(Behavior_E::STU_DISCUSS);
    /* 上讲台与演示合并成一个计数：时段要的是"有学生在台上"，两者谁成立都一样。
       两个都生效时同一个人会被计两次，而那只会让 nPresentNum 偏大一个，
       它在下面只跟 0 比（有没有人），不参与任何比例 */
    stFact.nPresentNum = m_stRule.activeNum(Behavior_E::STU_ON_PODIUM) +
                         m_stRule.activeNum(Behavior_E::STU_PRESENT);
    stFact.nListenPeriod = triToInt(stMetric.enListenPeriod);
    stFact.nDiscussPeriod = triToInt(stMetric.enDiscussPeriod);
    m_stScene.feed(stFact);
}

void CClassroomEngine::seatStep(const ConstFeatureFramePtr &spFrame, int32_t nRowNum,
                                int32_t nStepMs, int64_t llConfVer)
{
    if (!m_stSeat.param().bEnable || !spFrame)
    {
        /* 关掉时连网格都不重建：这张图的常驻是 1KB，而它此刻没有任何读者。
           重新打开时第一拍就会把图画出来（gridReady 为 false 触发下面的重试） */
        return;
    }

    /* 两个判据各挡一种情况。
       没建成：建图要同时有区域表和帧几何，而这两件事凑齐的时刻谁也不保证——装机顺序完全可能
       是"先起流、后画区"，甚至反过来，所以每拍都要能重试。
       版本变了：判据不能只用"还没建成"（那样建成一次就永远不再建）：席区被重画时格子号与
       外接框都换了，留着旧图等于把这张图铺到画面上错的那一块，而设备侧看着一切正常
       （真机上就是这么复现的：区域从整幅缩到左下四分之一之后，box 仍是 [0,0,1,1]、out 仍是 0）。
       用配置版本号判，与 CRuleEngine::ensureRegions 同一个信号——两侧读的是同一张区域表，
       换版不同步就会出现"区域已按新的算、格子还在按旧的铺"。
       代价是每次换配置多建一次图（≤16 次矩形换算），而 seat_heat 侧对"图没变"是幂等的：
       sameGrid 不清账、不打日志 */
    if (!m_stSeat.gridReady() || m_llSeatGridVer != llConfVer)
    {
        if (m_stSeat.rebuildGrid(m_stRule.regions(), spFrame->stGeom))
        {
            m_llSeatGridVer = llConfVer;
        }
    }

    /* 两端都夹：负数不能靠转成 size_t 处理（一转就是天文数字，等于把整张临时表都当成有效行，
       而那些行里躺着的是上一拍的旧人旧位置） */
    const int32_t nCap = static_cast<int32_t>(m_vecSeatObs.size());
    const int32_t nObsNum = (nRowNum > nCap) ? nCap : ((nRowNum > 0) ? nRowNum : 0);
    for (int32_t nRow = 0; nRow < nObsNum; ++nRow)
    {
        const CRowStatInput_S &stIn = m_vecRowInput[static_cast<size_t>(nRow)];
        CSeatObs_S &stObs = m_vecSeatObs[static_cast<size_t>(nRow)];
        stObs.nPersonId = stIn.nPersonId;
        /* 位置直接取行输入里那份归一化坐标：换算只发生在 fillRowSubject 一处，
           这里再调一次 dstToNormSrc 就是给同一个读数造第二个出口，两处会各自漂移 */
        stObs.fNormX = stIn.fNormX;
        stObs.fNormY = stIn.fNormY;
        stObs.bStudent = stIn.bStudent;
    }
    /* 代次必须在热力折样**之前**取：wipeRows() 一次就 +1，而它是折样过程中可能发生的
       （席区改判到格子真的挪了位置）。之后取到的号会把"这一拍的图"与"上一拍的图"混成
       同一个号——考勤那一条"同格子号 ⇒ 改绑"的规则正是靠号来认人的，混一次就并错一行 */
    const uint64_t u64GridEpoch = m_stSeat.gridEpoch();
    m_stSeat.fold(m_vecSeatObs.data(), nObsNum, nStepMs);

    /* 热力折完之后才轮到座位先验。两件事的先后不是风格问题：
       本层的输入是 `seatOf()` 的结果，而那个结果只在这一次 fold 之后才是"这一拍的格子归属" */
    const int32_t nPriorCap = static_cast<int32_t>(m_vecPriorIn.size());
    const int32_t nPriorNum = (nObsNum > nPriorCap) ? nPriorCap : nObsNum;
    const float fCellW = m_stSeat.cellWidth();
    for (int32_t nRow = 0; nRow < nPriorNum; ++nRow)
    {
        const CRowStatInput_S &stIn = m_vecRowInput[static_cast<size_t>(nRow)];
        CSeatPriorIn_S &stPit = m_vecPriorIn[static_cast<size_t>(nRow)];
        stPit.nPersonId = stIn.nPersonId;
        /* 格子号从热力现取而不是从 m_vecSeatObs 里搬：那张表里存的是**折样之前**的观测，
           本层要的是"这一拍他算坐在哪一格"，两者差一次改判 */
        stPit.nCell = m_stSeat.seatOf(stIn.nPersonId);
        stPit.fNormX = stIn.fNormX;
    }
    /* 空场（nPriorNum=0）也要折：与热力同一理由——让号期是按拍计的，不喂这一拍就是让那笔账
       停在上一节最后一个人身上。整表在格子换代那一刻由 fold 自己清，这里不另设出口 */
    m_stPrior.fold(m_vecPriorIn.data(), nPriorNum, u64GridEpoch, fCellW);

    /* 发号只是第 2 条契约的前半步：本拍之前那些特征帧里，换 ID 的那个人已经按"没见过的人"
       在账本上另起过一行（考勤吃每个特征帧，这张表只在采样拍折一次），不并回去就是
       "号稳了、名单上仍是两个人"。为什么这一句不在 attendStep 里：证据是 fold 当场产的，
       过期不候；而两处都在节拍线程上，账本无需加锁（attendance.h 的线程口径）
       bConsume 取的是与 attendStep 取号处**同一个开关**——关掉时考勤读的是格子号，
       这台机器就该整个跑成 #71 之前的样子，那才是"该不该开"的对照组 */
    applySeatPriorLinks(m_stPrior, m_stPrior.param().bEnable, m_stAttend);
}

void CClassroomEngine::stuTrackStep(int32_t nRowNum, int64_t llNowMs)
{
    /* 门是三态，且只能在引擎这一侧拼出来：bPodiumConfigured 与 bPodiumEmpty 是仲裁器一次遍历
       里的两条读数（subject_arbiter.h 那段 ★ 明确要求分开看），合成一个布尔就会把
       "这台设备不知道讲台在哪"报成"这节课讲台一直有人"——前者要去补区域，后者什么都不用做。
       不在 stu_track 里判：那一层不认识区域表也不认识仲裁器，它只该拿到一个事实 */
    const CSubjectView_S stView = m_stRule.subjects().view();
    const int32_t nGate = !stView.bPodiumConfigured ? STU_TRACK_GATE_NONE
                        : (stView.bPodiumEmpty ? STU_TRACK_GATE_FREE : STU_TRACK_GATE_BUSY);

    /* 两端都夹，理由与 seatStep 同一处：nRowNum 为负转成 size_t 就是天文数字，
       而那些行里躺着的是上一拍的旧人旧位置 */
    const int32_t nCap = static_cast<int32_t>(m_vecStuObs.size());
    const int32_t nObsNum = (nRowNum > nCap) ? nCap : ((nRowNum > 0) ? nRowNum : 0);
    for (int32_t nRow = 0; nRow < nObsNum; ++nRow)
    {
        const CRowStatInput_S &stIn = m_vecRowInput[static_cast<size_t>(nRow)];
        CStuTrackObs_S &stObs = m_vecStuObs[static_cast<size_t>(nRow)];
        stObs.nPersonId = stIn.nPersonId;
        /* 位置取行输入里那一份归一化坐标：换算只发生在 fillRowSubject 一处（§4.8 规则 1），
           这里再调一次 dstToNormSrc 就是给同一个读数造第二个出口，两处会各自漂移 */
        stObs.fNormX = stIn.fNormX;
        stObs.fNormY = stIn.fNormY;
        stObs.bStudent = stIn.bStudent;
        /* 两个理由取的是门表的**结论**（isActive 已经含了开关、门限与最短时长），
           这里不重抄一遍判据：那会出现"事件流说他在站着、轨迹说他没站"这种两份账 */
        const int nStand = studentMaskBit(Behavior_E::STU_STAND_UP);
        const int nBack  = studentMaskBit(Behavior_E::STU_TURN_BACK);
        stObs.bStand = (nStand >= 0) && (0 != (stIn.u32StuMask & (1u << nStand)));
        stObs.bBack  = (nBack >= 0) && (0 != (stIn.u32StuMask & (1u << nBack)));
    }
    /* 空场（nObsNum=0）也照样折：门的状态要跟着这一拍走。
       只在有人时才折的后果是"最后一拍谁在讲台上"被冻结进下一份报表——
       报文里的 gate 就会在空教室之后一直说 busy，而实情是没人可判 */
    m_stStu.fold(m_vecStuObs.data(), nObsNum, nGate, llNowMs);
}

void CClassroomEngine::ocrAttach(const CConfigSnapshot_S &stSnap)
{
    /* 只有空后端可挂的三种情形（关掉、模型表里没有 rec 那一行、上一次装配没起来）都走同一条收尾，
       所以先把"挂空"做成一个动作而不是在三处各写一遍 setBackend */
    const auto attachNull = [this] { m_stBoard.setBackend(&m_stOcrNull); };

    /* 关着就是真的省：那条线程、那张字典表、两块常驻缓冲都不该留着。
       字典生效位一起清掉，否则重新打开时"路径没变"会让它挂着一个已经 deinit 的后端 */
    if (!stSnap.stBoard.bEnable)
    {
        if (m_stOcrPP.available())
        {
            m_stOcrPP.deinit();
        }
        m_strOcrDictApplied.clear();
        attachNull();
        return;
    }

    /* 字典跟着 **rec** 那一颗 */
    std::string strDict;
    CModelDesc_S stRec;
    if (m_stPool.descOf(CLASSROOM_MODEL_KEY_OCR_REC, stRec))
    {
        strDict = ocrDictPathOf(stRec.strModelPath);
    }

    if (strDict.empty())
    {
        /* 模型表里没有 OcrRec：能力就是不在。
           不猜一个默认路径——猜中了是运气，猜不中每次心跳都在报一个假路径 */
        if (m_stOcrPP.available())
        {
            m_stOcrPP.deinit();
        }
        m_strOcrDictApplied.clear();
        attachNull();
        return;
    }

    COcrBackendParam_S stParam;
    stParam.strDictPath = strDict;
    /* 三道门与 box_max 取自配置快照。
       热换这条通道是第 4 刀接的，本刀只是把它的上游接上——两条路径（init 与 setDbParam）
       都读同一份 stParam.stDb，所以"改了门限不生效"不需要区分是哪一条 */
    stParam.stDb = stSnap.stOcr;
    if (strDict != m_strOcrDictApplied)
    {
        /* 换字典=换后端：init 内部先 deinit，所以在途作业会被丢掉——那是运维改动了 OCR 模型
           目录这一件事应得的后果（图 belong 于旧字典，认出来的字也不该进报表） */
        if (m_stOcrPP.init(m_spSched.get(), &m_stPool, stParam))
        {
            m_strOcrDictApplied = strDict;
        }
        else
        {
            /* 装配失败不记路径：下一次换配置（哪怕是运维改了个无关字段）会再试一遍。
               现场把字典补投之后不需要重启进程，这是"挂空后端而不是留空指针"的另一半理由 */
            m_strOcrDictApplied.clear();
            dlog_error("classroom engine: ocr 后端初始化失败, dict=%s, 板书 ocr 关闭",
                       strDict.c_str());
            attachNull();
            return;
        }
    }
    else
    {
        m_stOcrPP.setDbParam(stParam.stDb);
    }

    /* "挂 PP 还是挂空"只有这一个判据：available() 读的是后端自己的运行位，
       比在这里再数一遍"我刚才 init 成功了没"少一份会腐烂的账 */
    IOcrBackend *pBackend = &m_stOcrNull;
    if (m_stOcrPP.available())
    {
        pBackend = &m_stOcrPP;
    }
    m_stBoard.setBackend(pBackend);
}
void CClassroomEngine::boardStep(int64_t llNowMs)
{
    /* 先看开关再看总线：关掉的那条路连 latest() 都不取（一次原子读 + 一次引用计数），
       与 seatStep 同一处理。热路径上"没开"必须比"开了但没事做"更便宜，否则
       出厂默认全关的功能会先付一笔只为了说不的税 */
    if (!m_stBoard.param().bEnable)
    {
        return;
    }

    const ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap || !spSnap->stBoard.bEnable)
    {
        return;
    }

    /* 收结果排在降档闸**之前**：后端那格信箱只有一格，硬降档期间不来取的话它会一直占着——
       在途标记由 watcher 的 inflight_timeout 强制放行，而信箱那格没人放，
       于是恢复之后每次 submitJob 都被当场拒（读数上是"板书写了一整节而 ocr 一条不进"）。
       一次 takeResult 只是一次加锁取走，硬降档要省的从来不是这个 */
    COcrJobResult_S stJob;
    if (m_stOcrPP.takeResult(stJob))
    {
        /* #64 第 10 刀：取到结果这一拍顺手把后端两个读数带给 watcher——
           stat() 锁 m_mtx、dbStat() 无锁拷贝，都与既有 stat()/pipeStat() 同口径
           （读侧最坏"少看这几次"，不读到半个结构）；本线程就是持有后端的那条引擎线程，
           不违反 §4.2 规则 12（那条禁的是 watcher 跨线程碰后端，不禁引擎读自己的后端） */
        m_stBoard.onOcrResult(stJob.llDueMs, stJob.u32Seq, stJob.bOk, stJob.enState, stJob.vecText,
                              m_stOcrPP.stat().llLastJobMs, m_stOcrPP.dbStat().u64Truncated);
    }

    if (m_enLevel >= GuardLevel_E::GL_HARD)
    {
        /* 硬降档起整条让路：一次 OCR 是几十到几百毫秒，比一次姿态还贵，
           而降级的语义就是"先把节拍与内存保住"。软降档只拉采样的频，不动这条——
           它本来就是分钟尺度的事件，再降也降不出什么 */
        return;
    }

    /* 黑板区域按配置版本号刷新。换算不走 CFrameGeometry：那条链给的是**分析帧**框，
       而镜像/旋转之后"哪块是黑板"在分析帧里的位置会变，源帧归一化的不会——板子不会跟着装法挪。
       与座位网格同一套坐标约定，两处共用一个口径而不是各写一份 */
    if (m_llBoardRectVer != spSnap->llVersion)
    {
        m_llBoardRectVer = spSnap->llVersion;
        CRectf_S stNorm;
        const bool bKnown = boardRectNormSrc(*spSnap, stNorm);
        /* 区域被删掉时给一个空框而不是"保留上一次"：按旧框继续采是在拍一块已经不存在的板。
           watcher 收到空框会自己落到 rect=0，读数上就是 noreg 在涨 */
        m_stBoard.setBoardRectNorm(bKnown ? stNorm : CRectf_S(), spSnap->nSrcWidth, spSnap->nSrcHeight);
    }

    const ConstFeatureFramePtr spFrame = m_stFeatureBus.latest();
    const bool bGeomReady = (nullptr != spFrame) && spFrame->stGeom.isReady();

    /* 遮挡比只在"真的可能答应一次采集"时才算：全帧每人一个框与每块黑板求交是 60×16 次矩形运算，
       放在每拍无条件跑，等于为一件一分钟才发生一次的事付十倍节拍的税 */
    float fOcc = 0.0f;
    if (nullptr != spFrame &&
        (m_stBoard.episodePending() || !spSnap->stBoard.bRequireEpisode))
    {
        const CRegionTable &stRegions = m_stRule.regions();
        const std::vector<int32_t> &vecBoardIds = stRegions.idsOfRole(RegionRole_E::REGION_ROLE_BOARD);
        for (const int32_t nId : vecBoardIds)
        {
            const CRegionArea_S *pstArea = stRegions.regionOf(nId);
            if (nullptr == pstArea || pstArea->stBox.area() <= 0.0f)
            {
                continue;
            }
            float fCover = 0.0f;
            for (const CPersonFeature_S &stRow : spFrame->vecPersons)
            {
                if (stRow.nPersonId < 0)
                {
                    continue;
                }
                /* 用人体框而不是人头框：挡板书的是身子，头框量出来的小得多，
                   会把"整个人站在板上"读成 0.1 的遮挡而照样采 */
                fCover += rectIntersectionArea(pstArea->stBox, stRow.stBodyBox) / pstArea->stBox.area();
            }
            if (fCover > fOcc)
            {
                fOcc = fCover;
            }
        }
        if (fOcc > 1.0f)
        {
            fOcc = 1.0f;   /* 多块区域叠在同一个人身上时会加过头，比例的意义就没了 */
        }
    }

    /* 冷启动保持：边沿挂着而 OCR 从未驻留时，**不消费**这次机会——
       把活动闩推上去让暖组下拍装载，边沿留到装载完成。原先只靠下面那道 bWaitWarm 挡在消费
       之前，但那个门要求活动闩已置位，而闩只能由 captureBusy 在暖组里置位 ⇒ 冷启动时恒 0，
       第一个边沿必然当拍消费成 CAPTURE_MISS，之后闩永远没机会置位（机上 ep=11 全 miss、warm=0）。
       判据本体住在 warm_policy::boardColdStartHold，引擎这里只留闩动作：推进活动闩、整拍让路 */
    if (boardColdStartHold(spSnap->stBoard.bEnable, m_stBoard.captureBusy(),
                           !spSnap->stBoard.bRequireEpisode,
                           m_stPool.stateOf(CLASSROOM_MODEL_KEY_OCR_DET)))
    {
        m_llBoardLastActMs = llNowMs;
        return;
    }

    /* 「没驻留」有两种，只有值得等的那一种该把这次机会留给下一拍：需求在（warmBoardOcrWanted）
       而 det 的状态还是 UNLOADED（=已注册、此刻没驻留、也没有刚失败过）。MODEL_MISSING /
       OUT_OF_BUDGET / LOAD_FAILED 都是"再等也不会有"，照旧往下走，让 unav= 与 st= 把能力缺失
       报出去——不分辨这一格的话，一台没投模型文件的机器会把"缺件"读成"一直在等"，
       而这两件事在现场要查的东西完全相反（一个查部署清单，一个查内存与节拍） */
    const bool bWaitWarm = warmBoardOcrWanted(*spSnap, llNowMs, warmNeedOf()) &&
                           BackendState_E::UNLOADED == m_stPool.stateOf(CLASSROOM_MODEL_KEY_OCR_DET);

    CBoardCapture_S stCap;
    if (!m_stBoard.wantCapture(llNowMs, bGeomReady, fOcc, bWaitWarm, stCap))
    {
        return;
    }
    /* 走到这里时机已经判过、在途标记已经挂上。下面每一格失败都必须**当场回填**，
       只 return 的话这一次采集要等 inflight_timeout_ms 才放行（现象是"边沿都对、
       黑板区域也画了，但板书几十秒不动一下"）；传 0 号是因为这次采集根本没进后端 */
    const std::vector<CBoardText_S> vecNone;

    if (!m_stOcrPP.available())
    {
        /* 挂的是空后端：这就是"能力不在"，
           报 MODEL_MISSING 而不是 CAPTURE_MISS——后者会让现场去查镜像/旋转参数，
           而该查的是部署清单。这一格也是 CNullOcrBackend 存在的意义：
           它把"能力不在"说成一个每拍都被读出来的状态，而不是一条静默 */
        m_stBoard.onOcrResult(stCap.llDueMs, 0, false, BackendState_E::MODEL_MISSING, vecNone);
        return;
    }

    const CropUnitPtr spCrop = cropUnitOf(CLASSROOM_MODEL_KEY_OCR_DET);
    const FramePtr spPixel = m_stBus.latest();
    if (!spCrop || !spCrop->isReady() || !spPixel || !spPixel->pucNv12)
    {
        /* 后端在而这张图拿不出来：det 的裁剪档随组驻留（模型被时间片换下时档就没了），
           帧总线则可能整段没帧。两件事在心跳上都该读成同一档——"这一拍交不出一张可识别的图" */
        m_stBoard.onOcrResult(stCap.llDueMs, 0, false, BackendState_E::CAPTURE_MISS, vecNone);
        return;
    }

    /* 装法带镜像或旋转时，直接裁出来的像素是反字/倒字，而 OCR 对反字不报错：它给一串
       看着完全正常的乱码并**真的进报表**，板上没有一行读数说得清它是假的。
       第 5 刀起这一格不再是"宁可不要"——摆正计划由 `orientPlanOf` 给（与 CFrameGeometry
       同源、同一个配置值，不自己判断第二次），摆得正就摆，摆不正才放弃这一次采集 */
    const int nBufW = static_cast<int>(spCrop->inputWidth());
    const int nBufH = static_cast<int>(spCrop->inputHeight());
    const COrientPlan_S stPlan = orientPlanOf(spPixel->stGeom.mirrorOf(), spPixel->stGeom.rotationOf(),
                                              nBufW, nBufH);
    if (stPlan.bNeeded && !stPlan.bSupported)
    {
        /* 摆不出来的只有一种：要转 90/270 而 det 档不是方的（转完的画幅就不是模型要的那张）。
           这一档必须是"放弃"而不是"照做但结果可疑"——后者会把内容级错误读成 OCR 精度问题 */
        if (!m_bOcrMountWarned)
        {
            m_bOcrMountWarned = true;
            dlog_warn("classroom board: 挂载 mirror=%d rotate=%d 无法摆正到 %dx%d (bad=%d), 放弃本次采集",
                      static_cast<int>(spPixel->stGeom.mirrorOf()), spPixel->stGeom.rotationOf(),
                      nBufW, nBufH, static_cast<int>(stPlan.enBad));
        }
        m_stBoard.onOcrResult(stCap.llDueMs, 0, false, BackendState_E::CAPTURE_MISS, vecNone);
        return;
    }

    RgaBufPtr spRgb;
    if (!spCrop->cropRoi(*spPixel, stCap.stRectSrc, spRgb, nullptr, stPlan.nRgaRotation) || !spRgb)
    {
        m_stBoard.onOcrResult(stCap.llDueMs, 0, false, BackendState_E::CAPTURE_MISS, vecNone);
        return;
    }

    /* 先旋后翻，顺序就是判据：翻的轴已由 orientPlanOf 共轭过（90/270 换轴、180 不换），
       反过来做要先落一块中间缓冲，多一次 2.76 MB 搬运而换不到任何正确性。
       翻的是三元组、与 RGB/BGR 通道序无关，所以配置里的 ch_order 怎么切都不影响这一段 */
    if (stPlan.bFlipH)
    {
        flipRgb888H(spRgb->data(), nBufW, nBufH);
    }
    if (stPlan.bFlipV)
    {
        flipRgb888V(spRgb->data(), nBufW, nBufH);
    }

    /* 作业号由后端数（它才是"哪一格装着哪张图"的那个人），这里只把它登记进在途那一格。
       尺寸传裁剪档自己的输入宽高——后端按它比对 det 模型的输入，不符就当堂拒 */
    const uint32_t u32Seq = m_stOcrPP.submitJob(spRgb, static_cast<int32_t>(spCrop->inputWidth()),
                                               static_cast<int32_t>(spCrop->inputHeight()),
                                               stCap.llDueMs);
    if (0 == u32Seq)
    {
        /* 图已经裁好了而后端排不上（上一张还在跑，或它的结果还没被下一拍取走）：
           这一档与"没图"必须分开，前者查的是节拍与耗时，后者查的是部署与区域 */
        m_stBoard.onOcrResult(stCap.llDueMs, 0, false, BackendState_E::BACKEND_BUSY, vecNone);
        return;
    }
    m_stBoard.markCaptureSubmitted(u32Seq);
}

void CClassroomEngine::drainBoard(CReportBundle_S &rstBundle)
{
    const ConfigPtr spSnap = m_stConfig.current();
    rstBundle.bBoardEnable = (nullptr != spSnap) && spSnap->stBoard.bEnable;
    rstBundle.nBoardState = static_cast<int32_t>(m_stBoard.lastState());
    if (!rstBundle.bBoardEnable)
    {
        return;   /* 关着的不取：取走就少了 N 条再也报不出去的内容，而这段压根不会写进报文 */
    }

    /* 只有一个上限：条数。第 7 刀之后一条板书记录是定长的（五个整数，没有正文——
       正文按 PII 口径不出设备），所以"另给一个字节闸"那件事失去了对象：
       8 条最坏 810B，恒在预算内，算它只是把「peek 出来又放回去」那条支路留在代码里
       骗下一个读的人。字节的判据搬到了能真量到它的地方（report 用例
       testBoardSegmentIsFixedCost），搬不走的那部分理由写在 report_format.h
       ——谁往记录里加回变长字段，就要在这里把第二个闸加回来 */
    while (static_cast<int32_t>(rstBundle.vecBoard.size()) < CLASSROOM_BOARD_REPORT_MAX)
    {
        CBoardItem_S stItem;
        if (!m_stBoard.pop(stItem))
        {
            break;
        }
        rstBundle.vecBoard.push_back(stItem);
    }
}

void CClassroomEngine::drainScene(CReportBundle_S &rstBundle)
{
    const ConfigPtr spSnap = m_stConfig.current();
    rstBundle.bSceneEnable = (nullptr != spSnap) && spSnap->stScene.bEnable;
    if (!rstBundle.bSceneEnable)
    {
        return;
    }
    /* 正在延长的那一段每次都给（报文里它没有 end_t）：平台在课中就要能指出"现在是问答"，
       而只给闭合段的话整节课的报表里最后一段时间上永远是空的 */
    (void)m_stScene.current(rstBundle.stOpenScene);
    /* 条数上限在这里而不是在写报文的地方：超出的那些**留在环里下次再取**，
       在报文里截断就成了"取走了又丢掉" */
    rstBundle.vecScenes = m_stScene.takeClosed(CLASSROOM_SCENE_REPORT_MAX);
}

void CClassroomEngine::tryReport(int64_t llNowMs)
{
    const ConfigPtr spSnap = m_stConfig.current();
    if (!spSnap)
    {
        return;
    }

    /* 1. 先补旧账再发新的：平台按到达顺序排时间轴，新报表插在积压前面就会让时间轴倒流。
          这一段不看聚合中心是否就绪 —— 环里的字节是上一轮运行留下的，与本轮聚合无关 */
    drainSpool();

    if (!m_stData.isReady())
    {
        return;
    }

    /* 2. 会话收尾。判据用账本自己的起点而不是引擎记的一份布尔：
          llBeginMs 由第一条样本落下，清零即"没有会话"，一个事实只有一个持有者，
          不会出现"引擎以为在上课而账本是空的"这种两本账打架的状态 */
    const CSessionAccum_S stSession = m_stData.session();
    if (stSession.llBeginMs > 0)
    {
        const int64_t llMaxMs =
            static_cast<int64_t>(spSnap->stEgress.nSessionMaxSec) * 1000;
        const bool bTimeout = (llMaxMs > 0) && (llNowMs - stSession.llBeginMs >= llMaxMs);
        /* 静默收尾（#95）：连续 nSilenceSec 没有人「在场」→ 下课。口径是"人走光多久"
           （llLastPresentMs），不是"样本多久没来"（llLastMs）——后者是断流，处置是
           等它回来，不是把一节课拦腰收掉；所以静默成立还要求窗口内样本仍在流动。
           整节没见过人（llLastPresentMs==0）就按会话起点起算：空开 10 分钟也是一节废课 */
        const int64_t llSilMs =
            static_cast<int64_t>(spSnap->stEgress.nSilenceSec) * 1000;
        const int64_t llSilRef = (stSession.llLastPresentMs > 0)
                                     ? stSession.llLastPresentMs : stSession.llBeginMs;
        const bool bSilence = (llSilMs > 0) &&
                              (llNowMs - llSilRef >= llSilMs) &&
                              (llNowMs - stSession.llLastMs <= llSilMs);
        const char *pszCloseReason = nullptr;
        if (!spSnap->bEnable)
        {
            pszCloseReason = "enable_off";
        }
        else if (bTimeout)
        {
            pszCloseReason = "session_timeout";
        }
        else if (bSilence)
        {
            pszCloseReason = "silence_timeout";
        }
        if (nullptr != pszCloseReason)
        {
            /* 排在周期报表之前：下课这一份必须带着最终的会话账，
               而且 closeSession 已经把取数游标推走了，紧接着的周期拍自然就是空的一份 */
            closeSession(llNowMs, pszCloseReason);
        }
    }

    /* 3. 周期报表 */
    if (!beatDue(m_llLastReportMs, llNowMs, spSnap->stEgress.nReportIntervalSec * 1000))
    {
        return;
    }

    CReportBundle_S stBundle;
    if (!m_stData.buildReport(llNowMs, stBundle))
    {
        return;    /* 一条样本都没有（刚起线、或总开关刚打开）：不发空报表 */
    }
    if (stBundle.vecTimeline.empty() && stBundle.vecEvents.empty() && 0 == stBundle.nEventLost)
    {
        /* 到点但这一周期内什么都没发生（断流、或刚收尾完）。聚合中心的 latest 是常驻的，
           照发就会让平台看到"同一份最新指标又报了一次"，而它分辨不出这是新数据还是重复，
           时间轴上会长出一段假的平线 —— 宁可不发 */
        return;
    }

    /* 考勤与样本同车：到课人数是"此刻的账本状态"，与样本的采样时刻差不到一拍。
       单独开一条上报通道只会让平台拿到两份不同时刻的考勤，然后去对那个差 */
    m_stAttend.snapshot(stBundle.stAttend);
    /* 座位图同一处取，而且**只能在这里取**：snapshot() 会顺手扣掉已报的整秒，
       多调一次就等于白送一份"这些秒数已经报过了"，少调一次则下一份报表把两个周期并成一个。
       报表被上面那些早退条件挡掉时不取快照，于是秒数继续累加——那是真话：这段时间确实没报过 */
    m_stSeat.snapshot(stBundle.stSeat);
    /* 学生轨迹与热力同一处取，同一个理由：取走就没了（队列语义），
       所以只能在报表确定要交付的这一段之后取——上面那些早退条件挡掉报表时不取，
       攒着的点留给下一份，那是真话：这段时间确实没报过 */
    m_stStu.snapshot(stBundle.stStuTrack);
    /* 板书与上面两段同处取：它取的也是"这一节课攒下的会话级内容"，
       而且取走就没了——排在交付之前，报表被上面那些早退条件挡掉时就不会白吃掉一批记录 */
    drainBoard(stBundle);
    /* 时段同样在交付之前取：取走就是队列语义，取早了又没交付会少一段时间 */
    drainScene(stBundle);

    deliverReport(stBundle, SPOOL_TYPE_REPORT);
}

void CClassroomEngine::deliverReport(const CReportBundle_S &stBundle, uint32_t u32Type)
{
    /* 成对取墙钟与单调钟：报文里每个时间点都是"单调读数 → 墙钟"的换算，
       两个读数之间隔多久，整份报文就偏移多久 —— 一次 clock_gettime 的量级，可忽略 */
    const int64_t llWallMs = reportWallMs();
    const int64_t llAnchorMs = classroomNowMs();
    const std::string strPayload = reportBundleJson(stBundle, llWallMs, llAnchorMs);

    if (!m_fnUpload)
    {
        /* 没接通道：只落一条摘要，且**不**落滞留环（理由见 CUploadFn 末段）。
           这行的用途是让"聚合算错了"与"通道没接"在设备上长得不一样 */
        dlog_info("classroom report: 出报表 %s 类型=%s 字节=%zu 时间线条=%zu 事件条=%zu 丢失=%d 滞留=%d 开课=%llds 采样=%d",
                  groupMetricText(stBundle.stLatest).c_str(), CReportSpool::typeName(u32Type),
                  strPayload.size(), stBundle.vecTimeline.size(), stBundle.vecEvents.size(),
                  stBundle.nEventLost, stBundle.nEventDropped,
                  static_cast<long long>(stBundle.stSession.llPresentMs / 1000),
                  stBundle.stSession.nSampleNum);
        return;
    }

    bool bOk = false;
    try
    {
        bOk = m_fnUpload(u32Type, strPayload);
    }
    catch (const std::exception &stEx)
    {
        /* 通道多半是另一层的包装，异常同样必须在边界挡住：
           节拍线程一死，整个子系统会变成"帧还在进、报表永不出"的静默僵死 */
        dlog_error("classroom report: 上报抛出异常 %s, 按发送失败处理", stEx.what());
        bOk = false;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        if (bOk)
        {
            ++m_stStat.u64UploadOk;
        }
        else
        {
            ++m_stStat.u64UploadFail;
        }
    }

    if (bOk)
    {
        if (m_bUploadBroken)
        {
            m_bUploadBroken = false;
            dlog_info("classroom report: 上报通道恢复, 积压按序补传");
        }
        return;
    }

    m_bUploadBroken = true;
    dlog_error("classroom report: %s 上报失败 (%zuB), 进滞留环待补传",
               CReportSpool::typeName(u32Type), strPayload.size());

    std::string strErr;
    if (!m_stSpool.append(u32Type, strPayload.data(), strPayload.size(), strErr))
    {
        /* 落盘也失败才是真丢：这一条之后没有任何补救手段，日志必须说清丢了多少字节 */
        dlog_error("classroom report: %s 丢失, 滞留环写入失败: %s",
                   CReportSpool::typeName(u32Type), strErr.c_str());
        return;
    }
    dlog_warn("classroom report: 已进滞留环待补传, pending=%lldKB",
              static_cast<long long>(m_stSpool.pendingBytes() / 1024));
}

void CClassroomEngine::closeSession(int64_t llNowMs, const char *pszReason)
{
    /* 代码内原因 token 不动（enable_off/session_timeout/silence_timeout，对齐设计文档），
       日志只印中文——给人看的一行里不再带英文 */
    struct ReasonPair_S { const char *pszToken; const char *pszCn; };
    static const ReasonPair_S AST_REASON[] = {
        { "enable_off",      "总开关关闭" },
        { "session_timeout", "上课时长到点" },
        { "silence_timeout", "人走光超时" },
    };
    const char *pszCn = "未知原因";
    if (nullptr != pszReason)
    {
        for (size_t i = 0; i < sizeof(AST_REASON) / sizeof(AST_REASON[0]); ++i)
        {
            if (0 == strcmp(pszReason, AST_REASON[i].pszToken))
            {
                pszCn = AST_REASON[i].pszCn;
                break;
            }
        }
    }
    /* 收尾前先把合并器里挂着的 END 全部放行：END 在合并器里最多挂一个窗口（3s），
       而总开关关闭那一拍，shutdownActive 补发的 END 刚从 ruleStep 进来就撞上收尾——
       不放行的话，这节课最后一段行为的 END 要么迟到下一个会话、要么永远出不去，
       平台侧看到的是一节「有开始没结束」的课 */
    m_stMerger.flushAll();
    drainMerger();

    CReportBundle_S stBundle;
    if (!m_stData.closeSession(llNowMs, stBundle))
    {
        /* 理论上到不了这里（调用方已按账本起点判过"有没有会话"），留着是因为它便宜：
           真到了就是账本状态与判据不一致，那条线索比省一行日志值钱 */
        dlog_info("classroom engine: 跳过会话 close(%s)，无累计数据", pszReason);
        return;
    }

    /* 考勤先封账再取快照：收尾报表上的名单必须是终值。
       封账之后的观测会被账本自己拒收（u64RejectObs），那是"还有一路在途结果没排干"的读数 */
    m_stAttend.closeSession(llNowMs);
    m_stAttend.snapshot(stBundle.stAttend);
    m_bAttendOpen = false;

    /* 平台统计的最终一包：考勤取账本**封账后**的值（上面刚 snapshot），不能停在上一份 5s 样本上。
       统计会话与报表会话同起同落——报表收尾了这一节就结束了，统计继续报只会让平台多出一段
       没有下课的尾巴 */
    int32_t nLeftMidFinal = 0;
    if (stBundle.stAttend.bEnable && stBundle.stAttend.nPresentNum > 0)
    {
        const ConfigPtr spCfg = m_stConfig.current();
        const int64_t llLeaveMs = spCfg
                                      ? static_cast<int64_t>(spCfg->stStats.nLeaveAbsentSec) * 1000
                                      : 300000;
        m_vecAttendScratch.clear();
        const int32_t nRecNum = m_stAttend.records(m_vecAttendScratch);
        nLeftMidFinal = countLeftMid(m_vecAttendScratch.data(), nRecNum, llNowMs, llLeaveMs,
                                     m_stData.session().llLastPresentMs);
    }
    CStatsSessionSummary_S stStatsSummary;
    stStatsSummary.nPresentNum = stBundle.stAttend.nPresentNum;
    stStatsSummary.nNamedNum = stBundle.stAttend.nNamedNum;
    stStatsSummary.nExpectedNum = stBundle.stAttend.nExpectedNum;
    stStatsSummary.nAbsentNum = stBundle.stAttend.nAbsentNum;
    stStatsSummary.nLeftMid = nLeftMidFinal;
    stStatsSummary.bAttendEnable = stBundle.stAttend.bEnable;
    m_stStatsHub.closeSession(llNowMs, reportWallMs(), stStatsSummary);
    /* 座位图：收尾报表要带这一节课最终的分布，所以快照排在交付之前、清零排在交付之后。
       顺序反过来的话清零与快照之间没有任何东西能把它救回来——这张图是一节课唯一的一份。
       上面那条"什么都没攒到"的早退不清零：没交付就没有"已报"，攒下的秒数留给下一份报表，
       在这里清等于把几分钟的真实在场悄悄扔掉 */
    m_stSeat.snapshot(stBundle.stSeat);
    /* 学生轨迹同处：收尾报表要带最后一段轨迹，取在交付之前、清在 reset（下面那一处）*/
    m_stStu.snapshot(stBundle.stStuTrack);
    /* 板书同样在收尾这份里取一次：下课那一刻最后一段可能刚采完，而下一份周期报表
       未必还会来（开关就是被关掉的）——留在环里的记录会跟着这次收尾一起消失，
       所以取在交付之前，与座位图同一个理由 */
    drainBoard(stBundle);
    /* 场景段先闭合再取：正在延长的那一段在下课这一刻才成为一段真时段，
       "下课"是它唯一的结束时刻来源（下一个采样周期属于下一节课）。
       闭合后环里的段照样被这份收尾报表带走，交付之后不 reset——它们已被消费 */
    m_stScene.closeSession(llNowMs);
    drainScene(stBundle);

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64SessionClose;
    }
    dlog_info("classroom engine: 会话收尾(原因=%s) 开机毫秒=%lld 已持续=%llds 采样=%d "
              "人数峰值=%d 考勤: 到场=%d 认名=%d 应到=%d 缺席=%d",
              pszCn,
              static_cast<long long>(stBundle.stSession.llBeginMs),
              static_cast<long long>(stBundle.stSession.llPresentMs / 1000),
              stBundle.stSession.nSampleNum, stBundle.stSession.nPersonMax,
              stBundle.stAttend.nPresentNum, stBundle.stAttend.nNamedNum,
              stBundle.stAttend.nExpectedNum, stBundle.stAttend.nAbsentNum);

    deliverReport(stBundle, SPOOL_TYPE_SESSION);
    /* 交付之后才清：deliverReport 只认那份已经格式化好的字节，快照里的秒数此刻已经落在报文里
       （直发或落盘都一样），这一清才是"本节结束"而不是"本节没报" */
    m_stSeat.reset();
    /* 先验表跟着热力同一处清。要澄清一句"为什么这一条不是必需的、但仍然要写"：
       热力的 reset 会 wipeRows() → 格子代次 +1，本层下一次 fold 看到代次变了会自己整表作废，
       所以**不发错号这件事不依赖这一行**（依赖别的层的信号来兜自己的底，本身就是要避免的耦合）。
       这一行清的是那六个计数：留着上一节的 inherit/new/steal，下一节的心跳读起来就是
       "这班人特别爱换座"或者"steal 突然猛涨"，而那两个判据是本层唯一的现场体检表 */
    m_stPrior.reset();
    /* 轨迹跟着清行（累计计数留着，见 CStuTrack::reset）。收尾报表已经把最后一段带走，
       这一清是"本节结束"而不是"本节没报"——与热力、先验同一处，三处必须同一处：
       会话边界是这条链上唯一的清零时刻，分开写迟早有一处跟着改另一处没跟着改 */
    m_stStu.reset();
    /* 板书同理：清的是在途标记、重入下限与变更基线（下一节课的第一次板书必然与上一节不同，
       但那要由内容自己说出来）。已经取进上面那份报表的记录不在清理范围内——它们已交付 */
    m_stBoard.closeSession(llNowMs);
}

void CClassroomEngine::drainSpool()
{
    /* 逐条取发与异常边界在 data 层的 spoolDrainBeat：那条判据要能在主机
       闸门里打红，必须住在不链 rknn 的文件里；引擎只剩配预算、记账进状态两件事 */
    if (!m_stSpool.isReady() || !m_fnUpload || 0 >= m_stSpool.pendingBytes())
    {
        return;
    }

    const ConfigPtr spSnap = m_stConfig.current();
    /* 预算直接用配置值，不再就地抬高：抬高等于把现场为慢链路配的 8KB 变成"配了不生效" */
    const int64_t llBudgetKb = (spSnap ? spSnap->stEgress.nSpoolDrainMaxKb
                                       : CLASSROOM_SPOOL_DRAIN_KB);

    CSpoolDrainStat_S stDrain;
    spoolDrainBeat(m_stSpool, m_fnUpload, llBudgetKb, stDrain);

    if (stDrain.nUploadThrow > 0)
    {
        /* 通道抛异常也按"通道坏了"处理：通道是外部包装层，抛异常说明它自己的状态
           已经不对，继续取只会把它炸得更远；等下拍 deliverReport 成功时一并复位 */
        dlog_error("classroom report: 补传上报抛出异常 %s, 本拍停止排干",
                   stDrain.strThrowWhat.c_str());
        m_bUploadBroken = true;
    }
    if (stDrain.nUploadError > 0)
    {
        m_bUploadBroken = true;
    }
    if (stDrain.bCommitStuck)
    {
        dlog_error("classroom report: 补传已提交但游标未前进, seq=%llu",
                   static_cast<unsigned long long>(stDrain.u64StuckSeq));
    }

    if (stDrain.nSent > 0)
    {
        dlog_info("classroom report: 本拍补传 %d 条 %lldKB, pending=%lldKB",
                  stDrain.nSent, static_cast<long long>(stDrain.llSentKb),
                  static_cast<long long>(m_stSpool.pendingBytes() / 1024));
    }
}

bool CClassroomEngine::spoolApply(const CConfigSnapshot_S &stSnap, std::string &rstrErr)
{
    CSpoolParam_S stParam;
    stParam.bEnable      = stSnap.stEgress.bSpoolEnable;
    stParam.strDir       = stSnap.stEgress.strSpoolDir;
    stParam.nMaxMb       = stSnap.stEgress.nSpoolMaxMb;
    stParam.nSegMaxKb    = stSnap.stEgress.nSpoolSegMaxKb;
    stParam.nDrainMaxKb  = stSnap.stEgress.nSpoolDrainMaxKb;

    /* 只有补传预算不影响环的物理形态，改它不需要重建：重建要 close+opendir+stat 一遍目录，
       换回来的是同一个文件布局，纯浪费一拍。m_bSpoolApplied 是这条捷径的**前提**：
       上一次没建成就不算"已按这版参数应用过"，走捷径会把一次临时失败冻成永久无补传 */
    if (m_bSpoolApplied && m_stSpoolParam.strDir == stParam.strDir &&
        m_stSpoolParam.nMaxMb == stParam.nMaxMb &&
        m_stSpoolParam.nSegMaxKb == stParam.nSegMaxKb &&
        m_stSpoolParam.bEnable == stParam.bEnable)
    {
        m_stSpoolParam.nDrainMaxKb = stParam.nDrainMaxKb;
        return m_stSpool.isReady() || !stParam.bEnable;
    }

    m_stSpool.deinit();
    m_bSpoolApplied = false;
    if (!stParam.bEnable)
    {
        m_stSpoolParam = stParam;
        m_bSpoolApplied = true;
        dlog_info("classroom engine: 报表滞留环按配置关闭, 积压保留在闪存上");
        return true;
    }

    if (!m_stSpool.init(stParam, rstrErr))
    {
        /* 失败时**不**记下新参数：记了就等于"已经按这版配置建过了"，
           下一次下发只要参数没再变就不会重试，而目录多半是那时才挂上的（U 盘/网络盘同理） */
        m_stSpoolParam = CSpoolParam_S();
        return false;
    }

    m_stSpoolParam = stParam;
    dlog_info("classroom engine: 报表滞留环就绪: %s, %s", stParam.strDir.c_str(),
              m_stSpool.statLine().c_str());
    return true;
}

void CClassroomEngine::metricApply(const CConfigSnapshot_S &stSnap)
{
    /* 心跳落点的装配。它不像 spoolApply 那样有"失败要说得出口"的返回值：
       落点坏掉的后果是心跳回到业务日志里，那是**能读出来**的降级（mlog= 那一列），
       而一条降级日志本身不该再产生第三种失败 */
    CMetricLogParam_S stParam;
    stParam.bOwnFile      = stSnap.stEgress.bMetricOwnFile;
    stParam.strFile       = stSnap.stEgress.strMetricFile;
    stParam.nMaxKb        = stSnap.stEgress.nMetricMaxKb;
    stParam.nMaxFiles     = stSnap.stEgress.nMetricMaxFiles;
    stParam.nHeadPeriodMs = stSnap.stEgress.nMetricHeadPeriodMs;

    std::string strBefore;
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        strBefore = m_strMetricHow;
    }

    /* configure 在锁外跑：它可能开文件、可能打一条 warn，持着状态锁等 spdlog 不是好主意。
       锁只包住"发布"这个动作——与 m_strPerceiveDetail 完全同形：写侧在节拍线程，
       读侧在命令线程的 statusJson，两边都不等谁，读数最多滞后一次下发 */
    const std::string strNow = m_stMetricLog.configure(stParam);
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_strMetricHow = strNow;
    }

    if (strNow != strBefore)
    {
        /* 只在落点的"自我描述"变了的那一次出声。判据用它自己而不是配置版本号：
           版本号每 2s 轮询都可能涨一格而参数一个没变，照版本号打就是一堆重复行；
           而真正要看见的三件事（建成了 / 降级了 / 要求与在用的不一致）必然改这句话 */
        dlog_info("classroom engine: 心跳落点 -> %s", strNow.c_str());
    }
}

bool CClassroomEngine::beatDue(int64_t &rlLastMs, int64_t llNowMs, int32_t nIntervalMs) const
{
    const int64_t llInterval = (nIntervalMs > 0) ? nIntervalMs : 1;
    if (rlLastMs > 0 && llNowMs - rlLastMs < llInterval)
    {
        return false;
    }
    rlLastMs = llNowMs;
    return true;
}

void CClassroomEngine::watchConfigFile(int64_t llNowMs)
{
    /**
     * 为什么用文件轮询而不是事件：平台侧下发需要一个 action_code，而 action_code.h 在
     * share/ipc_share/common/ 里（公共头，本项目不改）——课堂命令通道因此还是一个待决的
     * 产品决策。现场调参最常见的动作却是"改一个数再看效果"，所以这里提供一条
     * 不需要动公共仓就能用的热更新通道：2s 一次 stat，代价可忽略。
     * 命令通道落地后走 CClassroomEngine::reloadConfigText()，两者共用同一套校验与换入，
     * 本函数继续留着也无害（文件没变就是空转）。
     */
    if (m_strConfigPath.empty() || llNowMs - m_llLastConfigCheckMs < CLASSROOM_CONFIG_WATCH_MS)
    {
        return;
    }
    m_llLastConfigCheckMs = llNowMs;

    struct stat stInfo;
    if (0 != ::stat(m_strConfigPath.c_str(), &stInfo))
    {
        if (m_bConfigFilePresent)
        {
            /* 文件被删（恢复出厂/误操作）：已生效的快照继续用，不因此关功能 */
            dlog_warn("classroom engine: 配置文件 %s 已消失，沿用当前快照",
                      m_strConfigPath.c_str());
            m_bConfigFilePresent = false;
        }
        return;
    }

    const int64_t llMTimeSec = static_cast<int64_t>(stInfo.st_mtime);
    const int64_t llSize = static_cast<int64_t>(stInfo.st_size);
    /* mtime+size 相同也要算一次内容指纹。
       否则"同秒同长改写"这条路永远走不到 reload（见 configFileHash 的注释）——
       而这正是现场调参最常见的形状：110→120 一字之差、字节数不变。
       指纹只在"会早退"的那一跳算，所以没有变更的稳态代价是 68KB/2s 的一次读+哈希。 */
    if (m_bConfigFilePresent && llMTimeSec == m_llConfigMTimeSec && llSize == m_llConfigSize)
    {
        const uint64_t u64Hash = configFileHash(m_strConfigPath.c_str());
        if (u64Hash != 0 && u64Hash == m_u64ConfigHash)
        {
            return;   /* 真的没变 */
        }
        /* 指纹不同 ⇒ 同秒同长改写了，落下去走 reload（版本号照旧会 +1） */
        dlog_info("classroom engine: 配置文件内容变了而 mtime/size 未动（同秒改写）");
    }

    m_bConfigFilePresent = true;
    m_llConfigMTimeSec = llMTimeSec;
    m_llConfigSize = llSize;
    m_u64ConfigHash = configFileHash(m_strConfigPath.c_str());

    std::string strErr;
    const int64_t llOldVersion = m_stConfig.current() ? m_stConfig.current()->llVersion : 0;
    if (m_stConfig.reloadFromFile(m_strConfigPath, strErr))
    {
        const ConfigPtr spNew = m_stConfig.current();
        dlog_info("classroom engine: 配置文件已变更，重载版本 %lld -> %lld",
                  static_cast<long long>(llOldVersion),
                  static_cast<long long>(spNew ? spNew->llVersion : 0));
        /* 不在这里 applySnapshot：让下一拍统一走版本变更分支，
           保证「应用配置」只有一条路径（两条路径应用同一份参数迟早会分叉） */
    }
    else
    {
        dlog_error("classroom engine: 配置文件 %s 被拒，保持版本 %lld: %s",
                   m_strConfigPath.c_str(), static_cast<long long>(llOldVersion), strErr.c_str());
    }
}

void CClassroomEngine::applySnapshot(const ConfigPtr &spSnap, int64_t llNowMs)
{
    const int64_t llOldVersion = m_llAppliedVersion;
    m_llAppliedVersion = spSnap->llVersion;

    /* 空闲卸载时长、时间片、预算余量都是热生效的 */
    m_stPool.setParam(spSnap->stPool);

    /* 模型表同样要热落地：不接这一步的话"下发成功"只是 JSON 换了一版，池子仍按旧路径找文件 */
    syncModelDescs(*spSnap);

    /* 上报周期也要热落地。它只服务一处（断档积分的基线），因此用窄接口换这一个数，
       不重跑 data center 的 init —— 那条路会把会话账本清零，改个数丢一节课不划算 */
    m_stData.setReportIntervalMs(static_cast<int64_t>(spSnap->stEgress.nReportIntervalSec) * 1000);

    /* 平台统计（0x62xx）参数热生效：只换周期/阈值，不清当前会话（关总开关时
       configure 会自己丢弃未送出的记录，避免下一节把旧账当本节发） */
    m_stStatsHub.configure(spSnap->stStats);
    /* 角色同样热生效：教师机/学生机切换不该为了清掉旧角色的累计账去重启进程。
       同角色调用无副作用；角色真的变了才把旧会话封口发最终包，下一拍按新角色的
       在场条件重开新会话（教师机以教师到场开课，学生机仍要学生到场） */
    m_stStatsHub.setDeviceRole(spSnap->enRole, llNowMs, reportWallMs());

    /* 滞留环参数同理：目录/容量/开关变了才重建（spoolApply 内部判），失败要说得出来 */
    std::string strSpoolErr;
    if (!spoolApply(*spSnap, strSpoolErr))
    {
        dlog_error("classroom engine: 报表滞留环重新挂载失败, 补传关闭: %s",
                   strSpoolErr.c_str());
    }
    /* 心跳落点同理，但它自己分得清哪些能当场生效（开关与回抄周期）、哪些只能等重启
       （环的几何）；等不了的那些不会悄悄不理，而是在心跳的 mlog= 里挂 geo_stale=1 */
    metricApply(*spSnap);

    /* 考勤参数热落地。刻意不 reset：已命名的人不会因为运维调了一下分数门就消失，
       已过到课门限的行也不会因为把 present_min_ms 调大又变回没到课——
       那些"已经数到的数"是真发生过的，撤销它们等于改历史 */
    m_stAttend.setParam(spSnap->stAttend);
    /* 座位热力同理：换参数不清图（setParam 只在格子编号真的变了才清）。
       这里刻意**不**跟着重建网格——区域表要到下面 m_stRule.applyConfig 才换成这一版的，
       而现在也没有可用的帧几何（总线可能还没建槽位）。重建只在 seatStep 一处发生：
       两处都能建，就会出现"配置线程用旧区域表算出一张图、盖掉节拍线程那张"的错位 */
    m_stSeat.setParam(spSnap->stSeat);
    /* 先验同理（换参数不清表）：把 release_beats 从 6 调到 8 不该让全班重新领一遍号。
       但 enable 这一位是**当场生效**的：它只切换考勤读哪个号，没有"半开"的状态可言，
       所以换完这一拍起，新起的行用先验号、已经在账上的行留着原号（下一节课才全新一套） */
    m_stPrior.setParam(spSnap->stPrior);
    /* 板书参数换入后把区域缓存判脏：黑板框是从这一份快照的 region_roles 现推的，
       只换参数不刷区域的话，改完黑板的位置还要等下一次换配置才生效（§1.18 那条
       "重画区域不生效"的同类缺陷就是漏了这个信号）*/
    m_stBoard.setParam(spSnap->stBoard);
    /* 后端换入排在 setParam 之后：它读的是这一版的 board 开关（关着就不该留那条线程与字典）。
       排在 syncModelDescs（本函数开头）之后同样是必须的——字典路径从 OcrRec 那一行的目录推出来 */
    ocrAttach(*spSnap);
    m_llBoardRectVer = -1;
    /* 镜像/旋转本身是配置项，换版之后那条"这一台拍不出能认的图"要能再说一遍 */
    m_bOcrMountWarned = false;
    /* 时段参数换入不清段：改滞回时长的人要看到的是"从这一拍起按新规则判"，
       不是"整节课的时段账本被清空重来"（与 CSeatHeat::setParam 同一条取舍） */
    m_stScene.setParam(spSnap->stScene);
    /* 底库换在同一处：它的来源（file/sqlite/none）是配置事实，而换库必须是"与参数同批"——
       分两处就会有一拍拿着新参数配旧名单，那批名字是错的人名而不是没名字 */
    applyGallery(*spSnap);

    /* 已驻留模型的裁剪档也要跟上这一版配置：ensureCropUnit 平时只在"刚拿到租约"时被调，
       于是改了 align.crop_px 会读起来"下发成功、画面照旧"。尺寸没变时它自己原样返回 */
    for (const auto &stDesc : spSnap->vecModels)
    {
        if (m_mapHeld.find(stDesc.strKey) != m_mapHeld.end())
        {
            ensureCropUnit(stDesc, *spSnap);
        }
    }

    /* ---- 规则引擎换配置 ----
       放在这里而不是配置线程：applyConfig 要给「这一版关掉的每条行为」补发结束事件，
       那是要写事件环的动作，而环只在节拍线程被读写。两个入参各管一头：
       快照决定下一拍用什么门限与区域，llNowMs 决定补发的那几条 END 标在什么时刻 */
    m_stRule.applyConfig(spSnap, llNowMs);
    if (!spSnap->bEnable)
    {
        /* 总开关关闭时逐条行为的开关可能一个都没动，applyConfig 那一步就一条都没收尾。
           不补这一刀，平台侧会看到一批「拉到底永不结束」的事件——用户读到的正是那条，
           而不是我们日志里的「enable=0」*/
        m_stRule.shutdownActive(llNowMs);
        /* 与之配对的**会话收尾**在 tryReport 里做，且刻意排在后面：这一拍 ruleStep 会把
           上面补发的那些 END 取进聚合中心，收尾的那一份才带着完整的账。
           把 closeSession 挪到这里的话，这批 END 会落进清零后的新账本——一节课的尾巴
           记到下一节课头上，这种事在报表上看不出来，只在"两节课的站立人数都差一点"时才现形 */
    }

    int32_t nOn = 0;
    for (const auto &stGate : spSnap->vecGates)
    {
        nOn += stGate.bEnable ? 1 : 0;
    }

    /* 几何参数热重载（P2-7）：镜像/旋转/画布尺寸三项是逐帧随帧下发的变换元数据，
       总线当场换入即可——在飞旧帧带着自己的旧几何（值拷贝，逐帧自洽），
       区域表由 ensureRegions 按几何值自愈重建，OSD 逐帧取用，消费端没有第二处要动。
       源尺寸变了仍然只能重启：槽缓冲按源尺寸一次分配（总线硬约束 1），
       且上游 VPSS AI 通道就是按源分辨率建的，本引擎热换了也没处生效——
       这种「改了没生效」必须继续把 stale 亮出来，否则现场会以为下发成功 = 画面已经转过来 */
    const CFrameGeometry stGeomOld = m_stBus.geometry();
    if (llOldVersion >= 0 && stGeomOld.isReady())
    {
        const GeomSwap_E enSwap = m_stBus.setupGeometry(
            spSnap->nSrcWidth, spSnap->nSrcHeight, spSnap->nCanvasWidth, spSnap->nCanvasHeight,
            spSnap->enMirror, spSnap->nRotate);
        const CFrameGeometry stGeomNew = m_stBus.geometry();
        const bool bMatched = stGeomNew.isReady() &&
                              spSnap->nSrcWidth == stGeomNew.srcWidth() &&
                              spSnap->nSrcHeight == stGeomNew.srcHeight() &&
                              spSnap->nCanvasWidth == stGeomNew.dstWidth() &&
                              spSnap->nCanvasHeight == stGeomNew.dstHeight() &&
                              spSnap->nRotate == stGeomNew.rotationOf() &&
                              spSnap->enMirror == stGeomNew.mirrorOf();
        /* 与快照全同才熄灯：换入成功（SWAPPED）或配置改回了与总线一致（SAME）都算对上 */
        m_bGeomStale = !bMatched;
        if (GeomSwap_E::SWAPPED == enSwap)
        {
            dlog_info("classroom engine: 几何热切换到画布 %dx%d (mirror=%d rotate=%d); 区域表下一帧重建, 在途帧仍用自己的几何",
                      stGeomNew.dstWidth(), stGeomNew.dstHeight(),
                      static_cast<int>(stGeomNew.mirrorOf()), stGeomNew.rotationOf());
        }
        else if (!bMatched)
        {
            dlog_warn("classroom engine: 几何配置已改但帧总线保持 %dx%d -> %dx%d (mirror=%d rotate=%d) 直到重启",
                      stGeomNew.srcWidth(), stGeomNew.srcHeight(), stGeomNew.dstWidth(),
                      stGeomNew.dstHeight(), static_cast<int>(stGeomNew.mirrorOf()),
                      stGeomNew.rotationOf());
        }
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_stStat.bGeomStale = m_bGeomStale;
        m_stStat.llConfigVersion = spSnap->llVersion;
    }

    dlog_info("classroom engine: 配置已生效 版本=%lld(原 %lld) 总开关=%d 行为=%d/%zu 模型组=%zu 需要组掩码=0x%x",
              static_cast<long long>(spSnap->llVersion), static_cast<long long>(llOldVersion),
              spSnap->bEnable ? 1 : 0, nOn, spSnap->vecGates.size(), spSnap->vecModels.size(),
              effectiveGroupMask(*spSnap, llNowMs));

    /* 换入后立刻落一份全量读数：现场动作就是"改个数看效果"，光靠 10s 心跳的字段拼不出
       当时到底生效了什么。这一行也让「touch 一下配置文件」成为一条不需要命令通道的自查手段 */
    dlog_info("classroom engine: 换入后状态(v%lld): %s",
              static_cast<long long>(spSnap->llVersion), statusJson().c_str());
}

void CClassroomEngine::syncModelDescs(const CConfigSnapshot_S &stSnap)
{
    for (const auto &stDesc : stSnap.vecModels)
    {
        CModelDesc_S stOld;
        if (!m_stPool.descOf(stDesc.strKey, stOld))
        {
            if (!m_stPool.registerModel(stDesc))
            {
                dlog_error("classroom engine: 注册模型 %s 失败", stDesc.strKey.c_str());
            }
            continue;
        }
        if (stOld.sameAs(stDesc))
        {
            continue;
        }

        if (BackendState_E::READY == m_stPool.stateOf(stDesc.strKey))
        {
            /* 换描述等于换那份已经算进预算的常驻内存，池子在驻留期是拒绝覆盖的。
               这里不硬卸：驻留中的模型可能正被推理线程用着，为了一次改路径去打断它不划算 */
            dlog_warn("classroom engine: 模型 %s 驻留中, %s -> %s 待该组释放后生效",
                      stDesc.strKey.c_str(), stOld.strModelPath.c_str(), stDesc.strModelPath.c_str());
            continue;
        }

        m_stPool.registerModel(stDesc);
        m_mapWarmState.erase(stDesc.strKey);    /* 状态口径变了，允许按新结果再报一次 */
        dlog_info("classroom engine: 模型 %s 描述已更新, group %s -> %s, path %s -> %s",
                  stDesc.strKey.c_str(), groupName(stOld.enGroup), groupName(stDesc.enGroup),
                  stOld.strModelPath.c_str(), stDesc.strModelPath.c_str());
    }

    /* 配置里删掉的键：引擎不再预热它。放掉租约让空闲巡检把内存收回来，
       在这里直接 unloadGroup 反而可能卸掉别组仍在用的模型 */
    for (const auto &stState : m_stPool.snapshot())
    {
        bool bKept = false;
        for (const auto &stDesc : stSnap.vecModels)
        {
            if (stDesc.strKey == stState.strKey)
            {
                bKept = true;
                break;
            }
        }
        if (bKept)
        {
            continue;
        }

        const size_t nErased = m_mapHeld.erase(stState.strKey);
        m_mapWarmState.erase(stState.strKey);
        if (nErased > 0)
        {
            /* 裁剪档跟着租约一起放（第 8 刀之后释放逐颗走 dropCropUnit，这一路是它漏下的最后一格）：
               档是按键存的，键从配置里消失后再没人查它，留着就是一块永不复用的 RGA 输入缓冲 */
            dropCropUnit(stState.strKey);
            dlog_info("classroom engine: 模型 %s 已从配置移除, 租约释放",
                      stState.strKey.c_str());
        }
    }

    std::lock_guard<std::mutex> stLock(m_mtxStat);
    m_stStat.u32HeldNum = static_cast<uint32_t>(m_mapHeld.size());
}

void CClassroomEngine::warmGroups(const CConfigSnapshot_S &stSnap, int64_t llNowMs)
{
    /* 复核需求续期。读的是规则层上一拍攒下的名单——那正是本拍感知段要消费的那一份，
       暖组在感知段之前跑，节拍顺序刚好把"先驻留、后使用"对上（§4.3 单 worker 串行链）。
       只在非空时续，不清零：名单空了靠到期时间自然回落，比每拍二值的开关少一处抖动 */
    if (!m_stRule.phoneRecheckIds().empty())
    {
        m_llPhoneWarmUntilMs = llNowMs + CLASSROOM_PHONE_WARM_HOLD_MS;
    }

    /* 板书侧那份闩：有采集请求挂着、或一次采集还在后端手里，就把"最近有活动"推到本拍。
       两条都要，缺一不可：pending 盖住"边沿刚进来、这一拍还没采"，in-flight 盖住"已经投出去、
       det + ≤N 次 rec 正在 ClsOcr 线程上跑"。少了后一条，保留期会在一次识别跑到一半时到期，
       而一次识别中途被放掉租约就是自己掐掉它——后端是每次推理各取一次租约，
       没有一份作业级的持有（见 board_ocr.h 的 D3：它只 peek 不 acquire 的理由同样在这条上） */
    if (m_stBoard.captureBusy())
    {
        m_llBoardLastActMs = llNowMs;
    }

    const CWarmNeed_S stNeed = warmNeedOf();
    const uint32_t u32Want = warmEffectiveGroupMask(stSnap, llNowMs, stNeed);
    const uint32_t u32Models = warmResidentModelMask(stSnap, llNowMs, stNeed, u32Want);

    if (u32Want != m_u32WantedMask || u32Models != m_u32ModelMask)
    {
        /* 逐颗放，不按组放。放租约必须在 unload 之前，否则 nUsers>0 会挡住卸载：这条顺序与按组放那版一样 */
        for (auto it = m_mapHeld.begin(); it != m_mapHeld.end();)
        {
            CModelDesc_S stDesc;
            /* descOf 查不到也放：池子里已经没有这颗的描述，留着租约等于留一份没人认领的驻留
               （按组放那一版不会走到这一格，因为它的条件是"组对得上"） */
            const bool bKeep = m_stPool.descOf(it->first, stDesc) &&
                               warmModelWanted(stSnap, llNowMs, stNeed, u32Want, stDesc);
            it = bKeep ? std::next(it) : releaseModel(it);
        }

        m_u32WantedMask = u32Want;
        m_u32ModelMask = u32Models;
        m_llLastWarmMs = 0;   /* 目标变了就立刻试一次，不等重试周期 */
        /* 不清 m_mapWarmState：它只为「状态变了才打日志」服务，而整表清空会让一颗长期
           缺件的模型每换一次片就重播一条 warning（轮转期是 8~20s，一天上万条）。
           保留表项也不会漏报：拒绝原因变了才打，比的是表里的旧值与本次的新值 */
    }

    if (0 == u32Want)
    {
        return;
    }
    if (!warmRetryDue(m_llLastWarmMs, llNowMs))
    {
        return;
    }
    m_llLastWarmMs = llNowMs;

    for (const auto &stDesc : stSnap.vecModels)
    {
        const uint32_t u32Bit = warmGroupBit(stDesc.enGroup);
        if (!warmModelWanted(stSnap, llNowMs, stNeed, u32Want, stDesc) ||
            m_mapHeld.find(stDesc.strKey) != m_mapHeld.end())
        {
            continue;
        }

        /* acquire 的等待发生在节拍线程：FastPose 实测 301ms，最坏 2s 超时，
           这条路径上的阻塞不会影响取流线程与推理线程 */
        BackendState_E enState = BackendState_E::DISABLED;
        CModelLease stLease = m_stPool.acquire(stDesc.strKey, enState,
                                               classroomNowMs() + CLASSROOM_WARM_WAIT_MS);
        if (!stLease.valid())
        {
            {
                std::lock_guard<std::mutex> stLock(m_mtxStat);
                ++m_stStat.u64WarmFail;
            }

            /* 拒绝原因变了才打这一行（判据与理由见 warmRefusalReport） */
            if (warmRefusalReport(m_mapWarmState, stDesc.strKey, enState))
            {
                dlog_warn("classroom engine: 预热 %s (%s 组) 被拒: %s",
                          stDesc.strKey.c_str(), groupName(stDesc.enGroup), stateName(enState));
            }

            if (warmRefuseArmsRotate(enState, u32Want, u32Bit, m_bNeedRotate))
            {
                /* 武装只改这几个字段，真正放掉另一组是下一拍 effectiveGroupMask 的事：
                   那一拍掩码变了，warmGroups 开头的释放分支才会动租约 */
                m_bNeedRotate = true;
                m_llRotateArmedMs = llNowMs;
                m_enHeavyTurn = (ModelGroup_E::GROUP_POSE == stDesc.enGroup)
                                    ? ModelGroup_E::GROUP_POSE : ModelGroup_E::GROUP_FACE;
                m_llHeavyUntilMs = classroomNowMs() + sliceOfGroup(m_enHeavyTurn);
                sliceBusySampleBegin();
                dlog_warn("classroom engine: %s 被拒 (%s) —— 两组重模型同时都要, "
                          "请开时间片轮转, 当前 turn=%s",
                          stDesc.strKey.c_str(), stateName(enState), groupName(m_enHeavyTurn));
            }
            continue;
        }

        m_mapHeld.emplace(stDesc.strKey, std::move(stLease));
        warmReadyAdopt(m_mapWarmState, stDesc.strKey);
        ensureCropUnit(stDesc, stSnap);

        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64WarmOk;
        m_stStat.u32HeldNum = static_cast<uint32_t>(m_mapHeld.size());
    }
}

void CClassroomEngine::sliceBusySampleBegin(void)
{
    m_llSliceBusySeenMs = 0;
    m_llSliceT0Ms = classroomNowMs();
    m_llSliceBusyPrevMs = m_spSched ? m_spSched->stat().llNpuBusyMs : -1;
}

void CClassroomEngine::rotateHeavyIfDue(int64_t llNowMs, const CConfigSnapshot_S &stSnap)
{
    if (!m_bNeedRotate)
    {
        return;
    }

    /* 片内占用的增量账（P1-5）：只在轮转开着时攒（两组同驻时没有片可言）。
       心跳把调度器的窗口账清过时（当前值 < 上一拍值），当前值整笔收进来——
       清零前后的忙毫秒都仍落在本片窗口里，这与 metrics() 各记各的互不干扰 */
    if (m_spSched)
    {
        const int64_t llBusyNow = m_spSched->stat().llNpuBusyMs;
        m_llSliceBusySeenMs += (llBusyNow >= m_llSliceBusyPrevMs)
                                   ? (llBusyNow - m_llSliceBusyPrevMs) : llBusyNow;
        m_llSliceBusyPrevMs = llBusyNow;
    }

    if (!warmRotateHasTarget(stSnap))
    {
        /* 只剩一个重资源组要了，轮转没有对象；把机会交回去，让 warmGroups 按原始目标走一次 */
        m_bNeedRotate = false;
        m_llHeavyUntilMs = 0;
        return;
    }

    /* 复议：轮转开着的时候被换下的那组根本不在期望集合里，我们再也问不到池子"现在两组
       装不装得下"，所以压力过去这件事只能靠定时重问获知。到点就关一次——本拍 warmGroups
       会按原始目标再要一遍：装得下就从此两组同驻（之后不会再有拒绝，轮转也就回不来了），
       装不下会在同一拍被重新武装并把本窗口重置 */
    if (llNowMs - m_llRotateArmedMs > CLASSROOM_ROTATE_RECONSIDER_MS)
    {
        m_bNeedRotate = false;
        m_llHeavyUntilMs = 0;
        dlog_info("classroom engine: 时间片已解除（历时 %lldms）, 重新向池子申请 "
                  "两组重模型",
                  static_cast<long long>(llNowMs - m_llRotateArmedMs));
        return;
    }

    if (llNowMs < m_llHeavyUntilMs)
    {
        return;
    }

    const ModelGroup_E enOut = m_enHeavyTurn;
    const ModelGroup_E enIn = (ModelGroup_E::GROUP_POSE == enOut) ? ModelGroup_E::GROUP_FACE
                                                                  : ModelGroup_E::GROUP_POSE;

    /* 片尾结算（P1-5）：本片的占用存档给换出组下一次用；换入组的片长按它自己
       上一片的存档调档（没量过 → -1 → 原样走配置值）。增量账在此一并清零开新片 */
    double dOutBusy = -1.0;
    if (m_spSched)
    {
        dOutBusy = heavySliceBusyPct(m_llSliceBusySeenMs, llNowMs - m_llSliceT0Ms);
    }
    m_ldHeavyLastBusyPct[(ModelGroup_E::GROUP_POSE == enOut) ? 0 : 1] = dOutBusy;
    const int64_t llSliceInMs = heavyAdaptSliceMs(
        sliceOfGroup(enIn), m_ldHeavyLastBusyPct[(ModelGroup_E::GROUP_POSE == enIn) ? 0 : 1]);

    releaseGroup(enOut);

    m_enHeavyTurn = enIn;
    m_llHeavyUntilMs = llNowMs + llSliceInMs;
    m_llLastWarmMs = 0;   /* 换入的新组本拍就要试，不等重试周期 */
    sliceBusySampleBegin();

    std::lock_guard<std::mutex> stLock(m_mtxStat);
    ++m_stStat.u64HeavySwitch;
    dlog_info("classroom engine: 重模型组时间片切换 %s -> %s (切出组占用 %.1f%%, "
              "下一片 %lldms)",
              groupName(enOut), groupName(enIn), dOutBusy, static_cast<long long>(llSliceInMs));
}

/* 放掉一颗模型（租约 + 裁剪档 + 驻留）。与 releaseGroup 的分工不是"大小"，是"同一组里还有没有别人要"：
   守护降档与轮转换片整组一起动，那两条仍走 releaseGroup；G_OTHER 里手机那颗与 OCR 那两颗
   的触发各自独立（3,676KB vs 135,820KB），只能逐颗放 */
ModelLeaseMap::iterator CClassroomEngine::releaseModel(ModelLeaseMap::iterator ritHeld)
{
    const std::string strKey = ritHeld->first;
    const ModelLeaseMap::iterator itNext = m_mapHeld.erase(ritHeld);   /* 先放租约：nUsers-- */
    dropCropUnit(strKey);
    const bool bUnloaded = m_stPool.unloadKey(strKey);

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_stStat.u32HeldNum = static_cast<uint32_t>(m_mapHeld.size());
    }
    if (!bUnloaded)
    {
        /* 没卸成只有一种可能：还有人正在推理它（池子的纪律是不从推理线程脚下抽句柄）。
           打出来是因为这一格的读数是"租约放了而内存没回来"，不解释就会被读成漏——
           而它下一次巡检（evictIdle）自然会收。卸成了不另外打字：池子自己那一行已经带了键 */
        dlog_info("classroom engine: %s 租约已撤销但仍在驻留（使用中）", strKey.c_str());
    }
    return itNext;
}

void CClassroomEngine::releaseGroup(ModelGroup_E enGroup)
{
    int32_t nDropped = 0;
    for (auto it = m_mapHeld.begin(); it != m_mapHeld.end();)
    {
        CModelDesc_S stDesc;
        if (m_stPool.descOf(it->first, stDesc) && stDesc.enGroup == enGroup)
        {
            it = m_mapHeld.erase(it);       /* 析构租约 = nUsers-- */
            ++nDropped;
        }
        else
        {
            ++it;
        }
    }

    dropCropUnits(enGroup);
    const int32_t nUnloaded = m_stPool.unloadGroup(enGroup);

    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_stStat.u32HeldNum = static_cast<uint32_t>(m_mapHeld.size());
    }
    /* 空放不打字：每换一次片，被换下的那组会被 releaseGroup 调用两次（本函数一次、
       warmGroups 的 drop 再一次），第二次的读数永远是 0，两行连打看着像放了两组 */
    if (nDropped > 0 || nUnloaded > 0)
    {
        dlog_info("classroom engine: 模型组 %s 已释放，收回租约 %d 个，卸载模型 %d 个",
                  groupName(enGroup), nDropped, nUnloaded);
    }
}

void CClassroomEngine::ensureCropUnit(const CModelDesc_S &stDesc, const CConfigSnapshot_S &stSnap)
{
    /* 这一路的档按 **align.crop_px** 而不是模型输入尺寸 */
    uint32_t u32W = stDesc.u32InputW;
    uint32_t u32H = stDesc.u32InputH;
    if (CLASSROOM_MODEL_KEY_FACE_FEATURE == stDesc.strKey && stSnap.nAlignCropPx > 0)
    {
        u32W = u32H = static_cast<uint32_t>(stSnap.nAlignCropPx);
    }

    std::lock_guard<std::mutex> stLock(m_mtxCrop);
    const auto cit = m_mapCrop.find(stDesc.strKey);
    if (cit != m_mapCrop.end())
    {
        if (cit->second && cit->second->inputWidth() == u32W && cit->second->inputHeight() == u32H)
        {
            return;
        }
        /* 尺寸变了才重建（换了就换整个对象，不在原档上改尺寸：档内的缓冲是按旧尺寸分配的）。
           正在被感知借用的那块由 shared_ptr 计数续命，这里放掉的只是地图上的一个引用 */
        dlog_info("classroom engine: %s 的裁剪池重建 %ux%u -> %ux%u",
                  stDesc.strKey.c_str(), cit->second ? cit->second->inputWidth() : 0u,
                  cit->second ? cit->second->inputHeight() : 0u, u32W, u32H);
        m_mapCrop.erase(cit);
    }

    /* 只按要裁的尺寸建档；抓拍/板书的整帧 RGB 另建整帧档 */
    /* letterbox（#93-1）只给两路**整帧检测**档：head/face。ROI 档（姿态/复核/对齐/
       板书）保持拉伸——框形状随目标走，pad 只会稀释有效分辨率；键名单列而不是按组判，
       是为了将来新增整帧路时必须显式决策一次，而不是被默认值悄悄带上 */
    const bool bLetterbox = letterboxForModel(stSnap, stDesc.strKey);
    const CropUnitPtr spCrop = std::make_shared<CCropUnit>();
    if (!spCrop->setup(u32W, u32H, CROP_BUF_NUM, stSnap.enModelChannel, bLetterbox))
    {
        dlog_error("classroom engine: %s 裁剪池建立失败 %ux%u",
                   stDesc.strKey.c_str(), u32W, u32H);
        return;
    }

    m_mapCrop.emplace(stDesc.strKey, spCrop);
}

void CClassroomEngine::dropCropUnits(ModelGroup_E enGroup)
{
    std::lock_guard<std::mutex> stLock(m_mtxCrop);
    for (auto it = m_mapCrop.begin(); it != m_mapCrop.end();)
    {
        CModelDesc_S stDesc;
        if (m_stPool.descOf(it->first, stDesc) && stDesc.enGroup == enGroup)
        {
            it = m_mapCrop.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CClassroomEngine::dropCropUnit(const std::string &strKey)
{
    std::lock_guard<std::mutex> stLock(m_mtxCrop);
    m_mapCrop.erase(strKey);
}

bool CClassroomEngine::groupHeld(ModelGroup_E enGroup) const
{
    for (const auto &pair : m_mapHeld)
    {
        CModelDesc_S stDesc;
        if (m_stPool.descOf(pair.first, stDesc) && stDesc.enGroup == enGroup)
        {
            return true;
        }
    }
    return false;
}

void CClassroomEngine::guardSample(int64_t llNowMs)
{
    if (llNowMs - m_llLastGuardMs < CLASSROOM_GUARD_PERIOD_MS)
    {
        return;
    }
    m_llLastGuardMs = llNowMs;

    const CMemInfo_S stMem = classroomQueryMem();
    if (!stMem.bValid)
    {
        /* 采样失败时保持现状：拿不到水位就乱降级，会把一次 /proc 抖动放大成功能开关 */
        dlog_warn("classroom engine: 内存采样失败，守护档位保持 %s", levelName(m_enLevel));
        return;
    }

    const int64_t llReserveKb = m_stPool.param().llReserveKb;
    GuardLevel_E enRaw = GuardLevel_E::GL_NORMAL;
    if (llReserveKb > 0)
    {
        const double ldAvail = static_cast<double>(stMem.llMemAvailableKb);
        if (ldAvail < static_cast<double>(llReserveKb))
        {
            enRaw = GuardLevel_E::GL_EMERGENCY;
        }
        else if (ldAvail <= static_cast<double>(llReserveKb) * 1.05)
        {
            enRaw = GuardLevel_E::GL_HARD;
        }
        else if (ldAvail <= static_cast<double>(llReserveKb) * 1.20)
        {
            enRaw = GuardLevel_E::GL_SOFT;
        }
        else if (ldAvail >= static_cast<double>(llReserveKb) * GUARD_RECOVER_RATIO)
        {
            enRaw = GuardLevel_E::GL_NORMAL;
        }
        else
        {
            /* 处于 1.20~1.50 之间：这是刻意留出的迟滞带，档位不变 */
            enRaw = m_enLevel;
        }
    }

    if (enRaw > m_enLevel)
    {
        /* 升档立即生效：内存紧张时每多留一个模型就多一次 OOM 机会 */
        m_enLevel = enRaw;
        m_nRecoverStreak = 0;
        applyGuardLevel(m_enLevel);
    }
    else if (enRaw < m_enLevel)
    {
        /* 降档要连续观察：水位在阈值附近来回跳时，阶梯反复横跳比停在低档更伤
           （一次模型装卸是百 ms + 百 MB 级的内存脉冲） */
        if (++m_nRecoverStreak >= GUARD_RECOVER_SAMPLES)
        {
            m_enLevel = static_cast<GuardLevel_E>(static_cast<int32_t>(m_enLevel) - 1);
            m_nRecoverStreak = 0;
            applyGuardLevel(m_enLevel);
        }
    }
    else
    {
        m_nRecoverStreak = 0;
    }

    std::lock_guard<std::mutex> stLock(m_mtxStat);
    m_stStat.llLastRssKb = stMem.llVmRssKb;
    m_stStat.llLastAvailKb = stMem.llMemAvailableKb;
    m_stStat.llLastMmzKb = stMem.llMmzUsedKb;
    if (stMem.llVmRssKb > m_stStat.llRssMaxKb)
    {
        m_stStat.llRssMaxKb = stMem.llVmRssKb;
    }
    if (0 == m_stStat.llAvailMinKb || stMem.llMemAvailableKb < m_stStat.llAvailMinKb)
    {
        m_stStat.llAvailMinKb = stMem.llMemAvailableKb;
    }
    if (stMem.llThreadNum > static_cast<int64_t>(m_stStat.u32ThreadNumMax))
    {
        m_stStat.u32ThreadNumMax = static_cast<uint32_t>(stMem.llThreadNum);
    }
}

void CClassroomEngine::applyGuardLevel(GuardLevel_E enNew)
{
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_stStat.enLevel = enNew;
        m_stStat.nSampleStretchPercent = stretchOfLevel(enNew);
        m_stStat.bNeedRotate = m_bNeedRotate;
        ++m_stStat.u64LevelChange;
    }

    /* 只改档位；租约的收放在 warmGroups 里跟着目标集合走。
       分开的代价是多等一拍（≤100ms），换来的是全类只有一个地方会改 m_mapHeld */
    if (enNew >= GuardLevel_E::GL_EMERGENCY)
    {
        /* 应急档起 ruleStep 就不再喂样（见那里的判断），不在此刻收尾就等于
           留一批「拉到底永不结束」的事件在平台上。本函数只在档位真的变了才被调，
           最快也是一拍守护周期（5s）一次，不像每拍的判定那样会把日志刷满 */
        m_stRule.shutdownActive(classroomNowMs());
    }

    dlog_warn("classroom engine: 守护档位 -> %s (avail=%lldKB reserve=%lldKB rss=%lldKB mmz=%lldKB stretch=%d%%)",
              levelName(enNew),
              static_cast<long long>(m_stStat.llLastAvailKb),
              static_cast<long long>(m_stPool.param().llReserveKb),
              static_cast<long long>(m_stStat.llLastRssKb),
              static_cast<long long>(m_stStat.llLastMmzKb),
              stretchOfLevel(enNew));
}

void CClassroomEngine::metrics(int64_t llNowMs)
{
    if (llNowMs - m_llLastMetricMs < CLASSROOM_METRIC_PERIOD_MS)
    {
        return;
    }
    m_llLastMetricMs = llNowMs;

    /* 采集一律在锁外：这些对象各自有锁，持着状态锁去拿它们的锁会平白引入一条锁序 */
    const CFrameBusStat_S stBus = m_stBus.stat();
    const CFeatureBusStat_S stFeat = m_stFeatureBus.stat();
    const CModelPoolStat_S stPool = m_stPool.stat();
    const double ldNpuBusy = m_spSched ? m_spSched->npuBusyPercent() : 0.0;
    if (m_spSched)
    {
        /* 读完就归零：不归零的话这里是「自调度器启动以来的累计平均」——开机那一波加载推理
           会被摊薄到之后每一个心跳里，心跳上看不出忙闲变化，而姿态段的启停迟滞
          （perceive_pipeline::poseWantNum）读的正是这个值，失灵方向是「该让路时不让路」 */
        m_spSched->resetNpuBudget();
    }
    const CEngineStat_S stOut = stat();

    /* 感知读数只在这里取：metrics() 与感知层写计数是同一条线程（tickOnce 的全部出口都调 metrics），
       换任何线程读那组无锁 uint64 都会在 32 位机上撕裂成半新半旧的值。
       取到就缓存，statusJson 读缓存，因此对外看到的感知段最多滞后一个心跳周期 */
    std::string strPerceive;
    if (m_fnPerceiveStat)
    {
        strPerceive = m_fnPerceiveStat();
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        m_strPerceiveDetail = strPerceive;
    }
    else
    {
        strPerceive = "-";   /* 未注入取串函数：保持键位在，便于「这一段没数据」与「忘了挂」区分 */
    }

    /* 规则段的线程安全与感知段不同，因此写法也不同：CRuleEngine 每拍末尾把无锁工作计数
       抄进发布件（publishStat），statLine/statJson 只取发布件且自带一次加锁——所以这里
       直接调即可，statusJson 在命令线程直接调也可以，不必像感知那样先缓存一份副本 */
    const std::string strRule = m_stRule.statLine();

    /* 群体聚合段与规则段同一线程域（都在节拍线程里被取），因此可以直接调 statLine()：
       它内部只加一次自己的锁、拷贝一行串，不存在 32 位机上读无锁 64 位计数的问题 */
    const std::string strData = m_stData.statLine();

    /* 归属段排在规则段前面，因为读它的顺序就是这个：教师那 11 列整段不出时，
       第一个要看的不是阈值而是 cfg=0/0（讲台与学生席两块角色区域都没画）——
       那一眼能看出"没配"与"配了但判不出"（tea=0 cand>0 才是阈值问题） */
    const std::string strSubject = m_stRule.subjects().statLine();

    /* 滞留环同理。这一段的存在理由：断网期间"数据到底还在不在"只能靠它回答，
       光看 out= 那一列分不清"发出去了"与"发不出去但落盘了" */
    const std::string strSpool = m_stSpool.statLine();

    /* 考勤段可以直接取：账本只在节拍线程被写，而 metrics 就跑在那条线程上。
       串里刻意不带任何姓名——心跳日志会随文件被导出到设备外，学生姓名是个人信息 */
    const std::string strAttend = m_stAttend.statText();
    /* 热力段与考勤段同一线程域、同一条心跳。它给的全是"为什么这张图是空的"那几项计数
       （没画席区 / 网格没建起来 / 位置换算没出数），一个姓名都不会出现在这里 */
    const std::string strSeat = m_stSeat.statText();
    /* 先验段紧跟热力段：这两段合起来才是"座位这一条链跑得对不对"，分开读会误判。
       它在 enable=false 时照样出——这张表在关着的时候仍在折样，那六个计数就是"该不该开"的证据。
       一个座位号、一个坐标、一个姓名都不会出现在这里（心跳会被导出到设备外） */
    const std::string strPrior = m_stPrior.statText();
    /* 板书段同理只在节拍线程读。它给的是"这一路为什么没采"的分解：没边沿（ep=0）、
       被下限压住（ep>want）、没画黑板（noreg）、没有 OCR 能力（unav）——四件事的处置各不相同 */
    /* 轨迹段与热力段同一线程域：它给的是"这条链有没有在偷偷截断"（evict/thin/nopos
       各自的当期与累计），一个跟踪号、一个坐标都不会出现在这里（心跳会被导出到设备外） */
    const std::string strStuTrack = m_stStu.statText();
    const std::string strBoard = m_stBoard.statText();
    const std::string strScene = m_stScene.statText();

    /* 底库身份与人数也在心跳给，而**不进 statusJson**：m_spGallery 会被 applyGallery 换掉，
       那是节拍线程的动作，而 statusJson 从命令线程来——这里读的是同一条线程刚换完的那一份，
       不需要锁也不会有两种答案。要跨线程给就得像感知段那样先缓存副本，为两个数不值当 */
    const int32_t nEnroll = m_spGallery->enrollNum();

    /* slice= 给的是"这一拍两组重组是不是只持着一组"。原先这件事只能去数 "slice switched"
       的行数，而驻留量在 28800/128128KB 之间来回看着像正常波动：P7-f 那条自锁就是这么漏的。
       m_enHeavyTurn 就是 effectiveGroupMask 留下的那一组，报它自己的名字 */
    const char *pSlice = m_bNeedRotate ? groupName(m_enHeavyTurn) : "-";

    /* 心跳自己的读数排在滞留环之后、detail 之前：mlog= 说的就是这一行落在哪儿，
       读的人先看到数据、再看到"这份数据的存放地点与还能回溯多久" */
    const std::string strMetric = m_stMetricLog.statLine(llNowMs);

    /* 标量列与合段列分两条串拼，是为了让"回抄进业务日志的那半行"**必然**是整行的前缀：
       两条各自 printf 出来的串会漂（改一边忘另一边是迟早的事），
       而 head 只有做成 line 的前缀，"两处读数对不上"这一类问题才在结构上不可能出现 */
    const std::string strHead = fmtMetricLine(
        "classroom metrics: up=%llds level=%s role=%s bin=%s frames=%llu pub=%llu drop=%llu|%llu|%llu "
        "perceive=%llu|%llu|%llu feat_pub=%llu resident=%lldKB/%d held=%u slice=%s load=%llu|%llu|%llu "
        "npu=%.1f%% queue=%u rss=%lldKB(+%lld) avail=%lldKB(min %lld) mmz=%lldKB thr=%u ver=%lld",
        static_cast<long long>(llNowMs - m_llBootMs) / 1000, levelName(m_enLevel),
        /* 设备角色（#126）：运维下发 role 后第一眼要能确认"这台机器现在按哪个角色在跑"，
           而不是去翻行为开关逐格推断。快照未就绪给 "-"（与未注入的感知段同一口径） */
        (m_stConfig.current() ? deviceRoleName(m_stConfig.current()->enRole) : "-"),
        /* 构建号（#95）：ver= 是配置 mtime，改一次配置就变，区分不了「换了程序」和
           「换了配置」。编译期注入（classroom_define.h），同一次构建内恒定，
           换件验收在心跳上就能读出"跑的是哪个构建"。空格换 '-'：心跳列按空白切列，
           带空格的值会把 log_ring.sh 一族的读数脚本全撑破 */
        binBuildColumn(),
        static_cast<unsigned long long>(stOut.u64FramesIn),
        static_cast<unsigned long long>(stBus.u64Published),
        static_cast<unsigned long long>(stBus.u64DroppedNoSlot),
        static_cast<unsigned long long>(stOut.u64FramesRejected),
        static_cast<unsigned long long>(stOut.u64FramesThrottled),
        static_cast<unsigned long long>(stOut.u64PerceiveRounds),
        static_cast<unsigned long long>(stOut.u64PerceiveFail),
        static_cast<unsigned long long>(stOut.u64PerceiveFailEdge),
        static_cast<unsigned long long>(stFeat.u64Published),
        static_cast<long long>(stPool.llResidentKb), stPool.nResidentNum,
        stOut.u32HeldNum,
        pSlice,
        static_cast<unsigned long long>(stPool.u64LoadOk),
        static_cast<unsigned long long>(stPool.u64LoadFail),
        static_cast<unsigned long long>(stPool.u64LoadFailCooling),
        ldNpuBusy, m_spSched ? m_spSched->pending() : 0u,
        static_cast<long long>(stOut.llLastRssKb),
        static_cast<long long>(stOut.llRssMaxKb - stOut.llRssBaseKb),
        static_cast<long long>(stOut.llLastAvailKb),
        static_cast<long long>(stOut.llAvailMinKb),
        static_cast<long long>(stOut.llLastMmzKb),
        stOut.u32ThreadNumMax, static_cast<long long>(stOut.llConfigVersion));

    const std::string strTail = fmtMetricLine(
        "subj=%s rules=%s data=%s gal=%s:%d att=%s seat=%s sprior=%s strk=%s %s %s out=%llu|%llu|%llu "
        "spool=%s %s detail=%s",
        strSubject.c_str(), strRule.c_str(), strData.c_str(),
        m_spGallery->name(), nEnroll, strAttend.c_str(), strSeat.c_str(), strPrior.c_str(),
        strStuTrack.c_str(), strBoard.c_str(), strScene.c_str(),
        static_cast<unsigned long long>(stOut.u64UploadOk),
        static_cast<unsigned long long>(stOut.u64UploadFail),
        static_cast<unsigned long long>(stOut.u64SessionClose),
        strSpool.c_str(), strMetric.c_str(), strPerceive.c_str());

    /* 落这一行的同时把 mlog= 那一段的账记上：走自己的文件时整行只落一处，
       业务日志里按 head 周期出现一条紧凑头，两条加起来才是"现场能看到的东西" */
    m_stMetricLog.write(strHead + " " + strTail, strHead, llNowMs);
}

std::string CClassroomEngine::statusJson() const
{
    const CEngineStat_S stStat = stat();
    const ConfigPtr spSnap = m_stConfig.current();
    const CFrameBusStat_S stBus = m_stBus.stat();
    const CFeatureBusStat_S stFeat = m_stFeatureBus.stat();
    const CModelPoolStat_S stPool = m_stPool.stat();
    const CInferSchedulerStat_S stSched = m_spSched ? m_spSched->stat() : CInferSchedulerStat_S();
    const std::vector<CModelState_S> vecModel = m_stPool.snapshot();
    const CMemInfo_S stMem = classroomQueryMem();
    const CModelPoolParam_S stParam = m_stPool.param();

    std::string strJson;
    strJson.reserve(2688 + vecModel.size() * 128);

    appendFmt(strJson, "{\"running\":%s,\"up_ms\":%lld,\"config_version\":%lld,\"guard\":\"%s\","
                       "\"degraded\":%s,\"geom_stale\":%s,\"rotate\":%s,\"sample_stretch\":%d",
              stStat.bRunning ? "true" : "false",
              static_cast<long long>(stStat.llUpMs),
              static_cast<long long>(stStat.llConfigVersion),
              levelName(stStat.enLevel),
              (GuardLevel_E::GL_NORMAL != stStat.enLevel) ? "true" : "false",
              stStat.bGeomStale ? "true" : "false",
              stStat.bNeedRotate ? "true" : "false",
              stStat.nSampleStretchPercent);

    appendFmt(strJson, ",\"frames\":{\"in\":%llu,\"published\":%llu,\"drop_no_slot\":%llu,"
                       "\"rejected\":%llu,\"throttled\":%llu,\"busy_max\":%u,\"slots\":%u}",
              static_cast<unsigned long long>(stStat.u64FramesIn),
              static_cast<unsigned long long>(stBus.u64Published),
              static_cast<unsigned long long>(stBus.u64DroppedNoSlot),
              static_cast<unsigned long long>(stStat.u64FramesRejected),
              static_cast<unsigned long long>(stStat.u64FramesThrottled),
              stBus.u32BusySlotsMax, stBus.u32Slots);

    appendFmt(strJson, ",\"feature\":{\"published\":%llu,\"perceive_rounds\":%llu,\"perceive_fail\":%llu,"
                       "\"perceive_fail_edge\":%llu,"
                       "\"drop_no_slot\":%llu,\"truncated\":%llu,\"person_peak\":%u,\"busy_max\":%u}",
              static_cast<unsigned long long>(stFeat.u64Published),
              static_cast<unsigned long long>(stStat.u64PerceiveRounds),
              static_cast<unsigned long long>(stStat.u64PerceiveFail),
              static_cast<unsigned long long>(stStat.u64PerceiveFailEdge),
              static_cast<unsigned long long>(stFeat.u64DroppedNoSlot),
              static_cast<unsigned long long>(stFeat.u64TruncatedFrames),
              stFeat.u32PersonPeak, stFeat.u32BusyMax);

    appendFmt(strJson, ",\"pool\":{\"resident_kb\":%lld,\"resident_num\":%d,\"held\":%u,"
                       "\"load_ok\":%llu,\"load_fail\":%llu,\"load_fail_cooling\":%llu,\"load_discard\":%llu,\"unload\":%llu,\"reject_budget\":%llu,"
                       "\"reject_slice\":%llu,"
                       "\"reject_missing\":%llu,\"group_switch\":%llu,\"last_load_ms\":%lld,"
                       "\"reserve_kb\":%lld,\"idle_unload_ms\":%lld}",
              static_cast<long long>(stPool.llResidentKb), stPool.nResidentNum,
              stStat.u32HeldNum,
              static_cast<unsigned long long>(stPool.u64LoadOk),
              static_cast<unsigned long long>(stPool.u64LoadFail),
              static_cast<unsigned long long>(stPool.u64LoadFailCooling),
              static_cast<unsigned long long>(stPool.u64LoadDiscard),
              static_cast<unsigned long long>(stPool.u64Unload),
              static_cast<unsigned long long>(stPool.u64RejectBudget),
              static_cast<unsigned long long>(stPool.u64RejectSlice),
              static_cast<unsigned long long>(stPool.u64RejectMissing),
              static_cast<unsigned long long>(stPool.u64GroupSwitch),
              static_cast<long long>(stPool.llLastLoadMs),
              static_cast<long long>(stParam.llReserveKb),
              static_cast<long long>(stParam.llIdleUnloadMs));

    appendFmt(strJson, ",\"npu\":{\"done_ok\":%llu,\"dropped_stale\":%llu,\"dropped_unready\":%llu,"
                       "\"dropped_full\":%llu,\"backend_fail\":%llu,\"queue\":%u,\"queue_peak\":%u,"
                       "\"busy_percent\":%.1f}",
              static_cast<unsigned long long>(stSched.u64DoneOk),
              static_cast<unsigned long long>(stSched.u64DroppedStale),
              static_cast<unsigned long long>(stSched.u64DroppedUnready),
              static_cast<unsigned long long>(stSched.u64DroppedFull),
              static_cast<unsigned long long>(stSched.u64BackendFail),
              m_spSched ? m_spSched->pending() : 0u,
              stSched.u32QueuePeak,
              m_spSched ? m_spSched->npuBusyPercent() : 0.0);

    appendFmt(strJson, ",\"mem\":{\"rss_kb\":%lld,\"rss_base_kb\":%lld,\"rss_max_kb\":%lld,"
                       "\"avail_kb\":%lld,\"avail_min_kb\":%lld,\"mmz_kb\":%lld,"
                       "\"threads\":%lld,\"threads_max\":%u}",
              static_cast<long long>(stMem.llVmRssKb),
              static_cast<long long>(stStat.llRssBaseKb),
              static_cast<long long>(stStat.llRssMaxKb),
              static_cast<long long>(stMem.llMemAvailableKb),
              static_cast<long long>(stStat.llAvailMinKb),
              static_cast<long long>(stMem.llMmzUsedKb),
              static_cast<long long>(stMem.llThreadNum), stStat.u32ThreadNumMax);

    /* 感知段取的是节拍线程缓存下来的串（见 CPerceiveStatFn）：这里锁的只是这份副本，
       绝不在 statusJson 里现取感知层的计数器——那组数无锁，跨线程读在 32 位机上会撕裂。
       代价是这一段的读数最多滞后一个心跳周期（10s），换到的是不会读出错值 */
    std::string strPerceive;
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        strPerceive = m_strPerceiveDetail;
    }
    if (!strPerceive.empty())
    {
        /* 串按约定只含 key=value 与数字（无引号、无反斜杠），故直接作 JSON 字符串值给出，
           不做转义。要往里塞自由文本之前，先在这里补转义 */
        appendFmt(strJson, ",\"perceive_detail\":\"%s\"", strPerceive.c_str());
    }

    /* 归属段：与心跳 subj= 同一份发布件（仲裁器每拍末尾抄一次），命令线程读到的是上一拍的
       完整状态。这一段是「教师行为为什么整段不出」的第一现场：podium_configured=false
       与 promote=0 同时出现就是区域没画，不是判据太严 */
    appendFmt(strJson, ",\"subject\":%s", m_stRule.subjects().statJson().c_str());
    /* 规则段是一个嵌套 JSON 对象而非 key=value 串，故不并进上面的缓存去：statJson 内部
       只读发布件（节拍线程每拍末尾抄一次），命令线程调它拿到的是上一拍的完整快照 */
    appendFmt(strJson, ",\"rules\":%s", m_stRule.statJson().c_str());
    /* 聚合段同理：CDataCenter 的 statJson 自带锁并返回一份完整对象，不取工作计数 */
    appendFmt(strJson, ",\"data\":%s", m_stData.statJson().c_str());

    /* 心跳落点走的是与 perceive_detail 同一套跨线程口径：sink 的计数只有节拍线程能安全读，
       这里取的是 metricApply 发布下来的那份副本，最多滞后一次下发的时机。
       lines/fb/head/keep 这几个逐拍计数刻意**不**进 statusJson——它们本来就在心跳里，
       要在这里也给出，就得为两个没人轮询的数在 sink 里补一把锁 */
    std::string strMetricHow;
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        strMetricHow = m_strMetricHow;
    }

    /* 出口段把「有没有通道」「成/败/收尾」与**生效的**上报周期、会话上限并排给出去：
       现场最常见的误判是"配置改了没动"，而三样东西分属配置、引擎、聚合中心三处，
       只看其中任一处都可能得出相反结论。生效值不回落编译期默认 —— 快照还没换入时
       这里就该是 0，运维读到 report_interval_sec:0 配 upload:false，得到的是
       "引擎还在空跑"这一条真相，而不是"谁把参数配错了" */
    /* 考勤段：obs 是账本实际收到的观测条数，
       region_skip 是按区域挡掉的行数，region_cfg_err 是"区域引用坏了"的次数。
       三个数放在 statusJson 里而不是只进心跳：回放/离线复算只看得到 statusJson，
       而"区域到底有没有生效"正是回放时最常要回答的问题 */
    {
        const CAttendStat_S stAttendStat = m_stAttend.stat();
        appendFmt(strJson, ",\"attend\":{\"obs\":%llu,\"region_skip\":%llu,\"region_cfg_err\":%llu}",
                  static_cast<unsigned long long>(stAttendStat.u64Obs),
                  static_cast<unsigned long long>(m_stAttendRegionSkip.load(std::memory_order_relaxed)),
                  static_cast<unsigned long long>(m_u64AttendRegionCfgErr.load(std::memory_order_relaxed)));
    }

    appendFmt(strJson, ",\"egress\":{\"upload\":%s,\"upload_ok\":%llu,\"upload_fail\":%llu,"
                       "\"evt_alarm\":{\"wired\":%s,\"attempt\":%llu,\"ok\":%llu,\"fail\":%llu},"
                       "\"session_close\":%llu,\"report_interval_sec\":%d,\"session_max_sec\":%d,"
                       "\"spool\":%s,\"metric_log\":\"%s\"}",
              stStat.bUploadWired ? "true" : "false",
              static_cast<unsigned long long>(stStat.u64UploadOk),
              static_cast<unsigned long long>(stStat.u64UploadFail),
              m_fnEventAlarm ? "true" : "false",
              static_cast<unsigned long long>(m_u64EvtAlarmAttempt.load(std::memory_order_relaxed)),
              static_cast<unsigned long long>(m_u64EvtAlarmOk.load(std::memory_order_relaxed)),
              static_cast<unsigned long long>(m_u64EvtAlarmFail.load(std::memory_order_relaxed)),
              static_cast<unsigned long long>(stStat.u64SessionClose),
              spSnap ? spSnap->stEgress.nReportIntervalSec : 0,
              spSnap ? spSnap->stEgress.nSessionMaxSec : 0,
              m_stSpool.statJson().c_str(), strMetricHow.c_str());

    /* 统计推送段（0x6200/0x6201/0x6202）：wired 是网关有没有把 SDK 通道接上，
       session_open 是"这一节课的账本还开着"，lost 是队列溢出丢掉的条数。
       平台侧"某段时间没有段/预警"时，先看这里：wired=false 是没接、lost>0 是发得太慢，
       两者与"判据没触发"是三种完全不同的故障，不能混为一谈 */
    {
        const CStatsHubStat_S stStatsStat = m_stStatsHub.stat();
        appendFmt(strJson, ",\"stats\":{\"wired\":%s,\"session_open\":%s,\"session_id\":%lld,"
                           "\"counter_msgs\":%llu,\"seg_msgs\":%llu,\"alert_msgs\":%llu,"
                           "\"seg_records\":%llu,\"alert_records\":%llu,\"fail\":%llu,"
                           "\"seg_lost\":%llu,\"alert_lost\":%llu,\"seg_q\":%d,\"alert_q\":%d}",
                  m_stStatsHub.pushWired() ? "true" : "false",
                  stStatsStat.bSessionOpen ? "true" : "false",
                  static_cast<long long>(stStatsStat.llSessionId),
                  static_cast<unsigned long long>(stStatsStat.u64CounterMsgs),
                  static_cast<unsigned long long>(stStatsStat.u64SegMsgs),
                  static_cast<unsigned long long>(stStatsStat.u64AlertMsgs),
                  static_cast<unsigned long long>(stStatsStat.u64SegRecords),
                  static_cast<unsigned long long>(stStatsStat.u64AlertRecords),
                  static_cast<unsigned long long>(stStatsStat.u64PushFail),
                  static_cast<unsigned long long>(stStatsStat.u64SegLost),
                  static_cast<unsigned long long>(stStatsStat.u64AlertLost),
                  stStatsStat.nSegQueued, stStatsStat.nAlertQueued);
    }

    strJson.append(",\"models\":[");
    for (size_t i = 0; i < vecModel.size(); ++i)
    {
        const CModelState_S &stItem = vecModel[i];
        appendFmt(strJson, "%s{\"key\":\"%s\",\"group\":\"%s\",\"state\":\"%s\",\"resident\":%s,"
                           "\"users\":%d,\"kb\":%lld}",
                  (0 == i) ? "" : ",", stItem.strKey.c_str(), groupName(stItem.enGroup),
                  stateName(stItem.enState), stItem.bResident ? "true" : "false",
                  stItem.nUsers, static_cast<long long>(stItem.llResidentKb));
    }
    strJson.append("]");

    if (spSnap)
    {
        int32_t nOn = 0;
        for (const auto &stGate : spSnap->vecGates)
        {
            nOn += stGate.bEnable ? 1 : 0;
        }
        appendFmt(strJson, ",\"conf\":{\"enable\":%s,\"behaviors_on\":%d,\"behaviors_num\":%zu,"
                           "\"models\":%zu,\"src\":\"%dx%d\",\"canvas\":\"%dx%d\",\"mirror\":%d,"
                           "\"rotate\":%d,\"channel\":\"%s\"}",
                  spSnap->bEnable ? "true" : "false", nOn, spSnap->vecGates.size(),
                  spSnap->vecModels.size(), spSnap->nSrcWidth, spSnap->nSrcHeight,
                  spSnap->nCanvasWidth, spSnap->nCanvasHeight,
                  static_cast<int>(spSnap->enMirror), spSnap->nRotate,
                  (ChannelOrder_E::CH_BGR == spSnap->enModelChannel) ? "bgr" : "rgb");
    }

    strJson.append("}");
    return strJson;
}

std::string CClassroomEngine::reportJson() const
{
    /* 课程报告页（GET 2952）只要六组实时读数 */
    CAttendSnap_S stAtt;
    m_stAttend.snapshot(stAtt);
    const CSessionAccum_S stSess = m_stData.session();

    CPanelReport_S stPanel;
    stPanel.nExpectedNum = stAtt.nExpectedNum;   /* -1 = 名单没下发，网页显示"--" */
    stPanel.nPresentNum = stAtt.nPresentNum;     /* -1 = 没在考勤 */
    /* 列序走 CDurationGate 的那一张表：101~112 / 201~213 只定义过一次，
       这里不重抄号段；报文里的键名由 panelReportJson 按同一列序取 */
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        stPanel.au64BehaviorBegin[nCol] = stSess.u64BeginAt[nCol];
    }
    for (int nExpr = 0; nExpr < CLASSROOM_EXPRESSION_CLASS_NUM; ++nExpr)
    {
        stPanel.au64ExprNum[nExpr] = stSess.u64ExprNum[nExpr];
    }
    return panelReportJson(stPanel);
}
}
