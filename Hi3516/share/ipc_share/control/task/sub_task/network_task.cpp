/**
 * @file network_task.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-14
 * 
 * @brief 
 */
#include "network_task.h"
#include "network_convert.h"
#include "system_convert.h"
#include "convert_interface.h"
#include "dlog.h"
#include "upnp_manage.h"
#include "email_manage.h"
#include "network_manage.h"
#include "snmp_manage.h"
#include "ca_manage.h"
#include "https_manage.h"
#include "ddns_manage.h"
#include "pppoe_manage.h"
#include "qos_manage.h"
#include "wifi_manage.h" 
#include "4g_manage.h"
#include "hostapd_manager.h"
// #include "SipModule.h"
// #include "SipType.h"
#include "path_define.h"
#include "network_define.h"
#include "burn_mac_udp_server.h"
#include "ipc_multicast_server.h"
#include "gb28181.hpp"
#include "rtsp_server.h"
#include "web_server.h"
#include "log_handler.h"
#include "gm_cert_manage.h"
#include "onvif_server.h"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif
#include "cJSON.h" 

/* 检查Mac地址是否符合规范 */
void Task::Network::GetCheckMacValid::handle()
{
    ::Network::Info_S stInfo;
    CNetworkManage::instance()->get_system_networkInfo(stInfo);

    ::Network::CheckMacValid_S stRetInfo;

    CBurnMacUdpServer::instance()->getMacValid(stRetInfo.bMacValid);

    std::vector<std::string> vecMac;
    std::string strMac = stInfo.stIp.physicalAddress;
    std::string strDelimiter = ":";
    size_t start = 0;
    size_t end = strMac.find(strDelimiter);

    while (end != std::string::npos) {
        vecMac.push_back(strMac.substr(start, end - start));
        start = end + strDelimiter.length();
        end = strMac.find(strDelimiter, start);
    }

    vecMac.push_back(strMac.substr(start));

    /* 判断mac地址是否有效 */
    if(vecMac.size() > 3)
    {
        if(vecMac[0].compare("30") == 0 && 
            ((vecMac[1].compare("3a") == 0 &&
                vecMac[2].compare("ba") == 0) ||
            (vecMac[1].compare("3A") == 0 &&
                vecMac[2].compare("BA") == 0) 
            ))
        {
            stRetInfo.bMacValid = true;
        }
    }
    dlog_info("获取MAC烧录状态：网卡[%s] Mac:[%s] 状态:[%d]", 
            stInfo.stIp.netName.c_str(), stInfo.stIp.physicalAddress.c_str(), 
            stRetInfo.bMacValid);

    result(Convert::to_string(stRetInfo));
}

