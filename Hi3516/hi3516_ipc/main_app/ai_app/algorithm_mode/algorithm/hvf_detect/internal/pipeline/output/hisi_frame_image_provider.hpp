/**
 * @FilePath     : hisi_frame_image_provider.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 海思原始帧图片能力实现，持有 Frame Lease 并负责 VGS/JPEG
 */

#pragma once

#include <memory>

#include "frame_image_provider.hpp"
#include "ot_common_video.h"

namespace HVFDetectInternal
{
/* 海思原始帧图片 Provider：图片编码结束前持有共享帧引用，防止 VPSS 帧被归还 */
class CHisiFrameImageProvider : public AiPipeline_NS::IFrameImageProvider
{
public:
    /**
     * @brief   : 构造海思帧图片 Provider
     */
    CHisiFrameImageProvider() = default;

    /**
     * @brief   : 设置当前帧 Frame Lease
     * @param    {const std::shared_ptr<ot_video_frame_info> &} pFrameInfo：当前帧共享引用
     * @return   {void}
     * @note    : memory: 帧所有权仍归调用方，本对象临时持有共享引用形成短生命周期 Lease
     */
    void set_frame(const std::shared_ptr<ot_video_frame_info> &pFrameInfo);

    /**
     * @brief   : 清空 Frame Lease
     * @return   {void}
     */
    void clear_frame();

    /**
     * @brief   : 构建当前帧全景图
     * @param    {EventStatistics_NS::ImagePayload_S &} stImage：输出图片负载
     * @return   {int} OK：成功，ERR：编码失败
     */
    int build_panorama(EventStatistics_NS::ImagePayload_S &stImage) override;

    /**
     * @brief   : 按归一化目标框构建目标裁剪图
     * @param    {const AiPipeline_NS::NormalizedRect_S &} stRect：归一化目标框
     * @param    {EventStatistics_NS::ImagePayload_S &} stImage：输出图片负载
     * @return   {int} OK：成功，ERR：裁剪或编码失败
     * @note    : 以较长边为基准扩大 1.5 倍，以中心点裁剪正方形，VGS 16×4 对齐
     */
    int build_target(const AiPipeline_NS::NormalizedRect_S &stRect, EventStatistics_NS::ImagePayload_S &stImage) override;

private:
    /* 当前帧共享引用，图片编码期间持有 */
    std::shared_ptr<ot_video_frame_info> m_pFrameInfo;
};
} // namespace HVFDetectInternal
