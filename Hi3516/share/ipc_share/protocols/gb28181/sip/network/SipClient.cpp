/**
 * @FilePath     : SipClient.cpp
 * @Author       : EasonLu
 * @Date         : 2025-02-24 09:23:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-23 09:09:06
 * @Description  : SIP客户端 
 */

#include "SipClient.h"
#include "GbDefine.h"
#include "MediaSession.h"
#include "MethodRequest.h"
#include "ModuleLog.h"
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
        MLOG_ERROR("eXosip_init failed");
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
        MLOG_ERROR("eXosip_listen_addr failed");
        eXosip_quit(m_pSipContext);
        return false;
    }

    /* 检测是否配置了对外IP */
    if (m_stClientInfo.stLocal.strIP.empty())
    {
        /* 没有配置，则进行猜测 */
        char achIP[32] = {0};
        eXosip_guess_localip(m_pSipContext, AF_INET, achIP, (int)sizeof(achIP));
        MLOG_INFO("SipClient没有设置对外IP，猜测IPv4为[%s]", achIP);
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
    MLOG_DEBUG("proxy[%s] nat_ip[%s] nat_port[%d]",
               stAccountInfo.proxy, stAccountInfo.nat_ip, stAccountInfo.nat_port);
    nRet = eXosip_set_option(m_pSipContext, EXOSIP_OPT_ADD_ACCOUNT_INFO, &stAccountInfo);
    MLOG_INFO("eXosip_set_option EXOSIP_OPT_ADD_ACCOUNT_INFO [%d]", nRet);
    int nEnable = 1;
    nRet = eXosip_set_option(m_pSipContext, EXOSIP_OPT_ENABLE_REUSE_TCP_PORT, &nEnable);
    MLOG_INFO("eXosip_set_option EXOSIP_OPT_ENABLE_REUSE_TCP_PORT [%d]", nRet);
    nEnable = 0;
    nRet = eXosip_set_option(m_pSipContext, EXOSIP_OPT_USE_RPORT, &(m_stServerInfo.nPort));
    MLOG_INFO("eXosip_set_option EXOSIP_OPT_USE_RPORT [%d]", nRet);
#endif
    m_bThreadRun = true;
    if (nullptr == m_thrListen)
    {
        m_thrListen = nullptr;
    }
    m_thrListen = std::make_shared<std::thread>(&SipClient::RecvEventThread, this);
    MLOG_INFO("SipClient Start IP[%s] Port[%d]",
              m_stServerInfo.strIP.c_str(),
              m_stServerInfo.nPort);
    
    /* 设置初始状态为注册中 */ 
    UpdateClientStatus(GB28181::GB28181ClientStatus_E::REGISTERING, "开始注册");

    /* 开启注册请求 */
    Register();
    //StartRegisterThr();
    
    return true;
}

bool SipClient::Stop()
{
    MLOG_INFO("执行GB客户端停止操作");
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
        MLOG_INFO("创建注销操作线程")
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
    MLOG_INFO("执行GB客户端停止操作完毕");
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
    MLOG_DEBUG("发送通道ID[%s]媒体数据[%d]", device.strChnID.c_str(), nLen);
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
    MLOG_INFO("SipClient Message New Method [%s], status_code [%d]",
              exosip_event->request->sip_method,
              exosip_event->request->status_code);

    /*gb35114控制信令响应分析,分析GB35114 Date、Note字段*/
    if (OK != CGm::instance()->gm_analyzing_control_signaling(exosip_event))
    {
        MLOG_ERROR("gb35114控制信令响应分析,分析GB35114 Date、Note字段失败");
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
            MLOG_INFO("SipClient Message Info Body\n%s", body->body);
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
                MLOG_ERROR("gb35114注册请求的服务端二次响应分析,分析GB35114 Securityinfo字段失败");
            }

            /* NOTE 可校验注册的服务器和返回消息的服务器一致 */
            StartHeartbeatThr();
            /* 注册成功，注册成功标志位置 true */
            m_bIsRegisterSucceed = true;
            MLOG_INFO("注册成功 [%s]",
                      m_pServerDev == nullptr ? "" : m_pServerDev->GetUri().c_str());
            m_bRegister = true;

            /* 更新状态为在线 */ 
            UpdateClientStatus(GB28181::GB28181ClientStatus_E::ONLINE, "注册成功");    

            /* 校时 */ 
            osip_header_t *dest = NULL;
            osip_message_get_date(e->m_pEvent->response, 0, &dest);
            if (nullptr != dest)
            {
                MLOG_INFO("time:[%s]", dest->hvalue);
                /*TODO设备校时上抛*/
                if (std::strlen(dest->hvalue) > 0)
                {
                     //SipSetSystemTime(dest->hvalue);
                    MLOG_INFO("time:[%s]", dest->hvalue);
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
                        MLOG_INFO("校时回调上抛设置成功 time:[%s]", dest->hvalue);
                    }
                    else
                    {
                        MLOG_INFO("校时回调上抛设置失败 time:[%s]", dest->hvalue);
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
                MLOG_INFO("唤醒注销线程");
                /* 唤醒注销线程 */
                m_pThrLogout->notify();
            }
        }
    }
    return 0;
}

