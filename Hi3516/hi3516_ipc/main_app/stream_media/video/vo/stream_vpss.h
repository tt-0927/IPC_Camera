/**
 * @FilePath     : stream_vpss.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:29:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-22 17:40:38
 * @Description  : VPSS 视频处理
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include "video_define.h"
#include "IpcRet.h"

extern "C"
{
#include "mpp_vpss.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "share_data.h"
}

/* VPSS编码通道号 */
typedef enum
{
    VPSS_CHN_MAIN = 0,
    VPSS_CHN_SUB,
    VPSS_CHN_MAX,
}VPSS_CHN_E;

/**
 * @brief   : stream VPSS初始化 
 * @param    {HiVpss_S} ***pHandle：三维Vpss句柄数组
 * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig：视频编码信息容器
 * @return   {HiVpss_S **}二维Vpss句柄数组：成功 NULL：失败
 */
HiVpss_S **streamVpss_init(HiVpss_S ***pHandle, const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

/**
 * @brief   : stream VPSS去初始化
 */
void streamVpss_uninit();

/**
 * @brief   : stream设置VPSS通道属性
 * @param    {HiVpss_S} *pHandle：句柄
 * @param    {int} nVpssChn：通道号
 * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig：视频编码信息容器
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_set_chnAttr(HiVpss_S *pHandle, int nVpssChn, std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

/**
 * @brief   : stream设置VPSS通道属性
 * @param    {HiVpss_S} *pHandle：句柄
 * @param    {VideoConfig_S} &stVideoConfig：视频编码信息
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_set_chnAttr(HiVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig);

/**
 * @brief   : stream设置VPSS通道裁剪
 * @param    {HiVpss_S} *pHandle：句柄
 * @param    {AreaCrop_S} &stAreaCrop：裁剪信息
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_set_chnCrop(HiVpss_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop);

/**
 * @brief   : 重新设置卷绕
 * @param    {HiVpss_S} *pHandle 句柄
 * @param    {VideoConfig_S} &stVideoConfig 视频编码信息
 * @return   {int} 0：成功 非零：失败
 */
int streamVpss_reset_wrap(HiVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig);
