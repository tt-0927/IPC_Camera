/**
 * @file face_recognition.cpp
 * @brief 人脸识别算法运行模块
 * @Date 原始创建日期未记录
 * @Author ITC
 * @Change 2026-10-09 沿用旧算法的 SDK 组包与告警接口，补齐合并人脸配置的抓拍推送。
 */

#include "face_recognition.hpp"

#include "action_code.h"
#include "capture_database.h"
#include "convert_interface.h"
#include "dlog.h"
#include "event_configure.h"
#include "storage_manage.h"
#include "stream_video.h"
#include "task_publish.h"
#include "time_utils.h"

#ifdef ENABLE_TVSDK_SRC
#include "control_manage.h"
#include "NetTVSDKServer.h"
#endif

#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

namespace
{
constexpr float FACE_DETECT_THRESHOLD = 0.5F;
constexpr float FACE_QUALITY_THRESHOLD = 0.05F;
constexpr int FACE_QUALITY_WIDTH = 112;
constexpr int FACE_QUALITY_HEIGHT = 112;
constexpr int FACE_ATTRIBUTE_WIDTH = 192;
constexpr int FACE_ATTRIBUTE_HEIGHT = 192;
constexpr int DEFAULT_MIN_IPD = 20;
constexpr float FACE_CLOSEUP_MARGIN_X = 1.0F;
constexpr float FACE_CLOSEUP_MARGIN_TOP = 0.8F;
constexpr float FACE_CLOSEUP_MARGIN_BOTTOM = 1.5F;
constexpr int FACE_CLOSEUP_MIN_SIDE = 96;
constexpr int FACE_CLOSEUP_MAX_SIDE = 640;

#ifdef ENABLE_TVSDK_SRC
constexpr int FACE_CAPTURE_REGION_POINT_COUNT = 4;
constexpr float FACE_CAPTURE_REGION_SCALE = 100.0F;

/**
 * @brief 按输入颜色格式编码抓拍图片，并校验 SDK 协议允许的图片长度。
 * @param [in] stImage RGB 全景图或 BGR 目标图。
 * @param [in] nQuality JPEG 编码质量。
 * @param [in] bInputRgb 输入是否为 RGB，目标小图必须为 false。
 * @param [out] stEncodedImage 持有 JPEG 内存的图片对象。
 * @return JPEG 非空且未超限返回 true，编码失败或异常返回 false。
 */
static bool face_recognition_encode_capture_image(const cv::Mat &stImage,
                                                   int nQuality,
                                                   bool bInputRgb,
                                                   EventTvSdkImage_S &stEncodedImage)
{
    try
    {
        if (!encode_mat_to_tvsdk_image(stImage, stEncodedImage, nQuality, bInputRgb))
        {
            return false;
        }
    }
    catch (const cv::Exception &stException)
    {
        dlog_warn("人脸抓拍编码异常: %s", stException.what());
        return false;
    }
    return !stEncodedImage.vecJpeg.empty() &&
           stEncodedImage.vecJpeg.size() <= static_cast<size_t>(NET_PIC_DATA_MAX_LEN);
}

/**
 * @brief 将检测目标框裁剪到全景边界，拒绝无效浮点值和退化区域。
 * @param [in] stResult 当前帧人脸检测结果，坐标与全景一致。
 * @param [in] stImageSize 全景尺寸。
 * @param [out] stCropRect 全景像素目标框，不等同于缩放或扩边后的 JPEG 尺寸。
 * @return 存在有效目标区域返回 true，否则返回 false。
 */
static bool face_recognition_get_capture_rect(const FaceDetect_NS::Result_S &stResult,
                                              const cv::Size &stImageSize,
                                              cv::Rect &stCropRect)
{
    if (stImageSize.width <= 0 || stImageSize.height <= 0 ||
        !std::isfinite(stResult.fX1) || !std::isfinite(stResult.fY1) ||
        !std::isfinite(stResult.fX2) || !std::isfinite(stResult.fY2) ||
        stResult.fX2 <= stResult.fX1 || stResult.fY2 <= stResult.fY1)
    {
        return false;
    }
    const double dLeft = std::max(0.0, static_cast<double>(stResult.fX1));
    const double dTop = std::max(0.0, static_cast<double>(stResult.fY1));
    const double dRight = std::min(static_cast<double>(stImageSize.width),
                                   static_cast<double>(stResult.fX2));
    const double dBottom = std::min(static_cast<double>(stImageSize.height),
                                    static_cast<double>(stResult.fY2));
    if (dRight <= dLeft || dBottom <= dTop)
    {
        return false;
    }
    const int nLeft = static_cast<int>(std::floor(dLeft));
    const int nTop = static_cast<int>(std::floor(dTop));
    const int nRight = static_cast<int>(std::ceil(dRight));
    const int nBottom = static_cast<int>(std::ceil(dBottom));
    stCropRect = cv::Rect(nLeft, nTop, nRight - nLeft, nBottom - nTop);
    return true;
}
#endif

cv::Scalar parseOverlayColor(const std::string &strColor)
{
    std::string strHex = strColor;
    if (!strHex.empty() && strHex.front() == '#')
    {
        strHex.erase(strHex.begin());
    }

    if (strHex.size() == 3)
    {
        strHex = {
            strHex[0], strHex[0],
            strHex[1], strHex[1],
            strHex[2], strHex[2]};
    }
    if (strHex.size() != 6)
    {
        return cv::Scalar(0, 0, 0);
    }

    try
    {
        const unsigned long nRgb = std::stoul(strHex, nullptr, 16);
        return cv::Scalar(
            nRgb & 0xFF,
            (nRgb >> 8) & 0xFF,
            (nRgb >> 16) & 0xFF);
    }
    catch (...)
    {
        return cv::Scalar(0, 0, 0);
    }
}

int findSameFaceTargetIndex(
    const std::vector<FaceDetect_NS::Result_S> &vstLastTargets,
    const std::vector<bool> &vbMatchedTargets,
    const FaceDetect_NS::Result_S &stCurrentTarget)
{
    for (size_t i = 0; i < vstLastTargets.size(); ++i)
    {
        if (i < vbMatchedTargets.size() && vbMatchedTargets[i])
        {
            continue;
        }

        const auto &stLastTarget = vstLastTargets[i];
        const float fWidth = stLastTarget.fX2 - stLastTarget.fX1;
        const float fHeight = stLastTarget.fY2 - stLastTarget.fY1;
        const float fDiagonal = std::hypot(fWidth, fHeight);
        if (fDiagonal <= 0.0F)
        {
            continue;
        }

        const float fCurrentCenterX =
            (stCurrentTarget.fX1 + stCurrentTarget.fX2) * 0.5F;
        const float fCurrentCenterY =
            (stCurrentTarget.fY1 + stCurrentTarget.fY2) * 0.5F;
        const float fLastCenterX =
            (stLastTarget.fX1 + stLastTarget.fX2) * 0.5F;
        const float fLastCenterY =
            (stLastTarget.fY1 + stLastTarget.fY2) * 0.5F;
        const float fCenterDistance = std::hypot(
            fCurrentCenterX - fLastCenterX,
            fCurrentCenterY - fLastCenterY);
        if (fCenterDistance <= fDiagonal * 0.25F)
        {
            return static_cast<int>(i);
        }
        if (fCenterDistance > fDiagonal * 0.30F)
        {
            continue;
        }

        const float fTopLeftDistance = std::hypot(
            stCurrentTarget.fX1 - stLastTarget.fX1,
            stCurrentTarget.fY1 - stLastTarget.fY1);
        const float fBottomRightDistance = std::hypot(
            stCurrentTarget.fX2 - stLastTarget.fX2,
            stCurrentTarget.fY2 - stLastTarget.fY2);
        if (fTopLeftDistance < fDiagonal * 0.30F ||
            fBottomRightDistance < fDiagonal * 0.30F)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}
}

CFaceRecognition::CFaceRecognition()
    : m_frameQueue(FRAME_QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CFaceRecognition::run, this);
    dlog_info("ai_app: 人脸识别算法模块创建");
}

CFaceRecognition::~CFaceRecognition()
{
    m_bEnabled.store(false);
    m_bRunning.store(false);
    m_threadCondition.notify_all();

    MediaData_S stMediaData;
    m_frameQueue.pushOrReplace(stMediaData);
    if (m_thread.joinable())
    {
        m_thread.join();
    }

    std::vector<Common::RectInfo_S> vstEmptyRect;
    send_detectionResult_to_osd(
        CONFIG_PIXEL_WIDTH,
        CONFIG_PIXEL_HEIGHT,
        vstEmptyRect);
    m_faceRecognitionStateMachine.endAlarmImmediately(Event::Type_E::FACE_RECOGNITION);
    unInit();
    dlog_info("ai_app: 人脸识别算法模块释放");
}

