/***
 * @FilePath     : preview_manage.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-11 17:21:30
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-09 11:16:47
 * @Description  : 预览管理
 */

#pragma once

#include "preview_define.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "rtp_audio_receiver.h"
#include <memory>
#include "av_configure.h"

class CPreviewManage : public CSingleton<CPreviewManage>
{
    CPreviewManage();

public:
    ~CPreviewManage() = default;
    friend class CSingleton<CPreviewManage>;

    /**
     * @brief 初始化
     * @return int 大于等于0 成功
     */
    IpcRet_E init();

    /**
     * @brief 反初始化
     */
    IpcRet_E deinit();

    /**
     * @brief 获取预览信息
     * @param stInfo 预览信息
     * @return int  int 大于等于0 成功
     */
    int get_preview_info(Preview::PreviewInfo_S &stInfo);

    /**
     * @brief 设置预览信息
     * @param stInfo 预览信息
     * @return int  int 大于等于0 成功
     */
    int set_preview_info(Preview::PreviewInfo_S stInfo);

    /**
     * @description : 获取采集音频信息
     * @author      : huangjunda
     * @param        {CollectAudioInfo_S} &stInfo
     * @return       {*}
     */
    int get_collect_audio_info(Preview::CollectAudioInfo_S &stInfo);

    /*** 
     * @description : 添加对象停用方法
     * @author      : huangjunda
     * @return       {*}
     */
    void deactivate();

    /*** 
     * @description : 设置对讲信息
     * @author      : huangjunda
     * @param        {IntercomInfo_S} &stInfo
     * @return       {*}
     */    
    int set_intercom_info(Preview::IntercomInfo_S stInfo);

    /*** 
     * @description : 设置广播信息
     * @author      : huangjunda
     * @param        {BroadcastInfo_S} &stInfo
     * @return       {*}
     */    
    int set_broadcast_info(Preview::BroadcastInfo_S stInfo);

    /*** 
     * @description : 设置蜂鸣器报警
     * @author      : huangjunda
     * @param        {BeepAlarm_S} &stInfo
     * @return       {*}
     */    
    int set_beep_alarm(Preview::BeepAlarm_S stInfo);

    /**
     * @brief   : 获取语音通讯状态
     * @note    : 用于判断是否在对讲中
     * @return   {bool} 对讲状态
     */
    bool get_intercom_status();

    /**
     * @brief   : 获取语音通讯当前的Ip
     * @note    : 用于判断语音通讯中当前的Ip
     * @return   {string} Ip
     */
    std::string get_intercom_ip();

private:
 
    /*** 
     * @description : 回调接收到的音频
     * @author      : cyc
     * @param        {uint8_t*} pData
     * @param        {size_t} length
     * @return       {*}
     */    
    void audioDataCallback(const uint8_t* pData, size_t length);

private:
    Preview::PreviewInfo_S m_stPreviewInfo;                                                       /* 预览信息 */
    Audio_NS::AudioFormat_E enCurFormat = Audio_NS::AudioFormat_E::G711A;                         /* 当前音频格式 */
    std::unique_ptr<RtpAudioReceiver> m_intercomReceiver = std::make_unique<RtpAudioReceiver>();  /* 对讲 */
    std::unique_ptr<RtpAudioReceiver> m_broadcastReceiver = std::make_unique<RtpAudioReceiver>(); /* 广播 */
    /* 语音通讯状态 */
    bool m_bIntercomStatus = false;
    /* 语音通讯Ip */
    std::string m_strIp;
};
