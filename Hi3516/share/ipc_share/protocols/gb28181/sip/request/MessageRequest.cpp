/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 16:51:51
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-24 21:23:26
 * @FilePath     : MessageRequest.cpp
 * @Description  : 消息请求
 */
#include "MessageRequest.h"
#include "CallSession.h"
#include "MediaRtp.h"
#include "RequestPool.h"
#include "SipDevice.h"
#include "SipModule.h"
#include "SipServer.h"
#include "StreamManager.h"
#include "pugixml.hpp"
#include "gm.h"
#include <sstream>
using namespace SIP;

std::atomic_uint64_t MessageRequest::_sn = 0;

MessageRequest::MessageRequest(eXosip_t *ctx, Device::Ptr device, REQUEST_MESSAGE_TYPE type)
    : BaseRequest(ctx, device, type)
{
    _request_sn = _sn++;
}

MessageRequest::~MessageRequest()
{
}

int MessageRequest::SendMessage(bool needcb)
{
    if (m_pContext)
    {
        std::string from_uri;
        if(!GetDeviceUrl().empty())
        {
            from_uri = GetDeviceUrl();
        }
        else
        {
            from_uri = SipModule::instance()->GetSipUriByContext(m_pContext);
        }
        auto to_uri = _device->GetUri();
        MLOG_DEBUG("from_uri: %s", from_uri.c_str());
        MLOG_DEBUG("to_uri: %s", to_uri.c_str());
        if (from_uri.empty() || to_uri.empty())
        {
            MLOG_ERROR("URI为空 from_uri: %s to_uri: %s",
                       from_uri.c_str(), to_uri.c_str());
            return -1;
        }
        osip_message_t *msg = nullptr;
        auto ret = eXosip_message_build_request(m_pContext, &msg, "MESSAGE", to_uri.c_str(), from_uri.c_str(), nullptr);
        if (ret != OSIP_SUCCESS)
        {
            return ret;
        }

        /* NOTE 派生类自行实现需要发送的业务报文 */
        auto body = make_manscdp_body();
        body = format_xml(body);
        MLOG_DEBUG("Send Message Body: \n%s", body.c_str());
        osip_message_set_body(msg, body.c_str(), body.length());
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");

        /*gb35114构建控制信令 Date、Note字段*/
        if (OK != CGm::instance()->gm_build_control_signaling_note(msg, body.c_str()))
        {
            MLOG_ERROR("gb35114构建控制信令 Date、Note字段失败");
        }

        eXosip_lock(m_pContext);
        ret = eXosip_message_send_request(m_pContext, msg);
        eXosip_unlock(m_pContext);

        if (needcb)
        {
            std::string request_id = _get_request_id_from_request(msg);
            if (request_id.length() > 0)
            {
                BaseRequest::Ptr request = shared_from_this();
                RequestPool::instance()->AddRequest(request_id, request);
            }
        }
        return 0;
    }

    return -1;
}

const std::string MessageRequest::GetRequestSN()
{
    return std::to_string(_request_sn);
}

const std::string MessageRequest::GetDeviceUrl()
{
    return std::string();
}

const std::string MessageRequest::make_manscdp_body()
{
    return std::string();
}

std::string MessageRequest::format_xml(const std::string &xml)
{
    pugi::xml_document doc;
    auto ret = doc.load_string(xml.c_str());
    if (ret.status != pugi::status_ok)
    {
        return xml;
    }

    std::stringstream ss;
    doc.save(ss);

    return ss.str();
}

