/**
 * @FilePath     : SipClient.cpp
 * @Author       : EasonLu
 * @Date         : 2025-02-24 09:23:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-26 17:51:15
 * @Description  : SIP客户端
 */

#include "SipClient.h"
#include "GbDefine.h"
#include "MediaSession.h"
#include "MethodRequest.h"
#include "dlog.h"
#include "SipUtils.h"
#include "gm.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include "SipModule.h"  //add by longll  20250604
#include "eXosip2/eXosip.h"
#include "RtpServer.h"

/* 注销等待时间（单位：毫秒） */
#ifndef SIP_CLIENT_LOGOUT_WAIT_TIME
#define SIP_CLIENT_LOGOUT_WAIT_TIME 500
#endif

/* 注册失败退避重试初始间隔（单位：秒） */
#ifndef SIP_CLIENT_REGISTER_RETRY_INITIAL_SEC
#define SIP_CLIENT_REGISTER_RETRY_INITIAL_SEC 10
#endif

/* 注册失败退避重试最大间隔（单位：秒） */
#ifndef SIP_CLIENT_REGISTER_RETRY_MAX_SEC
#define SIP_CLIENT_REGISTER_RETRY_MAX_SEC 60
#endif

#define SIP_CLIENT_DEBUG 1
using namespace SIP;

bool SipClient::Init(SipClientInfo_S stInfo)
{
    m_stClientInfo = stInfo;
    /* 必须更新一份本地监听服务器信息 */
    m_stServerInfo = m_stClientInfo.stLocal;
    /* 初始化本地的信息 */
    m_pLocalDev = std::make_shared<Device>(
        m_stClientInfo.stLocal.strID,
        m_stClientInfo.stLocal.strIP,
        std::to_string(m_stClientInfo.stLocal.nPort));
    SetSipClientInfo(m_stClientInfo);   //add by longll 20250604
    return true;
}

bool SipClient::Start()
{
    m_pSipContext = eXosip_malloc();
    if (OSIP_SUCCESS != eXosip_init(m_pSipContext))
    {
        dlog_error("eXosip_init failed");
        return false;
    }
    /* 设置代理名称 */
    eXosip_set_user_agent(m_pSipContext, m_strSipAgent.c_str());

    /* 根据初始化标记位来区分监听TCP/UDP协议 */
    int nRet = eXosip_listen_addr(
        m_pSipContext,
        m_stServerInfo.bTcp ? IPPROTO_TCP : IPPROTO_UDP,
        NULL, m_stServerInfo.nPort, AF_INET, 0);
    if (nRet != OSIP_SUCCESS)
    {
        dlog_error("eXosip_listen_addr failed");
        eXosip_quit(m_pSipContext);
        return false;
    }

    /* 检测是否配置了对外IP */
    if (m_stClientInfo.stLocal.strIP.empty())
    {
        /* 没有配置，则进行猜测 */
        char achIP[32] = {0};
        eXosip_guess_localip(m_pSipContext, AF_INET, achIP, (int)sizeof(achIP));
        dlog_info("SipClient没有设置对外IP，猜测IPv4为[%s]", achIP);
        /* 记录猜测的IP */
        m_stClientInfo.stLocal.strIP = achIP;
        if (m_pLocalDev)
        {
            m_pLocalDev->SetIP(achIP);
        }
    }
#if 0 /* Via字段调试 */
    struct eXosip_account_info stAccountInfo;
    memset(&stAccountInfo, 0, sizeof(stAccountInfo));
    snprintf(stAccountInfo.proxy, strlen(m_stClientInfo.stLocal.strIP.c_str()) + 1, "%s", stAccountInfo.proxy);
    snprintf(stAccountInfo.nat_ip, strlen(m_stClientInfo.stLocal.strIP.c_str()) + 1, "%s", stAccountInfo.nat_ip);
    stAccountInfo.nat_port = m_stServerInfo.nPort;
    dlog_debug("proxy[%s] nat_ip[%s] nat_port[%d]",
               stAccountInfo.proxy, stAccountInfo.nat_ip, stAccountInfo.nat_port);
    nRet = eXosip_set_option(m_pSipContext, EXOSIP_OPT_ADD_ACCOUNT_INFO, &stAccountInfo);
    dlog_info("eXosip_set_option EXOSIP_OPT_ADD_ACCOUNT_INFO [%d]", nRet);
    int nEnable = 1;
    nRet = eXosip_set_option(m_pSipContext, EXOSIP_OPT_ENABLE_REUSE_TCP_PORT, &nEnable);
    dlog_info("eXosip_set_option EXOSIP_OPT_ENABLE_REUSE_TCP_PORT [%d]", nRet);
    nEnable = 0;
    nRet = eXosip_set_option(m_pSipContext, EXOSIP_OPT_USE_RPORT, &(m_stServerInfo.nPort));
    dlog_info("eXosip_set_option EXOSIP_OPT_USE_RPORT [%d]", nRet);
#endif
    m_bThreadRun = true;
    m_bStopping = false;
    m_nRegisterRetryCount = 0;
    if (nullptr == m_thrListen)
    {
        m_thrListen = nullptr;
    }
    m_thrListen = std::make_shared<std::thread>(&SipClient::RecvEventThread, this);
    dlog_info("SipClient Start IP[%s] Port[%d]",
              m_stServerInfo.strIP.c_str(),
              m_stServerInfo.nPort);

    /* 设置初始状态为注册中 */
    UpdateClientStatus(GB28181::GB28181ClientStatus_E::REGISTERING, "开始注册");

    /* 开启注册请求 */
#if SIP_REGISTER_DURATION_ENABLED
    StartRegisterTiming();
#endif
    Register();
    //StartRegisterThr();

    return true;
}

