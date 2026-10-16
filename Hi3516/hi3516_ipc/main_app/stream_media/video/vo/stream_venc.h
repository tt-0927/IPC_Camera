/**
 * @FilePath     : stream_venc.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:29:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-05 14:07:16
 * @Description  : VENC 视频编码
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include "video_define.h"
#include "IpcRet.h"

extern "C"
{
#include "mpp_venc.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "share_data.h"
}

/* VENC编码通道号 */
typedef enum
{
    VENC_CHN_MAIN = 0,
    VENC_CHN_SUB,
    VENC_CHN_JPEG,
    /* 用户态 JPEG 按需编码通道（不绑定 VPSS，batch_crop 全景/抠图共用，见 CJpegEncodeService）。
       不参与常驻通道初始化/取流线程/OSD 遍历，生命周期由编码服务自管 */
    VENC_CHN_JPEG_USER,
    VENC_CHN_MAX,
    /* 常驻绑定通道数（MAIN/SUB/JPEG），供初始化与遍历循环做上界 */
    VENC_CHN_PERSISTENT_CNT = VENC_CHN_JPEG + 1,
}VENC_CHN_E;

/**
 * @brief       : 编码初始化
 * @author      : zhouzirui
 * @param        {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息
 * @param        {vector<Video_NS::VideoRoiConfig_S>} &vstVideoRoiConfig：视频感兴趣区域配置容器
 * @return       {HiVenc_S *} 句柄：成功 NULL：失败
 */
HiVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief       : 编码初始化
 * @param        {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息
 * @return       {HiVenc_S *} 句柄：成功 NULL：失败
 */
HiVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig);

/**
 * @brief       : 编码反初始化
 * @author      : zhouzirui
 * @param        {HiVenc_S*} pHandle：句柄
 * @return       {int}0：成功 非零：失败
 */
int streamVenc_uninit(HiVenc_S *&pHandle);

/**
 * @brief   : 编码重置
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息容器
 * @param    {vector<Video_NS::VideoRoiConfig_S>} &vstVideoRoiConfig：视频感兴趣区域配置容器
 * @return   {int}0：成功 非零：失败
 */
int streamVenc_reset(HiVenc_S *&pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief   : 编码属性重置
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息容器
 * @param    {vector<Video_NS::VideoRoiConfig_S>} &vstVideoRoiConfig：视频感兴趣区域配置容器
 * @return   {int}0：成功 非零：失败
 */
int streamVenc_reset_attr(HiVenc_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief   : 设置感兴趣编码区域属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {VideoRoiConfig_S} &stVideoRoiConfig：视频感兴趣区域配置
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_set_roi_attr(HiVenc_S *pHandle, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief   : stream设置VENC裁剪
 * @param    {HiVenc_S} *pHandle 句柄
 * @param    {AreaCrop_S} &stAreaCrop 裁剪信息
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_set_crop(HiVenc_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop);
