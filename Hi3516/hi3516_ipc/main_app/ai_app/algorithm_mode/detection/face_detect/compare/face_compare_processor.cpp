/**
 * @FilePath     : face_compare_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸比对处理器（原生实现）——best 帧提特征、全库余弦、统一判定与事件联动。
 */

#include "face_compare_processor.hpp"

#if CAP_AI_FACE_COMPARE

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include "capture_ctrl.h"
#include "email_manage.h"
#include "event_configure.h"
#include "face_detect_worker.hpp"
#include "face_manage.h"
#include "storage_manage.h"
#include "time_utils.h"
#include "video_frame_jpeg_encoder.hpp"

namespace
{
/* 特征模型输入宽度 */
constexpr int FACE_FEATURE_INPUT_WIDTH = 112;
/* 特征模型输入高度 */
constexpr int FACE_FEATURE_INPUT_HEIGHT = 112;
/* 相似度展示加成（平台已看惯该百分比口径，去掉会同人掉 30%） */
constexpr float FACE_SIMILARITY_DISPLAY_BONUS = 0.3f;
/* 配置相似度阈值非法（<=0 或 >1）时的回退判定阈值 */
constexpr float FACE_COMPARE_SUCCESS_THRESHOLD = 0.7f;
/* 逐帧调试开关：板端 touch /opt/cam/bin/testPrint 后生效（绝对路径，进程 CWD 可能在 /opt/cam/bin） */
constexpr const char *FACE_COMPARE_DEBUG_FLAG = "/opt/cam/bin/testPrint";

/*
 * 配置相似度阈值归一化：(0,1] 用原值，其余（未配置/非法）回退 0.7。
 * 事件判定与日志/TVSDK 共用同一归一化结果（D1 统一口径）。
 */
float normalizeFaceCompareThreshold(float fThreshold)
{
    if (fThreshold > 0.0f && fThreshold <= 1.0f)
    {
        return fThreshold;
    }
    return FACE_COMPARE_SUCCESS_THRESHOLD;
}

/* 浮点转固定小数位字符串（事件属性 SimilarityFloat/ThresholdFloat） */
std::string toFixedString(float fValue, int nPrecision = 3)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(nPrecision) << fValue;
    return oss.str();
}

/* 浮点转百分比整数字符串（事件属性 Similarity/Threshold） */
std::string toPercentString(float fValue)
{
    const float fClamped = std::max(0.0f, std::min(fValue, 1.0f));
    return std::to_string(static_cast<int>(fClamped * 100.0f));
}

#ifdef ENABLE_TVSDK_SRC
/*
 * 读取已落盘的 JPEG 到 TVSDK 事件图（优先复用抓拍/通用抓图产物，
 * 避免同一帧重复编码），失败由调用方回退当帧编码。
 */
bool loadJpegFileToEventImage(const std::string &strPath, ot_video_frame_info *pFrameInfo, EventTvSdkImage_S &stImage)
{
    if (strPath.empty() || pFrameInfo == nullptr)
    {
        return false;
    }

    std::ifstream file(strPath, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }
    file.seekg(0, std::ios::end);
    const std::streampos nSize = file.tellg();
    if (nSize <= 0)
    {
        return false;
    }
    file.seekg(0, std::ios::beg);
    stImage.vecJpeg.resize(static_cast<size_t>(nSize));
    file.read(reinterpret_cast<char *>(stImage.vecJpeg.data()), static_cast<std::streamsize>(nSize));
    if (file.gcount() != static_cast<std::streamsize>(nSize))
    {
        stImage.vecJpeg.clear();
        return false;
    }
    stImage.nWidth = pFrameInfo->video_frame.width;
    stImage.nHeight = pFrameInfo->video_frame.height;
    stImage.strTag = "panorama";
    return true;
}
#endif

/* 事件属性非空才写入（Id/Name/PhoneNum/FaceLibName/LibFacePath） */
void addFaceCompareAttrIfNotEmpty(EventTriggerContext_S &stContext, const std::string &strKey, const std::string &strValue)
{
    if (!strValue.empty())
    {
        stContext.mapAttrs[strKey] = strValue;
    }
}