bool SipClient::Stop()
{
    dlog_info("执行GB客户端停止操作");
    m_bStopping = true;
    CancelRegisterRetry();
    /* 置标志位，避免eXosip_automatic_action影响注销 */
    m_bIsRegisterSucceed = false;


    /* 更新状态为离线 */
    UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, "客户端停止");

    /* 先清空所有订阅，包括线程 */
    m_stSubscribeDeal.Clear();

    /* 先注销 */
    Register(false);
    if (m_pThrLogout != nullptr)
    {
        m_pThrLogout->stop();
        delete m_pThrLogout;
        m_pThrLogout = nullptr;
    }
    /* 注销先更新标记位，防止异步发送的报警信息引起eXosip_t引用错误 */
    m_bRegister = false;
    { /* 延时执行注销操作 */
        dlog_info("创建注销操作线程")
        int nTimeout = SIP_CLIENT_LOGOUT_WAIT_TIME;
        /* 延时指定时间，必定执行注销操作，防止服务器无响应没有执行 */
        m_pThrLogout = new BlockThread(
            std::bind(&SipClient::LogoutAction, this),
            std::chrono::milliseconds(nTimeout),
            true);
        m_pThrLogout->start();
    }
    /* NOTE 只能等待注销操作结束才能返回，存在同步重新注册的操作 */
    /* 等待时长必须要远大于(阻塞超时+执行时间)再返回 */
    std::this_thread::sleep_for(std::chrono::milliseconds(SIP_CLIENT_LOGOUT_WAIT_TIME * 4));

    /* 清空所有订阅 */
    m_stSubscribeDeal.Clear();
    CRtpServer::instance()->set_rtpStatus(RTP_IDLE,"",nullptr);
    /* 关闭本地所有的GB发流 */
    if (m_pLocalDev)
    {
        auto vecChn = m_pLocalDev->GetAllChannels();
        for (auto &pChn : vecChn)
        {
            pChn->ClosePlayAction();
            pChn->DestroyPlayNet();
            pChn->UpdateMediaStatus(false);
        }
    }
    dlog_info("执行GB客户端停止操作完毕");
    return true;
}

int SIP::SipClient::SendMedia(
    SipDeviceInfo_S &device,
    char *pBuf,
    int nLen,
    bool bIsAudio)
{
    if (nullptr == m_pLocalDev)
    {
        return -1;
    }

    auto pChn = m_pLocalDev->GetChannel(device.strChnID);
    if (nullptr == pChn)
    {
        return -2;
    }
#if SIP_CLIENT_DEBUG
    dlog_debug("发送通道ID[%s]媒体数据[%d]", device.strChnID.c_str(), nLen);
#endif
    return pChn->SendMedia(pBuf, nLen, bIsAudio);
}

int SIP::SipClient::SendMedia(
    const std::string &strCallID,
    char *pBuf,
    int nLen,
    bool bIsAudio)
{
    auto pSession = GetSession(strCallID);
    if (pSession)
    {
        return pSession->SendMedia(pBuf, nLen, bIsAudio);
    }
    return -1;
}

int SIP::SipClient::SetCodec(SipDeviceInfo_S &device)
{
    if (nullptr == m_pLocalDev)
    {
        return -1;
    }
    /* 更新点播的编码信息 */
    auto pChn = m_pLocalDev->GetChannel(device.strChnID);
    if (pChn)
    {
        pChn->SetSipDeviceInfo(device);
    }

    auto pSession = GetSession(device.strCallID);
    if (pSession)
    {
        return pSession->SetCodecInfo(device);
    }

    return 0;
}

int SIP::SipClient::SendAlarmInfo(GB28181::AlarmInfo_S &stInfo)
{
    /* 判断是否布防了 */
    if (!m_bGuard.load())
    {
        /* NOTE 设备没有布防，不转发报警信息给上级 */
        return -1;
    }
    /* 送到订阅类进行处理 */
    m_stSubscribeDeal.NotifyAlarm(stInfo);
    return 0;
}

int SIP::SipClient::SendUploadSnapShotFiniInfo(GB28181::UploadSnapShotFiniInfo_S &stInfo)
{
    /*  送到基本Message事件*/
    m_stMsgDeal.NotifySnapShotFinish(stInfo);

    return 0;
}

std::string SIP::SipClient::GetRemoteURI()
{
    if (m_pServerDev)
    {
        return m_pServerDev->GetUri();
    }
    return std::string();
}

void SIP::SipClient::setBroadcastTid(int nTid)
{
    g_nBroadcast = nTid;
}

Channel::Ptr SIP::SipClient::GetChannelByID(std::string strID)
{
    if(m_pLocalDev == nullptr)
    {
        return nullptr;
    }

    return m_pLocalDev->GetChannel(strID);
}

int SIP::SipClient::on_message_new(const SipEvent::Ptr &e)
{
    eXosip_event_t *exosip_event = e->m_pEvent;
    dlog_info("SipClient Message New Method [%s], status_code [%d]",
              exosip_event->request->sip_method,
              exosip_event->request->status_code);

    /*gb35114控制信令响应分析,分析GB35114 Date、Note字段*/
    if (OK != CGm::instance()->gm_analyzing_control_signaling(exosip_event))
    {
        dlog_error("gb35114控制信令响应分析,分析GB35114 Date、Note字段失败");
    }

    if (MSG_IS_REGISTER(exosip_event->request))
    {
        m_stRegisterDeal.HandleIncomingRequest(e);
    }
    else if (MSG_IS_MESSAGE(exosip_event->request))
    {
        m_stMsgDeal.HandleIncomingRequest(e);
    }
    else if (MSG_IS_BYE(exosip_event->request))
    {
    }
    else if (MSG_IS_INFO(exosip_event->request))
    {
        /* 输出报文内容 */
        osip_body_t *body = nullptr;
        osip_message_get_body(exosip_event->request, 0, &body);
        if (body)
        {
            dlog_info("SipClient Message Info Body\n%s", body->body);
        }
    }
    else
    {
    }

    return 0;
}

