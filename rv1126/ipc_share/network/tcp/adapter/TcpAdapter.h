/**
 * @file Session.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-07
 * 
 * @brief 
 */

#pragma once
#include <vector>

#include <functional>

#include "dlog.h"

using MessageCallback = std::function<void(const void *pData, int nDataLen)>;
using CloseCallback = std::function<void(void *pHandle)>;
using ErrorCallback = std::function<void(int nErrorCode)>;
using ConnectCallback = std::function<void(void *pHandle)>;
using LogCallback = std::function<void(const char *pLog)>;

class Callback {
public:
    Callback() = default;
    virtual ~Callback() {}
    void set_errorObserver(ErrorCallback errorCallback)
    {
        m_errorCallback = errorCallback;
    }
    void set_closeObserver(CloseCallback closeCallback)
    {
        m_closeCallback = closeCallback;
    }
    void set_connectObserver(ConnectCallback connectCallback)
    {
        m_connectCallback = connectCallback;
    }
    void set_logObserver(LogCallback logCallback)
    {
        m_logCallback = logCallback;
    }
    void set_messageObserver(MessageCallback messageCallback)
    {
        m_messageCallback = messageCallback;
    }
    void on_error(int nErrorCode)
    {
        if (m_errorCallback)
        {
            m_errorCallback(nErrorCode);
        }
    }
    void on_close(void *pHandle)
    {
        if (m_closeCallback)
        {
            m_closeCallback(pHandle);
        }
    }
    void on_connect(void *pHandle)
    {
        if (m_connectCallback)
        {
            m_connectCallback(pHandle);
        }
    }
    void on_log(const char *pLog)
    {
        if (m_logCallback)
        {
            m_logCallback(pLog);
        }
    }
    void on_message(const void *pData, int nDataLen)
    {
        if (m_messageCallback)
        {
            m_messageCallback(pData, nDataLen);
        }
    }
private:
    ErrorCallback m_errorCallback;
    CloseCallback m_closeCallback;
    ConnectCallback m_connectCallback;
    LogCallback m_logCallback;
    MessageCallback m_messageCallback;
};
// TcpAdapter类用于定义TCP适配器的接口
class TcpAdapter {
public:
    // 构造函数
    TcpAdapter() = default;
    // 虚析构函数
    virtual ~TcpAdapter() {}
    
    // 启动TCP适配器
    virtual int start() { return -1; }
    // 停止TCP适配器
    virtual int stop() { return -1; }
    virtual void stop_session(void *pHandle) { return; }
    virtual int reconnect() { return 0; }
    // 发送数据
    virtual int send(const void *pData, int nDataLen, void *pHandle = nullptr)  { return -1; } 
    // 接收数据
    virtual int receive(void *pData, int nDataLen) { return -1; }
    
    virtual bool is_connected() { return false; }
    virtual std::string get_ip() { return std::string(); }
    // 设置回调函数
    void set_callback(Callback& callback)
    {
        m_callback = callback;
    }
protected:
    // 存储回调函数的成员变量
    Callback m_callback;
};