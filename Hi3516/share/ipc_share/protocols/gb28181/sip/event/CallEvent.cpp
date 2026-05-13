/*
 * @Author       : EasonLu
 * @Date         : 2025-04-23 17:06:10
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-10 10:47:03
 * @FilePath     : CallEvent.cpp
 * @Description  : 会话事件
 */
#include "CallEvent.h"
#include "CallSession.h"
#include "DeviceManage.h"
#include "MediaSession.h"
#include "ModuleLog.h"
#include "RequestPool.h"
#include "SipClient.h"
#include "SipServer.h"
#include "SipUtils.h"
#include "StreamManager.h"
#include "RtpServer.h"

#define CALL_EVENT_DEBUG 0
using namespace SIP;

int CallEvent::HandleResponseSuccess(const SipEvent::Ptr e)
{
    std::string channel_id = e->m_pEvent->request->to->url->username;
    std::string host = e->m_pEvent->request->to->url->host;
    std::string port = e->m_pEvent->request->to->url->port;

    int call_id = e->m_pEvent->cid;
    int dialog_id = e->m_pEvent->did;

    MLOG_INFO("on_exosip_call_answered DeviceID:[%s]\tCallID:[%d]\tDialogID:[%d]",
              channel_id.c_str(), call_id, dialog_id);

    auto device = DeviceManage::instance()->GetDevice(host, port);
    if (device == nullptr)
    {
        MLOG_WARN("Device Not Exists:[%s:%s]", host.c_str(), port.c_str());
        return -1;
    }

    auto channel = device->GetChannel(channel_id);
    if (channel == nullptr)
    {
        MLOG_WARN("Channel Not Exists:[%s]", channel_id.c_str());
        return -1;
    }

    /* 解析出SDP协商后的payload type */
    auto pResponse = e->m_pEvent->response;
    if (nullptr == pResponse)
    {
        MLOG_WARN("on_call_answered response is null");
        return -2;
    }
    osip_body_t *body = nullptr;
    osip_message_get_body(pResponse, 0, &body);
    if (nullptr == body)
    {
        MLOG_WARN("on_call_answered body is null");
        return -3;
    }
    { /* 通过解析SDP报文上抛数据 */
        auto stSdpInfo = ::SDP::parseSdp(std::string(body->body, body->length));
        channel->SetSdpInfo(stSdpInfo);
    }

    auto sessions = StreamManager::instance()->GetStreamByType(STREAM_TYPE::STREAM_TYPE_GB);
    for (auto &&s : sessions)
    {
        auto session = std::dynamic_pointer_cast<CallSession>(s);

        if (session->GetCallID() == call_id)
        {
            session->SetDialogID(dialog_id);
            session->SetConnected(true);

            SendCallAck(e->m_pContext, dialog_id);
            return 0;
        }
    }

    return -1;
}

int CallEvent::HandleSuperiorResponseSuccess(const SipEvent::Ptr e)
{
    std::string channel_id = e->m_pEvent->request->to->url->username;
    std::string host = e->m_pEvent->request->to->url->host;
    std::string port = e->m_pEvent->request->to->url->port;

    int call_id = e->m_pEvent->cid;
    int dialog_id = e->m_pEvent->did;

    SendCallAck(e->m_pContext, dialog_id);

    MLOG_INFO("on_exosip_call_answered DeviceID:[%s]\tCallID:[%d]\tDialogID:[%d]",
              channel_id.c_str(), call_id, dialog_id);

    // auto device = DeviceManage::GetInstance()->GetDevice(host, port);
    // if (device == nullptr)
    // {
    //     MLOG_WARN("Device Not Exists:[%s:%s]", host.c_str(), port.c_str());
    //     return -1;
    // }
    if(m_pDevice == nullptr)
    {
        MLOG_WARN("Device Not Exists:[%s:%s]", host.c_str(), port.c_str());
        return -1;
    }

    auto channel = m_pDevice->GetChannel(channel_id);//device->GetChannel(channel_id);
    if (channel == nullptr)
    {
        MLOG_WARN("Channel Not Exists:[%s]", channel_id.c_str());
        return -1;
    }

    /* 解析出SDP协商后的payload type */
    auto pResponse = e->m_pEvent->response;
    if (nullptr == pResponse)
    {
        MLOG_WARN("on_call_answered response is null");
        return -2;
    }
    osip_body_t *body = nullptr;
    osip_message_get_body(pResponse, 0, &body);
    if (nullptr == body)
    {
        MLOG_WARN("on_call_answered body is null");
        return -3;
    }
    { /* 通过解析SDP报文上抛数据 */
        int nPort = (channel->GetPlayPort()+2);
        auto stSdpInfo = ::SDP::parseSdp(std::string(body->body, body->length));
        channel->CreateAudioServer(nPort);
        channel->CreatePlayRtp(true, stSdpInfo.strSSRC);
    }

    return -1;
}

