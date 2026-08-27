/*
 * @Author       : EasonLu
 * @Date         : 2025-04-22 10:14:31
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-30 17:10:32
 * @FilePath     : SipChannel.cpp
 * @Description  : 通道信息
 */
#include "SipChannel.h"
#include "dlog.h"
#include "SipUtils.h"
#include "StreamManager.h"
using namespace SIP;

/* 国标媒体流默认唯一标识 */
#define GB_DEFAULT_SSRC "0x47420001"

void Channel::SetParentID(const std::string &parent_id)
{
    strParentID = parent_id;
}

std::string Channel::GetParentID() const
{
    return strParentID;
}

void Channel::SetChannelID(const std::string &channel_id)
{
    strChannelID = channel_id;
    SetParental(std::to_string(channel_id.empty() ? 0 : 1));
}

std::string Channel::GetChannelID() const
{
    return strChannelID;
}

void Channel::SetVoiceChnID(const std::string &channel_id)
{
    strVoiceChnID = channel_id;
}

std::string Channel::GetVoiceChnID() const
{
    return strVoiceChnID;
}

void Channel::SetName(const std::string &name)
{
    strName = ToMbcsString(name);
}

std::string Channel::GetName() const
{
    return strName;
}

void Channel::SetNickName(const std::string &name)
{
    strNickname = ToMbcsString(name);
}

std::string Channel::GetNickName() const
{
    return strNickname.empty() ? strName : strNickname;
}

void Channel::SetManufacturer(const std::string &manufacturer)
{
    strManufacturer = ToMbcsString(manufacturer);
}

std::string Channel::GetManufacturer() const
{
    return strManufacturer;
}

void Channel::SetModel(const std::string &model)
{
    strModel = model;
}

std::string Channel::GetModel() const
{
    return strModel;
}

void Channel::SetOwner(const std::string &owner)
{
    strOwner = owner;
}

std::string Channel::GetOwner() const
{
    return strOwner;
}

void Channel::SetCivilCode(const std::string &civil_code)
{
    strCivilCode = civil_code;
}

std::string Channel::GetCivilCode() const
{
    return strCivilCode;
}

void Channel::SetAddress(const std::string &address)
{
    strAddress = ToMbcsString(address);
}

std::string Channel::GetAddress() const
{
    return strAddress;
}

void Channel::SetStatus(const std::string &status)
{
    strStatus = status;
}

void Channel::SetSipStatus(const SipDevStatus_E &status)
{
    enStatus = status;
    /* 离线后关闭需同步移除实时点播会话 */
    if (SIP_OFFLINE == enStatus)
    {
        /* 移除实时点播会话 */
        CallCloseAction();
    }
}

SipDevStatus_E Channel::GetSipStatus() const
{
    return enStatus;
}

std::string Channel::GetStatus() const
{
    return strStatus;
}

void Channel::SetParental(const std::string &parental)
{
    strParental = parental;
}

std::string Channel::GetParental() const
{
    return strParental;
}

void Channel::SetRegisterWay(const std::string &register_way)
{
    strRegisterWay = register_way;
}

std::string Channel::GetRegisterWay() const
{
    return strRegisterWay;
}

void Channel::SetSecrecy(const std::string &secrecy)
{
    strSecrecy = secrecy;
}

std::string Channel::GetSecrecy() const
{
    return strSecrecy;
}

void Channel::SetStreamNum(const std::string &stream_num)
{
    strStreamNum = stream_num;
}

std::string Channel::GetStreamNum() const
{
    return strStreamNum;
}

void Channel::SetExternIP(const std::string &ip)
{
    strExternIP = ip;
}

std::string Channel::GetExternIP() const
{
    return strExternIP;
}

void Channel::SetPtzType(const std::string &ptz_type)
{
    strPtzType = ptz_type;
}

std::string Channel::GetPtzType() const
{
    return strPtzType;
}

void Channel::SetDownloadSpeed(const std::string &speed)
{
    strDownloadSpeed = speed;
}

std::string Channel::GetDownloadSpeed() const
{
    return strDownloadSpeed;
}

void Channel::SetDefaultSSRC(const std::string &id)
{
    strSsrc = id;
}

std::string Channel::GetDefaultSSRC() const
{
    return strSsrc;
}

