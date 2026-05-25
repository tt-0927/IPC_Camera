/**
 * @FilePath     : network_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-08 17:44:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-21 14:32:04
 * @Description  : 网络相关定义结构体转换
 */

#include "network_convert.h"
#include "convert.h" /* 这个要放在network_convert.h的后面 */

void Convert::deal(Json::Object *pRootJson, Network::CheckMacValid_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "MacValid", stInfo.bMacValid);
}

void Convert::deal(Json::Object *pRootJson, Network::Ip_S &stIp, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "NetType", (int &)stIp.enType);
	convert.field(pRootJson, "EnableDhcp", stIp.bEnableDhcp);
	convert.field(pRootJson, "NetName", stIp.netName);
	convert.field(pRootJson, "Ipv4Ip", stIp.ipv4Ip);
	convert.field(pRootJson, "Ipv4Mask", stIp.ipv4Mask);
	convert.field(pRootJson, "Ipv4Gateway", stIp.ipv4Gateway);
	convert.field(pRootJson, "Ipv6Ip", stIp.ipv6Ip);
	convert.field(pRootJson, "Ipv6Gateway", stIp.ipv6Gateway);
	convert.field(pRootJson, "PhysicalAddress", stIp.physicalAddress);
	convert.field(pRootJson, "Mtu", stIp.nMtu);
	convert.field(pRootJson, "EnableMulticast", stIp.bEnableMulticast);
	convert.field(pRootJson, "MulticastAddress", stIp.multicastAddress);
}

void Convert::deal(Json::Object *pRootJson, Network::Dns_S &stDns, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableAutoDns", stDns.bEnableAutoDns);
	convert.field(pRootJson, "Main", stDns.main);
	convert.field(pRootJson, "Andby", stDns.standby);
}

void Convert::deal(Json::Object *pRootJson, Network::Info_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "NetInfos", stInfo.stIp);
	convert.structure(pRootJson, "DnsInfos", stInfo.stDns);
}

void Convert::deal(Json::Object *pRootJson, Network::Ddns_S &stDdns, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableDdns", stDdns.bEnableDdns);
	convert.field(pRootJson, "Address", stDdns.address);
	convert.field(pRootJson, "RealmName", stDdns.realmName);
	convert.field(pRootJson, "User", stDdns.user);
	convert.field(pRootJson, "Password", stDdns.password);
	convert.field(pRootJson, "Status", stDdns.nStatus);
}

void Convert::deal(Json::Object *pRootJson, Network::Pppoe_S &stPppoe, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnablePppoe", stPppoe.bEnablePppoe);
	convert.field(pRootJson, "Ip", stPppoe.strIp);
	convert.field(pRootJson, "User", stPppoe.strUser);
	convert.field(pRootJson, "Password", stPppoe.strPassword);
}

void Convert::deal(Json::Object *pRootJson, Network::PortConfig_S &stPortConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "HttpPort", stPortConfig.nHttpPort);
	convert.field(pRootJson, "RtspPort", stPortConfig.nRtspPort);
	convert.field(pRootJson, "HttpsPort", stPortConfig.nHttpsPort);
	convert.field(pRootJson, "ServerPort", stPortConfig.nServerPort);
	convert.field(pRootJson, "WebServerPort", stPortConfig.nWebServerPort);
}

void Convert::deal(Json::Object *pRootJson, Network::PortMap_S &stPortMap, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "PortType", stPortMap.nPortType);
	convert.field(pRootJson, "ExternPort", stPortMap.nExternPort);
	convert.field(pRootJson, "ExternIp", stPortMap.externIp);
	convert.field(pRootJson, "InternalPort", stPortMap.nInternalPort);
	convert.field(pRootJson, "Status", stPortMap.nStatus);
}

void Convert::deal(Json::Object *pRootJson, Network::PortMapConfig_S &stPortMapConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Name", stPortMapConfig.strName);
	convert.field(pRootJson, "EnablePortMap", stPortMapConfig.bEnablePortMap);
	convert.field(pRootJson, "MapType", stPortMapConfig.nMapType);
	convert.structure(pRootJson, "PortMap", stPortMapConfig.portMap);
}

