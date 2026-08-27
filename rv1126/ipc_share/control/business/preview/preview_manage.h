/**
 * @FilePath     : preview_manage.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-11 17:21:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 09:25:49
 * @Description  : 预览管理
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include "preview_define.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "rtp_audio_receiver.h"
#include <memory>
#include <mutex>
#include <thread>
#include "av_configure.h"

class CPreviewManage : public CSingleton<CPreviewManage>
{
    CPreviewManage();

public:
    ~CPreviewManage();
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
     * @brief 执行 TVSDK 设备控制。
     * @note 当前仅支持声光报警，声音使用报警输出 GPIO0，灯光使用白光闪烁。
     */
    int device_control(const Preview::DeviceControl_S &stInfo);

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

    /**
     * @brief 定时线程主循环
     * @note 等待新的报警任务，到达 m_alarmLightDeadline 后确认代次未过期，再自动关闭声光。
     */
    void alarm_light_timer_loop();

    /**
     * @brief 启动或刷新计时器 
     * @note 递增 m_alarmLightTimerGeneration、设置截止时间、标记 Armed=true，并唤醒定时线程。每次新的 START 都会覆盖旧的倒计时。
     */
    void arm_alarm_light_timer(int nDurationSec);

    /**
     * @brief 取消当前自动停止任务
     * @note 递增代次、设 Armed=false、唤醒线程，使旧定时器立即失效。由 STOP、启动失败和设备退出调用。
     */
    void cancel_alarm_light_timer();

    /**
     * @brief 执行硬件停止
     * @note 关闭 GPIO 报警输出、释放白光临时抢占，由 ISP reconciler 恢复原有补光配置。手动 STOP 和超时停止共用它，确保行为一致。
     */
    int stop_alarm_light_output();

private:
    Preview::PreviewInfo_S m_stPreviewInfo;                                                       /* 预览信息 */
    Audio_NS::AudioFormat_E enCurFormat = Audio_NS::AudioFormat_E::G711A;                         /* 当前音频格式 */
    std::unique_ptr<RtpAudioReceiver> m_intercomReceiver = std::make_unique<RtpAudioReceiver>();  /* 对讲 */
    std::unique_ptr<RtpAudioReceiver> m_broadcastReceiver = std::make_unique<RtpAudioReceiver>(); /* 广播 */
    /* 语音通讯状态 */
    bool m_bIntercomStatus = false;
    /* 语音通讯Ip */
    std::string m_strIp;

    std::mutex m_alarmLightOperationMutex;                          /* 串行化硬件操作。保证 GPIO、闪光灯的开启、停止、自动停止不会同时执行。 */
    std::mutex m_alarmLightTimerMutex;                              /* 仅保护定时器状态，如截止时间、是否已启用、代次号。避免与硬件操作锁混用。 */
    std::condition_variable m_alarmLightTimerCv;                    /* 定时线程等待超时时使用；收到新的 START、STOP 或退出时立即唤醒，不必傻等到原超时点。 */
    std::thread m_alarmLightTimerThread;                            /* 常驻定时线程，负责等待 dwDurationMs 到期后自动关闭声光。避免每次控制都创建 detached 线程 */
    std::chrono::steady_clock::time_point m_alarmLightDeadline;     /* 当前声光报警的自动停止时间点，使用 steady_clock，不会受系统校时/NTP 修改影响。 */
    uint64_t m_alarmLightTimerGeneration = 0;                       /* 定时器版本号。每次 START 或 STOP 都递增；旧定时任务发现版本不一致就失效，不能关闭新报警。 */
    uint64_t m_u64AlarmLightOverrideToken = 0; /* 当前 TVSDK 灯光抢占编号。仅由 m_alarmLightOperationMutex 保护，STOP
                                                  和超时必须使用该编号释放对应请求。 */
    bool m_alarmLightTimerArmed = false;                            /* 当前是否存在有效的自动停止任务。 */
    bool m_alarmLightTimerExit = false;                             /* deinit 或析构时通知定时线程退出，并配合 join() 保证不会留下后台线程。 */
};
