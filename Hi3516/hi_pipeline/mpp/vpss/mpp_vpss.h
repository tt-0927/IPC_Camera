/**
 * @FilePath     : mpp_vpss.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-20 10:56:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-22 09:25:20
 * @Description  : 海思vpss模块封装
 */

#ifndef _MPP_VPSS_H_
#define _MPP_VPSS_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include "ot_common_video.h"
#include "ss_mpi_vpss.h"
#include "mpp_sys.h"
#include "ot_buffer.h"
#include "sample_comm.h"

typedef struct _HiVpssGrpAttr
{
    /*图像格式*/
    ot_pixel_format enGrpPixelFormat;
    /*压缩模式*/
    ot_compress_mode enGrpComMode;

    /*vpss通道处理图像最大宽高*/
    int nMaxW;
    int nMaxH;
    /*输入源和目标帧率*/
    int nSrcFrameRate;
    int nDstFrameRate;
} HiVpssGrpAttr_S;

typedef struct _HiVpssChnAttr
{
    /*图像格式*/
    ot_pixel_format enChnPixelFormat;
    /*压缩模式*/
    ot_compress_mode enChnComMode;
    /*通道工作模式，自动模式或者用户设置模式
    通道设置为AUTO模式时，支持与VO、 VENC或PCIV模块绑定。
    VO设置为Single模式时，也支持，但不推荐VO为Single模式下使用AUTO模式。
    通道设置为AUTO模式时，只能与一个接收者绑定。只有工作在离线模式下，通道才支持AUTO模式。*/
    ot_vpss_chn_mode enVpssChnMode;
    /*通道目标图像宽高*/
    int nWidth;
    int nHeight;
    /* vpss通道处理图像最大宽高 */
    int nMaxWidth;
    int nMaxHeight;

    /* 存储深度 如果需要手动获取帧，必须>0 */
    int nDepth;
    // int nFrameBufCnt;
    /*输入源和目标帧率*/
    int nSrcFrameRate;
    int nDstFrameRate;
    /*是否开启卷绕*/
    td_bool bWrapEnable;
    /* 是否开启低延时 */
    td_bool bLowDelay;
    /* 是否开启低延时单Buffer模式 */
    td_bool bOneBufEn;
    /* 左右翻转 */
    td_bool bMirror;
    /* 上下翻转 */
    td_bool bFlip;
    /* 是否强制指定小码流尺寸 */
    td_bool bSmallStreamSize;
    /* 强制指定的小码流尺寸图像宽高 */
    int nSmallStreamWidth;
    int nSmallStreamHeight;
} HiVpssChnAttr_S;

typedef struct _HiVpssNeedParam
{
    /*组*/
    int nVpssGrp;

    /*每组下的通道总数*/
    int nVpssChnSum;

    /*组属性*/
    HiVpssGrpAttr_S stVpssGrpAttr;

    /*通道属性，默认4个，实际初始化数量以nVpssChnSum为准*/
    HiVpssChnAttr_S astVpssChnAttr[OT_VPSS_MAX_CHN_NUM];
} HiVpssNeedParam_S;

typedef struct _HiVpss HiVpss_S;

struct _HiVpss
{
    //info /**********************必需参数***************************/
    /*组*/
    int nVpssGrp;

    /*每组下的通道总数*/
    int nVpssChnSum;

    /*组属性*/
    HiVpssGrpAttr_S stVpssGrpAttr;

    /*通道属性，默认4个，实际初始化数量以nVpssChnSum为准*/
    HiVpssChnAttr_S astVpssChnAttr[OT_VPSS_MAX_CHN_NUM];

    /*视频缓冲区ID*/
    ot_vb_pool aVbPoolId[OT_VPSS_MAX_CHN_NUM];

    //info /**********************可选参数***************************/

    //info /**********************功能列表***************************/
    /*vpss初始化*/
    int (*mppVpss_init) (HiVpss_S *pHandle);

    /*vpss反初始化*/
    int (*mppVpss_uninit) (HiVpss_S *pHandle);

    /*设置通道属性,缩放比例，通道模式*/
    int (*mppVpss_set_chnAttr)(HiVpss_S *pHandle, HiVpssChnAttr_S *pChnAttr, int nVpssChn);

    /*获取通道属性,缩放比例，通道模式*/
    int (*mppVpss_get_chnAttr)(HiVpss_S *pHandle, HiVpssChnAttr_S *pChnAttr, int nVpssChn);

    /* 设置通道裁剪 */
    int (*mppVpss_set_chnCrop)(HiVpss_S *pHandle, ot_vpss_crop_info *pCropInfo, int nVpssChn);

    /* 设置VPSS通道低延时 */
    int (*mppVpss_set_chnLowdelay)(HiVpss_S *pHandle, int nChn, td_bool bEnable, td_bool bOneBufEn);

    /* 重新设置卷绕 */
    int (*mppVpss_reset_wrap)(HiVpss_S *pHandle, int nWidth, int nHeight);

    /*获取通道一帧图像*/
    int (*mppVpss_get_chnFrame)(HiVpss_S *pHandle, int nVpssChn, ot_video_frame_info *pFrameInfo, int nTimeoutMs);

    /*释放通道一帧图像*/
    int (*mppVpss_release_chnFrame)(HiVpss_S *pHandle, int nVpssChn, ot_video_frame_info *pFrameInfo);

    /*从Group获取一帧原始图像*/
    int (*mppVpss_get_grpFrame)(HiVpss_S *pHandle, ot_video_frame_info *pFrameInfo, int nTimeoutMs);

    /*释放一帧原始图像*/
    int (*mppVpss_release_grpFrame)(HiVpss_S *pHandle, ot_video_frame_info *pFrameInfo);
};

/**
 * @brief       : 分配vpss句柄
 * @author      : zhouzirui
 * @param        {HiVpssNeedParam_S} stNeedParam 必须参数
 * @return       {*}成功为HiVpss_S句柄，失败为null
 */
HiVpss_S *mppVpss_alloc(HiVpssNeedParam_S stNeedParam);

/**
 * @brief       : 释放vpss句柄
 * @author      : zhouzirui
 * @param        {HiVpss_S*} pHandle 句柄
 * @return       {*}
 */
void mppVpss_release(HiVpss_S* pHandle);

#ifdef __cplusplus
}
#endif
#endif //_MPP_VPSS_H_
