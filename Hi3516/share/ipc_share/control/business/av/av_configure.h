/**
 * @FilePath     : av_configure.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-25 20:14:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-16 16:17:24
 * @Description  : 音视频配置
 */

#pragma once

#include <functional>
#include <memory>
#include "Singleton.h"
#include "config_storage.h"
#include "config_storage_v2.h"
#include "video_define.h"
#include "audio_define.h"
#include "IpcRet.h"

/*设置视频配置的回调函数类型*/
using SetVideoConfigCallback = std::function<int(const Video_NS::VideoConfig_S &stVideoConfig)>;
/*设置音频配置的回调函数类型*/
using SetAudioConfigCallback = std::function<int(const Audio_NS::AudioConfig_S &stAudioConfig)>;
/* 设置音频模块AO采样率的回调函数类型 */
using SetAudioAoSampleRateCallback = std::function<int(const Audio_NS::AudioSamprate_E enSampRate)>;
/* 音频对讲的回调函数类型 */
using AudioSpeakCallback = std::function<int(const Audio_NS::AoInfo_S &stAoInfo)>;
/*设置ROI配置的回调函数类型*/
using SetVideoRoiConfigCallback = std::function<int(const Video_NS::VideoRoiConfig_S &stConfig)>;
/*设置区域裁剪配置的回调函数类型*/
using SetAreaCropConfigCallback = std::function<int(const Video_NS::AreaCrop_S &stConfig)>;

class CAVConfigure : public CSingleton<CAVConfigure>
{
    CAVConfigure();

public:
    ~CAVConfigure();
    friend class CSingleton<CAVConfigure>;

    // info /*----------------------- 视频配置相关接口 -----------------------*/
    /**
     * @brief 设置视频配置回调
     * @param callback 配置回调函数
     */
    void setVideoConfigCallback(const SetVideoConfigCallback &callback);

    /**
     * @brief   : 更新视频配置
     * @param    {VideoConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int update_configure(const Video_NS::VideoConfig_S &data);

    /**
     * @brief   : 设置视频配置
     * @param    {VideoConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Video_NS::VideoConfig_S &data);

    /**
     * @brief   : 获取视频配置
     * @param    {VideoConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Video_NS::VideoConfig_S &data) const;

    /**
     * @brief   : 获取视频配置
     * @param    {VideoConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(std::set<Video_NS::VideoConfig_S> &data) const;

    /**
     * @brief 设置视频ROI配置回调
     * @param callback 配置回调函数
     */
    void setVideoRoiConfigCallback(const SetVideoRoiConfigCallback &callback);

    /**
     * @brief   : 设置视频ROI配置
     * @param    {VideoRoiConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Video_NS::VideoRoiConfig_S &data);

    /**
     * @brief   : 获取视频ROI配置
     * @param    {VideoRoiConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Video_NS::VideoRoiConfig_S &data) const;

    /**
     * @brief   : 获取视频ROI配置
     * @param    {VideoRoiConfig_S} &data：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(std::set<Video_NS::VideoRoiConfig_S> &data) const;

    /**
     * @brief 设置视频ROI配置回调
     * @param callback 配置回调函数
     */
    void setAreaCropConfigCallback(const SetAreaCropConfigCallback &callback);

    /**
     * @brief   : 设置区域裁剪配置
     * @param    {AreaCrop_S} &data：区域裁剪配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Video_NS::AreaCrop_S &data);

    /**
     * @brief   : 获取区域裁剪配置
     * @param    {AreaCrop_S} &data：区域裁剪配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Video_NS::AreaCrop_S &data) const;

    /**
     * @brief   : 获取区域裁剪配置
     * @param    {AreaCrop_S} &data：区域裁剪配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(std::set<Video_NS::AreaCrop_S> &data) const;

    /**
     * @brief   : 设置视频能力集
     * @param    {VideoCapabilitySet_S} &data 视频能力集
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Video_NS::VideoCapabilitySet_S &data);

    /**
     * @brief   : 获取视频能力集
     * @param    {VideoCapabilitySet_S} &data 视频能力集
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Video_NS::VideoCapabilitySet_S &data) const;

    // info /*----------------------- 音频配置相关接口 -----------------------*/
    /**
     * @brief 设置音频配置回调
     * @param callback 配置回调函数
     */
    void setAudioConfigCallback(const SetAudioConfigCallback &callback);