int SIP::SipClient::on_registration_failure(const SipEvent::Ptr &e)
{
    if (e->m_pEvent->response == nullptr)
    {
        /* TODO 超时没响应，需单开线程一直注册，并上抛服务器不在线状态 */
        UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, "注册超时无响应");
        return -1;
    }
    int nStatus = e->m_pEvent->response->status_code;
    MLOG_WARN("注册失败，状态码[%d]", nStatus);

    /* 更新状态为离线，并说明原因 */ 
    std::string strReason = "注册失败，状态码: " + std::to_string(nStatus);
    UpdateClientStatus(GB28181::GB28181ClientStatus_E::OFFLINE, strReason);

    /* 403 407 */
    if (SIP_FORBIDDEN == nStatus || SIP_PROXY_AUTHENTICATION_REQUIRED == nStatus)
    {
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
            //std::cout << "Redirected to: " << new_uri << std::endl;
            MLOG_INFO("Redirected to:[%s]", new_uri);
            
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
            MLOG_ERROR("初始化注册信息失败");
        }

        /*构建gb35114第一次注册authorization授权字段*/
        eXosip_lock(m_pSipContext);
        if (OK != CGm::instance()->gm_build_first_register(pRegisterMsg))
        {
            MLOG_ERROR("构建gb35114第一次注册authorization授权字段失败");
        }

        /* 发送注册信息 */
        auto ret = eXosip_register_send_register(m_pSipContext, m_nRegisterID, pRegisterMsg);
        eXosip_unlock(m_pSipContext);
        if (OSIP_SUCCESS == ret)
        {
            MLOG_INFO("发送注册信息成功 Uri[%s]", m_pServerDev->GetUri().c_str());
        }
    }
    else if(SIP_UNAUTHORIZED == nStatus)    // 401
    {
        int nRet = 0;
        eXosip_lock(m_pSipContext);
        /* 清理注册信息 */
        nRet = eXosip_clear_authentication_info(m_pSipContext);
        if (nRet < 0)
        {
            eXosip_unlock(m_pSipContext);
            MLOG_ERROR("清除eXosip中存储的所有认证凭据失败: %d", nRet);
            return -1;
        }
        /*添加主叫用户的认证信息*/
        osip_www_authenticate_t *www_authenticate_header = nullptr;
        osip_message_get_www_authenticate(e->m_pEvent->response, 0, &www_authenticate_header);
        if (www_authenticate_header)
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
                MLOG_ERROR("eXosip_add_authentication_info failed: %d", nRet);
                return -1;
            }
        }
        /* 注册有效期 */
        int nExpires = m_stClientInfo.stRemote.nExpires;
        /* 构建第二次注册信息 */
        osip_message_t *pRegisterMsg = nullptr;
        nRet = eXosip_register_build_register(m_pSipContext, m_nRegisterID, nExpires, &pRegisterMsg);
        if (nRet < 0)
        {
            eXosip_unlock(m_pSipContext);
            MLOG_ERROR("eXosip_register_build_register failed: %d", nRet);
            return -1;
        }

        char *pDest = nullptr;
        size_t nLength = 0;
        osip_message_to_str(e->m_pEvent->response, &pDest, &nLength);
        MLOG_INFO("服务器返回的401注册消息:\n%s", pDest);
        osip_free(pDest);

        /* 构建gb35114第二次注册authorization授权字段 */
        if (OK != CGm::instance()->gm_build_second_register(e->m_pEvent->response, pRegisterMsg, m_stClientInfo))
        {
            eXosip_unlock(m_pSipContext);
            MLOG_ERROR("构建gb35114第二次注册authorization授权字段失败");
            return -1;
        }
        

        /* 发送注册信息 */
        nRet = eXosip_register_send_register(m_pSipContext, m_nRegisterID, pRegisterMsg);
        eXosip_unlock(m_pSipContext);
        if (OSIP_SUCCESS == nRet)
        {
            MLOG_INFO("发送注册信息成功 Uri[%s]", m_pServerDev->GetUri().c_str());
        }
        else
        {
            MLOG_ERROR("eXosip_register_send_register authorization error!  nRet=%d", nRet);
            return -1;
        }
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

    MLOG_INFO("on_call_message_new response reqid:[%s] callid:[%s]",
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
        MLOG_WARN("on_call_message_new 未处理的消息类型 [%s]",
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
            MLOG_INFO("注册服务器URI[%s]",
                      m_pServerDev->GetUri().c_str());
        }
        else
        {
            if (m_pServerDev != nullptr)
            {
                MLOG_INFO("注销服务器URI[%s]",
                          m_pServerDev->GetUri().c_str());
            }
        }
    }
    eXosip_lock(m_pSipContext);
    /* 清理注册信息 */
    int nRet = eXosip_clear_authentication_info(m_pSipContext);
    if(nRet != 0)
    {
        MLOG_ERROR("清理注册信息失败:%d", nRet);
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
        MLOG_ERROR("初始化注册信息失败");
    }

    /*构建gb35114第一次注册authorization授权字段*/
    if (OK != CGm::instance()->gm_build_first_register(pRegisterMsg))
    {
        MLOG_ERROR("构建gb35114第一次注册authorization授权字段失败, error code: %d", nRet);
    }

    /* 发送注册信息 */
    auto ret = eXosip_register_send_register(m_pSipContext, m_nRegisterID, pRegisterMsg);
    eXosip_unlock(m_pSipContext);
    if (OSIP_SUCCESS == ret)
    {
        MLOG_INFO("发送注册信息成功 Uri[%s]", m_pServerDev->GetUri().c_str());
    }
}

