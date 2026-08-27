/***
 * @FilePath     : preview_task.cpp
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2025-07-08 17:15:33
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-08 17:17:48
 * @Description  : 预览相关任务
 */

#include "preview_task.h"
#include "convert_interface.h"
#include "preview_manage.h"
#include "network_utils.h"

/* 获取预览配置 */
void Task::Preview::GetPreviewInfo::handle()
{
    ::Preview::PreviewInfo_S stPreviewInfo;
    CPreviewManage::instance()->get_preview_info(stPreviewInfo);
    result(Convert::to_string(stPreviewInfo));
}

/* 设置预览配置 */
void Task::Preview::SetPreviewInfo::handle()
{
    ::Preview::PreviewInfo_S stPreviewInfo;
    Convert::to_struct(m_taskData, stPreviewInfo);
    result(CPreviewManage::instance()->set_preview_info(stPreviewInfo));
}

/* 获取采集音频信息 */
void Task::Preview::GetCollectAudioInfo::handle()
{
    ::Preview::CollectAudioInfo_S stCollectAudioInfo;
    CPreviewManage::instance()->get_collect_audio_info(stCollectAudioInfo);
    result(Convert::to_string(stCollectAudioInfo));
}

/* 设置对讲信息 */
void Task::Preview::SetIntercomInfo::handle()
{
    int nRet = OK;
    ::Preview::IntercomInfo_S stIntercomInfo;
    Convert::to_struct(m_taskData, stIntercomInfo);

    /* 判断需要设置的对讲状态 */
    if (stIntercomInfo.bEnable)
    {
        /* 开启对讲 */
        if (!CPreviewManage::instance()->get_intercom_status())
        {
            stIntercomInfo.strLocalIp = m_strUserIp;
            /* 当前未进行对讲操作，开启对讲 */
            nRet = CPreviewManage::instance()->set_intercom_info(stIntercomInfo);
        }
        else
        {
            /* 当前已经在对讲，返回错误 */
            dlog_error("对讲已开启，无法重复开启");
            nRet = ERR_WEB_INTERCOM;
        }
    }
    else
    {
        /* 停止对讲 */
        if (CPreviewManage::instance()->get_intercom_status())
        {
            dlog_info("正在对讲");
            /* 获取语音通讯当前的IP */
            auto strIp = CPreviewManage::instance()->get_intercom_ip();
            /* 判断IP地址是否相同，只有相同才执行停止对讲操作 */
            if (NetworkUtils_NS::areIpsEqual(m_strUserIp, strIp))
            {
                dlog_info("IP地址相同,执行停止对讲");
                nRet = CPreviewManage::instance()->set_intercom_info(stIntercomInfo);
            }
        }
        else
        {
            dlog_error("对讲已停止，无法重复停止");
            nRet = ERR_WEB_INTERCOM;
        }
    }

    result(nRet);
}

/* 设置广播信息 */
void Task::Preview::SetBroadcastInfo::handle()
{
    int nRet = OK;
    ::Preview::BroadcastInfo_S stBroadcastInfo;
    Convert::to_struct(m_taskData, stBroadcastInfo);

    /* 判断需要设置的对讲状态 */
    if (stBroadcastInfo.bEnable)
    {
        /* 开启广播 */
        if (!CPreviewManage::instance()->get_intercom_status())
        {
            stBroadcastInfo.strLocalIp = m_strUserIp;
            /* 当前未进行广播操作，开启广播 */
            nRet = CPreviewManage::instance()->set_broadcast_info(stBroadcastInfo);
        }
        else
        {
            /* 当前已经在广播，返回错误 */
            dlog_error("广播已开启，无法重复开启");
            nRet = ERR_WEB_INTERCOM;
        }
    }
    else
    {
        /* 停止广播 */
        if (CPreviewManage::instance()->get_intercom_status())
        {
            dlog_info("正在广播");
            /* 获取语音通讯当前的IP */
            auto strIp = CPreviewManage::instance()->get_intercom_ip();
            /* 判断IP地址是否相同，只有相同才执行停止广播操作 */
            if (NetworkUtils_NS::areIpsEqual(m_strUserIp, strIp))
            {
                dlog_info("IP地址相同,执行停止广播");
                nRet = CPreviewManage::instance()->set_broadcast_info(stBroadcastInfo);
            }
        }
        else
        {
            dlog_error("广播已停止，无法重复停止");
            nRet = ERR_WEB_INTERCOM;
        }
    }

    result(nRet);
}

/* 设置蜂鸣器报警 */
void Task::Preview::SetBeepAlarm::handle()
{
    int nRet;
    ::Preview::BeepAlarm_S stBeepAlarm;
    Convert::to_struct(m_taskData, stBeepAlarm);
    nRet = CPreviewManage::instance()->set_beep_alarm(stBeepAlarm);
    result(nRet);
}

/* TVSDK 设备硬件控制 */
void Task::Preview::DeviceControl::handle()
{
    ::Preview::DeviceControl_S stControl;
    Convert::to_struct(m_taskData, stControl);
    result(CPreviewManage::instance()->device_control(stControl));
}

/* 获取对讲/广播状态 */
void Task::Preview::GetIntercomAndBroadcastStatus::handle()
{
    /* ture：正在对讲/广播 false：未在对讲/广播 */
    bool bStatus = CPreviewManage::instance()->get_intercom_status();
    Json::Object* pJsonData = Json::init();
    if (pJsonData)
    {
        Json::add(pJsonData, "Status", bStatus);
    }
    auto data = Json::to_string(pJsonData);
    Json::deinit(pJsonData);
    result(data);
}
