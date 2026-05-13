/*
 * @Author       : EasonLu
 * @Date         : 2025-04-30 16:07:11
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-08 09:33:11
 * @FilePath     : SipNetBase.cpp
 * @Description  : SIP网络模块的基类
 */
#include "SipNetBase.h"
#include "MediaSession.h"
#include "ModuleLog.h"
using namespace SIP;
int SIP::SipNetBase::AddSession(const std::string &strCallID, std::shared_ptr<MediaSession> pSession)
{
    m_mapSession[strCallID] = pSession;
    MLOG_INFO("添加Session: [%s]", strCallID.c_str());
    return 0;
}

int SIP::SipNetBase::DelSession(const std::string &strCallID)
{
    m_mapSession.erase(strCallID);
    MLOG_INFO("删除Session: [%s]", strCallID.c_str());
    return 0;
}

std::shared_ptr<MediaSession> SIP::SipNetBase::GetSession(const std::string &strCallID)
{
    if (m_mapSession.find(strCallID) != m_mapSession.end())
    {
        return m_mapSession[strCallID];
    }
    MLOG_WARN("获取Session失败: [%s]", strCallID.c_str());
    return nullptr;
}
