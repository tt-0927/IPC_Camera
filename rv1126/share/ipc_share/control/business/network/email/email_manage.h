/*** 
 * @FilePath     : email_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-10-14 17:37:08
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-18 16:47:45
 * @Description  : 邮件类定义
 */

#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <variant>
#include <queue>
#include <condition_variable>
#include <functional> 

#include "CSmtp.h"
#include "Singleton.h"
#include "network_define.h"
#include "IpcRet.h"

#define TEST_SUBJECT "IPC Email test message"                                       /* 测试邮件标题 */
#define TEST_MESSAGE "This e-mail is used to test whether your SMTP settings work." /* 测试邮件消息 */
#define TEST_FILE "/root/tianl/test.png"                                            /* 测试发送的邮件图片附件 */
#define XMAILER "The Bat! (v3.02) Professional"                                     /* smtp版本 */

/**
 * @brief 使用 std::variant 来支持多种类型的参数
 */
using EmailVariant = std::variant<Network::EmailInfo_S, Network::EmailUser_S, Network::EmailEventInfo_S>;

/**
 * @brief 邮件类，主要用于发送邮件
 */
class CEmailManage : public CSingleton<CEmailManage>
{
    CEmailManage();

public:
    virtual ~CEmailManage() = default;
    friend class CSingleton<CEmailManage>; // 允许 Singleton 访问私有构造函数

    /**
     * @brief 邮件初始化
     * @return IpcRet_E
     */
    IpcRet_E init();
    /**
    * @brief 邮件去初始化
    * @return IpcRet_E
    */
    IpcRet_E deinit();

    /**
     * @brief 处理邮件相关操作
     * @param emailRequest
     */
    int HandleEmail(const EmailVariant &emailRequest);
    /**
     * @brief 获取设置邮件信息
     * @param stEmInfo
     */
    void GetEmailInfo(Network::EmailInfo_S &stEmInfo);
    /**
     * @brief 发送测试邮件
     */
    int SendTestEmail(const Network::EmailUser_S &stTestRecipient,std::function<void( int)> result);
    /**
     * @brief 开启定时发送邮件线程
     * @param nCaptureTimeInterval
     */
    void StartSending(int nCaptureTimeInterval);
    /**
     * @brief 关闭邮件发送线程
     */
    void StopSending();
    /**
     * @brief 发送事件触发邮件
     * @param stEventInfo 事件信息
     */
    int SendEventEmail(const Network::EmailEventInfo_S &stEventInfo);
private:
    /// @brief 邮件信息
    Network::EmailInfo_S m_EmInfo;
    /// @brief 用于发送邮件的线程
    std::unique_ptr<std::thread> SendThread;
    /// @brief 线程停止标志
    std::atomic<bool> bEnStopThread = false;
    /**
     * @brief 配置文件
     */
    std::string m_configFile;
     /* 用于标记是否正在测试 */
    std::atomic<bool> m_bTesting;

    std::atomic<bool> m_running;
     /* 事件邮件触发队列 */
    std::mutex m_emailQueueMutex;
    std::queue<Network::EmailEventInfo_S> m_emailEventQueue;
    std::condition_variable m_emailCond;
    std::thread m_emailEventThread;

    /**
     * @brief 设置邮件信息
     * @param stNewEmInfo
     */
    int SetEmailInfo(const Network::EmailInfo_S &stNewEmInfo);

    /**
     * @brief 定时发送邮件
     * @param nCaptureTimeInterval 间隔时间,单位秒
     */
    void SendEmailPeriodically(int nCaptureTimeInterval);
    void process_emailQueue();
};
