/**
 * @FilePath     : frame_geometry.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 15:15:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 15:15:00
 * @Description  : 坐标链路唯一出口
 */

#pragma once

#include "classroom_define.h"

namespace Classroom
{
/**
 * @brief 帧间几何变换描述
 */
class CFrameGeometry
{
public:
    CFrameGeometry() = default;
    ~CFrameGeometry() = default;

    /**
     * @brief   : 建立源帧到目标帧的变换
     * @param    {int} nSrcW / nSrcH：源帧（AI 分析帧）宽高
     * @param    {int} nDstW / nDstH：目标帧（送模型 / OSD 的图）宽高
     * @param    {MirrorMode_E} enMirror：镜像模式，MIRROR_NONE 表示不变换
     * @param    {int} nRotate：镜像之后再顺时针旋转的角度，仅支持 0 / 90 / 180 / 270
     * @return   {bool} 参数非法（尺寸 <=0 或角度非 90 倍数）返回 false，且对象保持未初始化
     */
    bool setup(int nSrcW, int nSrcH, int nDstW, int nDstH, MirrorMode_E enMirror, int nRotate);

    /* 是否已完成有效初始化 */
    bool isReady() const { return m_bReady; }

    bool isMirrored() const { return m_enMirror != MirrorMode_E::MIRROR_NONE; }

    int  srcWidth() const  { return m_nSrcW; }
    int  srcHeight() const { return m_nSrcH; }
    int  dstWidth() const  { return m_nDstW; }
    int  dstHeight() const { return m_nDstH; }

    /* 建帧时用的镜像/旋转原值：配置热换时要比对「改了但没重建」，
       isMirrored() 只能回答"有没有镜像"，回答不了"是不是同一个镜像模式" */
    int rotationOf() const { return m_nRotate; }
    MirrorMode_E mirrorOf() const { return m_enMirror; }

    /* 缩放比（源帧 1 px → 目标帧多少 px）。对齐残差的噪声地板门要用它 */
    float scaleXOf() const { return m_fScaleX; }
    float scaleYOf() const { return m_fScaleY; }

    /* ---------------- 点 ---------------- */
    CPointf_S srcToDst(const CPointf_S &stPoint) const;
    CPointf_S dstToSrc(const CPointf_S &stPoint) const;

    /* ---------------- 矩形框 ---------------- */
    /* 正变换后按轴对齐取 min/max，90° 整数倍旋转与镜像均保持矩形性 */
    CRectf_S srcToDst(const CRectf_S &stRect) const;
    CRectf_S dstToSrc(const CRectf_S &stRect) const;

    /* 把目标帧内的框按 (fExpandW, fExpandH) 各向内外扩张（负值为收缩），并钳制到画面内
       用途：人头框向下扩 7 头高、左右各 3 头宽取人体框，越界必须钳制而非"假躯干" */
    CRectf_S expandInDst(const CRectf_S &stRect, float fExpandW, float fExpandH) const;

    /* ---------------- Web / nCode 区域 ---------------- */
    /**
     * @brief   : 下发的区域坐标 → 源帧坐标
     * @param    {CRectf_S} stRect：Web 侧矩形
     * @param    {CoordSpace_E} enSpace：取值空间（像素 or 0~8191 归一）
     * @param    {int} nRefW / nRefH：归一化基准分辨率（下发协议约定的画面分辨率）
     */
    CRectf_S webToSrc(const CRectf_S &stRect, CoordSpace_E enSpace, int nRefW, int nRefH) const;

    /**
     * @brief   : 源帧坐标 → 上报给平台的区域坐标（webToSrc 的逆）
     */
    CRectf_S srcToWeb(const CRectf_S &stRect, CoordSpace_E enSpace, int nRefW, int nRefH) const;

    /**
     * @brief   : 分析帧坐标下的点 → **源帧归一化**坐标（0~1，x 向右 y 向下）
     * @param    {const CPointf_S&} stPoint：分析帧坐标的点
     * @param    {CPointf_S&} rstOut：OUT=归一化点（越界钳到 [0,1]）
     * @return   {bool} false 表示几何未就绪或源画幅非法，rstOut 未被写过
     */
    bool dstToNormSrc(const CPointf_S &stPoint, CPointf_S &rstOut) const;

    /**
     * @brief   : 方向矢量（不含平移）从源帧变换到目标帧
     * @param    {float} fDx / fDy：源帧方向分量，右/下为正
     * @param    {float} &rfOutDx / &rfOutDy：目标帧方向分量
     */
    void vectorSrcToDst(float fDx, float fDy, float &rfOutDx, float &rfOutDy) const;

    /**
     * @brief   : 目标帧方向逆变换回源帧
     */
    void vectorDstToSrc(float fDx, float fDy, float &rfOutDx, float &rfOutDy) const;

    /**
     * @brief   : 自校验——任取若干源帧点，正变换后逆变换应回到原位
     * @return   {bool} 全部点在 1 像素容差内还原返回 true
     */
    bool selfCheck() const;

private:
    /* 源帧内做镜像，再旋转到 (m_nRotW, m_nRotH) 空间，最后缩放至目标帧 */
    void applyForward(const CPointf_S &stIn, CPointf_S &stOut) const;
    void applyInverse(const CPointf_S &stIn, CPointf_S &stOut) const;

    /* 纯镜像（对合运算，正反变换同一实现） */
    CPointf_S mirrorInSrc(const CPointf_S &stPoint) const;
    /* 顺时针旋转 nRotate 度，(nW,nH) 为旋转前画幅 */
    CPointf_S rotate(const CPointf_S &stPoint, int nW, int nH, int nRotate) const;

private:
    bool m_bReady = false;

    int m_nSrcW = 0;
    int m_nSrcH = 0;
    /* 镜像 + 旋转之后的画幅（90°/270° 时宽高互换），缩放比例以此为基准 */
    int m_nRotW = 0;
    int m_nRotH = 0;
    int m_nDstW = 0;
    int m_nDstH = 0;

    MirrorMode_E m_enMirror = MirrorMode_E::MIRROR_NONE;
    int m_nRotate = 0;

    float m_fScaleX = 1.0f;
    float m_fScaleY = 1.0f;
};
}
