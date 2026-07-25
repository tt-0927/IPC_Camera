/*
 * @Author       : EasonLu
 * @Date         : 2025-04-30 09:12:26
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-08 10:57:56
 * @FilePath     : MediaSession.cpp
 * @Description  : 媒体会话类
 */
#include "MediaSession.h"
#include "MediaClient.hpp"
#include "MediaServer.hpp"
#include "dlog.h"
#include "SipUtils.h"
#include <vector>

#ifndef SIP_MEDIA_SESSION_PORT_START
#define SIP_MEDIA_SESSION_PORT_START 17000
#endif

#ifndef SIP_MEDIA_SESSION_PORT_END
#define SIP_MEDIA_SESSION_PORT_END 18000
#endif

using namespace SIP;

int MediaSession::s_nLastPort = (SIP_MEDIA_SESSION_PORT_START - 1);

int MediaSession::Init(const SDP::ConnectionInfo_S &stInfo)
{
    /* NOTE 已远端发送的SDP信息为配置信息 */
    if (nullptr != m_pNetBase)
    {
        dlog_warn("MediaSession已经初始化");
        return -1;
    }
    /* 记录信息 */
    m_stConnInfo = stInfo;
    auto enProtocol = stInfo.bIsTcp ? MediaNetBase::Protocol::TCP : MediaNetBase::Protocol::UDP;
    if (stInfo.bTcpActive && stInfo.bIsTcp)
    {
        m_pNetBase = std::make_shared<MediaServer>();
        m_pNetBase->setCallback(std::bind(&SIP::MediaSession::NetworkCb, this, std::placeholders::_1));
    }
    else
    {
        m_pNetBase = std::make_shared<MediaClient>(stInfo.strIP, stInfo.nPort);
    }
    int nLocalPort = -1;
    for (++s_nLastPort; s_nLastPort < SIP_MEDIA_SESSION_PORT_END; s_nLastPort++)
    {
        if (m_pNetBase->init(s_nLastPort, enProtocol))
        {
            nLocalPort = s_nLastPort;
            break;
        }
    }
    dlog_info("MediaSession开启的本地端口[%d]", nLocalPort);
    /* 创建RTP */
    CreateRtp();
    return nLocalPort;
}

void SIP::MediaSession::NetworkCb(const MediaNetBase::CbData_S &stData)
{
    auto pRtpParser = std::dynamic_pointer_cast<RTP::Parser>(m_pRtpBase);
    if (pRtpParser == nullptr)
    {
        return;
    }
    /* 根据接收协议进行不同的解析 */
    if (stData.bIsTcp)
    {
        pRtpParser->parsePacketTcp(stData.pData, stData.nSize);
    }
    else
    {
        /* 送RTP解析 */
        pRtpParser->parsePacket(stData.pData, stData.nSize,stData.bAudio);
    }
}

void SIP::MediaSession::RtpMediaCb(const RTP::MediaInfo_S &stInfo)
{
#if 0 /* FIXME 只设计发送逻辑，接收逻辑待完善 */
    if (stInfo.nStreamType > 0)
    {
        bool bUpdate = false;
        /* 只有大于0时streamID才有效 */
        if (stInfo.bIsAudio)
        {
            /* 音频流数据类型 */
            auto enAudioType = ::FromPsmStreamIDByAudio(stInfo.nStreamType);
            bUpdate = enAudioType != m_stAudioInfo.enType;
            m_stAudioInfo.enType = enAudioType;
        }
        else
        {
            /* 视频流数据类型 */
            auto enVideoType = ::FromPsmStreamIDByVideo(stInfo.nStreamType);
            bUpdate = m_enVideoType != enVideoType;
            m_enVideoType = enVideoType;
        }
        if (bUpdate && m_pChn)
        {
            m_pChn->UploadInfo();
        }
    }
    if (m_stCbInfo.fnMediaUpdate)
    {
        SipMediaCbInfo_S stCbInfo;
        stCbInfo.strChnID = strChannelID;
        stCbInfo.strDevID = strParentID;
        stCbInfo.pData = stInfo.pData;
        stCbInfo.nLen = stInfo.nLen;
        stCbInfo.nTimestamp = stInfo.nTimestamp;
        stCbInfo.bIsAudio = stInfo.bIsAudio;
        stCbInfo.bIsKeyFrame = stInfo.bIsKeyFrame;
        stCbInfo.nStreamType = stInfo.nStreamType;
        m_stCbInfo.fnMediaUpdate(stCbInfo);
    }
#endif
}

