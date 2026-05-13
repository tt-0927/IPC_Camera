/**
 * @file ClientAlarmManager.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-25
 * 
 * @brief 客户端告警管理类 
 */
#pragma once

#include <tvsdkhttplib.h>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include "NetTVSDKClientInterface.h"

using namespace tvsdk;

class CClientAlarmManager
{
public:
    CClientAlarmManager(const std::string& host, int port, const std::string& user, const std::string& pass);
    ~CClientAlarmManager();

    bool StartListen(void* userHandle, const std::string& sessionId);
    
    void Stop();
    
    bool IsRunning() const { return isRunning_; }

    void SetCallback(NET_TV_AlarmCallBack cb, void* userData) 
    {
        alarmCb_ = cb;
        alarmUserData_ = userData;
    }

    void SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData)
    {
        channelStatusCb_ = cb;
        channelStatusUserData_ = userData;
    }

private:
    void AlarmLoop();

private:
    std::string host_;
    int port_;
    std::string username_;
    std::string password_;
    std::string sessionId_;
    void* userHandle_ = nullptr; // For callback identification

    std::unique_ptr<httplib::Client> client_;
    std::thread thread_;
    std::atomic<bool> isRunning_{false};

    NET_TV_AlarmCallBack alarmCb_ = nullptr;
    void* alarmUserData_ = nullptr;

    NET_TV_ChannelStatusCallBack channelStatusCb_ = nullptr;
    void* channelStatusUserData_ = nullptr;
};