void CFaceRecognition::recvMediaData(MediaData_S stMediaData)
{
    if (!m_bEnabled.load())
    {
        return;
    }

    if (!m_recvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        return;
    }

    if (m_frameQueue.size() >= FRAME_QUEUE_MAX)
    {
        dlog_warn("ai_app: 人脸识别数据队列已满[%d]", m_frameQueue.size());
    }
    m_frameQueue.pushOrReplace(stMediaData);
}

void CFaceRecognition::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    if (!stAlgoConfig.nEnFaceRecognition)
    {
        m_bEnabled.store(false);
        {
            std::lock_guard<std::mutex> lock(m_attributeTargetMutex);
            m_vstLastAttributeTargets.clear();
        }
        dlog_info("ai_app: 人脸识别退出布防时间");
        return;
    }

    Alarm::FaceRecognition_S stConfig;
    CEventConfigure::instance()->get_configure(stConfig);

    bool bClearDynamicAnalysis = false;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        bClearDynamicAnalysis = m_stRuntimeConfig.bDynamicAnalysisEnable &&
                                !stConfig.bDynamicAnalysisEnable;
    }
    updateRuntimeConfig(stConfig);

    if (bClearDynamicAnalysis)
    {
        std::vector<Common::RectInfo_S> vstEmptyRect;
        send_detectionResult_to_osd(
            CONFIG_PIXEL_WIDTH,
            CONFIG_PIXEL_HEIGHT,
            vstEmptyRect);
    }

    const bool bEnabled = stConfig.bEnable;
    m_bEnabled.store(bEnabled);
    dlog_info("ai_app: 更新人脸识别配置, 运行[%d] 抓拍[%d] 属性分析[%d] 动态分析[%d]",
              bEnabled,
              stConfig.bCaptureEnable,
              stConfig.bAttributeAnalysisEnable,
              stConfig.bDynamicAnalysisEnable);
}

void CFaceRecognition::updateRuntimeConfig(const Alarm::FaceRecognition_S &stConfig)
{
    Alarm::FaceRecognition_S stRuntimeConfig = stConfig;
    const long lDetectIntervalMs =
        static_cast<long>(std::max(1, stRuntimeConfig.stCaptureRule.nInterval)) * 1000L;

    bool bResetAttributeTargets = false;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        bResetAttributeTargets =
            m_stRuntimeConfig.bEnable != stRuntimeConfig.bEnable ||
            m_stRuntimeConfig.bAttributeAnalysisEnable !=
                stRuntimeConfig.bAttributeAnalysisEnable;
        m_stRuntimeConfig = std::move(stRuntimeConfig);
    }

    /* 抓拍间隔作为整个人脸识别事件的检测间隔使用。 */
    m_recvManager.setTimeWindow(lDetectIntervalMs);

    if (bResetAttributeTargets)
    {
        std::lock_guard<std::mutex> lock(m_attributeTargetMutex);
        m_vstLastAttributeTargets.clear();
    }
}

bool CFaceRecognition::init()
{
    if (!m_pFaceDetectHandle)
    {
        FaceDetect_NS::InParam_S stDetectParam;
        stDetectParam.strModelPath = "/opt/cam/model/FaceDetect.json";
        stDetectParam.bDebug = false;

        m_pFaceDetectHandle = new FaceDetect_NS::CFaceDetectV1_0(stDetectParam);
        if (!m_pFaceDetectHandle || !m_pFaceDetectHandle->init())
        {
            delete m_pFaceDetectHandle;
            m_pFaceDetectHandle = nullptr;
            dlog_error("ai_app: 人脸识别检测模型初始化失败");
            return false;
        }
        dlog_info("ai_app: 人脸识别检测模型初始化成功");
    }

    if (!m_pFaceQualityHandle)
    {
        FaceQualityAssessment_NS::InParam_S stQualityParam;
        stQualityParam.strModelPath = "/opt/cam/model/FaceEvaluate.json";
        stQualityParam.bDebug = false;

        m_pFaceQualityHandle = new FaceQualityAssessment_NS::CFaceQualityAssessmentV1_0(stQualityParam);
        if (!m_pFaceQualityHandle || !m_pFaceQualityHandle->init())
        {
            delete m_pFaceQualityHandle;
            m_pFaceQualityHandle = nullptr;
            dlog_error("ai_app: 人脸识别质量模型初始化失败");
            return false;
        }
        dlog_info("ai_app: 人脸识别质量模型初始化成功");
    }

    return true;
}

void CFaceRecognition::unInit()
{
    if (m_pFaceDetectHandle)
    {
        delete m_pFaceDetectHandle;
        m_pFaceDetectHandle = nullptr;
    }

    if (m_pFaceQualityHandle)
    {
        delete m_pFaceQualityHandle;
        m_pFaceQualityHandle = nullptr;
    }

    if (m_pFaceAttributeHandle)
    {
        delete m_pFaceAttributeHandle;
        m_pFaceAttributeHandle = nullptr;
    }
}

bool CFaceRecognition::initAttributeModel()
{
    if (m_pFaceAttributeHandle)
    {
        return true;
    }

    FaceAttribute_NS::InParam_S stAttributeParam;
    stAttributeParam.strModelPath = "/opt/cam/model/FaceAttribute.json";
    stAttributeParam.bDebug = false;

    m_pFaceAttributeHandle = new FaceAttribute_NS::CFaceAttributeV1_0(stAttributeParam);
    if (!m_pFaceAttributeHandle || !m_pFaceAttributeHandle->init())
    {
        delete m_pFaceAttributeHandle;
        m_pFaceAttributeHandle = nullptr;
        dlog_error("ai_app: 人脸识别属性模型初始化失败");
        return false;
    }

    dlog_info("ai_app: 人脸识别属性模型初始化成功");
    return true;
}

bool CFaceRecognition::isInRegion(const Alarm::Region_S &stRegion,
                               const FaceDetect_NS::Result_S &stResult) const
{
    if (!isRegionConfigured(stRegion))
    {
        return false;
    }

    const float fCenterX = (stResult.fX1 + stResult.fX2) * 0.5F;
    const float fCenterY = (stResult.fY1 + stResult.fY2) * 0.5F;
    int nIntersectCount = 0;
    constexpr float EPSILON = 1e-6F;

    for (size_t i = 0; i < stRegion.nPointNum; ++i)
    {
        const Common::PosF_S &stPoint1 = stRegion.aPoint[i];
        const Common::PosF_S &stPoint2 =
            stRegion.aPoint[(i + 1) % stRegion.nPointNum];
        const float fDeltaY = stPoint2.fY - stPoint1.fY;
        if (std::abs(fDeltaY) < EPSILON)
        {
            continue;
        }

        if ((stPoint1.fY > fCenterY) != (stPoint2.fY > fCenterY))
        {
            const float fIntersectX = (stPoint2.fX - stPoint1.fX) *
                                          (fCenterY - stPoint1.fY) / fDeltaY +
                                      stPoint1.fX;
            if (fCenterX < fIntersectX)
            {
                ++nIntersectCount;
            }
        }
    }

    return nIntersectCount % 2 == 1;
}

bool CFaceRecognition::isRegionConfigured(const Alarm::Region_S &stRegion) const
{
    if (!stRegion.IsValid())
    {
        return false;
    }

    double dTwiceArea = 0.0;
    for (size_t i = 0; i < stRegion.nPointNum; ++i)
    {
        const Common::PosF_S &stPoint1 = stRegion.aPoint[i];
        const Common::PosF_S &stPoint2 =
            stRegion.aPoint[(i + 1) % stRegion.nPointNum];
        dTwiceArea += static_cast<double>(stPoint1.fX) * stPoint2.fY -
                      static_cast<double>(stPoint2.fX) * stPoint1.fY;
    }

    constexpr double MIN_REGION_TWICE_AREA = 1e-3;
    return std::abs(dTwiceArea) > MIN_REGION_TWICE_AREA;
}

bool CFaceRecognition::evaluateFaceQuality(const cv::Mat &stRgbImage,
                                            const FaceDetect_NS::Result_S &stResult,
                                            float &fQualityScore)
{
    if (!m_pFaceQualityHandle || stRgbImage.empty())
    {
        return false;
    }

    FaceQualityAssessment_NS::InData_S stQualityInput;
    if (!cropFaceForModel(
            stRgbImage,
            stResult,
            FACE_QUALITY_WIDTH,
            FACE_QUALITY_HEIGHT,
            stQualityInput.inMat))
    {
        return false;
    }

    return m_pFaceQualityHandle->process(stQualityInput, fQualityScore);
}

