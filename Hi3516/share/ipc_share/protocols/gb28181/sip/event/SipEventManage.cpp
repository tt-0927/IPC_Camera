#include "SipEventManage.h"
#include "SipClient.h"
#include "SipServer.h"
#include "SipType.h"

#define CALLBACK_TEMPLATE(F) (std::bind(&EventManage::F, this, std::placeholders::_1))

using namespace SIP;

EventManage::EventManage()
{
    /* 事件类型与回调函数映射表 */
    EventDeal eventProcTable[EXOSIP_EVENT_COUNT] = {
        {"REGISTRATION_SUCCESS", CALLBACK_TEMPLATE(on_registration_success)},
        {"REGISTRATION_FAILURE", CALLBACK_TEMPLATE(on_registration_failure)},
        {"CALL_INVITE", CALLBACK_TEMPLATE(on_call_invite)},
        {"CALL_REINVITE", CALLBACK_TEMPLATE(on_call_reinvite)},
        {"CALL_NOANSWER", CALLBACK_TEMPLATE(on_call_noanswer)},
        {"CALL_PROCEEDING", CALLBACK_TEMPLATE(on_call_proceeding)},
        {"CALL_RINGING", CALLBACK_TEMPLATE(on_call_ringing)},
        {"CALL_ANSWERED", CALLBACK_TEMPLATE(on_call_answered)},
        {"CALL_REDIRECTED", CALLBACK_TEMPLATE(on_call_redirected)},
        {"CALL_REQUESTFAILURE", CALLBACK_TEMPLATE(on_call_requestfailure)},
        {"CALL_SERVERFAILURE", CALLBACK_TEMPLATE(on_call_serverfailure)},
        {"CALL_GLOBALFAILURE", CALLBACK_TEMPLATE(on_call_globalfailure)},
        {"CALL_ACK", CALLBACK_TEMPLATE(on_call_ack)},
        {"CALL_CANCELLED", CALLBACK_TEMPLATE(on_call_cancelled)},
        {"CALL_MESSAGE_NEW", CALLBACK_TEMPLATE(on_call_message_new)},
        {"CALL_MESSAGE_PROCEEDING", CALLBACK_TEMPLATE(on_call_message_proceeding)},
        {"CALL_MESSAGE_ANSWERED", CALLBACK_TEMPLATE(on_call_message_answered)},
        {"CALL_MESSAGE_REDIRECTED", CALLBACK_TEMPLATE(on_call_message_redirected)},
        {"CALL_MESSAGE_REQUESTFAILURE", CALLBACK_TEMPLATE(on_call_message_requestfailure)},
        {"CALL_MESSAGE_SERVERFAILURE", CALLBACK_TEMPLATE(on_call_message_serverfailure)},
        {"CALL_MESSAGE_GLOBALFAILURE", CALLBACK_TEMPLATE(on_call_message_globalfailure)},
        {"CALL_CLOSED", CALLBACK_TEMPLATE(on_call_closed)},
        {"CALL_RELEASED", CALLBACK_TEMPLATE(on_call_released)},
        {"MESSAGE_NEW", CALLBACK_TEMPLATE(on_message_new)},
        {"MESSAGE_PROCEEDING", CALLBACK_TEMPLATE(on_message_proceeding)},
        {"MESSAGE_ANSWERED", CALLBACK_TEMPLATE(on_message_answered)},
        {"MESSAGE_REDIRECTED", CALLBACK_TEMPLATE(on_message_redirected)},
        {"MESSAGE_REQUESTFAILURE", CALLBACK_TEMPLATE(on_message_requestfailure)},
        {"MESSAGE_SERVERFAILURE", CALLBACK_TEMPLATE(on_message_serverfailure)},
        {"MESSAGE_GLOBALFAILURE", CALLBACK_TEMPLATE(on_message_globalfailure)},
        {"SUBSCRIPTION_NOANSWER", CALLBACK_TEMPLATE(on_subscription_noanswer)},
        {"SUBSCRIPTION_PROCEEDING", CALLBACK_TEMPLATE(on_subscription_proceeding)},
        {"SUBSCRIPTION_ANSWERED", CALLBACK_TEMPLATE(on_subscription_answered)},
        {"SUBSCRIPTION_REDIRECTED", CALLBACK_TEMPLATE(on_subscription_redirected)},
        {"SUBSCRIPTION_REQUESTFAILURE", CALLBACK_TEMPLATE(on_subscription_requestfailure)},
        {"SUBSCRIPTION_SERVERFAILURE", CALLBACK_TEMPLATE(on_subscription_serverfailure)},
        {"SUBSCRIPTION_GLOBALFAILURE", CALLBACK_TEMPLATE(on_subscription_globalfailure)},
        {"SUBSCRIPTION_NOTIFY", CALLBACK_TEMPLATE(on_subscription_notify)},
        {"IN_SUBSCRIPTION_NEW", CALLBACK_TEMPLATE(on_in_subscription_new)},
        {"NOTIFICATION_NOANSWER", CALLBACK_TEMPLATE(on_notification_noanswer)},
        {"NOTIFICATION_PROCEEDING", CALLBACK_TEMPLATE(on_notification_proceeding)},
        {"NOTIFICATION_ANSWERED", CALLBACK_TEMPLATE(on_notification_answered)},
        {"NOTIFICATION_REDIRECTED", CALLBACK_TEMPLATE(on_notification_redirected)},
        {"NOTIFICATION_REQUESTFAILURE", CALLBACK_TEMPLATE(on_notification_requestfailure)},
        {"NOTIFICATION_SERVERFAILURE", CALLBACK_TEMPLATE(on_notification_serverfailure)},
        {"NOTIFICATION_GLOBALFAILURE", CALLBACK_TEMPLATE(on_notification_globalfailure)}};

    for (uint32_t i = 0; i < EXOSIP_EVENT_COUNT; ++i)
    {
        m_mapEventDeal.insert(std::make_pair(i, eventProcTable[i]));
    }
}