void SIP::Channel::SetStreamID(const std::string &id)
{
    if (id.empty() && !strStreamID.empty())
    {
        dlog_info("移除通道[%s]点播会话", strStreamID.c_str());
        /* 移除实时点播记录 */
        StreamManager::instance()->RemoveStream(strStreamID);
    }
    strStreamID = id;
}

std::string Channel::GetStreamID() const
{
    return strStreamID;
}

int SIP::Channel::Play(bool bStart)
{
    return 0;
}

bool SIP::Channel::CreateServer()
{
    bool bRet = false;
    if (m_pPlay && m_pPlay->isRunning())
    {
        return true;
    }
    m_pPlay = nullptr;
    m_pPlay = std::make_shared<MediaServer>();
    /* NOTE 暂定根据通道号偏移端口号 */
    /* 寻找合适的端口 */
    if (!m_pPlay->init((SIP_SERVER_PORT_START + nIndex), MediaNetBase::Protocol::ALL))
    {
        /* 指定偏移端口不行，尝试下一个 */
        for (int i = 0; i < SIP_RTP_MAX_CHANNEL; i++)
        {
            if (!m_pPlay->init((SIP_SERVER_PORT_START + i), MediaNetBase::Protocol::ALL))
            {
                continue;
            }
            bRet = true;
            break;
        }
    }
    else
    {
        bRet = true;
    }

    /* 不管成功与否，必须设置数据回调函数 */
    m_pPlay->setCallback(std::bind(&SIP::Channel::ServerCallback, this, std::placeholders::_1));
    return bRet;
}

void SIP::Channel::DestroyServer()
{
    if (m_pPlay)
    {
        m_pPlay = nullptr;
    }
    /* TODO 是否需要通知上层应用已断开 */
    CallCloseAction();
}

void SIP::Channel::CallCloseAction()
{
    /* 清空streamID */
    SetStreamID("");
    /* 清空编码器信息 */
    enVideo = SIP_VIDEO_NONE;
    enAudio = SipAudioInfo_S();
}

bool SIP::Channel::CreateAudioServer(int nPort)
{
    bool bRet = false;
    if (m_pAudioPlay && m_pAudioPlay->isRunning())
    {
        dlog_debug("正在对讲，无需再启动");
        return true;
    }
    m_pAudioPlay = nullptr;
    m_pAudioPlay = std::make_shared<MediaServer>();

    if (!m_pAudioPlay->init((nPort), MediaNetBase::Protocol::UDP))
    {
        dlog_error("m_pAudioPlay init error");
        return false;
    }
    else
    {
        bRet = true;
    }

    /* 不管成功与否，必须设置数据回调函数 */
    m_pAudioPlay->setCallback(std::bind(&SIP::Channel::ServerCallback, this, std::placeholders::_1));
    return bRet;

}

void SIP::Channel::ClosePlayAction()
{
    SetCallID("");
    UpdateMediaStatus(false);
    SetDefaultSSRC("");
    DestroyPlayRtp();
}

int SIP::Channel::CreateClient(const SDP::ConnectionInfo_S &stConnInfo)
{
    /* TODO 独立创建 */
    bool bRet = false;
    int nLocalPort = (SIP_CLIENT_PORT_START + nIndex);
    auto enProtocol = stConnInfo.bIsTcp ? MediaNetBase::Protocol::TCP : MediaNetBase::Protocol::UDP;
    /* 销毁重建 */
    if (m_pPlay)
    {
        m_pPlay = nullptr;
    }
    if (!stConnInfo.bTcpActive)
    {
        m_pPlay = std::make_shared<MediaClient>(stConnInfo.strIP, stConnInfo.nPort);
        if (!m_pPlay->init(nLocalPort, enProtocol))
        {
            /* 指定偏移端口不行，尝试下一个 */
            for (int i = 0; i < SIP_PORT_RANGE; i++)
            {
                nLocalPort = (SIP_CLIENT_PORT_START + i);
                if (!m_pPlay->init(nLocalPort, enProtocol))
                {
                    continue;
                }
                bRet = true;
                break;
            }
        }
        else
        {
            bRet = true;
        }
        nLocalPort = bRet ? nLocalPort : -1;
    }
    else
    {
        /* TCP主动模式 */
        m_pPlay = std::make_shared<MediaServer>();
        /* 先加白名单 */
        auto pServer = std::dynamic_pointer_cast<MediaServer>(m_pPlay);
        if (pServer)
        {
            pServer->addAllowTarget(stConnInfo.strIP, stConnInfo.nPort);
        }
        if (!m_pPlay->init(nLocalPort, enProtocol))
        {
            /* 指定偏移端口不行，尝试下一个 */
            for (int i = 0; i < SIP_PORT_RANGE; i++)
            {
                nLocalPort = (SIP_CLIENT_PORT_START + i);
                if (!m_pPlay->init(nLocalPort, enProtocol))
                {
                    continue;
                }
                bRet = true;
                break;
            }
        }
        else
        {
            bRet = true;
        }
        nLocalPort = bRet ? nLocalPort : -1;
    }
    dlog_info("开启GB客户端的端口[%d]", nLocalPort);
    return nLocalPort;
}