void SIP::SipClient::LogoutAction()
{
    MLOG_INFO("注销操作开始");
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

    if (m_pThrHeartbeat != nullptr)
    {
        m_pThrHeartbeat->stop();
        delete m_pThrHeartbeat;
        m_pThrHeartbeat = nullptr;
    }

    if (m_pThrRegister != nullptr)
    {
        m_pThrRegister->stop();
        delete m_pThrRegister;
        m_pThrRegister = nullptr;
    }

    m_bRegister = false;
    MLOG_INFO("SipClient 注销成功");
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
    MLOG_INFO("[SipClient]处理事件:[%d][%s][%s]\n%s",
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
        MLOG_ERROR("服务器设备信息为空，无法发送心跳");
        return;
    }
    MLOG_INFO("SipClient 发送心跳 [%s]", m_pServerDev->GetUri().c_str());
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
        MLOG_DEBUG("心跳发送成功");
        
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
        MLOG_ERROR("心跳发送失败: %d, 连续失败次数: %d/%d", 
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
                MLOG_INFO("心跳发送失败次数过多，尝试重新注册");
                Register();
                UpdateClientStatus(GB28181::GB28181ClientStatus_E::REGISTERING, "心跳失败后重新注册");
            }
        }
    }
}

void SIP::SipClient::RegisterExpireAction()
{
    MLOG_INFO("SipClient 注册已到期，重新注册 [%s]", m_pServerDev->GetUri().c_str());
    Register();
}

void SIP::SipClient::RegisterRepeated()
{
    if(!GetRegister())
    {
        //MLOG_INFO("SipClient 注册不上，重新注册 [%s]", m_pServerDev->GetUri().c_str());
        MLOG_INFO("SipClientccccc 注册不上，重新注册");
        Register();
    }
}

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
        MLOG_INFO("更新通道信息失败，通道数量为空！");
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
            MLOG_INFO("添加通道[%s]状态[%s]",
                       channel->GetChannelID().c_str(),
                       channel->GetStatus().c_str());
#endif
        }
        MLOG_DEBUG("添加通道[%s]状态[%s]",channel->GetChannelID().c_str(),channel->GetStatus().c_str());
        channel->SetExternIP(m_pLocalDev->GetIP());//channel->SetExternIP(m_stClientInfo.stLocal.strIP);//add by longll 响应INVITE 200 ok 应该拿服务器的IP, 
        MLOG_DEBUG("*********m_pLocalDev->GetIP() = [%s]状态hannel->GetExternIP()=[%s]",m_pLocalDev->GetIP().c_str(),channel->GetExternIP().c_str());
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
    
    MLOG_INFO("GB28181客户端状态变化: %d -> %d, 原因: %s", 
              static_cast<int>(oldStatus), static_cast<int>(enStatus), strReason.c_str());
}

bool SIP::SipClient::get_client_status()
{
    return m_enCurrentStatus == GB28181::GB28181ClientStatus_E::ONLINE ? true :false;
}

