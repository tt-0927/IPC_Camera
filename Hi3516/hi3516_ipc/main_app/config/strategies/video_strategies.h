/**
 * @FilePath     : video_strategies.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 11:14:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-30 11:21:45
 * @Description  : 视频配置策略
 */
 
#pragma once

#include "config_strategy.h"
#include "video_define.h"
#include "dlog.h"

using VecVideoConfig = std::vector<Video_NS::VideoConfig_S>;

/**
* @brief   : 默认/未知型号的视频配置策略
* @note    : 当产品型号未知或不匹配时，保底配置
*/
class CVideoStrategyDefault : public CConfigStrategy<VecVideoConfig>
{
public:
    void fillDefault(VecVideoConfig &config) override
    {
        dlog_info("填充保底的默认视频配置...");
        using namespace Video_NS;
        /* 设置默认主码流视频配置 */
        VideoConfig_S stVideoConfigMain;
        stVideoConfigMain.nId = 0;
        stVideoConfigMain.enVideoType = VideoType_E::COMPOSITE_STREAM;
        stVideoConfigMain.stVideoResolution.nWidth = PIXEL_WIDTH_1920;
        stVideoConfigMain.stVideoResolution.nHeight = PIXEL_HEIGHT_1080;
        stVideoConfigMain.enBitrateType = BitrateType_E::CBR;
        stVideoConfigMain.enImageQuality = ImageQuality_E::MEDIUM;
        stVideoConfigMain.enFrameRate = FRAME_RATE_30;
        stVideoConfigMain.nBitrateUpperLimit = 4096;
        stVideoConfigMain.nAverageBitrate = 2048;
        stVideoConfigMain.enVideoCodec = VideoCodec_E::H264;
        stVideoConfigMain.bSmartEnable = false;
        stVideoConfigMain.enEncodingComplexity = EncodingComplexity_E::Main;
        stVideoConfigMain.nIFrameInterval = 50;
        stVideoConfigMain.enSvcEnable = SvcMode_E::SVC_MODE_DISABLE;
        stVideoConfigMain.nBitrateSmoothing = 50;

        /* 设置默认子码流视频配置 */
        VideoConfig_S stVideoConfigSub;
        stVideoConfigSub.nId = 1;
        stVideoConfigSub.enVideoType = VideoType_E::COMPOSITE_STREAM;
        stVideoConfigSub.stVideoResolution.nWidth = PIXEL_WIDTH_704;
        stVideoConfigSub.stVideoResolution.nHeight = PIXEL_HEIGHT_576;
        stVideoConfigSub.enBitrateType = BitrateType_E::CBR;
        stVideoConfigSub.enImageQuality = ImageQuality_E::MEDIUM;
        stVideoConfigSub.enFrameRate = FRAME_RATE_30;
        stVideoConfigSub.nBitrateUpperLimit = 1024;
        stVideoConfigSub.nAverageBitrate = 512;
        stVideoConfigSub.enVideoCodec = VideoCodec_E::H264;
        stVideoConfigSub.bSmartEnable = false;
        stVideoConfigSub.enEncodingComplexity = EncodingComplexity_E::Main;
        stVideoConfigSub.nIFrameInterval = 50;
        stVideoConfigSub.enSvcEnable = SvcMode_E::SVC_MODE_DISABLE;
        stVideoConfigSub.nBitrateSmoothing = 50;

        config.clear();
        config.emplace_back(stVideoConfigMain);
        config.emplace_back(stVideoConfigSub);
    }

    void customize(VecVideoConfig &config) override
    {
        dlog_info("无特殊定制化。");
    }
};

/**
* @brief   : 产品型号TV_3852T的视频配置策略
* @note    : 型号TV_3852T
*/
class CVideoStrategy_TV_3852T : public CConfigStrategy<VecVideoConfig>
{
public:
    void fillDefault(VecVideoConfig &config) override
    {
        dlog_info("填充型号TV_3852T的默认视频配置...");
        using namespace Video_NS;
        /* 设置默认主码流视频配置 */
        VideoConfig_S stVideoConfigMain;
        stVideoConfigMain.nId = 0;
        stVideoConfigMain.enVideoType = VideoType_E::COMPOSITE_STREAM;
        stVideoConfigMain.stVideoResolution.nWidth = PIXEL_WIDTH_2_5K;
        stVideoConfigMain.stVideoResolution.nHeight = PIXEL_HEIGHT_2_5K;
        stVideoConfigMain.enBitrateType = BitrateType_E::CBR;
        stVideoConfigMain.enImageQuality = ImageQuality_E::MEDIUM;
        stVideoConfigMain.enFrameRate = FRAME_RATE_30;
        stVideoConfigMain.nBitrateUpperLimit = 4096;
        stVideoConfigMain.nAverageBitrate = 2048;
        stVideoConfigMain.enVideoCodec = VideoCodec_E::H264;
        stVideoConfigMain.bSmartEnable = false;
        stVideoConfigMain.enEncodingComplexity = EncodingComplexity_E::Main;
        stVideoConfigMain.nIFrameInterval = 50;
        stVideoConfigMain.enSvcEnable = SvcMode_E::SVC_MODE_DISABLE;
        stVideoConfigMain.nBitrateSmoothing = 50;

        /* 设置默认子码流视频配置 */
        VideoConfig_S stVideoConfigSub;
        stVideoConfigSub.nId = 1;
        stVideoConfigSub.enVideoType = VideoType_E::COMPOSITE_STREAM;
        stVideoConfigSub.stVideoResolution.nWidth = PIXEL_WIDTH_704;
        stVideoConfigSub.stVideoResolution.nHeight = PIXEL_HEIGHT_576;
        stVideoConfigSub.enBitrateType = BitrateType_E::CBR;
        stVideoConfigSub.enImageQuality = ImageQuality_E::MEDIUM;
        stVideoConfigSub.enFrameRate = FRAME_RATE_30;
        stVideoConfigSub.nBitrateUpperLimit = 1024;
        stVideoConfigSub.nAverageBitrate = 512;
        stVideoConfigSub.enVideoCodec = VideoCodec_E::H264;
        stVideoConfigSub.bSmartEnable = false;
        stVideoConfigSub.enEncodingComplexity = EncodingComplexity_E::Main;
        stVideoConfigSub.nIFrameInterval = 50;
        stVideoConfigSub.enSvcEnable = SvcMode_E::SVC_MODE_DISABLE;
        stVideoConfigSub.nBitrateSmoothing = 50;

        config.clear();
        config.emplace_back(stVideoConfigMain);
        config.emplace_back(stVideoConfigSub);
    }

    void customize(VecVideoConfig &config) override
    {
        dlog_info("定制化型号TV_3852T的视频配置...");
    }
};
