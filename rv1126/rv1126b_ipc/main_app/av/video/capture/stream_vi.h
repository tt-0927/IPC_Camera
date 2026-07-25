/**
 * @FilePath     : stream_vi.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-12-06 13:52:42
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-04 15:37:26
 * @Description  : VI 视频采集输入
 */

#pragma once
#include "dlog.h"

extern "C"
{
#include "rockit_vi.h"
#include "rockit_bind.h"
}

/* 通道设备名 */
#define VI_CHN_VIDEO_DEV "/dev/video13"
/* GDC文件路径 */
#define VI_GDCFILE_PATH  "/oem/usr/etc/iqfiles/sc850sl_CMK-OT2115-PC1_ldc.ini"

/**
 * @brief   : 视频输入采集初始化
 * @return   {RkVi_S *} NULL：失败 非空：句柄
 */
RkVi_S *streamVi_init();

/**
 * @brief   : 视频输入采集去初始化
 * @param    {RkVi_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
int streamVi_uninit(RkVi_S *pHandle);
