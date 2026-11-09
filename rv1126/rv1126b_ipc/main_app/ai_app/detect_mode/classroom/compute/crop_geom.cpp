/**
 * @FilePath     : crop_geom.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-08 10:16:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-08 10:16:00
 * @Description  : 裁剪窗口算式与摆正计划的实现
 */

#include "crop_geom.h"

#include <cmath>

namespace Classroom
{
namespace
{
/* 向下取偶：与 rga_image_transform 内部的 x &= ~1 完全同一口径 */
inline int evenDown(int nValue)
{
    return nValue & ~1;
}

/* 把任意角折进 [0,360) */
inline int foldAngle(int nAngle)
{
    int n = nAngle % 360;
    if (n < 0)
    {
        n += 360;
    }
    return n;
}
}

RoiAlignBad_E alignRoiToRga(int nSrcW, int nSrcH, const CRectf_S &stRectSrc, CRoiAlign_S &rstOut)
{
    if (nSrcW <= 0 || nSrcH <= 0)
    {
        return RoiAlignBad_E::ROI_BAD_FRAME;
    }

    /* NaN 必须在这里挡住而不是让它走完算式 */
    if (!(std::isfinite(stRectSrc.fX1) && std::isfinite(stRectSrc.fY1) &&
          std::isfinite(stRectSrc.fX2) && std::isfinite(stRectSrc.fY2)))
    {
        return RoiAlignBad_E::ROI_BAD_RECT;
    }

    /* 先按浮点夹进画面邻域，再转整数：1e30 是有限的（上一条挡不住它），而它直接
       static_cast<int> 就是 float-cast-overflow 这一类未定行为。"框远在画面外"与
       "框在边上一格外"钳制之后本就是同一件事，所以这里收窄而不是拒绝 */
    const float fBound = static_cast<float>(nSrcW + nSrcH) + 2.0f;
    auto clampNear = [fBound](float fValue) {
        return (fValue < -fBound) ? -fBound : ((fValue > fBound) ? fBound : fValue);
    };

    /* 浮点框转整数：左上向下取、右下向上取，保证不漏边 */
    int nX1 = static_cast<int>(std::floor(clampNear(stRectSrc.fX1)));
    int nY1 = static_cast<int>(std::floor(clampNear(stRectSrc.fY1)));
    int nX2 = static_cast<int>(std::ceil(clampNear(stRectSrc.fX2)));
    int nY2 = static_cast<int>(std::ceil(clampNear(stRectSrc.fY2)));

    /* 顺序不能换：先把左上钳进画面，再以它为上界钳右下。反过来会得到 nCw<0，
       而负宽进 RGA 是"收缩成 0"还是"按无符号天量"取决于实现，两种都不是我们想要的 */
    nX1 = clampValue(nX1, 0, nSrcW - 1);
    nY1 = clampValue(nY1, 0, nSrcH - 1);
    nX2 = clampValue(nX2, nX1, nSrcW);
    nY2 = clampValue(nY2, nY1, nSrcH);

    if (nX2 <= nX1 || nY2 <= nY1)
    {
        return RoiAlignBad_E::ROI_BAD_RECT;
    }

    CRoiAlign_S stAlign;
    stAlign.nCx = evenDown(nX1);
    stAlign.nCy = evenDown(nY1);
    stAlign.nCw = evenDown(nX2 - nX1);
    stAlign.nCh = evenDown(nY2 - nY1);

    /* 到这里不必再抄一遍 RGA 的"越界收缩" */

    if (stAlign.nCw < 2 || stAlign.nCh < 2)
    {
        return RoiAlignBad_E::ROI_TOO_SMALL;
    }

    rstOut = stAlign;
    return RoiAlignBad_E::ROI_OK;
}

COrientPlan_S orientPlanOf(MirrorMode_E enMirror, int nRotate, int nBufW, int nBufH)
{
    COrientPlan_S stPlan;

    const int nAngle = foldAngle(nRotate);
    if (0 != (nAngle % 90))
    {
        stPlan.enBad = OrientBad_E::ORIENT_BAD_ANGLE;
        return stPlan;
    }

    MirrorMode_E enEff = enMirror;
    int nRot = nAngle;

    /* MIRROR_BOTH 与"转 180°"在像素上同一件事 ⇒ 折进角度，省一次整幅原地遍历。
       折叠之后有效镜像只剩 NONE / H / V 三种，共轭规则也只在这一支之后才适用 */
    if (MirrorMode_E::MIRROR_BOTH == enEff)
    {
        enEff = MirrorMode_E::MIRROR_NONE;
        nRot = foldAngle(nRot + 180);
    }

    /* 共轭规则：真正要翻的轴是 Rθ∘M∘Rθ⁻¹，不是 M 本身 */
    const bool bSwap = (90 == nRot || 270 == nRot);
    bool bFlipH = false;
    bool bFlipV = false;
    if (MirrorMode_E::MIRROR_H_FLIP == enEff)
    {
        bFlipH = !bSwap;
        bFlipV = bSwap;
    }
    else if (MirrorMode_E::MIRROR_V_FLIP == enEff)
    {
        bFlipV = !bSwap;
        bFlipH = bSwap;
    }

    stPlan.nRgaRotation = nRot;
    stPlan.bFlipH = bFlipH;
    stPlan.bFlipV = bFlipV;
    stPlan.bNeeded = (0 != nRot) || bFlipH || bFlipV;

    if (!stPlan.bNeeded)
    {
        stPlan.enBad = OrientBad_E::ORIENT_NONE_NEEDED;   /* bSupported 留 false：调用方按"原样喂"处理 */
        return stPlan;
    }

    /* 90/270 会把内容转进一个**换了宽高**的形状，而这一块缓冲的尺寸就是模型输入档 */
    if ((90 == nRot || 270 == nRot) && nBufW != nBufH)
    {
        stPlan.enBad = OrientBad_E::ORIENT_NEED_SWAP;
        return stPlan;
    }
    if (nBufW <= 0 || nBufH <= 0)
    {
        stPlan.enBad = OrientBad_E::ORIENT_BAD_ANGLE;   /* 没有可摆的图：与"角度非法"同一出口（都不动） */
        return stPlan;
    }

    stPlan.bSupported = true;
    stPlan.enBad = OrientBad_E::ORIENT_OK;
    return stPlan;
}

void flipRgb888H(uint8_t *pucRgb, int nW, int nH)
{
    if (nullptr == pucRgb || nW < 2 || nH <= 0)
    {
        return;
    }

    for (int nY = 0; nY < nH; ++nY)
    {
        uint8_t *pucRow = pucRgb + static_cast<size_t>(nY) * nW * 3;
        for (int nX = 0; nX < nW / 2; ++nX)
        {
            const int nX2 = nW - 1 - nX;
            for (int nC = 0; nC < 3; ++nC)
            {
                const uint8_t t = pucRow[nX * 3 + nC];
                pucRow[nX * 3 + nC] = pucRow[nX2 * 3 + nC];
                pucRow[nX2 * 3 + nC] = t;
            }
        }
    }
}

void flipRgb888V(uint8_t *pucRgb, int nW, int nH)
{
    if (nullptr == pucRgb || nW <= 0 || nH < 2)
    {
        return;
    }

    const size_t nRowBytes = static_cast<size_t>(nW) * 3;
    for (int nY = 0; nY < nH / 2; ++nY)
    {
        uint8_t *pucA = pucRgb + static_cast<size_t>(nY) * nRowBytes;
        uint8_t *pucB = pucRgb + static_cast<size_t>(nH - 1 - nY) * nRowBytes;
        for (size_t nI = 0; nI < nRowBytes; ++nI)
        {
            const uint8_t t = pucA[nI];
            pucA[nI] = pucB[nI];
            pucB[nI] = t;
        }
    }
}

CLetterboxPlan_S letterboxPlan(int nUsedW, int nUsedH, int nModelW, int nModelH)
{
    /* 默认即 disable：任何一条前置不满足都整份放弃，绝不返回半套计划 */
    CLetterboxPlan_S stPlan;

    if (nUsedW < 2 || nUsedH < 2 || nModelW < 2 || nModelH < 2)
    {
        return stPlan;
    }
    /* 模型档的对齐是 CCropUnit::setup 的建档约束；这里再核一遍是因为
       pad 的落点合法性全靠它：W%4 让 contentX 天然偶对齐、H%2 同理 */
    if ((nModelW % 4) != 0 || (nModelH % 2) != 0)
    {
        return stPlan;
    }

    /* 保序缩放：s 取两方向的最小值。share 的 RGA 封装只接受"源矩形 → 整幅目标"，
       dst 子矩形写不进去 ⇒ 只有"内容恰好填满宽、pad 全落在底部"这一种形态可实现；
       宽不是约束边界（竖构图/方图，需要右侧 pad）一律 disable 回退现网拉伸。
       约束方向取模型档本身（不走"先除后乘"的浮点圆整：640/1920*1920 可能得 639.99，
       取整后内容填不满宽，主场景被静默打回拉伸）；另一方向整数式比例下取并齐到
       RGA 对齐档，绝不上取（上取会越界写进 pad 区之外的第 modelW 列） */
    const float fSx = static_cast<float>(nModelW) / static_cast<float>(nUsedW);
    const float fSy = static_cast<float>(nModelH) / static_cast<float>(nUsedH);
    if (fSx > fSy)
    {
        return stPlan;
    }

    int nContentW = nModelW;
    int nContentH = static_cast<int>(static_cast<float>(nUsedH) * fSx);
    nContentH -= nContentH % 2;
    if (nContentH < 2 || nContentH > nModelH)
    {
        return stPlan;
    }

    stPlan.bEnable = true;
    stPlan.nContentW = nContentW;
    stPlan.nContentH = nContentH;
    return stPlan;
}
}
