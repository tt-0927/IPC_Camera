/**
 * @FilePath     : ive_sharpness_adapter.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : IVE 硬件清晰度评分适配器（可选后端）。
 *
 * 将平台层 svpIveSharp C 封装包装为通用评分接口 ISharpnessScorer。
 * 注意：板端实测 IVE 与 NPU 并存存在硬件级冲突（推理输入 shape 被破坏、
 * VB 帧池耗尽），冲突定位前默认不装配本后端，仅保留切换通路。
 */

#pragma once

#include "sharpness_scorer.h"
#include "svp_ive_sharpness.h"

namespace FaceDetectInternal
{
class CIveSharpnessAdapter : public Bestshot_NS::ISharpnessScorer
{
public:
    /* 构造内分配 IVE 句柄（失败不抛出，后续 eval 返回 -1 退化置信度主导） */
    explicit CIveSharpnessAdapter(HiIveSharpParam_S stParam);
    ~CIveSharpnessAdapter() override;

    float eval(const Bestshot_NS::BestShotFrameView_S &stFrame, const Bestshot_NS::BestShotRect_S &stRoi) override;

private:
    HiIveSharp_S *m_pHandle = nullptr;
};
} // namespace FaceDetectInternal