void Task::Network::GetNetworkInfo::handle()
{
    ::Network::Info_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    CNetworkManage::instance()->get_system_networkInfo(stInfo);
    result(Convert::to_string(stInfo));
}
void Task::Network::SetNetworkInfo::handle()
{
    ::Network::Info_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    /* 通知多播服务器 */
    if (stInfo.stIp.bEnableMulticast && 0 != stInfo.stIp.multicastAddress.length())
    {
        CIpcMulticastServer::instance()->start(stInfo.stIp.multicastAddress.c_str());
    }
    else if (!stInfo.stIp.bEnableMulticast)
    {
        CIpcMulticastServer::instance()->stop();
    }

    bool bReboot = false;
    bReboot = CNetworkManage::instance()->need_reboot(stInfo);

    int nRet = CNetworkManage::instance()->set_system_networkInfo(stInfo,!bReboot);
    if (nRet != OK)
    {
        goto exit;
    }
    else
    {
        /* 重启标志命令码 */
        if(bReboot)
        {
            nRet = IpcRet_E::OK_SETNETWORK_AND_REBOOT;
        }
    }

    // /* 重启RtspServer */
    // nRet = CRtspServer::instance()->updateNetworkConfig(stInfo);
    // if (nRet != OK)
    // {
    //     goto exit;
    // }

exit:
    result(nRet);
}
void Task::Network::GetDdnsInfo::handle()
{
    ::Network::Ddns_S stDdns;
    CDdnsManage::instance()->get_ddns_config(stDdns);
    result(Convert::to_string(stDdns));
}
void Task::Network::SetDdnsInfo::handle()
{
    int nRet;
    ::Network::Ddns_S stDdns;
    Convert::to_struct(m_taskData, stDdns);
    nRet = CDdnsManage::instance()->config_ddns(stDdns);
    result(nRet);
}
void Task::Network::GetPppoeInfo::handle()
{
    ::Network::Pppoe_S stPppoe;
     Convert::read_file(PPPOE_CONFIG_FILE, stPppoe);
    result(Convert::to_string(stPppoe));
}
void Task::Network::SetPppoeInfo::handle()
{
    int nRet;
    ::Network::Pppoe_S stPppoe;
    Convert::to_struct(m_taskData, stPppoe);
    nRet = CPppoeManage::instance()->config_pppoe(stPppoe);
    if (nRet == 0)
    {
        Convert::write_file(PPPOE_CONFIG_FILE, stPppoe);
    }
    
    result(nRet);
}
void Task::Network::GetPortInfo::handle()
{ 
    ::Network::PortConfig_S stPortConfig;
    Convert::read_file(PORT_CONFIG_FILE, stPortConfig);
    result(Convert::to_string(stPortConfig));
}
void Task::Network::SetPortInfo::handle()
{
    int nRet = OK;
    ::Network::PortConfig_S stPortConfig;
    Convert::to_struct(m_taskData, stPortConfig);

    ::Network::PortConfig_S stPortConfigTemp;
    Convert::read_file(PORT_CONFIG_FILE, stPortConfigTemp);

    /* 设置http和https端口 */
    if (stPortConfigTemp.nHttpPort != stPortConfig.nHttpPort || stPortConfigTemp.nHttpsPort != stPortConfig.nHttpsPort)
    {
        nRet |= CHttpsManage::instance()->config_port(stPortConfig.nHttpPort,stPortConfig.nHttpsPort);
    }

    /* rtsp端口 */
    if (stPortConfigTemp.nRtspPort != stPortConfig.nRtspPort)
    {
        nRet |= CRtspServer::instance()->setPort(stPortConfig.nRtspPort);
    }

    Convert::write_file(PORT_CONFIG_FILE, stPortConfig);

    result(nRet);
}
void Task::Network::GetPortMapInfo::handle()
{
    ::Network::PortMapConfig_S stPortMapConfig;
    CUpnpManage::instance()->get_port_map(stPortMapConfig);
    result(Convert::to_string(stPortMapConfig));
}
void Task::Network::SetPortMapInfo::handle()
{
    int nRet = OK;
    ::Network::PortMapConfig_S stPortMapConfig;
    Convert::to_struct(m_taskData, stPortMapConfig);
    nRet = CUpnpManage::instance()->set_port_map(stPortMapConfig);
    result(nRet);
}
void Task::Network::GetLogServerInfo::handle()
{
    ::Network::LogServer_S stLogServer;
    result(Convert::to_string(stLogServer));
}
void Task::Network::SetLogServerInfo::handle()
{
    result(0);
}
void Task::Network::GetOhterBaseInfo::handle()
{
    result(0);
}
void Task::Network::SetOhterBaseInfo::handle()
{
    result(0);
}
void Task::Network::GetSnmpInfo::handle()
{
    ::Network::SnmpConfig_S stSnmpConfig;
    CSnmpManage::instance()->getSnmpInfo(stSnmpConfig);
    result(Convert::to_string(stSnmpConfig));
}
void Task::Network::SetSnmpInfo::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_CONFIG_SNMP;
    stLogInfo.host =  CWebServer::instance()->get_userclient_ip();
    LogHandler::instance()->write(stLogInfo);

    ::Network::SnmpConfig_S stSnmpConfig;
    Convert::to_struct(m_taskData, stSnmpConfig);
    result(CSnmpManage::instance()->initializeSnmp(stSnmpConfig));
}

