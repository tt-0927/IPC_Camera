/**
 * @FilePath     : stream_client.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 17:43:13
 * @Description  : 录制接流、配置客户端
 */

#include "stream_client.h"
#include "dlog.h"
#include "common_define.h"
#include "action_code.h"

#include "record_convert.h"
#include "convert_interface.h"

#include "TCPClient.h"
#include "UDSClient.h"

int StreamClient::init(int nPort)
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&StreamClient::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&StreamClient::deal_heartbeat, this, _1, _2);;
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&StreamClient::deal_status, this, _1, _2);;
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = nPort;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::UDSClient>(stParam);
    return 0;
}

void StreamClient::deinit()
{
    // m_pHandler.reset();
}

void StreamClient::set_recordHandler(CRecordFile *pRecordFile)
{

    // m_pRecordFile = pRecordFile;
}

void StreamClient::add_recordHandler(CRecordFile *pRecordFile)
{
    if (!pRecordFile)
        return;

    int nChnId = pRecordFile->get_chnId();

    if (m_mapCRecordFile.count(nChnId))
    {
        dlog_debug("m_mapCRecordFile中通道ID %d 已经存在", nChnId);
        return;
    }

    m_mapCRecordFile[nChnId] = pRecordFile;
}

int StreamClient::send(std::string data, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return -1;
    }
    dlog_debug("发送[%d]消息：%s", nActionCode, data.c_str());
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length() + 1;
    return m_pHandler->send(stMessage);
}

std::string StreamClient::get_data(std::string &jsonData)
{
    Json::Object *pJsonRoot = Json::init(jsonData);
    if (!pJsonRoot)
    {
        return std::string();
    }

    Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
    std::string data = Json::to_string(pJsonData);
    Json::deinit(pJsonRoot);
    return data;
}

void StreamClient::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}

void StreamClient::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    // int nStatus = *(const int *)stMessage.pData;
    // dlog_debug("接收到连接状态：%d", nStatus);
}

void StreamClient::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    if(m_mapCRecordFile.empty())
    {
        return;
    }

    for (const auto &[key, file] : m_mapCRecordFile)
    {
        if (file != nullptr)
        {
            /* 处理消息 */
            switch (stMessage.nActionCode)
            {
            case AC_CONTROL_VIDEO_CONFIG: //录制视频信息设置
            {
                const Record_NS::VideoConfigInfo_S *pInfo = static_cast<const Record_NS::VideoConfigInfo_S *>(stMessage.pData);
                file->set_videoInfo(*pInfo);
                break;
            }
            case AC_CONTROL_AUDIO_CONFIG: //录制音频信息设置
            {
                const Record_NS::AudioConfigInfo_S *pInfo = static_cast<const Record_NS::AudioConfigInfo_S *>(stMessage.pData);
                file->set_audioInfo(*pInfo);
                break;
            }
            case AC_NOTICE_RECORD_AUDIO_RESTART: //录制音频编码链路重启
            {
                file->notify_audio_restart();
                break;
            }
            case AC_STREAM_VIDEO_DATE: //录制流媒体视频数据
            {
                file->push(stMessage.pData, stMessage.nDataLength, Record_NS::MediaDataType::VIDEO_DATA);
                break;
            }
            case AC_STREAM_AUDIO_DATE: //录制流媒体音频数据
            {
                file->push(stMessage.pData, stMessage.nDataLength, Record_NS::MediaDataType::AUDIO_DATA);
                break;
            }
            default:
                break;
            }
        }
    }
}

void StreamClient::fill_head(std::string &data, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", "");

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
}

void StreamClient::fill_returnHead(std::string &data, int nActionCode, int nRet)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", "");
    Json::add(pJsonRoot, "Return", nRet);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
}
