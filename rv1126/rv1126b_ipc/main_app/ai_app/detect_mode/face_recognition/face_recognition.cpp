/*
 * @Author: zhengxh
 * @Date: 2026-09-24 08:45:07
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-11-02
 * @Description: 人脸识别主检测器（取流 + 人脸检测/质量评分基础管线）
 */

#include "face_recognition.hpp"

#include "action_code.h"
#include "algorithm_define.hpp" /* 模型配置路径宏 */
#include "convert_interface.h"
#include "dlog.h"
#include "face_recognition_sub_detector.hpp"
#include "kitchen_unrelated_sub_detector.hpp"
#include "social_relationship_sub_detector.hpp"
#include "stream_video.h"

#include <utility>

namespace
{
constexpr float FACE_DETECT_MODEL_THRESHOLD = 0.1F; /* 检测模型输入阈值，放宽到灵敏度映射下限 */
constexpr float FACE_QUALITY_THRESHOLD = 0.05F;
constexpr int FACE_QUALITY_WIDTH = 112;
constexpr int FACE_QUALITY_HEIGHT = 112;
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
    const bool bFaceArmed = stAlgoConfig.nEnFaceRecognition != 0;
    const bool bSocialArmed = stAlgoConfig.nSocialRelationshipAnalysis != 0;
    const bool bKitchenArmed = stAlgoConfig.nKitchenUnrelatedPersonEntry != 0;
    m_bFaceArmed.store(bFaceArmed);
    m_bSocialArmed.store(bSocialArmed);
    m_bKitchenArmed.store(bKitchenArmed);

    if (!bFaceArmed && !bSocialArmed && !bKitchenArmed)
    {
        m_bEnabled.store(false);
        /* 撤防复位子检测器跨会话状态（人脸子含属性推送去重基线） */
        for (auto &pSubDetector : m_arrSubDetectors)
        {
            if (pSubDetector)
            {
                pSubDetector->reset();
            }
        }
        dlog_info("ai_app: 人脸识别/社交关系分析/后厨无关人员进入退出布防时间");
        return;
    }

    /* 布防即创建子检测器并通知其自拉配置（规则下发不下沉到本层） */
    bool bFaceEnabled = false;
    if (bFaceArmed)
    {
        if (!m_arrSubDetectors[SUB_DETECTOR_FACE])
        {
            m_arrSubDetectors[SUB_DETECTOR_FACE] =
                std::make_unique<CFaceRecognitionSubDetector>();
        }
        m_arrSubDetectors[SUB_DETECTOR_FACE]->updateConfig();
        bFaceEnabled = m_arrSubDetectors[SUB_DETECTOR_FACE]->isEventEnabled();
    }
    else if (m_arrSubDetectors[SUB_DETECTOR_FACE])
    {
        m_arrSubDetectors[SUB_DETECTOR_FACE]->reset();
    }

    if (bSocialArmed)
    {
        if (!m_arrSubDetectors[SUB_DETECTOR_SOCIAL])
        {
            m_arrSubDetectors[SUB_DETECTOR_SOCIAL] =
                std::make_unique<CSocialRelationshipSubDetector>();
        }
        m_arrSubDetectors[SUB_DETECTOR_SOCIAL]->updateConfig();
    }
    else if (m_arrSubDetectors[SUB_DETECTOR_SOCIAL])
    {
        m_arrSubDetectors[SUB_DETECTOR_SOCIAL]->reset();
    }

    if (bKitchenArmed)
    {
        if (!m_arrSubDetectors[SUB_DETECTOR_KITCHEN])
        {
            m_arrSubDetectors[SUB_DETECTOR_KITCHEN] =
                std::make_unique<CKitchenUnrelatedSubDetector>();
        }
        m_arrSubDetectors[SUB_DETECTOR_KITCHEN]->updateConfig();
    }
    else if (m_arrSubDetectors[SUB_DETECTOR_KITCHEN])
    {
        m_arrSubDetectors[SUB_DETECTOR_KITCHEN]->reset();
    }

