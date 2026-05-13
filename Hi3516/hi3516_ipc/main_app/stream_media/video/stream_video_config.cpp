/**
 * @FilePath     : stream_video_config.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-03 17:08:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-16 16:33:11
 * @Description  : 视频流配置管理模块
 */

#include "stream_video_config.h"

CStreamVideoConfig::CStreamVideoConfig()
{
    /* 加载配置文件 */
    loadConfigs();
    /* 加载视频能力集配置文件 */
    loadVideoCapabilitySet();
}

CStreamVideoConfig::~CStreamVideoConfig()
{
}

void CStreamVideoConfig::loadConfigs()
{
    /* 读取视频配置文件 */
    if (Convert::read_file(VIDEO_CONFIG_FILE, m_vstVideoConfig))
    {
        dlog_info("未找到视频配置文件，生成默认配置");
        fill_default_videoConfig();
        saveVideoConfig();
    }

    /*读取视频感兴趣区域配置文件*/
    if (Convert::read_file(ROI_CONFIG_FILE, m_vstVideoRoiConfig))
    {
        dlog_info("未找到ROI配置文件，生成默认配置");
        fill_default_videoRoiConfig();
        saveVideoRoiConfig();
    }

    /*读取区域裁剪配置文件*/
    if (Convert::read_file(AREA_CROP_CONFIG_FILE, m_vstAreaCropConfig))
    {
        dlog_info("未找到区域裁剪配置文件，生成默认配置");
        fill_default_areaCropConfig();
        saveAreaCropConfig();
    }
}

void CStreamVideoConfig::loadVideoCapabilitySet()
{
    Video_NS::VideoCapabilitySet_S stCapabilitySet;
    if (Convert::read_file(VIDEO_CAPABILITY_SET_FILE, stCapabilitySet))
    {
        dlog_info("未找到视频能力集文件，生成默认配置");
        fill_default_capabilitySet(stCapabilitySet);
        Convert::write_file(VIDEO_CAPABILITY_SET_FILE, stCapabilitySet);
    }
}

const std::vector<Video_NS::VideoConfig_S> &CStreamVideoConfig::getVideoConfigs() const
{
    return m_vstVideoConfig;
}

const std::vector<Video_NS::VideoRoiConfig_S> &CStreamVideoConfig::getVideoRoiConfigs() const
{
    return m_vstVideoRoiConfig;
}

const std::vector<Video_NS::AreaCrop_S> &CStreamVideoConfig::getAreaCropConfigs() const
{
    return m_vstAreaCropConfig;
}

Video_NS::VideoConfig_S &CStreamVideoConfig::getVideoConfigRef(int nChn)
{
    return m_vstVideoConfig.at(nChn);
}

Video_NS::VideoRoiConfig_S &CStreamVideoConfig::getVideoRoiConfigRef(int nChn)
{
    return m_vstVideoRoiConfig.at(nChn);
}

Video_NS::AreaCrop_S &CStreamVideoConfig::getAreaCropConfigRef(int nChn)
{
    return m_vstAreaCropConfig.at(nChn);
}

void CStreamVideoConfig::updateVideoConfig(const Video_NS::VideoConfig_S &stConfig)
{
    m_vstVideoConfig.at(stConfig.nId) = stConfig;
}

void CStreamVideoConfig::updateVideoRoiConfig(const Video_NS::VideoRoiConfig_S &stConfig)
{
    m_vstVideoRoiConfig.at(stConfig.nId) = stConfig;
}

void CStreamVideoConfig::updateAreaCropConfig(const Video_NS::AreaCrop_S &stConfig)
{
    m_vstAreaCropConfig.at(stConfig.nId) = stConfig;
}

void CStreamVideoConfig::saveVideoConfig()
{
    Convert::write_file(VIDEO_CONFIG_FILE, m_vstVideoConfig);
}

void CStreamVideoConfig::saveVideoRoiConfig()
{
    Convert::write_file(ROI_CONFIG_FILE, m_vstVideoRoiConfig);
}

void CStreamVideoConfig::saveAreaCropConfig()
{
    Convert::write_file(AREA_CROP_CONFIG_FILE, m_vstAreaCropConfig);
}

void CStreamVideoConfig::fill_default_videoConfig()
{
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
    stVideoConfigMain.enVideoCodec = VideoCodec_E::H265;
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
    stVideoConfigSub.enVideoCodec = VideoCodec_E::H265;
    stVideoConfigSub.bSmartEnable = false;
    stVideoConfigSub.enEncodingComplexity = EncodingComplexity_E::Main;
    stVideoConfigSub.nIFrameInterval = 50;
    stVideoConfigSub.enSvcEnable = SvcMode_E::SVC_MODE_DISABLE;
    stVideoConfigSub.nBitrateSmoothing = 50;

    m_vstVideoConfig.clear();
    m_vstVideoConfig.emplace_back(stVideoConfigMain);
    m_vstVideoConfig.emplace_back(stVideoConfigSub);
    /* 抓拍JPEG编码通道 */
    stVideoConfigSub.nId = 2;
    m_vstVideoConfig.emplace_back(stVideoConfigSub);
}

