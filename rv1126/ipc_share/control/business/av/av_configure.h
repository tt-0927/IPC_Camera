/**
 * @FilePath     : av_configure.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-25 20:14:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-03 16:12:48
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
/* 等待 AO 通道缓冲区完全排空的回调函数类型，nTimeoutMs=-1 表示无限等待 */
using WaitAoDrainedCallback = std::function<int(int nChn, int nTimeoutMs)>;

// #if CAP_EVENT_AUDIO_PLAYBACK_V2
/*静音功放输出的回调函数类型（用于播放结束时提前关闭功放，消除结尾噗声）*/
using MuteAudioOutputCallback = std::function<void()>;
// #endif

/*设置ROI配置的回调函数类型*/
using SetVideoRoiConfigCallback = std::function<int(const Video_NS::VideoRoiConfig_S &stConfig)>;
/*设置区域裁剪配置的回调函数类型*/
using SetAreaCropConfigCallback = std::function<int(const Video_NS::AreaCrop_S &stConfig)>;

/**
 * @brief   : AV 视频配置应用接口
 * @note    : 由业务仓库实现，ipc_share 只依赖该抽象接口，避免直接依赖业务仓库的视频模块
 */
class IAVVideoConfigApplier
{
public:
    virtual ~IAVVideoConfigApplier() {}

    /**
     * @brief   : 应用视频配置
     * @param    {Video_NS::VideoConfig_S} &stConfig：视频配置
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_video_config(const Video_NS::VideoConfig_S &stConfig) = 0;

    /**
     * @brief   : 应用视频 ROI 配置
     * @param    {Video_NS::VideoRoiConfig_S} &stConfig：视频 ROI 配置
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_video_roi_config(const Video_NS::VideoRoiConfig_S &stConfig) = 0;

    /**
     * @brief   : 应用区域裁剪配置
     * @param    {Video_NS::AreaCrop_S} &stConfig：区域裁剪配置
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_area_crop_config(const Video_NS::AreaCrop_S &stConfig) = 0;
};

/**
 * @brief   : AV 音频配置应用接口
 * @note    : 由业务仓库实现，ipc_share 只依赖该抽象接口，避免直接依赖业务仓库的音频模块
 */
class IAVAudioConfigApplier
{
public:
    virtual ~IAVAudioConfigApplier() {}

    /**
     * @brief   : 应用音频配置
     * @param    {Audio_NS::AudioConfig_S} &stConfig：音频配置
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_audio_config(const Audio_NS::AudioConfig_S &stConfig) = 0;
};

class CAVConfigure : public CSingleton<CAVConfigure>
{
    CAVConfigure();

public:
    ~CAVConfigure();
    friend class CSingleton<CAVConfigure>;

    /**
     * @brief   : 设置 AV 视频配置应用接口
     * @param    {IAVVideoConfigApplier *} pApplier：业务仓库视频配置应用接口
     * @return   {int} OK：成功，非OK：失败
     */
    int setAVVideoConfigApplier(IAVVideoConfigApplier *pApplier);

    /**
     * @brief   : 清理 AV 视频配置应用接口
     * @param    {IAVVideoConfigApplier *} pApplier：需要清理的业务仓库视频配置应用接口
     * @return   {int} OK：成功，非OK：失败
     */
    int clearAVVideoConfigApplier(IAVVideoConfigApplier *pApplier);

    /**
     * @brief   : 设置 AV 音频配置应用接口
     * @param    {IAVAudioConfigApplier *} pApplier：业务仓库音频配置应用接口
     * @return   {int} OK：成功，非OK：失败
     */
    int setAVAudioConfigApplier(IAVAudioConfigApplier *pApplier);

    /**
     * @brief   : 清理 AV 音频配置应用接口
     * @param    {IAVAudioConfigApplier *} pApplier：需要清理的业务仓库音频配置应用接口
     * @return   {int} OK：成功，非OK：失败
     */
    int clearAVAudioConfigApplier(IAVAudioConfigApplier *pApplier);

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

#if CAP_EVENT_AUDIO_PLAYBACK_V2
    /**
     * @brief   : 通知音频输出立即标记为空闲（用于播放结束时快速停止 keepalive）
     */
    void setAudioAoIdle() const;

    /**
     * @brief   : 立即静音功放输出（关闭 GPIO），消除播放结尾噗声
     * @note    : 在声音联动/对讲/广播播放结束时调用，比 watchdog 2秒超时更快关闭功放
     */
    void muteAudioOutput() const;

    /**
     * @brief   : 设置静音功放输出回调
     * @param    {std::function<void()>} &callback
     */
    void setMuteAudioOutputCallback(const std::function<void()> &callback);
    #endif
    /**
     * @brief   : 设置音频输出空闲回调
     * @param    {std::function<void()>} &callback
     */
    void setAudioAoIdleCallback(const std::function<void()> &callback);

    #if CAP_IO_EXTERNAL_DDR_00S
    /** @brief 立即关闭当前音频输出功放GPIO */
    void muteAudioOutput() const;

    /** @brief 设置关闭音频输出功放的回调 */
    void setMuteAudioOutputCallback(const MuteAudioOutputCallback &callback);
    // #endif
#endif
    /**
     * @brief   : 设置 AO 排空等待回调
     * @param    {WaitAoDrainedCallback} &callback
     * @return   {int} 0：成功 非0：失败
     */
    int setWaitAoDrainedCallback(const WaitAoDrainedCallback &callback);

    /**
     * @brief   : 等待 AO 通道硬件缓冲区完全排空
     * @param    {int} nChn：通道号
     * @param    {int} nTimeoutMs：最大等待时间（ms），-1 表示无限等待
     * @return   {int} 0：已完全排空，非0：超时或错误
     */
    int waitAoDrained(int nChn, int nTimeoutMs) const;

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
    /* AV 视频配置应用接口 */
    IAVVideoConfigApplier *m_pVideoConfigApplier;
    /* AV 音频配置应用接口 */
    IAVAudioConfigApplier *m_pAudioConfigApplier;
    /*音频对讲更新回调*/
    AudioSpeakCallback m_setAoSpeakCallback;
    /* 设置音频模块AO采样率回调 */
    SetAudioAoSampleRateCallback m_setAudioAoSampleRateCallback;

#if CAP_EVENT_AUDIO_PLAYBACK_V2
    std::function<void()> m_setAudioAoIdleCallback;
    /* 静音功放输出回调 */
    std::function<void()> m_muteAudioOutputCallback;
#endif
#if CAP_IO_EXTERNAL_DDR_00S
    /* 静音功放输出回调，V1/V2播放路径共用 */
    MuteAudioOutputCallback m_muteAudioOutputCallback;
#endif
    /* 等待 AO 通道缓冲区完全排空回调 */
    WaitAoDrainedCallback m_waitAoDrainedCallback;
    /* 记录被强制修改前的I帧间隔值 */
    int m_nOriginalIFrameInterval;
    /* 标记I帧间隔是否被强制修改过 */
    bool m_bIFrameIntervalModified;
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