void Convert::deal(Json::Object *pRootJson, Network::LogServer_S &stLogServer, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableLogServer", stLogServer.bEnableLogServer);
	convert.field(pRootJson, "Address", stLogServer.address);
	convert.field(pRootJson, "Port", stLogServer.nPort);
	convert.field(pRootJson, "UploadInterval", stLogServer.nUploadInterval);
}

void Convert::deal(Json::Object *pRootJson, Network::SnmpSecuritySettings_S &stSnmpSecurity, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Auth", (int &)stSnmpSecurity.enAuth);
	convert.field(pRootJson, "AuthPassword", stSnmpSecurity.strAuthPassword);
	convert.field(pRootJson, "Priv", (int &)stSnmpSecurity.enPriv);
	convert.field(pRootJson, "PrivPassword", stSnmpSecurity.strPrivPassword);
}

void Convert::deal(Json::Object *pRootJson, Network::SnmpConfig_S &stSnmpConfig, bool bOutStruct)
{

	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableSnmp", stSnmpConfig.bEnableSnmp);
	convert.field(pRootJson, "EnableSmnpV3", stSnmpConfig.bEnableSmnpV3);
	convert.field(pRootJson, "ReadCommunityName", stSnmpConfig.strReadCommunityName);
	convert.field(pRootJson, "WriteCommunityName", stSnmpConfig.strWriteCommunityName);
	convert.field(pRootJson, "TrapAddress", stSnmpConfig.strTrapAddress);
	convert.field(pRootJson, "TrapPort", stSnmpConfig.nTrapPort);
	convert.field(pRootJson, "SnmpPort", stSnmpConfig.nSnmpPort);
	convert.field(pRootJson, "ReadSecurityName", stSnmpConfig.strReadSecurityName);
	convert.structure(pRootJson, "ReadInfo", stSnmpConfig.stReadSecuritySettings);
	convert.field(pRootJson, "WriteSecurityName", stSnmpConfig.strWriteSecurityName);
	convert.structure(pRootJson, "WriteInfo", stSnmpConfig.stWriteSecuritySettings);
}

void Convert::deal(Json::Object *pRootJson, Network::SmtpServer_S &stSmtp, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Address", stSmtp.strAddress);
	convert.field(pRootJson, "Port", stSmtp.nPort);
}

void Convert::deal(Json::Object *pRootJson, Network::EmailUser_S &stEmaiUser, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Name", stEmaiUser.strName);
	convert.field(pRootJson, "Address", stEmaiUser.strAddress);
}

void Convert::deal(Json::Object *pRootJson, Network::EmailEventInfo_S &stEmailEvent, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Subject", stEmailEvent.strSubject);
	convert.field(pRootJson, "Message", stEmailEvent.strMessage);
	convert.field(pRootJson, "ImageFile", stEmailEvent.vecImageFile);
}

void Convert::deal(Json::Object *pRootJson, Network::EmailInfo_S &stEmail, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableTLS", stEmail.bTlsEnable);
	convert.field(pRootJson, "IsServerAuthentication", stEmail.bEnServerAuthentication);
	convert.field(pRootJson, "User", stEmail.strUserName);
	convert.field(pRootJson, "Password", stEmail.strPassword);
	convert.field(pRootJson, "IsUseAttachment", stEmail.bImageAttachment);
	convert.field(pRootJson, "CapInterval", stEmail.nCaptureTimeInterval);
	convert.field(pRootJson, "SenderName", stEmail.stSender.strName);
	convert.field(pRootJson, "SenderAddress", stEmail.stSender.strAddress);
	convert.field(pRootJson, "SmtpAddress", stEmail.stServer.strAddress);
	convert.field(pRootJson, "SmtpPort", stEmail.stServer.nPort);
	convert.structure(pRootJson, "Recipients", stEmail.stRecipient);
}

void Convert::deal(Json::Object *pRootJson, Network::EncodingId_S &stEncode, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "AlarmInputId", stEncode.alarmInputId);
	convert.field(pRootJson, "ChnId", stEncode.chnId);
	convert.field(pRootJson, "WhiteList", stEncode.whiteList);
}


