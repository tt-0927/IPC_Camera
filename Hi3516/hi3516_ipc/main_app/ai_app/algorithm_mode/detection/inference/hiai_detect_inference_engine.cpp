/**
 * @FilePath     : hiai_detect_inference_engine.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 11:30:00
 * @Description  : 海思 AI Detect 推理引擎实现（包装 HiAiDetect_S，预留）
 */

#include "hiai_detect_inference_engine.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "IpcRet.h"
#include "dlog.h"
#include "share_data.h"
#include "stream_ai_detect.h"

#if CAP_AI_EXHIBITION_PEOPLE_FLOW

namespace
{
/**
 * @brief   : 映射海思跟踪状态到统一跟踪状态
 * @param    {ot_aidetect_track_status} enStatus：海思跟踪状态
 * @return   {AiPipeline_NS::TrackState_E} 统一跟踪状态
 * @note    : 与 hvf_result_converter.cpp map_track_status 保持一致：
 *            NEW->STARTED、UPDATE->TRACKED、DIE->ENDED（保留 Track ID 供状态清理）、
 *            VALID->UNAVAILABLE
 */
AiPipeline_NS::TrackState_E map_track_status(ot_aidetect_track_status enStatus)
{
    switch (enStatus)
    {
    case OT_AIDETECT_TRACK_STATUS_NEW:
        return AiPipeline_NS::TrackState_E::STARTED;
    case OT_AIDETECT_TRACK_STATUS_UPDATE:
        return AiPipeline_NS::TrackState_E::TRACKED;
    case OT_AIDETECT_TRACK_STATUS_DIE:
        return AiPipeline_NS::TrackState_E::ENDED;
    case OT_AIDETECT_TRACK_STATUS_VALID:
    default:
        return AiPipeline_NS::TrackState_E::UNAVAILABLE;
    }
}
} // namespace

CHiAiDetectInferenceEngine::CHiAiDetectInferenceEngine(int nChn, const std::string &strModelPath)
    : m_nChn(nChn), m_strModelPath(strModelPath)
{
}

CHiAiDetectInferenceEngine::~CHiAiDetectInferenceEngine()
{
    unInit();
}

int CHiAiDetectInferenceEngine::init(const std::string &strConfigPath)
{
    /* 仅引擎 Worker 线程调用；HiAiDetect 模型由构造参数（通道+模型路径）指定，
     * 海思模型无 JSON 配置，忽略接口的配置文件参数（与 streamAiDetect_init 语义一致） */
    (void)strConfigPath;
    if (m_pHiAiDetectHandle != nullptr)
    {
        return OK;
    }

    HiAiDetectNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(stNeedParam));
    stNeedParam.nChn = m_nChn;
    stNeedParam.enModelLoadMode = OT_AIDETECT_MODEL_LOAD_FROM_PATH;
    const int nPathLen = snprintf(stNeedParam.aModelPath, sizeof(stNeedParam.aModelPath), "%s", m_strModelPath.c_str());
    if (nPathLen < 0 || static_cast<size_t>(nPathLen) >= sizeof(stNeedParam.aModelPath))
    {
        dlog_error("HiAiDetect 引擎模型路径过长或格式化失败 [%s]", m_strModelPath.c_str());
        return ERR_PARAM;
    }

    m_pHiAiDetectHandle = svpAiDetect_alloc(stNeedParam);
    if (m_pHiAiDetectHandle == nullptr)
    {
        dlog_error("HiAiDetect 引擎初始化失败-分配句柄失败");
        return ERR;
    }
    if (m_pHiAiDetectHandle->svpAiDetect_init(m_pHiAiDetectHandle) != TD_SUCCESS)
    {
        svpAiDetect_release(m_pHiAiDetectHandle);
        m_pHiAiDetectHandle = nullptr;
        dlog_error("HiAiDetect 引擎初始化失败 [%s]", m_strModelPath.c_str());
        return ERR;
    }

    /* 读取模型输入分辨率（hvf_detect.cpp init() 同款读取，hvf_detect.cpp:271），
     * 失败时释放句柄返回 ERR，由引擎按 1s 周期重试 */
    ot_aidetect_model_info stModelInfo;
    memset(&stModelInfo, 0, sizeof(stModelInfo));
    if (TD_SUCCESS != ss_mpi_aidetect_get_model_info(m_nChn, &stModelInfo))
    {
        unInit();
        dlog_error("HiAiDetect 引擎读取模型信息失败");
        return ERR;
    }
    m_stModelInputSize.nWidth = stModelInfo.size.width;
    m_stModelInputSize.nHeight = stModelInfo.size.height;
    dlog_info("HiAiDetect 引擎初始化成功，通道[%d] 模型输入分辨率 [%ux%u]",
              m_nChn, m_stModelInputSize.nWidth, m_stModelInputSize.nHeight);
    return OK;
}

