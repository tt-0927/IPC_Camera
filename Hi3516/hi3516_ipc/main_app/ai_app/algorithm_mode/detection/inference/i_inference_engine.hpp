/**
 * @FilePath     : i_inference_engine.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 16:30:00
 * @Description  : 推理引擎统一抽象接口（YOLO/HVF 等模型引擎接入统一检测层的契约）
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "detection_types.hpp"
#include "stream_process_ext.hpp"

/* 统一检测目标框（模型输入坐标，整数像素） */
struct DetectionBox_S
{
    int nClassId = 0;         /* 类别 ID（与模型 label_name 顺序一致） */
    float fConfidence = 0.0F; /* 置信度 [0,1] */
    int nX1 = 0, nY1 = 0;     /* 左上角 */
    int nX2 = 0, nY2 = 0;     /* 右下角 */
    /* 可选 Track ID：HVF 引擎填充；YOLO 引擎不填（由模型适配器用跟踪器补充） */
    std::optional<uint64_t> optTrackId;
    /* 可选 Track 生命周期：HVF 引擎填充（STARTED/TRACKED/ENDED）；YOLO 引擎不填 */
    AiPipeline_NS::TrackState_E enTrackState = AiPipeline_NS::TrackState_E::UNAVAILABLE;
};

/**
 * @brief   : 推理引擎统一接口
 * @note    : 模型适配器通过本接口调用具体推理引擎（CYoloUltralytics / HiAiDetect_S），
 *            不再直接 new 具体引擎类；MD/OD（svp_md/svp_od）输出非目标框，不实现
 *            本接口，保持现有模型适配器内直接调用 SDK 的形态。
 *            引擎句柄生命周期仅由检测引擎 Worker 线程访问（init/unInit/infer 同线程串行），
 *            接口位于 hi3516_ipc（接触 ot_video_frame_info 与 TrackState_E），不进入共享仓库。
 */
class IInferenceEngine
{
public:
    virtual ~IInferenceEngine() = default;

    /**
     * @brief   : 初始化：读取模型配置文件，加载模型
     * @param    {const std::string} &strConfigPath：模型配置文件路径（HVF 引擎忽略）
     * @return   {int} OK：成功，非 OK：失败（调用方按 1s 周期重试）
     */
    virtual int init(const std::string &strConfigPath) = 0;

    /**
     * @brief   : 反初始化：释放引擎句柄与模型资源
     * @return   {void}
     */
    virtual void unInit() = 0;

    /**
     * @brief   : 单帧推理
     * @param    {const ot_video_frame_info} *pPreparedFrame：已准备帧（模型输入分辨率）
     * @param    {std::vector<DetectionBox_S>} &vOutBoxes：输出统一目标框
     * @return   {int} OK：成功，非 OK：失败（调用方跳过本帧）
     * @note    : 输出框为模型输入坐标（MODEL_INPUT 语义），左上角/右下角整数像素
     */
    virtual int infer(const ot_video_frame_info *pPreparedFrame,
                      std::vector<DetectionBox_S> &vOutBoxes) = 0;

    /**
     * @brief   : 获取模型输入分辨率（init 成功后有效）
     * @return   {AiPipeline_NS::FrameSize_S} 模型输入分辨率
     */
    virtual AiPipeline_NS::FrameSize_S model_input_size() const = 0;
};