/* 获取邮件信息 */
void Task::Network::GetEamilInfo::handle()
{
    ::Network::EmailInfo_S stEmailInfo;
    CEmailManage::instance()->GetEmailInfo(stEmailInfo);
    result(Convert::to_string(stEmailInfo));
}

/* 设置邮件信息 */
void Task::Network::SetEamilInfo::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_CONFIG_EMAIL;
    stLogInfo.host =  CWebServer::instance()->get_userclient_ip();
    LogHandler::instance()->write(stLogInfo);

    ::Network::EmailInfo_S stEmailInfo;
    Convert::to_struct(m_taskData, stEmailInfo);
    result(CEmailManage::instance()->HandleEmail(stEmailInfo));
}
void Task::Network::GetGb28181Info::handle()
{
    ::Network::GB28181Client_S stGB28181;
    CGB28181::instance()->get_gbClientInfo(stGB28181);
    result(Convert::to_string(stGB28181));
}
void Task::Network::SetGb28181Info::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_CONFIG_SIP_SERVER;
    stLogInfo.host =  CWebServer::instance()->get_userclient_ip();
    LogHandler::instance()->write(stLogInfo);

    ::Network::GB28181Client_S stGB28181;
    Convert::to_struct(m_taskData, stGB28181);
    /* 设置的参数写入配置文件 */
    Convert::write_file(GB28181_CLIENT_CONFIG_FILE, stGB28181);
    // /* 先去初始化 */
    // CGB28181::instance()->deinit();
    result(CGB28181::instance()->init());
}
// void Task::Network::GetOhterSeniorInfo::handle()
// {
//     ::Network::OtherConfig_S stOtherConfig;
//     result(Convert::to_string(stOtherConfig));
// }
void Task::Network::SetOhterSeniorInfo::handle()
{
    result(0);
}
void Task::Network::GetIntegrationProtoInfo::handle()
{
    ::Network::IntegrationProto_S stIntegrationProto;
    result(Convert::to_string(stIntegrationProto));
}
void Task::Network::SetIntegrationProtoInfo::handle()
{
    result(0);
}
void Task::Network::TestEamil::handle()
{
    ::Network::EmailUser_S stEmailUser;
    Convert::to_struct(m_taskData, stEmailUser);

    dlog_info("正在测试邮件功能");
    std::function<void(int)> func = 
    std::bind(static_cast<void(CTask::*)(int)>(&CTask::result), this, std::placeholders::_1);
    CEmailManage::instance()->SendTestEmail(stEmailUser,func);
}
void Task::Network::EventEamil::handle()
{
    ::Network::EmailEventInfo_S stEventInfo;
    Convert::to_struct(m_taskData, stEventInfo);
    int nRet;
    dlog_info("正在发送事件触发邮件");
    nRet = CEmailManage::instance()->HandleEmail(stEventInfo);
    result(nRet);
}

/* 获取所有的CA证书信息 */
void Task::Network::GetTrustCertInfo::handle()
{
    std::vector<::Network::CertFileInfo_S> stInfos;
    CCaManage::instance()->getTrustInfo(stInfos);
    
    result(Convert::to_string(stInfos));
}

/* 安装CA证书 */
void Task::Network::InstallTrustCert::handle()
{
    ::Network::CertFileInfo_S stCertFileInfo;
    int nRet = CCaManage::instance()->dealUploadCaCert();
    result(nRet);
}

/* 删除CA证书 */
void Task::Network::DeleteTrustCert::handle()
{
    ::Network::CertFileInfo_S stCertFileInfo;
    Convert::to_struct(m_taskData, stCertFileInfo);
    int nRet = CCaManage::instance()->deleteTrustceCert(stCertFileInfo);
    result(nRet);   
}

