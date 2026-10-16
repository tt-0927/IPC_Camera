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

/**
 * @brief   : VGS批量统计若干矩形区域的亮度总和（一次job，硬件完成，不拷贝像素）
 * @param    {ot_video_frame_info} *pSrcFrame：源视频帧信息（只需输入图）
 * @param    {ot_rect} *pstLumaRects：亮度统计区域数组
 * @param    {td_u32} u32Count：区域数量，范围[1,100]
 * @param    {td_u64} *pu64LumaSum：输出，第i个区域的亮度总和，除以区域像素数得平均亮度
 * @return   {int} 成功返回TD_SUCCESS，失败返回错误码
 * @note    : 区域起点与宽高须2像素对齐，尺寸范围[6x6, 4096x4096]（CV610），
 *            且必须在输入图像内；任一区域不合法即整体失败
 */
int mppVgs_luma(ot_video_frame_info *pSrcFrame, const ot_rect *pstLumaRects, td_u32 u32Count, td_u64 *pu64LumaSum);

#ifdef __cplusplus
}
#endif
#endif