void Convert::deal(Json::Object *pRootJson, std::vector<Network::EncodingId_S> &stEncode, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "EncodeId", stEncode);
}

void Convert::deal(Json::Object *pRootJson, Network::GB28181Client_S &stGB28181, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableGB28181", stGB28181.bEnableGB28181);
	convert.field(pRootJson, "status", stGB28181.bStatus);
	convert.field(pRootJson, "EnableGB35114", stGB28181.bEnableGB35114);
	convert.field(pRootJson, "LocalSipPort", stGB28181.nLocalSipPort);
	convert.field(pRootJson, "ServerID", stGB28181.serverID);
	convert.field(pRootJson, "Realm", stGB28181.realm);
	convert.field(pRootJson, "Address", stGB28181.address);
	convert.field(pRootJson, "ServerPort", stGB28181.nServerPort);
	convert.field(pRootJson, "UserId", stGB28181.userId);
	convert.field(pRootJson, "Password", stGB28181.password);
	convert.field(pRootJson, "ValidityPeriod", stGB28181.nValidityPeriod);
	convert.field(pRootJson, "HeartbeatInterval", stGB28181.nHeartbeatInterval);
	convert.field(pRootJson, "MaxHeartbeatTimeout", stGB28181.nMaxHeartbeatTimeout);
	convert.field(pRootJson, "SpeedType", stGB28181.nSpeedType);
	convert.field(pRootJson, "Speed", stGB28181.nSpeed);
	convert.field(pRootJson, "Magnification", stGB28181.nMagnification);
	convert.field(pRootJson, "StreamPrivateInfo", stGB28181.bStreamPrivateInfo);
	convert.structure(pRootJson,stGB28181.stEncode);

}

void Convert::deal(Json::Object *pRootJson, Network::Isup_S &stIsup, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableIsup", stIsup.bEnableIsup);
	convert.field(pRootJson, "Address", stIsup.address);
	convert.field(pRootJson, "Port", stIsup.nPort);
	convert.field(pRootJson, "RegisterStatus", stIsup.nRegisterStatus);
	convert.field(pRootJson, "ProtoVersion", stIsup.protoVersion);
	convert.field(pRootJson, "CrypKey", stIsup.encrypKey);
}

void Convert::deal(Json::Object *pRootJson, Network::IntegrationProto_S &stIntegrationProto, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableOnvif", stIntegrationProto.bEnableOnvif);
	convert.field(pRootJson, "OnvifMode", stIntegrationProto.nOnvifMode);
	convert.field(pRootJson, "EnableIsapi", stIntegrationProto.bEnableIsapi);
}

void Convert::deal(Json::Object *pRootJson, Network::CertApplyInfo_S &stCertApplyInfo, bool bOutStruct)
{

	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Valday", stCertApplyInfo.nValday);
	convert.field(pRootJson, "CN", stCertApplyInfo.strCN);
	convert.field(pRootJson, "C", stCertApplyInfo.strC);
	convert.field(pRootJson, "O", stCertApplyInfo.strO);
	convert.field(pRootJson, "OU", stCertApplyInfo.strOU);
	convert.field(pRootJson, "ST", stCertApplyInfo.strST);
	convert.field(pRootJson, "L", stCertApplyInfo.strL);
	convert.field(pRootJson, "Email", stCertApplyInfo.strEmail);
}

void Convert::deal(Json::Object *pRootJson, Network::CertFileInfo_S &stCertFileInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Num", stCertFileInfo.nNum);
	convert.field(pRootJson, "ExpiraDate", stCertFileInfo.strExpiraDate);
	convert.field(pRootJson, "SerialNum", stCertFileInfo.strSerialNum);
	convert.field(pRootJson, "User", stCertFileInfo.strUser);
	convert.field(pRootJson, "Licensor", stCertFileInfo.strLicensor);
	convert.field(pRootJson, "Path", stCertFileInfo.strPath);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Network::CertFileInfo_S> &stCertFileInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "CertFileInfos", stCertFileInfos);
}