bool CFaceRecognition::cropFaceForModel(const cv::Mat &stRgbImage,
                                        const FaceDetect_NS::Result_S &stResult,
                                        int nWidth,
                                        int nHeight,
                                        cv::Mat &stFaceImage) const
{
    if (stRgbImage.empty() || nWidth <= 0 || nHeight <= 0)
    {
        return false;
    }

    int nX1 = std::max(0, std::min(stRgbImage.cols, static_cast<int>(stResult.fX1)));
    int nY1 = std::max(0, std::min(stRgbImage.rows, static_cast<int>(stResult.fY1)));
    int nX2 = std::max(0, std::min(stRgbImage.cols, static_cast<int>(stResult.fX2)));
    int nY2 = std::max(0, std::min(stRgbImage.rows, static_cast<int>(stResult.fY2)));
    pointScaleUp(nX1, nY1, nX2, nY2, stRgbImage.cols, stRgbImage.rows, 1.5);
    if (nX2 <= nX1 || nY2 <= nY1)
    {
        return false;
    }

    const cv::Rect stFaceRoi(nX1, nY1, nX2 - nX1, nY2 - nY1);
    stFaceImage = cv::Mat(nHeight, nWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    return fillRGBToCenter(stRgbImage, stFaceRoi, stFaceImage);
}

bool CFaceRecognition::isCaptureCandidate(
    const FaceDetect_NS::Result_S &stResult,
    const Alarm::FaceRecognitionCaptureRule_S &stRule) const
{
    for (const auto &stShieldedRegion : stRule.vstShieldedRegion)
    {
        if (isRegionConfigured(stShieldedRegion) &&
            isInRegion(stShieldedRegion, stResult))
        {
            return false;
        }
    }

    int nMinIpd = DEFAULT_MIN_IPD;
    if (!stRule.stMinIpdRect.isEmpty() &&
        stRule.stMinIpdRect.IsValid())
    {
        nMinIpd = stRule.stMinIpdRect.nWidth;
    }

    if (stResult.vPoint.size() < 4)
    {
        return false;
    }

    const float fIpd = std::abs(stResult.vPoint[2] - stResult.vPoint[0]);
    return fIpd >= static_cast<float>(nMinIpd);
}

bool CFaceRecognition::analyzeFaceAttribute(
    const cv::Mat &stRgbImage,
    const FaceDetect_NS::Result_S &stResult,
    FaceAttribute_NS::Result_S &stAttributeResult)
{
    if (!initAttributeModel())
    {
        return false;
    }

    FaceAttribute_NS::InData_S stAttributeInput;
    if (!cropFaceForModel(
            stRgbImage,
            stResult,
            FACE_ATTRIBUTE_WIDTH,
            FACE_ATTRIBUTE_HEIGHT,
            stAttributeInput.inMat))
    {
        return false;
    }

    std::vector<FaceAttribute_NS::Result_S> vstAttributeResult;
    if (!m_pFaceAttributeHandle->process(stAttributeInput, vstAttributeResult) ||
        vstAttributeResult.empty())
    {
        return false;
    }

    stAttributeResult = vstAttributeResult.front();
    return true;
}

bool CFaceRecognition::prepareFrameContext(
    const MediaData_S &stMediaData,
    FrameContext_S &stContext)
{
    if (stMediaData.pData.get() == nullptr ||
        stMediaData.stMediaParam.nVideoWidth <= 0 ||
        stMediaData.stMediaParam.nVideoHeight <= 0)
    {
        return false;
    }

    const int nFrameWidth = stMediaData.stMediaParam.nVideoWidth;
    const int nFrameHeight = stMediaData.stMediaParam.nVideoHeight;
    stContext.nChannelId = stMediaData.stMediaParam.nChannel;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        stContext.stConfig = m_stRuntimeConfig;
    }
    stContext.stConfig.stRegion.ConvertResolution(
        CONFIG_PIXEL_WIDTH,
        CONFIG_PIXEL_HEIGHT,
        nFrameWidth,
        nFrameHeight);
    for (auto &stShieldedRegion :
         stContext.stConfig.stCaptureRule.vstShieldedRegion)
    {
        stShieldedRegion.ConvertResolution(
            CONFIG_PIXEL_WIDTH,
            CONFIG_PIXEL_HEIGHT,
            nFrameWidth,
            nFrameHeight);
    }

    CloseupFrame_S stCloseupFrame;
    if (CStreamVideo::instance()->grabFaceCloseupSource(
            stCloseupFrame,
            stMediaData.u64PTS) == OK)
    {
        CloseupSource_S &stCloseupSource = stContext.stCloseupSource;
        stCloseupSource.pNv12 = std::move(stCloseupFrame.pData);
        stCloseupSource.nDataSize = stCloseupFrame.nDataSize;
        stCloseupSource.nWidth = stCloseupFrame.nWidth;
        stCloseupSource.nHeight = stCloseupFrame.nHeight;
        stCloseupSource.nVirWidth = stCloseupFrame.nVirWidth;
        stCloseupSource.nVirHeight = stCloseupFrame.nVirHeight;
    }

    m_stPanoramaRgbImage.create(nFrameHeight, nFrameWidth, CV_8UC3);
    const bool bRgaSuccess =
        nFrameWidth % 4 == 0 &&
        nFrameHeight % 2 == 0 &&
        rga_image_transform(
            stMediaData.pData.get(),
            nFrameWidth,
            nFrameHeight,
            RK_FORMAT_YCbCr_420_SP,
            m_stPanoramaRgbImage.data,
            nFrameWidth,
            nFrameHeight,
            RK_FORMAT_RGB_888,
            0,
            0,
            nFrameWidth,
            nFrameHeight,
            0);
    if (!bRgaSuccess)
    {
        cv::Mat stNv12Image(
            nFrameHeight * 3 / 2,
            nFrameWidth,
            CV_8UC1,
            stMediaData.pData.get());
        cv::cvtColor(
            stNv12Image,
            m_stPanoramaRgbImage,
            cv::COLOR_YUV2RGB_NV12);
    }
    stContext.stPanoramaImage.stImage = m_stPanoramaRgbImage;
    return !stContext.stPanoramaImage.stImage.empty();
}

bool CFaceRecognition::detectFaces(FrameContext_S &stContext)
{
    FaceDetect_NS::InData_S stDetectInput;
    FaceDetect_NS::OutData_S stDetectOutput;
    std::vector<FaceDetect_NS::Result_S> vstDetectedResults;
    stDetectInput.nChnId = stContext.nChannelId;
    stDetectInput.stParam.fBoxThreshold = FACE_DETECT_THRESHOLD;
    stDetectInput.inMat = stContext.stPanoramaImage.stImage;

    if (!m_pFaceDetectHandle->process(
            stDetectInput,
            vstDetectedResults,
            &stDetectOutput))
    {
        dlog_error("ai_app: 人脸识别检测失败");
        return false;
    }

    stContext.vstTargets.clear();
    stContext.vstTargets.reserve(vstDetectedResults.size());
    for (auto &stResult : vstDetectedResults)
    {
        stContext.vstTargets.emplace_back(std::move(stResult));
    }
    return true;
}

void CFaceRecognition::filterDetectionResults(FrameContext_S &stContext) const
{
    const bool bRegionConfigured = isRegionConfigured(stContext.stConfig.stRegion);
    const auto iter = std::remove_if(
        stContext.vstTargets.begin(),
        stContext.vstTargets.end(),
        [this, &stContext, bRegionConfigured](const TargetResult_S &stTarget) {
            const FaceDetect_NS::Result_S &stResult = stTarget.stDetectResult;
            return stResult.fBoxConfidence < FACE_DETECT_THRESHOLD ||
                   (bRegionConfigured &&
                    !isInRegion(stContext.stConfig.stRegion, stResult));
        });
    stContext.vstTargets.erase(iter, stContext.vstTargets.end());
}

void CFaceRecognition::processQualityAssessment(FrameContext_S &stContext)
{
    stContext.nBestTargetIndex = -1;

    auto iter = stContext.vstTargets.begin();
    while (iter != stContext.vstTargets.end())
    {
        float fQualityScore = 0.0F;
        if (!evaluateFaceQuality(
                stContext.stPanoramaImage.stImage,
                iter->stDetectResult,
                fQualityScore) ||
            fQualityScore < FACE_QUALITY_THRESHOLD)
        {
            iter = stContext.vstTargets.erase(iter);
            continue;
        }
        iter->fQualityScore = fQualityScore;
        ++iter;
    }

    for (size_t i = 0; i < stContext.vstTargets.size(); ++i)
    {
        const TargetResult_S *pBestTarget = getBestTarget(stContext);
        if (!pBestTarget ||
            stContext.vstTargets[i].stDetectResult.fBoxConfidence >
                pBestTarget->stDetectResult.fBoxConfidence)
        {
            stContext.nBestTargetIndex = static_cast<int>(i);
        }
    }
}

CFaceRecognition::TargetResult_S *CFaceRecognition::getBestTarget(
    FrameContext_S &stContext)
{
    if (stContext.nBestTargetIndex < 0 ||
        static_cast<size_t>(stContext.nBestTargetIndex) >= stContext.vstTargets.size())
    {
        return nullptr;
    }
    return &stContext.vstTargets[stContext.nBestTargetIndex];
}

