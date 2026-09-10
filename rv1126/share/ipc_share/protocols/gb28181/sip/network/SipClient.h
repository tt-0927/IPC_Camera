/*
 * @Author       : EasonLu
 * @Date         : 2025-02-24 09:23:32
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-12 14:48:24
 * @FilePath     : SipClient.h
 * @Description  : SIP客户端
 */
#pragma once
#include "BlockThread.hpp"
#include "EventBusiness.h"
#include "SipChannel.h"
#include "SipDevice.h"
#include "SipEventManage.h"
#include "SipNetBase.h"
#include "SipType.h"
#include "GbDefine.h"
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>

/* 注册总耗时统计开关：0关闭，1开启。 */
#define SIP_REGISTER_DURATION_ENABLED 1

namespace SIP
{
    class SipClient : public std::enable_shared_from_this<SipClient>,
                      public SipNetBase,
                      public EventManage
    {
    public:
        SipClient() {};
        ~SipClient() {};

        /* 声明友元 */
        friend class BlockThread;
        friend class CallEvent;
        friend class SubscribeEvent;
        friend class QueryEvent;

        /**
         * @brief  初始化SIP客户端
         * @param  [SipClientInfo_S] stInfo - SIP客户端信息
         * @return [*]
         * @author EasonLu
         * @note
         */
        bool Init(SipClientInfo_S stInfo);

        /**
         * @brief  开启SIP客户端
         * @return [*]
         * @author EasonLu
         * @note
         */
        bool Start() override;

        /**
         * @brief  关闭SIP客户端
         * @return [*]
         * @author EasonLu
         * @note
         */
        bool Stop() override;

        int SendMedia(SipDeviceInfo_S &device, char *pBuf, int nLen, bool bIsAudio);

        int SendMedia(const std::string &strCallID, char *pBuf, int nLen, bool bIsAudio);

        int SetCodec(SipDeviceInfo_S &device);

        int SendAlarmInfo(GB28181::AlarmInfo_S &stInfo);

        int SendUploadSnapShotFiniInfo(GB28181::UploadSnapShotFiniInfo_S &stInfo);

        inline void SetGuard(bool bGuard) { m_bGuard.store(bGuard); }
        inline bool GetGuard() { return m_bGuard.load(); }

        inline bool GetRegister() { return m_bRegister.load(); }

        Channel::Ptr GetChannelByID(std::string strID);
        std::string GetRemoteURI();

        /**
         * @brief  获取客户端信息
         * @return [const SipClientInfo_S&] 客户端信息
         * @author EasonLu
         * @note
         */
        const SipClientInfo_S &GetSipClietnInfo() const { return m_stClientInfo; }

        /**
         * @brief  更新通道信息
         * @param  [vector<SipChannelInfo_S>] &vecChnInfo
         * @return [int] - 0成功，-1失败
         * @author EasonLu
         * @note
         */
        int UpdateChnInfo(std::vector<SipChannelInfo_S> &vecChnInfo);

        int GetChnInfo(std::vector<Channel::Ptr> &vecChnInfo);

        int GetIndexByChnID(const std::string &strChnID);
        std::string GetChnIDByIndex(const int &nIndex);
        void setBroadcastTid(int nTid);

        /*** 
         * @description : 客户端上线状态
         * @author      : cyc
         * @return       ture-上线，false-未上线
         */        
        bool get_client_status();

        int on_message_new(const SipEvent::Ptr &e) override;
        int on_registration_success(const SipEvent::Ptr &e) override;
        int on_registration_failure(const SipEvent::Ptr &e) override;
        int on_message_requestfailure(const SipEvent::Ptr &e) override;
        int on_call_message_new(const SipEvent::Ptr &e) override;
        int on_call_invite(const SipEvent::Ptr &e) override;
        int on_in_subscription_new(const SipEvent::Ptr &e) override;
        int on_call_answered(const SipEvent::Ptr &e) override;

    protected:
        /**
         * @brief  是否允许eXosip自动处理协议事务
         * @param  [void]
         * @return [bool] false：客户端禁用eXosip自动注册认证
         * @note   GB35114注册401 challenge由on_registration_failure手动构建二次REGISTER。
         */
        bool ShouldRunAutomaticAction() const override { return false; }

        /**
         * @brief  注册操作
         * @param  [bool] bLogin - 是否注册(默认注册)
         * @return [*]
         * @author EasonLu
         * @note
         */
        void Register(bool bLogin = true);

        /**
         * @brief  注销操作
         * @return [*]
         * @author EasonLu
         * @note   需要异步执行，内部有关闭线程的操作
         */
        void LogoutAction();

        /**
         * @brief  新建事件
         * @param  [eXosip_t] *exosip_context - SIP上下文
         * @param  [eXosip_event_t] *exosip_event - 事件
         * @return [SipEvent::Ptr] - 事件指针
         * @author EasonLu
         * @note
         */
        SipEvent::Ptr new_event(eXosip_t *exosip_context, eXosip_event_t *exosip_event) override;

        void StartHeartbeatThr();

        void StopHeartbeatThr();

        void HeartbeatAction();

        void RegisterExpireAction();

        void StartRegisterThr();

        void StopRegisterThr();
        
        void RegisterRepeated();

        /**
         * @brief  调度注册失败后的退避重试
         * @param  [string] strReason - 触发重试的原因
         * @return [void]
         * @author zhouzr
         * @note   仅用于平台二次认证失败、网络超时等可恢复场景
         */
        void ScheduleRegisterRetry(const std::string &strReason);

        /**
         * @brief  取消待执行的注册退避任务
         * @param  [void]
         * @return [void]
         * @author zhouzr
         * @note   停止GB28181或注册成功时必须调用，避免关闭后继续发REGISTER
         */
        void CancelRegisterRetry();

        /**
         * @brief  注册退避任务入口
         * @param  [int] nDelaySec - 退避等待秒数
         * @param  [int] nRetryCount - 当前重试次数
         * @param  [unsigned long long] ullRetrySeq - 注册重试任务序号
         * @param  [string] strReason - 触发重试的原因
         * @return [void]
         * @author zhouzr
         * @note   任务执行前会再次检查客户端运行状态
         */
        void RegisterRetryAction(int nDelaySec, int nRetryCount, unsigned long long ullRetrySeq, std::string strReason);

        std::string EncodeChannelID(SipChannelInfo_S stChnInfo);

        int CloseCallByCallID(const std::string &strCallID);


        /**
         * @brief  更新客户端状态并触发回调
         * @param  [GB28181ClientStatus_E] enStatus - 新状态
         * @param  [string] strReason - 状态变化原因
         * @return [*]
         * @author cyc
         * @note
         */
         void UpdateClientStatus(GB28181::GB28181ClientStatus_E enStatus, const std::string& strReason = "");

    private:
#if SIP_REGISTER_DURATION_ENABLED
        /**
         * @brief  开始一次新的注册耗时统计
         * @param  [void]
         * @return [void]
         */
        void StartRegisterTiming();

        /**
         * @brief  输出本次注册成功耗时并结束统计
         * @param  [void]
         * @return [void]
         */
        void LogRegisterDuration();
#endif

        SipClientInfo_S m_stClientInfo;
        int m_nRegisterID = -1; /* 注册ID */
        /* 用于记录连接服务器的设备信息 */
        Device::Ptr m_pServerDev = nullptr;
        /* 记录本地设备相关信息 */
        Device::Ptr m_pLocalDev = nullptr;
        /* 心跳线程 */
        BlockThread *m_pThrHeartbeat = nullptr;
        /* 重复注册线程 */
        BlockThread *m_pThrRegister = nullptr;
        /* 退避注册线程 */
        std::thread m_thrRegisterRetry;
        /* 注册有效期线程 */
        BlockThread *m_pThrRegisterExpire = nullptr;
        /* 延时注销操作线程 */
        BlockThread *m_pThrLogout = nullptr;
        /* 注册状态 */
        std::atomic_bool m_bRegister = false;
        /* 停止状态 */
        std::atomic_bool m_bStopping = false;
        /* 注册重试次数 */
        std::atomic<int> m_nRegisterRetryCount{0};
        /* 注册重试线程锁 */
        std::mutex m_mtxRegisterRetry;
        /* 注册重试任务序号 */
        std::atomic<unsigned long long> m_ullRegisterRetrySeq{0};
#if SIP_REGISTER_DURATION_ENABLED
        /* lock: 保护注册计时起点与有效标记，避免注册事件和重试线程并发访问。 */
        std::mutex m_mtxRegisterTiming;
        /* 注册耗时统计起点，使用单调时钟避免系统校时影响。 */
        std::chrono::steady_clock::time_point m_stRegisterStartTime;
        /* 注册耗时统计是否有效。 */
        bool m_bRegisterTimingActive = false;
#endif
        /* 布防状态 */
        std::atomic_bool m_bGuard = false;
        /*会话ID/用来分辨不同的会话：广播*/
        int g_nBroadcast = 0; 

        /* 当前状态 */
        std::atomic<GB28181::GB28181ClientStatus_E> m_enCurrentStatus{GB28181::GB28181ClientStatus_E::OFFLINE};
        /* 心跳发送失败次数 */
        std::atomic<int> m_nHeartbeatFailureCount{0};
        /* 最后心跳成功时间 */
        std::atomic<std::chrono::system_clock::time_point> m_lastHeartbeatSuccessTime;
    };
}