int SIP::SipClient::on_registration_success(const SipEvent::Ptr &e)
{
    auto pResponse = e->m_pEvent->response;
    if (pResponse != nullptr && pResponse->status_code == SIP_OK)
    {
        int nExpires = m_stRegisterDeal.GetExpires(e);
        if (nExpires > 0)
        {
            /*gb35114注册请求的服务端二次响应分析*/
            if (OK != CGm::instance()->gm_analyzing_second_response(e->m_pEvent))
            {
                dlog_error("gb35114注册请求的服务端二次响应分析,分析GB35114 Securityinfo字段失败");
            }

            /* NOTE 可校验注册的服务器和返回消息的服务器一致 */
            StartHeartbeatThr();
            CancelRegisterRetry();
            m_nRegisterRetryCount = 0;
            /* 注册成功，注册成功标志位置 true */
            m_bIsRegisterSucceed = true;
            dlog_info("注册成功 [%s]",
                      m_pServerDev == nullptr ? "" : m_pServerDev->GetUri().c_str());
            m_bRegister = true;

            /* 更新状态为在线 */
            UpdateClientStatus(GB28181::GB28181ClientStatus_E::ONLINE, "注册成功");
#if SIP_REGISTER_DURATION_ENABLED
            LogRegisterDuration();
#endif

            /* 校时 */
            osip_header_t *dest = NULL;
            osip_message_get_date(e->m_pEvent->response, 0, &dest);
            if (nullptr != dest)
            {
                dlog_info("time:[%s]", dest->hvalue);
                /*TODO设备校时上抛*/
                if (std::strlen(dest->hvalue) > 0)
                {
                     //SipSetSystemTime(dest->hvalue);
                    dlog_info("time:[%s]", dest->hvalue);
                    std::string strTime = dest->hvalue;
                     /*TODO 回调上抛*/
                    SipCbResult_S stResult;
                    { /* 配合上层实现业务功能 */
                        auto fnCb = ::SipModule::instance()->GetNVRSetTimeCb();
                        if (fnCb)
                        {
                            fnCb(strTime, stResult);
                        }
                    }
                    /* note 默认成功 */
                    stResult.nResult = 0;
                    if(stResult.nResult  == 0)
                    {
                        dlog_info("校时回调上抛设置成功 time:[%s]", dest->hvalue);
                    }
                    else
                    {
                        dlog_info("校时回调上抛设置失败 time:[%s]", dest->hvalue);
                    }
                }
            }
        }
        else if (0 == nExpires)
        {
            m_bRegister = false;
            /* 更新状态为离线 */
            UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, "注销成功");
            if (m_pThrLogout != nullptr)
            {
                dlog_info("唤醒注销线程");
                /* 唤醒注销线程 */
                m_pThrLogout->notify();
            }
        }
    }
    return 0;
}

