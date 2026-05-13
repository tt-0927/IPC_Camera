/*
 * @Author       : EasonLu
 * @Date         : 2025-04-21 09:11:43
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-21 09:12:43
 * @FilePath     : SubscribeEvent.h
 * @Description  : 订阅事件
 */
#pragma once
#include "BaseEvent.h"
#include "BlockQueue.hpp"
#include "BlockThread.hpp"
#include "GbDefine.h"
#include "SipType.h"
#include <atomic>
#include <map>
#include <mutex>
#include <thread>

namespace SIP
{
    class SipClient;
    class SubscribeEvent : public BaseEvent
    {
    public:
        SubscribeEvent();
        ~SubscribeEvent();

        typedef struct SubscribeData
        {
            /* 参考枚举值manscdp_cmdtype_e，已此值作为Key值 */
            int enCmdType = 0;
            int nExpires = 0;             /* 订阅时长（单位：秒数），0为关闭订阅 */
            int nLeftTime = 0;            /* 订阅剩余时长 */
            int nSubscribeID = 0;         /* 订阅ID中的会话ID-对应exosip_event中的did */
            uint64_t nStartTime = 0;      /* 开始订阅的时间 */
            std::string strSN;            /* 订阅请求中的SN码 */
            std::string strDevID;         /* 订阅请求中的设备ID */
            std::string strCmdType;       /* 订阅请求的枚举类型的字符串描述 */
            SipClient *pClient = nullptr; /* SIP客户端（一般只用客户端才接收订阅事件请求） */
            std::string strFromUri;
            std::string strToUri;
            /* 重载赋值运算符 */
            SubscribeData &operator=(const SubscribeData &data)
            {
                if (this != &data)
                {
                    enCmdType = data.enCmdType;
                    nExpires = data.nExpires;
                    nLeftTime = data.nLeftTime;
                    nSubscribeID = data.nSubscribeID;
                    nStartTime = data.nStartTime;
                    strSN = data.strSN;
                    strDevID = data.strDevID;
                    strCmdType = data.strCmdType;
                    pClient = data.pClient;
                    strFromUri = data.strFromUri;
                    strToUri = data.strToUri;
                }
                return *this;
            }
            /* 重载==运算符 */
            bool operator==(const SubscribeData &data) const
            {
                return enCmdType == data.enCmdType;
            }
            /* 重载!=运算符 */
            bool operator!=(const SubscribeData &data) const
            {
                return enCmdType != data.enCmdType;
            }
        } Data_S;

        virtual bool Handle(const SipEvent::Ptr &e) override;

        int NotifyCatalog();

        int NotifyAlarm(GB28181::AlarmInfo_S &stInfo);

        int Clear();

    protected:
        int HandleResponse(
            const SipEvent::Ptr &e,
            const manscdp_msgbody_header_t &header,
            bool bIsSupport);

        int SendNotify(const Data_S &stSubData, const std::string &strBody);

        void thrNotifyCatalog();

        void thrNotifyAlarm();

        bool GetSubData(int enCmdType, Data_S &stSubData);

        int SendAlarmMsg(GB28181::AlarmInfo_S &stInfo);

    private:
        std::mutex m_mutexMap; /* 订阅记录互斥锁 */
        std::mutex m_mutexCatalog; /* 目录线程互斥锁 */
        std::map<int, Data_S> m_mapSubscribe;
        BlockThread *m_pThrCatalog = nullptr;
        BlockQueue<GB28181::AlarmInfo_S> m_queAlarm;
        std::thread *m_pThrAlarm = nullptr;
        std::atomic_bool m_bThrRun = false;
    };

}