/*
 * 比对事件属性逐字段填充（与 Legacy 口径一致）：
 * CompareResult/CompareResultText/Similarity/SimilarityFloat/Threshold/ThresholdFloat/FaceId
 * + 命中人员时 Id/Name/PhoneNum/FaceLibName/LibFacePath + 图片路径内部字段。
 */
void fillFaceCompareAttrs(EventTriggerContext_S &stContext,
                          bool bSuccess,
                          int nFaceId,
                          float fSimilarityDisplay,
                          float fThreshold,
                          const std::string &strCaptureImagePath,
                          const FaceDataDB_NS::FaceLibsInfo_S &stMatchedFaceInfo)
{
    stContext.mapAttrs["CompareResult"] = bSuccess ? "1" : "0";
    stContext.mapAttrs["CompareResultText"] = bSuccess ? "success" : "fail";
    stContext.mapAttrs["Similarity"] = toPercentString(fSimilarityDisplay);
    stContext.mapAttrs["SimilarityFloat"] = toFixedString(fSimilarityDisplay);
    stContext.mapAttrs["Threshold"] = toPercentString(fThreshold);
    stContext.mapAttrs["ThresholdFloat"] = toFixedString(fThreshold);
    stContext.mapAttrs["FaceId"] = std::to_string(nFaceId);
    if (stMatchedFaceInfo.nId > 0)
    {
        stContext.mapAttrs["Id"] = std::to_string(stMatchedFaceInfo.nId);
        addFaceCompareAttrIfNotEmpty(stContext, "Name", stMatchedFaceInfo.strName);
        addFaceCompareAttrIfNotEmpty(stContext, "PhoneNum", stMatchedFaceInfo.strPhoneNum);
        addFaceCompareAttrIfNotEmpty(stContext, "FaceLibName", stMatchedFaceInfo.strFaceLibName);
        addFaceCompareAttrIfNotEmpty(stContext, "LibFacePath", stMatchedFaceInfo.strPicPath);
    }
    if (!strCaptureImagePath.empty())
    {
        /* 内部字段只给事件图片上传线程定位文件，MQTT报警正文会过滤掉 */
        stContext.mapAttrs["CaptureImagePath"] = strCaptureImagePath;
    }
}
} // namespace

namespace FaceChain_NS
{
void CFaceCompareProcessor::attachWorker(CFaceDetectWorker *pWorker)
{
    m_pWorker = pWorker;
}

void CFaceCompareProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        /*
         * 复位报警状态机（对齐 Legacy）；特征缓存不在此清——本接口在配置线程
         * 调用，缓存只在 Worker 回调线程读写，跨线程清 map 有数据竞争，
         * 交给 onTrackEnded/onAllCleared（同为 Worker 回调线程）随取优桶清理。
         */
        m_alarmStateMachine.reset();
    }
}

void CFaceCompareProcessor::setAlgoParamCfg(const Alarm::FaceCompare_S &stAlgoCfg)
{
    m_stAlgoCfg = stAlgoCfg;
    m_stSuccessLinkage = FaceDetectInternal::FaceCompareLinkageOptions_S();
    m_stFailLinkage = FaceDetectInternal::FaceCompareLinkageOptions_S();

    /* 成功/失败联动分别解析为快照（执行期间配置变更不影响本次事件口径） */
    for (auto &type : m_stAlgoCfg.stLinkageListSuccess.tradition)
    {
        if (type == int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE))
        {
            m_stSuccessLinkage.bPanoramaImage = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE))
        {
            m_stSuccessLinkage.bTargetImage = true;
        }
        else if (type == int(Alarm::LinkageType::SEND_EMAIL))
        {
            m_stSuccessLinkage.bEmail = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_SD_CARD))
        {
            m_stSuccessLinkage.bUploadSdCard = true;
        }
    }

    for (auto &type : m_stAlgoCfg.stLinkageListFail.tradition)
    {
        if (type == int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE))
        {
            m_stFailLinkage.bPanoramaImage = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE))
        {
            m_stFailLinkage.bTargetImage = true;
        }
        else if (type == int(Alarm::LinkageType::SEND_EMAIL))
        {
            m_stFailLinkage.bEmail = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_SD_CARD))
        {
            m_stFailLinkage.bUploadSdCard = true;
        }
    }
}