const CFaceRecognition::TargetResult_S *CFaceRecognition::getBestTarget(
    const FrameContext_S &stContext) const
{
    if (stContext.nBestTargetIndex < 0 ||
        static_cast<size_t>(stContext.nBestTargetIndex) >= stContext.vstTargets.size())
    {
        return nullptr;
    }
    return &stContext.vstTargets[stContext.nBestTargetIndex];
}

void CFaceRecognition::processAttributeAnalysis(FrameContext_S &stContext)
{
    if (!stContext.stConfig.bAttributeAnalysisEnable)
    {
        return;
    }

    for (auto &stTarget : stContext.vstTargets)
    {
        stTarget.bAttributeReady = analyzeFaceAttribute(
            stContext.stPanoramaImage.stImage,
            stTarget.stDetectResult,
            stTarget.stAttributeResult);
    }
}

bool CFaceRecognition::processAlgorithms(FrameContext_S &stContext)
{
    if (!detectFaces(stContext))
    {
        return false;
    }

    filterDetectionResults(stContext);
    processQualityAssessment(stContext);
    processAttributeAnalysis(stContext);
    return true;
}

void CFaceRecognition::processCapturePolicy(FrameContext_S &stContext)
{
    if (!stContext.stConfig.bCaptureEnable || stContext.vstTargets.empty())
    {
        return;
    }

    for (auto &stTarget : stContext.vstTargets)
    {
        if (isCaptureCandidate(
                stTarget.stDetectResult,
                stContext.stConfig.stCaptureRule))
        {
            stTarget.bCaptureTriggered = true;
        }
    }
}

void CFaceRecognition::processAttributePushPolicy(FrameContext_S &stContext)
{
    std::lock_guard<std::mutex> lock(m_attributeTargetMutex);
    if (!stContext.stConfig.bAttributeAnalysisEnable ||
        stContext.vstTargets.empty())
    {
        m_vstLastAttributeTargets.clear();
        return;
    }

    std::vector<bool> vbMatchedTargets(
        m_vstLastAttributeTargets.size(),
        false);
    for (auto &stTarget : stContext.vstTargets)
    {
        if (!stTarget.bAttributeReady)
        {
            continue;
        }

        const int nMatchedIndex = findSameFaceTargetIndex(
            m_vstLastAttributeTargets,
            vbMatchedTargets,
            stTarget.stDetectResult);
        stTarget.bAttributePushTriggered = nMatchedIndex < 0;
        if (nMatchedIndex >= 0)
        {
            vbMatchedTargets[static_cast<size_t>(nMatchedIndex)] = true;
        }
    }

    m_vstLastAttributeTargets.clear();
    m_vstLastAttributeTargets.reserve(stContext.vstTargets.size());
    for (const auto &stTarget : stContext.vstTargets)
    {
        if (stTarget.bAttributeReady)
        {
            m_vstLastAttributeTargets.emplace_back(stTarget.stDetectResult);
        }
    }
}

bool CFaceRecognition::cropCloseupImageByRga(
    const CloseupSource_S &stCloseupSource,
    const cv::Rect2f &stDetectRect,
    const cv::Size &stDetectCoordinateSize,
    cv::Mat &stTargetBgrImage) const
{
    stTargetBgrImage.release();
    if (!stCloseupSource.isValid() ||
        stCloseupSource.nWidth != stCloseupSource.nVirWidth ||
        stCloseupSource.nHeight != stCloseupSource.nVirHeight ||
        stCloseupSource.nWidth % 2 != 0 ||
        stCloseupSource.nHeight % 2 != 0 ||
        stDetectCoordinateSize.width <= 0 ||
        stDetectCoordinateSize.height <= 0 ||
        !std::isfinite(stDetectRect.x) ||
        !std::isfinite(stDetectRect.y) ||
        !std::isfinite(stDetectRect.width) ||
        !std::isfinite(stDetectRect.height) ||
        stDetectRect.width <= 0.0F ||
        stDetectRect.height <= 0.0F)
    {
        return false;
    }

    const std::size_t nWidth = static_cast<std::size_t>(stCloseupSource.nWidth);
    const std::size_t nHeight = static_cast<std::size_t>(stCloseupSource.nHeight);
    const std::size_t nMaxSize = std::numeric_limits<std::size_t>::max();
    if (nWidth > nMaxSize / nHeight)
    {
        return false;
    }
    const std::size_t nYSize = nWidth * nHeight;
    if (nYSize > nMaxSize - nYSize / 2U ||
        stCloseupSource.nDataSize < nYSize + nYSize / 2U)
    {
        return false;
    }

    const double dScaleX = static_cast<double>(stCloseupSource.nWidth) /
                           stDetectCoordinateSize.width;
    const double dScaleY = static_cast<double>(stCloseupSource.nHeight) /
                           stDetectCoordinateSize.height;
    const double dFaceLeft = std::max(
        0.0,
        static_cast<double>(stDetectRect.x) * dScaleX);
    const double dFaceTop = std::max(
        0.0,
        static_cast<double>(stDetectRect.y) * dScaleY);
    const double dFaceRight = std::min(
        static_cast<double>(stCloseupSource.nWidth),
        (static_cast<double>(stDetectRect.x) + stDetectRect.width) * dScaleX);
    const double dFaceBottom = std::min(
        static_cast<double>(stCloseupSource.nHeight),
        (static_cast<double>(stDetectRect.y) + stDetectRect.height) * dScaleY);
    if (dFaceRight <= dFaceLeft || dFaceBottom <= dFaceTop)
    {
        return false;
    }

    const int nFaceLeft = static_cast<int>(std::floor(dFaceLeft));
    const int nFaceTop = static_cast<int>(std::floor(dFaceTop));
    const int nFaceRight = static_cast<int>(std::ceil(dFaceRight));
    const int nFaceBottom = static_cast<int>(std::ceil(dFaceBottom));
    const int nFaceWidth = nFaceRight - nFaceLeft;
    const int nFaceHeight = nFaceBottom - nFaceTop;
    if (nFaceWidth <= 0 || nFaceHeight <= 0)
    {
        return false;
    }

    const int nMarginX = static_cast<int>(
        std::lround(nFaceWidth * FACE_CLOSEUP_MARGIN_X));
    const int nMarginTop = static_cast<int>(
        std::lround(nFaceHeight * FACE_CLOSEUP_MARGIN_TOP));
    const int nMarginBottom = static_cast<int>(
        std::lround(nFaceHeight * FACE_CLOSEUP_MARGIN_BOTTOM));
    const int nLeft = std::max(0, nFaceLeft - nMarginX) & ~1;
    const int nTop = std::max(0, nFaceTop - nMarginTop) & ~1;
    const int nRight = std::min(
        stCloseupSource.nWidth,
        nFaceRight + nMarginX);
    const int nBottom = std::min(
        stCloseupSource.nHeight,
        nFaceBottom + nMarginBottom);
    const int nCropWidth = (nRight - nLeft) & ~1;
    const int nCropHeight = (nBottom - nTop) & ~1;
    if (nCropWidth <= 0 || nCropHeight <= 0)
    {
        return false;
    }

    int nTargetWidth = nCropWidth;
    int nTargetHeight = nCropHeight;
    const int nLongSide = std::max(nTargetWidth, nTargetHeight);
    if (nLongSide > FACE_CLOSEUP_MAX_SIDE)
    {
        const double dScale =
            static_cast<double>(FACE_CLOSEUP_MAX_SIDE) / nLongSide;
        nTargetWidth = std::max(
            1,
            static_cast<int>(std::lround(nTargetWidth * dScale)));
        nTargetHeight = std::max(
            1,
            static_cast<int>(std::lround(nTargetHeight * dScale)));
    }

    const int nShortSide = std::min(nTargetWidth, nTargetHeight);
    if (nShortSide < FACE_CLOSEUP_MIN_SIDE)
    {
        const double dScale =
            static_cast<double>(FACE_CLOSEUP_MIN_SIDE) / nShortSide;
        nTargetWidth = std::max(
            1,
            static_cast<int>(std::lround(nTargetWidth * dScale)));
        nTargetHeight = std::max(
            1,
            static_cast<int>(std::lround(nTargetHeight * dScale)));
    }

    nTargetWidth = std::max(4, nTargetWidth & ~3);
    nTargetHeight = std::max(2, nTargetHeight & ~1);
    stTargetBgrImage.create(nTargetHeight, nTargetWidth, CV_8UC3);
    if (!rga_image_transform(
            stCloseupSource.pNv12.get(),
            stCloseupSource.nWidth,
            stCloseupSource.nHeight,
            RK_FORMAT_YCbCr_420_SP,
            stTargetBgrImage.data,
            nTargetWidth,
            nTargetHeight,
            RK_FORMAT_BGR_888,
            nLeft,
            nTop,
            nCropWidth,
            nCropHeight,
            0))
    {
        stTargetBgrImage.release();
        return false;
    }

    return true;
}