/* 下载CA证书 */
void Task::Network::DownloadTrustCert::handle()
{
    result(0);
}

/* 获取所有的设备证书 */
void Task::Network::GetDeviceCertInfo::handle()
{
    std::vector<::Network::CertFileInfo_S> stInfos;
    CCaManage::instance()->getDeviceInfo(stInfos);
    
    result(Convert::to_string(stInfos));
}

/* 安装设备证书 */
void Task::Network::InstallDeviceCert::handle()
{
   ::Network::CertFileInfo_S stCertFileInfo;
    Convert::to_struct(m_taskData, stCertFileInfo);
    int nRet = CCaManage::instance()->installDeviceCert(stCertFileInfo);
    result(nRet);
}

/* 删除设备证书 */
void Task::Network::DeleteDeviceCert::handle()
{
     ::Network::CertFileInfo_S stCertFileInfo;
    Convert::to_struct(m_taskData, stCertFileInfo);
    int nRet = CCaManage::instance()->deleteDeviceCert(stCertFileInfo);
    result(nRet);   
}

/* 下载设备证书 */
void Task::Network::DownloadDeviceCert::handle()
{
    result(0);
}

/* 创建请求文件 */
void Task::Network::CreateRequestCert::handle()
{
    ::Network::CertApplyInfo_S stApplyInfo;
    Convert::to_struct(m_taskData, stApplyInfo);
    int nRet = CCaManage::instance()->generateCsr(stApplyInfo);
    result(nRet);
}

/* 创建并安装设备证书 */
void Task::Network::CreateAndInstallDeviceCert::handle()
{
    ::Network::CertApplyInfo_S stApplyInfo;
    Convert::to_struct(m_taskData, stApplyInfo);
    int nRet = CCaManage::instance()->creatInstallDeviceCert(stApplyInfo);
    result(nRet);
}

/* 删除证书请求文件 */
void Task::Network::DeleteRuquestCsr::handle()
{
    int nRet = CCaManage::instance()->deleteRequestCsr();
    result(nRet);   
}

/* 获取https配置信息 */
void Task::Network::GetHttpsInfo::handle()
{
    ::Network::HttpsConfigInfo_S stHttpsConfigInfo;
    CHttpsManage::instance()->get_httpsInfo(stHttpsConfigInfo);
    result(Convert::to_string(stHttpsConfigInfo));   
}

/* 配置https信息 */
void Task::Network::ConfigHttpsInfo::handle()
{
    ::Network::HttpsConfigInfo_S stHttpsConfigInfo;
    Convert::to_struct(m_taskData, stHttpsConfigInfo);

    /* 是否开启rtsp */
    if (stHttpsConfigInfo.bEnRtsp && !CRtspServer::instance()->isInit())
    {
        CRtspServer::instance()->init();
    }
    else if (!stHttpsConfigInfo.bEnRtsp && CRtspServer::instance()->isInit())
    {
        CRtspServer::instance()->deinit();
    }

    /* 是否启用https服务 */
    std::function<void(int)> func = 
    std::bind(static_cast<void(CTask::*)(int)>(&CTask::result), this, std::placeholders::_1);
    CHttpsManage::instance()->config_https(stHttpsConfigInfo,func);

}

/* 获取认证方式 */
void Task::Network::GetAuthMethod::handle()
{
    ::System::SecurityCert_S stSecurityCert;
    Convert::read_file(SECURITY_CERT_CONFIG_FILE, stSecurityCert);
    result(Convert::to_string(stSecurityCert));   
}

/* 设置认证方式 */
void Task::Network::SetAuthMethod::handle()
{
    ::System::SecurityCert_S stSecurityCert;
    Convert::to_struct(m_taskData, stSecurityCert);
    int nRet = Convert::write_file(SECURITY_CERT_CONFIG_FILE, stSecurityCert);
    if (!nRet) {
        nRet = CRtspServer::instance()->updateRtspDigestAlgorithm();
        if (!nRet) {
            CRtspServer::instance()->reboot();
        }
    }

    result(nRet);  
}

