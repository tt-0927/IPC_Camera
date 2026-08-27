/*
 * @Author       : EasonLu
 * @Date         : 2025-02-27 15:38:23
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-03-14 11:27:44
 * @FilePath     : CallSession.h
 * @Description  : SIP会话类
 */
#pragma once
#include "MediaStream.hpp"
#include "SSRC_Config.h"
#include <condition_variable>
#include <mutex>

namespace SIP
{

    class CallSession : public MediaStream,
                        public std::enable_shared_from_this<CallSession>
    {
    public:
        typedef std::shared_ptr<CallSession> Ptr;
        CallSession(
            eXosip_t *pCtx,
            const std::string &app,
            const std::string &stream_id,
            const SSRCInfo::Ptr ssrc)
            : MediaStream(app, stream_id, STREAM_TYPE::STREAM_TYPE_GB), _ssrc(ssrc) { exosip_context = pCtx; };

        virtual ~CallSession() {}

        int GetCallID();
        void SetCallID(int id);
        int GetDialogID();
        void SetDialogID(int id);
        SSRCInfo::Ptr GetSSRCInfo();
        void SetSSRCInfo(SSRCInfo::Ptr ssrc);
        void SetConnected(bool flag);
        bool IsConnected();

        bool SendBye();

    private:
        SSRCInfo::Ptr _ssrc;
        bool _is_connected = false;
        int _call_id = 0;
        int _dialog_id = 0;
    };

}