int SIP::SipClient::on_registration_failure(const SipEvent::Ptr &e)
{
    if (m_bStopping.load())
    {
        dlog_warn("客户端正在停止，忽略注册失败事件");
        return -1;
    }

    /*
     * warn: GB35114 首次注册和续期注册都会先收到 401 challenge，不能在进入函数时
     *       直接清在线状态；只有最终认证失败或非挑战类失败才允许置离线。
     */
    const bool bWasRegistered = m_bRegister.load();
    auto fnMarkRegisterFailed = [this](const std::string &strReason) {
        m_bRegister.store(false);
        m_bIsRegisterSucceed.store(false);
        UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, strReason);
    };

    if (e->m_pEvent->response == nullptr)
    {
        /* TODO 超时没响应，需单开线程一直注册，并上抛服务器不在线状态 */
        fnMarkRegisterFailed("注册超时无响应");
        ScheduleRegisterRetry("注册超时无响应");
        return -1;
    }
    int nStatus = e->m_pEvent->response->status_code;
    dlog_warn("注册失败，状态码[%d]", nStatus);

    std::string strReason = "注册失败，状态码: " + std::to_string(nStatus);

    /* 403 407 */
    if (SIP_FORBIDDEN == nStatus || SIP_PROXY_AUTHENTICATION_REQUIRED == nStatus)
    {
        fnMarkRegisterFailed(strReason);
        osip_www_authenticate_t *www_authenticate_header = nullptr;
        osip_message_get_www_authenticate(e->m_pEvent->response, 0, &www_authenticate_header);
        if (www_authenticate_header)
        {
            /* 获取服务器域 */
            // _sip_server_domain = osip_strdup_without_quote(www_authenticate_header->realm);
            /* 使用md5摘要生成注册内容 */
            eXosip_add_authentication_info(
                e->m_pContext,
                m_stServerInfo.strID.c_str(),
                m_stServerInfo.strID.c_str(),
                m_stServerInfo.strPassword.c_str(),
                "md5",
                www_authenticate_header->realm);
            /* 配置完鉴权头后会自动重发 */
        }
    }
    else if(302 == nStatus)
    {
        fnMarkRegisterFailed(strReason);
        /* 清理注册信息 */
        eXosip_clear_authentication_info(m_pSipContext);
        /* 注册有效期 */
        int nExpires = m_stClientInfo.stRemote.nExpires;
        /* 构建注册信息 */
        osip_message_t *pRegisterMsg = nullptr;
        osip_contact_t* contact;
        osip_message_get_contact(e->m_pEvent->response, 0, &contact);

        if (contact) {
            char* new_uri = nullptr;
            int ret = osip_uri_to_str(contact->url, &new_uri);
            if (ret != OSIP_SUCCESS || new_uri == nullptr)
            {
                dlog_error("302重定向Contact URI解析失败: %d", ret);
                ScheduleRegisterRetry("302重定向Contact URI解析失败");
                return -1;
            }
            dlog_info("Redirected to:[%s]", new_uri);

            // 更新注册信息
            eXosip_register_remove(m_pSipContext, m_nRegisterID);
            m_nRegisterID = eXosip_register_build_initial_register(
                m_pSipContext,  m_stClientInfo.stLocal.GetUri().c_str(),
                new_uri, nullptr, nExpires, &pRegisterMsg);
            if(new_uri != nullptr)
            {
                osip_free(new_uri);
            }
        }

        if (pRegisterMsg == nullptr)
        {
            dlog_error("初始化注册信息失败");
            ScheduleRegisterRetry("重定向后初始化注册请求失败");
            return -1;
        }

        /*构建gb35114第一次注册authorization授权字段*/
        eXosip_lock(m_pSipContext);
        const int nGmRet = CGm::instance()->gm_build_first_register(pRegisterMsg);
        if (OK != nGmRet)
        {
            osip_message_free(pRegisterMsg);
            eXosip_unlock(m_pSipContext);
            dlog_error("构建gb35114第一次注册authorization授权字段失败: %d", nGmRet);
            ScheduleRegisterRetry("构建GB35114首次注册认证字段失败");
            return -1;
        }

        /* 发送注册信息 */
        auto ret = eXosip_register_send_register(m_pSipContext, m_nRegisterID, pRegisterMsg);
        eXosip_unlock(m_pSipContext);
        if (OSIP_SUCCESS == ret)
        {
            dlog_info("发送注册信息成功 Uri[%s]", m_pServerDev->GetUri().c_str());
        }
    }
    else if(SIP_UNAUTHORIZED == nStatus)    // 401
    {
        osip_authorization_t *request_authorization = nullptr;
        osip_message_get_authorization(e->m_pEvent->request, 0, &request_authorization);
        if (request_authorization != nullptr && request_authorization->auth_type != nullptr &&
            std::string(request_authorization->auth_type) == "Bidirection")
        {
            /* warn: 带 GB35114 授权后仍被 401 拒绝，必须退避重试，避免形成注册风暴。 */
            fnMarkRegisterFailed("GB35114授权注册仍被平台401拒绝");
            dlog_error("GB35114授权注册仍被平台401拒绝，进入退避重试");
            ScheduleRegisterRetry("GB35114授权注册仍被平台401拒绝");
            return -1;
        }

        int nRet = 0;
        eXosip_lock(m_pSipContext);
        /* 清理注册信息 */
        nRet = eXosip_clear_authentication_info(m_pSipContext);
        if (nRet < 0)
        {
            eXosip_unlock(m_pSipContext);
            dlog_error("清除eXosip中存储的所有认证凭据失败: %d", nRet);
            if (!bWasRegistered)
            {
                fnMarkRegisterFailed("清除SIP认证信息失败");
            }
            else
            {
                dlog_warn("已在线状态处理401 challenge时清除认证缓存失败，保持当前在线状态");
            }
            ScheduleRegisterRetry("清除SIP认证信息失败");
            return -1;
        }
        /*添加主叫用户的认证信息*/
        osip_www_authenticate_t *www_authenticate_header = nullptr;
        nRet = osip_message_get_www_authenticate(e->m_pEvent->response, 0, &www_authenticate_header);
        if (nRet < 0 || www_authenticate_header == nullptr || www_authenticate_header->auth_type == nullptr)
        {
            eXosip_unlock(m_pSipContext);
            dlog_error("401响应缺少有效的WWW-Authenticate挑战字段");
            if (!bWasRegistered)
            {
                fnMarkRegisterFailed("401响应认证挑战字段无效");
            }
            else
            {
                dlog_warn("已在线状态收到无效401 challenge，保持当前在线状态");
            }
            ScheduleRegisterRetry("401响应认证挑战字段无效");
            return -1;
        }

        const std::string strAuthType = www_authenticate_header->auth_type;
        const bool bIsGb35114Challenge = (strAuthType == "Unidirection" || strAuthType == "Bidirection");
        if (bIsGb35114Challenge && !CGm::instance()->isGmEnable())
        {
            eXosip_unlock(m_pSipContext);
            /*
             * warn: 平台返回 GB35114 认证挑战时，普通 Digest 重发不会携带有效授权字段。
             *       未启用 GB35114 时必须进入退避，避免同一秒内持续发送 REGISTER。
             */
            dlog_error("平台要求GB35114认证[%s]，但本地未启用GB35114，进入退避重试", strAuthType.c_str());
            if (!bWasRegistered)
            {
                fnMarkRegisterFailed("平台要求GB35114认证但本地未启用");
            }
            else
            {
                dlog_warn("已在线状态收到GB35114认证挑战但本地未启用，保持当前在线状态");
            }
            ScheduleRegisterRetry("平台要求GB35114认证但本地未启用");
            return -1;
        }
        else
        {
            /* 使用md5摘要生成注册内容 */
            nRet = eXosip_add_authentication_info(m_pSipContext,
                                                  m_stClientInfo.stLocal.strID.c_str(),
                                                  m_stClientInfo.stLocal.strID.c_str(),
                                                  m_stClientInfo.stLocal.strPassword.c_str(),
                                                  "MD5",
                                                  www_authenticate_header->realm);
            if (nRet < 0)
            {
                eXosip_unlock(m_pSipContext);
                dlog_error("eXosip_add_authentication_info failed: %d", nRet);
                if (!bWasRegistered)
                {
                    fnMarkRegisterFailed("添加SIP认证信息失败");
                }
                else
                {
                    dlog_warn("已在线状态处理401 challenge时添加认证信息失败，保持当前在线状态");
                }
                ScheduleRegisterRetry("添加SIP认证信息失败");
                return -1;
            }
        }
        /* 注册有效期 */
        int nExpires = m_stClientInfo.stRemote.nExpires;
        /* 构建第二次注册信息 */
        osip_message_t *pRegisterMsg = nullptr;
        nRet = eXosip_register_build_register(m_pSipContext, m_nRegisterID, nExpires, &pRegisterMsg);
        if (nRet < 0 || pRegisterMsg == nullptr)
        {
            eXosip_unlock(m_pSipContext);
            dlog_error("eXosip_register_build_register failed: %d", nRet);
            if (bWasRegistered)
            {
                /*
                 * warn: 已在线时 eXosip 可能连续上抛同一注册会话的 401 事件，部分事件无法再次
                 *       build_register。此时保持当前注册状态，等待后续 challenge 事件或注册有效期线程处理。
                 */
                dlog_warn("已在线状态处理401 challenge时构建二次注册失败，保持当前在线状态");
                return -1;
            }
            fnMarkRegisterFailed("构建二次注册请求失败");
            ScheduleRegisterRetry("构建二次注册请求失败");
            return -1;
        }

        char *pDest = nullptr;
        size_t nLength = 0;
        nRet = osip_message_to_str(e->m_pEvent->response, &pDest, &nLength);
        if (nRet == OSIP_SUCCESS && pDest != nullptr)
        {
            dlog_info("服务器返回的401注册消息:\n%s", pDest);
            osip_free(pDest);
        }
        else
        {
            dlog_warn("401注册响应序列化失败: %d", nRet);
        }

        /* 构建gb35114第二次注册authorization授权字段 */
        if (OK != CGm::instance()->gm_build_second_register(e->m_pEvent->response, pRegisterMsg, m_stClientInfo))
        {
            osip_message_free(pRegisterMsg);
            eXosip_unlock(m_pSipContext);
            dlog_error("构建gb35114第二次注册authorization授权字段失败");
            if (!bWasRegistered)
            {
                fnMarkRegisterFailed("GB35114认证挑战字段无效");
            }
            else
            {
                dlog_warn("已在线状态收到异常401 challenge，保持当前在线状态并等待后续注册刷新");
            }
            ScheduleRegisterRetry("GB35114认证挑战字段无效");
            return -1;
        }

        /* 发送注册信息 */
        nRet = eXosip_register_send_register(m_pSipContext, m_nRegisterID, pRegisterMsg);
        eXosip_unlock(m_pSipContext);
        if (OSIP_SUCCESS == nRet)
        {
            dlog_info("发送注册信息成功 Uri[%s]", m_pServerDev->GetUri().c_str());
        }
        else
        {
            dlog_error("eXosip_register_send_register authorization error!  nRet=%d", nRet);
            if (!bWasRegistered)
            {
                fnMarkRegisterFailed("发送二次注册请求失败");
            }
            else
            {
                dlog_warn("已在线状态发送二次注册失败，保持当前在线状态并进入退避刷新");
            }
            ScheduleRegisterRetry("发送二次注册请求失败");
            return -1;
        }
    }
    else
    {
        fnMarkRegisterFailed(strReason);
        ScheduleRegisterRetry(strReason);
        return -1;
    }
    return 0;
}

