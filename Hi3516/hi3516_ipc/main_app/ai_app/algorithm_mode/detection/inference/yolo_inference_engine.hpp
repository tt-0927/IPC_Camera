/**
 * @FilePath     : yolo_inference_engine.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 16:30:00
 * @Description  : YOLO 推理引擎声明（包装 CYoloUltralytics_rpn）
 */

#pragma once

#include "YoloUltralytics_rpn.hpp"
#include "i_inference_engine.hpp"

/**
 * @brief   : YOLO 推理引擎
 * @note    : 内部持有 Inference_NS::CYoloUltralytics（Hisilicon 版 YOLOV8 后处理），
 *            infer() 将 BoxData_S（左上角/右下角）转为统一 DetectionBox_S；
 *            YOLO 引擎不提供 Track ID/生命周期，由模型适配器用跟踪器补充。
 *            句柄仅由检测引擎 Worker 线程访问（init/unInit/infer 同线程串行）。
 */
class CYoloInferenceEngine : public IInferenceEngine
{
public:
    CYoloInferenceEngine() = default;
    ~CYoloInferenceEngine() override;

    /* 禁止拷贝 */
    CYoloInferenceEngine(const CYoloInferenceEngine &) = delete;
    CYoloInferenceEngine &operator=(const CYoloInferenceEngine &) = delete;

    int init(const std::string &strConfigPath) override;
    void unInit() override;
    int infer(const ot_video_frame_info *pPreparedFrame,
              std::vector<DetectionBox_S> &vOutBoxes) override;
    AiPipeline_NS::FrameSize_S model_input_size() const override;

private:
    /**
     * @brief   : 从配置 json 解析模型输入分辨率（pre_process.size，[0]=宽 [1]=高）
     * @param    {const std::string} &strConfigPath：模型配置文件路径
     * @param    {AiPipeline_NS::FrameSize_S} &stOut：输出分辨率
     * @return   {bool} true：解析成功且宽高为正
     * @note    : 失败时调用方回退 1024×576（展馆模型固定分辨率）并告警
     */
    bool parse_model_input_size(const std::string &strConfigPath,
                                AiPipeline_NS::FrameSize_S &stOut) const;

    /* YOLO 推理句柄（仅引擎 Worker 线程访问） */
    Inference_NS::CYoloUltralytics *m_pYoloHandle = nullptr;
    /* 模型输入分辨率（init 后有效） */
    AiPipeline_NS::FrameSize_S m_stModelInputSize;
    /* 累计丢弃非法框数（诊断用，仅 Worker 线程访问） */
    uint64_t m_ullDroppedObjectCount = 0;
    /* 上次诊断日志对应的丢弃数（丢弃数变化时才输出，dlog 全局限流兜底） */
    uint64_t m_ullLastWarnDropCount = 0;
};
