/**
 * @FilePath     : stream_vpss.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:29:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-30 15:14:08
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
 * @param    {ot_vpss_crop_info *} pstAppliedCrop：输出底层实际生效的裁剪参数，可为空
 * @return   {int}0：成功 非零：失败
 * @note    : 裁剪起点与尺寸会按底层约束对齐，调用方必须使用输出参数进行后续坐标换算。
 */
int streamVpss_set_chnCrop(HiVpss_S *pHandle,
                            const Video_NS::AreaCrop_S &stAreaCrop,
                            ot_vpss_crop_info *pstAppliedCrop = nullptr);

/**
 * @brief   : 获取 VPSS 通道当前实际生效的裁剪参数
 * @param    {HiVpss_S *} pHandle：VPSS 句柄
 * @param    {int} nVpssChn：VPSS 通道号
 * @param    {ot_vpss_crop_info &} stCropInfo：输出的底层裁剪参数
 * @return   {int} OK：成功，ERR：失败
 * @note    : 用于让 OSD 坐标转换严格采用底层已生效的对齐坐标，而非网页原始配置。
 */
int streamVpss_get_chnCrop(HiVpss_S *pHandle, int nVpssChn, ot_vpss_crop_info &stCropInfo);

/**
 * @brief   : 重新设置卷绕
 * @param    {HiVpss_S} *pHandle 句柄
 * @param    {VideoConfig_S} &stVideoConfig 视频编码信息
 * @return   {int} 0：成功 非零：失败
 */
int streamVpss_reset_wrap(HiVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig);