    const bool bSocialEnabled =
        bSocialArmed && m_arrSubDetectors[SUB_DETECTOR_SOCIAL] &&
        m_arrSubDetectors[SUB_DETECTOR_SOCIAL]->isEventEnabled();
    const bool bKitchenEnabled =
        bKitchenArmed && m_arrSubDetectors[SUB_DETECTOR_KITCHEN] &&
        m_arrSubDetectors[SUB_DETECTOR_KITCHEN]->isEventEnabled();

    m_bEnabled.store(bFaceEnabled || bSocialEnabled || bKitchenEnabled);
    dlog_info("ai_app: 更新人脸识别/社交关系分析/后厨无关人员进入配置, 人脸运行[%d] 社交运行[%d] 后厨运行[%d]",
              bFaceEnabled,
              bSocialEnabled,
              bKitchenEnabled);
}

bool CFaceRecognition::init()
{
    if (!m_pFaceDetectHandle)
    {
        m_pFaceDetectHandle = new CFaceDetectModel(FACE_DETECT_MODEL_CONFIG_PATH);
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
        m_pFaceQualityHandle = new CFaceQualityModel(FACE_QUALITY_MODEL_CONFIG_PATH);
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
}

bool CFaceRecognition::evaluateFaceQuality(const cv::Mat &stRgbImage,
                                            const FaceDetectedTarget_S &stResult,
                                            float &fQualityScore)
{
    if (!m_pFaceQualityHandle || stRgbImage.empty())
    {
        return false;
    }

    CFaceQualityModel::InData_S stQualityInput;
    if (!cropFaceForModel(
            stRgbImage,
            stResult,
            FACE_QUALITY_WIDTH,
            FACE_QUALITY_HEIGHT,
            stQualityInput.inMat))
    {
        return false;
    }

    return m_pFaceQualityHandle->process(stQualityInput, fQualityScore) == 0;
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
    stContext.matPanoramaImage = m_stPanoramaRgbImage;
    return !stContext.matPanoramaImage.empty();
}

void CFaceRecognition::detectFaces(DetectContext_S &stDetectContext)
{
    std::vector<FaceBaseTarget_S> &vstTargetResults = stDetectContext.vstDetectResult;
    vstTargetResults.clear();

    std::vector<FaceDetectedTarget_S> vstDetectedTargets;
    CFaceDetectModel::InData_S stDetectInput;
    stDetectInput.stParam.fBoxThreshold = FACE_DETECT_MODEL_THRESHOLD;
    stDetectInput.inMat = stDetectContext.stFrame.matPanoramaImage;
    if (m_pFaceDetectHandle->process(stDetectInput, vstDetectedTargets) != 0)
    {
        dlog_error("ai_app: 人脸识别检测失败");
        return;
    }

    vstTargetResults.reserve(vstDetectedTargets.size());
    for (auto &stDetected : vstDetectedTargets)
    {
        /* 检测默认绑定质量评估，评估失败保持0分 */
        float fQualityScore = 0.0F;
        evaluateFaceQuality(stDetectContext.stFrame.matPanoramaImage, stDetected, fQualityScore);
        vstTargetResults.emplace_back(std::move(stDetected));
        vstTargetResults.back().fQualityScore = fQualityScore;
    }
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

        DetectContext_S stDetectContext;
        if (!prepareFrameContext(stMediaData, stDetectContext.stFrame))
        {
            continue;
        }

        /* 基础管线：人脸检测 + 质量评分 */
        detectFaces(stDetectContext);

        /* 子检测器串行驱动（未布防槽位跳过，使能判定在子检测器内部） */
        if (m_bFaceArmed.load() && m_arrSubDetectors[SUB_DETECTOR_FACE])
        {
            m_arrSubDetectors[SUB_DETECTOR_FACE]->process(stDetectContext);
        }
        if (m_bSocialArmed.load() && m_arrSubDetectors[SUB_DETECTOR_SOCIAL])
        {
            m_arrSubDetectors[SUB_DETECTOR_SOCIAL]->process(stDetectContext);
        }
        if (m_bKitchenArmed.load() && m_arrSubDetectors[SUB_DETECTOR_KITCHEN])
        {
            m_arrSubDetectors[SUB_DETECTOR_KITCHEN]->process(stDetectContext);
        }
    }
}
