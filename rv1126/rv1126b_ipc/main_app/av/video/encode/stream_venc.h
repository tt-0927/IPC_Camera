/**
 * @FilePath     : stream_venc.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-09-26 13:45:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-10 20:48:13
 * @Description  : VENC 视频编码
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include "video_define.h"

extern "C"
{
#include "rockit_venc.h"
#include "share_data.h"
}

/* VENC编码通道号 */
typedef enum
{
    VENC_CHN_MAIN = 0,
    VENC_CHN_SUB,
    VENC_CHN_JPEG,
    VENC_CHN_MAX,
} VENC_CHN_E;

/**
 * @brief       : 编码初始化
 * @author      : zhouzirui
 * @param        {int} nChannel 编码通道号
 * @param        {VencConfig_S} stuVencConfig 视频编码信息
 * @return       {RkVenc_S *} 句柄：成功 NULL：失败
 */
RkVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief       : 编码初始化
 * @param        {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息
 * @return       {RkVenc_S *} 句柄：成功 NULL：失败
 */
RkVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig);

/**
 * @brief       : 编码反初始化
 * @author      : zhouzirui
 * @param        {RkVenc_S*} pHandle 句柄
 * @return       {int} 0：成功 非零：失败
 */
int streamVenc_uninit(RkVenc_S *pHandle);

/**
 * @brief   : 编码重置
 * @param    {RkVenc_S} *pHandle：句柄
 * @param    {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息容器
 * @param    {vector<Video_NS::VideoRoiConfig_S>} &vstVideoRoiConfig：视频感兴趣区域配置容器
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_reset(RkVenc_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief   : 编码属性重置
 * @param    {RkVenc_S} *pHandle：句柄
 * @param    {Video_NS::VideoConfig_S} &stVideoConfig：视频编码信息容器
 * @param    {vector<Video_NS::VideoRoiConfig_S>} &vstVideoRoiConfig：视频感兴趣区域配置容器
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_reset_attr(RkVenc_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief   : 设置感兴趣编码区域属性
 * @param    {RkVenc_S} *pHandle：句柄
 * @param    {vector<Video_NS::VideoRoiConfig_S>} &vstVideoRoiConfig：视频感兴趣区域配置容器
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_set_roi_attr(RkVenc_S *pHandle, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig);

/**
 * @brief   : stream设置VENC通道裁剪
 * @param    {RkVenc_S} *pHandle：句柄
 * @param    {AreaCrop_S} &stAreaCrop：裁剪信息
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_set_chnCrop(RkVenc_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop);

/**
 * @brief   : stream设置VENC通道裁剪缩放
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {AreaCrop_S} &stAreaCrop 裁剪信息
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_set_chnCropScale(RkVenc_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop);

/**
 * @brief   : 发送数据编码
 * @author  : zhouzirui
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VIDEO_FRAME_INFO_S} *pFrame 编码数据
 * @param    {int} nTimeOutMs 超时时间  -1：阻塞接⼝；0：⾮阻塞接⼝；⼤于0：超时等待时间。单位为毫秒（ms）
 * @return   {int} 0：成功 非零：失败
 */
int streamVenc_send_frame(RkVenc_S *pHandle, VIDEO_FRAME_INFO_S *pFrame, int nTimeOutMs);
