/*
 * @Author       : EasonLu
 * @Date         : 2025-04-30 09:12:11
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-06 20:36:35
 * @FilePath     : MediaSession.h
 * @Description  : 媒体会话类
 * @Note         : 目前只设计发送逻辑，接收逻辑待完善
 */
#pragma once
#include "MediaNetBase.h"
#include "MediaRtp.h"
#include "MediaSdp.h"
#include "SipType.h"
#include <memory>
#include <string>
namespace SIP
{
    class Channel;
    class MediaSession : public std::enable_shared_from_this<MediaSession>
    {
    public:
        MediaSession(const std::string &strSipCallID,
                     const std::string &strSSRC,
                     std::shared_ptr<Channel> pChn)
            : m_strSipCallID(strSipCallID),
              m_strSSRC(strSSRC),
              m_pChn(pChn)
        {
        }
        ~MediaSession() = default;

        int Init(const SDP::ConnectionInfo_S &stInfo);

        int SendMedia(char *pData, int nLen, bool bIsAudio);

        void NetworkCb(const MediaNetBase::CbData_S &stData);

        void RtpMediaCb(const RTP::MediaInfo_S &stInfo);

        void SetVideoType(const SipVideoType_E &enType);

        void SetAudioInfo(const SipAudioInfo_S &stInfo);

        void SetVideoFps(int nFps);

        int GetNetworkPort() const;

        int SetCodecInfo(const SipDeviceInfo_S &stInfo);

        inline std::shared_ptr<Channel> GetChn() { return m_pChn; }
        inline void SetChn(std::shared_ptr<Channel> pChn) { m_pChn = pChn; }
        inline const std::string &GetSipCallID() const { return m_strSipCallID; }
        inline const std::string &GetSSRC() const { return m_strSSRC; }
        inline const SDP::ConnectionInfo_S &GetConnInfo() const { return m_stConnInfo; }

    protected:
        int CreateRtp();

    private:
        static int s_nLastPort; /* 上一次使用过的端口 */
        std::string m_strSipCallID = "";
        std::string m_strSSRC = "";
        std::shared_ptr<Channel> m_pChn = nullptr;
        SDP::ConnectionInfo_S m_stConnInfo;
        int m_nCallID = -1;
        int m_nDialogID = -1;

        /* 音视频格式 */
        SipVideoType_E m_enVideoType = SIP_VIDEO_H264; /* 默认H264 */
        SipAudioInfo_S m_stAudioInfo;
        int m_nVideoFps = 0;

        MediaNetBase::Ptr m_pNetBase = nullptr;
        RTP::Base::Ptr m_pRtpBase = nullptr;
    };
} // namespace Sip