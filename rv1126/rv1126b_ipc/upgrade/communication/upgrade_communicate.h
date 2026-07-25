/*
 * @FilePath     : upgrade_communicate.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:10:16
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-18 10:44:24
 * @Description  : 通讯模块
 */

#pragma once

#include "IpcRet.h"
#include "IOBase.h"
#include "Singleton.h"
#include "TCPServer.h"

class UpgradeServer : public CSingleton<UpgradeServer>
{
    UpgradeServer() = default;

public:
    ~UpgradeServer() = default;
    friend class CSingleton<UpgradeServer>;

    /**
     * @brief   初始化
     * @return [*]
     * @author EasonLu
     * @note
     */
    int init();

    /**
     * @brief   反初始化
     * @return [*]
     * @author EasonLu
     * @note
     */
    void deinit();

    /**
     * @brief   发送消息
     * @param  [string] data
     * @param  [int] nActionCode
     * @param  [void] *pHdndler
     * @return [*]
     * @author EasonLu
     * @note
     */
    int send(std::string data, int nActionCode, void *pHdndler = nullptr);

    /**
     * @brief   设置心跳发送内容
     * @param  [void] *pData
     * @param  [size_t] nLength
     * @return [*]
     * @author EasonLu
     * @note
     */
    void set_heartbeat(const void *pData, size_t nLength);

    /**
     * @brief  处理回调心跳
     * @param  [Message_S&] stMessage
     * @param  [UserParam_S] &stUserParam
     * @return [*]
     * @author EasonLu
     * @note
     */
    void deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief  处理回调状态
     * @param  [Message_S&] stMessage
     * @param  [UserParam_S] &stUserParam
     * @return [*]
     * @author EasonLu
     * @note
     */
    void deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief  处理回调消息
     * @param  [Message_S&] stMessage
     * @param  [serParam_S] &stUserParam
     * @return [*]
     * @author EasonLu
     * @note
     */
    void deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

private:
    /**
     * @brief  填充Json数据头
     * @param  [string] &strSend - 返回填充完的字符串引用
     * @param  [string] strData -  需要返回的数据
     * @param  [int] nActionCode - 需要填充的指令码
     * @param  [int] nRetCode - 需要填充的返回操作码
     * @param  [string] strData - 需要返回的Data字段Json报文
     * @return [*]
     * @author EasonLu
     * @note
     */
    void fill_returnHead(
        std::string &strSend,
        std::string strData,
        int nActionCode,
        int nRetCode);

    void upgrade_getStatus(int &nStatus);

private:
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    std::string m_heartbeat;
};
