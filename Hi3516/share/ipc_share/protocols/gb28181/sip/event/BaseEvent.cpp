/*
 * @Author       : EasonLu
 * @Date         : 2025-02-13 10:35:11
 * @LastEditors  : EasonLu
 * @LastEditTime: 2025-05-10 18:33:39
 * @FilePath     : BaseEvent.cpp
 * @Description  : 事件基类实现
 */
#include "BaseEvent.h"
#include "SipClient.h"
#include "SipUtils.h"
#include "XmlParser.h"
#include "gm.h"
#include <cstring>
#include <sstream>

#define SIP_BASE_EVENT_DEBUG 0
using namespace SIP;

bool BaseEvent::Handle(const SipEvent::Ptr &e, pugi::xml_document &doc)
{
    return false;
}

int BaseEvent::SendResponse(const SipEvent::Ptr &e, int nStatus)
{
    osip_message_t *answer = nullptr;
    eXosip_lock(e->m_pContext);
    eXosip_message_build_answer(
        e->m_pContext, e->m_pEvent->tid, nStatus, &answer);
    int ret = eXosip_message_send_answer(
        e->m_pContext, e->m_pEvent->tid, nStatus, nullptr);
    char *pMsg = nullptr;
    size_t nLen = 0;
    osip_message_to_str(answer, &pMsg, &nLen);
    MLOG_INFO("SIP Response: \n%s", pMsg);
    osip_free(pMsg);
    eXosip_unlock(e->m_pContext);
    return ret;
}

std::string  SIP::BaseEvent::format_xml(const std::string &xml)
{
    pugi::xml_document doc;
    auto ret = doc.load_string(xml.c_str());
    if (ret.status != pugi::status_ok)
    {
        return xml;
    }

    std::stringstream ss;
    doc.save(ss,"  ", pugi::format_indent);

    return ss.str();
}

int SIP::BaseEvent::SendMessage(
    const SipEvent::Ptr &e,
    const std::string &strBody)
{
    /* NOTE 发送的Message都是独立于任何会话中的 */
    osip_message_t *message = nullptr;
    std::string strFromURI, strToURI;
    GetRequestURI(e, strFromURI, strToURI);
    eXosip_message_build_request(
        e->m_pContext,
        &message,
        "MESSAGE",
        strFromURI.c_str(), /* 发送的ToURI为接收的FromURI */
        strToURI.c_str(),   /* 发送的FromURI为接收的ToURI */
        nullptr);
    if (!strBody.empty())
    {
        osip_message_set_content_type(message, "Application/MANSCDP+xml");
        osip_message_set_body(message, strBody.c_str(), strBody.length());
    }
    eXosip_lock(e->m_pContext);
    int ret = eXosip_message_send_request(e->m_pContext, message);
    if (OSIP_SUCCESS == ret)
    {
        char *pMsg = nullptr;
        size_t nLen = 0;
        osip_message_to_str(message, &pMsg, &nLen);
        MLOG_INFO("SIP Message: \n%s", pMsg);
        osip_free(pMsg);
    }
    eXosip_unlock(e->m_pContext);
    return 0;
}

int SIP::BaseEvent::SendMessageWithCallID(
    const SipEvent::Ptr &e,
    const std::string &strBody)
{
    /* NOTE 统一Message中的Call-ID，将除了200应答以外的Message的应答串联起来 */
    osip_message_t *message = nullptr;
    std::string strFromURI, strToURI;
    GetRequestURI(e, strFromURI, strToURI);

    osip_message_init(&message);
    eXosip_message_build_request(
        e->m_pContext,
        &message,
        "MESSAGE",
        strFromURI.c_str(), /* 发送的ToURI为接收的FromURI */
        strToURI.c_str(),   /* 发送的FromURI为接收的ToURI */
        nullptr);
    std::string Body = strBody;
    if (!strBody.empty())
    {
        Body = format_xml(Body);
        osip_message_set_content_type(message, "Application/MANSCDP+xml");
        osip_message_set_body(message, Body.c_str(), Body.length());
    }
    /* 设置原先请求的Call-ID */
    osip_call_id_t *pOldCallID = osip_message_get_call_id(e->m_pEvent->request);
    if (pOldCallID)
    {
#if SIP_BASE_EVENT_DEBUG
        MLOG_DEBUG("Old Call-ID: [%s][%s]", pOldCallID->number, pOldCallID->host);
#endif
    }
    auto pNewCallID = osip_message_get_call_id(message);
    if (pNewCallID)
    {
#if SIP_BASE_EVENT_DEBUG
        MLOG_DEBUG("New Call-ID: [%s][%s]", pNewCallID->number, pNewCallID->host);
#endif
        /* 需要先释放eXosip_message_build_request时申请的Call-ID */
        osip_call_id_free(pNewCallID);
    }
    /* 直接克隆原先请求的Call-ID */
    osip_call_id_clone(pOldCallID, &message->call_id);
    
    /* gb35114构建控制信令 Date、Note字段 必须在更改Call-ID之后 */
    if (OK != CGm::instance()->gm_build_control_signaling_note(message, Body.c_str()))
    {
        MLOG_ERROR("gb35114构建控制信令 Date、Note字段失败");
    }

    eXosip_lock(e->m_pContext);
    int ret = eXosip_message_send_request(e->m_pContext, message);
    if (OSIP_SUCCESS == ret)
    {
#if SIP_BASE_EVENT_DEBUG
        char *pMsg = nullptr;
        size_t nLen = 0;
        osip_message_to_str(message, &pMsg, &nLen);
        MLOG_INFO("SIP Message: \n%s", pMsg);
        osip_free(pMsg);
#endif
    }
    eXosip_unlock(e->m_pContext);
    return 0;
}

