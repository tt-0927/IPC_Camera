/**
 * @FilePath     : algo_sdk_event_publisher_base.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:29:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:21:47
 * @Description  : AI 算法 SDK 事件推送公共基类实现
 */

#include "algo_sdk_event_publisher_base.hpp"

#include "control_manage.h"

namespace AiAppCommon
{
bool CAlgoSdkEventPublisherBase::hasClient() const
{
#ifndef ENABLE_TVSDK_SRC
    return false;
#else
    return ControlManage::instance()->tvsdk_get_client_count() > 0;
#endif
}

void CAlgoSdkEventPublisherBase::resetEvent()
{
    m_setActiveChannels.clear();
}

void CAlgoSdkEventPublisherBase::resetEvent(int nChnId)
{
    m_setActiveChannels.erase(normalizeChannel(nChnId));
}

int CAlgoSdkEventPublisherBase::normalizeChannel(int nChnId) const
{
    return nChnId < 0 ? 0 : nChnId;
}

bool CAlgoSdkEventPublisherBase::isEventActive(int nChnId) const
{
    return m_setActiveChannels.find(normalizeChannel(nChnId)) != m_setActiveChannels.end();
}

void CAlgoSdkEventPublisherBase::setEventActive(int nChnId, bool bActive)
{
    const int nNormalizedChnId = normalizeChannel(nChnId);
    if (bActive)
    {
        m_setActiveChannels.insert(nNormalizedChnId);
        return;
    }

    m_setActiveChannels.erase(nNormalizedChnId);
}
} // namespace AiAppCommon
