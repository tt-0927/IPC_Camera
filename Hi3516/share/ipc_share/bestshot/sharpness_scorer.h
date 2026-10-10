/**
 * @FilePath     : sharpness_scorer.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 清晰度评分后端接口（可插拔：CPU 软件 / 平台硬件实现）。
 *
 * 想切换评分实现时由业务装配点注入不同实现即可，选优逻辑与事件链路不感知后端。
 */

#pragma once

#include "bestshot_types.h"

namespace Bestshot_NS
{
/*
 * 清晰度评分后端接口。
 * 返回 [0.0, 1.0] 的清晰度评分；负值表示评分失败（帧视图无效 / ROI 过小 /
 * 后端资源异常），此时该帧综合分退化为置信度主导，选优链路不中断。
 */
class ISharpnessScorer
{
public:
    virtual ~ISharpnessScorer() = default;

    /* 评分在 Y 平面 ROI 上执行，实现内部自行处理对齐与越界约束 */
    virtual float eval(const BestShotFrameView_S &stFrame, const BestShotRect_S &stRoi) = 0;
};
} // namespace Bestshot_NS