void CFaceCompareProcessor::setSensitivityThreshold(unsigned int nSensitivity)
{
    /* 与抓拍 collectTargets 同款换算：灵敏度越高阈值越低 */
    m_fSensitivityThreshold = 1.0f - nSensitivity / 100.0f;
}

float CFaceCompareProcessor::sensitivityThreshold() const
{
    return m_fSensitivityThreshold;
}

bool CFaceCompareProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}

bool CFaceCompareProcessor::isInitialized() const
{
    return true;
}

bool CFaceCompareProcessor::init()
{
    /* 原生实现无独立模型资源（特征模型由 Worker 常驻加载），保留调用时序 */
    return true;
}

void CFaceCompareProcessor::deinit()
{
    /* 模型 unInit 顺序：先停 Worker 再本调用，此处清理无线程竞争 */
    m_mapFeatures.clear();
    m_alarmStateMachine.reset();
}

void CFaceCompareProcessor::onNewBest(int nTrackId,
                                      const Bestshot_NS::BestShotFeedResult_S &stResult,
                                      const ot_video_frame_info *pFrame,
                                      const ot_rect &stRoi)
{
    /* 比对关闭时提取特征是纯浪费（抓拍单独开启仍会驱动喂入） */
    if (!m_stAlgoCfg.bEnable || m_pWorker == nullptr)
    {
        return;
    }
    /* 无帧或无有效裁剪框（过小目标）的 best 没法提特征 */
    if (pFrame == nullptr || stRoi.width == 0 || stRoi.height == 0)
    {
        return;
    }

    std::vector<float> vecFeature;
    if (!extractFeatureFromBest(pFrame, stRoi, vecFeature))
    {
        /* 清掉旧特征防止"旧脸的特征配新脸的帧"，下次新 best 自动重试 */
        m_mapFeatures.erase(nTrackId);
        dlog_warn("[人脸比对] 目标%d 最佳帧提特征失败，已清掉旧特征，等待下次评选重试", nTrackId);
        return;
    }

    FeatureEntry_S stEntry;
    stEntry.vecFeature = std::move(vecFeature);
    stEntry.llBestTimestamp = TimeUtils_NS::get_currentTimestampMs();
    m_mapFeatures[nTrackId] = std::move(stEntry);
    if (!access(FACE_COMPARE_DEBUG_FLAG, F_OK))
    {
        dlog_info("[人脸比对] 目标%d 评出新的最佳帧并提特征成功，特征%d维，当前缓存%d人",
                  nTrackId,
                  static_cast<int>(m_mapFeatures[nTrackId].vecFeature.size()),
                  static_cast<int>(m_mapFeatures.size()));
    }
    (void) stResult;
}

void CFaceCompareProcessor::onTrackEnded(int nTrackId)
{
    /* 仅 Worker 回调线程调用，无锁 */
    m_mapFeatures.erase(nTrackId);
}

void CFaceCompareProcessor::onAllCleared()
{
    m_mapFeatures.clear();
}