void CHiAiDetectInferenceEngine::unInit()
{
    /* 仅引擎 Worker 线程调用 */
    if (m_pHiAiDetectHandle != nullptr)
    {
        /* memory: svpAiDetect_uninit 释放结果数组并销毁检测通道，
         * svpAiDetect_release 释放句柄内存（与 streamAiDetect_uninit 顺序一致） */
        const int nUninitRet = m_pHiAiDetectHandle->svpAiDetect_uninit(m_pHiAiDetectHandle);
        if (nUninitRet != TD_SUCCESS)
        {
            dlog_warn("HiAiDetect 引擎反初始化失败 [%d]", nUninitRet);
        }
        svpAiDetect_release(m_pHiAiDetectHandle);
        m_pHiAiDetectHandle = nullptr;
    }
    m_stModelInputSize = AiPipeline_NS::FrameSize_S();
}

int CHiAiDetectInferenceEngine::infer(const ot_video_frame_info *pPreparedFrame,
                                      std::vector<DetectionBox_S> &vOutBoxes)
{
    /* 仅引擎 Worker 线程调用 */
    if (pPreparedFrame == nullptr || m_pHiAiDetectHandle == nullptr)
    {
        return ERR_PARAM;
    }

    /* 送帧推理（与 hvf_detect.cpp run() 一致，hvf_detect.cpp:405），结果写入 stResult */
    ot_video_frame *pVideoFrame = const_cast<ot_video_frame *>(&pPreparedFrame->video_frame);
    if (m_pHiAiDetectHandle->svpAiDetect_sendFrame(m_pHiAiDetectHandle, pVideoFrame) != TD_SUCCESS)
    {
        dlog_warn("HiAiDetect 引擎送帧推理失败，本帧跳过");
        return ERR;
    }

    /* 结果数组结构损坏时整批失败（与 hvf_result_converter.cpp:105 校验一致） */
    const ot_aidetect_result_array &stResult = m_pHiAiDetectHandle->stResult;
    if (stResult.class_num > OT_AIDETECT_CLASS_BUTT)
    {
        dlog_warn("HiAiDetect 引擎结果类别数量非法 [%u]", stResult.class_num);
        return ERR_PARAM;
    }

    vOutBoxes.clear();
    for (uint32_t i = 0; i < stResult.class_num; ++i)
    {
        const ot_aidetect_object_of_one_class &stOneClass = stResult.object_class[i];
        if (stOneClass.objects == nullptr || stOneClass.object_num == 0U)
        {
            continue;
        }

        for (uint32_t j = 0; j < stOneClass.object_num; ++j)
        {
            const ot_aidetect_object &stObject = stOneClass.objects[j];

            DetectionBox_S stBox;
            /* HiAiDetect 无 label_name，类别 ID 直接使用海思类别枚举值（模型适配器再做语义映射） */
            stBox.nClassId = static_cast<int>(stOneClass.class_type);
            stBox.fConfidence = stObject.detect_confidence;
            stBox.optTrackId = static_cast<uint64_t>(stObject.track_id);
            stBox.enTrackState = map_track_status(stObject.track_status);

            /* DIE 目标坐标退化为零矩形，不参与几何计算，但保留 Track ID 供状态清理
             * （与 hvf_result_converter.cpp:147 语义一致） */
            if (stBox.enTrackState == AiPipeline_NS::TrackState_E::ENDED)
            {
                vOutBoxes.emplace_back(stBox);
                continue;
            }

            /* detect_rect 为左上角 (x,y) + 宽高（MODEL_INPUT 坐标），换算右下角 */
            const int nX1 = static_cast<int>(stObject.detect_rect.x);
            const int nY1 = static_cast<int>(stObject.detect_rect.y);
            const int nWidth = static_cast<int>(stObject.detect_rect.width);
            const int nHeight = static_cast<int>(stObject.detect_rect.height);
            if (nWidth <= 0 || nHeight <= 0)
            {
                ++m_ullDroppedObjectCount;
                continue;
            }
            if (!std::isfinite(stObject.detect_confidence) ||
                stObject.detect_confidence < 0.0F || stObject.detect_confidence > 1.0F)
            {
                ++m_ullDroppedObjectCount;
                continue;
            }

            stBox.nX1 = nX1;
            stBox.nY1 = nY1;
            stBox.nX2 = nX1 + nWidth;
            stBox.nY2 = nY1 + nHeight;
            vOutBoxes.emplace_back(stBox);
        }
    }

    /* 丢弃诊断：仅在本帧发生丢弃时输出（帧级热路径依赖 dlog 全局限流按调用点合并，
     * 不自行维护限频时间戳） */
    if (m_ullDroppedObjectCount != m_ullLastWarnDropCount)
    {
        m_ullLastWarnDropCount = m_ullDroppedObjectCount;
        dlog_warn("HiAiDetect 引擎丢弃非法目标，累计[%llu]",
                  static_cast<unsigned long long>(m_ullDroppedObjectCount));
    }
    return OK;
}

AiPipeline_NS::FrameSize_S CHiAiDetectInferenceEngine::model_input_size() const
{
    return m_stModelInputSize;
}

#endif // CAP_AI_EXHIBITION_PEOPLE_FLOW
