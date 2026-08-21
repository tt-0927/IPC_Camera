/**
 * @FilePath     : video_frame_jpeg_encoder.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-29 13:50:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 15:03:25
 * @Description  : 视频帧 JPEG 编码公共工具
 */

#pragma once

#include <string>

#include "ot_common_video.h"
#include "event_tvsdk_payload.h"

namespace AiAppCommon
{
/**
 * @brief   : 将 MPP 视频帧编码为 JPEG 文件
 * @param    {ot_video_frame_info} *pFrameInfo：待编码的视频帧
 * @param    {std::string} &strFilename：输出 JPEG 文件路径
 * @return   {int} 0：成功，非0：失败
 */
int encode_video_frame_to_jpeg_file(ot_video_frame_info *pFrameInfo, const std::string &strFilename);

/**
 * @brief   : 将 MPP 视频帧编码为 JPEG 内存数据
 * @param    {ot_video_frame_info} *pFrameInfo：待编码的视频帧
 * @param    {EventTvSdkImage_S} &stImage：输出图片结构（填充 vecJpeg/nWidth/nHeight）
 * @return   {int} 0：成功，非0：失败
 * @note    : 内部使用临时文件中转，调用方无需关心文件清理
 */
int encode_video_frame_to_jpeg_memory(ot_video_frame_info *pFrameInfo, EventTvSdkImage_S &stImage);

/**
 * @brief   : 判断当前是否需要编码事件全景图（TVSDK告警图）
 * @return   {bool} true：需要编码，false：不需要（无TVSDK客户端订阅）
 * @note    : 软件JPEG编码代价高昂（每帧重建FFmpeg上下文），
 *            事件报警帧编码前先调用本接口做门控，避免无订阅者时空转CPU；
 *            非ENABLE_TVSDK_SRC编译配置下恒返回false
 */
bool tvsdk_event_image_required();
} // namespace AiAppCommon
