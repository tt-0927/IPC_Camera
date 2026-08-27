/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 14:44:41
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-23 20:35:33
 * @FilePath     : SipEvent.hpp
 * @Description  : SIP事件定义
 */
#pragma once
#include "SipType.h"
// #include "gm.h"

namespace SIP
{
    /* 前向声明 */
    class SipServer;
    class SipClient;
    class SipNetBase;
    class SipEvent : public std::enable_shared_from_this<SipEvent>
    {
    public:
        typedef std::shared_ptr<SipEvent> Ptr;
        typedef std::function<int(const SipEvent::Ptr &)> fnEventDeal;

    public:
        int m_nValue;             /* 事件值 */
        const char *m_achName;    /* 事件名称 */
        fnEventDeal m_fnDeal;     /* 事件处理函数 */
        eXosip_t *m_pContext;     /* eXosip上下文 */
        eXosip_event_t *m_pEvent; /* eXosip事件 */
        uint64_t m_nID;           /* 事件id */
        /* 基类指针，存放对应的派生类实例 */
        SipNetBase *m_pNetBase = nullptr;
        void *m_pUser; /* 自定义用户数据 */
    };

    typedef struct SnapShotInfo
    {
        SipEvent::Ptr e;
        uint64_t nTime = 0;
        /* 重载赋值运算符 */
        SnapShotInfo &operator=(const SnapShotInfo &data)
        {
            if (this != &data)
            {
                e = data.e;
                nTime = data.nTime;
            }
            return *this;
        }
    }SnapShotInfo_S;   //add by longll 20250608
}