/**
 * @FilePath     : stream_server.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-30 13:57:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-02 17:22:51
 * @Description  : 录制送流、配置服务端
 */

#pragma once

#include <atomic>

#include "IpcRet.h"
#include "Singleton.h"
#include "video_define.h"
#include "audio_define.h"
#include "IOBase.h"
#include "UDSServer.h"


class CStreamServer : public CSingleton<CStreamServer>
{
    CStreamServer() = default;

public:
    ~CStreamServer() = default;
    friend class CSingleton<CStreamServer>;

    /**
     * @brief   : 初始化录制送流、配置服务端
     * @return   {int} 0：成功 小于零：失败
     */
    int init();

    /**
     * @brief   : 去初始化录制送流、配置服务端
     */
    void deinit();

    /**
     * @brief   : 发送数据、命令码
     * @param    {string} data：数据
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send(std::string data, int nActionCode, void *pHandle = nullptr);

    /**
     * @brief   : 发送数据、命令码
     * @param    {void} *pData：数据
     * @param    {int} nLen：数据长度
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send(void *pData, int nLen, int nActionCode, void *pHandle = nullptr);

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
    void fill_head(std::string &strData, int nActionCode);

    /**
     * @brief   : 填充包含Return返回码的消息体头部
     * @param    {string&} data：要填充到消息体的data数据
     * @param    {int} nActionCode：命令码
     * @param    {int} nReturn：返回码
     */
    void fill_returnHead(std::string &strData, int nActionCode, int nRetCode);

    /**
     * @brief   : 外部送视频数据
     * @param    {VideoFrame_S} *pVideoFrame：视频帧数据指针
     * @return   {int}非0：失败
     */
    int sendVideoData(Video_NS::VideoFrame_S *pVideoFrame);

    /**
     * @brief   : 外部送视频配置
     * @param    {VideoConfig_S} &stVideoConfig：视频配置
     * @return   {int}非0：失败
     */
    int sendVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig);

    /**
     * @brief   : 外部送音频数据
     * @param    {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @return   {int}非0：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief   : 外部送音频配置
     * @param    {AudioConfig_S} &stAudioConfig：音频配置
     * @return   {int}非0：失败
     */
    int sendAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig);

private:
    /*网络基础智能指针句柄*/
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    /*心跳字符串*/
    std::string m_heartbeat;
    /*客户端是否连接*/
    std::atomic_bool m_bConnect = false;
};