EventManage::EventDeal EventManage::GetEventDeal(eXosip_event_type_t type)
{
    auto value = m_mapEventDeal.find(type);
    if (value == m_mapEventDeal.end())
    {
        EventDeal stDeal = {nullptr, nullptr};
        MLOG_ERROR("GetEventDeal error , error type [%d]", type);
        return stDeal;
    }
    MLOG_INFO("GetEventDeal type [%d]", type);
    return value->second;
}

int EventManage::on_registration_success(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_registration_failure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_invite(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_reinvite(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_noanswer(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_proceeding(const SipEvent::Ptr &e)
{
    m_stCallDeal.on_proceeding(e);

    return 0;
}

int EventManage::on_call_ringing(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_answered(const SipEvent::Ptr &e)
{
    m_stCallDeal.HandleResponseSuccess(e);

    return 0;
}

int EventManage::on_call_redirected(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_requestfailure(const SipEvent::Ptr &e)
{
    if (e->m_pEvent->response)
    {
        MLOG_ERROR("on_call_requestfailure response code [%d][%s]",
                   e->m_pEvent->response->status_code,
                   e->m_pEvent->response->reason_phrase);
    }
    return 0;
}

int EventManage::on_call_serverfailure(const SipEvent::Ptr &e)
{
    if (e->m_pEvent->response)
    {
        MLOG_ERROR("on_call_requestfailure response code [%d][%s]",
                   e->m_pEvent->response->status_code,
                   e->m_pEvent->response->reason_phrase);
    }
    return 0;
}

int EventManage::on_call_globalfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_ack(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_cancelled(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_message_new(const SipEvent::Ptr &e)
{
    // 打印message
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

    MLOG_INFO("on_call_message_new response reqid:[%s]", reqid.c_str());

    if (MSG_IS_MESSAGE(e->m_pEvent->request))
    {
        m_stMsgDeal.HandleIncomingRequest(e);
    }
    else if (MSG_IS_BYE(e->m_pEvent->request))
    {
        /* NOTE 貌似底层已经响应200，上层可以无需再次响应 */
        // m_stCallDeal.SendResponse(e->m_pContext, e->m_pEvent->tid, SIP_OK);
    }
    return 0;
}

int EventManage::on_call_message_proceeding(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_message_answered(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_message_redirected(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_message_requestfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_message_serverfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_message_globalfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_call_closed(const SipEvent::Ptr &e)
{
    m_stCallDeal.HandleClose(e);
    return 0;
}

int EventManage::on_call_released(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_message_new(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_message_proceeding(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_message_answered(const SipEvent::Ptr &e)
{
    m_stMsgDeal.HandleResponseSuccess(e);
    return 0;
}

int EventManage::on_message_redirected(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_message_requestfailure(const SipEvent::Ptr &e)
{
    m_stMsgDeal.HandleResponseFailure(e);
    return 0;
}

int EventManage::on_message_serverfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_message_globalfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_noanswer(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_proceeding(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_answered(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_redirected(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_requestfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_serverfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_globalfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_subscription_notify(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_in_subscription_new(const SipEvent::Ptr &e)
{
    /* 派生类不重载操作,默认拒绝订阅 */
    // m_stSubscribeDeal.HandleResponse(e, SIP_FORBIDDEN);
    return 0;
}

int EventManage::on_notification_noanswer(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_notification_proceeding(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_notification_answered(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_notification_redirected(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_notification_requestfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_notification_serverfailure(const SipEvent::Ptr &e)
{
    return 0;
}

int EventManage::on_notification_globalfailure(const SipEvent::Ptr &e)
{
    return 0;
}
