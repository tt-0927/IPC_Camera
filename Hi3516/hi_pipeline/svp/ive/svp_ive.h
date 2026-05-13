/**
 * @FilePath     : svp_ive.h
 * @Author       : cyc
 * @Date         : 2025-07-22 15:17:17
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-24 09:33:49
 * @Description  : 海思ive模块封装
 */

#ifndef _SVP_IVE_H_
#define _SVP_IVE_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ot_common_ive.h"  
#include "ss_mpi_ive.h" 

typedef struct _HiIve_S HiIve_S;

/**
 * @brief 创建 IVE 图像所需的必备参数
 */
typedef struct _HiIveNeedParam_S
{
    ot_svp_img_type enType;         /* 图像格式枚举 */
    td_u32 nWidth;                  /* 图像宽度 */
    td_u32 nHeight;                 /* 图像高度 */
} HiIveNeedParam_S;

/**
 * @brief 图像格式对应的内存遍历信息
 * note:用于内部计算各 plane 高度及像素字节数。
 */
typedef struct {
    td_u32 ele_size;                    /* 每个像素/元素所占字节数 */
    td_u32 loop_c;                      /* plane（或通道）数量 */
    td_u32 loop_h[OT_SVP_IMG_ADDR_NUM]; /* 各 plane 对应的行数 */
} ot_ive_image_loop_info;

/**
 * @brief IVE 操作句柄
 *
 * 通过该句柄调用统一的创建/销毁接口。
 */
struct _HiIve_S
{
    /**********************必需参数***************************/
    HiIveNeedParam_S stNeedParam;
    
    /**********************功能列表***************************/
    /**
     * @brief 创建 IVE 图像内存
     * @param[in]  pHandle  本句柄指针
     * @param[out] pImg     待填充的 ot_svp_img 结构体
     * @retval 0  成功
     * @retval -1 失败（打印日志）
     */
    int (*create_ive_image)(HiIve_S *pHandle,ot_svp_img *pImg);

    /**
     * @brief 销毁 IVE 图像内存
     * @param[in] pImg  由 create_ive_image 创建的图像
     * @retval 0  成功
     * @retval -1 失败（打印日志）
     */
    int (*destroy_ive_image)(ot_svp_img *pImg);
};


 /*** 
  * @description : 分配ive句柄
  * @author      : cyc
  * @param        {HiIveNeedParam_S} stNeedParam ive必须参数
  * @return       {*}成功返回句柄，失败返回NULL
  */ 
 HiIve_S *svpIve_alloc(HiIveNeedParam_S stNeedParam);

/*** 
 * @description : 释放ive句柄
 * @author      : cyc
 * @param        {HiIve_S} *pHandle：句柄
 * @return       {*}
 */ 
void svpIve_release(HiIve_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
