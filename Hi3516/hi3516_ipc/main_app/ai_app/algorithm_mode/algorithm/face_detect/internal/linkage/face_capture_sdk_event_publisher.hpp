/**
 * @FilePath     : face_capture_sdk_event_publisher.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:30:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:20:53
 * @Description  : 人脸抓拍 SDK 事件推送器
 */

#pragma once

#include <functional>
#include <vector>

#include "algo_image_event_publisher.hpp"
#include "algorithm.hpp"
#include "face_capture_types.hpp"

namespace FaceDetectInternal
{
class CFaceCaptureSdkEventPublisher : public AiAppCommon::CAlgoImageEventPublisher
{
public:
    /* 构建 SDK 全景图回调，避免公共推送器直接依赖处理器内部编码实现 */
    using BuildPanoramaImageFunc = std::function<bool(ot_video_frame_info *, std::vector<unsigned char> &)>;
    /* 构建 SDK 目标小图回调，按目标框和批次序号生成 JPEG 数据 */
    using BuildTargetImageFunc = std::function<bool(const Common::RectInfo_S &,
                                                    ot_video_frame_info *,
                                                    size_t,
                                                    std::vector<unsigned char> &)>;

    /**
     * @brief   : 推送人脸抓拍 SDK 图片事件
     * @param    {std::vector<FaceCaptureTarget_S>} &vecTargets：人脸抓拍目标列表
     * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
     * @param    {int} nChnId：通道号
     * @param    {BuildPanoramaImageFunc} &fnBuildPanoramaImage：全景图构建回调
     * @param    {BuildTargetImageFunc} &fnBuildTargetImage：目标小图构建回调
     * @return   {void}
     */
    void publish(const std::vector<FaceCaptureTarget_S> &vecTargets,
                 ot_video_frame_info *pFrameInfo,
                 int nChnId,
                 long long llTimestampMs,
                 const BuildPanoramaImageFunc &fnBuildPanoramaImage,
                 const BuildTargetImageFunc &fnBuildTargetImage);
};
} // namespace FaceDetectInternal