void Convert::deal(Json::Object *pRootJson, Network::HttpsConfigInfo_S &stHttpsConfigInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnHttps", stHttpsConfigInfo.bEnHttps);
	convert.field(pRootJson, "CertPath", stHttpsConfigInfo.strCertPath);
	convert.field(pRootJson, "KeyPath", stHttpsConfigInfo.strKeyPath);
	convert.field(pRootJson, "EnRtsp", stHttpsConfigInfo.bEnRtsp);
}

void Convert::deal(Json::Object* pRootJson, Network::OnvifConfigInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnOnvif", stInfo.bEnOnvif);
	convert.field(pRootJson, "OnvifAuthMode", stInfo.nOnvifAuthMode);
}

void Convert::deal(Json::Object* pRootJson, Network::QosConfigInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "MediaDSCP", stInfo.nMediaDscp);
	convert.field(pRootJson, "AlarmDSCP", stInfo.nAlarmDscp);
	convert.field(pRootJson, "ManageDSCP", stInfo.nManageDscp);
}

void Convert::deal(Json::Object* pRootJson, Network::BonjourConfigInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnBonjour", stInfo.bEnBonjour);
	convert.field(pRootJson, "HostName", stInfo.strHostName);
	convert.field(pRootJson, "ServerName", stInfo.strServerName);
	convert.field(pRootJson, "Port", stInfo.nPort);
}

void Convert::deal(Json::Object *pRootJson, Network::GmCertFileInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Num", stInfo.nNum);
	convert.field(pRootJson, "SerialNum", stInfo.strSerialNum);
	convert.field(pRootJson, "User", stInfo.strUser);
	convert.field(pRootJson, "Function", stInfo.strFunction);
	convert.field(pRootJson, "EffectiveDate", stInfo.strEffectiveDate);
	convert.field(pRootJson, "ExpiraDate", stInfo.strExpiraDate);
	convert.field(pRootJson, "Path", stInfo.strPath);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Network::GmCertFileInfo_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "GmCertFileInfo", stInfo);
}

void Convert::deal(Json::Object *pRootJson, std::set<Network::GmCertFileInfo_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "GmCertFileInfo", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Network::RevokedCertInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "SerialNum", stInfo.strSerialNum);
	convert.field(pRootJson, "RevocationDate", stInfo.strRevocationDate);
	convert.field(pRootJson, "ReasonCode", stInfo.strReasonCode);
	convert.field(pRootJson, "InvalidityDate", stInfo.strInvalidityDate);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Network::RevokedCertInfo_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "RevokedCertInfo", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Network::GmCrlFileInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "strIssuer", stInfo.strIssuer);
	convert.field(pRootJson, "strThisUpdate", stInfo.strThisUpdate);
	convert.field(pRootJson, "strNextUpdate", stInfo.strNextUpdate);
	convert.field(pRootJson, "nCrlNumber", stInfo.nCrlNumber);
	convert.field(pRootJson, "strPath", stInfo.strPath);
	convert.structure(pRootJson, "RevokedCerts", stInfo.vecRevokedCerts);
}

#ifdef ENABLE_GAT1400_SRC
void Convert::deal(Json::Object* pRootJson, Network::Gat1400Client_S &stInfo, bool bOutStruct)
{
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "enableGat1400", stInfo.enableGat1400);
	convert.field(pRootJson, "status", stInfo.status);
	convert.field(pRootJson, "deviceID", stInfo.deviceID);
	convert.field(pRootJson, "ip", stInfo.ip);
	convert.field(pRootJson, "port", stInfo.port);
	convert.field(pRootJson, "name", stInfo.name);
	convert.field(pRootJson, "passwd", stInfo.passwd);
	convert.field(pRootJson, "pointType", stInfo.pointType);
	convert.field(pRootJson, "keepAlive", stInfo.keepAlive);
	convert.field(pRootJson, "enableFace", stInfo.enableFace);
	convert.field(pRootJson, "enablePerson", stInfo.enablePerson);
	convert.field(pRootJson, "enableMotorvehicle", stInfo.enableMotorvehicle);
	convert.field(pRootJson, "enableNonmotorvehicle", stInfo.enableNonmotorvehicle);
	convert.field(pRootJson, "version", stInfo.version);
}
#endif