int SIP::SipClient::on_message_requestfailure(const SipEvent::Ptr &e)
{
    /* 发送请求失败 */
    /* TODO 查看请求记录池是什么请求，根据进行不同的处理 */
    return 0;
}

int SIP::SipClient::on_call_message_new(const SipEvent::Ptr &e)
{
    // 打印message
    std::string reqid;
    osip_generic_param_t *tag = nullptr;
    osip_uri_param_get_byname(&e->m_pEvent->request->from->gen_params, (char *)"tag", &tag);
    if (nullptr == tag || nullptr == tag->gvalue)
    {
        reqid = "";
    }
    else
    {
        reqid = (const char *)tag->gvalue;
    }

    std::string strCallID;
    if (e->m_pEvent->request &&
        e->m_pEvent->request->call_id &&
        e->m_pEvent->request->call_id->number)
    {
        strCallID = e->m_pEvent->request->call_id->number;
    }

    dlog_info("on_call_message_new response reqid:[%s] callid:[%s]",
              reqid.c_str(), strCallID.c_str());

    if (MSG_IS_MESSAGE(e->m_pEvent->request))
    {
        m_stMsgDeal.HandleIncomingRequest(e);
    }
    else if (MSG_IS_BYE(e->m_pEvent->request))
    {
        CloseCallByCallID(strCallID);
    }
    else if (MSG_IS_INFO(e->m_pEvent->request))
    {
        m_stCallDeal.HandleCallAction(e);
    }
    else
    {
        dlog_warn("on_call_message_new 未处理的消息类型 [%s]",
                  e->m_pEvent->request->sip_method);
    }
    return 0;
}

int SIP::SipClient::on_call_invite(const SipEvent::Ptr &e)
{
    m_stCallDeal.SetDeivce(m_pLocalDev);
    return m_stCallDeal.HandleIncomingRequest(e);
}

int SIP::SipClient::on_call_answered(const SipEvent::Ptr &e)
{
    m_stCallDeal.SetDeivce(m_pLocalDev);
    g_nBroadcast = e->m_pEvent->tid + 1;
    return m_stCallDeal.HandleSuperiorResponseSuccess(e);
}

int SIP::SipClient::on_in_subscription_new(const SipEvent::Ptr &e)
{
    m_stSubscribeDeal.Handle(e);
    /* TODO 记录订阅的信息 */
    return 0;
}

void SipClient::Register(bool bLogin)
{
    { /* 设定连接服务器的信息 */
        if (bLogin)
        {
            if (m_pServerDev != nullptr)
            {
                m_pServerDev = nullptr;
            }

            m_pServerDev = std::make_shared<Device>(
                m_stClientInfo.stRemote.strID,
                m_stClientInfo.stRemote.strIP,
                std::to_string(m_stClientInfo.stRemote.nPort));
            dlog_info("注册服务器URI[%s]",
                      m_pServerDev->GetUri().c_str());
        }
        else
        {
            if (m_pServerDev != nullptr)
            {
                dlog_info("注销服务器URI[%s]",
                          m_pServerDev->GetUri().c_str());
            }
        }
    }
    eXosip_lock(m_pSipContext);
    /* 清理注册信息 */
    int nRet = eXosip_clear_authentication_info(m_pSipContext);
    if(nRet != 0)
    {
        dlog_error("清理注册信息失败:%d", nRet);
    }
    /* 注册有效期 */
    int nExpires = bLogin ? m_stClientInfo.stRemote.nExpires : 0;
    /* 构建注册信息 */
    osip_message_t *pRegisterMsg = nullptr;
    m_nRegisterID = eXosip_register_build_initial_register(m_pSipContext,
                                                           m_stClientInfo.stLocal.GetUri().c_str(),
                                                           m_pServerDev->GetUri().c_str(),
                                                           nullptr,
                                                           nExpires,
                                                           &pRegisterMsg);
    if (pRegisterMsg == nullptr)
    {
        dlog_error("初始化注册信息失败");
        eXosip_unlock(m_pSipContext);
        if (bLogin)
        {
            ScheduleRegisterRetry("初始化注册请求失败");
        }
        return;
    }

    /*构建gb35114第一次注册authorization授权字段*/
    const int nGmRet = CGm::instance()->gm_build_first_register(pRegisterMsg);
    if (OK != nGmRet)
    {
        osip_message_free(pRegisterMsg);
        eXosip_unlock(m_pSipContext);
        dlog_error("构建gb35114第一次注册authorization授权字段失败: %d", nGmRet);
        if (bLogin)
        {
            ScheduleRegisterRetry("构建GB35114首次注册认证字段失败");
        }
        return;
    }

    /* 发送注册信息 */
    auto ret = eXosip_register_send_register(m_pSipContext, m_nRegisterID, pRegisterMsg);
    eXosip_unlock(m_pSipContext);
    if (OSIP_SUCCESS == ret)
    {
        dlog_info("发送注册信息成功 Uri[%s]", m_pServerDev->GetUri().c_str());
    }
}

