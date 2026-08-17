/**
 * @FilePath     : control_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-03-27 19:38:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 14:10:24
 * @Description  : 控制事务任务管理
 */

#include "control_manage.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "time_manage.h"
#include "user_manage.h"
#include "system_manage.h"

#include "ip_filter_manage.h"
#include "bonjour_manage.h"
#include "ddns_manage.h"
#include "email_manage.h"
#include "pppoe_manage.h"
#include "snmp_manage.h"
#include "upnp_manage.h"
#include "qos_manage.h"
#include "network_manage.h"
#include "platform_manager.h"
#include "log_handler.h"
#include "register_manage.h"
#include "preview_manage.h"
#include "https_manage.h"

#include "operation_client.h"
#include "burn_mac_udp_server.h"
#include "multicast_udp_server.h"
#include "ipc_multicast_server.h"
#include "upgrade_client.h"
#include "gb28181.hpp"
#include "onvif_server.h"
#include "record_server.h"
#include "stream_server.h"
#include "web_server.h"
#include "web_ssl_server.h"

#include "action_code.h"
#include "path_define.h"
#include "retrieval_task.h"
#include "replay_task.h"
#include "storage_manage_task.h"
#include "isp_manage.h"
#include "record_ctrl.h"
#include "capture_ctrl.h"
#include "storage_manage.h"
#include "onvif_firmware_upgrade_server.h"
#include "event_abnormal_detector.h"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif
#include "wifi_manage.h"
#include "production_test_task.h"
#include "4g_manage.h"
#include "hostapd_manager.h"
#ifdef ENABLE_AI_STUDENT
#include "ai_student_business.hpp"
#include "ai_student_task.h"
#endif



/* 初始化入口 */
IpcRet_E ControlManage::init()
{
    /* 确保只初始化一次 */
    if (m_initialized)
    {
        return ERR;
    }
    int nRet = OK;
    /* 绑定任务 */
    std::shared_ptr<CTaskManage> pTaskManage = nullptr;
    bind_task(pTaskManage);
    if (!pTaskManage)
    {
        dlog_error("pTaskManage is nullptr");
        return ERR;
    }
    /* 初始化事务 */
    nRet = init_business();
    if (nRet < OK)
    {
        dlog_error("事务初始化失败：%d", nRet);
        /* 初始化失败，进行去初始化 */ 
        deinit_business();
        return ERR;
    }
    /* 初始化服务 */
    nRet = init_server(pTaskManage);
    if (nRet < OK)
    {
        dlog_error("服务初始化失败：%d", nRet);
        /* 初始化失败，进行去初始化 */ 
        deinit_server();
        return ERR;
    }
    /* 图像初始化完后加载 */
    CIspManage::instance()->apply_awb_config();
    m_initialized = true;

    return OK;
}

/* 去初始化入口 */
IpcRet_E ControlManage::deinit()
{
    m_initialized = false;
    deinit_server();
    deinit_business();
    return OK;
}

/* 初始化事务 */
int ControlManage::init_business()
{
    int nRet = OK;
    /* 用户初始化 */
    nRet = CUserManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("用户管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* 时间初始化 */
    nRet = CTimeManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("时间管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* 系统管理初始化 */
    nRet = SystemManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("系统管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* 初始化日志上传开关（从配置文件读取） */
    LogHandler::instance()->initLogUpload();
    /* ip地址过滤初始化 */
    nRet = CIpFilterManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("ip地址过滤管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* bonjour初始化 */
    nRet = CBonjourManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("bonjour管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* ddns初始化 */
    nRet = CDdnsManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("ddns管理模块初始化失败：%d", nRet);
        return nRet;
    }
#if CAP_NETWORK_WIFI
    /* wifi初始化 */
    // nRet = CWifiManager::instance()->init();
    // if (nRet < OK)
    // {
    //     dlog_error("wifi管理模块初始化失败：%d", nRet);
    //     return nRet;
    // }
    Network::HotspotConfig hotspotConfig;
    HostapdManager::instance()->get_hostapd_config(hotspotConfig);

    if (hotspotConfig.enabled)
    {
        /* 热点和 WiFi STA 共用 wlan0，配置为热点时只恢复热点，不能再启动 STA。 */
        InitResult hotspotResult = HostapdManager::instance()->Init(hotspotConfig, "eth0");
        if (hotspotResult != InitResult::SUCCESS)
        {
            dlog_error("热点开机恢复失败：%d", static_cast<int>(hotspotResult));
            return ERR;
        }
        dlog_info("热点已根据持久化配置自动恢复：%s", hotspotConfig.ssid.c_str());
    }
    else
    {        /* 热点未启用时，按原逻辑启动 WiFi STA。 */
        nRet = CWifiManager::instance()->init();
        if (nRet < OK)
        {
            dlog_error("wifi管理模块初始化失败：%d", nRet);
            return nRet;
        }
    }
#endif
#if CAP_NETWORK_4G
    nRet = FourGManager::instance()-> init();
    if (nRet < OK)
    {
        dlog_error("4G管理模块初始化失败：%d", nRet);
        // return nRet;
    }
#endif
    /* 邮件初始化 */
    nRet = CEmailManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("邮件管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* pppoe初始化 */
    nRet = CPppoeManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("pppoe管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* snmp初始化 */
    nRet = CSnmpManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("snmp管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* upnp初始化 */
    nRet = CUpnpManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("upnp管理模块初始化失败：%d", nRet);
        return nRet;
    }
    /* qos初始化 */
    nRet = CQosManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("qos管理模块初始化失败：%d", nRet);
        return nRet;
    }
#if CAP_GARBAGE_STATION_PLATFORM
    /* 平台管理初始化 */
    nRet = CPlatformManager::instance()->init();
    if (nRet < OK)
    {
        dlog_error("平台管理模块初始化失败：%d", nRet);
        /* 平台管理初始化失败不阻塞主流程，记录日志后继续 */
    }
#endif
    /* 注册激活初始化 */
    nRet = CRegisterManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("注册激活模块初始化失败：%d", nRet);
        return nRet;
    }
    /* 预览初始化 */
    nRet = CPreviewManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("预览模块初始化失败：%d", nRet);
        return nRet;
    }
    /* 图像初始化 */
    nRet = CIspManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("图像模块初始化失败：%d", nRet);
    }
    /* 存储管理初始化 */
    nRet = CStorageManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("存储管理模块初始化失败：%d", nRet);
    }
    /* 录制初始化 */
    nRet = CRecordCtrl::instance()->init();
    if (nRet < OK)
    {
        dlog_error("录制模块初始化失败：%d", nRet);
    }
    /* 抓图初始化 */
    nRet = CCaptureCtrl::instance()->init();
    if (nRet < OK)
    {
        dlog_error("抓图模块初始化失败：%d", nRet);
    }
    /* 异常报警检测初始化，启动检测线程并加载已持久化的联动配置 */
    nRet = CAbnormalDetector::instance()->init();
    if (nRet < OK)
    {
        dlog_error("异常报警检测初始化失败：%d", nRet);
    }
    /* onvif http固件升级服务初始化 */
    nRet = COnvifFirmwareUpgradeServer::instance()->init();
    if (nRet < OK)
    {
        dlog_error("onvif http 固件升级服务初始化失败：%d", nRet);
    }
#ifdef ENABLE_AI_STUDENT
    /* ai student */
    AiStudentBusiness_NS::CAiStudentBusiness::instance()->init();
#endif
    return nRet;
}

/* 去初始化事务 */
void ControlManage::deinit_business()
{
    int nRet = OK;

    /* 异常报警检测去初始化，停止检测线程 */
    nRet = CAbnormalDetector::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("异常报警检测去初始化失败：%d", nRet);
    }
    /* onvif http固件升级服务去初始化 */
    nRet = COnvifFirmwareUpgradeServer::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("onvif http固件升级服务去初始化失败：%d", nRet);
    }
    /* 抓图去初始化 */
    nRet = CCaptureCtrl::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("抓图模块去初始化失败：%d", nRet);
    } 
    /* 录制去初始化 */
    nRet = CRecordCtrl::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("录制模块去初始化失败：%d", nRet);
    } 
    /* 存储管理去初始化 */
    nRet = CStorageManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("存储管理模块去初始化失败：%d", nRet);
    }
    /* 图像去初始化 */
    nRet = CIspManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("图像模块去初始化失败：%d", nRet);
    } 
    /* 预览去初始化 */
    nRet = CPreviewManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("预览模块去初始化失败：%d", nRet);
    }
    /* https去初始化 */
    nRet = CHttpsManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("https去初始化失败：%d", nRet);
    }
    /* 注册激活去初始化 */
    nRet = CRegisterManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("注册激活模块去初始化失败：%d", nRet);
    }
