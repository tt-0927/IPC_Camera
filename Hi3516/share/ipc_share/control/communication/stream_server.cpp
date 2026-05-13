/**
 * @FilePath     : stream_server.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-30 13:57:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-17 14:51:01
 * @Description  : 录制送流、配置服务端
 */

#include "stream_server.h"

#include "dlog.h"
#include <unistd.h>
#include "Json.h"
#include "record_define.h"
#include "record_ctrl.h"
#include "action_code.h"
#include "libavcodec/codec_id.h"
#include "libavutil/samplefmt.h"
#include "algo_detect.h"

int CStreamServer::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CStreamServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CStreamServer::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CStreamServer::deal_status, this, _1, _2);

    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_RECORD_STREAM_PROT_ONE;

    /* 创建服务端 */
    m_pHandler = std::make_shared<Net::UDSServer>(stParam);
    return OK;
}

void CStreamServer::deinit()
{
}

int CStreamServer::send(std::string data, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return ERR;
    }

    if (!m_bConnect.load(std::memory_order_acquire))
    {
        return ERR;
    }

    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length() + 1;

    dlog_info("发送[%d]消息：%s", stMessage.nActionCode, stMessage.pData);

    return m_pHandler->send(stMessage);
}

int CStreamServer::send(void *pData, int nLen, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return ERR_UNINIT;
    }

    if (!pData || nLen <= 0 || nActionCode < 0)
    {
        return ERR_PTR_NULL;
    }

    if (!m_bConnect.load(std::memory_order_acquire))
    {
        return ERR;
    }

    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = pData;
    stMessage.nDataLength = nLen;

    return m_pHandler->send(stMessage);
}

void CStreamServer::deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_info("接收到心跳消息：%s", stMessage.pData);
}

void CStreamServer::deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    int nStatus = *(const int *)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        m_bConnect.store(true, std::memory_order_release);
        CRecordCtrl::instance()->set_record_process_status(true);
        dlog_info("record客户端已接入");
    }
    else
    {
        m_bConnect.store(false, std::memory_order_release);
        CRecordCtrl::instance()->set_record_process_status(false);
        dlog_info("record客户端已断开");
    }
}

void CStreamServer::deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // int nRetCode = 0;

    dlog_info("接收到[%d]消息：%s", stMessage.nActionCode, (char *)stMessage.pData);

    /* 接收到的数据 */
    // std::string strMsgData(static_cast<const char*>(stMessage.pData));

    /* Data字段数据 */
    // std::string strData;
    // std::string   data;
    // Json::Object* pJsonRoot = Json::init(strMsgData);
    // if (!pJsonRoot)
    // {
    //     ddlog_error("参数错误，不是json数据");
    //     return;
    // }

    // Json::Object* pJsonData = Json::get(pJsonRoot, "Data");
    // if(pJsonData != nullptr)
    // {
    // 	data      = Json::to_string(pJsonData);
    // }
    // Json::deinit(pJsonRoot);
    // fill_returnHead(strData, stMessage.nActionCode, nRetCode);
    // send(strData, stMessage.nActionCode);
}

/* 填充json头数据 */
void CStreamServer::fill_returnHead(std::string &strData, int nActionCode, int nRetCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "Return", nRetCode);

    if (!strData.empty())
    {
        Json::Object *pJsonData = Json::init(strData);
        if (pJsonData)
        {
            Json::add(pJsonRoot, "Data", pJsonData);
        }
    }

    strData = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

void CStreamServer::fill_head(std::string &strData, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");

    if (!strData.empty())
    {
        Json::Object *pJsonData = Json::init(strData);
        if (pJsonData)
        {
            Json::add(pJsonRoot, "Data", pJsonData);
        }
    }

    strData = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

int CStreamServer::sendVideoData(Video_NS::VideoFrame_S *pVideoFrame)
{
    // return send(pVideoFrame, sizeof(Video_NS::VideoFrame_S) + pVideoFrame->nLen, AC_STREAM_VIDEO_DATE);
    return send(pVideoFrame->pData, pVideoFrame->nLen, AC_STREAM_VIDEO_DATE);
}

int CStreamServer::sendVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig)
{
    Record_NS::VideoConfigInfo_S stVideoConfigInfo;
    stVideoConfigInfo.nVencWidth = stVideoConfig.stVideoResolution.nWidth;
    stVideoConfigInfo.nVencHeight = stVideoConfig.stVideoResolution.nHeight;
    stVideoConfigInfo.nFps = stVideoConfig.getFrameRateAsInt();
    stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
    if (stVideoConfig.enVideoCodec == Video_NS::VideoCodec_E::H264)
    {
        stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
    }
    else if (stVideoConfig.enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_HEVC;
    }

    return send(&stVideoConfigInfo, sizeof(Record_NS::VideoConfigInfo_S), AC_CONTROL_VIDEO_CONFIG);
}

int CStreamServer::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame)
{
    // return send(pAudioFrame, sizeof(Audio_NS::AudioFrame_S) + pAudioFrame->nLen, AC_STREAM_AUDIO_DATE);
    return send(pAudioFrame->pData, pAudioFrame->nLen, AC_STREAM_AUDIO_DATE);
}

int CStreamServer::sendAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig)
{
    Record_NS::AudioConfigInfo_S stAudioConfigInfo;
    stAudioConfigInfo.nSampleRate = 8000;
    stAudioConfigInfo.nAudioCodeID = AV_CODEC_ID_AAC;
    stAudioConfigInfo.nSampleFmt = AV_SAMPLE_FMT_S16;
    stAudioConfigInfo.nChannel = 1;

    return send(&stAudioConfigInfo, sizeof(Record_NS::AudioConfigInfo_S), AC_CONTROL_AUDIO_CONFIG);
}
