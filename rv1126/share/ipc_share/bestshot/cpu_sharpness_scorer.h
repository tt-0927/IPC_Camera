/**
 * @FilePath     : cpu_sharpness_scorer.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : CPU 软件清晰度评分后端。
 *
 * 算法为 Tenengrad 归一化变体（与 IVE 硬件后端同语义，可互换）：
 * Sobel 3x3 求梯度 -> 幅值超过阈值记为显著梯度 -> 显著像素占比即评分。
 * 运动模糊帧梯度幅值整体下降，被阈值截断后占比骤降，可有效区分清晰帧与模糊帧。
 * 纯软件实现无平台依赖，评分窗与采样步进可配以控算力。
 */

#pragma once

#include "sharpness_scorer.h"

namespace Bestshot_NS
{
class CCpuSharpnessScorer : public ISharpnessScorer
{
public:
    struct Config_S
    {
        uint16_t nGradThreshold = 30; /* 显著梯度阈值（Sobel 幅值域），噪声大场景调高 */
        uint32_t unMaxRoiSide = 512;  /* 评分窗口边长上限（像素），超限中心截取控算力 */
        uint32_t unSampleStep = 1;    /* 行列采样步进，2 = 隔行隔列（算力约减半） */
    };

    CCpuSharpnessScorer();
    explicit CCpuSharpnessScorer(const Config_S &stCfg);

    float eval(const BestShotFrameView_S &stFrame, const BestShotRect_S &stRoi) override;

private:
    Config_S m_stCfg;
};
} // namespace Bestshot_NS
