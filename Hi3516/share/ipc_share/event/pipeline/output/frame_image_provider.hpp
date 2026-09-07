/**
 * @FilePath     : frame_image_provider.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : AI 帧图片能力抽象端口，实现位于平台业务仓库
 */

#pragma once

#include "detection_types.hpp"
#include "event_alarm/statistics/event_statistics_report.hpp"

namespace AiPipeline_NS
{
/* 帧图片同步端口：平台实现负责 VGS/JPEG 等厂商能力，共享处理器只依赖本接口 */
class IFrameImageProvider
{
public:
    /**
     * @brief   : 析构帧图片端口
     * @return   {void}
     */
    virtual ~IFrameImageProvider() = default;

    /**
     * @brief   : 构建当前帧全景图
     * @param    {EventStatistics_NS::ImagePayload_S &} stImage：输出图片负载
     * @return   {int} OK：成功，非 OK：失败
     */
    virtual int build_panorama(EventStatistics_NS::ImagePayload_S &stImage) = 0;

    /**
     * @brief   : 按归一化目标框构建目标裁剪图
     * @param    {const NormalizedRect_S &} stRect：归一化目标框
     * @param    {EventStatistics_NS::ImagePayload_S &} stImage：输出图片负载
     * @return   {int} OK：成功，非 OK：失败
     */
    virtual int build_target(const NormalizedRect_S &stRect, EventStatistics_NS::ImagePayload_S &stImage) = 0;
};
} // namespace AiPipeline_NS