void SIP::Channel::DestroyPlayNet()
{
    if (m_pPlay)
    {
        m_pPlay = nullptr;
    }
}

int SIP::Channel::GetPlayPort() const
{
    if (m_pPlay)
    {
        return m_pPlay->port();
    }
    return 0;
}

int SIP::Channel::CreatePlayRtp(bool bIsParser, std::string strSSRC)
{
    /* 如果SSRC未空，则使用默认值 */
    if(strSSRC.empty())
    {
        strSSRC = GB_DEFAULT_SSRC;
    }

    if (bIsParser)
    {
        /* 创建RTP解析类 */
        CreateRtpParser(strSSRC);
    }
    else
    {
        /* 创建RTP打包类 */
        CreateRtpPacker(strSSRC);
    }
    return 0;
}

void SIP::Channel::CreateRtpParser(const std::string &strSSRC)
{
    /* 创建RTP解析类 */
    if (nullptr != m_pRtpPlay)
    {
        m_pRtpPlay = nullptr;
    }
    /* NOTE 每次都需重新创建，SSRC和解包情况会不一样 */
    m_pRtpPlay = std::make_shared<RTP::Parser>(
        std::stoul(strSSRC),
        std::bind(&Channel::RtpMediaCallback, this, std::placeholders::_1));
}

void SIP::Channel::CreateRtpPacker(const std::string &strSSRC)
{
    std::lock_guard<std::mutex> lk(m_mutexPlayRtp);
    /* 创建RTP打包类 */
    if (nullptr != m_pRtpPlay)
    {
        m_pRtpPlay = nullptr;
    }

    
    
    bool bIsTcp = false;
    if (m_pPlay)
    {
        bIsTcp = m_pPlay->isTcp();
    }

    /* NOTE 每次都需重新创建，SSRC和打包情况会不一样 */
    m_pRtpPlay = std::make_shared<RTP::Packer>(
        std::stoul(strSSRC),
        nVideoFps,
        bIsTcp);
    auto pRtpPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpPlay);
    pRtpPacker->setVideoType(::ToPsmStreamIDByVideo(enVideo));
    pRtpPacker->setAudioType(::ToPsmStreamIDByAudio(enAudio.enType));
}

void SIP::Channel::DestroyPlayRtp()
{
    std::lock_guard<std::mutex> lk(m_mutexPlayRtp);
    /* 释放RTP打包类共享指针 */
    if (nullptr != m_pRtpPlay)
    {
        m_pRtpPlay = nullptr;
    }
}