void SIP::SipClient::LogoutAction()
{
    dlog_info("注销操作开始");
    /* 更新状态为离线 */
    UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, "执行注销操作");
    /* 等待一段时间后再停止线程，防止在下面回收的线程中执行此操作 */
    std::this_thread::sleep_for(std::chrono::milliseconds(SIP_CLIENT_LOGOUT_WAIT_TIME));
    /* 服务器实例必须在业务中最后进行释放，存在线程安全问题 */
    m_bThreadRun = false;

    if (m_thrListen && m_thrListen->joinable())
    {
        m_thrListen->join();
        m_thrListen = nullptr;
    }

    /* ! 注销时必须同时回收心跳和注册有效期线程，避免旧线程访问已释放的SIP上下文。 */
    StopHeartbeatThr();

    if (m_pThrRegister != nullptr)
    {
        m_pThrRegister->stop();
        delete m_pThrRegister;
        m_pThrRegister = nullptr;
    }

    CancelRegisterRetry();

    m_bRegister = false;
    dlog_info("SipClient 注销成功");
    eXosip_quit(m_pSipContext);
    m_pSipContext = nullptr;
}

SipEvent::Ptr SIP::SipClient::new_event(eXosip_t *exosip_context, eXosip_event_t *exosip_event)
{
    if (exosip_event == nullptr)
    {
        return nullptr;
    }
    if (exosip_event->type < EXOSIP_REGISTRATION_SUCCESS ||
        exosip_event->type > EXOSIP_NOTIFICATION_GLOBALFAILURE)
    {
        return nullptr;
    }

    /* 匹配对应事件的处理函数 */
    SipEvent::Ptr pEvent = std::make_shared<SipEvent>();
    auto stEventDeal = GetEventDeal(exosip_event->type);
    if (stEventDeal.achName == nullptr || stEventDeal.fnDeal == nullptr)
    {
        return nullptr;
    }
    char *pDest = nullptr;
    size_t nLength = 0;
    osip_message_to_str(exosip_event->request, &pDest, &nLength);
    dlog_info("[SipClient]处理事件:[%d][%s][%s]\n%s",
              exosip_event->type, exosip_event->textinfo,
              stEventDeal.achName, pDest);
    osip_free(pDest);
    pEvent->m_nValue = exosip_event->type;
    pEvent->m_achName = stEventDeal.achName;
    pEvent->m_fnDeal = stEventDeal.fnDeal;
    pEvent->m_pContext = exosip_context;
    pEvent->m_pEvent = exosip_event;
    pEvent->m_nID = m_nEventID++;
    pEvent->m_pNetBase = this;
    pEvent->m_pUser = nullptr;
    return pEvent;
}

void SIP::SipClient::StartHeartbeatThr()
{
    if (!m_bThreadRun)
    {
        return;
    }
    StopHeartbeatThr();
    { /* 发送心跳线程 */
        int nTimeout = m_stClientInfo.stRemote.nHeartbeatInterval * 1000;

        m_pThrHeartbeat = new BlockThread(
            std::bind(&SipClient::HeartbeatAction, this),
            std::chrono::milliseconds(nTimeout));
        m_pThrHeartbeat->start();
        dlog_info("心跳线程已启动，发送周期[%d]秒", m_stClientInfo.stRemote.nHeartbeatInterval);
    }
    { /* 注册有效期线程 */
        /* 在有限期的提前5秒重新进行注册 */
        int nTimeout = (m_stClientInfo.stRemote.nExpires - 5) * 1000;
        m_pThrRegisterExpire = new BlockThread(
            std::bind(&SipClient::RegisterExpireAction, this),
            std::chrono::milliseconds(nTimeout));
        m_pThrRegisterExpire->start();
    }
}

void SIP::SipClient::StartRegisterThr()
{
    Register();
    if (!m_bThreadRun)
    {
        return;
    }
    StopRegisterThr();
    { /* 开启注册请求线程 */
        int nTimeout = 10*1000; /*10s发一次*/

        m_pThrRegister = new BlockThread(
            std::bind(&SipClient::RegisterRepeated, this),
            std::chrono::milliseconds(nTimeout));
        m_pThrRegister->start();
    }
}

void SIP::SipClient::StopHeartbeatThr()
{
    if (m_pThrHeartbeat != nullptr)
    {
        m_pThrHeartbeat->stop();
        delete m_pThrHeartbeat;
        m_pThrHeartbeat = nullptr;
    }

    if (m_pThrRegisterExpire != nullptr)
    {
        m_pThrRegisterExpire->stop();
        delete m_pThrRegisterExpire;
        m_pThrRegisterExpire = nullptr;
    }
}

void SIP::SipClient::StopRegisterThr()
{
    if (m_pThrRegister != nullptr)
    {
        m_pThrRegister->stop();
        delete m_pThrRegister;
        m_pThrRegister = nullptr;
    }
}

void SIP::SipClient::HeartbeatAction()
{
    if (!m_pServerDev)
    {
        dlog_error("服务器设备信息为空，无法发送心跳");
        return;
    }
    dlog_info("SipClient 发送心跳 [%s]", m_pServerDev->GetUri().c_str());
    auto request = std::make_shared<HeartbeatRequest>(m_pSipContext,
                                                      m_pServerDev,
                                                      m_stClientInfo.stLocal.strID,
                                                      m_pLocalDev->GetUri());
    int nRet = request->SendMessage(false);
    if (nRet == 0)
    {
        /* 心跳发送成功 */
        m_lastHeartbeatSuccessTime = std::chrono::system_clock::now();
        /* 重置失败计数 */
        m_nHeartbeatFailureCount = 0;
        dlog_debug("心跳发送成功");

        // 如果当前不是在线状态，更新为在线
        if (m_enCurrentStatus.load() != GB28181::GB28181ClientStatus_E::ONLINE)
        {
            UpdateClientStatus(GB28181::GB28181ClientStatus_E::ONLINE, "心跳发送成功，恢复在线");
        }
    }
    else
    {
        /* 心跳发送失败，累加失败次数 */
        m_nHeartbeatFailureCount++;
        dlog_error("心跳发送失败: %d, 连续失败次数: %d/%d",
                   nRet, m_nHeartbeatFailureCount.load(), m_stClientInfo.stRemote.nMaxHeartTimes);

        // 检查是否超过最大失败次数
        if (m_nHeartbeatFailureCount >= m_stClientInfo.stRemote.nMaxHeartTimes)
        {
            std::string strReason = "心跳发送连续失败 " + std::to_string(m_nHeartbeatFailureCount.load()) + " 次";
            UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, strReason);

            // 重置失败计数，准备重新注册
            m_nHeartbeatFailureCount = 0;

            // 尝试重新注册
            if (m_bThreadRun)
            {
                dlog_info("心跳发送失败次数过多，进入注册退避重试");
                ScheduleRegisterRetry("心跳连续失败");
            }
        }
    }
}

