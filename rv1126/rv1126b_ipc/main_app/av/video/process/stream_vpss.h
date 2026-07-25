/*
 * @FilePath     : stream_vpss.h
 * @Author       : zhouzirui
 * @Date         : 2024-09-26 11:23:24
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-11-11 20:23:16
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

#include "rockit_vpss.h"
#include "dlog.h"
#include "share_data.h"
#include "share_define.h"

}

/* VPSS编码通道号 */
typedef enum
{
    VPSS_CHN_MAIN = 0,
    VPSS_CHN_SUB,
    VPSS_CHN_MAX,
}VPSS_CHN_E;


/* stream vpss帧数据结构体 */
typedef struct _STREAMVPSS_S
{
    /*通道号*/
    int channel;
    /*帧数据*/
    unsigned char* framedata;
    /*帧数据大小*/
    int framesize;

    VIDEO_FRAME_INFO_S pstVideoFrame;
    
}StreamVpssFrame_t;


/**
 * @brief       : stream 视频处理⼦系统初始化
 * @author      : zhouzirui
 * @param        {RkVpss_S} * 三维Vpss句柄数组
 * @param        {VencConfig_S} stVencConfig 视频编码信息
 * @return       {*}二维Vpss句柄数组：成功 NULL：失败
 */
RkVpss_S **streamVpss_init(RkVpss_S ***pHandle, const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);
/**
 * @brief       : stream 视频处理⼦系统去初始化
 * @author      : zhouzirui
 * @return       {*}
 */
void streamVpss_uninit();


/**
 * @brief   : stream 视频处理⼦系统设置通道属性
 * @param    {RkVpss_S} *pHandle：句柄
 * @param    {int} nVpssChn：通道号
 * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig：视频编码信息容器
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_set_chnAttr(RkVpss_S *pHandle, int nVpssChn, std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

/**
 * @brief   : stream 视频处理⼦系统设置通道属性
 * @param    {RkVpss_S} *pHandle：句柄
 * @param    {VideoConfig_S} &stVideoConfig：视频编码信息
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_set_chnAttr(RkVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig);


/**
 * @brief   : stream 视频处理⼦系统获取一帧数据
 * @param    {RkVpss_S} *pHandle：句柄
 * @param    (StreamVpssFrame_t}   stVpssFrame 帧数据结构体
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_get_chnFrame(RkVpss_S *pHandle,StreamVpssFrame_t* stVpssFrame);

/**
 * @brief   : stream 视频处理⼦系统释放帧数据
 * @param    {RkVpss_S} *pHandle：句柄
 * @param    (StreamVpssFrame_t}   stVpssFrame 帧数据结构体
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_release_chnFrame(RkVpss_S *pHandle,StreamVpssFrame_t* stVpssFrame);

/**
 * @brief   : stream设置VPSS通道裁剪
 * @param    {RkVpss_S} *pHandle：句柄
 * @param    {AreaCrop_S} &stAreaCrop：裁剪信息
 * @return   {int}0：成功 非零：失败
 */
int streamVpss_set_chnCrop(RkVpss_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop);

