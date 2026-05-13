/**
 * @FilePath     : record_client.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-01 19:51:59
 * @Description  : 录制控制客户端
 */

#pragma once

#include <unordered_map>

#include "Singleton.h"
#include "IpcRet.h"
#include "IOBase.h"

#include "record_manage.h"

/*录制控制客户端*/
class CRecordClient : public CSingleton<CRecordClient>
{
    CRecordClient() = default;

public:
    ~CRecordClient() = default;
    friend class CSingleton<CRecordClient>;

    /**
     * @brief   : 初始化录制控制客户端
     * @return   {int} 0：成功 小于零：失败
     */
    int init();

    /**
     * @brief   : 去初始化录制控制客户端
     */
    void deinit();

    /**
     * @brief   : 设置录制管理类实例句柄
     * @param    {CRecordManage} *recordManage：录制管理类实例句柄
     */
    void set_recordManage(CRecordManage *recordManage);

    /**
     * @brief   : 发送消息、命令码
     * @param    {string} data：消息
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send(std::string data, int nActionCode, void *pHandle = nullptr);

    /**
     * @brief   : 获取接收消息的data数据
     * @param    {string} &jsonData：接收到的消息
     * @return   {string}接收消息的data数据
     */
    std::string get_data(std::string &jsonData);

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
     * @brief   : 填充包含Return返回码的消息体头部
     * @param    {string&} data：要填充到消息体的data数据
     * @param    {int} nActionCode：命令码
     * @param    {int} nReturn：返回码
     */
    void fill_returnHead(std::string &data, int nActionCode, int nReturn);

private:
    /*网络基础智能指针句柄*/
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    /*心跳字符串*/
    std::string m_heartbeat;
    /*录制管理类实例句柄*/
    CRecordManage *m_pRecordManage = nullptr;
};
