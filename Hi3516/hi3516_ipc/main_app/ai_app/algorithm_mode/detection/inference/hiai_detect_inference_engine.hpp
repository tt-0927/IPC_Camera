/**
 * @FilePath     : hiai_detect_inference_engine.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 11:30:00
 * @Description  : 海思 AI Detect 推理引擎声明（包装 HiAiDetect_S，预留）
 */

#pragma once

#if CAP_AI_EXHIBITION_PEOPLE_FLOW

#include <string>

#include "i_inference_engine.hpp"

extern "C"
{
#include "svp_ai_detect.h"
}

/**
 * @brief   : 海思 AI Detect 推理引擎
 * @note    : 包装 HiAiDetect_S 句柄，与模型无关——同一引擎可加载不同模型，
 *            通道与模型路径在构造时指定（海思模型无 JSON 配置，与
 *            streamAiDetect_init(nChn, pModelPath) 语义一致）：
 *            - AI_DETECT_CHN_HVF：海思预置 HVF 模型（行人/车）
 *            - AI_DETECT_CHN_PET：宠物模型（CPetRecognition 同引擎）
 *            迁移宠物识别/人脸等 HiAiDetect 类模型时按对应通道/路径构造即可，
 *            引擎代码无需修改。infer() 将 ot_aidetect_result_array 转为统一
 *            DetectionBox_S 并填充 Track ID 与生命周期（NEW->STARTED、
 *            UPDATE->TRACKED、DIE->ENDED、VALID->UNAVAILABLE，
 *            映射规则与 hvf_result_converter.cpp 一致）。
 *            预留组件：HiAiDetect 类模型迁移到统一检测层时注册使用
 *            （当前展馆用 YOLO 引擎），本文件在 CAP_AI_EXHIBITION_PEOPLE_FLOW 宏下编译。
 *            句柄仅由检测引擎 Worker 线程访问（init/unInit/infer 同线程串行）。
 */
class CHiAiDetectInferenceEngine : public IInferenceEngine
{
public:
    /**
     * @brief   : 构造海思 AI Detect 推理引擎
     * @param    {int} nChn：AI_DETECT_CHN_* 通道号（决定加载哪个模型）
     * @param    {const std::string} &strModelPath：模型文件路径
     */
    CHiAiDetectInferenceEngine(int nChn, const std::string &strModelPath);
    ~CHiAiDetectInferenceEngine() override;

    /* 禁止拷贝 */
    CHiAiDetectInferenceEngine(const CHiAiDetectInferenceEngine &) = delete;
    CHiAiDetectInferenceEngine &operator=(const CHiAiDetectInferenceEngine &) = delete;

    int init(const std::string &strConfigPath) override;
    void unInit() override;
    int infer(const ot_video_frame_info *pPreparedFrame,
              std::vector<DetectionBox_S> &vOutBoxes) override;
    AiPipeline_NS::FrameSize_S model_input_size() const override;

private:
    /* AI_DETECT_CHN_* 通道号（构造时指定，仅 Worker 线程访问） */
    int m_nChn = 0;
    /* 模型文件路径（构造时指定，仅 Worker 线程访问） */
    std::string m_strModelPath;
    /* HiAiDetect 句柄（仅引擎 Worker 线程访问） */
    HiAiDetect_S *m_pHiAiDetectHandle = nullptr;
    /* 模型输入分辨率（init 后由 ss_mpi_aidetect_get_model_info 读取） */
    AiPipeline_NS::FrameSize_S m_stModelInputSize;
    /* 累计丢弃非法目标数（诊断用，仅 Worker 线程访问） */
    uint64_t m_ullDroppedObjectCount = 0;
    /* 上次诊断日志对应的丢弃数（丢弃数变化时才输出，dlog 全局限流兜底） */
    uint64_t m_ullLastWarnDropCount = 0;
};

#endif // CAP_AI_EXHIBITION_PEOPLE_FLOW
