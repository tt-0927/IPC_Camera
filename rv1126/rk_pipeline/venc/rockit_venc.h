/*************************************************************************
	> File Name: rockit_venc.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月11日 星期三 16时16分04秒
    > LastEditors  : leiyy
    > LastEditTime : 2025-08-08 14:27:54
    > Description  : RK VENC 视频编码

 ************************************************************************/

#ifndef _ROCKIT_VENC_H
#define _ROCKIT_VENC_H

#ifdef __cplusplus
extern "C" {    
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

#include "mpi_common.h"

#include "rk_debug.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_cal.h"

/* 编码支持最大ROI个数 */
#define VENC_MAX_ROI_NUM 8
/*venc码率控制器统计时间默认值*/
#define VENC_RT_STAT_TIME_DEFAULT (3)

/* VENC编码通道号 */
typedef enum
{
    RK_VENC_CHN_MAIN = 0,
    RK_VENC_CHN_SUB,
    RK_VENC_CHN_JPEG,
    RK_VENC_CHN_MAX,
} RK_VENC_CHN_E;

typedef struct _RkVenc RkVenc_S;

/*编码的必须参数*/
typedef struct _RkVencNeedParam
{
    unsigned int unWidth;
    unsigned int unHeight;
    unsigned int unVirWidth;
    unsigned int unVirHeight;
    /*图像格式*/
    PIXEL_FORMAT_E enPixFormat;
    /*编码协议类型*/
    RK_CODEC_ID_E enCodec;
    /*GOP值*/
    int nGop;
    /*GOP类型*/
    VENC_GOP_MODE_E enGopMode;
    /*编码通道*/
    int nChn;
    /*输入帧率*/
    int nInFrameRate;
    /*输出帧率*/
    int nOutFrameRate;

    /*压缩模式 默认不压缩*/
    COMPRESS_MODE_E enCompressMode;
    /*强制触发IDR标志*/
    bool bForceIDR;
} RkVencNeedParam_S;

/*编码功能参数*/
typedef struct _RkVencExparam
{
    /*码流模式 默认定码率码率*/
    VENC_RC_MODE_E enRcMode;
    /*内存块数 默认4块*/
    unsigned unMbCnt;
    /*编码等级 */
    int nProfile;
    /*智能编码使能*/
    RK_BOOL bSvcEnable;
    /*图像质量*/
    int nImageQuality;
    /*码流平滑 [ 清晰<->平滑 ]*/
    int nBitrateSmoothing;
    /*编码帧数 默认-1*/
    int nSnapPicCount;
    /*码流大小*/
    int nBitRate;
    /*最大码率 动码率和可变码率用*/
    int nMaxBitRate;
    /*最小码率 动码率和可变码率用*/
    int nMinBitRate;
    /*平均码率*/
    int nAverageBitrate;
    /*--------------高级码率控制-------------------*/
    /*第一帧步进值*/
    int nFirstQp;
    /*最大步进设置*/
    int nStepQp;
    /*P帧最大qp设置*/
    int nMaxQp;
    /*P帧最小qp设置*/
    int nMinQp;
    /*I帧最大qp设置*/
    int nMaxIQp;
    /*I帧最大qp设置*/
    int nMinIQp;
    /*I帧前几帧P帧平均QP与I帧的差值*/
    int nDeltIpQp;
    /*--------------end-------------------*/

    /*编码感兴趣区域信息数组*/
    RK_BOOL bRoiEnable;
    /*编码感兴趣区域信息数组*/
    VENC_ROI_ATTR_S astRoiAttr[VENC_MAX_ROI_NUM];

    /* JPEG/MJPEG格式 品质因数 */
    unsigned int u32Qfactor;
    /* MJPEG格式 最大品质因数 */
    unsigned int u32MaxQfactor;
    /* MJPEG格式 最小品质因数 */
    unsigned int u32MinQfactor;
    /* SLICE分割 */
    VENC_SLICE_SPLIT_S stSliceSplit;
    /* 一张图像的大小,这个也是创建内存池一块大小 默认 0内部计算 */
    unsigned int unBufferSize;
    /* 输出内存池使用用户模式 默认 RK_FALSE */
    RK_BOOL bAttachPool;

} RkVencExParam_S;

struct _RkVenc
{
    // info /**********************必须参数***************************/
    RkVencNeedParam_S stNeedParam;

    // info /**********************功能参数***************************/
    RkVencExParam_S stExParam;

    // info /**********************辅助参数***************************/
    /* 输入内存池 */
    MB_POOL vencPoolInput;
    /* 输出内存池 */
    MB_POOL vencPoolOutput;

    // info /**********************功能列表***************************/

    /* venc初始化 */
    int (*rockitVenc_init)(RkVenc_S *pHandle);

    /* venc去初始化 */
    int (*rockitVenc_unInit)(RkVenc_S *pHandle);

    /*发送编码数据*/
    int (*rockitVenc_send_frame)(RkVenc_S *pHandle, void *pParam, int nSize, void *(*send_pic)(void *pData, void *pParam, int nSize));

    /*发送原始图像进行编码*/
    int (*rockitVenc_send_VFrame)(RkVenc_S *pHandle, VIDEO_FRAME_INFO_S *pVFrame, int nTimeOutMs);

    /*获取编码码流*/
    int (*rockitVenc_get_stream)(RkVenc_S *pHandle, VENC_STREAM_S *pFrame, VENC_PACK_S *pPackArray, uint32_t nPackCount, int nTimeOutMs);

    /*获取码流的虚拟地址*/
    uint8_t *(*rockitVenc_get_streamVirdata)(VENC_PACK_S *pPack);

    /*释放码流缓存*/
    int (*rockitVenc_release_stream)(RkVenc_S *pHandle, VENC_STREAM_S *stFrame);

    /*获取编码通道GOP*/
    int (*rockitVenc_get_gop)(RkVenc_S *pHandle, unsigned int *uGop);

    /*设置编码通道GOP*/
    int (*rockitVenc_set_gop)(RkVenc_S *pHandle, unsigned int uGop);

    /*设置h264/h265编码通道的感兴趣区域编码配置*/
    int (*rockitVenc_set_roiAttr)(RkVenc_S *pHandle, VENC_ROI_ATTR_S *pRoiAttr);

    /*设置通道编码裁剪缩放或者裁剪 x y w h 要2字节对齐，否则不成功* */
    int (*rockitVenc_set_corpOrScale)(RkVenc_S *pHandle, VENC_CROP_TYPE_E enCropType, RECT_S *pstCorp, VENC_SCALE_RECT_S *pstScale);

    /* 请求 IDR 帧 */
    int (*rockitVenc_request_idr)(RkVenc_S *pHandle);
};

/**
 * @brief   : 分配VENC句柄
 * @param    {RkVencNeedParam_S} stParam VENC必须参数
 * @return   {RkVenc_S *} 成功返回句柄，失败返回NULL
 */
RkVenc_S *rockitVenc_alloc(RkVencNeedParam_S stParam);

/**
 * @brief   : 释放VENC句柄
 * @param    {RkVenc_S} *pHandle VENC句柄
 */
void rockitVenc_release(RkVenc_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
