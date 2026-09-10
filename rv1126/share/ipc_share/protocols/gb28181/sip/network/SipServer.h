/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 16:01:39
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-03-14 11:40:28
 * @FilePath     : SipServer.h
 * @Description  : SIP服务器
 */
#pragma once
#include "SipEvent.hpp"
#include "SipEventManage.h"
#include "SipNetBase.h"
#include "SipType.h"
#include <atomic>
#include <thread>

namespace SIP
{
    class SipServer : public std::enable_shared_from_this<SipServer>,
                      public SipNetBase,
                      public EventManage
    {
    public:
        typedef std::shared_ptr<SipServer> Ptr;

    public:
        /**
         * @brief  初始化服务器配置
         * @param  [SipServerInfo_S] stInfo - SIP服务器信息
         * @return [*]
         * @author EasonLu
         * @note
         */
        bool Init(SipServerInfo_S stInfo);

        /**
         * @brief  开启SIP服务器
         * @return [*]
         * @author EasonLu
         * @note
         */
        bool Start() override;

        /**
         * @brief  关闭SIP服务器
         * @return [*]
         * @author EasonLu
         * @note
         */
        bool Stop() override;

        int on_message_new(const SipEvent::Ptr &e) override;
        int on_call_message_answered(const SipEvent::Ptr &e) override;

        int on_call_invite(const SipEvent::Ptr &e) override;//add by longll

        SipServer() {};
        ~SipServer() {};

    private:
        /**
         * @brief  新建事件
         * @param  [eXosip_t] *exosip_context - SIP上下文
         * @param  [eXosip_event_t] *exosip_event - 事件
         * @return [SipEvent::Ptr] - 事件指针
         * @author EasonLu
         * @note
         */
        SipEvent::Ptr new_event(eXosip_t *exosip_context, eXosip_event_t *exosip_event) override;
    };
}