/* 获取onvif */
void Task::Network::GetOnvifConfigInfo::handle()
{
    ::Network::OnvifConfigInfo_S stOnvifInfo;
    Convert::read_file(ONVIF_CONFIG_FILE, stOnvifInfo);
    result(Convert::to_string(stOnvifInfo));   
}

/* 设置onvif */
void Task::Network::SetOnvifConfigInfo::handle()
{
    ::Network::OnvifConfigInfo_S stOnvifInfo;
    Convert::to_struct(m_taskData, stOnvifInfo);
    COnvifServer::instance()->set_config(stOnvifInfo);
    int nRet = Convert::write_file(ONVIF_CONFIG_FILE, stOnvifInfo);
    result(nRet);  
}

/* 获取QOS */
void Task::Network::GetQosInfo::handle()
{
    ::Network::QosConfigInfo_S stQosInfo;
    CQosManage::instance()->get_qos_config(stQosInfo);
    result(Convert::to_string(stQosInfo));   
}
/* 设置QOS */
void Task::Network::SetQosInfo::handle()
{
    int nRet = 0;
    ::Network::QosConfigInfo_S stQosInfo;
    Convert::to_struct(m_taskData, stQosInfo);
    nRet = CQosManage::instance()->set_qos_config(stQosInfo);

    result(nRet);  
}
/* 获取BONJOUR */
void Task::Network::GetBonjourInfo::handle()
{
    ::Network::BonjourConfigInfo_S stBonjourInfo;
    Convert::read_file(BONJOUR_CONFIG_FILE, stBonjourInfo);
    result(Convert::to_string(stBonjourInfo));   
}
/* 设置BONJOUR */
void Task::Network::SetBonjourInfo::handle()
{
    ::Network::BonjourConfigInfo_S stBonjourInfo;
    Convert::to_struct(m_taskData, stBonjourInfo);
    int nRet = Convert::write_file(BONJOUR_CONFIG_FILE, stBonjourInfo);
    result(nRet);  
}

/**
 * @brief   : 国际证书管理（国密）
 */

/* 创建证书请求文件 */
void Task::Network::GmCreateCertRequestFile::handle()
{
    ::Network::GmCertNetworkType_E enType = ::Network::GmCertNetworkType_E::PUBLIC_SECURITY_INFORMATION_NETWORK;
    /* 解析证书持有者网络类型 */
    Json::Object *pRootJson = Json::init(m_taskData);
    Json::get(pRootJson,"CertNetworkType", (int &)enType);
    Json::deinit(pRootJson);
    std::string strFilePath =  CGmCertManage::instance()->create_cert_request(enType);

    /* 创建国标证书失败 */
    if (strFilePath.empty())
    {
        result(ERR_CREATE_CERT_REQUEST);
    }
    else
    {
        /* 返回创建好的证书请求文件路径 */
        Json::Object *pReturnRootJson = Json::init();
        Json::add(pReturnRootJson, "CertRequestPath", strFilePath);
        result(Json::to_string(pReturnRootJson));
        Json::deinit(pReturnRootJson);
    }
}

/* 上传CA证书 */
void Task::Network::GmUploadCaCert::handle()
{
    /* 证书文件路径 */
    std::string strPath;        
    /* 解析证书文件路径 */
    Json::Object *pRootJson = Json::init(m_taskData);
    Json::get(pRootJson, "Path", strPath);
    Json::deinit(pRootJson);
    ::Network::GmCertFileInfo_S stInfo;
    stInfo.strPath = GM_CA_TRUST_CERT;
    dlog_debug("strPath:%s stInfo.strPath:%s", strPath.c_str(),stInfo.strPath.c_str());

    int nRet = CGmCertManage::instance()->cert_parse(strPath, stInfo);
    if(nRet != OK)
    {
        goto exit;
    }
    nRet = CGmCertConfigure::instance()->set_configure(stInfo);
    if(nRet != OK)
    {
        goto exit;
    }

exit:
    std::set<::Network::GmCertFileInfo_S> astInfo;
    CGmCertConfigure::instance()->get_configure(astInfo);
    result(Convert::to_string(astInfo), nRet);
}

