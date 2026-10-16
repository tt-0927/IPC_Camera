/**
 * @FilePath     : ive_sharpness_adapter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : IVE 硬件清晰度评分适配器实现。
 */

#include "ive_sharpness_adapter.h"

namespace FaceDetectInternal
{
namespace
{
/* IVE 硬件 ROI 边长下限（像素），低于下限不评分 */
constexpr uint32_t IVE_ROI_MIN = 64;
} // namespace

CIveSharpnessAdapter::CIveSharpnessAdapter(HiIveSharpParam_S stParam)
{
    m_pHandle = svpIveSharp_alloc(stParam);
}

CIveSharpnessAdapter::~CIveSharpnessAdapter()
{
    if (m_pHandle != nullptr)
    {
        svpIveSharp_release(m_pHandle);
        m_pHandle = nullptr;
    }
}

float CIveSharpnessAdapter::eval(const Bestshot_NS::BestShotFrameView_S &stFrame, const Bestshot_NS::BestShotRect_S &stRoi)
{
    if (m_pHandle == nullptr || stFrame.unYPhysAddr == 0)
    {
        return -1.0f;
    }
    if (stRoi.unWidth < IVE_ROI_MIN || stRoi.unHeight < IVE_ROI_MIN)
    {
        return -1.0f;
    }

    ot_rect stIveRoi;
    stIveRoi.x = stRoi.nX;
    stIveRoi.y = stRoi.nY;
    stIveRoi.width = stRoi.unWidth;
    stIveRoi.height = stRoi.unHeight;
    return svpIveSharp_eval(m_pHandle, static_cast<td_phys_addr_t>(stFrame.unYPhysAddr), stFrame.unStride, &stIveRoi);
}
} // namespace FaceDetectInternal