int CallEvent::on_proceeding(const SipEvent::Ptr e)
{
    std::string reqid;
    osip_generic_param_t *tag = nullptr;
    // osip_to_get_tag(e->m_pEvent->request->from, &tag);
    osip_uri_param_get_byname(&e->m_pEvent->request->from->gen_params, (char *)"tag", &tag);

    if (nullptr == tag || nullptr == tag->gvalue)
    {
        reqid = "";
    }
    else
    {
        reqid = (const char *)tag->gvalue;
    }
    RequestPool::instance()->RemoveRequest(reqid);
    MLOG_INFO("on_exosip_call_proceeding response reqid:[%s]", reqid.c_str());
    return 0;
}

int CallEvent::HandleClose(const SipEvent::Ptr e)
{
    auto pServer = dynamic_cast<SipServer *>(e->m_pNetBase);
    if (nullptr == pServer)
    {
        /* 不是来自服务器的不处理 */
        return 0;
    }
    std::string device_id = e->m_pEvent->request->to->url->username;

    int call_id = e->m_pEvent->cid;
    int dialog_id = e->m_pEvent->did;

    MLOG_INFO("Close Call DeviceID:[%s]\tCallID:[%d]\tDialogID:[%d]",
              device_id.c_str(), call_id, dialog_id);

    auto sessions = StreamManager::instance()->GetStreamByType(STREAM_TYPE::STREAM_TYPE_GB);
    for (auto &&s : sessions)
    {
        auto session = std::dynamic_pointer_cast<CallSession>(s);

        if (session->GetCallID() == call_id)
        {
            session->SetDialogID(dialog_id);
            session->SetConnected(false);
            StreamManager::instance()->RemoveStream(session->GetStreamID());
            return 0;
        }
    }
    MLOG_WARN("CallID not found:[%d]", call_id);
    return -1;
}

