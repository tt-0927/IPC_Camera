/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 人脸识别事件子检测器实现
 */

#include "face_recognition_sub_detector.hpp"

#include "action_code.h"
#include "algorithm_define.hpp" /* 模型配置路径宏 */
#include "capture_database.h"
#include "convert_interface.h"
#include "dlog.h"
#include "event_configure.h"
#include "storage_manage.h"
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
constexpr int FACE_ATTRIBUTE_WIDTH = 192;
constexpr int FACE_ATTRIBUTE_HEIGHT = 192;
constexpr int FACE_EXPRESSION_WIDTH = 224;
constexpr int FACE_EXPRESSION_HEIGHT = 224;
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
static bool face_recognition_get_capture_rect(const FaceDetectedTarget_S &stResult,
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
    const std::vector<FaceDetectedTarget_S> &vstLastTargets,
    const std::vector<bool> &vbMatchedTargets,
    const FaceDetectedTarget_S &stCurrentTarget)
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

CFaceRecognitionSubDetector::CFaceRecognitionSubDetector()
{
    dlog_info("FACE_SUB: 人脸识别子检测器创建");
}

CFaceRecognitionSubDetector::~CFaceRecognitionSubDetector()
{
    m_stateMachine.endAlarmImmediately(Event::Type_E::FACE_RECOGNITION);

    delete m_pFaceAttributeHandle;
    m_pFaceAttributeHandle = nullptr;
    delete m_pFaceExpressionHandle;
    m_pFaceExpressionHandle = nullptr;
}

void CFaceRecognitionSubDetector::updateConfig()
{
    Alarm::FaceRecognition_S stConfig;
    CEventConfigure::instance()->get_configure(stConfig);

    /* 使能或属性分析开关跳变时，算法线程清属性推送去重基线 */
    bool bResetAttributeTargets = false;
    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        bResetAttributeTargets =
            m_stConfig.bEnable != stConfig.bEnable ||
            m_stConfig.bAttributeAnalysisEnable != stConfig.bAttributeAnalysisEnable;
        m_stConfig = stConfig;
    }
    if (bResetAttributeTargets)
    {
        m_bResetAttributeTargets.store(true);
    }

    /* 处理器规则与配置快照同源更新 */
    m_processor.setRules(stConfig);

    /* 抓拍间隔仅作为事件消费节拍（EventManager 内部加锁，跨线程安全） */
    const long lCaptureIntervalMs =
        static_cast<long>(std::max(1, stConfig.stCaptureRule.nInterval)) * 1000L;
    m_faceCaptureManager.setTimeWindow(lCaptureIntervalMs);
}

bool CFaceRecognitionSubDetector::isEventEnabled() const
{
    std::lock_guard<std::mutex> lock(m_mutexConfig);
    return m_stConfig.bEnable;
}

void CFaceRecognitionSubDetector::reset()
{
    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        m_stConfig.bEnable = false;
    }
    m_bResetAttributeTargets.store(true);
}

int CFaceRecognitionSubDetector::process(const DetectContext_S &stDetectContext)
{
    /* 配置快照（updateConfig 可能来自配置线程） */
    Alarm::FaceRecognition_S stConfig;
    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        if (!m_stConfig.bEnable)
        {
            return 0;
        }
        stConfig = m_stConfig;
    }

    /* 跨会话/开关跳变清去重基线（向量仅算法线程访问） */
    if (m_bResetAttributeTargets.exchange(false))
    {
        m_vstLastAttributeTargets.clear();
    }

    /* 基础目标 → 富目标（特化分析结果仅本子检测器内部流转，与基础目标同序） */
    std::vector<FaceTargetResult_S> vstTargets;
    vstTargets.reserve(stDetectContext.vstDetectResult.size());
    for (const auto &stBase : stDetectContext.vstDetectResult)
    {
        vstTargets.emplace_back(stBase);
    }

    /* 特化分析：属性/表情按配置逐帧执行 */
    processAttributeAnalysis(stDetectContext.stFrame.matPanoramaImage, vstTargets, stConfig);
    processExpressionAnalysis(stDetectContext.stFrame.matPanoramaImage, vstTargets, stConfig);

    /* 人脸识别事件按抓拍间隔节拍消费 */
    if (m_faceCaptureManager.handleEvent(stDetectContext.stFrame.nChannelId))
    {
        processBusinessOutput(stDetectContext.stFrame, stDetectContext.vstDetectResult,
                              vstTargets, stConfig);
    }
    return 0;
}

