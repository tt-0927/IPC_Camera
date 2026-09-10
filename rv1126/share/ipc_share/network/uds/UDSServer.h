#pragma once

#include "IOBase.h"
#include <thread>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include "Heartbeat.h"

class UDSAdapter;
namespace Net
{

class UDSServer : public IOBase
{
public:
    /** 
     * @brief 默认构造函数
     */
    UDSServer() = default;

    /** 
     * @brief 带参数的构造函数
     * @param stParam 参数结构体
     */
    UDSServer(Param_S &stParam);

    /** 
     * @brief 析构函数
     */
    ~UDSServer() override;

    /** 
     * @brief 发送消息
     * @param stMessage 要发送的消息
     * @return 发送结果
     */
    int send(const Message_S stMessage) override;

    /** 
     * @brief 接收消息
     * @param stMessage 存储接收消息的结构体
     * @return 接收结果
     */
    int receive(Message_S &stMessage) override;

    /** 
     * @brief 设置心跳包数据
     * @param pData 心跳包数据指针
     * @param nLength 数据长度
     */
    void set_heartbeat(const void *pData, size_t nLength) override;    
    
     /**
      * @brief 处理心跳状态的回调函数
      * @param bStatus 心跳状态的布尔值
      */ 
     void heartbeat_status(bool bStatus);
private:

    /**
     * @brief   : 清理函数
     */
    void cleanup();

    /**
     * @brief 接收数据
     * 
     * @param pSession 指向 UDSAdapter 的指针
     */
    void receive(UDSAdapter* pSession);

    
    void throw_status(int nStatus, void *pHandle);

    /**
     * @brief 接收消息
     * 
     * @param stMessage 消息结构体
     * @param stUserParam 用户参数结构体
     */
    void receive(Message_S& stMessage, UserParam_S &stUserParam);

    /**
     * @brief 处理数据
     * 
     * @param pData 数据指针
     * @param nDataLen 数据长度
     */
    void deal_data(const void *pData, int nDataLen);

    /**
     * @brief 处理连接
     * 
     * @param pHandle 连接句柄
     */
    void deal_connect(void *pHandle);

    /**
     * @brief 处理断开连接
     * 
     * @param pHandle 连接句柄
     */
    void deal_disconnect(void *pHandle);

    /**
     * @brief 处理错误
     * 
     * @param nError 错误代码
     */
    void deal_error(int nError);
private:
    /**
     * @brief 存储 UDS 服务器参数配置
     */
    Param_S m_stParam;
    
    /**
     * @brief UDS 适配器的共享指针，用于管理服务器实例
     */
    std::shared_ptr<UDSAdapter> m_server;
    
    /**
     * @brief 心跳检测的共享指针，用于维持连接活性
     */
    std::shared_ptr<Heartbeat> m_heartbeat;
    
    /**
     * @brief 存储所有活动会话的唯一集合
     */
    std::unordered_set<UDSAdapter*> m_sessions;
    
    /**
     * @brief 心跳检测的时间间隔
     */
    int m_nHeartbeatInterval;
    
    /**
     * @brief 用于保护共享资源的互斥量
     */
    std::mutex m_mutex;
    
    /**
     * @brief 指示服务器是否应退出的标志
     */
    std::atomic_bool m_bExit{false};
    
    /**
     * @brief 执行服务器任务的线程
     */
    std::thread m_tid;
};

}