int CallEvent::HandleIncomingRequest(const SipEvent::Ptr &e)
{
    MLOG_INFO("接收到INVITE");
    if (e->m_pEvent == nullptr)
    {
        MLOG_ERROR("event is nullptr");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    if (e->m_pNetBase == nullptr)
    {
        MLOG_ERROR("netbase is nullptr");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    osip_body_t *sdp_body = nullptr;
    auto pRequest = e->m_pEvent->request;
    osip_message_get_body(pRequest, 0, &sdp_body);
    if (sdp_body == nullptr)
    {
        MLOG_ERROR("未找到SDP数据");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    MLOG_DEBUG("invite -----> \n %s", sdp_body->body);
    sdp_message_t *sdp = nullptr;
    std::string strChnID;
    if (OSIP_SUCCESS != sdp_message_init(&sdp))
    {
        MLOG_ERROR("初始化SDP数据失败");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -2;
    }

    /* 解析请求的通道ID是否存在 */
    Channel::Ptr pChannel = nullptr;
    if (pRequest && pRequest->req_uri && pRequest->req_uri->username)
    {
        strChnID = pRequest->req_uri->username;
        MLOG_INFO("INVITE:[%s]", strChnID.data());
        if (m_pDevice)
        {
            pChannel = m_pDevice->GetChannel(strChnID);
        }
        if (pChannel == nullptr)
        {
            MLOG_ERROR("未找到对应Channel:[%s]", strChnID.data());
            SendInviteResponse(e, SIP_NOT_FOUND);
            return -3;
        }
    }
    else
    {
        MLOG_ERROR("请求uri解析错误");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -4;
    }

    MLOG_INFO("成功解析INVITE的通道ID:[%s]", pChannel->GetChannelID().data());

    auto stSdpInfo = ::SDP::parseSdp(std::string(sdp_body->body, sdp_body->length));
    if (::SDP::Session_E::Play == stSdpInfo.enSessionType)
    {
        MLOG_INFO("实时点播模式");
        /* 先通知上层进行发流，再做剩余操作，开启发流才会有本地编码器信息 */
        pChannel->UpdateMediaStatus(true);
    }
    if (stSdpInfo.stVideoConn.bHaveConnection)
    {
        MLOG_INFO("视频连接信息IP[%s]Port[%d]TCP[%d]IPv6[%d]Active[%d]SSRC[%s]",
                  stSdpInfo.stVideoConn.strIP.c_str(),
                  stSdpInfo.stVideoConn.nPort,
                  stSdpInfo.stVideoConn.bIsTcp,
                  stSdpInfo.stVideoConn.bIsIPV6,
                  stSdpInfo.stVideoConn.bTcpActive,
                  stSdpInfo.strSSRC.c_str());
    }
    if (stSdpInfo.stAudioConn.bHaveConnection)
    {
        MLOG_INFO("音频连接信息IP[%s]Port[%d]TCP[%d]IPv6[%d]Active[%d]SSRC[%s]",
                  stSdpInfo.stAudioConn.strIP.c_str(),
                  stSdpInfo.stAudioConn.nPort,
                  stSdpInfo.stAudioConn.bIsTcp,
                  stSdpInfo.stAudioConn.bIsIPV6,
                  stSdpInfo.stAudioConn.bTcpActive,
                  stSdpInfo.strSSRC.c_str());
    }

    /* 记录服务器中对应的CallID */
    std::string strCallID = "";
    if (pRequest->call_id)
    {
        pChannel->SetCallID(pRequest->call_id->number);
        strCallID = pRequest->call_id->number;
        MLOG_INFO("INVITE CallID:[%s]", pRequest->call_id->number);
    }
    else
    {
        MLOG_ERROR("解析INVITE CallID失败");
        SendInviteResponse(e, SIP_INTERNAL_SERVER_ERROR);
        return -5;
    }

    int nLocalPort = 0;
    /* NOTE
        1、点播（Play）逻辑和通道强相关绑定，存在收发双向操作
        2、回放（Playback）和下载（Download），只存在发送单向操作，统一在客户端层级管理即可
    */
    /* 上抛回放或下载的录像信息给上层 */
    if (::SDP::Session_E::Playback == stSdpInfo.enSessionType ||
        ::SDP::Session_E::Download == stSdpInfo.enSessionType)
    {
        MLOG_INFO("回放或下载模式");
        /* 先创建，后上抛 */
        { /* 创建会话 */
            auto pSession = std::make_shared<MediaSession>(strCallID, stSdpInfo.strSSRC, pChannel);
            pSession->Init(stSdpInfo.stVideoConn);
            e->m_pNetBase->AddSession(strCallID, pSession);
            nLocalPort = pSession->GetNetworkPort();
        }
        { /* 上抛读取的录制文件信息 */
            SipGetRecFile_S stGetInfo;
            stGetInfo.nStartTime = stSdpInfo.nStartTime;
            stGetInfo.nEndTime = stSdpInfo.nEndTime;
            stGetInfo.strCallID = strCallID;
            pChannel->SetGetRecFile(stGetInfo);
        }

#if CALL_EVENT_DEBUG
        MLOG_DEBUG("回放或下载模式，下载速度为:[%d]", stSdpInfo.nDownloadSpeed);
#endif
        /* 直接更新文件读取速度 */
        if (stSdpInfo.nDownloadSpeed > 0)
        {
            SipReadFileAction_S stAction;
            stAction.strCallID = strCallID;
            stAction.enAction = SIP_READFILE_SPEED;
            stAction.dSpeed = (double)stSdpInfo.nDownloadSpeed;
            pChannel->UpdateReadFileAction(stAction);
        }
    }
    else
    {
        MLOG_INFO("实时点播模式");
        /* 通知上层开启发流 */
        pChannel->UpdateMediaStatus(true);
        SipDeviceInfo_S devInfo;
        pChannel->GetSipDeviceInfo(devInfo);
		// devInfo.nFps = 30;
		// devInfo.enVideo = SIP_VIDEO_H264;
		// devInfo.nWidth = 2880;
		// devInfo.nHeight = 1620;
		pChannel->SetSipDeviceInfo(devInfo);
        CRtpServer::instance()->set_rtpStatus(RTP_RUNNING,strChnID.data(),m_pDevice);
        nLocalPort = pChannel->CreateClient(stSdpInfo.stVideoConn);
        /* 更新点播的相关数据 */
        pChannel->SetCallID(strCallID);
        /* 更新SSRC */
        pChannel->SetDefaultSSRC(stSdpInfo.strSSRC);
        /* 开启RTP和PS的封装 */
        pChannel->CreatePlayRtp(false, stSdpInfo.strSSRC);
    }

    if (nLocalPort <= 0)
    {
        MLOG_ERROR("分配本地端口失败");
        SendInviteResponse(e, SIP_INTERNAL_SERVER_ERROR);
        if (::SDP::Session_E::Play == stSdpInfo.enSessionType)
        {
            CRtpServer::instance()->set_rtpStatus(RTP_IDLE,strChnID,m_pDevice);
            /* 无法开启进行SIP会话，关闭本地发流 */
            pChannel->ClosePlayAction();
        }
        return -6;
    }

    using namespace SIP::SDP;
    SdpNegotiate_S stNegInfo;
    stNegInfo.strID = pChannel->GetChannelID();
    stNegInfo.strIP = pChannel->GetExternIP();
    MLOG_DEBUG("*********stNegInfo.strIP = [%s] pChannel->GetExternIP()=[%s]",stNegInfo.strIP.c_str(),pChannel->GetExternIP().c_str());
    stNegInfo.nPort = nLocalPort;
    /* 暂时只添加音视频格式数据 */
    stNegInfo.stVideo.enType = (VideoType_E)(pChannel->GetVideoType());
    auto stAudioInfo = pChannel->GetAudioInfo();
    stNegInfo.stAudio.enType = (AudioType_E)(stAudioInfo.enType);

    auto strNegSdp = negotiateSdp(stSdpInfo, stNegInfo);
    MLOG_DEBUG("协商后的SDP报文:\n%s", strNegSdp.c_str());

    SendInviteResponse(e, SIP_OK, strNegSdp);
    return 0;
}

int CallEvent::HandleIncomingAudioRequest(const SipEvent::Ptr &e)
{
    MLOG_INFO("接收到INVITE");
    if (e->m_pEvent == nullptr)
    {
        MLOG_ERROR("event is nullptr");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    if (e->m_pNetBase == nullptr)
    {
        MLOG_ERROR("netbase is nullptr");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    osip_body_t *sdp_body = nullptr;
    auto pRequest = e->m_pEvent->request;
    osip_message_get_body(pRequest, 0, &sdp_body);
    if (sdp_body == nullptr)
    {
        MLOG_ERROR("未找到SDP数据");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    MLOG_DEBUG("invite -----> \n %s", sdp_body->body);
    sdp_message_t *sdp = nullptr;
    if (OSIP_SUCCESS != sdp_message_init(&sdp))
    {
        MLOG_ERROR("初始化SDP数据失败");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        return -2;
    }

    std::string strDeviceID;
    if(e->m_pEvent->request->from->url != nullptr)
    {
        //osip_from_t* from = osip_message_get_from(e->m_pEvent->request);
        // osip_uri_t *url;
        char *szDevice = nullptr;
        osip_uri_to_str(e->m_pEvent->request->from->url, &szDevice);
        if(szDevice != nullptr)
        {
            std::string sttr = szDevice;
            int first = sttr.find_first_of(':');
            int end = sttr.find_first_of('@');
            strDeviceID = sttr.substr(first+1, end-4);
            free(szDevice);
        }
    }

    /* 解析请求的通道ID是否存在 */
    Channel::Ptr pChannel = nullptr;
    if (pRequest && pRequest->req_uri && pRequest->req_uri->username)
    {
        std::string strChnID = strDeviceID;//std::string strChnID = pRequest->req_uri->username;
        MLOG_INFO("INVITE:[%s]", strChnID.data());
        if (m_pDevice)
        {
            pChannel = m_pDevice->GetChannel(strChnID);
            MLOG_ERROR("未找到对应Channel 1111111111:m_pDevice");
        }
        //获取设备
        auto device = DeviceManage::instance()->GetDevice(strChnID);
        if(device != nullptr)
        {
            pChannel = device->GetChannel(strChnID);
            MLOG_ERROR("未找到对应Channel 2222222:m_pDevice");
        }
        else
        {
            MLOG_ERROR("未找到对device:%s", strChnID.c_str());
        }
        
        if (pChannel == nullptr)
        {
            MLOG_ERROR("未找到对应Channel:[%s]", strChnID.data());
            SendInviteResponse(e, SIP_NOT_FOUND);
            return -3;
        }
        MLOG_ERROR("未找到对应Channel 1111111111:[%s]", strChnID.data());
    }
    else
    {
        MLOG_ERROR("请求uri解析错误");
        SendInviteResponse(e, SIP_BAD_REQUEST);
        MLOG_ERROR("未找到对应Channel 1111111111:[%s]", strDeviceID.data());
        return -4;
    }

    MLOG_INFO("成功解析INVITE的通道ID:[%s]", pChannel->GetChannelID().data());

    auto stSdpInfo = ::SDP::parseSdp(std::string(sdp_body->body, sdp_body->length));
    if (::SDP::Session_E::Play == stSdpInfo.enSessionType || ::SDP::Session_E::Talk == stSdpInfo.enSessionType)
    {
        MLOG_INFO("实时点播模式");
        /* 先通知上层进行发流，再做剩余操作，开启发流才会有本地编码器信息 */
        pChannel->UpdateMediaStatus(true);
    }
    if (stSdpInfo.stVideoConn.bHaveConnection)
    {
        MLOG_INFO("视频连接信息IP[%s]Port[%d]TCP[%d]IPv6[%d]Active[%d]SSRC[%s]",
                  stSdpInfo.stVideoConn.strIP.c_str(),
                  stSdpInfo.stVideoConn.nPort,
                  stSdpInfo.stVideoConn.bIsTcp,
                  stSdpInfo.stVideoConn.bIsIPV6,
                  stSdpInfo.stVideoConn.bTcpActive,
                  stSdpInfo.strSSRC.c_str());
    }
    if (stSdpInfo.stAudioConn.bHaveConnection)
    {
        MLOG_INFO("音频连接信息IP[%s]Port[%d]TCP[%d]IPv6[%d]Active[%d]SSRC[%s]",
                  stSdpInfo.stAudioConn.strIP.c_str(),
                  stSdpInfo.stAudioConn.nPort,
                  stSdpInfo.stAudioConn.bIsTcp,
                  stSdpInfo.stAudioConn.bIsIPV6,
                  stSdpInfo.stAudioConn.bTcpActive,
                  stSdpInfo.strSSRC.c_str());
    }

    /* 记录服务器中对应的CallID */
    std::string strCallID = "";
    if (pRequest->call_id)
    {
        pChannel->SetCallID(pRequest->call_id->number);
        strCallID = pRequest->call_id->number;
        MLOG_INFO("INVITE CallID:[%s]", pRequest->call_id->number);
    }
    else
    {
        MLOG_ERROR("解析INVITE CallID失败");
        SendInviteResponse(e, SIP_INTERNAL_SERVER_ERROR);
        return -5;
    }

    int nLocalPort = 0;
    /* NOTE
        1、点播（Play）逻辑和通道强相关绑定，存在收发双向操作
        2、回放（Playback）和下载（Download），只存在发送单向操作，统一在客户端层级管理即可
    */
    /* 上抛回放或下载的录像信息给上层 */
    if (::SDP::Session_E::Playback == stSdpInfo.enSessionType ||
        ::SDP::Session_E::Download == stSdpInfo.enSessionType)
    {
        MLOG_INFO("回放或下载模式");
        /* 先创建，后上抛 */
        { /* 创建会话 */
            auto pSession = std::make_shared<MediaSession>(strCallID, stSdpInfo.strSSRC, pChannel);
            pSession->Init(stSdpInfo.stVideoConn);
            e->m_pNetBase->AddSession(strCallID, pSession);
            nLocalPort = pSession->GetNetworkPort();
        }
        { /* 上抛读取的录制文件信息 */
            SipGetRecFile_S stGetInfo;
            stGetInfo.nStartTime = stSdpInfo.nStartTime;
            stGetInfo.nEndTime = stSdpInfo.nEndTime;
            stGetInfo.strCallID = strCallID;
            pChannel->SetGetRecFile(stGetInfo);
        }

#if CALL_EVENT_DEBUG
        MLOG_DEBUG("回放或下载模式，下载速度为:[%d]", stSdpInfo.nDownloadSpeed);
#endif
        /* 直接更新文件读取速度 */
        if (stSdpInfo.nDownloadSpeed > 0)
        {
            SipReadFileAction_S stAction;
            stAction.strCallID = strCallID;
            stAction.enAction = SIP_READFILE_SPEED;
            stAction.dSpeed = (double)stSdpInfo.nDownloadSpeed;
            pChannel->UpdateReadFileAction(stAction);
        }
    }
    else
    {
        MLOG_INFO("实时点播模式");
        /* 通知上层开启发流 */
        pChannel->UpdateMediaStatus(true);
        // /* note 默认udp模式 */
        // stSdpInfo.stVideoConn.bTcpActive = false;
        nLocalPort = pChannel->CreateClient(stSdpInfo.stVideoConn);
        /* 更新点播的相关数据 */
        pChannel->SetCallID(strCallID);
        /* 更新SSRC */
        pChannel->SetDefaultSSRC(stSdpInfo.strSSRC);
        /* 开启RTP和PS的封装 */
        pChannel->CreatePlayRtp(false, stSdpInfo.strSSRC);
    }
#if 1 //test
    nLocalPort = 1563;
    if (nLocalPort <= 0)
    {
        MLOG_ERROR("分配本地端口失败");
        SendInviteResponse(e, SIP_INTERNAL_SERVER_ERROR);
        if (::SDP::Session_E::Play == stSdpInfo.enSessionType)
        {
            /* 无法开启进行SIP会话，关闭本地发流 */
            pChannel->ClosePlayAction();
        }
        return -6;
    }

    using namespace SIP::SDP;
    SdpNegotiate_S stNegInfo;
    stNegInfo.strID = pRequest->req_uri->username;//pChannel->GetChannelID();
    stNegInfo.strIP = "172.16.25.143";//pChannel->GetExternIP();
    stNegInfo.nPort = nLocalPort;
    /* 暂时只添加音视频格式数据 */
    stNegInfo.stVideo.enType = (VideoType_E)(pChannel->GetVideoType());
    // auto stAudioInfo = pChannel->GetAudioInfo();
    stNegInfo.stAudio.enType = (AudioType_E)(stSdpInfo.stAudio.enType);
    stNegInfo.stAudio.enBit = (AudioBit_E)(stSdpInfo.stAudio.enBit);
    stNegInfo.stAudio.enSampleRate = (AudioSampleRate_E)(stSdpInfo.stAudio.enSampleRate);
#endif
    auto strNegSdp = negotiateSdp(stSdpInfo, stNegInfo);
#if CALL_EVENT_DEBUG
    MLOG_DEBUG("协商后的SDP报文:\n%s", strNegSdp.c_str());
#endif

    SendInviteResponse(e, SIP_OK, strNegSdp);
    return 0;
}

int SIP::CallEvent::HandleCallAction(const SipEvent::Ptr &e)
{
    /* NOTE 目前只处理实时流协议（MANSRTSP）命令集中的请求
     *       格式可参考 GB/T 28181-2022 附录B
     *       针对会话操作，和通道无关联
     */
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (nullptr == pClient)
    {
        MLOG_WARN("转换客户端实例失败");
        return -1;
    }
    /* 先查CallID对应的会话是否还存在 */
    std::string strCallID = e->m_pEvent->request->call_id->number;
    auto pSession = pClient->GetSession(strCallID);
    if (pSession == nullptr)
    {
        MLOG_INFO("[SipClient]会话不存在，忽略会话操作请求");
        return -2;
    }
    /* 提取操作报文内容 */
    osip_body_t *body = nullptr;
    std::string strAction;
    osip_message_get_body(e->m_pEvent->request, 0, &body);
    if (body)
    {
        MLOG_INFO("Call Info Action\n%s", body->body);
        strAction = body->body;
    }
    /* 调试不同指令解析 */
#if 0
    static bool bPause = false;
    if (bPause)
    {
        strAction = std::string("PLAY RTSP/1.0\r\nCSeq: 1\r\nRange: npt=now\r\n");
    }
    else
    {
        strAction = std::string("PAUSE RTSP/1.0\r\nCSeq: 1\r\nPauseTime: now\r\n");
    }
    bPause = !bPause;
#endif
    SipReadFileAction_S stReadAction;
    SipCbResult_S stResult;
    ::ParseReadFileAction(strAction, stReadAction);
    stReadAction.strCallID = strCallID;
    if (pClient->m_stCbInfo.fnReadFileAction)
    {
        pClient->m_stCbInfo.fnReadFileAction(stReadAction, stResult);
    }
    /* TODO 返回响应信息 */
    SendInviteResponse(e, SIP_OK);
    return 0;
}

void SIP::CallEvent::SendInviteResponse(const SipEvent::Ptr &e, int status, const std::string &sdp)
{
    osip_message_t *message = nullptr;
    int ret = OSIP_SUCCESS;

    ret = eXosip_call_build_answer(e->m_pContext, e->m_pEvent->tid, status, &message);
    if (ret != OSIP_SUCCESS)
    {
        /* 会出现重复接收到INVITE的情况？第二次就收到重复的INVITE时，这里会build失败，然后直接返回即可。 */
        MLOG_ERROR("eXosip_call_build_answer failed");
        return;
    }
    /* 设置SDP */
    if (!sdp.empty())
    {
        osip_message_set_content_type(message, "APPLICATION/SDP");
        osip_message_set_body(message, sdp.c_str(), sdp.length());
        MLOG_INFO("Call Response SDP:\n%s", sdp.c_str());
    }

    eXosip_lock(e->m_pContext);
    ret = eXosip_call_send_answer(e->m_pContext, e->m_pEvent->tid, status, message);
    eXosip_unlock(e->m_pContext);

    if (ret != OSIP_SUCCESS)
    {
        MLOG_ERROR("eXosip_call_send_answer failed");
        return;
    }
    MLOG_INFO("Call Response Status[%d]", status);
}
