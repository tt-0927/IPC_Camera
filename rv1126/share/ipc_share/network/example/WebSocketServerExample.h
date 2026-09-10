/**
 * @file WebSocketServerExample.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-08
 * 
 * @brief 
 */

#include "NetDefine.h"
#include "WebSocketServer.h"

#include <string>
#include <memory>
class WebSocketServerExample
{
public:
    int init(); 
    void deinit();
    int send(std::string data, int nActionCode);
    void set_heartbeat(const void *pData, size_t nLength);
    void deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
private:
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
};