bool CFaceRecognition::ensureTargetImage(
    FrameContext_S &stContext,
    TargetResult_S &stTarget)
{
    if (!stTarget.stTargetImage.stImage.empty())
    {
        return true;
    }

    const FaceDetect_NS::Result_S &stResult = stTarget.stDetectResult;
    const cv::Rect2f stFaceRect(
        stResult.fX1,
        stResult.fY1,
        stResult.fX2 - stResult.fX1,
        stResult.fY2 - stResult.fY1);

    const CloseupSource_S &stCloseupSource = stContext.stCloseupSource;
    if (stCloseupSource.isValid())
    {
        cv::Mat stCloseupBgrImage;
        bool bCropSuccess = cropCloseupImageByRga(
            stCloseupSource,
            stFaceRect,
            stContext.stPanoramaImage.stImage.size(),
            stCloseupBgrImage);
        if (!bCropSuccess)
        {
            bCropSuccess = cropTargetImageNv12(
                stCloseupSource.pNv12.get(),
                stCloseupSource.nDataSize,
                stCloseupSource.nWidth,
                stCloseupSource.nHeight,
                stCloseupSource.nVirWidth,
                stCloseupSource.nVirHeight,
                stFaceRect,
                stContext.stPanoramaImage.stImage.size(),
                stCloseupBgrImage,
                FACE_CLOSEUP_MARGIN_X,
                FACE_CLOSEUP_MARGIN_TOP,
                FACE_CLOSEUP_MARGIN_BOTTOM,
                FACE_CLOSEUP_MAX_SIDE);
        }
        if (bCropSuccess)
        {
            const int nShortSide = std::min(
                stCloseupBgrImage.cols,
                stCloseupBgrImage.rows);
            if (nShortSide > 0 && nShortSide < FACE_CLOSEUP_MIN_SIDE)
            {
                const double dScale =
                    static_cast<double>(FACE_CLOSEUP_MIN_SIDE) / nShortSide;
                cv::resize(
                    stCloseupBgrImage,
                    stCloseupBgrImage,
                    cv::Size(),
                    dScale,
                    dScale,
                    cv::INTER_CUBIC);
            }
            stTarget.stTargetImage.stImage = std::move(stCloseupBgrImage);
            return true;
        }
        dlog_warn("ai_app: 人脸识别特写源裁剪失败, 回退全景图裁剪");
    }

    if (!cropTargetImageFace(
            stContext.stPanoramaImage.stImage,
            stFaceRect,
            stContext.stPanoramaImage.stImage.size(),
            stTarget.stTargetImage.stImage))
    {
        dlog_warn("ai_app: 人脸识别目标小图裁剪失败");
        return false;
    }

    cv::cvtColor(
        stTarget.stTargetImage.stImage,
        stTarget.stTargetImage.stImage,
        cv::COLOR_RGB2BGR);

    return true;
}

void CFaceRecognition::addCaptureOverlay(cv::Mat &stBgrImage) const
{
    if (stBgrImage.empty())
    {
        return;
    }

    Alarm::OverlayInfo_S stOverlayInfo;
    CEventConfigure::instance()->get_configure(stOverlayInfo);

    cv::Scalar stColor(0, 0, 0);
    if (stOverlayInfo.enFontColor == Osd::OSD_COLOR_WHITE)
    {
        stColor = cv::Scalar(255, 255, 255);
    }
    else if (stOverlayInfo.enFontColor != Osd::OSD_COLOR_BLACK)
    {
        stColor = parseOverlayColor(stOverlayInfo.strFontColor);
    }

    std::vector<std::string> vstTopLeftText;
    if (stOverlayInfo.bOverlayDeviceID)
    {
        vstTopLeftText.emplace_back(std::to_string(stOverlayInfo.nDeviceID));
    }
    if (stOverlayInfo.bOverlayMonitoryPointInfo &&
        !stOverlayInfo.strMonitoryPointInfo.empty())
    {
        vstTopLeftText.emplace_back(stOverlayInfo.strMonitoryPointInfo);
    }

    constexpr int TEXT_MARGIN = 10;
    constexpr int TEXT_LINE_HEIGHT = 30;
    constexpr double TEXT_FONT_SCALE = 1.0;
    constexpr int TEXT_THICKNESS = 2;
    for (size_t i = 0; i < vstTopLeftText.size(); ++i)
    {
        cv::putText(
            stBgrImage,
            vstTopLeftText[i],
            cv::Point(TEXT_MARGIN, TEXT_LINE_HEIGHT * static_cast<int>(i + 1)),
            cv::FONT_HERSHEY_SIMPLEX,
            TEXT_FONT_SCALE,
            stColor,
            TEXT_THICKNESS);
    }

    if (stOverlayInfo.bOverlayCaptureTime)
    {
        cv::putText(
            stBgrImage,
            TimeUtils_NS::get_currentDateAndTimeNoT(),
            cv::Point(TEXT_MARGIN, stBgrImage.rows - TEXT_MARGIN),
            cv::FONT_HERSHEY_SIMPLEX,
            TEXT_FONT_SCALE,
            stColor,
            TEXT_THICKNESS);
    }
}