void CStreamVideoConfig::fill_default_videoRoiConfig()
{
    using namespace Video_NS;
    m_vstVideoRoiConfig.clear();
    /* 设置默认视频感兴趣区域配置 */
    VideoRoiConfig_S stVideoRoiConfig = VideoRoiConfig_S::CreateWithDefaultRule();
    for (size_t i = 0; i < m_vstVideoConfig.size() - 1; i++)
    {
        stVideoRoiConfig.nId = i;
        m_vstVideoRoiConfig.emplace_back(stVideoRoiConfig);
    }
}

void CStreamVideoConfig::fill_default_areaCropConfig()
{
    using namespace Video_NS;
    m_vstAreaCropConfig.clear();

    Video_NS::VideoCapabilitySet_S stVideoCapabilitySet;
    fill_default_capabilitySet(stVideoCapabilitySet);
    /* 设置默认视频区域裁剪配置 */
    for (size_t i = 0; i < m_vstVideoConfig.size() - 1; i++)
    {
        Video_NS::AreaCrop_S stConfig;
        stConfig.nId = i;
        /* 默认裁剪分辨率为视频能力集中最小的分辨率 */
        if (i == 0)
        {
            stConfig.stResolution.parse_string(stVideoCapabilitySet.stMain.aResolution.back().strName);
        }
        else
        {
            stConfig.stResolution.parse_string(stVideoCapabilitySet.stSub.aResolution.back().strName);
        }
        stConfig.stRect.nWidth = stConfig.stResolution.nWidth;
        stConfig.stRect.nHeight = stConfig.stResolution.nHeight;
        /* 转换区域坐标。裁剪区域的坐标系，在文件及配置中以插件的分辨率来保存坐标 */
        stConfig.stRect.ConvertResolution(m_vstVideoConfig[i].stVideoResolution.nWidth,
                                          m_vstVideoConfig[i].stVideoResolution.nHeight,
                                          PLUG_IN_WIDTH_DEFAULT,
                                          PLUG_IN_HEIGHT_DEFAULT);
        m_vstAreaCropConfig.emplace_back(stConfig);
    }
}

void CStreamVideoConfig::fill_default_capabilitySet(Video_NS::VideoCapabilitySet_S &stCapabilitySet)
{
    /* 主码流 */
    auto &main = stCapabilitySet.stMain;
    main.nId = 0;
    main.bSupportMultiStream = 1;

    /* 添加主码流支持的编码格式 */
    main.addEncodeAbility("H.264", 1, { 0, 1, 2 }, 1, 1);
    main.addEncodeAbility("H.265", 0, { 1 }, 1, 1);
    main.addEncodeAbility("MJPEG");
    // main.addEncodeAbility("SVAC", 0, { 1 }, 1, 1);

    main.nEncodeTypeNum = main.aEncodeAbility.size();

    /* 添加主码流支持的分辨率 */
    const std::vector<std::pair<int, int>> mainResolutions = {
        {PIXEL_WIDTH_2_5K, PIXEL_HEIGHT_2_5K},
        {  PIXEL_WIDTH_2K,   PIXEL_HEIGHT_2K},
        {PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080},
        // {PIXEL_WIDTH_1280,  PIXEL_HEIGHT_960},
        // {PIXEL_WIDTH_1280,  PIXEL_HEIGHT_720}
    };
    for (const auto &res : mainResolutions)
    {
        main.addResolution(res.first, res.second);
    }
    main.nResolutionNum = main.aResolution.size();

    /* 子码流 */
    auto &sub = stCapabilitySet.stSub;
    sub.nId = 1;
    sub.bSupportMultiStream = 1;

    /* 添加子码流支持的编码格式 */
    sub.addEncodeAbility("H.264", 1, { 0, 1, 2 }, 1, 1);
    sub.addEncodeAbility("H.265", 0, { 1 }, 1, 1);
    // sub.addEncodeAbility("MJPEG");
    // sub.addEncodeAbility("SVAC", 0, { 1 }, 1, 1);

    sub.nEncodeTypeNum = sub.aEncodeAbility.size();

    /* 添加子码流支持的分辨率 */
    const std::vector<std::pair<int, int>> subResolutions = {
        {PIXEL_WIDTH_704, PIXEL_HEIGHT_576},
        {PIXEL_WIDTH_640, PIXEL_HEIGHT_480},
        { PIXEL_WIDTH_352, PIXEL_HEIGHT_288}
        // {PIXEL_WIDTH_1280, PIXEL_HEIGHT_720},
        // {PIXEL_WIDTH_1024, PIXEL_HEIGHT_576},
        // { PIXEL_WIDTH_960, PIXEL_HEIGHT_540},
        // { PIXEL_WIDTH_640, PIXEL_HEIGHT_480}
    };

    for (const auto &res : subResolutions)
    {
        sub.addResolution(res.first, res.second);
    }
    sub.nResolutionNum = sub.aResolution.size();
}