bool CFaceCompareProcessor::extractFeatureFromBest(const ot_video_frame_info *pFrame, const ot_rect &stRoi, std::vector<float> &vecFeature)
{
    vecFeature.clear();

    /*
     * 裁剪框按 VGS 约束回退对齐（x 16 / y 4 像素）并 clamp 到源帧内。
     * stRoi 已是源帧坐标（枢纽契约），不再做检测空间映射。
     */
    const int nFrameWidth = static_cast<int>(pFrame->video_frame.width);
    const int nFrameHeight = static_cast<int>(pFrame->video_frame.height);
    Common::RectInfo_S stFaceRect;
    stFaceRect.nX1 = std::min(ALIGN_BACK(static_cast<int>(stRoi.x), 16), nFrameWidth - 1);
    stFaceRect.nY1 = std::min(ALIGN_BACK(static_cast<int>(stRoi.y), 4), nFrameHeight - 1);
    stFaceRect.nX2 = std::min(ALIGN_BACK(static_cast<int>(stRoi.x) + static_cast<int>(stRoi.width), 16), nFrameWidth);
    stFaceRect.nY2 = std::min(ALIGN_BACK(static_cast<int>(stRoi.y) + static_cast<int>(stRoi.height), 4), nFrameHeight);
    if (stFaceRect.nX2 <= stFaceRect.nX1 || stFaceRect.nY2 <= stFaceRect.nY1)
    {
        dlog_warn("[人脸比对] 最佳帧裁剪框无效，跳过提特征 [%d,%d,%d,%d]", stFaceRect.nX1, stFaceRect.nY1, stFaceRect.nX2, stFaceRect.nY2);
        return false;
    }

    /* 裁剪帧：人脸局部图，缩放完成后立即销毁，不跨帧 */
    ot_video_frame_info stCropFrame;
    memset(&stCropFrame, 0, sizeof(stCropFrame));
    const unsigned int unCropWidth = static_cast<unsigned int>(stFaceRect.nX2 - stFaceRect.nX1);
    const unsigned int unCropHeight = static_cast<unsigned int>(stFaceRect.nY2 - stFaceRect.nY1);
    if (TD_SUCCESS != mppVgs_create_video_frame_info(unCropWidth, unCropHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stCropFrame))
    {
        return false;
    }

    /* 特征模型输入帧：112x112 */
    ot_video_frame_info stFaceFrame;
    memset(&stFaceFrame, 0, sizeof(stFaceFrame));
    bool bReady = false;
    do
    {
        ot_rect stCropRect;
        stCropRect.x = stFaceRect.nX1;
        stCropRect.y = stFaceRect.nY1;
        stCropRect.width = unCropWidth;
        stCropRect.height = unCropHeight;
        /* VGS 裁剪只读源帧，接口未标 const */
        if (TD_SUCCESS != mppVgs_crop(const_cast<ot_video_frame_info *>(pFrame), &stCropFrame, &stCropRect))
        {
            dlog_error("[人脸比对] 最佳帧人脸裁剪失败");
            break;
        }
        if (TD_SUCCESS != mppVgs_create_video_frame_info(FACE_FEATURE_INPUT_WIDTH,
                                                         FACE_FEATURE_INPUT_HEIGHT,
                                                         OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                         &stFaceFrame))
        {
            break;
        }
        if (TD_SUCCESS != mppVgs_scale(&stCropFrame, &stFaceFrame))
        {
            mppVgs_destroy_video_frame_info(&stFaceFrame);
            break;
        }
        bReady = true;
    }
    while (false);
    mppVgs_destroy_video_frame_info(&stCropFrame);
    if (!bReady)
    {
        return false;
    }

    /*
     * 同步推理提特征（调用方式对齐 Legacy extractFeatureDirect）：
     * 候选帧像素只存在于本函数栈上，推理完成即毁。
     */
    bool bExtractOk = false;
    do
    {
        auto *pFeatureHandle = m_pWorker->getFeatureHandle();
        if (pFeatureHandle == nullptr)
        {
            dlog_warn("[人脸比对] 特征模型未就绪，本张最佳帧不提特征");
            break;
        }

        Inference_NS::InputData_S stInputData;
        stInputData.pData = reinterpret_cast<float *>(stFaceFrame.video_frame.virt_addr[0]);
        stInputData.nDataSize = static_cast<int>(FACE_FEATURE_INPUT_WIDTH * FACE_FEATURE_INPUT_HEIGHT * 1.5) * sizeof(float);
        std::vector<Inference_NS::ClsData_S> vClsDatas;
        pFeatureHandle->inference(stInputData, vClsDatas);
        if (vClsDatas.empty() || vClsDatas[0].vFeature.empty())
        {
            dlog_warn("[人脸比对] 最佳帧特征推理失败");
            break;
        }
        vecFeature = vClsDatas[0].vFeature;
        bExtractOk = true;
    }
    while (false);
    mppVgs_destroy_video_frame_info(&stFaceFrame);
    return bExtractOk;
}

