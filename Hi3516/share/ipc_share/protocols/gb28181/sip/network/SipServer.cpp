/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 16:04:16
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-23 20:09:49
 * @FilePath     : SipServer.cpp
 * @Description  : SIP服务器
 */

#include "SipServer.h"
#include "dlog.h"
#include "SipUtils.h"

using namespace SIP;
bool SipServer::Init(SipServerInfo_S stInfo)
{
    m_stServerInfo = stInfo;
    /* NOTE 检查部分必要配置是否已经配齐 */
    if (m_stServerInfo.strNonce.empty())
    {
        /* 自动生成一个随机字符串作为标识符 */
        m_stServerInfo.strNonce = ::GenerateRandomString(16);
        dlog_debug("SIP服务器Nonce配置为空，随机生成[%s]",
                   m_stServerInfo.strNonce.c_str());
    }
    return true;
}

bool SipServer::Start()
{
    m_pSipContext = eXosip_malloc();
    if (OSIP_SUCCESS != eXosip_init(m_pSipContext))
    {
        dlog_error("eXosip_init failed");
        return false;
    }
    /* 设置代理名称 */
    eXosip_set_user_agent(m_pSipContext, m_strSipAgent.c_str());

    /* 根据初始化标记位来区分监听TCP/UDP协议 */
    int nRet = eXosip_listen_addr(
        m_pSipContext,
        m_stServerInfo.bTcp ? IPPROTO_TCP : IPPROTO_UDP,
        nullptr,
        m_stServerInfo.nPort, AF_INET, 0);
    if (nRet != OSIP_SUCCESS)
    {
        dlog_error("eXosip_listen_addr failed");
        eXosip_quit(m_pSipContext);
        return false;
    }
    m_bThreadRun = true;
    m_thrListen = std::make_shared<std::thread>(&SipServer::RecvEventThread, this);
    dlog_info("SipServer Start IP[%s] Port[%d] Tcp[%d] Pw[%s] Ret[%d]",
              m_stServerInfo.strIP.c_str(),
              m_stServerInfo.nPort,
              m_stServerInfo.bTcp,
              m_stServerInfo.strPassword.c_str(),
              nRet);
    return true;
}

bool SipServer::Stop()
{
    /* 服务器实例必须在业务中最后进行释放，存在线程安全问题 */
    m_bThreadRun = false;

    if (m_thrListen && m_thrListen->joinable())
    {
        m_thrListen->join();
        m_thrListen = nullptr;
    }

    dlog_info("SipServer Stop");
    eXosip_quit(m_pSipContext);
    m_pSipContext = nullptr;
    return true;
}

int SIP::SipServer::on_message_new(const SipEvent::Ptr &e)
{
    eXosip_event_t *exosip_event = e->m_pEvent;
    dlog_info("SipServer Message New Method [%s]",
              exosip_event->request->sip_method);
    if (MSG_IS_REGISTER(exosip_event->request))
    {
        m_stRegisterDeal.HandleIncomingRequest(e);
    }
    else if (MSG_IS_MESSAGE(exosip_event->request))
    {
        m_stMsgDeal.HandleIncomingRequest(e);
    }
    else if (MSG_IS_BYE(exosip_event->request))
    {
    }
    else
    {
    }

    return 0;
}

int SIP::SipServer::on_call_message_answered(const SipEvent::Ptr &e)
{
    auto *exosip_event = e->m_pEvent;
    if (MSG_IS_BYE(exosip_event->request))
    {
        m_stCallDeal.HandleClose(e);
    }
    return 0;
}

SipEvent::Ptr SipServer::new_event(
    eXosip_t *exosip_context,
    eXosip_event_t *exosip_event)
{
    if (exosip_event == nullptr)
    {
        return nullptr;
    }
    if (exosip_event->type < EXOSIP_REGISTRATION_SUCCESS ||
        exosip_event->type > EXOSIP_NOTIFICATION_GLOBALFAILURE)
    {
        return nullptr;
    }

    /* 匹配对应事件的处理函数 */
    SipEvent::Ptr pEvent = std::make_shared<SipEvent>();
    auto stEventDeal = GetEventDeal(exosip_event->type);
    if (stEventDeal.achName == nullptr || stEventDeal.fnDeal == nullptr)
    {
        return nullptr;
    }

    size_t nLength = 0;
    dlog_info("[SipServer]处理事件: [%d][%s][%s]",
              exosip_event->type, stEventDeal.achName, exosip_event->textinfo);
#if 1 /* DEBUG 调试输出SIP消息 */
    if (exosip_event->request)
    {
        char *pRequest = nullptr;
        osip_message_to_str(exosip_event->request, &pRequest, &nLength);
        dlog_debug("=============>>[%d] Request:\n%s", exosip_event->type, pRequest)
        osip_free(pRequest);
    }

    if (exosip_event->response)
    {
        char *pResponse = nullptr;
        osip_message_to_str(exosip_event->response, &pResponse, &nLength);
        dlog_debug("=============>>[%d] Response:\n%s", exosip_event->type, pResponse)
        osip_free(pResponse);
    }

    if (exosip_event->ack)
    {
        char *pAck = nullptr;
        osip_message_to_str(exosip_event->ack, &pAck, &nLength);
        dlog_debug("=============>>[%d] Ack:\n%s", exosip_event->type, pAck)
        osip_free(pAck);
    }
#endif
    pEvent->m_nValue = exosip_event->type;
    pEvent->m_achName = stEventDeal.achName;
    pEvent->m_fnDeal = stEventDeal.fnDeal;
    pEvent->m_pContext = exosip_context;
    pEvent->m_pEvent = exosip_event;
    pEvent->m_nID = m_nEventID++;
    /* 将当前服务器实例传进去进行调用 */
    pEvent->m_pNetBase = this;
    pEvent->m_pUser = nullptr;
    return pEvent;
}

int SipServer::on_call_invite(const SipEvent::Ptr &e)
{
    //m_stCallDeal.SetDeivce(m_pLocalDev);
    return m_stCallDeal.HandleIncomingAudioRequest(e);
}
