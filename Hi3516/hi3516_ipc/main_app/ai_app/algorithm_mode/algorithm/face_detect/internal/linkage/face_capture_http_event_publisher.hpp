/**
 * @FilePath     : face_capture_http_event_publisher.hpp
 * @Description  : Face capture HTTP event publisher
 */

#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "algorithm.hpp"
#include "face_capture_types.hpp"

namespace FaceDetectInternal
{
/**
 * @brief   : 人脸抓拍 HTTP 事件推送器
 */
class CFaceCaptureHttpEventPublisher
{
public:
    /* 构建抓拍全景图 JPEG 数据回调 */
    using BuildPanoramaImageFunc = std::function<bool(ot_video_frame_info *, std::vector<unsigned char> &)>;
    /* 构建抓拍目标小图 JPEG 数据回调 */
    using BuildTargetImageFunc = std::function<bool(const Common::RectInfo_S &,
                                                    ot_video_frame_info *,
                                                    size_t,
                                                    std::vector<unsigned char> &)>;

    /**
     * @brief   : 判断人脸抓拍 HTTP 推送是否启用
     * @return   {bool} true：启用 false：未启用
     */
    bool isEnabled() const;

    /**
     * @brief   : 通过 HTTP 推送人脸抓拍结果
     * @param    {std::vector<FaceCaptureTarget_S>} &vecTargets：抓拍目标列表
     * @param    {ot_video_frame_info} *pFrameInfo：当前视频帧
     * @param    {int} nChnId：通道号
     * @param    {BuildPanoramaImageFunc} &fnBuildPanoramaImage：全景图构建回调
     * @param    {BuildTargetImageFunc} &fnBuildTargetImage：目标小图构建回调
     * @return   {bool} true：已加入 HTTP 推送队列 false：未推送
     */
    bool publish(const std::vector<FaceCaptureTarget_S> &vecTargets,
                 ot_video_frame_info *pFrameInfo,
                 int nChnId,
                 const BuildPanoramaImageFunc &fnBuildPanoramaImage,
                 const BuildTargetImageFunc &fnBuildTargetImage) const;
};
} // namespace FaceDetectInternal
