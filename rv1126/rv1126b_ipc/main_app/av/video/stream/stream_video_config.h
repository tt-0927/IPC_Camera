/**
 * @FilePath     : stream_video_config.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-03 16:26:51
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-03 19:19:52
 * @Description  : 视频流配置管理模块头文件
 */
#pragma once

#include <vector>
#include <string>
#include "dlog.h"
#include "share_data.h"
#include "av_configure.h"

/**
 * @brief   : 视频流配置管理类
 */
class CStreamVideoConfig
{
public:
    CStreamVideoConfig();
    ~CStreamVideoConfig();

    /**
     * @brief   : 获取视频配置，非const 
     * @return   {vector<Video_NS::VideoConfig_S>} 视频配置
     */
    std::vector<Video_NS::VideoConfig_S>& getVideoConfigs();

    /**
     * @brief   : 获取视频配置
     * @return   {vector<Video_NS::VideoConfig_S>} 视频配置
     */
    const std::vector<Video_NS::VideoConfig_S> &getVideoConfigs() const;

    /**
     * @brief   : 获取视频感兴趣配置
     * @return   {vector<Video_NS::VideoRoiConfig_S>} 视频感兴趣配置
     */
    const std::vector<Video_NS::VideoRoiConfig_S> &getVideoRoiConfigs() const;

    /**
     * @brief   : 获取视频区域裁剪配置
     * @return   {vector<Video_NS::AreaCrop_S>} 视频区域裁剪配置
     */
    const std::vector<Video_NS::AreaCrop_S> &getAreaCropConfigs() const;

    /**
     * @brief   : 获取指定通道的视频配置
     * @param    {int} nChn 通道号
     * @return   {Video_NS::VideoConfig_S} 指定通道的视频配置
     */
    Video_NS::VideoConfig_S &getVideoConfigRef(int nChn);

    /**
     * @brief   : 获取指定通道的视频感兴趣配置
     * @param    {int} nChn 通道号
     * @return   {Video_NS::VideoRoiConfig_S} 指定通道的视频感兴趣配置
     */
    Video_NS::VideoRoiConfig_S &getVideoRoiConfigRef(int nChn);

    /**
     * @brief   : 获取指定通道的视频区域裁剪配置
     * @param    {int} nChn 通道号
     * @return   {Video_NS::AreaCrop_S} 指定通道的视频区域裁剪配置
     */
    Video_NS::AreaCrop_S &getAreaCropConfigRef(int nChn);

    /**
     * @brief   : 更新指定通道的视频配置
     * @param    {VideoConfig_S} &stConfig 视频配置
     */
    void updateVideoConfig(const Video_NS::VideoConfig_S &stConfig);

    /**
     * @brief   : 更新指定通道的视频感兴趣配置
     * @param    {VideoRoiConfig_S} &stConfig 视频感兴趣配置
     */
    void updateVideoRoiConfig(const Video_NS::VideoRoiConfig_S &stConfig);

    /**
     * @brief   : 更新指定通道的视频区域裁剪配置
     * @param    {AreaCrop_S} &stConfig 视频区域裁剪配置
     */
    void updateAreaCropConfig(const Video_NS::AreaCrop_S &stConfig);

    /**
     * @brief   : 保存视频配置至指定路径
     */
    void saveVideoConfig();

    /**
     * @brief   : 保存视频感兴趣配置至指定路径
     */
    void saveVideoRoiConfig();

    /**
     * @brief   : 保存视频区域裁剪配置至指定路径
     */
    void saveAreaCropConfig();

private:
    /**
     * @brief   : 加载配置文件
     * @note    : 从指定路径下读取，不存在则创建默认配置文件
     */
    void loadConfigs();

    /**
     * @brief   : 加载视频能力集配置文件
     */
    void loadVideoCapabilitySet();

    /**
     * @brief   : 填充默认视频配置
     */
    void fill_default_videoConfig();

    /**
     * @brief   : 填充默认视频感兴趣配置
     */
    void fill_default_videoRoiConfig();

    /**
     * @brief   : 填充默认视频区域裁剪配置
     */
    void fill_default_areaCropConfig();

    /**
     * @brief   : 填充默认视频能力集配置
     * @param    {VideoCapabilitySet_S} &stCapabilitySet 视频能力集配置
     */
    void fill_default_capabilitySet(Video_NS::VideoCapabilitySet_S &stCapabilitySet);

private:
    /*视频配置*/
    std::vector<Video_NS::VideoConfig_S> m_vstVideoConfig;
    /*视频感兴趣区域配置*/
    std::vector<Video_NS::VideoRoiConfig_S> m_vstVideoRoiConfig;
    /*区域裁剪配置*/
    std::vector<Video_NS::AreaCrop_S> m_vstAreaCropConfig;
};