std::string CFaceRecognition::saveCaptureImage(
    const cv::Mat &stBgrImage,
    Alarm::LinkageType_E enImageType,
    const std::string &strCurrentDate,
    const std::string &strCurrentTime,
    int nJpegQuality,
    int nTargetIndex) const
{
    if (stBgrImage.empty() ||
        CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
    {
        return {};
    }

    const std::filesystem::path stCaptureDirectory =
        std::filesystem::path(CAPTURE_PATH) / strCurrentDate;
    std::error_code stError;
    std::filesystem::create_directories(stCaptureDirectory, stError);
    if (stError)
    {
        dlog_error("ai_app: 创建人脸识别抓拍目录失败[%s]: %s",
                   stCaptureDirectory.c_str(),
                   stError.message().c_str());
        return {};
    }

    std::string strFilename =
        strCurrentDate + "_" +
        strCurrentTime + "_" +
        std::to_string(static_cast<int>(Event::Type_E::FACE_RECOGNITION)) + "_" +
        std::to_string(static_cast<int>(enImageType));
    if (nTargetIndex >= 0)
    {
        strFilename += "_" + std::to_string(nTargetIndex);
    }
    strFilename += ".jpg";
    const std::filesystem::path stImagePath = stCaptureDirectory / strFilename;
    const std::vector<int> vstJpegParams = {
        cv::IMWRITE_JPEG_QUALITY,
        nJpegQuality};
    if (!cv::imwrite(stImagePath.string(), stBgrImage, vstJpegParams))
    {
        dlog_error("ai_app: 保存人脸识别抓拍图片失败[%s]", stImagePath.c_str());
        return {};
    }

    return stImagePath.string();
}

bool CFaceRecognition::saveCaptureRecord(
    const std::string &strImagePath,
    int nChannelId,
    const std::string &strCaptureTime) const
{
    if (strImagePath.empty())
    {
        return false;
    }

    Capture_NS::CaptureInfo_S stCaptureInfo;
    stCaptureInfo.nChnId = nChannelId;
    stCaptureInfo.enType = Event::Type_E::FACE_RECOGNITION;
    stCaptureInfo.strStartTime = strCaptureTime;
    stCaptureInfo.strEndTime = strCaptureTime;
    stCaptureInfo.strImagePath = strImagePath;

    std::error_code stError;
    const std::uintmax_t nFileSize = std::filesystem::file_size(strImagePath, stError);
    if (stError)
    {
        dlog_warn("ai_app: 获取人脸识别抓拍图片大小失败[%s]: %s",
                  strImagePath.c_str(),
                  stError.message().c_str());
        stCaptureInfo.nImageSize = 0;
    }
    else
    {
        stCaptureInfo.nImageSize = static_cast<int>(std::min<std::uintmax_t>(
            nFileSize,
            static_cast<std::uintmax_t>(std::numeric_limits<int>::max())));
    }

    if (Db::CCaptureDatabase::instance()->add(stCaptureInfo) < 0)
    {
        dlog_error("ai_app: 写入人脸识别抓拍记录失败[%s]", strImagePath.c_str());
        return false;
    }

    Capture_NS::CaptureDirInfo_S stDirectoryInfo;
    stDirectoryInfo.nChnId = nChannelId;
    const int nRet = Db::CCaptureDatabase::instance()->get_itemInfo(stDirectoryInfo);
    stDirectoryInfo.nTotalSize += stCaptureInfo.nImageSize;
    ++stDirectoryInfo.nCount;
    if (nRet < 0)
    {
        return Db::CCaptureDatabase::instance()->add(stDirectoryInfo) >= 0;
    }
    return Db::CCaptureDatabase::instance()->update(stDirectoryInfo) >= 0;
}

void CFaceRecognition::ensureImageTimeInfo(
    FrameContext_S &stContext,
    ImageSaveInfo_S &stImageInfo) const
{
    ImageSaveInfo_S &stPanoramaImage = stContext.stPanoramaImage;
    if (stPanoramaImage.strImageDate.empty())
    {
        stPanoramaImage.strImageDate = TimeUtils_NS::get_currentDate();
        stPanoramaImage.strImageTime = TimeUtils_NS::get_currentTimeMs();
        stPanoramaImage.strCaptureTime =
            TimeUtils_NS::get_currentDateAndTimeNoT();
    }

    if (stImageInfo.strImageDate.empty())
    {
        stImageInfo.strImageDate = stPanoramaImage.strImageDate;
        stImageInfo.strImageTime = stPanoramaImage.strImageTime;
        stImageInfo.strCaptureTime = stPanoramaImage.strCaptureTime;
    }
}

bool CFaceRecognition::ensurePanoramaImageSaved(FrameContext_S &stContext)
{
    ImageSaveInfo_S &stPanoramaImage = stContext.stPanoramaImage;
    if (!stPanoramaImage.strImagePath.empty())
    {
        return true;
    }
    if (stPanoramaImage.stImage.empty())
    {
        return false;
    }

    ensureImageTimeInfo(stContext, stPanoramaImage);
    cv::Mat stPanoramaBgrImage;
    cv::cvtColor(
        stPanoramaImage.stImage,
        stPanoramaBgrImage,
        cv::COLOR_RGB2BGR);
    addCaptureOverlay(stPanoramaBgrImage);
    stPanoramaImage.strImagePath = saveCaptureImage(
        stPanoramaBgrImage,
        Alarm::UPLOAD_PANORAMIC_IMAGE,
        stPanoramaImage.strImageDate,
        stPanoramaImage.strImageTime,
        JPEG_QUALITY_PANORAMA);
    if (stPanoramaImage.strImagePath.empty())
    {
        return false;
    }

    saveCaptureRecord(
        stPanoramaImage.strImagePath,
        stContext.nChannelId,
        stPanoramaImage.strCaptureTime);
    return true;
}

bool CFaceRecognition::ensureTargetImageSaved(
    FrameContext_S &stContext,
    TargetResult_S &stTarget,
    int nTargetIndex)
{
    ImageSaveInfo_S &stTargetImage = stTarget.stTargetImage;
    if (!stTargetImage.strImagePath.empty())
    {
        return true;
    }

    if (!ensureTargetImage(stContext, stTarget))
    {
        return false;
    }

    ensureImageTimeInfo(stContext, stTargetImage);
    stTargetImage.strImagePath = saveCaptureImage(
        stTargetImage.stImage,
        Alarm::UPLOAD_TARGET_IMAGE,
        stTargetImage.strImageDate,
        stTargetImage.strImageTime,
        JPEG_QUALITY_TARGET,
        nTargetIndex);
    if (stTargetImage.strImagePath.empty())
    {
        return false;
    }

    saveCaptureRecord(
        stTargetImage.strImagePath,
        stContext.nChannelId,
        stTargetImage.strCaptureTime);
    return true;
}

void CFaceRecognition::processCaptureResult(FrameContext_S &stContext)
{
    processCapturePolicy(stContext);

    const auto &vstLinkage = stContext.stConfig.stLinkageList.tradition;
    const bool bSavePanorama = std::find(
                                   vstLinkage.begin(),
                                   vstLinkage.end(),
                                   static_cast<int>(Alarm::UPLOAD_PANORAMIC_IMAGE)) !=
                               vstLinkage.end();
    const bool bSaveTarget = std::find(
                                vstLinkage.begin(),
                                vstLinkage.end(),
                                static_cast<int>(Alarm::UPLOAD_TARGET_IMAGE)) !=
                            vstLinkage.end();
    if (!bSavePanorama && !bSaveTarget)
    {
        return;
    }

    const bool bHasTriggeredTarget = std::any_of(
        stContext.vstTargets.begin(),
        stContext.vstTargets.end(),
        [](const TargetResult_S &stTarget) {
            return stTarget.bCaptureTriggered;
        });
    if (!bHasTriggeredTarget)
    {
        return;
    }

    if (bSavePanorama)
    {
        ensurePanoramaImageSaved(stContext);
    }

    if (!bSaveTarget)
    {
        return;
    }

    for (size_t i = 0; i < stContext.vstTargets.size(); ++i)
    {
        auto &stTarget = stContext.vstTargets[i];
        if (stTarget.bCaptureTriggered)
        {
            ensureTargetImageSaved(
                stContext,
                stTarget,
                static_cast<int>(i));
        }
    }
}

void CFaceRecognition::pushAttributeResult(
    const FrameContext_S &stContext,
    const TargetResult_S &stTarget) const
{
    if (!stTarget.bAttributeReady ||
        (!stTarget.bCaptureTriggered &&
         !stTarget.bAttributePushTriggered) ||
        (stContext.stPanoramaImage.strImagePath.empty() &&
         stTarget.stTargetImage.strImagePath.empty()))
    {
        return;
    }

    const FaceAttribute_NS::Result_S &stAttribute = stTarget.stAttributeResult;
    Alarm::FaceAlarmInfo_S stFaceAlarmInfo;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMale = stAttribute.bIsMale;
    stFaceAlarmInfo.stFaceAlarmAttribute.nAgeLabel = stAttribute.nAgeLabel;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsGlasses = stAttribute.bIsGlasses;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsBeard = stAttribute.bIsBeard;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMask = stAttribute.bIsMask;
    stFaceAlarmInfo.stFaceAlarmAttribute.nEmotionLabel = stAttribute.nEmotionLabel;
    stFaceAlarmInfo.strCurrentPicture =
        stContext.stPanoramaImage.strImagePath;
    stFaceAlarmInfo.strFacePicture =
        stTarget.stTargetImage.strImagePath;
    stFaceAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();
    stFaceAlarmInfo.bIsDownLoad =
        !stFaceAlarmInfo.strCurrentPicture.empty() ||
        !stFaceAlarmInfo.strFacePicture.empty();

    const FaceDetect_NS::Result_S &stResult = stTarget.stDetectResult;
    const int nFrameWidth = stContext.stPanoramaImage.stImage.cols;
    const int nFrameHeight = stContext.stPanoramaImage.stImage.rows;
    if (nFrameWidth > 0 && nFrameHeight > 0)
    {
        const float fLeft = std::max(
            0.0F,
            std::min(100.0F, stResult.fX1 * 100.0F / nFrameWidth));
        const float fTop = std::max(
            0.0F,
            std::min(100.0F, stResult.fY1 * 100.0F / nFrameHeight));
        const float fRight = std::max(
            fLeft,
            std::min(100.0F, stResult.fX2 * 100.0F / nFrameWidth));
        const float fBottom = std::max(
            fTop,
            std::min(100.0F, stResult.fY2 * 100.0F / nFrameHeight));
        stFaceAlarmInfo.stFaceRegion.nPointNum = 4;
        stFaceAlarmInfo.stFaceRegion.aPoint = {
            Common::PosF_S{fLeft, fTop},
            Common::PosF_S{fRight, fTop},
            Common::PosF_S{fRight, fBottom},
            Common::PosF_S{fLeft, fBottom}};
    }

    TaskPublish::instance()->message(
        AC_PUSH_FACE_CAPTURE_INFO,
        Convert::to_string(stFaceAlarmInfo));
}

void CFaceRecognition::processAttributeResult(FrameContext_S &stContext)
{
    processAttributePushPolicy(stContext);

    for (size_t i = 0; i < stContext.vstTargets.size(); ++i)
    {
        auto &stTarget = stContext.vstTargets[i];
        if (!stTarget.bAttributeReady ||
            (!stTarget.bCaptureTriggered &&
             !stTarget.bAttributePushTriggered))
        {
            continue;
        }

        ensurePanoramaImageSaved(stContext);
        ensureTargetImageSaved(
            stContext,
            stTarget,
            static_cast<int>(i));
        pushAttributeResult(stContext, stTarget);
    }
}

#ifdef ENABLE_TVSDK_SRC
/**
 * @brief 沿用旧人脸抓拍协议直接组包，通过统一告警接口同步提交到 SDK。
 * @param [in,out] stContext 当前帧配置、检测和属性结果，按需准备 BGR 目标图。
 * @param [out] 无。
 * @return 无，单个目标失败不阻断其余目标或网页、GAT1400 输出。
 */
void CFaceRecognition::processTvSdkCaptureResult(FrameContext_S &stContext)
{
    if (!m_bEnabled.load() || !stContext.stConfig.bEnable ||
        !stContext.stConfig.bCaptureEnable || stContext.stPanoramaImage.stImage.empty())
    {
        return;
    }
    const bool bHasTriggeredTarget = std::any_of(
        stContext.vstTargets.begin(), stContext.vstTargets.end(),
        [](const TargetResult_S &stTarget) { return stTarget.bCaptureTriggered; });
    if (!bHasTriggeredTarget)
    {
        return;
    }

    /* 算法工作线程每帧只编码一次全景，局部容器保持到所有同步推送完成。 */
    EventTvSdkImage_S stPanoramaImage{};
    if (!face_recognition_encode_capture_image(stContext.stPanoramaImage.stImage,
                                               JPEG_QUALITY_PANORAMA, true, stPanoramaImage))
    {
        dlog_warn("人脸抓拍全景图编码失败或超限: channel[%d]", stContext.nChannelId);
        return;
    }
    const cv::Size stPanoramaSize = stContext.stPanoramaImage.stImage.size();
    const std::string strTimestamp = TimeUtils_NS::get_currentDateAndTimeNoT();
    for (size_t nTargetIndex = 0; nTargetIndex < stContext.vstTargets.size(); ++nTargetIndex)
    {
        TargetResult_S &stTarget = stContext.vstTargets[nTargetIndex];
        if (!stTarget.bCaptureTriggered)
        {
            continue;
        }
        cv::Rect stCropRect{};
        if (!face_recognition_get_capture_rect(stTarget.stDetectResult, stPanoramaSize, stCropRect))
        {
            dlog_warn("人脸抓拍目标框无效: target[%zu]", nTargetIndex);
            continue;
        }
        try
        {
            if (!ensureTargetImage(stContext, stTarget))
            {
                dlog_warn("人脸抓拍目标图准备失败: target[%zu]", nTargetIndex);
                continue;
            }
        }
        catch (const cv::Exception &stException)
        {
            dlog_warn("人脸抓拍目标图准备异常: target[%zu], reason[%s]",
                      nTargetIndex, stException.what());
            continue;
        }
        EventTvSdkImage_S stTargetImage{};
        if (!face_recognition_encode_capture_image(stTarget.stTargetImage.stImage,
                                                   JPEG_QUALITY_TARGET, false, stTargetImage))
        {
            dlog_warn("人脸抓拍目标图编码失败或超限: target[%zu]", nTargetIndex);
            continue;
        }

        NET_AlarmCaptureInfo_S stInfo{};
        stInfo.uAlarmType = NET_ALARM_CAPTURE_FACE;
        stInfo.uCaptureType = NET_CAPTURE_TYPE_FACE;
        stInfo.uChannel = static_cast<UINT32>(std::max(0, stContext.nChannelId));
        stInfo.llTimestampMs = stContext.llTimestamp;
        stInfo.uPanoramaWidth = static_cast<UINT32>(stPanoramaSize.width);
        stInfo.uPanoramaHeight = static_cast<UINT32>(stPanoramaSize.height);
        stInfo.stPanoramaImg.pData = stPanoramaImage.vecJpeg.data();
        stInfo.stPanoramaImg.uDataLen = static_cast<UINT32>(stPanoramaImage.vecJpeg.size());
        stInfo.uCropCount = 1;
        NET_CropImage_S &stCropImage = stInfo.stCropImages[0];
        stCropImage.uCropX = static_cast<UINT32>(stCropRect.x);
        stCropImage.uCropY = static_cast<UINT32>(stCropRect.y);
        stCropImage.uCropWidth = static_cast<UINT32>(stCropRect.width);
        stCropImage.uCropHeight = static_cast<UINT32>(stCropRect.height);
        stCropImage.uTargetType = NET_CAPTURE_TYPE_FACE;
        stCropImage.fConfidence = std::isfinite(stTarget.stDetectResult.fBoxConfidence)
                                     ? std::clamp(stTarget.stDetectResult.fBoxConfidence, 0.0F, 1.0F)
                                     : 0.0F;
        stCropImage.nTrackID = -1;
        stCropImage.stImage.pData = stTargetImage.vecJpeg.data();
        stCropImage.stImage.uDataLen = static_cast<UINT32>(stTargetImage.vecJpeg.size());

        const bool bAttributeReady = stContext.stConfig.bAttributeAnalysisEnable && stTarget.bAttributeReady;
        /* 旧协议没有属性有效位；未分析时保持零值，不伪造属性结果。 */
        if (bAttributeReady)
        {
            const FaceAttribute_NS::Result_S &stAttribute = stTarget.stAttributeResult;
            stInfo.stExtraInfo.bMale = stAttribute.bIsMale ? TRUE : FALSE;
            stInfo.stExtraInfo.nAgeLabel = stAttribute.nAgeLabel;
            stInfo.stExtraInfo.bGlasses = stAttribute.bIsGlasses ? TRUE : FALSE;
            stInfo.stExtraInfo.bBeard = stAttribute.bIsBeard ? TRUE : FALSE;
            stInfo.stExtraInfo.bMask = stAttribute.bIsMask ? TRUE : FALSE;
            stInfo.stExtraInfo.nEmotionLabel = stAttribute.nEmotionLabel;
        }
        const float fLeft = stCropRect.x * FACE_CAPTURE_REGION_SCALE / stPanoramaSize.width;
        const float fTop = stCropRect.y * FACE_CAPTURE_REGION_SCALE / stPanoramaSize.height;
        const float fRight = (stCropRect.x + stCropRect.width) * FACE_CAPTURE_REGION_SCALE / stPanoramaSize.width;
        const float fBottom = (stCropRect.y + stCropRect.height) * FACE_CAPTURE_REGION_SCALE / stPanoramaSize.height;
        NET_CapturePolygon_S &stRegion = stInfo.stExtraInfo.stTargetRegion;
        stRegion.uPointCount = FACE_CAPTURE_REGION_POINT_COUNT;
        stRegion.afPointX[0] = fLeft;
        stRegion.afPointY[0] = fTop;
        stRegion.afPointX[1] = fRight;
        stRegion.afPointY[1] = fTop;
        stRegion.afPointX[2] = fRight;
        stRegion.afPointY[2] = fBottom;
        stRegion.afPointX[3] = fLeft;
        stRegion.afPointY[3] = fBottom;
        std::strncpy(stInfo.stExtraInfo.strTimestamp, strTimestamp.c_str(),
                     sizeof(stInfo.stExtraInfo.strTimestamp) - 1);

        /* SDK 在返回前完成图片序列化；后续发送队列持有 JSON，不持有这些局部图片指针。 */
        const int nRet = ControlManage::instance()->tvsdk_push_alarm(
            NET_ALARM_CAPTURE_FACE, &stInfo, static_cast<int>(sizeof(stInfo)));
        if (nRet < 0)
        {
            dlog_warn("TVSDK人脸抓拍推送失败: target[%zu], ret[%d]", nTargetIndex, nRet);
        }
        else
        {
            dlog_info("TVSDK人脸抓拍推送成功: cmd[0x%x], channel[%u], target[%zu], "
                      "time[%lld], panorama[%u], face[%u], attribute_ready[%d]",
                      NET_ALARM_CAPTURE_FACE, stInfo.uChannel, nTargetIndex,
                      static_cast<long long>(stInfo.llTimestampMs), stInfo.stPanoramaImg.uDataLen,
                      stCropImage.stImage.uDataLen, bAttributeReady);
        }
    }
}
#endif

void CFaceRecognition::processGat1400Result(FrameContext_S &stContext)
{
#ifdef ENABLE_GAT1400_SRC
    if (stContext.vstTargets.empty())
    {
        return;
    }

    Network::Gat1400Client_S stGat1400Config;
    GAT1400::CGAT1400::instance()->getGat1400Config(stGat1400Config);
    if (!stGat1400Config.enableGat1400 || !stGat1400Config.enableFace)
    {
        return;
    }

    for (auto &stTarget : stContext.vstTargets)
    {
        ensureTargetImage(stContext, stTarget);
    }
    if (stContext.stPanoramaImage.stImage.empty())
    {
        dlog_error("ai_app: GAT1400人脸全景图为空");
        return;
    }

    const auto encodeBgrImage = [](
                                    const cv::Mat &stBgrImage,
                                    int nJpegQuality,
                                    const char *pImageType,
                                    security_subimage_info_t &stImageInfo) {
        if (stBgrImage.empty())
        {
            return false;
        }

        std::vector<unsigned char> vstJpegData;
        const std::vector<int> vstJpegParams = {
            cv::IMWRITE_JPEG_QUALITY,
            nJpegQuality};
        if (!cv::imencode(".jpg", stBgrImage, vstJpegData, vstJpegParams))
        {
            return false;
        }

        stImageInfo.Data.assign(
            reinterpret_cast<const char *>(vstJpegData.data()),
            vstJpegData.size());
        stImageInfo.FileFormat = "Jpeg";
        stImageInfo.Width = stBgrImage.cols;
        stImageInfo.Height = stBgrImage.rows;
        stImageInfo.Type = pImageType;
        return true;
    };

    cv::Mat stPanoramaBgrImage;
    cv::cvtColor(
        stContext.stPanoramaImage.stImage,
        stPanoramaBgrImage,
        cv::COLOR_RGB2BGR);
    security_subimage_info_t stPanoramaImage;
    if (!encodeBgrImage(
            stPanoramaBgrImage,
            JPEG_QUALITY_PANORAMA,
            IMAGE_TYPE_SCENE,
            stPanoramaImage))
    {
        dlog_error("ai_app: GAT1400人脸全景图编码失败");
        return;
    }

    security_faces_t vstFaces;
    for (const auto &stTarget : stContext.vstTargets)
    {
        security_face_t stFace;
        stFace.InfoKind = SecurityInfoType::Auto;
        const FaceDetect_NS::Result_S &stResult = stTarget.stDetectResult;
        const int nFrameWidth = stContext.stPanoramaImage.stImage.cols;
        const int nFrameHeight = stContext.stPanoramaImage.stImage.rows;
        stFace.LeftTopX = std::max(
            0,
            std::min(nFrameWidth, static_cast<int>(std::lround(stResult.fX1))));
        stFace.LeftTopY = std::max(
            0,
            std::min(nFrameHeight, static_cast<int>(std::lround(stResult.fY1))));
        stFace.RightBtmX = std::max(
            stFace.LeftTopX,
            std::min(nFrameWidth, static_cast<int>(std::lround(stResult.fX2))));
        stFace.RightBtmY = std::max(
            stFace.LeftTopY,
            std::min(nFrameHeight, static_cast<int>(std::lround(stResult.fY2))));

        if (stTarget.bAttributeReady)
        {
            const FaceAttribute_NS::Result_S &stAttribute =
                stTarget.stAttributeResult;
            stFace.GenderCode = stAttribute.bIsMale ? "1" : "2";
            if (stAttribute.nAgeLabel > 0 &&
                static_cast<size_t>(stAttribute.nAgeLabel) <=
                    g_vAgeLabelList.size())
            {
                const auto &stAgeRange =
                    g_vAgeLabelList[stAttribute.nAgeLabel - 1];
                stFace.AgeLowerLimit = stAgeRange.first;
                stFace.AgeUpLimit = stAgeRange.second;
            }
            stFace.GlassStyle = stAttribute.bIsGlasses ? "99" : "";
            stFace.GlassColor = stAttribute.bIsGlasses ? "99" : "";
            stFace.MustacheStyle = stAttribute.bIsBeard ? "留有胡子" : "";
            stFace.RespiratorColor = stAttribute.bIsMask ? "99" : "";
        }

        stFace.SubImageList.push_back(stPanoramaImage);
        if (!stTarget.stTargetImage.stImage.empty())
        {
            security_subimage_info_t stTargetImage;
            if (encodeBgrImage(
                    stTarget.stTargetImage.stImage,
                    JPEG_QUALITY_TARGET,
                    IMAGE_TYPE_FACE,
                    stTargetImage))
            {
                stFace.SubImageList.push_back(std::move(stTargetImage));
            }
            else
            {
                dlog_warn("ai_app: GAT1400人脸目标图编码失败");
            }
        }

        vstFaces.push_back(std::move(stFace));
    }

    const int nRet = GAT1400::CGAT1400::instance()->uploadFaces(vstFaces);
    if (nRet != 0)
    {
        dlog_warn("ai_app: GAT1400人脸推送失败[%d]", nRet);
    }
#else
    (void)stContext;
#endif
}

void CFaceRecognition::outputOsd(const FrameContext_S &stContext)
{
    if (!stContext.stConfig.bDynamicAnalysisEnable)
    {
        return;
    }

    std::vector<Common::RectInfo_S> vstAcceptedRect;
    vstAcceptedRect.reserve(stContext.vstTargets.size());
    for (const auto &stTarget : stContext.vstTargets)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = stTarget.stDetectResult.fX1;
        stRectInfo.nY1 = stTarget.stDetectResult.fY1;
        stRectInfo.nX2 = stTarget.stDetectResult.fX2;
        stRectInfo.nY2 = stTarget.stDetectResult.fY2;
        vstAcceptedRect.emplace_back(stRectInfo);
    }

    send_detectionResult_to_osd(
        stContext.stPanoramaImage.stImage.cols,
        stContext.stPanoramaImage.stImage.rows,
        vstAcceptedRect);
}

