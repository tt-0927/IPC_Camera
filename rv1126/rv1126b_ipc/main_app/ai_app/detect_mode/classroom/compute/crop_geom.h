/**
 * @FilePath     : crop_geom.h
 * @Author       : leiyy
 * @Date         : 2026-10-08 10:16:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-08 10:16:00
 * @Description  : 裁剪窗口算式与"把图摆正"的计划
 */

#pragma once

#include <cstdint>

#include "classroom_define.h"

namespace Classroom
{
/**
 * @brief 一次 RGA 裁剪真正使用的整数窗口（源帧像素）
 */
typedef struct _RoiAlign_
{
    int nCx = 0;
    int nCy = 0;
    int nCw = 0;
    int nCh = 0;
} CRoiAlign_S;

/**
 * @brief `alignRoiToRga` 的分岔
 */
typedef enum class RoiAlignBad : int
{
    ROI_OK = 0,
    ROI_BAD_RECT,    /* 框退化（右下 <= 左上）或 NaN */
    ROI_BAD_FRAME,   /* 源帧尺寸非法或指针为空 */
    ROI_TOO_SMALL,   /* 钳制 + 取偶后不足 2×2 */
} RoiAlignBad_E;

/**
 * @brief   : 浮点框 → RGA 真正接受的整数窗口（左上向下取、右下向上取，再各自取偶并收缩）
 * @param    {int} nSrcW / nSrcH：源帧宽高
 * @param    {const CRectf_S&} stRectSrc：源帧坐标系下的浮点框，允许越界
 * @param    {CRoiAlign_S&} rstOut：OUT=仅在 ROI_OK 时写；其余分岔**不写出参**
 * @return   {RoiAlignBad_E} 见枚举
 * @note     : 与 `rga_image_transform` 内部的 `&= ~1` 与越界收缩逐字同口径。
 *             自己先算一遍的意义不是"替硬件做校验"，而是让"框太小"成为一个**可判定**的返回码
 */
RoiAlignBad_E alignRoiToRga(int nSrcW, int nSrcH, const CRectf_S &stRectSrc, CRoiAlign_S &rstOut);

/**
 * @brief "摆正"计划里没能摆正的原因
 */
typedef enum class OrientBad : int
{
    ORIENT_OK = 0,
    ORIENT_NONE_NEEDED,   /* 装法不需要任何动作：bNeeded=false，这不是失败 */
    ORIENT_BAD_ANGLE,     /* 角度不是 90 的整数倍（几何链路本身也不接受这种配置） */
    ORIENT_NEED_SWAP,     /* 要转 90/270 而缓冲不是方的：转完的宽高与模型输入档不符 */
} OrientBad_E;

/**
 * @brief 把一张从源帧裁出的图摆成"现场看到的样子"需要做的动作
 */
typedef struct _OrientPlan_
{
    bool bNeeded = false;     /* true=要动（翻或转），false=原样喂 */
    bool bSupported = false;  /* false=摆不出来，调用方应当**放弃这一次**而不是硬喂 */
    int  nRgaRotation = 0;    /* 0/90/180/270，顺时针，直接进 rga_image_transform 的 rotation */
    bool bFlipH = false;      /* 水平翻（x → W-1-x），在旋转**之后**的缓冲上做 */
    bool bFlipV = false;      /* 垂直翻（y → H-1-y），同上 */
    OrientBad_E enBad = OrientBad_E::ORIENT_NONE_NEEDED;
} COrientPlan_S;

/**
 * @brief   : 由装法（配置里的镜像 + 旋转）算出摆正计划
 * @param    {MirrorMode_E} enMirror：与 `CFrameGeometry` 用的是同一个配置值（不自己判断第二次）
 * @param    {int} nRotate：同上，镜像之后再顺时针旋转的角度
 * @param    {int} nBufW / nBufH：**摆正之后**那张图的宽高（= 模型输入档）
 * @return   {COrientPlan_S} 见结构体
 * @note     : 传缓冲尺寸而不是模型尺寸是本函数的本意：90/270 会换掉宽高，
 *             只有"档本来就是方的"才摆得正；这一条不是优化项而是**能不能做**的判据
 */
COrientPlan_S orientPlanOf(MirrorMode_E enMirror, int nRotate, int nBufW, int nBufH);

/**
 * @brief 等比缩放计划
 */
typedef struct _LetterboxPlan_
{
    bool bEnable = false;   /* false=回退整幅拉伸（现网形态），调用方不必区分原因 */
    int  nContentW = 0;     /* 实际写入的宽（恒等于模型档宽，%4 对齐随之满足） */
    int  nContentH = 0;     /* 实际写入的高（%2==0） */
} CLetterboxPlan_S;

/* letterbox pad 的填充灰阶（YOLO 惯例 114）：像素侧只有 crop_unit 用它铺底，
   放在本篇是为了与计划同源——改填充值不用去找像素搬运那篇 */
#define CLASSROOM_LETTERBOX_PAD_GRAY (114)

/**
 * @brief   : 等比缩放计划：usedSrc 宽高 → 模型输入档，保宽高比、pad 只落底部
 * @param    {int} nUsedW / nUsedH：真正喂进变换的源矩形宽高（整帧档=帧宽高）
 * @param    {int} nModelW / nModelH：模型输入档（须 W%4==0 且 H%2==0，即 CCropUnit::setup 的建档约束）
 * @return   {CLetterboxPlan_S} 见结构体；模型档不对齐、源退化、或宽不是约束边界（右侧要 pad）时 disable
 */
CLetterboxPlan_S letterboxPlan(int nUsedW, int nUsedH, int nModelW, int nModelH);


/**
 * @brief   : 紧密排列 RGB888 缓冲的水平/垂直翻（原地）
 * @param    {uint8_t*} pucRgb：W×H×3 紧密缓冲（stride==width，与裁剪档同一假定）
 * @note     : 只做逐像素三元组交换，不分配、不缩放。960×960 一次约 2.76 MB 遍历，
 *             分钟尺度一次，代价与一次 RGA 搬运同量级——而这是**唯一**能拿到正确字序的花费
 */
void flipRgb888H(uint8_t *pucRgb, int nW, int nH);
void flipRgb888V(uint8_t *pucRgb, int nW, int nH);
}