int BaseEvent::SendResponseAndGetAddress(eXosip_t *excontext, int tid, int status, std::string &address, uint16_t &port)
{
    osip_message_t *answer = nullptr;

    eXosip_lock(excontext);
    eXosip_message_build_answer(excontext, tid, status, &answer);

    char addr[50] = {0};

    osip_via_t *via = nullptr;
    osip_message_get_via(answer, 0, &via);

    if (via && via->host)
    {
        osip_generic_param_t *param = nullptr;
        osip_via_param_get_byname(via, (char *)"received", &param);
        if (param && param->gvalue)
        {
            snprintf(addr,sizeof(addr), "%s",param->gvalue);
        }
        else
        {
            snprintf(addr,sizeof(addr), "%s",via->host);
        }
        address = std::string(addr);

        osip_via_param_get_byname(via, (char *)"rport", &param);
        if (param && param->gvalue)
        {
            port = std::stoi(param->gvalue);
        }
    }

    // 设置时间
    time_t timep;
    time(&timep);
    std::string timeT = CurrentTimeTToISO8601();
    osip_message_set_date((osip_message_t *)answer, timeT.c_str());

    int ret = eXosip_message_send_answer(excontext, tid, status, answer);
    char *pMsg = nullptr;
    size_t nLen = 0;
    osip_message_to_str(answer, &pMsg, &nLen);
    MLOG_DEBUG("SIP Response: \n%s", pMsg);
    eXosip_unlock(excontext);
    return ret;
}

int BaseEvent::SendCallAck(eXosip_t *excontext, int did)
{
    osip_message_t *ack = nullptr;
    eXosip_lock(excontext);
    eXosip_call_build_ack(excontext, did, &ack);
    int ret = eXosip_call_send_ack(excontext, did, ack);
    eXosip_unlock(excontext);
    return ret;
}

int BaseEvent::GetStatusCodeFromResponse(osip_message_t *response)
{
    return response != nullptr ? response->status_code : -1;
}

std::string BaseEvent::GetMsgIDFromRequest(osip_message_t *request)
{
    osip_generic_param_t *tag = nullptr;
    // osip_to_get_tag(request->from, &tag);
    if (nullptr == request)
    {
        return "";
    }
    osip_uri_param_get_byname(&request->from->gen_params, (char *)"tag", &tag);

    if (nullptr == tag || nullptr == tag->gvalue)
    {
        return "";
    }
    return tag->gvalue;
}

int SIP::BaseEvent::SendResponse(
    eXosip_t *excontext,
    const std::string &strToUri,
    const std::string &strFromUri,
    const std::string &strBody)
{
    osip_message_t *request = nullptr;
    auto ret = eXosip_message_build_request(
        excontext, &request, "MESSAGE", strToUri.c_str(), strFromUri.c_str(), nullptr);
    if (ret != OSIP_SUCCESS)
    {
        MLOG_ERROR("eXosip_message_build_request failed");
        return ret;
    }

    osip_message_set_content_type(request, "Application/MANSCDP+xml");
    osip_message_set_body(request, strBody.c_str(), strBody.length());

    eXosip_lock(excontext);
    ret = eXosip_message_send_request(excontext, request);
    eXosip_unlock(excontext);
    if (ret < OSIP_SUCCESS)
    {
        MLOG_ERROR("eXosip_message_send_request failed");
    }
    return ret;
}

