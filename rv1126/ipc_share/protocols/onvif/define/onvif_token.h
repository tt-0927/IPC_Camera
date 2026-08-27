/**
 * @file onvif_token.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-25
 * 
 * @brief onvif 默认token定义 
 */
#pragma once

//TOKEN 名称定义
#define CONFIG_TOKEN_NAME                   "Configuration"
#define VIDEOSOURCE_TOKEN_NAME              "VideoSourceConfigurationToken"
#define VIDEOANALTICS_TOKEN_NAME            "VideoAnalyticsConfigurationToken"
#define RULE_TOKEN_NAME                     "Rule"
#define AUDIOSOURCE_TOKEN_NAME              "AudioSourceConfigurationToken"
#define AUDIOANALTICS_TOKEN_NAME            "VideoSourceConfigurationToken"
#define RECORDJOB_TOKEN_NAME                "RecordingJobToken"

//网络接口token
#define NETWORKINTERFACE_TOKEN  "eth0"

//==========================	媒体 Media Token		==========================
// 媒体能力 name、token
#define PROFILE1_NAME "mainStream"
#define PROFILE2_NAME "subStream"

#define PROFILE1_TOKEN "Profile_1"
#define PROFILE2_TOKEN "Profile_2"

// 视频源配置 Token
// #define PROFILE1_VIDEOSOURCE_NAME "VideoSourceConfig"

// #define PROFILE1_VIDEOSOURCE_TOKEN "VideoSourceToken"
// #define PROFILE2_VIDEOSOURCE_TOKEN "VideoSourceToken"

#define VIDEOSOURCE_NAME "VideoSourceConfig"
#define VIDEOSOURCE_TOKEN "VideoSourceToken"
#define PROFILE1_VIDEOSOURCE_SOURCETOKEN "VideoSource_1"

// 视频编码配置 Token
#define PROFILE1_VIDEOENCODER_NAME "videoEncoder_Name1"
#define PROFILE2_VIDEOENCODER_NAME "videoEncoder_Name2"

#define PROFILE1_VIDEOENCODER_TOKEN "videoEncoder_Token1"
#define PROFILE2_VIDEOENCODER_TOKEN "videoEncoder_Token2"

// 音频编码配置 Token
#define PROFILE_AUDIOSOURCE_NAME "audioSource_Name"
#define PROFILE_AUDIOSOURCE_TOKEN "audioSource_Token"
#define PROFILE_AUDIOSOURCE_SOURCETOKEN "audioSource_SourceToken"
#define PROFILE_AUDIOENCODER_NAME "audioEncoder_Name"
#define PROFILE_AUDIOENCODER_TOKEN "audioEncoder_Token"

//==========================	视频分析 VideoAnalytics Token		==========================
#define VIDEOANALTICS_TOKEN_NUM        1
#define VIDEOANALTICS_TOKEN            "VideoAnalyticsToken"