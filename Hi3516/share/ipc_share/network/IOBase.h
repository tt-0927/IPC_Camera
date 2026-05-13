#pragma once

#include "NetDefine.h"
#include "dlog.h"
namespace Net
{

class IOBase
{
public:
    IOBase() = default;
    IOBase(Param_S &stParam) {}
    virtual ~IOBase() {}
    /**
     * @brief 发送消息，默认异步消息
     * 
     * @param stMessage 消息
     * @return int <0 失败
     */
    virtual int send(const Message_S stMessage) = 0;
    /**
     * @brief 接收消息, 同步消息时使用
     * 
     * @param stMessage 消息
     * @return int <0 失败
     */
    virtual int receive(Message_S &stMessage) = 0;
    /**
     * @brief Set the heartbeat
     * 
     * @param stMessage 消息
     * @return int 
     */
    virtual void set_heartbeat(const void *pData, size_t nLength) = 0;
};

}