#if CAP_GARBAGE_STATION_PLATFORM
    /* 平台管理去初始化 */
    nRet = CPlatformManager::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("平台管理模块去初始化失败：%d", nRet);
    }
#endif
    /* qos去初始化 */
    nRet = CQosManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("qos管理模块去初始化失败：%d", nRet);
    }
    /* upnp去初始化 */
    nRet = CUpnpManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("upnp管理模块去初始化失败：%d", nRet);
    }
    // /* snmp去初始化 */
    // nRet = CSnmpManage::instance()->deinit();
    // if (nRet < OK)
    // {
    //     dlog_error("snmp管理模块去初始化失败：%d", nRet);
    // }
    /* pppoe去初始化 */
    nRet = CPppoeManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("pppoe管理模块去初始化失败：%d", nRet);
    }
    /* 邮件去初始化 */
    nRet = CEmailManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("邮件管理模块去初始化失败：%d", nRet);
    }
    /* ddns去初始化 */
    nRet = CDdnsManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("ddns管理模块去初始化失败：%d", nRet);
    }
    /* bonjour去初始化 */
    nRet = CBonjourManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("bonjour管理模块去初始化失败：%d", nRet);
    }
    /* ip地址过滤初始化 */
    nRet = CIpFilterManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("ip地址过滤管理模块去初始化失败：%d", nRet);
    }
    /* 系统管理去初始化 */
    nRet = SystemManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("系统管理模块去初始化失败：%d", nRet);
    }
    /* 时间去初始化 */
    nRet = CTimeManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("时间管理模块去初始化失败：%d", nRet);
    }
    /* 用户去初始化 */
    nRet = CUserManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("用户管理模块去初始化失败：%d", nRet);
    }
}

/* 初始化服务 */
int ControlManage::init_server(std::shared_ptr<CTaskManage> &pTaskManage)
{
    int nRet = OK;
    /* 初始化运维日志服务 */
    COperationClient::instance()->set_taskManage(pTaskManage);
    nRet = COperationClient::instance()->init();
    if (nRet < OK)
    {
        dlog_error("运维日志服务初始化失败：%d", nRet);
        return nRet;
    }
    /* 初始化烧录MAC服务 */
    nRet = CBurnMacUdpServer::instance()->init();
    if (nRet < OK)
    {
        dlog_error("烧录MAC服务端初始化失败：%d", nRet);
        return nRet;
    }
    /* 初始化组播服务 */
    nRet = CMulticastUdpServer::instance()->init();
    if (nRet < OK)
    {
        dlog_error("组播服务端初始化失败：%d", nRet);
        return nRet;
    }
    /* 初始化多播服务 */
    nRet = CIpcMulticastServer::instance()->init();
    if (nRet < OK)
    {
        dlog_error("多播服务端初始化失败：%d", nRet);
        return nRet;
    }
    /* web服务端初始化 */
    CWebServer::instance()->set_taskManage(pTaskManage);
    nRet = CWebServer::instance()->init();
    if (nRet < OK)
    {
        dlog_error("web服务端初始化失败：%d", nRet);
        return nRet;
    }
    /* web加密端初始化 */
    CWebSslServer::instance()->set_taskManage(pTaskManage);
    /* https初始化 */
    nRet = CHttpsManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("https初始化失败：%d", nRet);
        return nRet;
    }
    /* GB28181客户端初始化 */
    nRet = CGB28181::instance()->init();
    if(nRet < OK)
    {
        dlog_error("GB28181模块初始化失败：%d", nRet);
        return nRet;
    }
    /* onvif服务端初始化 */
    nRet = COnvifServer::instance()->init();
    if (nRet < OK)
    {
        dlog_error("onvif服务端初始化失败：%d", nRet);
        return nRet;
    }
    /* 系统升级客户端初始化 */
    CUpgradeClient::instance()->set_taskManage(pTaskManage);
    nRet = CUpgradeClient::instance()->init();
    if(nRet < OK)
    {
        dlog_error("系统升级模块初始化失败：%d", nRet);
        return nRet;
    }

    /* 送数据至录制的服务端初始化 */
    CStreamServer::instance()->init();

    /* 录制控制服务端初始化 */
    CRecordServer::instance()->set_taskManage(pTaskManage);
    // CRecordServer::instance()->set_statusObserver(std::bind(&SystemManage::notify_programStatus, SystemManage::instance(), std::placeholders::_1, std::placeholders::_2));
    CRecordServer::instance()->init();

#ifdef ENABLE_TVSDK_SRC
    /* TVSDK 服务端初始化 */
    m_pTvSdkServer = std::make_unique<CTvSdkServer>();
    m_pTvSdkServer->set_taskManage(pTaskManage);
    nRet = m_pTvSdkServer->init();
    if (nRet < OK)
    {
        dlog_error("TVSDK服务端初始化失败：%d", nRet);
        return nRet;
    }
#endif

#if CAP_GARBAGE_STATION_PLATFORM
    /* 平台管理模块注入 CTaskManage，用于 MQTT SDK 网关命令转发 */
    CPlatformManager::instance()->set_taskManage(pTaskManage.get());
#endif

    /* 设备激活客户端初始化 */
    // CRegisterClient::instance()->set_taskManage(pTaskManage);
    // CRegisterClient::instance()->init();;
    /* Sdk服务端初始化 */
    // SdkServer::instance()->set_taskManage(pTaskManage);
    // SdkServer::instance()->init();
    /* 平台服务端初始化 */
    // PlatformServer::instance()->set_taskManage(pTaskManage);
    // PlatformServer::instance()->init();
    /* 回放初始化 */
    // ReplayClient::instance()->set_taskManage(pTaskManage);
    // ReplayClient::instance()->init();

#ifdef ENABLE_GAT1400_SRC
    /* GAT1400模块初始化 */
    GAT1400::CGAT1400::instance()->init();
#endif

    return OK;
}

/* 去初始化服务 */
void ControlManage::deinit_server()
{
    int nRet = OK;

#ifdef ENABLE_TVSDK_SRC
    /* TVSDK 服务端去初始化 */
    if (m_pTvSdkServer)
    {
        m_pTvSdkServer->deinit();
        m_pTvSdkServer.reset();
    }
#endif

    /* 系统升级客户端去初始化 */
    nRet = CUpgradeClient::instance()->deinit();
    if(nRet < OK)
    {
        dlog_error("系统升级模块去初始化失败：%d", nRet);
    }
    /* 去初始化onvif服务端 */
    nRet = COnvifServer::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("onvif服务端去初始化失败：%d", nRet);
    }
    /* 去初始化GB28181模块 */
    nRet = CGB28181::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("GB28181模块去初始化失败：%d", nRet);
    }
    /* 去初始化https */
    nRet = CHttpsManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("https去初始化失败：%d", nRet);
    }
    /* 去初始化web服务端 */
    nRet = CWebServer::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("web服务端去初始化失败：%d", nRet);
    }
    /* 去初始化多播服务 */
    nRet = CIpcMulticastServer::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("多播服务端去初始化失败：%d", nRet);
    }
    /* 去初始化组播服务 */
    nRet = CMulticastUdpServer::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("组播服务端去初始化失败：%d", nRet);
    }
    /* 去初始化烧录MAC服务 */
    nRet = CBurnMacUdpServer::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("烧录MAC服务端去初始化失败：%d", nRet);
    }
    /* 去初始化运维日志服务 */
    nRet = COperationClient::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("运维日志服务去初始化失败：%d", nRet);
    }
}