void CFaceCompareProcessor::process(FaceFrameContext_S &stCtx,
                                    const std::vector<FaceTarget_S> &vTargets,
                                    std::vector<std::string> &vecImageFile)
{
    if (!m_stAlgoCfg.bEnable || stCtx.pSrcFrameInfo == nullptr)
    {
        return;
    }

    /*
     * 目标入口灵敏度门限（保持 Legacy "比对目标不吃低分框"语义）：检测层
     * 置信度保底阈值（0.2）放行低分框维持跟踪连续后，稳定目标全集会包含
     * 低于抓拍灵敏度阈值的目标；比对目标集按抓拍同款阈值过滤后再参与比对。
     */
    std::vector<FaceTarget_S> vOrdered;
    vOrdered.reserve(vTargets.size());
    for (const auto &stTarget : vTargets)
    {
        if (stTarget.fConfidence < m_fSensitivityThreshold)
        {
            continue;
        }
        vOrdered.emplace_back(stTarget);
    }
    if (vOrdered.empty())
    {
        return;
    }

    /*
     * 多目标按置信度降序（同分比面积）：优先比最优目标，保证冷却窗内的
     * 事件属于置信度最高的人脸（对齐既有语义）；冷却期内其余目标仅完成
     * 比对与日志，不重复出事件。
     */
    std::sort(vOrdered.begin(),
              vOrdered.end(),
              [](const FaceTarget_S &lhs, const FaceTarget_S &rhs)
              {
                  if (lhs.fConfidence != rhs.fConfidence)
                  {
                      return lhs.fConfidence > rhs.fConfidence;
                  }
                  const long long llLeftArea = static_cast<long long>(std::max(0, lhs.stRect.nX2 - lhs.stRect.nX1)) *
                                               static_cast<long long>(std::max(0, lhs.stRect.nY2 - lhs.stRect.nY1));
                  const long long llRightArea = static_cast<long long>(std::max(0, rhs.stRect.nX2 - rhs.stRect.nX1)) *
                                                static_cast<long long>(std::max(0, rhs.stRect.nY2 - rhs.stRect.nY1));
                  return llLeftArea > llRightArea;
              });

    const long long llBaseTimestamp = stCtx.llTimestamp > 0 ? stCtx.llTimestamp : TimeUtils_NS::get_currentTimestampMs();

    for (const auto &stTarget : vOrdered)
    {
        /*
         * 查特征缓存：目标尚无 best（首帧窗口内）或上次提取失败时没有特征，
         * 跳过本轮比对（不阻断后续目标），best 产生后自动恢复。
         */
        const auto itFeature = m_mapFeatures.find(stTarget.nTrackId);
        if (itFeature == m_mapFeatures.end() || itFeature->second.vecFeature.empty())
        {
            if (!access(FACE_COMPARE_DEBUG_FLAG, F_OK))
            {
                dlog_info("[人脸比对] 目标%d 暂无人脸特征（尚未评出最佳帧或上次提取失败），本轮跳过比对", stTarget.nTrackId);
            }
            continue;
        }

        /* 全名单库余弦比对（共享 AIFaceManage 单例） */
        int nFaceLibId = -1;
        float fSimilarity = 0.0f;
        FaceManage::AIFaceManage::instance()->comparisonFaceLib(itFeature->second.vecFeature, nFaceLibId, fSimilarity);

        /*
         * D1 统一判定（唯一一条，事件类型/日志/TVSDK 曝光全链共用）：
         * 展示相似度 = 原始相似度 + 0.3（平台已看惯该数值）；
         * 阈值 = 配置归一化值（非法/<=0 回退 0.7）。
         */
        Alarm::FaceCompare_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        const float fThreshold = normalizeFaceCompareThreshold(stInfo.TargetLibInfos.Similarity);
        const float fSimilarityDisplay = fSimilarity + FACE_SIMILARITY_DISPLAY_BONUS;
        const bool bSuccess = fSimilarityDisplay >= fThreshold;
        dlog_info("[人脸比对] 目标%d 比对完成：名单人员编号%d 相似度%.3f 阈值%.3f result=%s",
                  stTarget.nTrackId,
                  nFaceLibId,
                  fSimilarityDisplay,
                  fThreshold,
                  bSuccess ? "success" : "fail");

        handleCompareLinkage(bSuccess,
                             stCtx.pSrcFrameInfo,
                             stCtx.nChnId,
                             llBaseTimestamp,
                             nFaceLibId,
                             fSimilarityDisplay,
                             fThreshold,
                             stCtx.stImageCache,
                             vecImageFile);
    }
}

