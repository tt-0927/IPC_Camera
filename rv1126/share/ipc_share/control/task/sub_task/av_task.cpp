/**
 * @FilePath     : av_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-26 11:05:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-09 16:05:18
 * @Description  : 音视频任务
 */


#include "av_task.h"
#include "convert_interface.h"
#include "av_configure.h"
#include "action_code.h"

/*获取视频配置*/
void Task::AV::GetVideoConfig::handle()
{
    std::set<Video_NS::VideoConfig_S> stVideoConfig;
    Convert::to_struct(m_taskData, stVideoConfig);
    CAVConfigure::instance()->get_configure(stVideoConfig);
    result(Convert::to_string(stVideoConfig));
}

/*设置视频配置*/
void Task::AV::SetVideoConfig::handle()
{
    Video_NS::VideoConfig_S stVideoConfig;
    Convert::to_struct(m_taskData, stVideoConfig);
    int nRet = CAVConfigure::instance()->set_configure(stVideoConfig);
    result(nRet);
}

/*获取视频能力集*/
void Task::AV::GetVideoCapabilitySet::handle()
{
    Video_NS::VideoCapabilitySet_S stVideoCapabilitySet;
    CAVConfigure::instance()->get_configure(stVideoCapabilitySet);
    result(Convert::to_string(stVideoCapabilitySet));
}

/*获取音频配置*/
void Task::AV::GetAudioConfig::handle()
{
    Audio_NS::AudioConfig_S stAudioConfig;
    Convert::to_struct(m_taskData, stAudioConfig);
    CAVConfigure::instance()->get_configure(stAudioConfig);
    result(Convert::to_string(stAudioConfig));
}

/*获取音频能力集*/
void Task::AV::GetAudioCapabilitySet::handle()
{
    Audio_NS::AudioCapabilitySet_S stAudioCapabilitySet;
    CAVConfigure::instance()->get_configure(stAudioCapabilitySet);
    result(Convert::to_string(stAudioCapabilitySet));
}

/*设置音频配置*/
void Task::AV::SetAudioConfig::handle()
{
    Audio_NS::AudioConfig_S stAudioConfig;
    Convert::to_struct(m_taskData, stAudioConfig);
    int nRet = CAVConfigure::instance()->set_configure(stAudioConfig);
    result(nRet);
}

/*获取视频ROI配置*/
void Task::AV::GetVideoRoiConfig::handle()
{
    std::set<Video_NS::VideoRoiConfig_S> stConfig;
    Convert::to_struct(m_taskData, stConfig);
    CAVConfigure::instance()->get_configure(stConfig);
    result(Convert::to_string(stConfig));
}

/*设置视频ROI配置*/
void Task::AV::SetVideoRoiConfig::handle()
{
    Video_NS::VideoRoiConfig_S stConfig;
    Convert::to_struct(m_taskData, stConfig);
    int nRet = CAVConfigure::instance()->set_configure(stConfig);
    result(nRet);
}

/*获取区域裁剪配置*/
void Task::AV::GetAreaCropConfig::handle()
{
    std::set<Video_NS::AreaCrop_S> stConfig;
    Convert::to_struct(m_taskData, stConfig);
    CAVConfigure::instance()->get_configure(stConfig);
    result(Convert::to_string(stConfig));
}

/*设置区域裁剪配置*/
void Task::AV::SetAreaCropConfig::handle()
{
    Video_NS::AreaCrop_S stConfig;
    Convert::to_struct(m_taskData, stConfig);
    Video_NS::VideoConfig_S stVideoConfig;
    stVideoConfig.nId = stConfig.nId;
    CAVConfigure::instance()->get_configure(stVideoConfig);

    /* 视频分辨率和区域裁剪分辨率相同且区域裁剪启用时，不进行区域裁剪 */
    if (stVideoConfig.stVideoResolution == stConfig.stResolution && stConfig.bEnable)
    {
        dlog_warn("视频分辨率和区域裁剪分辨率相同，不进行区域裁剪");
        result(ERR_WEB_REGION_CROP);
        return;
    }
    int nRet = CAVConfigure::instance()->set_configure(stConfig);
    result(nRet);
}

/* 获取视频区域裁剪换算分辨率 */
void Task::AV::GetAreaCropConversionResolution::handle()
{
    /* 给web转换送插件的区域坐标 */
    Video_NS::AreaCrop_S stAreaCrop;
    Convert::to_struct(m_taskData, stAreaCrop);
    /* 换算区域分辨率结果 */
    Common::Rect_S stLimitArea;

    if(stAreaCrop.stResolution.nWidth && stAreaCrop.stResolution.nHeight)
    {
        /* 获取当前分辨率 */
        Video_NS::VideoConfig_S stVideoConfig;
        stVideoConfig.nId = stAreaCrop.nId;
        CAVConfigure::instance()->get_configure(stVideoConfig);

        /* 转换送插件的区域坐标 */
        stLimitArea.nWidth = stAreaCrop.stResolution.nWidth;
        stLimitArea.nHeight = stAreaCrop.stResolution.nHeight;
        stLimitArea.ConvertResolution(stVideoConfig.stVideoResolution.nWidth,
                                      stVideoConfig.stVideoResolution.nHeight,
                                      1920,
                                      1080);
    }
    result(Convert::to_string(stLimitArea));
}