void  Convert::deal(Json::Object* pRootJson, Network::WifiStaInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "enable_wifi", stInfo.bEnableWifi);
    convert.field(pRootJson, "enable_boost", stInfo.bEnableBoost);
}

void Convert::deal(Json::Object* pRootJson, Network::WifiStaConncet_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    Convert::CConvert convert(bOutStruct);

    // ===== 基础字段 =====
    convert.field(pRootJson, "ssid", stInfo.ssid);
    convert.field(pRootJson, "password", stInfo.password);
    convert.field(pRootJson, "pairwise", stInfo.pairwise);

    // ===== mode =====
    std::string modeStr;
    convert.field(pRootJson, "mode", modeStr);

    if (modeStr == "OPEN")
        stInfo.mode = Network::WifiSecurityMode::OPEN;
    else if (modeStr == "WEP")
        stInfo.mode = Network::WifiSecurityMode::WEP;
    else if (modeStr == "WPA_PERSONAL")
        stInfo.mode = Network::WifiSecurityMode::WPA_PERSONAL;
    else if (modeStr == "EAP_PEAP")
        stInfo.mode = Network::WifiSecurityMode::EAP_PEAP;
    else if (modeStr == "EAP_TLS")
        stInfo.mode = Network::WifiSecurityMode::EAP_TLS;
	else if (modeStr == "EAP_TTLS")
        stInfo.mode = Network::WifiSecurityMode::EAP_TTLS;

    // ===== 分模式 =====
    switch (stInfo.mode)
    {
        case Network::WifiSecurityMode::WEP:
        {
            convert.field(pRootJson, "wep_key_len", stInfo.wep_key_len);
            convert.field(pRootJson, "wep_is_hex", stInfo.wep_is_hex);
            convert.field(pRootJson, "auth_alg", stInfo.auth_alg);

            convert.structure(pRootJson, "wep_keys", stInfo.wep_keys);
            break;
        }

        case Network::WifiSecurityMode::EAP_PEAP:
        {
            convert.field(pRootJson, "eap_identity", stInfo.eap_identity);
            convert.field(pRootJson, "eap_password", stInfo.eap_password);
            convert.field(pRootJson, "peap_version", stInfo.peap_version);
            convert.field(pRootJson, "phase2", stInfo.phase2);
            convert.field(pRootJson, "anonymous_identity", stInfo.anonymous_identity);
            convert.field(pRootJson, "ca_cert_path", stInfo.ca_cert_path);
			convert.field(pRootJson, "peap_label", stInfo.peap_label);
            break;
        }

        case Network::WifiSecurityMode::EAP_TLS:
        {
            convert.field(pRootJson, "tls_identity", stInfo.tls_identity);
            convert.field(pRootJson, "private_key_passwd", stInfo.private_key_passwd);
            convert.field(pRootJson, "client_cert_path", stInfo.client_cert_path);
            convert.field(pRootJson, "private_key_path", stInfo.private_key_path);
            break;
        }

		case Network::WifiSecurityMode::EAP_TTLS:
        {
            convert.field(pRootJson, "ttls_identity", stInfo.eap_identity );
			convert.field(pRootJson, "ttls_password", stInfo.eap_password);
            convert.field(pRootJson, "anonymous_identity", stInfo.anonymous_identity);/*匿名身份 */
            convert.field(pRootJson, "eap_ttls_phase2", stInfo.eap_ttls_phase2);/*内部认证*/
            convert.field(pRootJson, "ca_cert_path", stInfo.ca_cert_path);
            break;
        }

        default:
            break;
    }

    convert.field(pRootJson, "eapol_version", stInfo.eapol_version);
}


void Convert::deal(Json::Object* pJson, Network::WifiConnectResult &key, bool bOutStruct)
{
	Convert::CConvert convert(bOutStruct);

    convert.field(pJson, "success", key.success);
    convert.field(pJson, "ip_address", key.ip_address);
	convert.field(pJson, "error_code", key.error_code);
}

void Convert::deal(Json::Object* pJson, Network::WepKeyConfig &key, bool bOutStruct)
{
    if (!pJson)
        return;

    Convert::CConvert convert(bOutStruct);

    convert.field(pJson, "index", key.index);
    convert.field(pJson, "value", key.value);
}