void CFaceRecognitionSubDetector::processAttributeAnalysis(
    const cv::Mat &stRgbImage,
    std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig)
{
    /* 事件使能已在 process 判定，这里只看属性分析开关 */
    if (!stConfig.bAttributeAnalysisEnable)
    {
        return;
    }

    for (auto &stTarget : vstTargets)
    {
        stTarget.bAttributeReady = analyzeFaceAttribute(
            stRgbImage,
            stTarget.stDetectResult,
            stTarget.stAttributeResult);
    }
}

void CFaceRecognitionSubDetector::processExpressionAnalysis(
    const cv::Mat &stRgbImage,
    std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig)
{
    if (!stConfig.bFaceExpressionEnable)
    {
        return;
    }

    for (auto &stTarget : vstTargets)
    {
        stTarget.bExpressionReady = analyzeFaceExpression(
            stRgbImage,
            stTarget.stDetectResult,
            stTarget.nExpressionLabel);
    }
}

void CFaceRecognitionSubDetector::processBusinessOutput(
    const FrameContext_S &stFrame,
    const std::vector<FaceBaseTarget_S> &vstBaseTargets,
    const std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig)
{
    // 人脸识别处理器处理, 输出目标符合规则要求（候选下标映射回富目标）
    std::vector<int> vstCandidateIndices;
    m_processor.process(vstBaseTargets, vstCandidateIndices);
    std::vector<FaceTargetResult_S> vstRuleTargets;
    vstRuleTargets.reserve(vstCandidateIndices.size());
    for (const int nIndex : vstCandidateIndices)
    {
        vstRuleTargets.push_back(vstTargets[nIndex]);
    }

    // 输出OSD
    if (stConfig.bDynamicAnalysisEnable && !vstRuleTargets.empty())
    {
        outputOsd(stFrame, vstRuleTargets);
    }

    // 判断是否需要保存数据库
    std::vector<bool> vbPushFlags;
    updateAttributePushFlags(vstRuleTargets, stConfig, vbPushFlags);
    FrameCaptureResult_S stSaveResult;
    saveAlarmImages(stFrame, vstRuleTargets, stConfig, vbPushFlags, stSaveResult);
    // 判断是否需要推送属性信息
    processAttributeResult(stFrame, vstRuleTargets, stConfig,
                           vbPushFlags, stSaveResult);
#ifdef ENABLE_TVSDK_SRC
    processTvSdkCaptureResult(stFrame, vstTargets, stConfig);
#endif
    // GAT1400推送
    processGat1400Result(stFrame, vstRuleTargets);
    // 处理人脸识别事件
    processFaceRecognitionEvent(stFrame, vstRuleTargets, stConfig, stSaveResult);
}