FaceDetectInternal::FaceCompareLinkageOptions_S CFaceCompareProcessor::buildLinkageOptions(bool bSuccess) const
{
    return bSuccess ? m_stSuccessLinkage : m_stFailLinkage;
}

void CFaceCompareProcessor::handleCompareLinkage(bool bSuccess,
                                                 ot_video_frame_info *pFrameInfo,
                                                 int nChnId,
                                                 long long llTimestamp,
                                                 int nFaceId,
                                                 float fSimilarityDisplay,
                                                 float fThreshold,
                                                 FaceFrameImageCache_S &stImageCache,
                                                 std::vector<std::string> &vecImageFile)
{
    const FaceDetectInternal::FaceCompareLinkageOptions_S stOptions = buildLinkageOptions(bSuccess);
    const long long llEventTimestamp = llTimestamp > 0 ? llTimestamp : TimeUtils_NS::get_currentTimestampMs();
    std::string strUploadImagePath;
    FaceDataDB_NS::FaceLibsInfo_S stMatchedFaceInfo;

    /*
     * 图片编码、抓拍和上传都必须服从事件状态机。持续事件或冷却期直接返回，
     * 防止比对失败的陌生人停留在画面中时，每个检测周期都生成一张 JPEG。
     */
    if (!m_alarmStateMachine.canStartAlarm())
    {
        return;
    }

    EventTriggerContext_S stExposureContext;
    stExposureContext.enEventType = bSuccess ? Event::Type_E::FACE_COMPARE_SUCCESS : Event::Type_E::FACE_COMPARE_FAIL;
    /* 比对属性统一由 fillFaceCompareAttrs 填充（见下方调用），此处不重复手填 */

    stExposureContext.nChnId = nChnId;
    stExposureContext.llTimestamp = llEventTimestamp;
    if (nFaceId > 0)
    {
        FaceManage::AIFaceManage::instance()->searchFaceInfoById(nFaceId, stMatchedFaceInfo);
        if (stMatchedFaceInfo.nId > 0)
        {
            dlog_info("[人脸比对] 命中名单人员：编号%d 姓名%s 名单%s 相似度%.3f 结果%s",
                      stMatchedFaceInfo.nId,
                      stMatchedFaceInfo.strName.c_str(),
                      stMatchedFaceInfo.strFaceLibName.c_str(),
                      fSimilarityDisplay,
                      bSuccess ? "success" : "fail");
        }
        else
        {
            dlog_warn("[人脸比对] 未查到编号%d 的人员详情（相似度%.3f 结果%s）",
                      nFaceId,
                      fSimilarityDisplay,
                      bSuccess ? "success" : "fail");
        }
    }

    /*
     * 人脸比对不区分目标图和全景图。勾选上传SD卡后只走通用抓图链路，
     * 并将其首张图片同时提供给平台上传和邮件，避免重复编码JPEG。
     */
    std::string strPanoramaImagePath = stImageCache.strPanoramaImagePath;
    Event::Info_S stEventInfo;
    stEventInfo.enType = Event::Type_E::FACE_COMPARE;
    stEventInfo.strDate = TimeUtils_NS::get_currentDate();
    stEventInfo.strTime = TimeUtils_NS::get_currentTimeMs();
    stEventInfo.nChnId = nChnId < 0 ? 0 : nChnId;
    stEventInfo.strStartTime = TimeUtils_NS::get_currentDateWithDash() + " " + TimeUtils_NS::get_currentTimeWithColon();
    stEventInfo.strEndTime = stEventInfo.strStartTime;

    if (stOptions.bUploadSdCard && strPanoramaImagePath.empty() &&
        CStorageManage::instance()->get_SdCardStatus() == SD_CARD_STATUS_E::NORMAL &&
        CCaptureCtrl::instance()->set_event_capture(false, stEventInfo) == OK)
    {
        strPanoramaImagePath = CCaptureCtrl::instance()->get_face_capture_file();
        CCaptureCtrl::instance()->set_event_capture(true, stEventInfo);
        if (!strPanoramaImagePath.empty())
        {
            stImageCache.strPanoramaImagePath = strPanoramaImagePath;
        }
    }

    if (!strPanoramaImagePath.empty())
    {
        strUploadImagePath = strPanoramaImagePath;
    }

    /* 邮件与平台复用通用抓图模块生成的同一张图片。 */
    const std::string &strEmailImagePath = strUploadImagePath;
    if (stOptions.bEmail && !strEmailImagePath.empty())
    {
        vecImageFile.emplace_back(strEmailImagePath);
    }

    fillFaceCompareAttrs(stExposureContext, bSuccess, nFaceId, fSimilarityDisplay, fThreshold, strUploadImagePath, stMatchedFaceInfo);
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅且比对成功时才软件编码全景图，无订阅者跳过编码（冷却期已在入口早退挡住） */
    if (bSuccess && pFrameInfo != nullptr && AiAppCommon::tvsdk_event_image_required())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stExposureContext.enEventType);
        const bool bImageReady = (!stImageCache.strPanoramaImagePath.empty() &&
                                  loadJpegFileToEventImage(stImageCache.strPanoramaImagePath, pFrameInfo, pPayload->stPanoramaImage)) ||
                                 (AiAppCommon::encode_video_frame_to_jpeg_memory(pFrameInfo, pPayload->stPanoramaImage) == OK);
        if (bImageReady)
        {
            stExposureContext.pTvSdkPayload = pPayload;
        }
    }
