/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 后厨无关人员进入事件子检测器实现
 */

#include "kitchen_unrelated_sub_detector.hpp"

#include <algorithm>
#include <chrono>

#include "algorithm_define.hpp" /* FACE_ATTRIBUTE_MODEL_CONFIG_PATH */
#include "dlog.h"
#include "event_configure.h"
#include "share_data.h"

namespace
{
constexpr int KITCHEN_ATTRIBUTE_WIDTH = 192;  /* 属性模型输入宽 */
constexpr int KITCHEN_ATTRIBUTE_HEIGHT = 192; /* 属性模型输入高 */
}

CKitchenUnrelatedSubDetector::~CKitchenUnrelatedSubDetector()
{
    delete m_pFaceAttributeHandle;
}

bool CKitchenUnrelatedSubDetector::initAttributeModel()
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
        dlog_error("KITCHEN_SUB: 属性模型初始化失败");
        return false;
    }

    dlog_info("KITCHEN_SUB: 属性模型初始化成功");
    return true;
}

void CKitchenUnrelatedSubDetector::analyzeAttributes(
    const DetectContext_S &stDetectContext,
    std::vector<FaceAttributeTarget_S> &vstAttributeResults,
    std::vector<bool> &vbAttributeReady)
{
    /* 模型未就绪跳过本帧，目标保持未分析由规则过滤兜底 */
    if (!initAttributeModel())
    {
        return;
    }

    const cv::Mat &stRgbImage = stDetectContext.stFrame.matPanoramaImage;
    for (size_t i = 0; i < stDetectContext.vstDetectResult.size(); ++i)
    {
        CFaceAttributeModel::InData_S stInput;
        if (cropFaceForModel(stRgbImage,
                             stDetectContext.vstDetectResult[i].stDetectResult,
                             KITCHEN_ATTRIBUTE_WIDTH, KITCHEN_ATTRIBUTE_HEIGHT,
                             stInput.inMat))
        {
            vbAttributeReady[i] =
                m_pFaceAttributeHandle->process(stInput, vstAttributeResults[i]) == 0;
        }
    }
}

void CKitchenUnrelatedSubDetector::updateConfig()
{
    Alarm::KitchenUnrelatedPersonEntry_S stConfig;
    CEventConfigure::instance()->get_configure(stConfig);

    m_processor.setRules(stConfig);

    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        m_bEnable = stConfig.bEnable;
    }
}

int CKitchenUnrelatedSubDetector::process(const DetectContext_S &stDetectContext)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        if (!m_bEnable)
        {
            return 0;
        }
    }

    /* 口罩判定依赖属性结果，先做本帧属性分析（结果对齐基础目标） */
    std::vector<FaceAttributeTarget_S> vstAttributeResults(
        stDetectContext.vstDetectResult.size());
    std::vector<bool> vbAttributeReady(
        stDetectContext.vstDetectResult.size(), false);
    analyzeAttributes(stDetectContext, vstAttributeResults, vbAttributeReady);

    std::vector<int> vstCandidateIndices;
    m_processor.process(stDetectContext.vstDetectResult, vstAttributeResults,
                        vbAttributeReady, vstCandidateIndices);

#ifdef ENABLE_TVSDK_SRC
    /* 组装触发上下文，状态机驱动事件开始/结束上报 */
    const bool bAlarm = !vstCandidateIndices.empty();
    const cv::Mat &rgbMat = stDetectContext.stFrame.matPanoramaImage;
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::KITCHEN_UNRELATED_PERSON_ENTRY;
    stContext.nChnId = stDetectContext.stFrame.nChannelId;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    /* 首个候选目标携带人脸框与大小图 无跟踪ID 人脸目标类型为人 */
    if (bAlarm && !rgbMat.empty())
    {
        const FaceDetectedTarget_S &stHitTarget =
            stDetectContext.vstDetectResult[vstCandidateIndices.front()].stDetectResult;
        stContext.nObjectType = 1; /* 人 */
        stContext.fConfidence = stHitTarget.fConfidence;
        stContext.nLeft   = static_cast<int>(stHitTarget.fX1);
        stContext.nTop    = static_cast<int>(stHitTarget.fY1);
        stContext.nRight  = static_cast<int>(stHitTarget.fX2);
        stContext.nBottom = static_cast<int>(stHitTarget.fY2);

        /* 小图按人脸框裁剪 裁剪异常仅缺小图不影响上报 */
        encode_mat_to_tvsdk_image(rgbMat, stContext.stPanoramaImage, JPEG_QUALITY_PANORAMA);

        const int nX1 = std::max(0, stContext.nLeft);
        const int nY1 = std::max(0, stContext.nTop);
        const int nX2 = std::min(rgbMat.cols, stContext.nRight);
        const int nY2 = std::min(rgbMat.rows, stContext.nBottom);
        if (nX2 > nX1 && nY2 > nY1)
        {
            cv::Mat targetMat = rgbMat(cv::Rect(nX1, nY1, nX2 - nX1, nY2 - nY1)).clone();
            encode_mat_to_tvsdk_image(targetMat, stContext.stTargetImage, JPEG_QUALITY_TARGET);
        }
    }

    m_stateMachine.handleAlarmState(bAlarm, stContext);
#else
    m_stateMachine.handleAlarmState(!vstCandidateIndices.empty(),
                                    Event::Type_E::KITCHEN_UNRELATED_PERSON_ENTRY);
#endif
    return 0;
}

bool CKitchenUnrelatedSubDetector::isEventEnabled() const
{
    std::lock_guard<std::mutex> lock(m_mutexConfig);
    return m_bEnable;
}

void CKitchenUnrelatedSubDetector::reset()
{
    std::lock_guard<std::mutex> lock(m_mutexConfig);
    m_bEnable = false;
}
