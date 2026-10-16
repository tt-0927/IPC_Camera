/**
 * @FilePath     : svp_ive_sharpness.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : IVE 硬件清晰度评分封装（抓拍取优用）。
 *
 * 评分流水线（全硬件算子，ROI 小图上执行）：
 *   mag_and_ang（5x5 sobel 模板求梯度幅值 U16，内置 threshold 截断弱梯度）
 *   -> threshold_u16（显著梯度二值化为 0/1）
 *   -> integ SUM（U32 积分图，任意 ROI 四点采样 O(1) 求和）
 *   输出 = ROI 内显著梯度像素占比（0.0~1.0）。
 *
 * 语义：运动模糊帧梯度幅值整体下降，被 threshold 截断后占比骤降，
 * 可有效区分清晰帧与模糊帧（业界 Tenengrad/EOH 同源思路）。
 *
 * 约束（IVE 硬件）：ROI 宽高 64x64 ~ 1920x1024；起点地址 16 字节对齐
 * （x 坐标内部向下 16 像素对齐修正，最多左移 15 像素）。
 */

#ifndef _SVP_IVE_SHARPNESS_H_
#define _SVP_IVE_SHARPNESS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_common_ive.h"
#include "ot_common_video.h"
#include "ss_mpi_ive.h"

typedef struct _HiIveSharp_S HiIveSharp_S;

/* 清晰度评分配置 */
typedef struct
{
    td_u32 width;          /* 最大 ROI 宽 [64, 1920]，实际 ROI 不超过该值 */
    td_u32 height;         /* 最大 ROI 高 [64, 1024] */
    td_u16 grad_threshold; /* 显著梯度阈值（mag_and_ang 内置截断），噪声大场景调高 */
} HiIveSharpParam_S;

/**
 * @brief   分配清晰度评分句柄（按最大 ROI 预分配全部 IVE 中间内存）
 * @return  成功返回句柄，失败返回 NULL
 */
HiIveSharp_S *svpIveSharp_alloc(HiIveSharpParam_S stParam);

/**
 * @brief   释放清晰度评分句柄
 */
void svpIveSharp_release(HiIveSharp_S *pHandle);

/**
 * @brief   计算帧内 ROI 的显著梯度占比（清晰度评分）
 * @param   pHandle      句柄
 * @param   physY        帧_Y_平面物理地址（VB 帧首平面）
 * @param   frameStride  帧行跨度（字节，16 对齐）
 * @param   pRoi         ROI 框（帧像素坐标；x 内部 16 对齐修正）
 * @return  [0.0, 1.0] 占比；负值表示失败（ROI 越界/过小/IVE 错误）
 */
float svpIveSharp_eval(HiIveSharp_S *pHandle, td_phys_addr_t physY, td_u32 frameStride, const ot_rect *pRoi);

#ifdef __cplusplus
}
#endif

#endif /* _SVP_IVE_SHARPNESS_H_ */
