/**
 * @FilePath     : best_shot_selector.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 抓拍取优器（目标类型无关，多帧滞回选优）。
 *
 * 业界 best-shot 通行策略（海康专利 WO2020094091A1 / Frigate is_better_thumbnail）：
 * 跟踪期间对候选帧评分，质量显著更优（滞回阈值，防抖动）才覆盖当前最优；
 * 目标消失或超时后以最优帧出图。
 *
 * 评分来源由门面（CBestShotManager）注入的后端提供：清晰度（主项）+
 * 检测置信度 + 目标面积，ROI 平均亮度已知时再叠加亮度维度（门限否决 +
 * 亮度项 + 暗光清晰度降权），亮度未知则回退三维加权。
 */

#pragma once

#include "bestshot_types.h"

namespace Bestshot_NS
{
/* 取优候选帧（清晰度已由评分后端计算） */
struct BestShotCandidate_S
{
    BestShotRect_S stRoi;
    float fSharpness = -1.0f; /* 清晰度 [0,1]；负值表示评分失败（退化置信度主导） */
    float fDetectScore = 0.0f;
    uint64_t unArea = 0;
    long long llTimestamp = 0;
    float fBrightness = -1.0f; /* ROI 平均亮度 [0,255]，-1 表示未知 */
};

class CBestShotSelector
{
public:
    explicit CBestShotSelector(const BestShotConfig_S &stCfg);

    void reset();

    /* 喂入候选，返回 true 表示产生新 best（调用方此时编码缓存小图）。
       被亮度门限否决的候选（isGated）不参与评选，也不刷新保持窗口 */
    bool feed(const BestShotCandidate_S &stCand);

    /*
     * 亮度门限判定：过暗/过曝返回 true（不参与评选）。
     * 以下情况恒 false（放行）：亮度未知；该目标尚无候选帧（首帧保底——
     * 人可能整个窗口都逆光/过曝，中途离开也要有图可出，后续光照恢复时
     * 正常帧分数远超滞回门槛，会自然替换保底帧）。
     */
    bool isGated(const BestShotCandidate_S &stCand) const;

    /* 亮度超出 [下限, 上限] 范围的原始阈值判定（不含首帧保底放行），供日志标注 */
    bool outOfBrightnessRange(const BestShotCandidate_S &stCand) const;

    /* 无候选帧时调用：超过保持窗口（目标已离开）返回 true 并清空 best */
    bool expire(long long llNowMs);

    bool hasBest() const
    {
        return m_bHasBest;
    }
    const BestShotCandidate_S &best() const
    {
        return m_stBest;
    }

    /*
     * 综合评分：
     * - 亮度未知：清晰度 0.7 + 置信度 0.2 + 面积 0.1（清晰度失败时置信度主导）
     * - 亮度已知：0.6×(清晰度×暗光降权) + 0.2×置信度 + 0.1×面积 + 0.1×亮度项，
     *   暗光降权在亮度低于 fDimLightTh 时按亮度比例压制清晰度（夜间噪声虚高）
     */
    float score(const BestShotCandidate_S &stCand) const;

private:
    BestShotConfig_S m_stCfg;
    bool m_bHasBest = false;
    BestShotCandidate_S m_stBest;
    float m_fBestScore = 0.0f;
    long long m_llLastFeedMs = 0;
};
} // namespace Bestshot_NS