const std::string SIP::InviteRequest::make_sdp_body()
{
    // 测试，只有将t放在前边才能够相机被识别到，不然会返回 415 Unsupported Media Type

    /*
        根据SDP（Session Description Protocol）的规范，SDP 中的各个字段其实是没有顺序要求的。
        每一行的含义都是独立的，可以按任意顺序排列。
        但是，为了方便阅读和解析，通常会按照规定的顺序来排列。
            v= (协议版本)
            o= (创建者和会话标识符)
            s= (会话名称)
            i=* (会话信息)
            u=* (描述的 URI)
            e=* (电子邮件地址)
            p=* (电话号码)
            c=* (连接信息 - 如果在所有媒体中都包含则不需要)
            b=* (带宽信息)
            t=* (会话起止时间)
            r=* (重复时间)
            z=* (时区调整)
            k=* (加密密钥)
            a=* (零个或多个会话属性行)
            Zero or more media descriptions
    */
    std::stringstream ss;
    ss << "v=0\r\n";
    ss << "o=" << m_stRequestParam.strChnID << " 0 0 IN IP4 " << m_stRequestParam.strLocalIP << "\r\n";
    if (Playback == m_stRequestParam.enType)
    {
        ss << "s=Playback\r\n";
    }
    else if (Download == m_stRequestParam.enType)
    {
        ss << "s=Download\r\n";
    }
    else
    {
        ss << "s=Play\r\n";
    }
    ss << "c=IN IP4 " << m_stRequestParam.strLocalIP << "\r\n";
    ss << "t=" << m_stRequestParam.nStartTime << " " << m_stRequestParam.nEndTime << "\r\n";
    /* NOTE RTP/AVP为默认UDP协议，TCP/RTP/AVP为TCP协议，只采用PS封装格式 */
    ss << "m=video " << m_stRequestParam.nLocalPort << " ";/* 保留空格 */
    if (m_stRequestParam.bUsingTcp)
    {
        ss << "TCP/";
    }

    ss <<"RTP/AVP 96\r\n";
    ss << "a=recvonly\r\n"; /* 仅接收，不发送数据 */
    if (m_stRequestParam.bUsingTcp && m_stRequestParam.bTcpActive)
    {
        ss << "a=setup:active\r\n"; /* TCP协议必须设置为主动连接，否则无法播放 */
    }
    else
    {
        ss << "a=setup:passive\r\n"; /* TCP协议必须设置为被动连接，否则无法播放 */
    }
    ss << "a=connection:new\r\n"; /* 开启新连接，不复用旧连接 */
    ss << "a=rtpmap:96 PS/90000\r\n";
    // ss << "a=rtpmap:97 MPEG4/90000\r\n";
    // ss << "a=rtpmap:98 H264/90000\r\n";
    // ss << "a=rtpmap:99 H265/90000\r\n";
    ss << "y=" << m_stRequestParam.strSSRC << "\r\n";
    return ss.str();
}

int InviteRequest::SendCall(bool needcb)
{

    osip_message_t *msg = nullptr;
    auto nRet = eXosip_call_build_initial_invite(
        m_pContext,
        &msg,
        m_stRequestParam.strToUri.c_str(),
        m_stRequestParam.strFromUri.c_str(),
        m_stRequestParam.strRoute.c_str(),
        m_stRequestParam.strSubject.c_str());
    if (nRet != OSIP_SUCCESS)
    {
        MLOG_ERROR("构建INVITE请求失败，错误值为: [%d]", nRet);
        return -1;
    }

    _ssrc_info = std::make_shared<SSRCInfo>(
        m_stRequestParam.nLocalPort,
        m_stRequestParam.strSSRC,
        m_stRequestParam.strStreamID);
    auto session = std::make_shared<CallSession>(
        m_pContext,
        "rtp",
        m_stRequestParam.strStreamID,
        _ssrc_info);
    StreamManager::instance()->AddStream(session);

    auto sdp_body = make_sdp_body();

    osip_message_set_body(msg, sdp_body.c_str(), sdp_body.length());
    osip_message_set_content_type(msg, "APPLICATION/SDP");

    eXosip_lock(m_pContext);
    int call_id = eXosip_call_send_initial_invite(m_pContext, msg);
    eXosip_unlock(m_pContext);

    if (call_id > 0)
    {
        MLOG_INFO("eXosip_call_send_initial_invite: [%d]", call_id);
    }
    session->SetCallID(call_id);
    session->exosip_context = m_pContext;
    MLOG_INFO("==================================SDP: \n%s", sdp_body.c_str());

    if (needcb)
    {
        std::string request_id = _get_request_id_from_request(msg);
        if (!request_id.empty())
        {
            BaseRequest::Ptr request = shared_from_this();
            RequestPool::instance()->AddRequest(request_id, request);
        }
    }    

    return 0;
}

