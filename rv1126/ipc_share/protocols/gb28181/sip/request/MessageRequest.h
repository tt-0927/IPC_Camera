/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 16:51:46
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-25 08:48:42
 * @FilePath     : MessageRequest.h
 * @Description  : 消息请求
 */
#pragma once
#include "BaseRequest.h"
#include "SSRC_Config.h"
#include "SipDevice.h"
#include <atomic>
#include <deque>
namespace SIP
{
    class MessageRequest : public BaseRequest
    {
    public:
        typedef std::shared_ptr<MessageRequest> Ptr;
        MessageRequest(eXosip_t *ctx, Device::Ptr device, REQUEST_MESSAGE_TYPE type);
        virtual ~MessageRequest();
        virtual int SendMessage(bool needcb = true);
        const std::string GetRequestSN();

    protected:
        virtual const std::string make_manscdp_body() = 0;
        virtual const std::string GetDeviceUrl();

    protected:
        uint64_t _request_sn;
        /* 需要分次发送数据时，保存发送的分片数据 */
        std::deque<std::string> m_dequeMsg;
        /* 全局序列号 */
        static std::atomic_uint64_t _sn;       
    private:
        std::string format_xml(const std::string &xml);
    };

    class InviteRequest : public BaseRequest
    {
    public:
        enum InviteType
        {
            Play = 0,
            Playback,
            Download,
            BROADCAST,
        };
        struct InviteParam
        {
            /* eXoSip相关字段 */
            std::string strFromUri = "";
            std::string strToUri = "";
            std::string strRoute = "";
            std::string strSubject = "";
            /* sdp相关字段 */
            std::string strChnID = "";
            std::string strLocalIP = "";
            int nLocalPort = 0;
            std::string strSSRC = "";
            std::string strStreamID = "";
            bool bUsingTcp = true;   /* 默认采用TCP */
            bool bTcpActive = false; /* 默认采用TCP被动 */
            InviteType enType = InviteType::Play;
            /* 回放和点播时应用的时间区间 */
            uint64_t nStartTime = 0;
            uint64_t nEndTime = 0;
            /* 重载赋值运算符 */
            InviteParam &operator=(const InviteParam &rhs)
            {
                if (this != &rhs)
                {
                    strFromUri = rhs.strFromUri;
                    strToUri = rhs.strToUri;
                    strRoute = rhs.strRoute;
                    strSubject = rhs.strSubject;
                    strChnID = rhs.strChnID;
                    strLocalIP = rhs.strLocalIP;
                    nLocalPort = rhs.nLocalPort;
                    strSSRC = rhs.strSSRC;
                    strStreamID = rhs.strStreamID;
                    bUsingTcp = rhs.bUsingTcp;
                    bTcpActive = rhs.bTcpActive;
                    enType = rhs.enType;
                    nStartTime = rhs.nStartTime;
                    nEndTime = rhs.nEndTime;
                }
                return *this;
            }
        };
        typedef std::shared_ptr<InviteRequest> Ptr;
        InviteRequest(eXosip_t *ctx, InviteParam &stParam)
            : BaseRequest(ctx, nullptr, REQUEST_MESSAGE_TYPE::REQUEST_CALL_INVITE), m_stRequestParam(stParam) {};

        virtual int SendCall(bool needcb = true);

    protected:
        const std::string make_sdp_body();

    private:
        std::string _channel_id;
        std::string _ssrc;
        std::string _stream_id;
        int m_nRtpPort = 0;
        SSRCConfig::Mode _play_mode = SSRCConfig::Mode::Playback;
        bool m_bUsingTcp = true;
        int64_t _start_time = 0;
        int64_t _end_time = 0;
        SSRCInfo::Ptr _ssrc_info = nullptr;
        InviteParam m_stRequestParam;
    };

    class InviteAudioRequest : public BaseRequest
    {
    public:
        enum InviteType
        {
            Play = 0,
            Playback,
            Download,
            BROADCAST,
        };
        struct InviteParam
        {
            /* eXoSip相关字段 */
            std::string strFromUri = "";
            std::string strToUri = "";
            std::string strRoute = "";
            std::string strSubject = "";
            /* sdp相关字段 */
            std::string strChnID = "";
            std::string strLocalIP = "";
            int nLocalPort = 0;
            std::string strSSRC = "";
            std::string strStreamID = "";
            bool bUsingTcp = true;   /* 默认采用TCP */
            bool bTcpActive = false; /* 默认采用TCP被动 */
            InviteType enType = InviteType::Play;
            /* 回放和点播时应用的时间区间 */
            uint64_t nStartTime = 0;
            uint64_t nEndTime = 0;
            /* 重载赋值运算符 */
            InviteParam &operator=(const InviteParam &rhs)
            {
                if (this != &rhs)
                {
                    strFromUri = rhs.strFromUri;
                    strToUri = rhs.strToUri;
                    strRoute = rhs.strRoute;
                    strSubject = rhs.strSubject;
                    strChnID = rhs.strChnID;
                    strLocalIP = rhs.strLocalIP;
                    nLocalPort = rhs.nLocalPort;
                    strSSRC = rhs.strSSRC;
                    strStreamID = rhs.strStreamID;
                    bUsingTcp = rhs.bUsingTcp;
                    bTcpActive = rhs.bTcpActive;
                    enType = rhs.enType;
                    nStartTime = rhs.nStartTime;
                    nEndTime = rhs.nEndTime;
                }
                return *this;
            }
        };
        typedef std::shared_ptr<InviteAudioRequest> Ptr;
        InviteAudioRequest(eXosip_t *ctx, InviteParam &stParam)
            : BaseRequest(ctx, nullptr, REQUEST_MESSAGE_TYPE::REQUEST_CALL_INVITE), m_stRequestParam(stParam) {};

        virtual int SendCall(bool needcb = true);

    protected:
        const std::string make_sdp_body();

    private:
        std::string _channel_id;
        std::string _ssrc;
        std::string _stream_id;
        int m_nRtpPort = 0;
        SSRCConfig::Mode _play_mode = SSRCConfig::Mode::Playback;
        bool m_bUsingTcp = true;
        int64_t _start_time = 0;
        int64_t _end_time = 0;
        SSRCInfo::Ptr _ssrc_info = nullptr;
        InviteParam m_stRequestParam;
    };
}