void SIP::Channel::ServerCallback(const MediaNetBase::CbData_S &stData)
{
    if (m_pRtpPlay == nullptr)
    {
        return;
    }
    auto pRtpParser = std::dynamic_pointer_cast<RTP::Parser>(m_pRtpPlay);
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

void SIP::Channel::RtpMediaCallback(const RTP::MediaInfo_S &stInfo)
{
    if (stInfo.nStreamType > 0)
    {
        bool bUpdate = false;
        /* 只有大于0时streamID才有效 */
        if (stInfo.bIsAudio)
        {
            /* 音频流数据类型 */
            auto enAudioType = ::FromPsmStreamIDByAudio(stInfo.nStreamType);
            bUpdate = enAudioType != enAudio.enType;
            enAudio.enType = enAudioType;
        }
        else
        {
            /* 视频流数据类型 */
            auto enVideoType = ::FromPsmStreamIDByVideo(stInfo.nStreamType);
            bUpdate = enVideo != enVideoType;
            enVideo = enVideoType;
        }
        if (bUpdate)
        {
            UploadInfo();
        }
    }
    std::shared_lock<std::shared_mutex> lk(m_mutexCbInfo);
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
}

int SIP::Channel::SendMedia(char *pData, int nLen, bool bIsAudio)
{
    std::lock_guard<std::mutex> lk(m_mutexPlayRtp);
    if (nullptr == m_pRtpPlay)
    {
        return -1;
    }
    auto pRtpPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpPlay);
    if (pRtpPacker == nullptr)
    {
        return -2;
    }
    std::vector<std::vector<char>> output;
    pRtpPacker->packRtpPackage(pData, nLen, bIsAudio, output);
    /* 直接发送 */
    for (auto &pVec : output)
    {
        SendPlayData(pVec.data(), pVec.size());
    }
    return 0;
}

int SIP::Channel::SendPlayData(const char *pData, int nLen)
{
    if (m_pPlay)
    {
        return m_pPlay->sendData(pData, nLen);
    }
    return -1;
}

int SIP::Channel::UpdateMediaStatus(bool bStart)
{
    std::shared_lock<std::shared_mutex> lk(m_mutexCbInfo);
    if (m_stCbInfo.fnMediaStatus)
    {
        SipMediaStatus_S stCbData;
        stCbData.nIndex = nIndex;
        stCbData.strDevID = strParentID;
        stCbData.strChnID = strChannelID;
        stCbData.bStart = bStart;
        m_stCbInfo.fnMediaStatus(stCbData);
    }
    return 0;
}

void SIP::Channel::SetSipDeviceInfo(const SipDeviceInfo_S &devInfo)
{
    dlog_info("设置设备信息编码[%d]宽度[%d]高度[%d]帧率[%d]",
              devInfo.enVideo, devInfo.nWidth, devInfo.nHeight, devInfo.nFps);
    auto pRtpPacker = std::dynamic_pointer_cast<RTP::Packer>(m_pRtpPlay);
    if (enVideo != devInfo.enVideo && pRtpPacker)
    {
        pRtpPacker->setVideoType(::ToPsmStreamIDByVideo(devInfo.enVideo));
    }
    enVideo = devInfo.enVideo;
    nVideoWidth = devInfo.nWidth;
    nVideoHeight = devInfo.nHeight;
    if (nVideoFps != devInfo.nFps && pRtpPacker)
    {
        pRtpPacker->setVideoFps(devInfo.nFps);
    }
    nVideoFps = devInfo.nFps;
    if (enAudio.enType != devInfo.enAudio.enType && pRtpPacker)
    {
        pRtpPacker->setAudioType(::ToPsmStreamIDByAudio(devInfo.enAudio.enType));
    }
    enAudio = devInfo.enAudio;
}

void SIP::Channel::ToSipDevice(SipDeviceInfo_S &device_info)
{
    device_info.strID = strParentID;
    device_info.strChnID = strChannelID;
    device_info.strIP = strExternIP;
    device_info.nPort = nPort;
    /* 需要转码上抛给上层 */
    device_info.strName = ::ToUtf8String(strName);
    device_info.enStatus = enStatus;
    device_info.enVideo = enVideo;
    device_info.enAudio = enAudio;
}

void SIP::Channel::UploadInfo()
{
    if (m_bIsServer)
    {
        std::shared_lock<std::shared_mutex> lk(m_mutexCbInfo);
        if (m_stCbInfo.fnDevUpdate)
        {
            SipDeviceInfo_S device_info;
            ToSipDevice(device_info);
            m_stCbInfo.fnDevUpdate(device_info);
        }
    }
}

void SIP::Channel::SetSdpInfo(const SDP::SdpInfo_S &stInfo)
{
    m_stSdpInfo = stInfo;
    /* 更新通道协商后的音视频格式 */
}

SDP::SdpInfo_S SIP::Channel::GetSdpInfo() const
{
    return m_stSdpInfo;
}

