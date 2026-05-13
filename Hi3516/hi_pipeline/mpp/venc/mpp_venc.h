/**
 * @FilePath     : mpp_venc.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-20 15:50:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-15 16:41:01
 * @Description  : 海思venc模块封装
 */

#ifndef _MPP_VENC_H_
#define _MPP_VENC_H_

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
#include <sys/select.h>

#include "ot_common_video.h"
#include "ss_mpi_venc.h"
#include "securec.h"

/*venc码率控制器统计时间默认值*/
#define VENC_RT_STATS_TIME_DEFAULT (2)
/*venc码率控制器vbv虚拟缓存时间默认值 取值范围[1,100]*/
#define VENC_RT_VBV_BUF_DELAY (50)

typedef struct  _HiVenc  HiVenc_S;

/*编码的必须参数*/
typedef struct _HiVencNeedParam
{
    unsigned int unWidth;
    unsigned int unHeight;
    unsigned int unVirWidth;
    unsigned int unVirHeight;
    /*图像格式*/
    ot_pixel_format enPixFormat;
    /*编码协议类型*/
    ot_payload_type enCodec;
    /*GOP值*/
    int nGop;
    /*GOP类型*/
    ot_venc_gop_mode enGopMode;
    /*编码通道*/
    int nChn;
    /*输入帧率*/
    int nInFrameRate;
    /*输出帧率*/
    int nOutFrameRate;

    /*压缩模式 默认不压缩*/
    // ot_compress_mode enCompressMode;
    /*是否开启卷绕*/
    td_bool bWrapEnable;
} HiVencNeedParam_S;

/*编码功能参数*/
typedef struct _HiVencExparam
{
    /*码流模式 默认定码率码率*/
    ot_venc_rc_mode enRcMode;
    /*编码等级 */
    int nProfile;
    /*智能编码使能*/
    td_bool bSvcEnable;
    /*图像质量*/
    int nImageQuality;
    /*码流平滑 [ 清晰<->平滑 ]*/
    int nBitrateSmoothing;
    /*码率大小*/
    int nBitRate;
    /*最大码率 动码率和可变码率用*/
    int nMaxBitRate;
    /*最小码率 动码率和可变码率用*/
    int nMinBitRate;
    /*平均码率*/
    int nAverageBitrate;
    /*--------------高级码率控制-------------------*/
    unsigned int i_qp;
    unsigned int p_qp;
    unsigned int b_qp;
    /*--------------end-------------------*/
    /*编码感兴趣区域信息数组*/
    ot_venc_roi_attr astRoiAttr[OT_VENC_MAX_ROI_NUM];
    /* JPEG格式 品质因数 */
    unsigned int uQFactor;
} HiVencExParam_S;

struct _HiVenc
{
    //info /**********************必需参数***************************/
    HiVencNeedParam_S stNeedParam;
    //info /**********************功能参数***************************/
    HiVencExParam_S stExParam;
    //info /**********************辅助参数***************************/
    /*编码通道设备描述符*/
    int nVencFd;
    //info /**********************函数列表***************************/
    /*初始化编码句柄*/
    int (*mppVenc_init)(HiVenc_S *pHandle);

    /*反初始化编码*/
    int (*mppVenc_unInit)(HiVenc_S *pHandle);

    /*发送原始图像进行编码*/
    int (*mppVenc_send_stream)(HiVenc_S *pHandle, const ot_video_frame_info *pFrameInfo, int nTimeoutMs);

    /*获取编码码流*/
    int (*mppVenc_get_stream)(HiVenc_S *pHandle, ot_venc_stream *pStream, int nTimeoutMs);

    /*释放码流缓存*/
    int (*mppVenc_release_stream)(HiVenc_S *pHandle, ot_venc_stream *pStream);

    /*请求IDR帧*/
    int (*mppVenc_request_idr)(HiVenc_S *pHandle, td_bool bInstant);

    /*设置感兴趣编码区域属性*/
    int (*mppVenc_set_roi_attr)(HiVenc_S *pHandle, ot_venc_roi_attr *pRoiAttr);

    /*获取感兴趣编码区域属性*/
    int (*mppVenc_get_roi_attr)(HiVenc_S *pHandle, ot_venc_roi_attr *pRoiAttr);

    /*设置编码通道码率控制器的高级参数*/
    int (*mppVenc_set_rc_param)(HiVenc_S *pHandle);

    /*获取编码通道属性*/
    int (*mppVenc_get_chn_attr)(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr);

    /*设置编码通道属性*/
    int (*mppVenc_set_chn_attr)(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr);

    /*重新设置编码通道属性*/
    int (*mppVenc_reset_chn_attr)(HiVenc_S *pHandle);

    /*重新设置编码动态属性*/
    int (*mppVenc_reset_attr)(HiVenc_S *pHandle);

    /* 设置通道截取Crop参数 */
    int (*mppVenc_set_chn_crop)(HiVenc_S *pHandle, ot_crop_info stCropInfo);
};

/**
 * @brief       : 分配venc句柄
 * @author      : zhouzirui
 * @param        {HiVencNeedParam_S} stParam
 * @return       {*}成功为HiVenc_S句柄，失败为null
 */
HiVenc_S* mppVenc_alloc( HiVencNeedParam_S stParam);

/**
 * @brief       : 释放venc句柄
 * @author      : zhouzirui
 * @param        {HiVenc_S*} pHandle
 * @return       {*}
 */
void mppVenc_release(HiVenc_S* pHandle);

#ifdef __cplusplus
}
#endif
#endif
