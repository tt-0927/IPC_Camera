/**
 * @FilePath     : best_shot_selector.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 抓拍取优器实现。
 */

#include "best_shot_selector.h"

#include <algorithm>
#include <cmath>

namespace Bestshot_NS
{
namespace
{
/* 面积归一化上限（像素数超过该值面积项满分） */
constexpr float BEST_SHOT_AREA_FULL = 300.0f * 300.0f;
/* 亮度项的满分基准（灰度均值落在 127 附近亮度项满分，过暗/过曝趋向 0 分） */
constexpr float BRIGHTNESS_CENTER = 127.0f;
} // namespace

CBestShotSelector::CBestShotSelector(const BestShotConfig_S &stCfg) : m_stCfg(stCfg)
{
    /* 配置保护：滞回与保持窗口非法时回落默认值 */
    if (m_stCfg.fHysteresis <= 0.0f)
    {
        m_stCfg.fHysteresis = 0.1f;
    }
    if (m_stCfg.llHoldWindowMs <= 0)
    {
        m_stCfg.llHoldWindowMs = 5000;
    }
}

void CBestShotSelector::reset()
{
    m_bHasBest = false;
    m_fBestScore = 0.0f;
    m_llLastFeedMs = 0;
}

bool CBestShotSelector::isGated(const BestShotCandidate_S &stCand) const
{
    /* 亮度未知（查询失败或平台未接入亮度统计）不设门限，保证链路可退化出图 */
    if (stCand.fBrightness < 0.0f)
    {
        return false;
    }
    /* 首帧保底：该目标还没有任何候选帧时放行——人可能整个窗口都逆光/过曝，
       中途离开也要有图可出；后续光照恢复后正常帧分数远超滞回门槛，自然替换 */
    if (!m_bHasBest)
    {
        return false;
    }
    return outOfBrightnessRange(stCand);
}

bool CBestShotSelector::outOfBrightnessRange(const BestShotCandidate_S &stCand) const
{
    if (stCand.fBrightness < 0.0f)
    {
        return false;
    }
    return stCand.fBrightness < m_stCfg.fBrightnessMin || stCand.fBrightness > m_stCfg.fBrightnessMax;
}

float CBestShotSelector::score(const BestShotCandidate_S &stCand) const
{
    /* 清晰度失败（负值）时该项置 0，由置信度主导，保证链路可退化出图 */
    const float fSharp = std::max(0.0f, stCand.fSharpness);
    const float fArea = std::min(1.0f, static_cast<float>(stCand.unArea) / BEST_SHOT_AREA_FULL);
    const float fConf = std::min(1.0f, std::max(0.0f, stCand.fDetectScore));

    if (stCand.fBrightness < 0.0f)
    {
        return 0.7f * fSharp + 0.2f * fConf + 0.1f * fArea;
    }

    /* 暗光降权：夜间高增益噪声让梯度占比虚高，亮度低于阈值时清晰度按比例打折 */
    float fDimScale = 1.0f;
    if (m_stCfg.fDimLightTh > 0.0f)
    {
        fDimScale = std::min(1.0f, stCand.fBrightness / m_stCfg.fDimLightTh);
    }
    /* 亮度项：均值越接近中间灰度分越高，过暗/过曝趋向 0 分 */
    const float fBright = std::max(0.0f, 1.0f - std::fabs(stCand.fBrightness - BRIGHTNESS_CENTER) / BRIGHTNESS_CENTER);
    return 0.6f * (fSharp * fDimScale) + 0.2f * fConf + 0.1f * fArea + 0.1f * fBright;
}

bool CBestShotSelector::feed(const BestShotCandidate_S &stCand)
{
    /* 亮度门限否决的帧不参与评选，也不刷新保持窗口（有效候选才算目标还在） */
    if (isGated(stCand))
    {
        return false;
    }
    m_llLastFeedMs = stCand.llTimestamp;
    const float fScore = score(stCand);
    if (m_bHasBest && fScore <= m_fBestScore * (1.0f + m_stCfg.fHysteresis))
    {
        return false;
    }
    m_stBest = stCand;
    m_fBestScore = fScore;
    m_bHasBest = true;
    return true;
}

bool CBestShotSelector::expire(long long llNowMs)
{
    if (m_bHasBest && m_llLastFeedMs > 0 && llNowMs - m_llLastFeedMs > m_stCfg.llHoldWindowMs)
    {
        reset();
        return true;
    }
    return false;
}
} // namespace Bestshot_NS