void SIP::SipClient::RegisterExpireAction()
{
    dlog_info("SipClient 注册已到期，重新注册 [%s]", m_pServerDev->GetUri().c_str());
#if SIP_REGISTER_DURATION_ENABLED
    StartRegisterTiming();
#endif
    Register();
}

void SIP::SipClient::RegisterRepeated()
{
    if(!GetRegister())
    {
        //dlog_info("SipClient 注册不上，重新注册 [%s]", m_pServerDev->GetUri().c_str());
        dlog_info("SipClientccccc 注册不上，重新注册");
        Register();
    }
}

void SIP::SipClient::ScheduleRegisterRetry(const std::string &strReason)
{
    if (!m_bThreadRun || m_bStopping.load())
    {
        dlog_warn("GB客户端未运行或正在停止，跳过注册重试: %s", strReason.c_str());
        return;
    }

    int nRetryCount = ++m_nRegisterRetryCount;
    int nDelaySec = nRetryCount * SIP_CLIENT_REGISTER_RETRY_INITIAL_SEC;
    if (nDelaySec > SIP_CLIENT_REGISTER_RETRY_MAX_SEC)
    {
        nDelaySec = SIP_CLIENT_REGISTER_RETRY_MAX_SEC;
    }

    std::thread thrOldRetry;
    unsigned long long ullRetrySeq = ++m_ullRegisterRetrySeq;
    {
        std::lock_guard<std::mutex> lock(m_mtxRegisterRetry);
        if (m_thrRegisterRetry.joinable())
        {
            thrOldRetry = std::move(m_thrRegisterRetry);
        }
    }
    if (thrOldRetry.joinable())
    {
        thrOldRetry.join();
    }

    {
        std::lock_guard<std::mutex> lock(m_mtxRegisterRetry);
        m_thrRegisterRetry = std::thread(&SipClient::RegisterRetryAction, this, nDelaySec, nRetryCount, ullRetrySeq, strReason);
    }

    dlog_warn("注册失败进入退避重试: reason[%s] retry[%d] delay[%d]s",
              strReason.c_str(), nRetryCount, nDelaySec);
}

void SIP::SipClient::CancelRegisterRetry()
{
    std::thread thrOldRetry;
    ++m_ullRegisterRetrySeq;
    {
        std::lock_guard<std::mutex> lock(m_mtxRegisterRetry);
        if (m_thrRegisterRetry.joinable())
        {
            thrOldRetry = std::move(m_thrRegisterRetry);
        }
    }
    if (thrOldRetry.joinable())
    {
        thrOldRetry.join();
    }
}

