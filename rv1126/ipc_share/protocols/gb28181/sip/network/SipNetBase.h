/*
 * @Author       : EasonLu
 * @Date         : 2025-04-30 08:57:55
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-30 16:09:11
 * @FilePath     : SipNetBase.h
 * @Description  : SIP网络模块的基类
 */
#pragma once
#include "SipEvent.hpp"
#include "SipEventManage.h"
#include "SipType.h"
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>

/* SIP事件的超时时间（单位：毫秒） */
#ifndef SIP_EVENT_TIMEOUT
#define SIP_EVENT_TIMEOUT 20
#endif
namespace SIP
{
    class MediaSession;
    class SipNetBase
    {
    public:
        SipNetBase() = default;
        virtual ~SipNetBase() = default;

        virtual bool Start() = 0;
        virtual bool Stop() = 0;

        virtual void RecvEventThread()
        {
            pthread_setname_np(pthread_self(), "SIPRecvEvent");

            while (m_bThreadRun)
            {
                eXosip_event_t *pEvent = nullptr;
                pEvent = eXosip_event_wait(m_pSipContext, 0, SIP_EVENT_TIMEOUT);

                if (pEvent == nullptr)
                {
                    continue;
                }

                /* 未注册成功前，不执行eXosip_automatic_action */
                if(m_bIsRegisterSucceed && ShouldRunAutomaticAction())
                {
                    eXosip_lock(m_pSipContext);
                    eXosip_automatic_action(m_pSipContext);
                    eXosip_unlock(m_pSipContext);
                }
                /* 派生类自行继承创建的事件 */
                auto sip_event = new_event(m_pSipContext, pEvent);
                if (sip_event == nullptr)
                {
                    continue;
                }

                /* TODO 最好异步处理 */
                /* 指定对应类型事件的回调函数 */
                if (sip_event->m_fnDeal != nullptr)
                {
                    sip_event->m_fnDeal(sip_event);
                }
                eXosip_event_free(sip_event->m_pEvent);
            }
        }

        virtual SipEvent::Ptr new_event(eXosip_t *, eXosip_event_t *) { return nullptr; }
        bool IsRunning() const { return m_bThreadRun; }
        eXosip_t *GetSipContext() { return m_pSipContext; }
        const SipServerInfo_S &GetSipServerInfo() const { return m_stServerInfo; }
        const SipClientInfo_S &GetSipClientInfo() const { return m_stClientInfo; }      //add by longll 20250604
        virtual void SetSipClientInfo(const SipClientInfo_S &info) {m_stClientInfo = info;}  //add by longll 20250604
        virtual void UpdateCbInfo(const CbInfo_S &info) { m_stCbInfo = info; }
        inline CbInfo_S &GetCbInfo() { return m_stCbInfo; }

        virtual int AddSession(const std::string &strCallID, std::shared_ptr<MediaSession> pSession);
        virtual int DelSession(const std::string &strCallID);
        virtual std::shared_ptr<MediaSession> GetSession(const std::string &strCallID);

    protected:
        /**
         * @brief  是否允许eXosip自动处理协议事务
         * @param  [void]
         * @return [bool] true：允许，false：禁止
         * @note   GB35114客户端注册认证需要手动构造Authorization，不能由eXosip自动重发REGISTER。
         */
        virtual bool ShouldRunAutomaticAction() const { return true; }

        SipServerInfo_S m_stServerInfo;        /* SIP服务器信息 */
        SipClientInfo_S m_stClientInfo;        /* SIP本地客户端信息*/   //add by lonogll 20250604
        std::atomic_bool m_bThreadRun = false; /* 线程运行标记位 */
        eXosip_t *m_pSipContext = nullptr;     /* SIP上下文 */
        std::string m_strSipAgent = "IPC Camera"; /* SIP代理 */
        uint64_t m_nEventID = 0;               /* 事件id 自增 */
        std::atomic<bool> m_bIsRegisterSucceed = false; /* 是否注册成功 */
        /* 监听SIP事件线程 */
        std::shared_ptr<std::thread> m_thrListen = nullptr;
        /* 模块初始化的回调函数 */
        CbInfo_S m_stCbInfo;
        /* 会话ID——会话类 */
        std::unordered_map<std::string, std::shared_ptr<MediaSession>> m_mapSession;
    };
}