    /**
     * @brief   : 设置音频配置
     * @param    {AudioConfig_S} &data：音频配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Audio_NS::AudioConfig_S &data);

    /**
     * @brief   : 获取音频配置
     * @param    {AudioConfig_S} &data：音频配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Audio_NS::AudioConfig_S &data) const;

    /**
     * @brief   : 设置音频能力集
     * @param    {AudioCapabilitySet_S} &data 音频能力集
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Audio_NS::AudioCapabilitySet_S &data);

    /**
     * @brief   : 获取音频能力集
     * @param    {AudioCapabilitySet_S} &data 音频能力集
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Audio_NS::AudioCapabilitySet_S &data) const;

    /**
     * @brief 对讲数据回调
     * @param callback 配置回调函数
     */
    void setAoSpeakCallback(const AudioSpeakCallback &callback);

    /**
     * @brief 设置对讲信息
     * @param data ao信息
     */
    void setAoSpeakInfo(const Audio_NS::AoInfo_S &data) const;

    /**
     * @brief   : 设置音频模块AO采样率回调
     * @param    {SetAudioAoSampleRateCallback} &callback 回调函数
     * @return   {int} 0：成功 非0：失败
     */
    int setAudioAoSampleRateCallback(const SetAudioAoSampleRateCallback &callback);

    /**
     * @brief   : 设置音频模块AO采样率
     * @param    {AudioSamprate_E} enSampRate 采样率
     * @return   {int} 0：成功 非0：失败
     */
    int setAudioAoSampleRate(const Audio_NS::AudioSamprate_E enSampRate) const;

private:
    /**
     * @brief 校验音频配置是否在能力集范围内
     * @param data 音频配置
     * @return true 合法
     * @return false 非法
     */
    bool is_audio_config_supported(const Audio_NS::AudioConfig_S &data) const;

    /**
     * @brief 填充默认音频能力集
     * @param data 音频能力集
     */
    void fill_default_audio_capability_set(Audio_NS::AudioCapabilitySet_S &data) const;

    /*视频配置更新回调*/
    SetVideoConfigCallback m_setVideoConfigCallback;
    /*视频ROI配置更新回调*/
    SetVideoRoiConfigCallback m_setVideoRoiConfigCallback;
    /*区域裁剪配置更新回调*/
    SetAreaCropConfigCallback m_setAreaCropConfigCallback;
    /*音频配置更新回调*/
    SetAudioConfigCallback m_setAudioConfigCallback;
    /*音频对讲更新回调*/
    AudioSpeakCallback m_setAoSpeakCallback;
    /* 设置音频模块AO采样率回调 */
    SetAudioAoSampleRateCallback m_setAudioAoSampleRateCallback;
    /*视频配置*/
    VersionedConfigStorage<Video_NS::VideoConfig_S> m_videoConfig;
    /*音频配置*/
    ConfigStorage<Audio_NS::AudioConfig_S, StorageType_E::Single> m_audioConfig;
    /*音频能力集*/
    ConfigStorage<Audio_NS::AudioCapabilitySet_S, StorageType_E::Single> m_audioCapabilitySet;
    /*视频ROI配置*/
    VersionedConfigStorage<Video_NS::VideoRoiConfig_S> m_videoRoiConfig;
    /*区域裁剪配置*/
    VersionedConfigStorage<Video_NS::AreaCrop_S> m_areaCropConfig;
    /*视频能力集*/
    ConfigStorage<Video_NS::VideoCapabilitySet_S, StorageType_E::Single> m_capabilitySet;
};