void SIP::SipClient::RegisterRetryAction(int nDelaySec, int nRetryCount, unsigned long long ullRetrySeq, std::string strReason)
{
    pthread_setname_np(pthread_self(), "SipRegRetry");

    auto tmDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(nDelaySec);
    dlog_info("注册退避任务启动: reason[%s] retry[%d] delay[%d]s", strReason.c_str(), nRetryCount, nDelaySec);

    /* step: 使用steady_clock轮询等待，避免板端condition_variable计时行为差异导致退避任务不触发。 */
    while (std::chrono::steady_clock::now() < tmDeadline)
    {
        if (m_bStopping.load() || !m_bThreadRun || m_ullRegisterRetrySeq.load() != ullRetrySeq)
        {
            dlog_warn("注册退避任务已取消: reason[%s] retry[%d]", strReason.c_str(), nRetryCount);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (!m_bThreadRun || m_bStopping.load() || m_ullRegisterRetrySeq.load() != ullRetrySeq)
    {
        dlog_warn("GB客户端未运行或正在停止，取消本次注册重试");
        return;
    }

    UpdateClientStatus(GB28181::GB28181ClientStatus_E::REGISTERING, "退避后重新注册");
    dlog_info("注册退避时间到，重新发起 REGISTER: reason[%s] retry[%d] delay[%d]s", strReason.c_str(), nRetryCount, nDelaySec);
#if SIP_REGISTER_DURATION_ENABLED
    StartRegisterTiming();
#endif
    Register();
}

#if SIP_REGISTER_DURATION_ENABLED
void SIP::SipClient::StartRegisterTiming()
{
    std::lock_guard<std::mutex> lock(m_mtxRegisterTiming);
    m_stRegisterStartTime = std::chrono::steady_clock::now();
    m_bRegisterTimingActive = true;
}

void SIP::SipClient::LogRegisterDuration()
{
    std::lock_guard<std::mutex> lock(m_mtxRegisterTiming);
    if (!m_bRegisterTimingActive)
    {
        dlog_warn("注册成功，但未记录注册计时起点");
        return;
    }

    const auto stElapsed = std::chrono::steady_clock::now() - m_stRegisterStartTime;
    const auto nElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stElapsed).count();
    m_bRegisterTimingActive = false;
    dlog_info("GB28181注册成功，总耗时[%lld]ms", static_cast<long long>(nElapsedMs));
}
#endif

std::string SIP::SipClient::EncodeChannelID(SipChannelInfo_S stChnInfo)
{

    /* 前十位跟随设备ID */
    std::string strRealm = m_stClientInfo.stLocal.strRealm;
    /* 默认为IPC设备类型-占3位 */
    std::string strType = std::to_string(static_cast<int>(GB28181::DevType_E::PERIPHERY_IPC));
    /* 网络标识编码 */
    std::string strNet = std::to_string(static_cast<int>(GB28181::NetType_E::DEFAULT));
    /* 设备自定义编码-根据通道号作为编码——偏移1位，原本从0开始 */
    int nNum = stChnInfo.nIndex + 1;
    std::ostringstream oss;
    oss << std::setw(6) << std::setfill('0') << nNum; /* 宽度6，填充'0' */
    std::string strChnID = strRealm + strType + strNet + oss.str();
    return strChnID;
}

int SIP::SipClient::CloseCallByCallID(const std::string &strCallID)
{
    /* NOTE 理论上一个会话ID只有一个归属权——优先移除回放的会话ID */
    /* 回放和下载的模式从会话管理中匹配 */
    auto pSession = GetSession(strCallID);
    if (pSession != nullptr)
    {
        /* 通知上层关闭 */
        auto pChn = pSession->GetChn();
        if (pChn)
        {
            SipReadFileAction_S stAction;
            stAction.strCallID = strCallID;
            stAction.enAction = SIP_READFILE_STOP;
            pChn->UpdateReadFileAction(stAction);
        }
        /* 通知完毕即可删除 */
        DelSession(strCallID);
        return 0;
    }

    /* 点播模式可以从通道中匹配 */
    auto pChn = m_pLocalDev->GetChannelByCallID(strCallID);
    if (pChn != nullptr)
    {
        pChn->ClosePlayAction();
        return 0;
    }

    return 0;
}

int SIP::SipClient::UpdateChnInfo(
    std::vector<SipChannelInfo_S> &vecChnInfo)
{
    if (nullptr == m_pLocalDev)
    {
        m_pLocalDev = std::make_shared<Device>(
            m_stClientInfo.stLocal.strID,
            m_stClientInfo.stLocal.strIP,
            std::to_string(m_stClientInfo.stLocal.nPort));
    }
    /* 计算补0的个数 */
    auto nUpdateTatol = vecChnInfo.size();
    if(nUpdateTatol == 0)
    {
        dlog_info("更新通道信息失败，通道数量为空！");
        return -1;
    }
    int nFillCount = 0;
    while (nUpdateTatol != 0)
    {
        nUpdateTatol /= 10;
        nFillCount++;
    }

    for (size_t i = 0; i < vecChnInfo.size(); i++)
    {
        /* 没有名字的通道统一给IP Camera + 通道编号 */
        auto &item = vecChnInfo[i];
        if (item.strName.empty())
        {
            int nNum = item.nIndex + 1;
            std::ostringstream oss;
            /* 填充'0' */
            oss << std::setw(nFillCount) << std::setfill('0') << nNum;
            item.strName = "IP Camera " + oss.str();
        }
        /* 编码通道ID */
        //item.strChannelID = EncodeChannelID(item);
        auto channel = m_pLocalDev->GetChannel(item.strChannelID);
        if (nullptr == channel)
        {
            /* 创建客户端的通道类型 */
            channel = std::make_shared<Channel>(item, m_stCbInfo, false);
            /* 更新一些必备信息 */
            channel->SetParentID(m_stClientInfo.stRemote.strID);
            /* TODO 字段空值校验，需要完善 */
            m_pLocalDev->InsertChannel(
                m_stClientInfo.stRemote.strID,
                item.strChannelID,
                channel);
#if SIP_CLIENT_DEBUG
            dlog_info("添加通道[%s]状态[%s]",
                       channel->GetChannelID().c_str(),
                       channel->GetStatus().c_str());
#endif
        }
        dlog_debug("添加通道[%s]状态[%s]",channel->GetChannelID().c_str(),channel->GetStatus().c_str());
        channel->SetExternIP(m_pLocalDev->GetIP());//channel->SetExternIP(m_stClientInfo.stLocal.strIP);//add by longll 响应INVITE 200 ok 应该拿服务器的IP,
        dlog_debug("*********m_pLocalDev->GetIP() = [%s]状态hannel->GetExternIP()=[%s]",m_pLocalDev->GetIP().c_str(),channel->GetExternIP().c_str());
        channel->UpdateInfo(item);
        channel->UploadInfo();
    }
    /* 发送Notify通知，设备目录信息发生变更 */
    m_stSubscribeDeal.NotifyCatalog();
    return 0;
}

int SIP::SipClient::GetChnInfo(std::vector<Channel::Ptr> &vecChnInfo)
{
    if (m_pLocalDev == nullptr)
    {
        return -1;
    }
    vecChnInfo = m_pLocalDev->GetAllChannels();
    return vecChnInfo.size();
}

int SIP::SipClient::GetIndexByChnID(const std::string &strChnID)
{
    if (m_pLocalDev == nullptr)
    {
        return -1;
    }
    auto pChn = m_pLocalDev->GetChannel(strChnID);
    if (pChn == nullptr)
    {
        return -2;
    }
    return pChn->nIndex;
}

std::string SIP::SipClient::GetChnIDByIndex(const int &nIndex)
{
    if (m_pLocalDev == nullptr)
    {
        return std::string();
    }
    auto pChn = m_pLocalDev->GetChannelByIndex(nIndex);
    if (pChn == nullptr)
    {
        return std::string();
    }
    return pChn->GetChannelID();
}

void SIP::SipClient::UpdateClientStatus(GB28181::GB28181ClientStatus_E enStatus, const std::string& strReason)
{
    GB28181::GB28181ClientStatus_E oldStatus = m_enCurrentStatus.load();

    // 如果状态没有变化，则不触发回调
    if (oldStatus == enStatus)
    {
        return;
    }

    m_enCurrentStatus = enStatus;

    /*TODO 回调上抛状态 */
    SipCbResult_S stResult;
    {
        auto fnCb = ::SipModule::instance()->GetOnlineStatusCb();
        if (fnCb)
        {
            fnCb(enStatus, stResult);
        }
    }

    dlog_info("GB28181客户端状态变化: %d -> %d, 原因: %s",
              static_cast<int>(oldStatus), static_cast<int>(enStatus), strReason.c_str());
}

bool SIP::SipClient::get_client_status()
{
    return m_enCurrentStatus == GB28181::GB28181ClientStatus_E::ONLINE ? true :false;
}