void CFaceRecognition::processFaceRecognitionEvent(
    const FrameContext_S &stContext)
{
    const TargetResult_S *pBestTarget = getBestTarget(stContext);

    EventTriggerContext_S stEventContext;
    stEventContext.enEventType = Event::Type_E::FACE_RECOGNITION;
    stEventContext.nChnId = stContext.nChannelId;
    stEventContext.llTimestamp = stContext.llTimestamp;
    stEventContext.mapAttrs["capture_enabled"] =
        stContext.stConfig.bCaptureEnable ? "1" : "0";
    stEventContext.mapAttrs["attribute_enabled"] =
        stContext.stConfig.bAttributeAnalysisEnable ? "1" : "0";
    const bool bCaptureTriggered = std::any_of(
        stContext.vstTargets.begin(),
        stContext.vstTargets.end(),
        [](const TargetResult_S &stTarget) {
            return stTarget.bCaptureTriggered;
        });
    stEventContext.mapAttrs["capture_triggered"] =
        bCaptureTriggered ? "1" : "0";
    const auto &vstLinkage = stContext.stConfig.stLinkageList.tradition;
    const bool bAttachPanorama =
        bCaptureTriggered &&
        std::find(
            vstLinkage.begin(),
            vstLinkage.end(),
            static_cast<int>(Alarm::UPLOAD_PANORAMIC_IMAGE)) !=
            vstLinkage.end();

    if (pBestTarget && pBestTarget->bAttributeReady)
    {
        const FaceAttribute_NS::Result_S &stAttribute =
            pBestTarget->stAttributeResult;
        stEventContext.mapAttrs["attribute_ready"] = "1";
        stEventContext.mapAttrs["gender"] = stAttribute.bIsMale ? "male" : "female";
        stEventContext.mapAttrs["age"] = std::to_string(stAttribute.nAgeLabel);
        stEventContext.mapAttrs["glasses"] = stAttribute.bIsGlasses ? "1" : "0";
        stEventContext.mapAttrs["beard"] = stAttribute.bIsBeard ? "1" : "0";
        stEventContext.mapAttrs["mask"] = stAttribute.bIsMask ? "1" : "0";
        stEventContext.mapAttrs["emotion"] = std::to_string(stAttribute.nEmotionLabel);
    }
    else
    {
        stEventContext.mapAttrs["attribute_ready"] = "0";
    }

    if (pBestTarget)
    {
        const FaceDetect_NS::Result_S &stDetectResult = pBestTarget->stDetectResult;
        stEventContext.nTargetId = -1;
        stEventContext.nObjectType = 1;
        stEventContext.fConfidence = stDetectResult.fBoxConfidence;
        stEventContext.nLeft = static_cast<int>(stDetectResult.fX1);
        stEventContext.nTop = static_cast<int>(stDetectResult.fY1);
        stEventContext.nRight = static_cast<int>(stDetectResult.fX2);
        stEventContext.nBottom = static_cast<int>(stDetectResult.fY2);

        if (bAttachPanorama &&
            !stContext.stPanoramaImage.strImagePath.empty())
        {
            stEventContext.mapAttrs["CaptureImagePath"] =
                stContext.stPanoramaImage.strImagePath;
        }

        if (bAttachPanorama &&
            !stContext.stPanoramaImage.stImage.empty())
        {
            encode_mat_to_tvsdk_image(
                stContext.stPanoramaImage.stImage,
                stEventContext.stPanoramaImage,
                JPEG_QUALITY_PANORAMA);
        }
    }

    m_faceRecognitionStateMachine.handleAlarmState(
        !stContext.vstTargets.empty(),
        stEventContext);
}

void CFaceRecognition::processBusinessOutput(FrameContext_S &stContext)
{
    outputOsd(stContext);
    processCaptureResult(stContext);
    processAttributeResult(stContext);
#ifdef ENABLE_TVSDK_SRC
    processTvSdkCaptureResult(stContext);
#endif
    processGat1400Result(stContext);
    processFaceRecognitionEvent(stContext);
}

void CFaceRecognition::run()
{
    while (m_bRunning.load())
    {
        if (!init())
        {
            std::unique_lock<std::mutex> lock(m_threadMutex);
            m_threadCondition.wait_for(lock, std::chrono::seconds(1), [this]() {
                return !m_bRunning.load();
            });
            continue;
        }

        MediaData_S stMediaData;
        m_frameQueue.pop(stMediaData, -1);

        if (!m_bRunning.load())
        {
            break;
        }
        if (!m_bEnabled.load() || stMediaData.nSize == 0)
        {
            continue;
        }

        FrameContext_S stContext;
        if (!prepareFrameContext(stMediaData, stContext))
        {
            continue;
        }
        if (!processAlgorithms(stContext))
        {
            continue;
        }
        processBusinessOutput(stContext);
    }
}
