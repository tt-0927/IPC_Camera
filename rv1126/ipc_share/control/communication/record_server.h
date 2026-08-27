/**
 * @FilePath     : record_server.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-30 11:38:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-01 20:01:52
 * @Description  : 录制通讯服务端
 */

#pragma once

#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "TCPClient.h"
#include "IOBase.h"
#include "common_define.h"

class CRecordServer : public CSingleton<CRecordServer>
{
    CRecordServer() = default;

public:
    ~CRecordServer() = default;
    friend class CSingleton<CRecordServer>;

    /**
     * @brief   : 初始化录制通讯服务端
     * @return   {int} 0：成功 小于零：失败
     */
    int init();

    /**
     * @brief   : 去初始化录制通讯服务端
     */
    void deinit();

    /**
     * @brief   : 设置任务管理类智能指针实例句柄
     * @param    {shared_ptr<CTaskManage>} pTaskManage：任务管理类智能指针实例句柄
     */
    void set_taskManage(std::shared_ptr<CTaskManage> pTaskManage);

    /**
     * @brief   : 发送数据消息、命令码
     * @param    {void} *pData：数据消息
     * @param    {int} nDataLen：数据消息长度
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send(const void *pData, int nDataLen, int nActionCode, void *pHandle = nullptr);

    /**
     * @brief   : 发送消息体带头部信息
     * @param    {string} data：要填充到消息体的data数据
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send_withHead(std::string data, int nActionCode, void *pHandle = nullptr);

    /**
     * @brief   : 处理心跳
     * @param    {Message_S&} stMessage：数据回调参数
     * @param    {UserParam_S} &stUserParam：用户参数
     */
    void deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief   : 处理连接状态
     * @param    {Message_S&} stMessage：数据回调参数
     * @param    {UserParam_S} &stUserParam：用户参数
     */
    void deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief   : 处理数据
     * @param    {Message_S&} stMessage：数据回调参数
     * @param    {UserParam_S} &stUserParam：用户参数
     */
    void deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief   : 填充不包含Return返回码的消息体头部
     * @param    {string&} data：要填充到消息体的data数据
     * @param    {int} nActionCode：命令码
     */
    void fill_head(std::string &data, int nActionCode);

    /**
     * @brief   : 设置公共状态回调
     * @param    {StatusCallback} observer：公共状态回调
     */
    void set_statusObserver(Common::StatusCallback observer);

private:
    /*网络基础智能指针句柄*/
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    /*任务管理类智能指针实例句柄*/
    std::shared_ptr<CTaskManage> m_pTaskManage = nullptr;
    /*心跳字符串*/
    std::string m_heartbeat;
    /*公共状态回调*/
    Common::StatusCallback m_statusObserver;
};
