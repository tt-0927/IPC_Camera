/*
 * @Author       : EasonLu
 * @Date         : 2025-04-23 17:06:05
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-23 17:13:42
 * @FilePath     : CallEvent.h
 * @Description  : 会话事件
 */
#pragma once
#include "BaseEvent.h"
#include "SipType.h"
#include "MediaSdp.h"

namespace SIP
{
    class CallEvent : public BaseEvent
    {
    public:
        int HandleResponseSuccess(const SipEvent::Ptr e);
        int HandleSuperiorResponseSuccess(const SipEvent::Ptr e);

        int on_proceeding(const SipEvent::Ptr e);

        int HandleClose(const SipEvent::Ptr e);

        int HandleIncomingRequest(const SipEvent::Ptr &e);
        int HandleIncomingAudioRequest(const SipEvent::Ptr &e);

        int HandleCallAction(const SipEvent::Ptr &e);

    private:
        void SendInviteResponse(const SipEvent::Ptr &e, int status, const std::string &sdp = "");
    };
} // namespace SIP