int InviteAudioRequest::SendCall(bool needcb)
{

    osip_message_t *msg = nullptr;
    auto nRet = eXosip_call_build_initial_invite(
        m_pContext,
        &msg,
        m_stRequestParam.strToUri.c_str(),
        m_stRequestParam.strFromUri.c_str(),
        m_stRequestParam.strRoute.c_str(),
        m_stRequestParam.strSubject.c_str());
    if (nRet != OSIP_SUCCESS)
    {
        MLOG_ERROR("构建INVITE请求失败，错误值为: [%d]", nRet);
        return -1;
    }

    _ssrc_info = std::make_shared<SSRCInfo>(
        m_stRequestParam.nLocalPort,
        m_stRequestParam.strSSRC,
        m_stRequestParam.strStreamID);
    auto session = std::make_shared<CallSession>(
        m_pContext,
        "rtp",
        m_stRequestParam.strStreamID,
        _ssrc_info);
    StreamManager::instance()->AddStream(session);

    auto sdp_body = make_sdp_body();

    osip_message_set_body(msg, sdp_body.c_str(), sdp_body.length());
    osip_message_set_content_type(msg, "APPLICATION/SDP");

    eXosip_lock(m_pContext);
    int call_id = eXosip_call_send_initial_invite(m_pContext, msg);
    eXosip_unlock(m_pContext);

    if (call_id > 0)
    {
        MLOG_INFO("eXosip_call_send_initial_invite: [%d]", call_id);
    }
    session->SetCallID(call_id);
    session->exosip_context = m_pContext;
    MLOG_INFO("==================================SDP: \n%s", sdp_body.c_str());

    if (needcb)
    {
        std::string request_id = _get_request_id_from_request(msg);
        if (!request_id.empty())
        {
            BaseRequest::Ptr request = shared_from_this();
            RequestPool::instance()->AddRequest(request_id, request);
        }
    }    

    return 0;
}

const std::string SIP::InviteAudioRequest::make_sdp_body()
{
    // 测试，只有将t放在前边才能够相机被识别到，不然会返回 415 Unsupported Media Type

    /*
        根据SDP（Session Description Protocol）的规范，SDP 中的各个字段其实是没有顺序要求的。
        每一行的含义都是独立的，可以按任意顺序排列。
        但是，为了方便阅读和解析，通常会按照规定的顺序来排列。
            v= (协议版本)
            o= (创建者和会话标识符)
            s= (会话名称)
            i=* (会话信息)
            u=* (描述的 URI)
            e=* (电子邮件地址)
            p=* (电话号码)
            c=* (连接信息 - 如果在所有媒体中都包含则不需要)
            b=* (带宽信息)
            t=* (会话起止时间)
            r=* (重复时间)
            z=* (时区调整)
            k=* (加密密钥)
            a=* (零个或多个会话属性行)
            Zero or more media descriptions
    */
    std::stringstream ss;
    ss << "v=0\r\n";
    ss << "o=" << m_stRequestParam.strChnID << " 0 0 IN IP4 " << m_stRequestParam.strLocalIP << "\r\n";
    if (Playback == m_stRequestParam.enType)
    {
        ss << "s=Playback\r\n";
    }
    else if (Download == m_stRequestParam.enType)
    {
        ss << "s=Download\r\n";
    }
    else
    {
        ss << "s=Play\r\n";
    }
    ss << "c=IN IP4 " << m_stRequestParam.strLocalIP << "\r\n";
    ss << "t=" << m_stRequestParam.nStartTime << " " << m_stRequestParam.nEndTime << "\r\n";
    /* NOTE RTP/AVP为默认UDP协议，TCP/RTP/AVP为TCP协议，只采用PS封装格式 */
    ss << "m=audio " << m_stRequestParam.nLocalPort << " ";/* 保留空格 */
    if (m_stRequestParam.bUsingTcp)
    {
        ss << "TCP/";
    }

    ss <<"RTP/AVP 8 96\r\n";
    ss << "a=recvonly\r\n"; /* 仅接收，不发送数据 */
    ss << "a=rtpmap:8 PCMA/8000\r\n";
    ss << "a=rtpmap:96 PS/90000\r\n";
    //ss << "a=rtpmap:97 MPEG4/90000\r\n";
    // ss << "a=rtpmap:98 H264/90000\r\n";
    // ss << "a=rtpmap:99 H265/90000\r\n";
    ss << "y=" << m_stRequestParam.strSSRC << "\r\n";
    ss << "f=v/////a/1/8/1\r\n";
    return ss.str();
}
