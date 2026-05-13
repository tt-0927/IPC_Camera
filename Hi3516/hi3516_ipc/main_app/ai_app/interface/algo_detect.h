/**
 * @FilePath     : algo_detect.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 17:49:18
 * @Description  : AI_APP 初始化和通讯转发
 */

#pragma once

#include "ot_common_video.h"
#include "ot_common_aio.h"

//info /*----------------------- 初始化、去初始化接口 -----------------------*/
/**
 * @brief 初始化接口
 * @return int 
 */
/**
 * @brief   : 初始化接口 AI_APP
 * @return   {int}0：成功，非0：失败
 */
int algo_detect_init();

/**
 * @brief   : 去初始化接口 AI_APP
 * @return   {int}0：成功，非0：失败
 */
int algo_detect_deinit();

//info /*----------------------- 通讯转发接口 -----------------------*/
/**
 * @brief   : 发送视频数据到 AI_APP
 * @param    {ot_video_frame_info} *pFrameInfo 视频图像帧信息
 */
void algo_send_videoStreamData(const ot_video_frame_info *pFrameInfo);

/**
 * @brief   : 发送视频数据到 AI_APP
 * @param    {void*} pData 视频图像帧数据指针
 * @param    {int} nLength 字节数
 * @param    {int} nWidth 图像宽度
 * @param    {int} nHeight 图像高度
 */
void algo_send_videoStreamData(const void *pData, int nLength, int nWidth, int nHeight);

/**
 * @brief   : 发送音频数据到 AI_APP
 * @param    {ot_audio_frame} *pFrame 音频帧信息
 */
void algo_send_audioStreamData(const ot_audio_frame *pFrame);

/**
 * @brief   : 发送音频数据到 AI_APP
 * @param    {void*} pData 音频帧数据指针
 * @param    {int} nLength 字节数
 */
void algo_send_audioStreamData(const void *pData, int nLength);

/**
 * @brief   : 接受 control 命令到 AI_APP
 * @param    {int} nCode
 * @param    {char*} strData
 */
void algo_send_controlData(int nCode, const char *strData = nullptr, void *pData = nullptr);
