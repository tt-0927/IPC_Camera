/**
 * @FilePath     : svp_md.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-28 19:22:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-23 14:17:44
 * @Description  : 海思移动侦测模块封装
 */

#ifndef _MPP_SVP_MD_H_
#define _MPP_SVP_MD_H_
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
#include <sys/time.h>
#include "sample_common_ive.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_sys.h"
#include "ot_common_svp.h"
#include "ot_common_md.h"
#include "ss_ivs_md.h"

#include "mpi_common.h"

/*海思 移动侦测通道枚举*/
typedef enum _HiMdChn_
{
    MOTION_DETECT_CHN = 0,       // 移动侦测通道
    SCENE_CHANGE_DETECT_CHN = 1, // 场景变更侦测通道
} HiMdChn_E;

/*海思 移动侦测必需参数*/
typedef struct _HiMdNeedParam_S
{
    /* 通道号 范围：[0, 3]*/
    int nChn;
    /* 图像宽 ，必须为宏块宽的偶数倍，范围：[64, 1920]*/
    td_u32 u32Width;
    /* 图像高，必须为宏块高的偶数倍，范围：[64, 1080] */
    td_u32 u32Height;
} HiMdNeedParam_S;

typedef struct _HiMdExParam_S
{
    /* 算法模式 */
    ot_md_alg_mode enAlgMode;
    /* Sad计算模式 按4x4、8x8、16x16像素块计算SAD*/
    ot_ive_sad_mode enSadMode;
    /* Sad输出控制模式 */
    ot_ive_sad_out_ctrl enSadOutCtrl; 
    /* Sad阈值 值越低，越灵敏 */
    td_u16 u16SadThreshold;           
    /* 连通区域标记控制参数 */
    ot_ive_ccl_ctrl stCclCtrl;      
    /* 两图像的加权加控制参数 */
    ot_ive_add_ctrl stAddCtrl;
    /* 动态分析结果比例 */
    /* 宽 */
    int nWRatio;
    /* 高 */
    int nHRatio;
    /* 是否手动更新参考帧 */
    td_bool bUpdateRef;
} HiMdExParam_S;

typedef struct _HiMd_S HiMd_S;
struct _HiMd_S
{
    //info /**********************必需参数***************************/
    HiMdNeedParam_S stNeedParam;
    HiMdExParam_S stExParam;

    //info /**********************辅助参数***************************/
    ot_svp_src_img stRefFrame;      // 参考帧
    ot_svp_src_img stCurFrame;     // 当前帧
    ot_svp_dst_img stSadImg;        // SAD 输出图像
    ot_svp_dst_mem_info stBlob;     // MD结果输出
    td_bool bFirstFrame;            // 是否为第一帧标志
    td_bool bInited;                // 是否已初始化

    //info /**********************功能列表***************************/
    /*移动侦测初始化*/
    int (*svpMd_init)(HiMd_S *pHandle);

    /*移动侦测去初始化*/
    int (*svpMd_uninit)(HiMd_S *pHandle);

    /*送帧给MD进行检测处理*/
    int (*svpMd_sendFrame)(HiMd_S *pHandle, ot_video_frame_info *pFrameInfo, ot_svp_dst_mem_info **ppResult);

    /*获取移动侦测结果*/
    int (*svpMd_getResult)(HiMd_S *pHandle, ot_sample_svp_rect_info *pRectInfo);

    /*清除移动侦测结果*/
    void (*svpMd_clearResult)(HiMd_S *pHandle);

    /*手动更新参考帧*/
    int (*svpMd_updateRef)(HiMd_S *pHandle, ot_video_frame_info *pFrameInfo);
};

/**
 * @brief       : 分配移动侦测模块句柄
 * @author      : zhouzirui
 * @param        {HiMdNeedParam_S} stNeedParam：ai必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiMd_S *svpMd_alloc(HiMdNeedParam_S stNeedParam);

/**
 * @brief       : 释放移动侦测模块句柄
 * @author      : zhouzirui
 * @param        {HiMd_S} *pHandle：句柄
 * @return       {*}
 */
void svpMd_release(HiMd_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
