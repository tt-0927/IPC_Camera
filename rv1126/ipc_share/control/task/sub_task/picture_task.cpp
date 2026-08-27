/**
 * @FilePath     : picture_task.cpp
 * @Author       : cyc
 * @Date         : 2025-06-13 14:15:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : 图像任务
 */

#include "picture_task.h"
#include "IpcRet.h"
#include "dlog.h"
#include "osd_manage.h"
#include "isp_configure.h"
#include "convert_interface.h"
#include "isp_manage.h"


/// @brief 获取日夜切换参数
void Task::Pic::GetDayNight::handle()
{
    ISP::DayNightAttr_S stDayNightAttr;
    int nRet = CIspConfigure::instance()->get_configure(stDayNightAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取日夜配置失败: %d", nRet);
        result(nRet);
        return;
    }

    result(Convert::to_string(stDayNightAttr));
}

/// @brief 设置日夜切换参数
void Task::Pic::SetDayNight::handle()
{
    ISP::DayNightAttr_S stDayNightAttr;
    Convert::to_struct(m_taskData, stDayNightAttr);
    result(CIspManage::instance()->set_daynight_config(stDayNightAttr));
}

/// @brief 获取图像参数
void Task::Pic::GetImageParam::handle()
{
    ISP::ImageParam_S stImage;
    dlog_debug("nBrightness:%u",stImage.nBrightness);
    CIspConfigure::instance()->get_configure(stImage);
    dlog_debug("nBrightness:%u",stImage.nBrightness);
    result(Convert::to_string(stImage));
}

/// @brief 设置图像参数
void Task::Pic::SetImageParam::handle()
{
    ISP::ImageParam_S stImage;
    Convert::to_struct(m_taskData, stImage);
    result(CIspManage::instance()->set_image_config(stImage));
}

/// @brief 获取曝光参数
void Task::Pic::GetExposureParam::handle()
{
    ISP::ExposureAttr_S stExpAttr;
    CIspConfigure::instance()->get_configure(stExpAttr);
    result(Convert::to_string(stExpAttr));
}

/// @brief 设置曝光参数
void Task::Pic::SetExposureParam::handle()
{
    ISP::ExposureAttr_S stExpAttr;
    Convert::to_struct(m_taskData, stExpAttr);
    result(CIspManage::instance()->set_exposure_config(stExpAttr));
}

/// @brief 获取背光参数
void Task::Pic::GetBackLightParam::handle()
{
    ISP::BackLightArrt_S stBackAttr;
    CIspConfigure::instance()->get_configure(stBackAttr);
    result(Convert::to_string(stBackAttr));
}

/// @brief 设置背光参数
void Task::Pic::SetBackLightParam::handle()
{
    ISP::BackLightArrt_S stBackAttr;
    Convert::to_struct(m_taskData, stBackAttr);
    result(CIspManage::instance()->set_backlight_config(stBackAttr));
}

/// @brief 获取白平衡参数
void Task::Pic::GetAwbParam::handle()
{
    ISP::AwbAttr_S stAwbInfo;
    CIspConfigure::instance()->get_configure(stAwbInfo);
    result(Convert::to_string(stAwbInfo));
}

/// @brief 设置白平衡参数
void Task::Pic::SetAwbParam::handle()
{
    ISP::AwbAttr_S stAwbInfo;
    Convert::to_struct(m_taskData, stAwbInfo);
    result(CIspManage::instance()->set_awb_config(stAwbInfo));
}

/// @brief 获取图像降噪参数
void Task::Pic::GetDnrParam::handle()
{
    ISP::DnrAttr_S stDnrInfo;
    CIspConfigure::instance()->get_configure(stDnrInfo);
    result(Convert::to_string(stDnrInfo));
}

/// @brief 设置图像降噪参数
void Task::Pic::SetDnrParam::handle()
{
    ISP::DnrAttr_S stDnrInfo;
    Convert::to_struct(m_taskData, stDnrInfo);
    result(CIspManage::instance()->set_nr_config(stDnrInfo));
}

/// @brief 获取视频调整参数
void Task::Pic::GetVideoMirrorParam::handle()
{
    ISP::VideoAdjust_S stVideoAdjust;
    CIspConfigure::instance()->get_configure(stVideoAdjust);
    result(Convert::to_string(stVideoAdjust));
}

/// @brief 设置视频调整参数
void Task::Pic::SetVideoMirrorParam::handle()
{
    ISP::VideoAdjust_S stVideoAdjust;
    Convert::to_struct(m_taskData, stVideoAdjust);
    result(CIspManage::instance()->set_mirror_config(stVideoAdjust));
}

/// @brief 设置图像计划配置参数
void Task::Pic::SetSchedule::handle()
{
    ISP::SceneSchedule_S stSchedule;
    Convert::to_struct(m_taskData, stSchedule);
    result(CIspManage::instance()->set_scene_schedule(stSchedule));
}

/// @brief 获取图像计划配置参数
void Task::Pic::GetSchedule::handle()
{
    ISP::SceneSchedule_S stSchedule;
    Convert::to_struct(m_taskData, stSchedule);
    CIspConfigure::instance()->get_configure(stSchedule);
    result(Convert::to_string(stSchedule));
}

/// @brief 设置场景配置参数
void Task::Pic::SetScene::handle()
{
    ISP::SceneType_E enSceneType;
    Convert::to_struct(m_taskData, enSceneType);
    result(CIspManage::instance()->set_user_scene(enSceneType));
}

/// @brief 获取场景配置参数
void Task::Pic::GetScene::handle()
{
    ISP::SceneType_E enSceneType;
    Convert::to_struct(m_taskData, enSceneType);
    CIspConfigure::instance()->get_configure(enSceneType);
    result(Convert::to_string(enSceneType));
}

/// @brief 设置恢复默认参数
void Task::Pic::SetDefault::handle()
{
    result(CIspManage::instance()->restore_default_config());
}

/* 获取OSD配置 */
void Task::Pic::GetOsdConfigParam::handle()
{
    Osd::OsdConfig_S stOsdConfig;
    COsdManage::instance()->get_osd_config(stOsdConfig);
    result(Convert::to_string(stOsdConfig));
}

/* 设置OSD配置 */
void Task::Pic::SetOsdConfigParam::handle()
{
    Osd::OsdConfig_S stOsdConfig;
    Convert::to_struct(m_taskData, stOsdConfig);
    result(COsdManage::instance()->set_osd_config(stOsdConfig));
}

/* 获取COVER配置 */
void Task::Pic::GetCoverConfigParam::handle()
{
    Osd::CoverConfig_S stCoverConfig;
    COsdManage::instance()->get_cover_config(stCoverConfig);
    result(Convert::to_string(stCoverConfig));
}

/* 设置COVER配置 */
void Task::Pic::SetCoverConfigParam::handle()
{
    Osd::CoverConfig_S stCoverConfig;
    Convert::to_struct(m_taskData, stCoverConfig);
    result(COsdManage::instance()->set_cover_config(stCoverConfig));  
}
