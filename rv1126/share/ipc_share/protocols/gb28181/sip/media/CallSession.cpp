/*
 * @Author       : EasonLu
 * @Date         : 2025-02-27 15:39:56
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-27 16:24:02
 * @FilePath     : CallSession.cpp
 * @Description  : SIP会话类
 */
#include "CallSession.h"
using namespace SIP;

int CallSession::GetCallID()
{
    return _call_id;
}
void CallSession::SetCallID(int id)
{
    _call_id = id;
}
int CallSession::GetDialogID()
{
    return _dialog_id;
}
void CallSession::SetDialogID(int id)
{
    _dialog_id = id;
}
SSRCInfo::Ptr CallSession::GetSSRCInfo()
{
    return _ssrc;
}
void CallSession::SetSSRCInfo(SSRCInfo::Ptr ssrc)
{
    _ssrc = ssrc;
}
void CallSession::SetConnected(bool flag)
{
    _is_connected = flag;
}
bool CallSession::IsConnected()
{
    return _is_connected;
}

bool SIP::CallSession::SendBye()
{
    if (exosip_context != nullptr && _call_id > 0 && _dialog_id > 0)
    {
        if (OSIP_SUCCESS == eXosip_call_terminate(exosip_context, _call_id, _dialog_id))
        {
            return true;
        }
    }
    return false;
}
