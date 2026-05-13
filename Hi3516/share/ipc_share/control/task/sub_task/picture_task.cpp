/**
 * @FilePath     : picture_task.cpp
 * @Author       : cyc
 * @Date         : 2025-06-13 14:15:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-21 16:15:11
 * @Description  : 图像任务
 */

#include "picture_task.h"
#include "IpcRet.h"
#include "dlog.h"
#include "osd_manage.h"
#include "isp_configure.h"
#include "convert_interface.h"
#include "isp_sceneCtrl.h"
#include "isp_manage.h"


/// @brief 获取日夜切换参数
void Task::Pic::GetDayNight::handle()
{
    ISP::DayNightAttr_S stDayNightAttr;
    CIspConfigure::instance()->get_configure(stDayNightAttr);
    result(Convert::to_string(stDayNightAttr));
}

/// @brief 设置日夜切换参数
void Task::Pic::SetDayNight::handle()
{
    /* 本次收到的日夜配置 */
    ISP::DayNightAttr_S stDayNightAttr;

    /* 更新前保存的日夜配置 */
    ISP::DayNightAttr_S stOldDayNightAttr;

    /* 先保留更新前配置，后续需要据此判断是否属于“夜晚状态未变化但补光配置变化”的场景。 */
    CIspConfigure::instance()->get_configure(stOldDayNightAttr);
    Convert::to_struct(m_taskData, stDayNightAttr);
#if 0
    /* 要设置的日夜切换为自动模式或定时模式,强制设置曝光时间为默认值 */
    if (stDayNightAttr.enDayNightMode == ISP::DayNightMode_E::AUTO_MODE ||
        stDayNightAttr.enDayNightMode == ISP::DayNightMode_E::TIME_MODE)
    {
        ISP::ExposureAttr_S stExposureAttr;
        /* 曝光时间默认值 */
        auto enExpTime = stExposureAttr.enExpTime;
        /* 获取当前曝光属性 */
        CIspConfigure::instance()->get_configure(stExposureAttr);
        /* 曝光时间不为默认值，强制置默认值 */
        if (stExposureAttr.enExpTime != enExpTime)
        {
            stExposureAttr.enExpTime = enExpTime;
            /* 设置当前曝光属性 */
            CIspConfigure::instance()->set_configure(stExposureAttr);
            /* 更新配置 */
            CIspManage::instance()->update(ISP::PicConfigureType_E::EXPOSURE);
        }
    }
#endif
    /* 日夜配置写入结果 */
    int nRet = CIspConfigure::instance()->set_configure(stDayNightAttr);

    /* 只有配置写入成功后，才允许继续下发硬件，避免旧配置和新硬件状态不一致。 */
    if (nRet == IpcRet_E::OK)
    {
        /* 带上新旧配置一起更新，便于底层识别补光模式变更但日夜状态未变化的情况。 */
        nRet = CIspManage::instance()->update_daynight(stOldDayNightAttr, stDayNightAttr);
    }

    result(nRet);
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
    Convert::to_struct(m_taskData,stImage);
    int nRet = CIspConfigure::instance()->set_configure(stImage);
    CIspManage::instance()->update(ISP::PicConfigureType_E::IAMGE);
    result(nRet); 
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
    Convert::to_struct(m_taskData,stExpAttr);
    int nRet = CIspConfigure::instance()->set_configure(stExpAttr);
    /* 更新配置 */
    CIspManage::instance()->update(ISP::PicConfigureType_E::EXPOSURE);
    result(nRet);
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
    Convert::to_struct(m_taskData,stBackAttr);
    int nRet = CIspConfigure::instance()->set_configure(stBackAttr);
    CIspManage::instance()->update(ISP::PicConfigureType_E::BACKLIGHT);
    result(nRet); 
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
    Convert::to_struct(m_taskData,stAwbInfo);
    int nRet = CIspConfigure::instance()->set_configure(stAwbInfo); 
    CIspManage::instance()->update(ISP::PicConfigureType_E::AWB);
    result(nRet); 
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
    Convert::to_struct(m_taskData,stDnrInfo);
    int nRet = CIspConfigure::instance()->set_configure(stDnrInfo); 
    CIspManage::instance()->update(ISP::PicConfigureType_E::NR);
    result(nRet);  
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
    Convert::to_struct(m_taskData,stVideoAdjust);
    int nRet = CIspConfigure::instance()->set_configure(stVideoAdjust); 
    CIspManage::instance()->update(ISP::PicConfigureType_E::MIRROR);
    result(nRet); 
}

/// @brief 设置图像计划配置参数
void Task::Pic::SetSchedule::handle()
{
    ISP::SceneSchedule_S stSchedule;
    Convert::to_struct(m_taskData, stSchedule);
    
    int nRet = CIspConfigure::instance()->set_configure(stSchedule);
    CSceneCtrl::instance()->update();
    result(nRet);
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
    int nRet = -1;
    Convert::to_struct(m_taskData, enSceneType);
    if(enSceneType >= ISP::SceneType_E::SCENE_NORMAL && enSceneType < ISP::SceneType_E::SCENE_MAX)
    {
        nRet = CIspManage::instance()->apply_scene_all_params(enSceneType);
    }
    else 
    {
        dlog_error("场景配置参数错误 %d", enSceneType);
    }

    result(nRet);
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
    ISP::SceneType_E enSceneType;
    int nRet = CIspConfigure::instance()->set_configure();
    if(nRet == OK)
    {
        /* 获取当前场景 */
        CIspConfigure::instance()->get_configure(enSceneType);
        CIspManage::instance()->apply_scene_all_params(enSceneType);
        /* 镜像恢复默认 */
        CIspConfigure::instance()->set_configure(ISP::VideoAdjust_S{}); 
        CIspManage::instance()->update(ISP::PicConfigureType_E::MIRROR);

    }
    result(nRet);
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



