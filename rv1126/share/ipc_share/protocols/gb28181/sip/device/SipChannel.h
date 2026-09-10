/*
 * @Author       : EasonLu
 * @Date         : 2025-04-22 10:14:28
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-30 16:30:59
 * @FilePath     : SipChannel.h
 * @Description  : 通道信息
 */
#pragma once
#include "GbDefine.h"
#include "MediaClient.hpp"
#include "MediaNetBase.h"
#include "MediaRtp.h"
#include "MediaSdp.h"
#include "MediaServer.hpp"
#include "SipType.h"
#include <map>
#include <memory>
#include <shared_mutex>

/* 服务器通道起始端口 */
#ifndef SIP_SERVER_PORT_START
#define SIP_SERVER_PORT_START 17000
#endif

/* 客户端通道起始端口 */
#ifndef SIP_CLIENT_PORT_START
#define SIP_CLIENT_PORT_START 18000
#endif

/* 端口使用最大范围 */
#ifndef SIP_PORT_RANGE
#define SIP_PORT_RANGE 500
#endif

/* 最大通道数 */
#ifndef SIP_RTP_MAX_CHANNEL
#define SIP_RTP_MAX_CHANNEL 32
#endif

namespace SIP
{
    /// @brief 通道信息
    class Channel : public std::enable_shared_from_this<Channel>,
                    public SipChannelInfo_S
    {
    public:
        typedef std::shared_ptr<Channel> Ptr;
        /* 构造必须携带回调数据 */
        Channel(CbInfo_S stCbInfo, bool bIsServer)
            : m_stCbInfo(stCbInfo), m_bIsServer(bIsServer) {};
        Channel(const SipChannelInfo_S &stBase, CbInfo_S stCbInfo, bool bIsServer)
            : SipChannelInfo_S(stBase), m_stCbInfo(stCbInfo), m_bIsServer(bIsServer) {}

        void SetParentID(const std::string &parent_id);
        std::string GetParentID() const;

        void SetChannelID(const std::string &channel_id);
        std::string GetChannelID() const;

        void SetVoiceChnID(const std::string &channel_id);
        std::string GetVoiceChnID() const;

        void SetName(const std::string &name);
        std::string GetName() const;

        void SetNickName(const std::string &name);
        std::string GetNickName() const;

        void SetManufacturer(const std::string &manufacturer);
        std::string GetManufacturer() const;

        void SetModel(const std::string &model);
        std::string GetModel() const;

        void SetOwner(const std::string &owner);
        std::string GetOwner() const;

        void SetCivilCode(const std::string &civil_code);
        std::string GetCivilCode() const;

        void SetAddress(const std::string &address);
        std::string GetAddress() const;

        void SetStatus(const std::string &status);
        std::string GetStatus() const;

        void SetSipStatus(const SipDevStatus_E &status);
        SipDevStatus_E GetSipStatus() const;

        void SetParental(const std::string &parental);
        std::string GetParental() const;

        void SetRegisterWay(const std::string &register_way);
        std::string GetRegisterWay() const;

        void SetSecrecy(const std::string &secrecy);
        std::string GetSecrecy() const;

        void SetStreamNum(const std::string &stream_num);
        std::string GetStreamNum() const;

        void SetExternIP(const std::string &ip);
        std::string GetExternIP() const;

        void SetPtzType(const std::string &ptz_type);
        std::string GetPtzType() const;

        void SetDownloadSpeed(const std::string &speed);
        std::string GetDownloadSpeed() const;

        void SetDefaultSSRC(const std::string &id);
        std::string GetDefaultSSRC() const;

        void SetStreamID(const std::string &id);
        std::string GetStreamID() const;

        int Play(bool bStart);

        bool CreateServer();
        void DestroyServer();
        void CallCloseAction();

        void ClosePlayAction();

        int CreateClient(const SDP::ConnectionInfo_S &stConnInfo);

        int GetPlayPort() const;
        int SendPlayData(const char *pData, int nLen);
        void DestroyPlayNet();

        int CreatePlayRtp(bool bIsParser, std::string strSSRC);
        void CreateRtpParser(const std::string &strSSRC);
        void CreateRtpPacker(const std::string &strSSRC);
        void DestroyPlayRtp();

        void ServerCallback(const MediaNetBase::CbData_S &stData);
        void RtpMediaCallback(const RTP::MediaInfo_S &stInfo);

        bool CreateAudioServer(int nPort);

        /**
         * @brief	发送裸媒体数据
         * @param  [char] *pData - 媒体数据
         * @param  [int] nLen - 数据大小
         * @param  [bool] bIsAudio - 是否为音频数据
         * @return [*]
         * @author EasonLu
         * @note   需要进行ps和rtp封装
         */
        int SendMedia(char *pData, int nLen, bool bIsAudio);

        int UpdateMediaStatus(bool bStart);

        void SetSipDeviceInfo(const SipDeviceInfo_S &devInfo);
        void ToSipDevice(SipDeviceInfo_S &device_info);
        void UploadInfo();

        void SetSdpInfo(const SDP::SdpInfo_S &stInfo);
        SDP::SdpInfo_S GetSdpInfo() const;

        inline SipVideoType_E GetVideoType() const { return enVideo; }
        inline SipAudioInfo_S GetAudioInfo() const { return enAudio; }

        inline void SetCallID(const std::string &id) { m_strCallID = id; }
        inline std::string GetCallID() const { return m_strCallID; }

        void SetCbInfo(const CbInfo_S &stInfo);

        int SetGetRecFile(const SipGetRecFile_S &stInfo);

        int UpdateReadFileAction(const SipReadFileAction_S &stInfo);

        int UpdateInfo(const SipChannelInfo_S &stInfo);

        SipChannelType_E GetChannelType();
        void SetChannelTypeFromId(const std::string& strChannelId);
        void GetSipDeviceInfo(SipDeviceInfo_S &devInfo);
    private:
        /* NOTE 点播和通道强绑定 */
        /* 点播——不管是客户端还是服务器，均只有一路点播 */
        MediaNetBase::Ptr m_pPlay = nullptr;
        /* 语音播放 */
        MediaNetBase::Ptr m_pAudioPlay = nullptr;
        /* 点播的RTP处理 */
        RTP::Base::Ptr m_pRtpPlay = nullptr;
        std::mutex m_mutexPlayRtp;
        /* 一般都是存放协商后的SDP信息，本机协商后已同样存放此处 */
        SDP::SdpInfo_S m_stSdpInfo;
        /* 客户端被服务器点播后，服务器分配的CallID，从INVITE请求中获取 */
        std::string m_strCallID;
        /* 模块初始化的回调函数 */
        CbInfo_S m_stCbInfo;
        std::shared_mutex m_mutexCbInfo;
        /* 是否为服务端 */
        bool m_bIsServer = false;
    };

}