void Convert::deal(Json::Object* pJson, Network::SIM_Info_t &stInfo, bool bOutStruct)
{
    if (!pJson)
	{
    	return;
	}

	Convert::CConvert convert(bOutStruct);

	// 字符串类型字段映射
	convert.field(pJson, "status", stInfo.status);
	convert.field(pJson, "ip_address", stInfo.ip_address);
	convert.field(pJson, "subnet_mask", stInfo.subnet_mask);
	convert.field(pJson, "gateway", stInfo.gateway);
	convert.field(pJson, "dns_address", stInfo.dns_address);
	convert.field(pJson, "signal_quality", stInfo.signal_quality);
	convert.field(pJson, "operator_name", stInfo.operator_name);
	
	// 布尔类型字段映射
	convert.field(pJson, "is_registered", stInfo.is_registered);

	convert.field(pJson, "imei", stInfo.imei);
	convert.field(pJson, "iccid", stInfo.iccid);
	convert.field(pJson, "network_type",stInfo.network_type);

	convert.field(pJson, "apn", stInfo.set_config_ret.apn);
    convert.field(pJson, "username", stInfo.set_config_ret.username);
    convert.field(pJson, "password", stInfo.set_config_ret.password);
    convert.field(pJson, "call_number", stInfo.set_config_ret.call_number);
	convert.field(pJson, "phone_number", stInfo.set_config_ret.phone_number);
    convert.field(pJson, "mtu", stInfo.set_config_ret.mtu);
    convert.field(pJson, "auth_mode", stInfo.set_config_ret.auth_mode);
    convert.field(pJson, "network_mode", stInfo.set_config_ret.network_mode);
    convert.field(pJson, "dial_mode", stInfo.set_config_ret.dial_mode);
	

}

void Convert::deal(Json::Object* pJson, Network::Network_4G_Config_t &stInfo, bool bOutStruct)
{
    if (!pJson)
	{
    	return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pJson, "apn", stInfo.apn);
    convert.field(pJson, "username", stInfo.username);
    convert.field(pJson, "password", stInfo.password);
    convert.field(pJson, "call_number", stInfo.call_number);
	convert.field(pJson, "phone_number", stInfo.phone_number);
    convert.field(pJson, "mtu", stInfo.mtu);
    convert.field(pJson, "auth_mode", stInfo.auth_mode);
    convert.field(pJson, "network_mode", stInfo.network_mode);
    convert.field(pJson, "dial_mode", stInfo.dial_mode);
	
}


void Convert::deal(Json::Object* pJson, Network::HotspotConfig &stInfo, bool bOutStruct)
{
    if (!pJson)
	{
    	return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pJson, "enabled", stInfo.enabled);
    convert.field(pJson, "ssid", stInfo.ssid);
    convert.field(pJson, "password", stInfo.password);
    convert.field(pJson, "securityMode", stInfo.securityMode);
    convert.field(pJson, "encryptionType", stInfo.encryptionType);
    convert.field(pJson, "confirmPassword", stInfo.confirmPassword);
	
}

void Convert::deal(Json::Object* pJson, Network::Platform_Info_t &stInfo, bool bOutStruct)
{
    if (!pJson)
	{
    	return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pJson, "server_ip", stInfo.server_ip);
    convert.field(pJson, "server_port", stInfo.server_port);
    convert.field(pJson, "RtmpPort", stInfo.rtmp_port);
    convert.field(pJson, "MqttPort", stInfo.mqtt_port);
    convert.field(pJson, "user", stInfo.user);
    convert.field(pJson, "password", stInfo.password);
	convert.field(pJson, "enable", stInfo.enable);
	convert.field(pJson, "Custom", stInfo.Custom);
}

void Convert::deal(Json::Object* pJson, Network::Platform_Store_Info_t &stInfo, bool bOutStruct)
{
    if (!pJson)
	{
    	return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pJson, "access_token", stInfo.access_token);
    convert.field(pJson, "user", stInfo.user);
    convert.field(pJson, "password", stInfo.password);
}