void SIP::MediaSession::SetVideoType(const SipVideoType_E &enType)
{
    m_enVideoType = enType;
    auto pPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpBase);
    if (pPacker)
    {
        pPacker->setVideoType(::ToPsmStreamIDByVideo(enType));
    }
}

void SIP::MediaSession::SetAudioInfo(const SipAudioInfo_S &stInfo)
{
    m_stAudioInfo = stInfo;
    auto pPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpBase);
    if (pPacker)
    {
        pPacker->setAudioType(::ToPsmStreamIDByAudio(stInfo.enType));
    }
}

void SIP::MediaSession::SetVideoFps(int nFps)
{
    m_nVideoFps = nFps;
    auto pPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpBase);
    if (pPacker)
    {
        pPacker->setVideoFps(nFps);
    }
}

int SIP::MediaSession::GetNetworkPort() const
{
    if (m_pNetBase)
    {
        return m_pNetBase->port();
    }
    return 0;
}

int SIP::MediaSession::SetCodecInfo(const SipDeviceInfo_S &stInfo)
{
    if (stInfo.strCallID.empty() || stInfo.strCallID != m_strSipCallID)
    {
        dlog_warn("CallID为空或不匹配");
        return -1;
    }
    dlog_info("设置会话通道[%s]视频编码信息[%d]帧率[%d]",
              stInfo.strCallID.c_str(), stInfo.enVideo, stInfo.nFps);
    auto pRtpPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpBase);
    if (nullptr == pRtpPacker)
    {
        dlog_warn("当前会话不是RTP打包类");
        return -1;
    }

    pRtpPacker->setVideoType(::ToPsmStreamIDByVideo(stInfo.enVideo));
    m_enVideoType = stInfo.enVideo;
    pRtpPacker->setVideoFps(stInfo.nFps);
    m_nVideoFps = stInfo.nFps;
    pRtpPacker->setAudioType(::ToPsmStreamIDByAudio(stInfo.enAudio.enType));
    m_stAudioInfo = stInfo.enAudio;
    return 0;
}

int SIP::MediaSession::CreateRtp()
{
    /* 重置RTP基类指针 */
    if (m_pRtpBase)
    {
        m_pRtpBase = nullptr;
    }
    if (!m_stConnInfo.bRecvOnly)
    {
        dlog_info("创建RTP解析类");
        m_pRtpBase = std::make_shared<RTP::Parser>(
            std::stoul(m_strSSRC),
            std::bind(&MediaSession::RtpMediaCb, this, std::placeholders::_1));
    }
    else
    {
        dlog_info("创建RTP打包类");
        /* 先创建，视频参数后续再设置 */
        m_pRtpBase = std::make_shared<RTP::Packer>();
        auto pPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpBase);
        pPacker->setSSRC(std::stoul(m_strSSRC));
        pPacker->setIsTcp(m_stConnInfo.bIsTcp);
        /* TODO 还需接收到媒体参数后，再设置音视频格式和视频帧率 */
    }
    return 0;
}

int SIP::MediaSession::SendMedia(char *pData, int nLen, bool bIsAudio)
{
    auto pPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpBase);
    if (nullptr == pPacker)
    {
        return -1;
    }
    std::vector<std::vector<char>> output;
    pPacker->packRtpPackage(pData, nLen, bIsAudio, output);
    /* 直接发送 */
    for (auto &pVec : output)
    {
        if (m_pNetBase)
        {
            m_pNetBase->sendData(pVec.data(), pVec.size());
        }
    }
    return 0;
}