int SIP::BaseEvent::GetExpires(const SipEvent::Ptr &e)
{
    /* NOTE SIP消息头中的Expires字段为注册有效时间（单位：秒） */
    char achExpires[] = "Expires";
    /* Expires字段为0时，则为注销 */
    int nRetExpires = -1;
    /* NOTE 先从Content字段中获取，获取不到则从Headers字段中获取 */
    osip_uri_param_t *tag = nullptr;
    /* 默认提取第一个Contact字段 */
    osip_contact_t *pGetContact = nullptr;
    int nPos = 0;
    /* 从Contact字段读取Expires数据 */
    while (osip_message_get_contact(e->m_pEvent->request, nPos++, &pGetContact) == OSIP_SUCCESS)
    {
        if (nullptr == pGetContact)
        {
            continue;
        }
        osip_uri_param_get_byname(&(pGetContact->gen_params), achExpires, &tag);
        if (nullptr != tag)
        {
            MLOG_DEBUG("Get Expires Tag From Contacts Value:%s", tag->gvalue);
            nRetExpires = atoi(tag->gvalue);
        }
    }
    /* 如果获取不到则从Headers的Expires独立字段中获取 */
    if (nRetExpires < 0)
    {
        MLOG_INFO("Get Expires Tag From Contacts Failed");
        osip_uri_param_get_byname(&(e->m_pEvent->request->headers), achExpires, &tag);
        if (nullptr != tag)
        {
            MLOG_DEBUG("Get Expires Tag From Headers Value:%s", tag->gvalue);
            nRetExpires = atoi(tag->gvalue);
        }
    }
    return nRetExpires;
}

int SIP::BaseEvent::GetEvent(const SipEvent::Ptr &e, std::string &strEvent)
{
    int bRet = -1;
    char achEvent[] = "Event";
    osip_uri_param_t *tag = nullptr;
    osip_uri_param_get_byname(&(e->m_pEvent->request->headers), achEvent, &tag);
    if (nullptr != tag)
    {
        MLOG_DEBUG("Get Event Tag From Headers Value:%s", tag->gvalue);
        strEvent = tag->gvalue;
        bRet = 1;
    }
    return bRet;
}

int SIP::BaseEvent::ParseHeader(
    const SipEvent::Ptr &e)
{
    osip_body_t *body = nullptr;
    osip_message_get_body(e->m_pEvent->request, 0, &body);
    if (body == nullptr)
    {
        SendResponse(e, SIP_BAD_REQUEST);
        return -1;
    }

    auto ret = XmlParser::Parse(body->body, (int)body->length, m_doc);
    if (!ret)
    {
        SendResponse(e, SIP_BAD_REQUEST);
        return -1;
    }

    ret = XmlParser::ParseHeader(m_header, m_doc);
    if (!ret)
    {
        SendResponse(e, SIP_BAD_REQUEST);
        return -1;
    }
    return 0;
}

int SIP::BaseEvent::GetRequestURI(
    const SipEvent::Ptr &e,
    std::string &strFromUri,
    std::string &strToUri)
{
    char *pFromUri = nullptr;
    char *pToUri = nullptr;
    auto pRequest = e->m_pEvent->request;
    if (nullptr == pRequest)
    {
        return -1;
    }

    /* 假设来自上级的事件，From URI直接从SipClient中获取 */
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient != nullptr)
    {
        strFromUri = pClient->GetRemoteURI();
#if SIP_BASE_EVENT_DEBUG
        MLOG_INFO("Get RequestURI SipClient URI: %s", strFromUri.c_str());
#endif
    }

    /* 非上级事件，或获取失败时，则从From字段中获取 */
    if (pClient == nullptr || strFromUri.empty())
    {
        /* FIXME 存在From URI中缺少IP和端口的情况，只能从最新一级的Via字段中获取 */
        if (pRequest->from != nullptr && pRequest->from->url != nullptr)
        {
            osip_uri_to_str(e->m_pEvent->request->from->url, &pFromUri);
            if (pFromUri != nullptr)
            {
                strFromUri = std::string(pFromUri);
            }
            osip_free(pFromUri);
#if SIP_BASE_EVENT_DEBUG
            MLOG_INFO("Get RequestURI From URI: %s", strFromUri.c_str());
#endif
        }
    }

    if (pRequest->to != nullptr && pRequest->to->url != nullptr)
    {
        osip_uri_to_str(e->m_pEvent->request->to->url, &pToUri);
        if (pToUri != nullptr)
        {
            strToUri = std::string(pToUri);
        }
        osip_free(pToUri);
#if SIP_BASE_EVENT_DEBUG
        MLOG_INFO("Get RequestURI To URI: %s", strToUri.c_str());
#endif
    }
    return 0;
}