void SIP::Channel::SetCbInfo(const CbInfo_S &stInfo)
{
    /* 使用独占锁更新回调数据 */
    std::unique_lock<std::shared_mutex> lock(m_mutexCbInfo);
    m_stCbInfo = stInfo;
}

int SIP::Channel::SetGetRecFile(const SipGetRecFile_S &stInfo)
{
    if (m_stCbInfo.fnGetRecFile)
    {
        SipGetRecFile_S stGetRecFile;
        stGetRecFile = stInfo;
        /* 额外更新通道号即可——确保通道号正确 */
        stGetRecFile.nIndex = nIndex;
        SipCbResult_S stResult;
        m_stCbInfo.fnGetRecFile(stGetRecFile, stResult);
        return 0;
    }
    return -1;
}

int SIP::Channel::UpdateReadFileAction(const SipReadFileAction_S &stInfo)
{
    if (m_stCbInfo.fnReadFileAction)
    {
        SipCbResult_S stResult;
        m_stCbInfo.fnReadFileAction(stInfo, stResult);
    }
    return 0;
}

int SIP::Channel::UpdateInfo(const SipChannelInfo_S &stInfo)
{
    /* 只同步上层下发的信息字段即可 */
    strExternIP = stInfo.strExternIP;
    strName = stInfo.strName;
    strStatus = stInfo.strStatus;
    strModel = stInfo.strModel;
    strManufacturer = stInfo.strManufacturer;
    nIndex = stInfo.nIndex;
    return 0;
}

SipChannelType_E SIP::Channel::GetChannelType()
{
    return enChannelType;
}

void SIP::Channel::SetChannelTypeFromId(const std::string& strChannelId)
{
    if (strChannelId.length() < 13) 
    {
       enChannelType = SipChannelType_E::CHANNELTYPE_UNKNOWN;
    }
     // 提取第11-13位（索引10-12）的子字符串
    std::string strTypeStr = strChannelId.substr(10, 3);
    int nTypeValue = std::stoi(strTypeStr);

    dlog_info("=======提取到通道类型字段[%d]=======", nTypeValue);

    if (nTypeValue >= 111 && nTypeValue <= 118) 
    {
       enChannelType =  static_cast<SipChannelType_E>(nTypeValue);
    }
    else if (nTypeValue == 130) 
    {
        enChannelType = SipChannelType_E::HYBRID_DISK_RECORDER_IVR_ENCODE;
    } else if (nTypeValue >= 131 && nTypeValue <= 139) 
    {
        enChannelType = static_cast<SipChannelType_E>(nTypeValue);
    } 
    else if (nTypeValue >= 200 && nTypeValue <= 211) 
    {
        enChannelType = static_cast<SipChannelType_E>(nTypeValue);
    } else if (nTypeValue == 215 || nTypeValue == 216) 
    {
        enChannelType = static_cast<SipChannelType_E>(nTypeValue);
    } 
    else if (nTypeValue >= 300 && nTypeValue <= 343) 
    {
        enChannelType = SipChannelType_E::INDUSTRY_ROLE_USER;
    } 
    else if (nTypeValue >= 344 && nTypeValue <= 399) 
    {
        enChannelType = SipChannelType_E::CENTER_USER_EXTEND;
    } 
    else if (nTypeValue >= 400 && nTypeValue <= 443) 
    {
        enChannelType = SipChannelType_E::INDUSTRY_ROLE_TERMINAL_USER;
    } 
    else if (nTypeValue >= 444 && nTypeValue <= 499) 
    {
        enChannelType = SipChannelType_E::TERMINAL_USER_EXTEND;
    } 
    else if (nTypeValue >= 500 && nTypeValue <= 501) 
    {
        enChannelType = static_cast<SipChannelType_E>(nTypeValue);
    } 
    else if (nTypeValue >= 502 && nTypeValue <= 599) 
    {
        enChannelType = SipChannelType_E::PLATFORM_EXTERNAL_SERVER_EXTEND;
    } 
    else if (nTypeValue >= 600 && nTypeValue <= 999) 
    {
        enChannelType = SipChannelType_E::EXTEND_TYPE;
    }
}

void SIP::Channel::GetSipDeviceInfo(SipDeviceInfo_S &devInfo)
{

    if (m_stCbInfo.fnDevUpdate)
    {
     
        m_stCbInfo.fnDevUpdate(devInfo);
    }
}