/* 上传本地设备证书 */
void Task::Network::GmUploadDeviceCert::handle()
{
    /* 证书文件路径 */
    std::string strPath;
    /* 解析证书文件路径 */
    Json::Object *pRootJson = Json::init(m_taskData);
    Json::get(pRootJson, "Path", strPath);
    Json::deinit(pRootJson);
    ::Network::GmCertFileInfo_S stInfo;
    stInfo.strPath = GM_CA_DEVICE_CERT;
    dlog_debug("strPath:%s stInfo.strPath:%s", strPath.c_str(), stInfo.strPath.c_str());

    int nRet = CGmCertManage::instance()->cert_parse(strPath, stInfo);
    if (nRet != OK)
    {
        goto exit;
    }
    nRet = CGmCertConfigure::instance()->set_configure(stInfo);
    if (nRet != OK)
    {
        goto exit;
    }

exit:
    std::set<::Network::GmCertFileInfo_S> astInfo;
    CGmCertConfigure::instance()->get_configure(astInfo);
    result(Convert::to_string(astInfo), nRet);
}

/* 上传CRL文件 */
void Task::Network::GmUploadCrlFile::handle()
{
    /* 证书文件路径 */
    std::string strPath;
    /* 解析证书文件路径 */
    Json::Object *pRootJson = Json::init(m_taskData);
    Json::get(pRootJson, "Path", strPath);
    Json::deinit(pRootJson);
    ::Network::GmCrlFileInfo_S  stInfo;
    stInfo.strPath = GM_CA_TRUST_CRL;
    dlog_debug("strPath:%s stInfo.strPath:%s", strPath.c_str(), stInfo.strPath.c_str());

    int nRet = CGmCertManage::instance()->crl_parse(strPath, stInfo);
    if (nRet != OK)
    {
        goto exit;
    }
    nRet = CGmCertConfigure::instance()->set_configure(stInfo);
    if (nRet != OK)
    {
        goto exit;
    }
exit:
    result(nRet);
}

/* 获取当前证书信息 */
void Task::Network::GmGetCertInfo::handle()
{
    std::set<::Network::GmCertFileInfo_S> astInfo;
    CGmCertConfigure::instance()->get_configure(astInfo);
    result(Convert::to_string(astInfo));
}

/* 删除证书文件 */
void Task::Network::GmDeleteCertFile::handle()
{
    ::Network::GmCertFileInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    CGmCertConfigure::instance()->del_configure(stInfo);

    /* 重新编号，从1开始依次递增 */
    CGmCertConfigure::instance()->sort_configure(stInfo);

    std::set<::Network::GmCertFileInfo_S> astInfo;
    CGmCertConfigure::instance()->get_configure(astInfo);
    result(Convert::to_string(astInfo));
}

