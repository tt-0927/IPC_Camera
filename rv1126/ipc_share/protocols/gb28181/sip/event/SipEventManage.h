/*
 * @Author       : EasonLu
 * @Date         : 2025-02-13 14:58:09
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-23 17:10:11
 * @FilePath     : SipEventManage.h
 * @Description  : 事件的管理和调度
 */
#pragma once
#include "EventBusiness.h"
#include "QueryEvent.h"
#include "SipEvent.hpp"
#include "SipType.h"
#include "SubscribeEvent.h"
#include "CallEvent.h"
#include <map>

namespace SIP
{

    /* NOTE 服务器和客户端都继承这个事件回调管理
            统一的处理逻辑可以在此处实现，需要区分服务端和客户端的则需独自重载
     */
    class EventManage
    {
    public:
        typedef struct
        {
            const char *achName;          /* 事件名称 */
            SipEvent::fnEventDeal fnDeal; /* 事件处理函数 */
        } EventDeal;

        EventManage();

        EventDeal GetEventDeal(eXosip_event_type_t type);

    public:
        /* REGISTER related events */
        virtual int on_registration_success(const SipEvent::Ptr &e);
        virtual int on_registration_failure(const SipEvent::Ptr &e);

    public:
        /* INVITE related events within calls */
        virtual int on_call_invite(const SipEvent::Ptr &e);
        virtual int on_call_reinvite(const SipEvent::Ptr &e);
        virtual int on_call_noanswer(const SipEvent::Ptr &e);
        virtual int on_call_proceeding(const SipEvent::Ptr &e);
        virtual int on_call_ringing(const SipEvent::Ptr &e);
        virtual int on_call_answered(const SipEvent::Ptr &e);
        virtual int on_call_redirected(const SipEvent::Ptr &e);
        virtual int on_call_requestfailure(const SipEvent::Ptr &e);
        virtual int on_call_serverfailure(const SipEvent::Ptr &e);
        virtual int on_call_globalfailure(const SipEvent::Ptr &e);
        virtual int on_call_ack(const SipEvent::Ptr &e);
        virtual int on_call_cancelled(const SipEvent::Ptr &e);

    public:
        /* request related events within calls (except INVITE) */
        virtual int on_call_message_new(const SipEvent::Ptr &e);
        virtual int on_call_message_proceeding(const SipEvent::Ptr &e);
        virtual int on_call_message_answered(const SipEvent::Ptr &e);
        virtual int on_call_message_redirected(const SipEvent::Ptr &e);
        virtual int on_call_message_requestfailure(const SipEvent::Ptr &e);
        virtual int on_call_message_serverfailure(const SipEvent::Ptr &e);
        virtual int on_call_message_globalfailure(const SipEvent::Ptr &e);
        virtual int on_call_closed(const SipEvent::Ptr &e);

    public:
        /* for both UAS & UAC events */
        virtual int on_call_released(const SipEvent::Ptr &e);

    public:
        /* events received for request outside calls */
        virtual int on_message_new(const SipEvent::Ptr &e);
        virtual int on_message_proceeding(const SipEvent::Ptr &e);
        virtual int on_message_answered(const SipEvent::Ptr &e);
        virtual int on_message_redirected(const SipEvent::Ptr &e);
        virtual int on_message_requestfailure(const SipEvent::Ptr &e);
        virtual int on_message_serverfailure(const SipEvent::Ptr &e);
        virtual int on_message_globalfailure(const SipEvent::Ptr &e);

    public:
        /* Presence and Instant Messaging */
        virtual int on_subscription_noanswer(const SipEvent::Ptr &e);
        virtual int on_subscription_proceeding(const SipEvent::Ptr &e);
        virtual int on_subscription_answered(const SipEvent::Ptr &e);
        virtual int on_subscription_redirected(const SipEvent::Ptr &e);
        virtual int on_subscription_requestfailure(const SipEvent::Ptr &e);
        virtual int on_subscription_serverfailure(const SipEvent::Ptr &e);
        virtual int on_subscription_globalfailure(const SipEvent::Ptr &e);
        virtual int on_subscription_notify(const SipEvent::Ptr &e);

    public:
        virtual int on_in_subscription_new(const SipEvent::Ptr &e);

    public:
        virtual int on_notification_noanswer(const SipEvent::Ptr &e);
        virtual int on_notification_proceeding(const SipEvent::Ptr &e);
        virtual int on_notification_answered(const SipEvent::Ptr &e);
        virtual int on_notification_redirected(const SipEvent::Ptr &e);
        virtual int on_notification_requestfailure(const SipEvent::Ptr &e);
        virtual int on_notification_serverfailure(const SipEvent::Ptr &e);
        virtual int on_notification_globalfailure(const SipEvent::Ptr &e);

    protected:
        std::map<uint32_t, EventDeal> m_mapEventDeal;
        /* 各类事件业务处理 */
        MessageEvent m_stMsgDeal;         /* 基本Message事件 */
        RegisterEvent m_stRegisterDeal;   /* 注册事件 */
        CallEvent m_stCallDeal;           /* 通话事件 */
        SubscribeEvent m_stSubscribeDeal; /* 订阅事件 */
        QueryEvent m_stQueryDeal;         /* 查询事件 */
    };
}