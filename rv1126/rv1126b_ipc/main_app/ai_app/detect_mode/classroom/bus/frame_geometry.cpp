/**
 * @FilePath     : frame_geometry.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 15:15:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 15:15:00
 * @Description  : 坐标链路唯一出口实现
 */

#include "frame_geometry.h"

#include <cmath>

#define CLASSROOM_NORM_COORD_BASE (8192.0f)

namespace Classroom
{
bool CFrameGeometry::setup(int nSrcW, int nSrcH, int nDstW, int nDstH, MirrorMode_E enMirror, int nRotate)
{
    if (nSrcW <= 0 || nSrcH <= 0 || nDstW <= 0 || nDstH <= 0)
    {
        m_bReady = false;
        return false;
    }

    /* 只接受 90 的整数倍：非直角旋转会让轴对齐框变成四边形，本子系统不支持 */
    int nNormalized = nRotate % 360;
    if (nNormalized < 0)
    {
        nNormalized += 360;
    }
    if (nNormalized % 90 != 0)
    {
        m_bReady = false;
        return false;
    }

    m_nSrcW   = nSrcW;
    m_nSrcH   = nSrcH;
    m_nDstW   = nDstW;
    m_nDstH   = nDstH;
    m_enMirror = enMirror;
    m_nRotate  = nNormalized;

    /* 90/270 度会互换画幅 */
    if (90 == m_nRotate || 270 == m_nRotate)
    {
        m_nRotW = nSrcH;
        m_nRotH = nSrcW;
    }
    else
    {
        m_nRotW = nSrcW;
        m_nRotH = nSrcH;
    }

    m_fScaleX = static_cast<float>(nDstW) / static_cast<float>(m_nRotW);
    m_fScaleY = static_cast<float>(nDstH) / static_cast<float>(m_nRotH);

    m_bReady = true;
    return true;
}

CPointf_S CFrameGeometry::mirrorInSrc(const CPointf_S &stPoint) const
{
    CPointf_S stOut = stPoint;

    if (MirrorMode_E::MIRROR_H_FLIP == m_enMirror || MirrorMode_E::MIRROR_BOTH == m_enMirror)
    {
        stOut.fX = static_cast<float>(m_nSrcW - 1) - stOut.fX;
    }
    if (MirrorMode_E::MIRROR_V_FLIP == m_enMirror || MirrorMode_E::MIRROR_BOTH == m_enMirror)
    {
        stOut.fY = static_cast<float>(m_nSrcH - 1) - stOut.fY;
    }
    return stOut;
}

CPointf_S CFrameGeometry::rotate(const CPointf_S &stPoint, int nW, int nH, int nRotate) const
{
    CPointf_S stOut;

    switch (nRotate)
    {
        case 90:
            /* 顺时针 90°：左上角 → 右上角 */
            stOut.fX = static_cast<float>(nH - 1) - stPoint.fY;
            stOut.fY = stPoint.fX;
            break;
        case 180:
            stOut.fX = static_cast<float>(nW - 1) - stPoint.fX;
            stOut.fY = static_cast<float>(nH - 1) - stPoint.fY;
            break;
        case 270:
            /* 顺时针 270°（逆时针 90°）：左上角 → 左下角 */
            stOut.fX = stPoint.fY;
            stOut.fY = static_cast<float>(nW - 1) - stPoint.fX;
            break;
        case 0:
        default:
            stOut = stPoint;
            break;
    }
    return stOut;
}

void CFrameGeometry::applyForward(const CPointf_S &stIn, CPointf_S &stOut) const
{
    CPointf_S stMirrored = mirrorInSrc(stIn);
    /* 镜像后仍在源画幅内，旋转以源宽高为基准 */
    CPointf_S stRotated = rotate(stMirrored, m_nSrcW, m_nSrcH, m_nRotate);

    stOut.fX = stRotated.fX * m_fScaleX;
    stOut.fY = stRotated.fY * m_fScaleY;
}

void CFrameGeometry::applyInverse(const CPointf_S &stIn, CPointf_S &stOut) const
{
    CPointf_S stScaled;
    stScaled.fX = (m_fScaleX > 0.0f) ? (stIn.fX / m_fScaleX) : 0.0f;
    stScaled.fY = (m_fScaleY > 0.0f) ? (stIn.fY / m_fScaleY) : 0.0f;

    /* 逆旋转 = 反向旋转，基准是旋转后的画幅 */
    CPointf_S stRotated = rotate(stScaled, m_nRotW, m_nRotH, (360 - m_nRotate) % 360);
    /* 镜像是对合运算，正逆同一实现 */
    stOut = mirrorInSrc(stRotated);
}

CPointf_S CFrameGeometry::srcToDst(const CPointf_S &stPoint) const
{
    CPointf_S stOut = stPoint;
    if (!m_bReady)
    {
        return stOut;
    }
    applyForward(stPoint, stOut);
    return stOut;
}

CPointf_S CFrameGeometry::dstToSrc(const CPointf_S &stPoint) const
{
    CPointf_S stOut = stPoint;
    if (!m_bReady)
    {
        return stOut;
    }
    applyInverse(stPoint, stOut);
    return stOut;
}

CRectf_S CFrameGeometry::srcToDst(const CRectf_S &stRect) const
{
    if (!m_bReady)
    {
        return stRect;
    }

    CPointf_S stLT = srcToDst(CPointf_S{ stRect.fX1, stRect.fY1 });
    CPointf_S stRB = srcToDst(CPointf_S{ stRect.fX2, stRect.fY2 });

    CRectf_S stOut;
    stOut.fX1 = (stLT.fX < stRB.fX) ? stLT.fX : stRB.fX;
    stOut.fY1 = (stLT.fY < stRB.fY) ? stLT.fY : stRB.fY;
    stOut.fX2 = (stLT.fX > stRB.fX) ? stLT.fX : stRB.fX;
    stOut.fY2 = (stLT.fY > stRB.fY) ? stLT.fY : stRB.fY;

    stOut.fX1 = clampValue(stOut.fX1, 0.0f, static_cast<float>(m_nDstW));
    stOut.fY1 = clampValue(stOut.fY1, 0.0f, static_cast<float>(m_nDstH));
    stOut.fX2 = clampValue(stOut.fX2, 0.0f, static_cast<float>(m_nDstW));
    stOut.fY2 = clampValue(stOut.fY2, 0.0f, static_cast<float>(m_nDstH));
    return stOut;
}

CRectf_S CFrameGeometry::dstToSrc(const CRectf_S &stRect) const
{
    if (!m_bReady)
    {
        return stRect;
    }

    CPointf_S stLT = dstToSrc(CPointf_S{ stRect.fX1, stRect.fY1 });
    CPointf_S stRB = dstToSrc(CPointf_S{ stRect.fX2, stRect.fY2 });

    CRectf_S stOut;
    stOut.fX1 = (stLT.fX < stRB.fX) ? stLT.fX : stRB.fX;
    stOut.fY1 = (stLT.fY < stRB.fY) ? stLT.fY : stRB.fY;
    stOut.fX2 = (stLT.fX > stRB.fX) ? stLT.fX : stRB.fX;
    stOut.fY2 = (stLT.fY > stRB.fY) ? stLT.fY : stRB.fY;

    stOut.fX1 = clampValue(stOut.fX1, 0.0f, static_cast<float>(m_nSrcW));
    stOut.fY1 = clampValue(stOut.fY1, 0.0f, static_cast<float>(m_nSrcH));
    stOut.fX2 = clampValue(stOut.fX2, 0.0f, static_cast<float>(m_nSrcW));
    stOut.fY2 = clampValue(stOut.fY2, 0.0f, static_cast<float>(m_nSrcH));
    return stOut;
}

CRectf_S CFrameGeometry::expandInDst(const CRectf_S &stRect, float fExpandW, float fExpandH) const
{
    CRectf_S stOut = stRect;

    stOut.fX1 -= fExpandW;
    stOut.fX2 += fExpandW;
    stOut.fY1 -= fExpandH;
    stOut.fY2 += fExpandH;

    /* 外扩后必须钳制到画面内：越界裁剪交给 RGA 做，不允许用"假躯干"补数据 */
    stOut.fX1 = clampValue(stOut.fX1, 0.0f, static_cast<float>(m_nDstW - 1));
    stOut.fY1 = clampValue(stOut.fY1, 0.0f, static_cast<float>(m_nDstH - 1));
    stOut.fX2 = clampValue(stOut.fX2, 0.0f, static_cast<float>(m_nDstW));
    stOut.fY2 = clampValue(stOut.fY2, 0.0f, static_cast<float>(m_nDstH));

    if (stOut.fX2 < stOut.fX1)
    {
        stOut.fX2 = stOut.fX1;
    }
    if (stOut.fY2 < stOut.fY1)
    {
        stOut.fY2 = stOut.fY1;
    }
    return stOut;
}

CRectf_S CFrameGeometry::webToSrc(const CRectf_S &stRect, CoordSpace_E enSpace, int nRefW, int nRefH) const
{
    CRectf_S stOut = stRect;

    if (!m_bReady || nRefW <= 0 || nRefH <= 0)
    {
        return stOut;
    }

    if (CoordSpace_E::COORD_SPACE_NORM_8192 == enSpace)
    {
        /* 0~8191 归一：先还原到基准分辨率像素，再缩放到源帧 */
        stOut.fX1 = stRect.fX1 / CLASSROOM_NORM_COORD_BASE * static_cast<float>(m_nSrcW);
        stOut.fY1 = stRect.fY1 / CLASSROOM_NORM_COORD_BASE * static_cast<float>(m_nSrcH);
        stOut.fX2 = stRect.fX2 / CLASSROOM_NORM_COORD_BASE * static_cast<float>(m_nSrcW);
        stOut.fY2 = stRect.fY2 / CLASSROOM_NORM_COORD_BASE * static_cast<float>(m_nSrcH);
    }
    else
    {
        float fSx = static_cast<float>(m_nSrcW) / static_cast<float>(nRefW);
        float fSy = static_cast<float>(m_nSrcH) / static_cast<float>(nRefH);

        stOut.fX1 = stRect.fX1 * fSx;
        stOut.fY1 = stRect.fY1 * fSy;
        stOut.fX2 = stRect.fX2 * fSx;
        stOut.fY2 = stRect.fY2 * fSy;
    }

    stOut.fX1 = clampValue(stOut.fX1, 0.0f, static_cast<float>(m_nSrcW));
    stOut.fY1 = clampValue(stOut.fY1, 0.0f, static_cast<float>(m_nSrcH));
    stOut.fX2 = clampValue(stOut.fX2, 0.0f, static_cast<float>(m_nSrcW));
    stOut.fY2 = clampValue(stOut.fY2, 0.0f, static_cast<float>(m_nSrcH));
    return stOut;
}

CRectf_S CFrameGeometry::srcToWeb(const CRectf_S &stRect, CoordSpace_E enSpace, int nRefW, int nRefH) const
{
    CRectf_S stOut = stRect;

    if (!m_bReady || nRefW <= 0 || nRefH <= 0)
    {
        return stOut;
    }

    if (CoordSpace_E::COORD_SPACE_NORM_8192 == enSpace)
    {
        stOut.fX1 = stRect.fX1 / static_cast<float>(m_nSrcW) * CLASSROOM_NORM_COORD_BASE;
        stOut.fY1 = stRect.fY1 / static_cast<float>(m_nSrcH) * CLASSROOM_NORM_COORD_BASE;
        stOut.fX2 = stRect.fX2 / static_cast<float>(m_nSrcW) * CLASSROOM_NORM_COORD_BASE;
        stOut.fY2 = stRect.fY2 / static_cast<float>(m_nSrcH) * CLASSROOM_NORM_COORD_BASE;
    }
    else
    {
        float fSx = static_cast<float>(nRefW) / static_cast<float>(m_nSrcW);
        float fSy = static_cast<float>(nRefH) / static_cast<float>(m_nSrcH);

        stOut.fX1 = stRect.fX1 * fSx;
        stOut.fY1 = stRect.fY1 * fSy;
        stOut.fX2 = stRect.fX2 * fSx;
        stOut.fY2 = stRect.fY2 * fSy;
    }
    return stOut;
}

bool CFrameGeometry::dstToNormSrc(const CPointf_S &stPoint, CPointf_S &rstOut) const
{
    /* 未就绪一律返回 false 而不是"照算"：没几何时 dstToSrc 是恒等变换，
       再除以源宽高会得到一个看起来合法、实际偏了一整档的数——轨迹点错位在日志里
       没有任何痕迹，只能在这一句挡住。调用方拿到 false 必须发"无位置"，不得发 0 */
    if (!m_bReady || m_nSrcW <= 0 || m_nSrcH <= 0)
    {
        return false;
    }

    const CPointf_S stSrc = dstToSrc(stPoint);

    rstOut.fX = clampValue(stSrc.fX / static_cast<float>(m_nSrcW), 0.0f, 1.0f);
    rstOut.fY = clampValue(stSrc.fY / static_cast<float>(m_nSrcH), 0.0f, 1.0f);
    return true;
}

void CFrameGeometry::vectorSrcToDst(float fDx, float fDy, float &rfOutDx, float &rfOutDy) const
{
    float fX = fDx;
    float fY = fDy;

    /* 镜像只翻符号，不翻长度 */
    if (MirrorMode_E::MIRROR_H_FLIP == m_enMirror || MirrorMode_E::MIRROR_BOTH == m_enMirror)
    {
        fX = -fX;
    }
    if (MirrorMode_E::MIRROR_V_FLIP == m_enMirror || MirrorMode_E::MIRROR_BOTH == m_enMirror)
    {
        fY = -fY;
    }

    float fRX = fX;
    float fRY = fY;
    switch (m_nRotate)
    {
        case 90:
            fRX = -fY;
            fRY = fX;
            break;
        case 180:
            fRX = -fX;
            fRY = -fY;
            break;
        case 270:
            fRX = fY;
            fRY = -fX;
            break;
        default:
            break;
    }

    /* 非等比缩放会改变矢量的倾角，朝向阈值要按实际画面算，故此处一并乘上缩放比 */
    rfOutDx = fRX * m_fScaleX;
    rfOutDy = fRY * m_fScaleY;
}

void CFrameGeometry::vectorDstToSrc(float fDx, float fDy, float &rfOutDx, float &rfOutDy) const
{
    float fX = (m_fScaleX > 0.0f) ? (fDx / m_fScaleX) : 0.0f;
    float fY = (m_fScaleY > 0.0f) ? (fDy / m_fScaleY) : 0.0f;

    float fRX = fX;
    float fRY = fY;
    switch (m_nRotate)
    {
        case 90:
            fRX = fY;
            fRY = -fX;
            break;
        case 180:
            fRX = -fX;
            fRY = -fY;
            break;
        case 270:
            fRX = -fY;
            fRY = fX;
            break;
        default:
            break;
    }

    if (MirrorMode_E::MIRROR_H_FLIP == m_enMirror || MirrorMode_E::MIRROR_BOTH == m_enMirror)
    {
        fRX = -fRX;
    }
    if (MirrorMode_E::MIRROR_V_FLIP == m_enMirror || MirrorMode_E::MIRROR_BOTH == m_enMirror)
    {
        fRY = -fRY;
    }

    rfOutDx = fRX;
    rfOutDy = fRY;
}

bool CFrameGeometry::selfCheck() const
{
    if (!m_bReady)
    {
        return false;
    }

    /* 取四角与中心共 5 个代表点做往返校验，1 像素容差覆盖浮点与 -1 索引约定带来的误差 */
    const float fW = static_cast<float>(m_nSrcW - 1);
    const float fH = static_cast<float>(m_nSrcH - 1);
    const CPointf_S astSamples[5] = {
        CPointf_S{ 0.0f, 0.0f },
        CPointf_S{ fW, 0.0f },
        CPointf_S{ 0.0f, fH },
        CPointf_S{ fW, fH },
        CPointf_S{ fW * 0.5f, fH * 0.5f },
    };

    for (int i = 0; i < 5; ++i)
    {
        CPointf_S stForward = srcToDst(astSamples[i]);
        CPointf_S stBack = dstToSrc(stForward);

        if (std::fabs(stBack.fX - astSamples[i].fX) > 1.0f ||
            std::fabs(stBack.fY - astSamples[i].fY) > 1.0f)
        {
            return false;
        }
    }
    return true;
}
}