#ifdef ENABLE_GAT1400_SRC
void Task::Network::GetGat1400Info::handle()
{
    ::Network::Gat1400Client_S stInfo;
    GAT1400::CGAT1400::instance()->getGat1400Config(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Network::SetGat1400Info::handle()
{
    ::Network::Gat1400Client_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 设置的参数写入配置文件 */
    Convert::write_file(GAT1400_CONFIG_FILE, stInfo);

    result(GAT1400::CGAT1400::instance()->init());
}
#endif


#ifdef CAP_NETWORK_WIFI
void Task::Network::SetWifiStaInfo::handle()
{
    ::Network::WifiStaInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    
    bool EnhancedMode = false; 
    std::string resultStr = "[]"; 
    set_wifi_config(stInfo);

    if(stInfo.bEnableWifi)
    {
    /* wifi初始化 */
        int nRet = CWifiManager::instance()->init();
        if (nRet < OK)
        {
            dlog_error("wifi管理模块初始化失败：%d", nRet);
            result(resultStr);
            return;
        }

        auto scanResults = CWifiManager::instance()->scanWifi(); 
        std::cout << "===== WiFi 扫描开始，共发现 " << scanResults.size() << " 个热点 =====" << std::endl;
        // 1. 创建根对象 (cJSON 风格)
        cJSON* rootObj = cJSON_CreateObject();
        cJSON* apArray = cJSON_CreateArray(); // 创建数组

        for (const auto& ap : scanResults) 
        {
            cJSON* apObj = cJSON_CreateObject(); // 创建单个对象


            std::cout << "[SCAN] SSID: " << ap.ssid 
                      << " | 信号: " << ap.signal_level << " dBm"
                      << " | 加密: " << ap.security_type
                      << std::endl;
            // 添加字符串字段
            cJSON_AddStringToObject(apObj, "ssid", ap.ssid.c_str());
            cJSON_AddStringToObject(apObj, "security_type", ap.security_type.c_str());
            cJSON_AddStringToObject(apObj, "band", ap.band.c_str());
            
            // 添加布尔字段
            cJSON_AddBoolToObject(apObj, "is_current", ap.is_current);

            // 处理信号强度 (rssi)
            std::string rssiText;
            if(ap.signal_level >= -50) rssiText = "极强";
            else if(ap.signal_level >= -70) rssiText = "强";
            else if(ap.signal_level >= -80) rssiText = "较差";
            else rssiText = "极差";
            
            cJSON_AddStringToObject(apObj, "rssi", rssiText.c_str());
            // 也可以顺便加上原始数值
            cJSON_AddNumberToObject(apObj, "signal_value", ap.signal_level);

            // 将对象加入数组
            cJSON_AddItemToArray(apArray, apObj);
        }

        // 将数组加入根对象
        cJSON_AddItemToObject(rootObj, "list", apArray);

        // 处理增强模式
        if(stInfo.bEnableBoost)
        {
            EnhancedMode = CWifiManager::instance()->setWifiEnhancedMode();
            cJSON_AddBoolToObject(rootObj, "enhancedMode", EnhancedMode);
        }


        // 2. 序列化为字符串
        char* jsonStr = cJSON_PrintUnformatted(rootObj);
        if (jsonStr) {
            resultStr = jsonStr;
            free(jsonStr); // cJSON_PrintUnformatted 分配的内存需要手动释放
        }

        // 3. 释放 cJSON 对象树，防止内存泄漏
        cJSON_Delete(rootObj); 
    }
    else
    {
        // WiFi 未启用
        cJSON* emptyObj = cJSON_CreateObject();
        cJSON_AddStringToObject(emptyObj, "status", "wifi_disabled");
        
        char* jsonStr = cJSON_PrintUnformatted(emptyObj);
        if (jsonStr) {
            resultStr = jsonStr;
            free(jsonStr);
        }
        cJSON_Delete(emptyObj);
    }

    result(resultStr);
}

void Task::Network::ConnectWifiSta::handle()
{
    ::Network::WifiStaConncet_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    
    ::Network::WifiConnectResult res  = CWifiManager::instance()-> connectToWifi(stInfo);
    result(Convert::to_string(res));
}

void Task::Network::DisconnectWifiSta::handle()
{
    bool nRet = CWifiManager::instance()-> disconnectWifi();
    if(nRet)
    {
        result(0);
    }else {
        result(-1);
    }
}
#endif
#ifdef CAP_NETWORK_4G
void Task::Network::Get4GInfo::handle()
{
    ::Network::SIM_Info_t info;
    int nRet = FourGManager::instance()-> init();
    if(nRet != 0)
    {
        result(-1);
        return;
    }
    nRet = FourGManager::instance()-> getSimInfo(info);
    if(nRet == 0)
    {
        result(Convert::to_string(info));
    }
}


void Task::Network::Set4GInfo::handle()
{
    ::Network::Network_4G_Config_t stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = FourGManager::instance()-> init();
    if(nRet != 0)
    {
        result(-1);
        return;
    }
    nRet = FourGManager::instance()-> setConfig(stInfo);
    if(nRet == 0)
    {
        result(0);
    }
}

#endif

#ifdef CAP_NETWORK_WIFI
void Task::Network::SetHotspot::handle()
{
    ::Network::HotspotConfig stInfo;
    APConfig ap_cfg;
    Convert::to_struct(m_taskData, stInfo);
    if(stInfo.enabled)
    {
        if(stInfo.password == stInfo.confirmPassword)
        {
            ap_cfg.ssid = stInfo.ssid;
            ap_cfg.password = stInfo.password;
            ap_cfg.confirm_password = stInfo.confirmPassword;
            if(stInfo.encryptionType == "AES")
            {
                ap_cfg.encryption = EncryptionType::AES;
            }else if(stInfo.encryptionType == "TKIP")
            {
                ap_cfg.encryption = EncryptionType::TKIP;
            }else if(stInfo.encryptionType == "TKIP_AES")
            {
                ap_cfg.encryption = EncryptionType::TKIP_AES;
            }
            if (!HostapdManager::instance()->SetNetworkConfig(ap_cfg)) {
                std::cerr << "Config Error" << std::endl;
                return;
            }
            DHCPConfig dhcp_cfg;
            dhcp_cfg.interface = "wlan0";
            dhcp_cfg.gateway = "192.168.4.1";
            HostapdManager::instance()->SetDHCPConfig(dhcp_cfg);
            CWifiManager::instance()->deinit();/*关闭WiFi功能 */
            auto ret = HostapdManager::instance()->Init("eth0");
            if (ret != InitResult::SUCCESS) {
                switch (ret) {
                    case InitResult::ERR_SSID_EMPTY:
                        printf("初始化失败：SSID 不能为空！\n");
                        result(1);
                        break;
                    case InitResult::ERR_CONFIG_FILE:
                        printf("初始化失败：无法写入配置文件，请检查权限！\n");
                        result(3);
                        break;
                    case InitResult::ERR_STARTUP_FAILED:
                        printf("初始化失败：热点进程启动失败（可能是密码太短或配置冲突）！\n");
                        result(4);
                        break;
                    default:
                        printf("初始化失败：未知错误！\n");
                        result(5);
                        break;
                }
            } else {
                printf("热点启动成功！\n");
            }
            result(0);
        }
    }
    else {
        HostapdManager::instance()->Deinit();

        result(0);
    }
    
    
}
void Task::Network::GetHotspotConn::handle()
{
    auto clients = HostapdManager::instance()->GetConnectedDevices();
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        std::cerr << "Failed to  create JSON object" << std::endl;
        return;
    }

    cJSON_AddStringToObject(root, "status", "success");
    cJSON_AddNumberToObject(root, "total", clients.size());

    cJSON *devices_array = cJSON_CreateArray();
    if (devices_array == NULL) {
        cJSON_Delete(root); 
        return;
    }
    

    for (const auto& c : clients) {

        cJSON *device_item = cJSON_CreateObject();
        
        cJSON_AddNumberToObject(device_item, "index", c.index);
        cJSON_AddStringToObject(device_item, "mac", c.mac.c_str());
        cJSON_AddStringToObject(device_item, "ip", c.ip.c_str());
        cJSON_AddStringToObject(device_item, "conn_time", c.conn_time.c_str());

        cJSON_AddItemToArray(devices_array, device_item);
    }
    cJSON_AddItemToObject(root, "devices", devices_array);
    
    char *json_string = cJSON_Print(root);

    if (json_string != NULL) {
        std::cout << "=== JSON Output ===" << std::endl;
        std::cout << json_string << std::endl;
        
        result(json_string);
    }
    free(json_string);
    
    cJSON_Delete(root);
}
#endif