bool CFaceRecognitionSubDetector::initAttributeModel()
{
    if (m_pFaceAttributeHandle)
    {
        return true;
    }

    m_pFaceAttributeHandle = new CFaceAttributeModel(FACE_ATTRIBUTE_MODEL_CONFIG_PATH);
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

bool CFaceRecognitionSubDetector::initExpressionModel()
{
    if (m_pFaceExpressionHandle)
    {
        return true;
    }

    m_pFaceExpressionHandle = new CFaceExpression(FACE_EXPRESSION_MODEL_CONFIG_PATH);
    if (!m_pFaceExpressionHandle || !m_pFaceExpressionHandle->init())
    {
        delete m_pFaceExpressionHandle;
        m_pFaceExpressionHandle = nullptr;
        dlog_error("ai_app: 人脸识别表情模型初始化失败");
        return false;
    }

    dlog_info("ai_app: 人脸识别表情模型初始化成功");
    return true;
}

bool CFaceRecognitionSubDetector::analyzeFaceAttribute(
    const cv::Mat &stRgbImage,
    const FaceDetectedTarget_S &stResult,
    FaceAttributeTarget_S &stAttributeResult)
{
    if (!initAttributeModel())
    {
        return false;
    }

    CFaceAttributeModel::InData_S stAttributeInput;
    if (!cropFaceForModel(
            stRgbImage,
            stResult,
            FACE_ATTRIBUTE_WIDTH,
            FACE_ATTRIBUTE_HEIGHT,
            stAttributeInput.inMat))
    {
        return false;
    }

    return m_pFaceAttributeHandle->process(stAttributeInput, stAttributeResult) == 0;
}

bool CFaceRecognitionSubDetector::analyzeFaceExpression(
    const cv::Mat &stRgbImage,
    const FaceDetectedTarget_S &stResult,
    int &nExpressionLabel)
{
    nExpressionLabel = -1;

    if (!initExpressionModel())
    {
        return false;
    }

    CFaceExpression::InData_S stExpressionInput;
    if (!cropFaceForModel(
            stRgbImage,
            stResult,
            FACE_EXPRESSION_WIDTH,
            FACE_EXPRESSION_HEIGHT,
            stExpressionInput.inMat))
    {
        return false;
    }

    nExpressionLabel = m_pFaceExpressionHandle->process(stExpressionInput);
    return nExpressionLabel >= 0;
}

const FaceTargetResult_S *CFaceRecognitionSubDetector::getBestTarget(
    const std::vector<FaceTargetResult_S> &vstTargetResults) const
{
    const FaceTargetResult_S *pBestTarget = nullptr;
    for (const auto &stTarget : vstTargetResults)
    {
        if (pBestTarget == nullptr ||
            stTarget.fQualityScore > pBestTarget->fQualityScore)
        {
            pBestTarget = &stTarget;
        }
    }
    return pBestTarget;
}

void CFaceRecognitionSubDetector::updateAttributePushFlags(
    const std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig,
    std::vector<bool> &vbPushFlags)
{
    vbPushFlags.assign(vstTargets.size(), false);
    if (!stConfig.bEnable || !stConfig.bAttributeAnalysisEnable)
    {
        m_vstLastAttributeTargets.clear();
        return;
    }

    std::vector<bool> vbMatchedTargets(
        m_vstLastAttributeTargets.size(),
        false);
    for (size_t i = 0; i < vstTargets.size(); ++i)
    {
        const FaceTargetResult_S &stTarget = vstTargets[i];
        if (!stTarget.bAttributeReady)
        {
            continue;
        }

        const int nMatchedIndex = findSameFaceTargetIndex(
            m_vstLastAttributeTargets,
            vbMatchedTargets,
            stTarget.stDetectResult);
        vbPushFlags[i] = nMatchedIndex < 0;
        if (nMatchedIndex >= 0)
        {
            vbMatchedTargets[static_cast<size_t>(nMatchedIndex)] = true;
        }
    }

    m_vstLastAttributeTargets.clear();
    m_vstLastAttributeTargets.reserve(vstTargets.size());
    for (const auto &stTarget : vstTargets)
    {
        if (stTarget.bAttributeReady)
        {
            m_vstLastAttributeTargets.emplace_back(stTarget.stDetectResult);
        }
    }
}

bool CFaceRecognitionSubDetector::cropCloseupImageByRga(
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

cv::Mat CFaceRecognitionSubDetector::cropTargetImage(
    const FrameContext_S &stContext,
    const FaceTargetResult_S &stTarget) const
{
    cv::Mat stTargetBgrImage;

    const FaceDetectedTarget_S &stResult = stTarget.stDetectResult;
    const cv::Rect2f stFaceRect(
        stResult.fX1,
        stResult.fY1,
        stResult.fX2 - stResult.fX1,
        stResult.fY2 - stResult.fY1);

    const CloseupSource_S &stCloseupSource = stContext.stCloseupSource;
    if (stCloseupSource.isValid())
    {
        bool bCropSuccess = cropCloseupImageByRga(
            stCloseupSource,
            stFaceRect,
            stContext.matPanoramaImage.size(),
            stTargetBgrImage);
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
                stContext.matPanoramaImage.size(),
                stTargetBgrImage,
                FACE_CLOSEUP_MARGIN_X,
                FACE_CLOSEUP_MARGIN_TOP,
                FACE_CLOSEUP_MARGIN_BOTTOM,
                FACE_CLOSEUP_MAX_SIDE);
        }
        if (bCropSuccess)
        {
            const int nShortSide = std::min(
                stTargetBgrImage.cols,
                stTargetBgrImage.rows);
            if (nShortSide > 0 && nShortSide < FACE_CLOSEUP_MIN_SIDE)
            {
                const double dScale =
                    static_cast<double>(FACE_CLOSEUP_MIN_SIDE) / nShortSide;
                cv::resize(
                    stTargetBgrImage,
                    stTargetBgrImage,
                    cv::Size(),
                    dScale,
                    dScale,
                    cv::INTER_CUBIC);
            }
            return stTargetBgrImage;
        }
        dlog_warn("ai_app: 人脸识别特写源裁剪失败, 回退全景图裁剪");
    }

    cv::Mat stFaceImage;
    if (!cropTargetImageFace(
            stContext.matPanoramaImage,
            stFaceRect,
            stContext.matPanoramaImage.size(),
            stFaceImage))
    {
        dlog_warn("ai_app: 人脸识别目标小图裁剪失败");
        return {};
    }

    cv::cvtColor(
        stFaceImage,
        stTargetBgrImage,
        cv::COLOR_RGB2BGR);
    return stTargetBgrImage;
}

