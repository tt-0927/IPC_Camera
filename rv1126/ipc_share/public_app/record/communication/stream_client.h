/**
 * @FilePath     : stream_client.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-03 17:05:09
 * @Description  : 录制接流、配置客户端
 */

#pragma once

#include <unordered_map>

#include "Singleton.h"
#include "IpcRet.h"
#include "IOBase.h"
#include "record_file.h"

/*录制接流、配置客户端*/
class StreamClient : public std::enable_shared_from_this<StreamClient>
{
public:
    StreamClient() = default;
    ~StreamClient() = default;

    /**
     * @brief   : 初始化录制接流、配置客户端
     * @param    {int} nPort：网络端口
     * @return   {int} 0：成功 小于零：失败
     */
    int init(int nPort);

    /**
     * @brief   : 去初始化录制接流、配置客户端
     */
    void deinit();

    /**
     * @brief   : 设置录制数据类实例句柄
     * @param    {CRecordFile} *pRecordFile：录制数据类实例句柄
     */
    void set_recordHandler(CRecordFile *pRecordFile);

    /**
     * @brief   : 添加录制数据类实例句柄
     * @param    {CRecordFile} *pRecordFile：录制数据类实例句柄
     */
    void add_recordHandler(CRecordFile *pRecordFile);
    /**
     * @brief   : 发送数据、命令码
     * @param    {string} data：数据 
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
    /*map存储录制数据类实例句柄*/
    std::unordered_map<int, CRecordFile*> m_mapCRecordFile;
};
