/**
 * @FilePath     : mpp_vi.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-19 14:59:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-17 19:24:39
 * @Description  : 海思vi模块封装
 */

#ifndef _MPP_VI_H_
#define _MPP_VI_H_
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
// #include <pthread.h>
// #include "ss_mpi_sys.h"
#include "mpp_sys.h"
#include "ss_mpi_vi.h"
#include "ot_common_video.h"
#include "ot_mipi_rx.h"
#include "ot_common_isp.h"
#include "sample_comm.h"
#include "sensor_common.h"

/*海思 vi必需参数*/
typedef struct _HiViNeedParam_S
{
    /*设备ID*/
    int nDevId;
    /*通道号*/
    int nChannel;
    /*通道设备名*/
    char aEnityName[64];
    int nWidth;
    int nHeight;
    /*图像格式*/
    ot_pixel_format enPixelFormat;
    /*压缩模式*/
    ot_compress_mode enCompressMode;
    /*sensor类型*/
    sample_sns_type sns_type;

} HiViNeedParam_S;

typedef struct _HiViExParam_S
{
    /*管道号*/
    int nPipeId;
    /*帧率控制  默认 -1*/
    int nSrcFrameRate;
    int nDstFrameRate;
    /*获取图像的队列深度*/
    int nDepth;
    /*是否手动dump图像数据*/
    td_bool bDumpEnable;
    /*输出通道的缓冲块数*/
    // int nBufCount;
    /*缩放宽高*/
    int nScaWidth;
    int nScaHeight;
    /*VI模块数据结构*/
    sample_vi_cfg vi_cfg;
    /* 左右翻转 */
    td_bool bMirror;
    /* 上下翻转 */
    td_bool bFlip;
    /* 是否使用sensor翻转 */
    td_bool bUseSensorRollingOver;
} HiViExParam_S;

typedef struct _HiVi_S HiVi_S;
struct _HiVi_S
{
    //info /**********************必需参数***************************/
    HiViNeedParam_S stNeedParam;
    HiViExParam_S stExParam;
    //info /**********************辅助参数***************************/
    // int nFd;
    /*视频缓冲区ID*/
    ot_vb_pool aVbPoolId;
    //info /**********************功能列表***************************/
    /*获取通道采集帧*/
    int (*mppVi_getChnFrame)(HiVi_S *pHandle, int nChn, ot_video_frame_info *pFrameInfo, int nTimeoutMs);

    /*释放通道采集帧*/
    int (*mppVi_releaseChnFrame)(HiVi_S *pHandle, int nChn, ot_video_frame_info *pFrameInfo);

    /*获取管道采集帧*/
    int (*mppVi_getPipeStream)(HiVi_S *pHandle,  ot_video_frame_info *pFrameInfo, int nTimeoutMs);

    /*释放通道采集帧*/
    int (*mppVi_releasePipeStream)(HiVi_S *pHandle,  ot_video_frame_info *pFrameInfo);

    /* 设置上下左右翻转 */
    int (*mppVi_set_mirror_flip)(HiVi_S *pHandle);

    /* 设置sensor上下左右翻转 */
    int (*mppVi_set_sensor_mirror_flip)(HiVi_S *pHandle);

    /*禁用通道*/
    int (*mppVi_disable_chn)(HiVi_S *pHandle);

    /*启用通道*/
    int (*mppVi_enable_chn)(HiVi_S *pHandle);

    /*暂停通道*/
    int (*mppVi_pause_chn)(HiVi_S *pHandle);

    /*恢复通道*/
    int (*mppVi_resume_chn)(HiVi_S *pHandle);

    /*设置vi的属性*/
    int (*mppVi_set_attr)(HiVi_S *pHandle, HiViNeedParam_S stNeedParam, HiViExParam_S stExParam);

    /*初始化vi*/
    int (*mppVi_init)(HiVi_S *pHandle);

    /*反初始化vi*/
    int (*mppVi_uninit)(HiVi_S *pHandle);
};

/**
 * @brief       : 分配vi句柄
 * @author      : zhouzirui
 * @param        {HiViNeedParam_S} stNeedParam：vi必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiVi_S *mppVi_alloc(HiViNeedParam_S stNeedParam);

/**
 * @brief       : 释放vi句柄
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @return       {*}
 */
void mppVi_release(HiVi_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
