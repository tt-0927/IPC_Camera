/*** 
 * @FilePath     : svp_od.h
 * @Author       : cyc
 * @Date         : 2025-07-25 09:23:21
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-29 15:34:45
 * @Description  : 海思遮挡侦测封装
 */

#ifndef _MPP_SVP_OD_H_
#define _MPP_SVP_OD_H_
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
#include "sample_common_ive.h"
#include <math.h>

typedef struct _HiOd_S HiOd_S;

/**
 * @brief 创建 IVE 图像所需的必备参数
 */
typedef struct _HiOdNeedParam_S
{
    ot_svp_img_type enType;         /* 图像格式枚举 */
    td_u32 nWidth;                  /* 图像宽度，必须为宏块宽的偶数倍，范围：[64, 1920] */
    td_u32 nHeight;                 /* 图像高度，必须为宏块高的偶数倍，范围：[64, 1920] */
} HiOdNeedParam_S;

struct _HiOd_S
{
    //info /**********************必需参数***************************/
    HiOdNeedParam_S stNeedParam;

    //info /**********************辅助参数***************************/
    ot_svp_src_img stSrcFrame;      /* 灰度图缓冲区 */ 
    ot_svp_dst_img stIntegFrame;    /* 积分图结果缓冲区 */ 
    ot_ive_integ_ctrl stIntegCtrl;  /* 积分图控制参数 */
    td_u32 nWidth;                  /* 分块后横向 cell 数 */
    td_u32 nHeight;                 /* 分块后纵向 cell 数 */
    td_bool bInited;                /* 是否已初始化 */ 

    //info /**********************功能列表***************************/
    /*遮挡侦测初始化*/
    int (*svpOd_init)(HiOd_S *pHandle);

    /*遮挡侦测去初始化*/
    int (*svpOd_uninit)(HiOd_S *pHandle);

    /*送帧给OD进行检测处理*/
    int (*svpOd_sendFrame)(HiOd_S *pHandle, ot_video_frame_info *pFrameInfo);
 
};

/**
 * @brief       : 分配遮挡侦测模块句柄
 * @author      : cyc
 * @param        {HiOdNeedParam_S} stNeedParam：ai必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
 HiOd_S *svpOd_alloc(HiOdNeedParam_S stNeedParam);

/**
 * @brief       : 释放遮挡侦测模块句柄
 * @author      : cyc
 * @param        {HiOd_S} *pHandle：句柄
 * @return       {*}
 */
void svpOd_release(HiOd_S *pHandle);


#ifdef __cplusplus
}
#endif
#endif
