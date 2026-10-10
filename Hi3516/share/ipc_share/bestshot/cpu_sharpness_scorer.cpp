/**
 * @FilePath     : cpu_sharpness_scorer.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : CPU 软件清晰度评分后端实现。
 */

#include "cpu_sharpness_scorer.h"

#include <algorithm>

namespace Bestshot_NS
{
namespace
{
/* 评分 ROI 最小边长（像素）：过小的窗口梯度统计无意义 */
constexpr uint32_t CPU_SHARP_ROI_MIN = 8;
} // namespace

CCpuSharpnessScorer::CCpuSharpnessScorer() : CCpuSharpnessScorer(Config_S())
{
}

CCpuSharpnessScorer::CCpuSharpnessScorer(const Config_S &stCfg) : m_stCfg(stCfg)
{
    /* 配置保护：步进至少为 1，窗口上限不低于 ROI 下限 */
    m_stCfg.unSampleStep = std::max(1u, m_stCfg.unSampleStep);
    m_stCfg.unMaxRoiSide = std::max(CPU_SHARP_ROI_MIN, m_stCfg.unMaxRoiSide);
}

float CCpuSharpnessScorer::eval(const BestShotFrameView_S &stFrame, const BestShotRect_S &stRoi)
{
    if (stFrame.pYVirAddr == nullptr || stFrame.unStride == 0)
    {
        return -1.0f;
    }
    if (stRoi.unWidth < CPU_SHARP_ROI_MIN || stRoi.unHeight < CPU_SHARP_ROI_MIN || stRoi.nX < 0 || stRoi.nY < 0)
    {
        return -1.0f;
    }

    /* 评分窗口：ROI 超上限时中心截取（控算力，语义与 IVE 后端一致） */
    const uint32_t w = std::min(stRoi.unWidth, m_stCfg.unMaxRoiSide);
    const uint32_t h = std::min(stRoi.unHeight, m_stCfg.unMaxRoiSide);
    const int32_t x0 = stRoi.nX + static_cast<int32_t>((stRoi.unWidth - w) / 2);
    const int32_t y0 = stRoi.nY + static_cast<int32_t>((stRoi.unHeight - h) / 2);

    /* 幅值平方与阈值平方比较，避免逐像素开方 */
    const uint32_t th2 = static_cast<uint32_t>(m_stCfg.nGradThreshold) * m_stCfg.nGradThreshold;
    const uint32_t step = m_stCfg.unSampleStep;
    const size_t stride = stFrame.unStride;
    const uint8_t *base = stFrame.pYVirAddr;

    /* 边界一圈不参与统计（Sobel 3x3 需要邻域），ROI 本身已在帧内 */
    uint32_t total = 0;
    uint32_t strong = 0;
    for (uint32_t y = 1; y + 1 < h; y += step)
    {
        const uint8_t *row0 = base + static_cast<size_t>(y0 + y - 1) * stride + x0;
        const uint8_t *row1 = row0 + stride;
        const uint8_t *row2 = row1 + stride;
        for (uint32_t x = 1; x + 1 < w; x += step)
        {
            /* Sobel 3x3 水平/垂直梯度 */
            const int32_t gx = -row0[x - 1] + row0[x + 1] - 2 * row1[x - 1] + 2 * row1[x + 1] - row2[x - 1] + row2[x + 1];
            const int32_t gy = -row0[x - 1] - 2 * row0[x] - row0[x + 1] + row2[x - 1] + 2 * row2[x] + row2[x + 1];
            const uint32_t mag2 = static_cast<uint32_t>(gx * gx) + static_cast<uint32_t>(gy * gy);
            if (mag2 > th2)
            {
                strong++;
            }
            total++;
        }
    }
    if (total == 0)
    {
        return -1.0f;
    }
    return static_cast<float>(strong) / static_cast<float>(total);
}
} // namespace Bestshot_NS