#endif

    m_alarmStateMachine.handleAlarmState(true, stExposureContext);

    /*
     * 比对事件为瞬时语义（D3 即时出事件）：报警开始后立即结束并进入 7 秒
     * 冷却（D2 全局单状态机）；状态机无活跃事件时 end 无操作，与旧链路
     * "每目标无条件收尾两类型"行为等价。
     */
    m_alarmStateMachine.endAlarmImmediately(stExposureContext);

    if (stOptions.bEmail)
    {
        Network::EmailEventInfo_S stEmailInfo;

        std::string strCompareResult;
        if (bSuccess)
        {
            stEmailInfo.strSubject = "人脸比对成功";
            strCompareResult = "人脸比对成功";
        }
        else
        {
            stEmailInfo.strSubject = "人脸比对失败";
            strCompareResult = "人脸比对失败";
        }
        std::ostringstream oss;
        oss << "人脸比对结果: " << strCompareResult << "\n"
            << "日期: " << TimeUtils_NS::get_currentDateWithDash() << "\n"
            << "时间: " << TimeUtils_NS::get_currentTimeWithColon();
        stEmailInfo.strMessage = oss.str();
        stEmailInfo.vecImageFile = vecImageFile;
        CEmailManage::instance()->HandleEmail(stEmailInfo);
    }

    vecImageFile.clear();
}
} // namespace FaceChain_NS

#endif /* CAP_AI_FACE_COMPARE */
