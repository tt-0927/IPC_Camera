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

#include "common_define.h"
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
 * @brief   : 按检测区域编码事件图（TVSDK 全景图），VENC 硬编码优先
 * @param    {ot_video_frame_info} *pSrcFrameInfo：送算法的原始源帧（事件图优先从它出）
 * @param    {ot_video_frame_info} *pAlgoFrame：算法分辨率准备帧，源帧缺失时兜底整帧编码
 * @param    {const Common::Rect_S} &stDetectRect：检测区域（算法坐标系）
 * @param    {int} nDetectWidth：算法坐标系宽（如移动侦测 1024）
 * @param    {int} nDetectHeight：算法坐标系高（如移动侦测 576）
 * @param    {EventTvSdkImage_S} &stImage：输出图片（填充 vecJpeg/nWidth/nHeight）
 * @return   {int} 0：成功，非0：失败
 * @note    : 区域覆盖全屏时源帧整帧硬编码（1920x1080 走 VENC 全景图通道）；
 *            局部区域先等比映射回源帧坐标，再 encodeCrops 硬件抠图（对齐
 *            x 8 / y 2、最小 32x32）；硬编码条件不满足时回退源帧整幅编码
 */
int encode_event_image_to_jpeg_memory(ot_video_frame_info *pSrcFrameInfo,
                                      ot_video_frame_info *pAlgoFrame,
                                      const Common::Rect_S &stDetectRect,
                                      int nDetectWidth,
                                      int nDetectHeight,
                                      EventTvSdkImage_S &stImage);

/**
 * @brief   : 判断当前是否需要编码事件全景图（TVSDK告警图）
 * @return   {bool} true：需要编码，false：不需要（无TVSDK客户端订阅）
 * @note    : 软件JPEG编码代价高昂（每帧重建FFmpeg上下文），
 *            事件报警帧编码前先调用本接口做门控，避免无订阅者时空转CPU；
 *            非ENABLE_TVSDK_SRC编译配置下恒返回false
 */
bool tvsdk_event_image_required();
} // namespace AiAppCommon
