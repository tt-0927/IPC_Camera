/**
 * @FilePath     : mpp_vgs.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-28 09:43:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-07 11:30:19
 * @Description  : 海思 vgs 模块封装
 */

#ifndef _MPP_VGS_H_
#define _MPP_VGS_H_

#ifdef __cplusplus
extern "C"
{
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
#include "ot_common_vgs.h"
#include "ss_mpi_vgs.h"
#include "mpi_common.h"

#include "ot_buffer.h"
#include "ot_common_vb.h"
#include "ss_mpi_vb.h"
#include "ot_common_sys.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_sys_mem.h"

#include "mpp_sys.h"

/**
 * @brief   : 创建video_frame_info结构
 * @param    {td_u32} u32Width: 图像宽度
 * @param    {td_u32} u32Height: 图像高度
 * @param    {ot_pixel_format} enPixelFormat: 像素格式
 * @param    {ot_video_frame_info} *pFrameInfo: 输出的帧信息指针的指针
 * @return   {int} 成功返回TD_SUCCESS，失败返回错误码
 */
int mppVgs_create_video_frame_info(td_u32 u32Width, td_u32 u32Height, ot_pixel_format enPixelFormat, ot_video_frame_info *pFrameInfo);

/**
 * @brief   : 销毁video_frame_info结构
 * @param    {ot_video_frame_info} *pFrameInfo: 要销毁的帧信息指针
 * @return   {int} 成功返回TD_SUCCESS，失败返回错误码
 */
int mppVgs_destroy_video_frame_info(ot_video_frame_info *pFrameInfo);

/**
 * @brief   : VGS裁剪函数
 * @param    {ot_video_frame_info} *pSrcFrame：源视频帧信息
 * @param    {ot_video_frame_info} *pDstFrame：目标视频帧信息
 * @param    {ot_rect} *pstCropRect：裁剪区域
 * @return   {int} 成功返回TD_SUCCESS，失败返回错误码
 */
int mppVgs_crop(ot_video_frame_info *pSrcFrame, ot_video_frame_info *pDstFrame, ot_rect *pstCropRect);

/**
 * @brief   : VGS缩放函数
 * @param    {ot_video_frame_info} *pSrcFrame：源视频帧信息
 * @param    {ot_video_frame_info} *pDstFrame：目标视频帧信息
 * @return   {int} 成功返回TD_SUCCESS，失败返回错误码
 */
int mppVgs_scale(ot_video_frame_info *pSrcFrame, ot_video_frame_info *pDstFrame);

#ifdef __cplusplus
}
#endif
#endif