#ifdef ENABLE_TVSDK_SRC
int ControlManage::tvsdk_push_alarm(int lCommand, const void *pAlarmInfo, int dwBufLen, const void *pAlarmer)
{
    if (!m_pTvSdkServer)
    {
        return ERR;
    }

    return m_pTvSdkServer->push_alarm(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}

/**
 * @brief 向已连接 TVSDK 客户端推送动态图片 V2 告警。
 * @param [in] lCommand 告警命令码。
 * @param [in] pAlarmInfo V2 告警结构体。
 * @param [in] dwBufLen 告警结构体长度。
 * @param [in] pAlarmer 可选的告警设备信息。
 * @return 成功返回 OK，失败返回 ERR。
 */
int ControlManage::tvsdk_push_alarm_v2(int lCommand,
                                       const void *pAlarmInfo,
                                       int dwBufLen,
                                       const void *pAlarmer)
{
    if (!m_pTvSdkServer)
    {
        return ERR;
    }

    return m_pTvSdkServer->push_alarm_v2(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}

int ControlManage::tvsdk_get_client_count() const
{
    if (!m_pTvSdkServer)
    {
        return ERR;
    }

    return m_pTvSdkServer->get_client_count();
}
#endif


/* 绑定任务 */
void ControlManage::bind_task(std::shared_ptr<CTaskManage> &pTaskManage)
{
    pTaskManage = std::make_shared<CTaskManage>();

    /* 用户相关 */
    pTaskManage->bind<Task::User::Login>(AC_LOGIN);
    pTaskManage->bind<Task::User::Add>(AC_ADD_USER_INFO);
    pTaskManage->bind<Task::User::Del>(AC_DEL_USER_INFO);
    pTaskManage->bind<Task::User::Update>(AC_SET_USER_INFO);
    pTaskManage->bind<Task::User::Find>(AC_GET_USER_INFO);
    pTaskManage->bind<Task::User::GetLoginErrorInfo>(AC_GET_LOGIN_INFO);
    pTaskManage->bind<Task::User::GetAllUser>(AC_GET_ALLUSER_INFO);
    pTaskManage->bind<Task::User::VerificationAdmin>(AC_VER_ADMIN_INFO);
    // pTaskManage->bind<Task::User::SetPreviewChannle>(AC_SET_USER_LOCK_CHANNLE);
    // pTaskManage->bind<Task::User::GetPreviewChannle>(AC_GET_USER_LOCK_CHANNLE);
    pTaskManage->bind<Task::User::GetOnlineUser>(AC_GET_ONLINE_USERS);
    pTaskManage->bind<Task::User::DeleteOnlinUser>(AC_DELETE_ONLINE_USERS);
    pTaskManage->bind<Task::User::UserPermissionAuth>(AC_USER_PERMISSION_AUTH);
    pTaskManage->bind<Task::User::DeleUserExit>(AC_DEL_AND_EXIT_USER);
    pTaskManage->bind<Task::User::UpdateUserExit>(AC_UPDATE_AND_EXIT_USER);
    pTaskManage->bind<Task::User::UpdateLocalOnlineUser>(AC_LOCAL_ONLINE_USER);
    pTaskManage->bind<Task::User::UpdateStatus>(AC_UNLOCK_USER_STATUS);
    pTaskManage->bind<Task::User::ResetPassword>(AC_RESET_USER_PASSWORD);

    /* 系统配置相关 */
    pTaskManage->bind<Task::WebPlugin::GetParam>(AC_GET_WEB_PLUGIN_PARAM);
    pTaskManage->bind<Task::WebPlugin::SetParam>(AC_SET_WEB_PLUGIN_PARAM);
    pTaskManage->bind<Task::System::GetDeviceInfo>(AC_GET_DEVICE_INFO);
    pTaskManage->bind<Task::System::SetDeviceInfo>(AC_SET_DEVICE_INFO);
    pTaskManage->bind<Task::System::GetTimeInfo>(AC_GET_TIME_INFO);
    pTaskManage->bind<Task::System::SetTimeInfo>(AC_SET_TIME_INFO);
    pTaskManage->bind<Task::System::GetNowTime>(AC_GET_NOW_TIME);
    pTaskManage->bind<Task::System::TestNtp>(AC_TEST_NTPSERVER);
    pTaskManage->bind<Task::System::GetDeviceConfig>(AC_GET_DEVICE_CONFIG);
    pTaskManage->bind<Task::System::SetDeviceConfig>(AC_SET_DEVICE_CONFIG);
    pTaskManage->bind<Task::System::GetPeripheralConfig>(AC_GET_PERIPHERAL_INFO);
    pTaskManage->bind<Task::System::SetPeripheralConfig>(AC_SET_PERIPHERAL_INFO);
    pTaskManage->bind<Task::System::GetSmartEventEnableStatus>(AC_GET_SMART_EVENT_ENABLE_STATUS);
    pTaskManage->bind<Task::System::SetSmartEventEnableStatus>(AC_SET_SMART_EVENT_ENABLE_STATUS);
    pTaskManage->bind<Task::System::GetMetadataConfig>(AC_GET_METADATA_CONFIG);
    pTaskManage->bind<Task::System::SetMetadataConfig>(AC_SET_METADATA_CONFIG);

    /* 系统维护相关 */
    pTaskManage->bind<Task::System::Reboot>(AC_REBOOT);
    pTaskManage->bind<Task::System::ResetSimple>(AC_RESET_SIMPLE);
    pTaskManage->bind<Task::System::ResetCompletely>(AC_RESET_COMPLETELY);
    pTaskManage->bind<Task::System::ExportDeviceParam>(AC_EXPORT_DEVICE_PARAM);
    pTaskManage->bind<Task::System::ImportDeviceParam>(AC_IMPORT_DEVICE_PARAM);
    pTaskManage->bind<Task::System::DoUpgrade>(AC_DO_UPGRADE);
    pTaskManage->bind<Task::System::GetUpgradeStatus>(AC_GET_UPGRADE_STATUS);
    pTaskManage->bind<Task::System::SetUpgrade>(AC_SET_UPGRADE);
    pTaskManage->bind<Task::System::CheckUpgrade>(AC_CHECK_UPGRADE);
    pTaskManage->bind<Task::System::SetUpgradeMaintain>(AC_SET_UPGRADE_MAINTAIN);
    pTaskManage->bind<Task::System::GetUpgradeMaintain>(AC_GET_UPGRADE_MAINTAIN);
    pTaskManage->bind<Task::System::FindLog>(AC_FIND_LOG);
    pTaskManage->bind<Task::System::ExportLog>(AC_EXPORT_LOG);
    pTaskManage->bind<Task::System::TestLogServer>(AC_TEST_LOG_SERVER);
    pTaskManage->bind<Task::System::SetLogServer>(AC_SET_LOG_SERVER);
    pTaskManage->bind<Task::System::GetLogServer>(AC_GET_LOG_SERVER);
    pTaskManage->bind<Task::System::GetSecurityServicesInfo>(AC_GET_SECURITY_SERVICES_INFO);
    pTaskManage->bind<Task::System::SetSecurityServicesInfo>(AC_SET_SECURITY_SERVICES_INFO);
    pTaskManage->bind<Task::System::GetSshCountdown>(AC_GET_SSH_COUNTDOWN);
    pTaskManage->bind<Task::System::GetIpFilterInfo>(AC_GET_IP_FILTER_INFO);
    pTaskManage->bind<Task::System::SetIpFilterInfo>(AC_SET_IP_FILTER_INFO);
    pTaskManage->bind<Task::System::AddIpFilterAddress>(AC_ADD_IP_FILTER_ADDRESS);
    pTaskManage->bind<Task::System::RemoveIpFilterAddress>(AC_REMOVE_IP_FILTER_ADDRESS);
    pTaskManage->bind<Task::System::ModifyIpFilterAddress>(AC_MODIFY_IP_FILTER_ADDRESS);
    
    /* 产测相关 */
    pTaskManage->bind<Task::ProductionTest::GetItems>(AC_GET_PRODUCTION_TEST_ITEMS);
    pTaskManage->bind<Task::ProductionTest::GetResult>(AC_GET_PRODUCTION_TEST_RESULT);
    pTaskManage->bind<Task::ProductionTest::SaveResult>(AC_SAVE_PRODUCTION_TEST_RESULT);
    pTaskManage->bind<Task::ProductionTest::UploadResult>(AC_UPLOAD_PRODUCTION_TEST);
    pTaskManage->bind<Task::ProductionTest::ResetResult>(AC_RESET_PRODUCTION_TEST);

    /* 网络配置相关 */
    pTaskManage->bind<Task::Network::GetCheckMacValid>(AC_GET_CHECK_MAC_VALID);
    pTaskManage->bind<Task::Network::GetNetworkInfo>(AC_GET_NETWORK_INFO);
    pTaskManage->bind<Task::Network::SetNetworkInfo>(AC_SET_NETWORK_INFO);
    pTaskManage->bind<Task::Network::GetDdnsInfo>(AC_GET_DDNS_INFO);
    pTaskManage->bind<Task::Network::SetDdnsInfo>(AC_SET_DDNS_INFO);
    pTaskManage->bind<Task::Network::GetPppoeInfo>(AC_GET_PPPOE_INFO);
    pTaskManage->bind<Task::Network::SetPppoeInfo>(AC_SET_PPPOE_INFO);
    pTaskManage->bind<Task::Network::GetPortInfo>(AC_GET_PORT_INFO);
    pTaskManage->bind<Task::Network::SetPortInfo>(AC_SET_PORT_INFO);
    pTaskManage->bind<Task::Network::GetPortMapInfo>(AC_GET_PORT_MAP_INFO);
    pTaskManage->bind<Task::Network::SetPortMapInfo>(AC_SET_PORT_MAP_INFO);
    pTaskManage->bind<Task::Network::GetLogServerInfo>(AC_GET_LOG_SERVER_INFO);
    pTaskManage->bind<Task::Network::SetLogServerInfo>(AC_SET_LOG_SERVER_INFO);
    pTaskManage->bind<Task::Network::GetOhterBaseInfo>(AC_GET_OHTER_BASE_INFO);
    pTaskManage->bind<Task::Network::SetOhterBaseInfo>(AC_SET_OHTER_BASE_INFO);

    pTaskManage->bind<Task::Network::GetSnmpInfo>(AC_GET_SNMP_INFO);
    pTaskManage->bind<Task::Network::SetSnmpInfo>(AC_SET_SNMP_INFO);
    pTaskManage->bind<Task::Network::GetEamilInfo>(AC_GET_EAMIL_INFO);
    pTaskManage->bind<Task::Network::SetEamilInfo>(AC_SET_EAMIL_INFO);
    pTaskManage->bind<Task::Network::GetGb28181Info>(AC_GET_GB28181_INFO);
    pTaskManage->bind<Task::Network::SetGb28181Info>(AC_SET_GB28181_INFO);
    // pTaskManage->bind<Task::Network::GetOhterSeniorInfo>(AC_GET_OHTER_SENIOR_INFO);
    pTaskManage->bind<Task::Network::SetOhterSeniorInfo>(AC_SET_OHTER_SENIOR_INFO);
    pTaskManage->bind<Task::Network::GetIntegrationProtoInfo>(AC_GET_INTEGRATION_PROTO_INFO);
    pTaskManage->bind<Task::Network::SetIntegrationProtoInfo>(AC_SET_INTEGRATION_PROTO_INFO);
    pTaskManage->bind<Task::Network::TestEamil>(AC_TEST_EAMIL);
    pTaskManage->bind<Task::Network::EventEamil>(AC_EVENT_EAMIL);
    pTaskManage->bind<Task::Network::GetTrustCertInfo>(AC_GET_TRUST_CA_INFO);
    pTaskManage->bind<Task::Network::InstallTrustCert>(AC_INSTALL_TRUST_CA_FILE);
    pTaskManage->bind<Task::Network::DeleteTrustCert>(AC_DELETE_TRUST_CA_FILE);
    pTaskManage->bind<Task::Network::DownloadTrustCert>(AC_DOWNLOAD_TRUST_CA_FILE);
    pTaskManage->bind<Task::Network::GetDeviceCertInfo>(AC_GET_DEVICE_CA_INFO);
    pTaskManage->bind<Task::Network::InstallDeviceCert>(AC_INSTALL_DEVICE_CA_FILE);
    pTaskManage->bind<Task::Network::DeleteDeviceCert>(AC_DELETE_DEVICE_CA_FILE);
    pTaskManage->bind<Task::Network::DownloadDeviceCert>(AC_DOWNLOAD_DEVICE_CA_FILE);
    pTaskManage->bind<Task::Network::CreateRequestCert>(AC_CREATE_REQUEST_CA_FILE);
    pTaskManage->bind<Task::Network::CreateAndInstallDeviceCert>(AC_CREATE_INSTALL_DEVICE_CA_FILE);
    pTaskManage->bind<Task::Network::DeleteRuquestCsr>(AC_DELETE_REQUEST_CSR_FILE);
    pTaskManage->bind<Task::Network::GetHttpsInfo>(AC_GET_HTTPS_INFO);
    pTaskManage->bind<Task::Network::ConfigHttpsInfo>(AC_CONFIG_HTTPS_INFO);
    pTaskManage->bind<Task::Network::GetAuthMethod>(AC_GET_AUTH_METHOD);
    pTaskManage->bind<Task::Network::SetAuthMethod>(AC_SET_AUTH_METHOD);
    /* onvif */
    pTaskManage->bind<Task::Network::GetOnvifConfigInfo>(AC_GET_ONVIF_INFO);
    pTaskManage->bind<Task::Network::SetOnvifConfigInfo>(AC_SET_ONVIF_INFO);
    /* qos */
    pTaskManage->bind<Task::Network::GetQosInfo>(AC_GET_QOS_INFO);
    pTaskManage->bind<Task::Network::SetQosInfo>(AC_SET_QOS_INFO);
    pTaskManage->bind<Task::Network::GetBonjourInfo>(AC_GET_BONJOUR_INFO);
    pTaskManage->bind<Task::Network::SetBonjourInfo>(AC_SET_BONJOUR_INFO);

    /* 国标证书管理 */
    pTaskManage->bind<Task::Network::GmCreateCertRequestFile>(AC_GM_CREATE_CERT_REQUEST_FILE);
    pTaskManage->bind<Task::Network::GmUploadCaCert>(AC_GM_UPLOAD_CA_CERT);
    pTaskManage->bind<Task::Network::GmUploadPlatformCert>(AC_GM_UPLOAD_PLATFORM_CERT);
    pTaskManage->bind<Task::Network::GmUploadDeviceCert>(AC_GM_UPLOAD_DEVICE_CERT);
    pTaskManage->bind<Task::Network::GmUploadCrlFile>(AC_GM_UPLOAD_CRL_FILE);
    pTaskManage->bind<Task::Network::GmGetCertInfo>(AC_GM_GET_CERT_INFO);
    pTaskManage->bind<Task::Network::GmDeleteCertFile>(AC_GM_DELETE_CERT_FILE);

    #if CAP_NETWORK_WIFI
    /*WIFI */
    pTaskManage->bind<Task::Network::SetWifiStaInfo>(AC_SET_CONFIG_WIFI_STA);
    pTaskManage->bind<Task::Network::GetWifiStaInfo>(AC_GET_CONFIG_WIFI_STA);
    pTaskManage->bind<Task::Network::ConnectWifiSta>(AC_CONNECT_WIFI_STA);
    pTaskManage->bind<Task::Network::DisconnectWifiSta>(AC_DISCONNECT_WIFI_STA);
    #endif

    #if CAP_NETWORK_4G
    /*4G */
    pTaskManage->bind<Task::Network::Get4GInfo>(AC_GET_4G_INFO);
    pTaskManage->bind<Task::Network::Set4GInfo>(AC_SET_4G_INFO);
    #endif

    #if CAP_NETWORK_WIFI
    /*热点 */
    pTaskManage->bind<Task::Network::SetHotspot>(AC_SET_HOTSPOT_INFO);
    pTaskManage->bind<Task::Network::GetHotspotConn>(AC_GET_HOTSPOT_CONN);
    pTaskManage->bind<Task::Network::GetHotspot>(AC_GET_HOTSPOT_INFO);
    #endif

    #if CAP_GARBAGE_STATION_PLATFORM
    /* 平台管理 */
    pTaskManage->bind<Task::Network::ConnPlatform>(AC_PLATFORM_CONN);
    pTaskManage->bind<Task::Network::storePlatformDevices>(AC_PLATFORM_STOREDEVICE);
    pTaskManage->bind<Task::Network::GetConnPlatformInfo>(AC_PLATFORM_GETINFO);
    #endif

    #ifdef ENABLE_GAT1400_SRC
    /* GAT1400相关 */
    pTaskManage->bind<Task::Network::GetGat1400Info>(AC_GET_GAT1400_INFO);
    pTaskManage->bind<Task::Network::SetGat1400Info>(AC_SET_GAT1400_INFO);
#endif

    /* 图像相关 */
    pTaskManage->bind<Task::Pic::GetDayNight>(AC_GET_DAY_NIGHT_INFO);
    pTaskManage->bind<Task::Pic::SetDayNight>(AC_SET_DAY_NIGHT_INFO);
    pTaskManage->bind<Task::Pic::GetImageParam>(AC_GET_VIDEO_EFFECT_INFO);
    pTaskManage->bind<Task::Pic::SetImageParam>(AC_SET_VIDEO_EFFECT_INFO);
    pTaskManage->bind<Task::Pic::GetExposureParam>(AC_GET_EXPOSURE_INFO);
    pTaskManage->bind<Task::Pic::SetExposureParam>(AC_SET_EXPOSURE_INFO);
    pTaskManage->bind<Task::Pic::GetBackLightParam>(AC_GET_BACK_LIGHT_INFO);
    pTaskManage->bind<Task::Pic::SetBackLightParam>(AC_SET_BACK_LIGHT_INFO);
    pTaskManage->bind<Task::Pic::GetAwbParam>(AC_GET_WHITE_BALANCE_INFO);
    pTaskManage->bind<Task::Pic::SetAwbParam>(AC_SET_WHITE_BALANCE_INFO);
    pTaskManage->bind<Task::Pic::GetDnrParam>(AC_GET_NOISE_REMOVE_INFO);
    pTaskManage->bind<Task::Pic::SetDnrParam>(AC_SET_NOISE_REMOVE_INFO);
    pTaskManage->bind<Task::Pic::GetVideoMirrorParam>(AC_GET_VIDEO_MIRROR_INFO);
    pTaskManage->bind<Task::Pic::SetVideoMirrorParam>(AC_SET_VIDEO_MIRROR_INFO);
    pTaskManage->bind<Task::Pic::GetOsdConfigParam>(AC_GET_OSD_CONFIG);
    pTaskManage->bind<Task::Pic::SetOsdConfigParam>(AC_SET_OSD_CONFIG);
    pTaskManage->bind<Task::Pic::GetCoverConfigParam>(AC_GET_COVER_CONFIG);
    pTaskManage->bind<Task::Pic::SetCoverConfigParam>(AC_SET_COVER_CONFIG);
    pTaskManage->bind<Task::Pic::GetCoverConfigParam>(AC_GET_SHELTER_INFO);
    pTaskManage->bind<Task::Pic::SetCoverConfigParam>(AC_SET_SHELTER_INFO);
    pTaskManage->bind<Task::Pic::GetSchedule>(AC_GET_IMAGE_SCHEDULE_INFO);
    pTaskManage->bind<Task::Pic::SetSchedule>(AC_SET_IMAGE_SCHEDULE_INFO);
    pTaskManage->bind<Task::Pic::GetScene>(AC_GET_VIDEO_SCENE_INFO);
    pTaskManage->bind<Task::Pic::SetScene>(AC_SET_VIDEO_SCENE_INFO);
    pTaskManage->bind<Task::Pic::SetDefault>(AC_SET_PIC_DEFAULT_INFO);

    /*视音频配置*/
    pTaskManage->bind<Task::AV::GetVideoConfig>(AC_GET_VIDEO_CONFIG);
    pTaskManage->bind<Task::AV::SetVideoConfig>(AC_SET_VIDEO_CONFIG);
    pTaskManage->bind<Task::AV::GetVideoCapabilitySet>(AC_GET_VIDEO_CAPABILITY_SET);
    pTaskManage->bind<Task::AV::GetAudioConfig>(AC_GET_AUDIO_CONFIG);
    pTaskManage->bind<Task::AV::GetAudioCapabilitySet>(AC_GET_AUDIO_CAPABILITY_SET);
    pTaskManage->bind<Task::AV::SetAudioConfig>(AC_SET_AUDIO_CONFIG);
    pTaskManage->bind<Task::AV::GetVideoRoiConfig>(AC_GET_VIDEO_ROI_CONFIG);
    pTaskManage->bind<Task::AV::SetVideoRoiConfig>(AC_SET_VIDEO_ROI_CONFIG);
    pTaskManage->bind<Task::AV::GetAreaCropConfig>(AC_GET_VIDEO_AREA_CROP_CONFIG);
    pTaskManage->bind<Task::AV::SetAreaCropConfig>(AC_SET_VIDEO_AREA_CROP_CONFIG);
    pTaskManage->bind<Task::AV::GetAreaCropConversionResolution>(AC_GET_VIDEO_AREA_CROP_CONVERSION_RESOLUTION);

    /* 事件相关 */
    /**
     * @brief   : 普通事件
     */
    /*移动侦测*/
    pTaskManage->bind<Task::Event::GetMotionDetectionInfo>(AC_GET_MOTION_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetMotionDetectionInfo>(AC_SET_MOTION_DETECT_INFO);
    /*遮挡报警*/
    pTaskManage->bind<Task::Event::GetHideAlarmInfo>(AG_GET_HIDE_ALARM_INFO);
    pTaskManage->bind<Task::Event::SetHideAlarmInfo>(AG_SET_HIDE_ALARM_INFO);
    /*异常报警*/
    pTaskManage->bind<Task::Event::GetAbnormalAlarmInfo>(AC_GET_ANOMALY_ALARM_INFO);
    pTaskManage->bind<Task::Event::SetAbnormalAlarmInfo>(AC_SET_ANOMALY_ALARM_INFO);
    /*声音报警*/
    pTaskManage->bind<Task::Event::GetAudioAlarmInfo>(AC_GET_AUDIBLE_ALARM_INFO);
    pTaskManage->bind<Task::Event::SetAudioAlarmInfo>(AC_SET_AUDIBLE_ALARM_INFO);
    pTaskManage->bind<Task::Event::EditAudioAlarmCustomInfo>(AC_EDIT_AUDIO_CUSTOM_INFO);
    pTaskManage->bind<Task::Event::GetAudioAlarmCustomInfo>(AC_GET_AUDIO_CUSTOM_INFO);
    pTaskManage->bind<Task::Event::SetAudioAlarmCustomInfo>(AC_SET_AUDIO_CUSTOM_INFO);
    /*报警输入*/
    pTaskManage->bind<Task::Event::GetIoInputInfo>(AC_GET_ALARM_INPUT_INFO);
    pTaskManage->bind<Task::Event::SetIoInputInfo>(AC_SET_ALARM_INPUT_INFO);
    /*报警输出*/
    pTaskManage->bind<Task::Event::GetIoOutputInfo>(AC_GET_ALARM_OUTPUT_INFO);
    pTaskManage->bind<Task::Event::SetIoOutputInfo>(AC_SET_ALARM_OUTPUT_INFO);
    /*闪光报警*/
    pTaskManage->bind<Task::Event::GetFlashAlarmInfo>(AC_GET_FLASHING_LIGHT_ALARM_INFO);
    pTaskManage->bind<Task::Event::SetFlashAlarmInfo>(AC_SET_FLASHING_LIGHT_ALARM_INFO);
    /*手动声光报警联动*/
    pTaskManage->bind<Task::Event::TriggerSoundLightAlarm>(AC_TRIGGER_SOUND_LIGHT_ALARM);
    /*PIR报警*/
    pTaskManage->bind<Task::Event::GetPirAlarmInfo>(AC_GET_PIR_ALARM_INFO);
    pTaskManage->bind<Task::Event::SetPirAlarmInfo>(AC_SET_PIR_ALARM_INFO);

    /**
     * @brief   : 周界事件
     */

    /* 越界侦测 */
    pTaskManage->bind<Task::Event::GetBoundaryDetectionInfo>(AC_GET_LINE_CROSSING_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetBoundaryDetectionInfo>(AC_SET_LINE_CROSSING_DETECT_INFO);
    /* 区域入侵侦测 */
    pTaskManage->bind<Task::Event::GetFieldDetectionInfo>(AC_GET_REGIONAL_INTRUSION_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetFieldDetectionInfo>(AC_SET_REGIONAL_INTRUSION_DETECT_INFO);
    /* 进入区域侦测 */
    pTaskManage->bind<Task::Event::GetEnterRegionDetectInfo>(AC_GET_ENTER_REGION_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetEnterRegionDetectInfo>(AC_SET_ENTER_REGION_DETECT_INFO);
    /* 离开区域侦测 */
    pTaskManage->bind<Task::Event::GetLeaveRegionDetectInfo>(AC_GET_LEAVE_REGION_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetLeaveRegionDetectInfo>(AC_SET_LEAVE_REGION_DETECT_INFO);

    /**
     * @brief   : smart事件
     */
    /* 音频异常侦测 */
    pTaskManage->bind<Task::Event::GetAudioAnomalyInfo>(AC_GET_AUDIO_ANOMALY_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetAudioAnomalyInfo>(AC_SET_AUDIO_ANOMALY_DETECT_INFO);
    pTaskManage->bind<Task::Event::GetAudioAnomalyCurrentDb>(AC_GET_AUDIO_ANOMALY_DETECT_CURRENT_DB);
    /* 场景变更侦测 */
    pTaskManage->bind<Task::Event::GetSceneChangeInfo>(AC_GET_SCENE_CHANGE_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetSceneChangeInfo>(AC_SET_SCENE_CHANGE_DETECT_INFO);
    /* 人脸侦测 */
    pTaskManage->bind<Task::Event::GetFaceDetectionInfo>(AC_GET_FACE_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetFaceDetectionInfo>(AC_SET_FACE_DETECT_INFO);
    /* 徘徊侦测 */
    pTaskManage->bind<Task::Event::GetLoiteringDetectionInfo>(AC_GET_LOITERING_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetLoiteringDetectionInfo>(AC_SET_LOITERING_DETECT_INFO);
    /* 人员聚集侦测 */
    pTaskManage->bind<Task::Event::GetCrowdGatheringInfo>(AC_GET_CROWD_GATHERING_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetCrowdGatheringInfo>(AC_SET_CROWD_GATHERING_DETECT_INFO);
    /* 停车侦测 */
    pTaskManage->bind<Task::Event::GetParkDetectionInfo>(AC_GET_PARKING_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetParkDetectionInfo>(AC_SET_PARKING_DETECT_INFO);
    /* 物品遗留侦测 */
    pTaskManage->bind<Task::Event::GetUnattendedObjectInfo>(AC_GET_UNATTENDED_OBJECT_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetUnattendedObjectInfo>(AC_SET_UNATTENDED_OBJECT_DETECT_INFO);
    /* 物品拿取侦测 */
    pTaskManage->bind<Task::Event::GetObjectRemovalInfo>(AC_GET_OBJECT_REMOVAL_DETECT_INFO);
    pTaskManage->bind<Task::Event::SetObjectRemovalInfo>(AC_SET_OBJECT_REMOVAL_DETECT_INFO);
    /* 宠物识别 */
    pTaskManage->bind<Task::Event::GetPetRecognitionInfo>(AC_GET_PET_RECOGNITION_INFO);
    pTaskManage->bind<Task::Event::SetPetRecognitionInfo>(AC_SET_PET_RECOGNITION_INFO);
    /* 人脸抓拍 */
    pTaskManage->bind<Task::Event::GetFaceCaptureInfo>(AC_GET_FACE_CAPTURE_INFO);
    pTaskManage->bind<Task::Event::SetFaceCaptureInfo>(AC_SET_FACE_CAPTURE_INFO);
#if CAP_AI_FACE_COMPARE
    /*人脸比对 */
    pTaskManage->bind<Task::Event::SetFaceCompareInfo>(AC_SET_FACE_COMPARE_INFO);
    pTaskManage->bind<Task::Event::GetFaceCompareInfo>(AC_GET_FACE_COMPARE_INFO);
#endif
    /* 人脸抓拍叠加信息 */
    pTaskManage->bind<Task::Event::GetFaceCaptureOverlayInfo>(AC_GET_FACE_CAPTURE_OVERLAY_INFO_INFO);
    pTaskManage->bind<Task::Event::SetFaceCaptureOverlayInfo>(AC_SET_FACE_CAPTURE_OVERLAY_INFO_INFO);
    
#if CAP_AI_FACE_COMPARE
    pTaskManage->bind<Task::Event::AddTargetLib>(AC_ADD_TARGET_LIB);
    pTaskManage->bind<Task::Event::DelTargetLib>(AC_DEL_TARGET_LIB);
    pTaskManage->bind<Task::Event::SetTargetLib>(AC_SET_TARGET_LIB);
    pTaskManage->bind<Task::Event::GetTargetLib>(AC_GET_TARGET_LIB);
    pTaskManage->bind<Task::Event::AddFaceInfo>(AC_ADD_FACE_INFO);
    pTaskManage->bind<Task::Event::DelFaceInfo>(AC_DEL_FACE_INFO);
    pTaskManage->bind<Task::Event::SetFaceInfo>(AC_SET_FACE_INFO);
    pTaskManage->bind<Task::Event::GetFaceInfo>(AC_GET_FACE_INFO);
    pTaskManage->bind<Task::Event::DelFaceFile>(AC_DEL_FACE_FILE);
#endif

#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
    * @brief   : 场景智能分析
    */
    /* 画面分析 */
    pTaskManage->bind<Task::Event::GetImageAnalysisInfo>(AC_GET_IMAGE_ANALYSIS_INFO);
    pTaskManage->bind<Task::Event::SetImageAnalysisInfo>(AC_SET_IMAGE_ANALYSIS_INFO);
    /* 文字预设任务 */
    pTaskManage->bind<Task::Event::GetTextPresetTaskInfo>(AC_GET_TEXT_PRESET_TASK_INFO);
    pTaskManage->bind<Task::Event::SetTextPresetTaskInfo>(AC_SET_TEXT_PRESET_TASK_INFO);
    /* 实时预警推送 */
    pTaskManage->bind<Task::Event::GetRealAlarmPushInfo>(AC_GET_REAL_ALARM_PUSH_INFO);
    pTaskManage->bind<Task::Event::GetRealAlarmProcessInfo>(AC_GET_REAL_ALARM_PROCESS_INFO);
    pTaskManage->bind<Task::Event::SetRealAlarmPushInfo>(AC_SET_REAL_ALARM_PUSH_INFO);
    /* 画面分析记录 */
    pTaskManage->bind<Task::Event::OperateImageAnalysisRecord>(AC_OPERATE_IMAGE_ANALYSIS_RECORD);
    /* 画面分析结果返回 */
    pTaskManage->bind<Task::Event::RtImageAnalysisInfoResult>(AC_RETURN_IMAGE_ANALYSIS_RESULT);
    /*推理分析中断停止*/
    pTaskManage->bind<Task::Event::CtrlImageAnalysisStop>(AC_SET_IMAGE_ANALYSIS_STOP);
#endif

#ifdef SCENE_INTELLIGENCE
    /**
     * @brief   : 场景智能
     */

    /* 属性检测开关信息 */
    pTaskManage->bind<Task::Event::SetAttributeInfo>(AC_SET_ATTRIBUTE_DETECT_INFO);
    pTaskManage->bind<Task::Event::GetAttributeInfo>(AC_GET_ATTRIBUTE_DETECT_INFO);

    /* 推送人脸抓拍信息 */
    pTaskManage->bind<Task::Event::PushFaceCaptureInfo>(AC_PUSH_FACE_CAPTURE_INFO);
    /* 设置推送人脸抓拍信息 */
    pTaskManage->bind<Task::Event::SetPushFaceCaptureInfo>(AC_SET_PUSH_FACE_CAPTURE_INFO);

    /* 推送行人抓拍信息 */
    pTaskManage->bind<Task::Event::PushPersonCaptureInfo>(AC_PUSH_PERSON_CAPTURE_INFO);
    /* 推送机动车抓拍信息 */
    pTaskManage->bind<Task::Event::PushMotorVehicleCaptureInfo>(AC_PUSH_MOTORVEHICLE_CAPTURE_INFO);
    /* 推送非机动车抓拍信息 */
    pTaskManage->bind<Task::Event::PushNonMotorVehicleCaptureInfo>(AC_PUSH_NONMOTORVEHICLE_CAPTURE_INFO);

    pTaskManage->bind<Task::Event::GetFenceClimbingInfo>(AC_GET_CLIMB_FENCE_INFO);
    pTaskManage->bind<Task::Event::SetFenceClimbingInfo>(AC_SET_CLIMB_FENCE_INFO);

    pTaskManage->bind<Task::Event::GetLeavePostInfo>(AC_GET_DIMISSION_INFO);
    pTaskManage->bind<Task::Event::SetLeavePostInfo>(AC_SET_DIMISSION_INFO);

    pTaskManage->bind<Task::Event::GetIllegalLaneChangeInfo>(AC_GET_ILLEGAL_LANE_INFO);
    pTaskManage->bind<Task::Event::SetIllegalLaneChangeInfo>(AC_SET_ILLEGAL_LANE_INFO);

    pTaskManage->bind<Task::Event::GetReverseDirectionInfo>(AC_GET_RETROGRADE_INFO);
    pTaskManage->bind<Task::Event::SetReverseDirectionInfo>(AC_SET_RETROGRADE_INFO);

    pTaskManage->bind<Task::Event::GetNonMotorVehicleIntrusionInfo>(AC_GET_NONMOROT_VEHIINTRU_INFO);
    pTaskManage->bind<Task::Event::SetNonMotorVehicleIntrusionInfo>(AC_SET_NONMOROT_VEHIINTRU_INFO);

    pTaskManage->bind<Task::Event::GetEmergencyLaneOccupancyInfo>(AC_GET_OCCUPATION_EMERGENCY_INFO);
    pTaskManage->bind<Task::Event::SetEmergencyLaneOccupancyInfo>(AC_SET_OCCUPATION_EMERGENCY_INFO);

    pTaskManage->bind<Task::Event::GetPedestrianIntrusionInfo>(AC_GET_PEDESTRAN_INTRUSION_INFO);
    pTaskManage->bind<Task::Event::SetPedestrianIntrusionInfo>(AC_SET_PEDESTRAN_INTRUSION_INFO);

    pTaskManage->bind<Task::Event::GetSmokeFireInfo>(AC_GET_SMOKE_FIRE_CFG);
    pTaskManage->bind<Task::Event::SetSmokeFireInfo>(AC_SET_SMOKE_FIRE_CFG);

    pTaskManage->bind<Task::Event::GetRoadPondingInfo>(AC_GET_ROAD_PONDING_CFG);
    pTaskManage->bind<Task::Event::SetRoadPondingInfo>(AC_SET_ROAD_PONDING_CFG);

    pTaskManage->bind<Task::Event::GetManholeCoverAbnormalInfo>(AC_GET_MANHOLE_COVER_ABNORMAL_CFG);
    pTaskManage->bind<Task::Event::SetManholeCoverAbnormalInfo>(AC_SET_MANHOLE_COVER_ABNORMAL_CFG);

    pTaskManage->bind<Task::Event::GetSleepOnDutyInfo>(AC_GET_SLEEP_ON_DUTY_CFG);
    pTaskManage->bind<Task::Event::SetSleepOnDutyInfo>(AC_SET_SLEEP_ON_DUTY_CFG);

    pTaskManage->bind<Task::Event::GetElectricVehicleInElevatorInfo>(AC_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG);
    pTaskManage->bind<Task::Event::SetElectricVehicleInElevatorInfo>(AC_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG);

    pTaskManage->bind<Task::Event::GetPersonFallDownInfo>(AC_GET_PERSON_FALL_DOWN_CFG);
    pTaskManage->bind<Task::Event::SetPersonFallDownInfo>(AC_SET_PERSON_FALL_DOWN_CFG);

    pTaskManage->bind<Task::Event::GetConstructionOccupyRoadInfo>(AC_GET_CONSTRUCTION_OCCUPY_ROAD_CFG);
    pTaskManage->bind<Task::Event::SetConstructionOccupyRoadInfo>(AC_SET_CONSTRUCTION_OCCUPY_ROAD_CFG);

    pTaskManage->bind<Task::Event::GetCongestionInfo>(AC_GET_CONGESTION_CFG);
    pTaskManage->bind<Task::Event::SetCongestionInfo>(AC_SET_CONGESTION_CFG);

    pTaskManage->bind<Task::Event::GetLicensePlateRecognitionInfo>(AC_GET_LICENSE_PLATE_RECOGNITION_CFG);
    pTaskManage->bind<Task::Event::SetLicensePlateRecognitionInfo>(AC_SET_LICENSE_PLATE_RECOGNITION_CFG);
    
    pTaskManage->bind<Task::Event::GetHighAltitudeSeatbeltInfo>(AC_GET_HIGH_ALTITUDE_SEATBELT_CFG);
    pTaskManage->bind<Task::Event::SetHighAltitudeSeatbeltInfo>(AC_SET_HIGH_ALTITUDE_SEATBELT_CFG);
    
    pTaskManage->bind<Task::Event::GetSafetyHelmetInfo>(AC_GET_SAFETY_HELMET_CFG);
    pTaskManage->bind<Task::Event::SetSafetyHelmetInfo>(AC_SET_SAFETY_HELMET_CFG);

    pTaskManage->bind<Task::Event::GetPersonFallInfo>(AC_GET_PERSON_FALL_CFG);
    pTaskManage->bind<Task::Event::SetPersonFallInfo>(AC_SET_PERSON_FALL_CFG);

    pTaskManage->bind<Task::Event::GetPhoneUsageInfo>(AC_GET_PHONE_USAGE_CFG);
    pTaskManage->bind<Task::Event::SetPhoneUsageInfo>(AC_SET_PHONE_USAGE_CFG);

    pTaskManage->bind<Task::Event::GetSmokingInfo>(AC_GET_SMOKING_CFG);
    pTaskManage->bind<Task::Event::SetSmokingInfo>(AC_SET_SMOKING_CFG);

    pTaskManage->bind<Task::Event::GetOpenFlameInfo>(AC_GET_OPEN_FLAME_CFG);
    pTaskManage->bind<Task::Event::SetOpenFlameInfo>(AC_SET_OPEN_FLAME_CFG);

    pTaskManage->bind<Task::Event::GetBareSoilInfo>(AC_GET_BARE_SOIL_CFG);
    pTaskManage->bind<Task::Event::SetBareSoilInfo>(AC_SET_BARE_SOIL_CFG);

    pTaskManage->bind<Task::Event::GetHoleProtectionBarInfo>(AC_GET_HOLE_PROTECTION_BAR_CFG);
    pTaskManage->bind<Task::Event::SetHoleProtectionBarInfo>(AC_SET_HOLE_PROTECTION_BAR_CFG);

    pTaskManage->bind<Task::Event::GetReflectiveClothingInfo>(AC_GET_REFLECTIVE_CLOTHING_CFG);
    pTaskManage->bind<Task::Event::SetReflectiveClothingInfo>(AC_SET_REFLECTIVE_CLOTHING_CFG);
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    /* 垃圾暴露识别 */
    pTaskManage->bind<Task::Event::GetGarbageExposureInfo>(AC_GET_GARBAGE_EXPOSURE_CFG);
    pTaskManage->bind<Task::Event::SetGarbageExposureInfo>(AC_SET_GARBAGE_EXPOSURE_CFG);
    /* 垃圾满溢识别 */
    pTaskManage->bind<Task::Event::GetGarbageOverflowInfo>(AC_GET_GARBAGE_OVERFLOW_CFG);
    pTaskManage->bind<Task::Event::SetGarbageOverflowInfo>(AC_SET_GARBAGE_OVERFLOW_CFG);
#endif

#if CAP_AI_PEOPLE_STATISTICS
    /* 人流统计 */
    pTaskManage->bind<Task::Event::GetPeopleFlowStatisticsInfo>(AC_GET_PEOPLE_FLOW_STATISTICS_INFO);
    pTaskManage->bind<Task::Event::SetPeopleFlowStatisticsInfo>(AC_SET_PEOPLE_FLOW_STATISTICS_INFO);
    pTaskManage->bind<Task::Event::ClearPeopleFlowStatisticsResult>(AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT);
    /* 人员密度检测 */
    pTaskManage->bind<Task::Event::GetPeopleDensityDetectionInfo>(AC_GET_PEOPLE_DENSITY_DETECTION_INFO);
    pTaskManage->bind<Task::Event::SetPeopleDensityDetectionInfo>(AC_SET_PEOPLE_DENSITY_DETECTION_INFO);
#endif

    /* 数据检索相关 */
    pTaskManage->bind<Task::Retrieval::SearchByRecordType>(AC_SEARCH_BY_RECORD_TYPE);
    pTaskManage->bind<Task::Retrieval::SearchByImageType>(AC_SEARCH_BY_IMAGE); 
    pTaskManage->bind<Task::Retrieval::SearchByRecordTS>(AC_SEARCH_BY_RECORD_TS_TYPE);

    /* 图片下载 */
    pTaskManage->bind<Task::Retrieval::DownloadImageFileInfo>(AC_DOWNLOAD_IMAGE_FILE_INFO);

    /* 回放相关 */
    pTaskManage->bind<Task::Replay::SetLayoutInfo>(AC_SET_REPLAY_LAYOUT_INFO);
    pTaskManage->bind<Task::Replay::SetLayoutInfo>(AC_GET_VIDEO_TIME); /* 跟设置布局用同一个接口 */


    /* 录制相关 */
    pTaskManage->bind<Task::Record::CtrlRecordInfo>(AC_CONTROL_RECORD_INFO);
    pTaskManage->bind<Task::Record::NoticeRecordFileInfo>(AC_NOTICE_RECORD_FILE_INFO);
    pTaskManage->bind<Task::Record::DelRecordFileInfo>(AC_DEL_RECORD_FILE_INFO);
    pTaskManage->bind<Task::Record::SetRecordFileInfo>(AC_SET_RECORD_FILE_INFO);
    pTaskManage->bind<Task::Record::FindRecordFileInfo>(AC_FIND_RECORD_FILE_INFO);
    pTaskManage->bind<Task::Record::NoticeRecordTsFileInfo>(AC_NOTICE_RECORD_TS_FILE_INFO);
    pTaskManage->bind<Task::Record::NoticeRecordException>(AC_NOTICE_RECORD_EXCEPTION);
    pTaskManage->bind<Task::Record::GetHumanRecord>(AC_GET_HUMAN_RECORD);
    pTaskManage->bind<Task::Record::SetHumanRecord>(AC_SET_HUMAN_RECORD);
    pTaskManage->bind<Task::Record::DownloadRecordFile>(AC_DOWNLOAD_RECORD_FILE);
    pTaskManage->bind<Task::Record::NoticeDownloadRecordProgress>(AC_NOTICE_DOWNLOAD_RECORD_PROGRESS);
    pTaskManage->bind<Task::Record::GetAdvancedParam>(AC_GET_RECORD_ADVANCED_PARAM);
    pTaskManage->bind<Task::Record::SetAdvancedParam>(AC_SET_RECORD_ADVANCED_PARAM);
    pTaskManage->bind<Task::Record::GetSchedule>(AC_GET_RECORD_SCHEDULE);
    pTaskManage->bind<Task::Record::SetSchedule>(AC_SET_RECORD_SCHEDULE);
    // pTaskManage->bind<Task::Record::GetHolidayInfo>(AC_GET_HOLIDAY_INFO);
    // pTaskManage->bind<Task::Record::SetHolidayInfo>(AC_SET_HOLIDAY_INFO);
    // pTaskManage->bind<Task::Record::GetRecordOtherInfo>(AC_GET_RECORD_OTHER_INFO);
    // pTaskManage->bind<Task::Record::SetRecordOtherInfo>(AC_SET_RECORD_OTHER_INFO);
    pTaskManage->bind<Task::Record::GetRecordStatusInfo>(AC_GET_RECORD_STATUS);

    /* 抓图相关 */
    pTaskManage->bind<Task::Capture::GetCapturePlanInfo>(AC_GET_CAPTURE_PLAN_INFO);
    pTaskManage->bind<Task::Capture::SetCapturePlanInfo>(AC_SET_CAPTURE_PLAN_INFO);
    pTaskManage->bind<Task::Capture::GetCaptureParamInfo>(AC_GET_CAPTURE_PARAM_INFO);
    pTaskManage->bind<Task::Capture::SetCaptureParamInfo>(AC_SET_CAPTURE_PARAM_INFO);

    /* 存储管理相关 */
    pTaskManage->bind<Task::StorageManage::GetStorageManageInfo>(AC_GET_STORAGE_MANAGE_INFO);
    pTaskManage->bind<Task::StorageManage::SetStorageManageInfo>(AC_SET_STORAGE_MANAGE_INFO);
    pTaskManage->bind<Task::StorageManage::FormatSdCard>(AC_INIT_SD_CARD);


    /* 预览配置相关 */
    pTaskManage->bind<Task::Preview::GetPreviewInfo>(AC_GET_PREVIEW_INFO);
    pTaskManage->bind<Task::Preview::SetPreviewInfo>(AC_SET_PREVIEW_INFO);
    pTaskManage->bind<Task::Preview::GetCollectAudioInfo>(AC_GET_COLLECT_AUDIO_INFO);
    pTaskManage->bind<Task::Preview::SetIntercomInfo>(AC_SET_INTERCOM_INFO);
    pTaskManage->bind<Task::Preview::SetBroadcastInfo>(AC_SET_BROADCAST_INFO);
    pTaskManage->bind<Task::Preview::SetBeepAlarm>(AC_SET_BEEP_ALARM);
    pTaskManage->bind<Task::Preview::GetIntercomAndBroadcastStatus>(AC_GET_INTERCOM_AND_BROADCAST_STATUS);
    pTaskManage->bind<Task::Preview::DeviceControl>(AC_DEVICE_CONTROL);

    /* 设备激活相关 */
    pTaskManage->bind<Task::Register::GetReisterInfo>(AC_GET_REGISTER_INFO);
    pTaskManage->bind<Task::Register::SetRegisterEg>(AC_SET_REGISTRATION_CODE);
    pTaskManage->bind<Task::Register::SetActivationPasswd>(AC_SET_ACTIVATIONPWD);
    pTaskManage->bind<Task::Register::GetTimeInfo>(AC_REGISTER_GET_TIMEINFO);
    pTaskManage->bind<Task::Register::ManaualConfigTime>(AC_MANUAL_TIMECONFIG);
    pTaskManage->bind<Task::Register::ManaualConfigNetWork>(AC_MANUAL_NETWORKCONFIG);
    pTaskManage->bind<Task::Register::GetActivationInfo>(AC_GET_ACTIVATIONINFO);
    pTaskManage->bind<Task::Register::AutoConfigNetwork>(AC_AUTO_CONFIG_NETWORK);

#ifdef ENABLE_AI_STUDENT
    pTaskManage->bind<Task::AI_STUDENT::GetClassInfo>(AC_GET_CLASS_INFO);
    pTaskManage->bind<Task::AI_STUDENT::SetClassInfo>(AC_SET_CLASS_INFO);
    pTaskManage->bind<Task::AI_STUDENT::GetAttendanceRecordInfo>(AC_GET_ATTENDANCE_INFO);
    pTaskManage->bind<Task::AI_STUDENT::GetStudentBehaviorInfo>(AC_GET_STUDENT_BEHAVIOR_INFO);
    pTaskManage->bind<Task::AI_STUDENT::GetStudentFerformanceInfo>(AC_GET_STUDENT_PERFORMANCE_INFO);
#endif

    TaskPublish::instance()->set_manage(pTaskManage);
}
