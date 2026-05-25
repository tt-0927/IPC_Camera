/*** 
 * @FilePath     : network_convert.h
 * @Author       : huangjunda
 * @Date         : 2025-03-25 11:03:38
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-01 20:06:35
 * @Description  : 
 */
#pragma once
#include <set>
#include "Json.h"
#include "network_define.h"

namespace Convert
{
	void deal(Json::Object* pRootJson, Network::CheckMacValid_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::Ip_S &stIp, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::Dns_S &stDns, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::Info_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::Ddns_S &stDdns, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::Pppoe_S &stPppoe, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::PortConfig_S &stPortConfig, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::PortMap_S &stPortMap, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::PortMapConfig_S &stPortMapConfig, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::LogServer_S &stLogServer, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::SnmpSecuritySettings_S &stSnmpSecurity, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::SnmpConfig_S &stSnmpConfig, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::SmtpServer_S &stSmtp, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::EmailUser_S &stEmaiUser, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::EmailEventInfo_S &stEmailEvent, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::EmailInfo_S &stEmail, bool bOutStruct);
	void deal(Json::Object *pRootJson, Network::EncodingId_S &stEncode, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Network::EncodingId_S> &stEncode, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::GB28181Client_S &stGB28181, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::Isup_S &stIsup, bool bOutStruct);
	// void deal(Json::Object* pRootJson, Network::OtherConfig_S &stOtherConfig, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::IntegrationProto_S &stIntegrationProto, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::CertApplyInfo_S &stCertApplyInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::CertFileInfo_S &stCertFileInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Network::CertFileInfo_S> &stCertFileInfos, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::HttpsConfigInfo_S &stHttpsConfigInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::OnvifConfigInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::QosConfigInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::BonjourConfigInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::GmCertFileInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Network::GmCertFileInfo_S> &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::set<Network::GmCertFileInfo_S> &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::RevokedCertInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Network::RevokedCertInfo_S> &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::GmCrlFileInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::WifiStaInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Network::WifiStaConncet_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pJson, Network::WifiConnectResult &key, bool bOutStruct);
	void deal(Json::Object* pJson, Network::WepKeyConfig &key, bool bOutStruct);
	void deal(Json::Object* pJson, Network::SIM_Info_t &stInfo, bool bOutStruct);
	void deal(Json::Object* pJson, Network::Network_4G_Config_t &stInfo, bool bOutStruct);
	void deal(Json::Object* pJson, Network::HotspotConfig &stInfo, bool bOutStruct);
	void deal(Json::Object* pJson, Network::Platform_Info_t &stInfo, bool bOutStruct);
	void deal(Json::Object* pJson, Network::Platform_Store_Info_t &stInfo, bool bOutStruct);
#ifdef ENABLE_GAT1400_SRC
	void deal(Json::Object* pRootJson, Network::Gat1400Client_S &stInfo, bool bOutStruct);
#endif
}