void CFaceRecognitionSubDetector::addCaptureOverlay(cv::Mat &stBgrImage) const
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

std::string CFaceRecognitionSubDetector::saveCaptureImage(
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

bool CFaceRecognitionSubDetector::saveCaptureRecord(
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

void CFaceRecognitionSubDetector::saveAlarmImages(
    const FrameContext_S &stContext,
    const std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig,
    const std::vector<bool> &vbPushFlags,
    FrameCaptureResult_S &stResult)
{
    /* 空目标帧不落盘，避免无意义的图片和数据库记录 */
    if (vstTargets.empty())
    {
        return;
    }

    stResult.vstTargets.assign(vstTargets.size(), CaptureRecord_S{});

    /* 属性需推送的目标：抓拍开启时全推，关闭时仅新出现人脸 */
    std::vector<bool> vbAttributePush(vstTargets.size(), false);
    bool bAnyAttributePush = false;
    for (size_t i = 0; i < vstTargets.size(); ++i)
    {
        vbAttributePush[i] = vstTargets[i].bAttributeReady &&
                             (stConfig.bCaptureEnable || vbPushFlags[i]);
        bAnyAttributePush = bAnyAttributePush || vbAttributePush[i];
    }

    const auto &vstLinkage = stConfig.stLinkageList.tradition;
    const bool bLinkagePanorama = std::find(
                                      vstLinkage.begin(),
                                      vstLinkage.end(),
                                      static_cast<int>(Alarm::UPLOAD_PANORAMIC_IMAGE)) !=
                                  vstLinkage.end();
    const bool bLinkageTarget = std::find(
                                    vstLinkage.begin(),
                                    vstLinkage.end(),
                                    static_cast<int>(Alarm::UPLOAD_TARGET_IMAGE)) !=
                                vstLinkage.end();

    /* 抓拍链路按联动配置保存，属性推送需要消息附带图片路径 */
    const bool bNeedPanorama =
        (stConfig.bCaptureEnable && bLinkagePanorama) || bAnyAttributePush;
    bool bAnyTargetSave = bAnyAttributePush;
    std::vector<bool> vbNeedTargetSave(vstTargets.size(), false);
    for (size_t i = 0; i < vstTargets.size(); ++i)
    {
        vbNeedTargetSave[i] =
            (stConfig.bCaptureEnable && bLinkageTarget) || vbAttributePush[i];
        bAnyTargetSave = bAnyTargetSave || vbNeedTargetSave[i];
    }
    if (!bNeedPanorama && !bAnyTargetSave)
    {
        return;
    }

    /* 一帧统一时间基准，文件名与数据库记录共用 */
    CaptureRecord_S stTimeBase;
    stTimeBase.strImageDate = TimeUtils_NS::get_currentDate();
    stTimeBase.strImageTime = TimeUtils_NS::get_currentTimeMs();
    stTimeBase.strCaptureTime = TimeUtils_NS::get_currentDateAndTimeNoT();

    if (bNeedPanorama && !stContext.matPanoramaImage.empty())
    {
        cv::Mat stPanoramaBgrImage;
        cv::cvtColor(
            stContext.matPanoramaImage,
            stPanoramaBgrImage,
            cv::COLOR_RGB2BGR);
        addCaptureOverlay(stPanoramaBgrImage);

        stResult.stPanorama = stTimeBase;
        stResult.stPanorama.strImagePath = saveCaptureImage(
            stPanoramaBgrImage,
            Alarm::UPLOAD_PANORAMIC_IMAGE,
            stTimeBase.strImageDate,
            stTimeBase.strImageTime,
            JPEG_QUALITY_PANORAMA);
        if (!stResult.stPanorama.strImagePath.empty())
        {
            saveCaptureRecord(
                stResult.stPanorama.strImagePath,
                stContext.nChannelId,
                stTimeBase.strCaptureTime);
        }
    }

    for (size_t i = 0; i < vstTargets.size(); ++i)
    {
        if (!vbNeedTargetSave[i])
        {
            continue;
        }

        const cv::Mat stTargetBgrImage = cropTargetImage(stContext, vstTargets[i]);
        if (stTargetBgrImage.empty())
        {
            continue;
        }

        CaptureRecord_S &stRecord = stResult.vstTargets[i];
        stRecord = stTimeBase;
        stRecord.strImagePath = saveCaptureImage(
            stTargetBgrImage,
            Alarm::UPLOAD_TARGET_IMAGE,
            stTimeBase.strImageDate,
            stTimeBase.strImageTime,
            JPEG_QUALITY_TARGET,
            static_cast<int>(i));
        if (!stRecord.strImagePath.empty())
        {
            saveCaptureRecord(
                stRecord.strImagePath,
                stContext.nChannelId,
                stTimeBase.strCaptureTime);
        }
    }
}

void CFaceRecognitionSubDetector::pushAttributeResult(
    const FrameContext_S &stContext,
    const FaceTargetResult_S &stTarget,
    const CaptureRecord_S &stPanoramaRecord,
    const CaptureRecord_S &stTargetRecord) const
{
    if (!stTarget.bAttributeReady ||
        (stPanoramaRecord.strImagePath.empty() &&
         stTargetRecord.strImagePath.empty()))
    {
        return;
    }

    const FaceAttributeTarget_S &stAttribute = stTarget.stAttributeResult;
    Alarm::FaceAlarmInfo_S stFaceAlarmInfo;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMale = stAttribute.bIsMale;
    stFaceAlarmInfo.stFaceAlarmAttribute.nAgeLabel = stAttribute.nAgeLabel;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsGlasses = stAttribute.bIsGlasses;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsBeard = stAttribute.bIsBeard;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMask = stAttribute.bIsMask;
    stFaceAlarmInfo.stFaceAlarmAttribute.nEmotionLabel = stAttribute.nEmotionLabel;
    stFaceAlarmInfo.strCurrentPicture = stPanoramaRecord.strImagePath;
    stFaceAlarmInfo.strFacePicture = stTargetRecord.strImagePath;
    stFaceAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();
    stFaceAlarmInfo.bIsDownLoad =
        !stFaceAlarmInfo.strCurrentPicture.empty() ||
        !stFaceAlarmInfo.strFacePicture.empty();

    const FaceDetectedTarget_S &stResult = stTarget.stDetectResult;
    const int nFrameWidth = stContext.matPanoramaImage.cols;
    const int nFrameHeight = stContext.matPanoramaImage.rows;
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

void CFaceRecognitionSubDetector::processAttributeResult(
    const FrameContext_S &stContext,
    const std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig,
    const std::vector<bool> &vbPushFlags,
    const FrameCaptureResult_S &stSaveResult)
{
    /* 抓拍开启时目标组已过滤为抓拍候选，属性就绪即推送；
       关闭时仅推送相对上一帧新出现的人脸；图片路径取自保存记录 */
    const bool bPushAll = stConfig.bCaptureEnable;

    for (size_t i = 0; i < vstTargets.size(); ++i)
    {
        const FaceTargetResult_S &stTarget = vstTargets[i];
        if (!stTarget.bAttributeReady ||
            (!bPushAll && !vbPushFlags[i]))
        {
            continue;
        }

        pushAttributeResult(
            stContext,
            stTarget,
            stSaveResult.stPanorama,
            stSaveResult.vstTargets[i]);
    }
}

#ifdef ENABLE_TVSDK_SRC
/**
 * @brief 沿用旧人脸抓拍协议直接组包，通过统一告警接口同步提交到 SDK。
 * @param [in] stContext 当前帧信息，提供全景图与通道时间戳。
 * @param [in] vstTargets 当前帧检测目标。
 * @param [in] stConfig 人脸识别运行配置快照。
 * @param [out] 无。
 * @return 无，单个目标失败不阻断其余目标或网页、GAT1400 输出。
 */
void CFaceRecognitionSubDetector::processTvSdkCaptureResult(
    const FrameContext_S &stContext,
    const std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig)
{
    if (!stConfig.bEnable ||
        !stConfig.bCaptureEnable || stContext.matPanoramaImage.empty() ||
        vstTargets.empty())
    {
        return;
    }

    /* 算法工作线程每帧只编码一次全景，局部容器保持到所有同步推送完成。 */
    EventTvSdkImage_S stPanoramaImage{};
    if (!face_recognition_encode_capture_image(stContext.matPanoramaImage,
                                               JPEG_QUALITY_PANORAMA, true, stPanoramaImage))
    {
        dlog_warn("人脸抓拍全景图编码失败或超限: channel[%d]", stContext.nChannelId);
        return;
    }
    const cv::Size stPanoramaSize = stContext.matPanoramaImage.size();
    const std::string strTimestamp = TimeUtils_NS::get_currentDateAndTimeNoT();
    for (size_t nTargetIndex = 0; nTargetIndex < vstTargets.size(); ++nTargetIndex)
    {
        const FaceTargetResult_S &stTarget = vstTargets[nTargetIndex];
        cv::Rect stCropRect{};
        if (!face_recognition_get_capture_rect(stTarget.stDetectResult, stPanoramaSize, stCropRect))
        {
            dlog_warn("人脸抓拍目标框无效: target[%zu]", nTargetIndex);
            continue;
        }
        cv::Mat matTargetImage;
        try
        {
            matTargetImage = cropTargetImage(stContext, stTarget);
        }
        catch (const cv::Exception &stException)
        {
            dlog_warn("人脸抓拍目标图裁剪异常: target[%zu], reason[%s]",
                      nTargetIndex, stException.what());
            continue;
        }
        EventTvSdkImage_S stTargetImage{};
        if (!face_recognition_encode_capture_image(matTargetImage,
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
        stCropImage.fConfidence = std::isfinite(stTarget.stDetectResult.fConfidence)
                                     ? std::clamp(stTarget.stDetectResult.fConfidence, 0.0F, 1.0F)
                                     : 0.0F;
        stCropImage.nTrackID = -1;
        stCropImage.stImage.pData = stTargetImage.vecJpeg.data();
        stCropImage.stImage.uDataLen = static_cast<UINT32>(stTargetImage.vecJpeg.size());

        const bool bAttributeReady = stConfig.bAttributeAnalysisEnable && stTarget.bAttributeReady;
        const bool bFaceExpressionReady = stConfig.bFaceExpressionEnable && stTarget.bExpressionReady;
        /* 旧协议没有属性有效位；未分析时保持零值，不伪造属性结果。 */
        if (bAttributeReady)
        {
            const FaceAttributeTarget_S &stAttribute = stTarget.stAttributeResult;
            stInfo.stExtraInfo.bMale = stAttribute.bIsMale ? TRUE : FALSE;
            stInfo.stExtraInfo.nAgeLabel = stAttribute.nAgeLabel;
            stInfo.stExtraInfo.bGlasses = stAttribute.bIsGlasses ? TRUE : FALSE;
            stInfo.stExtraInfo.bBeard = stAttribute.bIsBeard ? TRUE : FALSE;
            stInfo.stExtraInfo.bMask = stAttribute.bIsMask ? TRUE : FALSE;
            stInfo.stExtraInfo.nEmotionLabel = stAttribute.nEmotionLabel;
        }

        /* 面部情绪分析字段 */
        stInfo.stExtraInfo.nFaceExpression = bFaceExpressionReady ? stTarget.nExpressionLabel : -1;

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

void CFaceRecognitionSubDetector::processGat1400Result(
    const FrameContext_S &stContext,
    const std::vector<FaceTargetResult_S> &vstTargets)
{
#ifdef ENABLE_GAT1400_SRC
    if (vstTargets.empty())
    {
        return;
    }

    Network::Gat1400Client_S stGat1400Config;
    GAT1400::CGAT1400::instance()->getGat1400Config(stGat1400Config);
    if (!stGat1400Config.enableGat1400 || !stGat1400Config.enableFace)
    {
        return;
    }

    if (stContext.matPanoramaImage.empty())
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
        stContext.matPanoramaImage,
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
    for (const auto &stTarget : vstTargets)
    {
        security_face_t stFace;
        stFace.InfoKind = SecurityInfoType::Auto;
        const FaceDetectedTarget_S &stResult = stTarget.stDetectResult;
        const int nFrameWidth = stContext.matPanoramaImage.cols;
        const int nFrameHeight = stContext.matPanoramaImage.rows;
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
            const FaceAttributeTarget_S &stAttribute =
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
        const cv::Mat stTargetBgrImage = cropTargetImage(stContext, stTarget);
        if (!stTargetBgrImage.empty())
        {
            security_subimage_info_t stTargetImage;
            if (encodeBgrImage(
                    stTargetBgrImage,
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
    (void)vstTargets;
#endif
}


void CFaceRecognitionSubDetector::outputOsd(
    const FrameContext_S &stContext,
    const std::vector<FaceTargetResult_S> &vstTargets)
{
    std::vector<Common::RectInfo_S> vstAcceptedRect;
    vstAcceptedRect.reserve(vstTargets.size());
    for (const auto &stTarget : vstTargets)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = stTarget.stDetectResult.fX1;
        stRectInfo.nY1 = stTarget.stDetectResult.fY1;
        stRectInfo.nX2 = stTarget.stDetectResult.fX2;
        stRectInfo.nY2 = stTarget.stDetectResult.fY2;
        vstAcceptedRect.emplace_back(stRectInfo);
    }

    send_detectionResult_to_osd(
        stContext.matPanoramaImage.cols,
        stContext.matPanoramaImage.rows,
        vstAcceptedRect);
}

/**
 * @brief 在算法工作线程组装人脸识别事件，按联动选项附加全景图和同一目标的小图。
 * @param [in] stContext 当前帧信息。
 * @param [in] vstTargets 当前帧检测目标。
 * @param [in] stConfig 人脸识别运行配置快照。
 * @param [in] stSaveResult 本帧图片保存记录，事件附带路径取自全景记录。
 * @param [out] 无，图片由事件上下文的容器持有，不向异步联动传递临时图片指针。
 * @return 无，小图处理失败仅记录日志，不中断全景图及事件状态处理。
 */
void CFaceRecognitionSubDetector::processFaceRecognitionEvent(
    const FrameContext_S &stContext,
    const std::vector<FaceTargetResult_S> &vstTargets,
    const Alarm::FaceRecognition_S &stConfig,
    const FrameCaptureResult_S &stSaveResult)
{
    const FaceTargetResult_S *pBestTarget = getBestTarget(vstTargets);

    EventTriggerContext_S stEventContext;
    stEventContext.enEventType = Event::Type_E::FACE_RECOGNITION;
    stEventContext.nChnId = stContext.nChannelId;
    stEventContext.llTimestamp = stContext.llTimestamp;

    if (pBestTarget)
    {
        const FaceDetectedTarget_S &stDetectResult = pBestTarget->stDetectResult;
        stEventContext.nTargetId = -1;
        stEventContext.nObjectType = 1;
        stEventContext.fConfidence = stDetectResult.fConfidence;
        stEventContext.nLeft = static_cast<int>(stDetectResult.fX1);
        stEventContext.nTop = static_cast<int>(stDetectResult.fY1);
        stEventContext.nRight = static_cast<int>(stDetectResult.fX2);
        stEventContext.nBottom = static_cast<int>(stDetectResult.fY2);
    }
    m_stateMachine.handleAlarmState(
        !vstTargets.empty(),
        stEventContext);
}
