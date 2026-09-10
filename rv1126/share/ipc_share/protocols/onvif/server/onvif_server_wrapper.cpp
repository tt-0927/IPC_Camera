/**
 * @FilePath     : onvif_server_wrapper.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-04-02 09:59:36
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 17:13:15
 * @Description  : onvif服务端调用接口封装
 */

#include "onvif_server_wrapper.h"
#include "onvif_server.h"
#include "rtsp_server.h"
#include "network_manage.h"
#include "time_manage.h"
#include "stream_video.h"
#include "system_define.h"
#include "isp_control.h"
#include "stream_audio.h"
#include "log_handler.h"
#include "user_manage.h"
#include "isp_configure.h"
#include "isp_manage.h"
#include "av_configure.h"
#include "event_configure.h"
#include "https_manage.h"

#include "onvif_convert.h"
#include "onvif_SubscriptionManager.hpp"
#include "event_resource.h"
#include "event_manage.h"

    // Helper to convert AlgorithmConfig to SmartEventEnableStatus for Resource Check
    static void helper_convert_to_status(const Event::AlgorithmConfig_S& algo, Event::SmartEventEnableStatus_S& status) {
        status.bLineCrossing = algo.nEnLineCrossing;
        status.bIntrusion = algo.nEnIntrusion;
        status.bEnterRegion = algo.nEnEnterRegion;
        status.bLeaveRegion = algo.nEnLeaveRegion;
        status.bLoiteringDetect = algo.nEnLoiteringDetect;
        status.bCrowdGathering = algo.nEnCrowdGathering;
        status.bParkingDetect = algo.nEnParkingDetect;
        status.bAudioAnomaly = algo.nEnAudioAnomaly;
        status.bSceneChange = algo.nEnSceneChange;
        status.bUnattendedObject = algo.nEnUnattendedObject;
        status.bObjectRemoval = algo.nEnObjectRemoval;
        status.bFaceDetect = algo.nEnFaceDetect;
        status.bPetRecognition = algo.nEnPetRecognition;
        status.bFaceCapture = algo.nEnFaceCapture;
        
        #ifdef SCENE_INTELLIGENCE
        status.bSleepOnDuty = algo.nEnSleepOnDuty;
        status.bLeavePost = algo.nEnLeavePost;
        status.bElectricVehicleInElevator = algo.nEnElectricVehicleInElevator;
        status.bPersonFallDown = algo.nEnPersonFallDown;
        status.bFenceClimbing = algo.nEnFenceClimbing;
        status.bTrip = algo.nEnTrip;
        status.bSmoking = algo.nEnSmoking;
        status.bPhoneUsage = algo.nEnPhoneUsage;
        status.bGarbageExposure = algo.nEnGarbageExposure;
        status.bSmokeFire = algo.nEnSmokeFire;
        status.bOpenFlame = algo.nEnOpenFlame;
        status.bGarbageOverflow = algo.nEnGarbageOverflow;
        status.bManholeCoverAbnormal = algo.nEnManholeCoverAbnormal;
        status.bBareSoil = algo.nEnBareSoil;
        status.bHoleProtectionBar = algo.nEnHoleProtectionBar;
        status.bPedestrianIntrusion = algo.nEnPedestrianIntrusion;
        status.bSafetyHelmet = algo.nEnSafetyHelmet;
        status.bReflectiveClothing = algo.nEnReflectiveClothing;
        status.bHighAltitudeSeatbelt = algo.nEnHighAltitudeSeatbelt;
        status.bConstructionOccupyRoad = algo.nEnConstructionOccupyRoad;
        status.bEmergencyLaneOccupancy = algo.nEnEmergencyLaneOccupancy;
        status.bReverseDirection = algo.nEnReverseDirection;
        status.bNonMotorVehicleIntrusion = algo.nEnNonMotorVehicleIntrusion;
        status.bRoadPonding = algo.nEnRoadPonding;
        status.bCongestion = algo.nEnCongestion;
        status.bIllegalParking = algo.nEnIllegalParking;
        status.bIllegalLaneChange = algo.nEnIllegalLaneChange;
        status.bPlateNumber = algo.nPlateNumber;
        #endif
    }

    static int check_analytics_resource(const Event::Type_E enable_type, const bool bEnable) {
        if (!bEnable) return 0; // Disabling is always allowed

        Event::AlgorithmConfig_S algoConfig;
        CEventConfigure::instance()->get_configure(algoConfig);
        
        // Check if already enabled (if so, no resource conflict for same event)
        bool already_enabled = false;
        switch (enable_type) {
            case Event::Type::LINE_CROSSING: already_enabled = algoConfig.nEnLineCrossing; break;
            case Event::Type::INTRUSION: already_enabled = algoConfig.nEnIntrusion; break;
            case Event::Type::ENTER_REGION: already_enabled = algoConfig.nEnEnterRegion; break;
            case Event::Type::LEAVE_REGION: already_enabled = algoConfig.nEnLeaveRegion; break;
            case Event::Type::LOITERING_DETECT: already_enabled = algoConfig.nEnLoiteringDetect; break;
            case Event::Type::CROWD_GATHERING: already_enabled = algoConfig.nEnCrowdGathering; break;
            case Event::Type::PARKING_DETECT: already_enabled = algoConfig.nEnParkingDetect; break;
            case Event::Type::AUDIO_ANOMALY: already_enabled = algoConfig.nEnAudioAnomaly; break;
            case Event::Type::SCENE_CHANGE: already_enabled = algoConfig.nEnSceneChange; break;
            case Event::Type::UNATTENDED_OBJECT: already_enabled = algoConfig.nEnUnattendedObject; break;
            case Event::Type::OBJECT_REMOVAL: already_enabled = algoConfig.nEnObjectRemoval; break;
            case Event::Type::FACE_DETECT: already_enabled = algoConfig.nEnFaceDetect; break;
            case Event::Type::PET_RECOGNITION: already_enabled = algoConfig.nEnPetRecognition; break;
            case Event::Type::FACE_CAPTURE: already_enabled = algoConfig.nEnFaceCapture; break;
            default: break; 
        }
        
        if (already_enabled) return 0;

        Event::SmartEventEnableStatus_S status;
        helper_convert_to_status(algoConfig, status);

        std::vector<Event::Type_E> aCanEnableEvent;
        CEventResource::instance()->get_canEventResource_rules(status, aCanEnableEvent);
        
        for (const auto& ev : aCanEnableEvent) {
            if (ev == enable_type) return 0;
        }
        
        dlog_error("check_analytics_resource failed for type: %d. Resource conflict.", (int)enable_type);
        return ONVIF_ERR_EVENT_RESOURCE_CONFLICT;
    }


    /* Arming Schedule Serialization Helpers */
    static void onvif_serialize_schedule(const std::vector<std::vector<Common::SchedTime_S>>& aAlarmTime, char* buffer, size_t len)
    {
        if (!buffer || len == 0) return;
        buffer[0] = '\0';
        
        std::string strXml = "<Schedule>";
        
        for (int i = 0; i < 7; ++i) {
            std::string strDay = "<Day index=\"" + std::to_string(i) + "\">";
            bool hasValidTime = false;

            if (i < (int)aAlarmTime.size() && !aAlarmTime[i].empty()) {
                for (const auto& time : aAlarmTime[i]) {
                    /* Skip valid 0 check if you want to allow 0-0, but assuming 0-0 means invalid/skip */
                    if (time.stStart.nHour == 0 && time.stStart.nMinute == 0 && time.stStart.nSecond == 0 &&
                        time.stStop.nHour == 0 && time.stStop.nMinute == 0 && time.stStop.nSecond == 0) {
                         continue;
                    }
                    
                    char szTime[128];
                    snprintf(szTime, sizeof(szTime), "<Time start=\"%02d:%02d:%02d\" end=\"%02d:%02d:%02d\"/>",
                        time.stStart.nHour, time.stStart.nMinute, time.stStart.nSecond,
                        time.stStop.nHour, time.stStop.nMinute, time.stStop.nSecond);
                    strDay += szTime;
                    hasValidTime = true;
                }
            }

            if (!hasValidTime) {
                strDay += "<Time start=\"00:00:00\" end=\"00:00:00\"/>";
            }

            strDay += "</Day>";
            strXml += strDay;
        }
        strXml += "</Schedule>";
        
        if (strXml.length() < len) {
            strcpy(buffer, strXml.c_str());
        } else {
            dlog_error("Schedule XML too long for buffer");
        }
    }

    static void onvif_deserialize_schedule(const char* buffer, std::vector<std::vector<Common::SchedTime_S>>& aAlarmTime)
    {
        if (!buffer) return;
        
        // Clear existing schedule
        for(auto& day : aAlarmTime) day.clear();
        if(aAlarmTime.size() < 7) aAlarmTime.resize(7);

        const char* pDay = strstr(buffer, "<Day");
        while (pDay) {
            int nDayIndex = -1;
            const char* pIndex = strstr(pDay, "index=\"");
            if (pIndex) {
               nDayIndex = atoi(pIndex + 7);
            }
            
            if (nDayIndex >= 0 && nDayIndex < 7) {
                const char* pTime = strstr(pDay, "<Time");
                // Limit search to within this Day tag
                const char* pDayEnd = strstr(pDay, "</Day>");
                
                while (pTime && (!pDayEnd || pTime < pDayEnd)) {
                    Common::SchedTime_S stTime;
                    
                    int sh, sm, ss, eh, em, es;
                    const char* pStart = strstr(pTime, "start=\"");
                    const char* pEnd = strstr(pTime, "end=\"");
                    
                    if (pStart && pEnd) {
                        sscanf(pStart + 7, "%d:%d:%d", &sh, &sm, &ss);
                        sscanf(pEnd + 5, "%d:%d:%d", &eh, &em, &es);
                        
                        stTime.stStart.nHour = sh; stTime.stStart.nMinute = sm; stTime.stStart.nSecond = ss;
                        stTime.stStop.nHour = eh; stTime.stStop.nMinute = em; stTime.stStop.nSecond = es;
                        
                        aAlarmTime[nDayIndex].push_back(stTime);
                    }
                    
                    pTime = strstr(pTime + 1, "<Time");
                }
            }
            
            pDay = strstr(pDay + 1, "<Day");
        }
    }

/* auth鉴权 */
int onvif_access_control(struct soap *soap)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("使用auth鉴权");
#endif
    const  char *username = soap_wsse_get_Username(soap);
#if ONVIF_LOG_SWITCH
    dlog_info("username:%s", username);
#endif
    if (username == NULL)
    {
        soap_wsse_delete_Security(soap);
#if ONVIF_LOG_SWITCH
    dlog_error("用户名不存在");
#endif
        return soap->error;  
    }



    if (onvif_vert_user(username) != 0)
    {
#if ONVIF_LOG_SWITCH
    dlog_error("用户名验证失败");
#endif
        soap_wsse_delete_Security(soap);
        return 401;
    }
	
    char *passwd = onvif_get_passwd((char *)username);
#if ONVIF_LOG_SWITCH
    dlog_info("获取到的密码:%s",passwd);
#endif
    if (soap_wsse_verify_Password(soap, passwd))
    {
#if ONVIF_LOG_SWITCH
    dlog_error("鉴权失败");
#endif
        soap_wsse_delete_Security(soap); 
        if(passwd != NULL)
        {
            free(passwd);
        }
        return soap->error;             
    }
#if ONVIF_LOG_SWITCH
    dlog_info("鉴权成功");
#endif
    if(passwd != NULL)
    {
        free(passwd);
    }
	//删除鉴权头
	soap_wsse_delete_Security(soap); 
    return SOAP_OK;
}

int onvif_http_digest_auth(struct soap *soap)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("使用digest鉴权");
#endif
    // +++ 关键修改：总是先设置好 authrealm +++
    // 这样 http_da 插件才能知道在哪个域下工作，并能正确处理 nonce。
    soap->authrealm = AUTHREALM;
    // dlog_info("Authrealm: %s, UserID: %s", soap->authrealm, soap->userid);
    // http_da 插件会在 soap_serve 内部自动解析头部并填充 soap->userid。
    if (soap->userid && *soap->userid) // 确保 userid 不为 null 且不为空
    {
#if ONVIF_LOG_SWITCH
        dlog_info("用户: %s, 正在验证...", soap->userid);
#endif
        if (onvif_vert_user((char *)soap->userid) == 0)
        {
            char *passwd = onvif_get_passwd((char *)soap->userid);
#if ONVIF_LOG_SWITCH
            dlog_info("获取密码：%s", passwd);
#endif
            if (passwd)
            {
                // http_da_verify_post 会使用 soap->userid 和 passwd 验证客户端发来的摘要
                if (http_da_verify_post(soap, passwd) == SOAP_OK)
                {
#if ONVIF_LOG_SWITCH
                    dlog_info("Digest 鉴权成功");
#endif
                    free(passwd);
                    return SOAP_OK; // 鉴权成功
                }
                free(passwd);
            }
            else
            {
                dlog_info("Digest 鉴权失败，密码为空");
            }
        }
    }
    // 如果代码执行到这里，意味着：
    // 1. 客户端首次请求，没有提供 userid。
    // 2. 客户端提供了 userid，但用户不存在或密码验证失败。
#if ONVIF_LOG_SWITCH
    dlog_warn("鉴权失败 (UserID: %s)", soap->userid ? soap->userid : "null");
#endif
    // 返回 401，gSOAP 会自动附上 WWW-Authenticate 头，因为我们已经设置了 soap->authrealm
    return 401; 
}


int onvif_authentication(struct soap *soap)
{
    char client_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(soap->ip), client_ip_str, INET_ADDRSTRLEN);
    #if ONVIF_LOG_SWITCH
        dlog_info("Authentication request from client IP: %s", client_ip_str);
    #endif

    int nRet = 0;
    int nMode = onvif_get_auth_mode();

    /* 选择鉴权方式 */ 
    if (nMode == ONVIF_AUTH_MODE) 
    {
        nRet = onvif_access_control(soap);
    } 
    else if (nMode == ONVIF_DIGEST_MODE) 
    {
        nRet = onvif_http_digest_auth(soap);
    } else 
    {
#if ONVIF_LOG_SWITCH
    dlog_error("不支持的鉴权模式: %d", nMode);
#endif
        return SOAP_EOF; 
    }

    return nRet;
}

int onvif_get_httpPort()
{
	return CHttpsManage::instance()->get_httpPort();
}


/* 设备支持的音频采样率列表 */
static const Audio_NS::AudioSamprate_E AUDIO_SAMPLERATE_MAP[AUDIO_SAMPRATE_LIST] =
{
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_8000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_11025,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_12000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_22050,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_24000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_32000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_44100,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_48000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_64000,
	Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_96000
};

/* 设备支持的音频码率列表 */
static const Audio_NS::AudioBitrate_E AUDIO_BITRATE_MAP[AUDIO_BITRATE_LIST] =
{
	Audio_NS::AudioBitrate_E::AUDIO_BITRATE_16K,
	Audio_NS::AudioBitrate_E::AUDIO_BITRATE_32K,
	Audio_NS::AudioBitrate_E::AUDIO_BITRATE_64K,
	Audio_NS::AudioBitrate_E::AUDIO_BITRATE_96K,
	Audio_NS::AudioBitrate_E::AUDIO_BITRATE_128K,
	Audio_NS::AudioBitrate_E::AUDIO_BITRATE_256K 
};

extern "C"
{
	int onvif_isValidIpv4(const char *str)
	{
		unsigned char buf[sizeof(struct in_addr)];
		return inet_pton(AF_INET, str, buf);
	}

	char *onvif_get_mac()
	{
		std::string strMac = CNetworkManage::instance()->get_macAddress(ETH0_INTERFACE);
		char* pMac = new char[strMac.size() + 1];
		strcpy(pMac, strMac.c_str());
		return pMac;
	}

	int onvif_get_auth_mode()
	{
		Network::OnvifConfigInfo_S stInfo;
    	Convert::read_file(ONVIF_CONFIG_FILE, stInfo);
		return stInfo.nOnvifAuthMode;
	}

	int onvif_vert_user(const char *pUser)
	{
		return CUserManage::instance()->verifi_user(pUser);
	}

	char *onvif_get_user()
	{
		std::vector<User::UserInfo_S> vecInfo;
		CUserManage::instance()->get_all_user(vecInfo);

		::System::SecurityServices_S stSecurityServicesInfo;
   		Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);

		char* cstr = new char[vecInfo[0].stAccountInfo.account.size() + 1];
		strcpy(cstr, vecInfo[0].stAccountInfo.account.c_str());
		// dlog_info("获取onvif用户：%s",vecInfo.at(0).account.c_str());
		return cstr;
	}

	char *onvif_get_passwd(char * pUser)
	{
		std::string str;
		str = CUserManage::instance()->get_passwd(pUser);
		char* cstr = new char[str.size() + 1];
		strcpy(cstr, str.c_str());
		// dlog_info("获取onvif用户：%s 密码:%s",pUser,cstr);
		return cstr;
	}

	void onvif_write_log(int nType, int nAction, const char *pclientIp)
	{
		Log::Info_S stLogInfo;
		stLogInfo.user = "onvif";
		stLogInfo.nType = nType;
		stLogInfo.nAction = nAction;
		if (pclientIp) 
		{
            stLogInfo.host = pclientIp;
        } else {
            stLogInfo.host = "unknown";
        }
		LogHandler::instance()->write(stLogInfo);
		dlog_debug("%s:%s", stLogInfo.user.c_str(), to_string((Log::Action_E)stLogInfo.nAction).c_str());
	}

	int onvif_reboot(struct soap *soap)
	{
		/* 从 soap 上下文中安全地获取客户端IP */ 
		char client_ip_str[INET_ADDRSTRLEN] = "unknown";
		if (soap) 
		{
			inet_ntop(AF_INET, &(soap->ip), client_ip_str, INET_ADDRSTRLEN);
		}

		onvif_write_log(Log::Type_E::OPERATION, Log::Action_E::REMOTE_REBOOT,client_ip_str);
		SystemManage::instance()->system_reboot([](int nRet) {});
		return OK;
	}

	int onvif_get_device_info(OnvifDeviceInfo_t *stInfo)
	{
		System::DeviceInfo_S stDeviceInfo;
		if (SystemManage::instance()->get_device_info(stDeviceInfo))
		{
			return ERR;
		}
        snprintf(stInfo->achManufacturer, sizeof(stInfo->achManufacturer), MANFACTURER);
        snprintf(stInfo->achModel, sizeof(stInfo->achModel), "%s", stDeviceInfo.strUnitTpye.c_str());
        snprintf(stInfo->achFirmwareVersion, sizeof(stInfo->achFirmwareVersion), "%s", stDeviceInfo.hardwareVersion.c_str());
        snprintf(stInfo->achSerialNumber, sizeof(stInfo->achSerialNumber), "%s", stDeviceInfo.serialNumber.c_str());
        snprintf(stInfo->achHardwareId, sizeof(stInfo->achHardwareId), "%d", stDeviceInfo.deviceID);
        return OK;
	}

	char *onvif_get_rtsp_url(int nChn)
	{
		if (nChn < 0)
		{
			dlog_error("nChn error");
			return NULL;
		}
		return CRtspServer::instance()->getRtspUrl(nChn);
	}
	/* 根据子网掩码获取子网掩码长度 */
	int getPrefixLength(const char *achSubnetMask) 
	{
		unsigned int octets[4] = {0};
		int nCount = 0;

		if (sscanf(achSubnetMask, "%u.%u.%u.%u", &octets[0], &octets[1], &octets[2], &octets[3]) != 4) 
		{
			dlog_info("无效子网掩码：%s",achSubnetMask);
			return -1;
		}
		for (int i = 0; i < 4; ++i) 
		{
			unsigned int octet = octets[i];
			for (int j = 7; j >= 0; --j) 
			{
				if ((octet >> j) & 1) 
				{
					nCount++;
				} 
				else 
				{

					if (octet & ((1 << j) - 1)) 
					{
						dlog_info("无效子网掩码：%s",achSubnetMask);
						return -1; 
					}
					break;
				}
			}
		}

		return nCount;
	}

	int getSubnetMaskByPrefix(int prefix_len, char* buffer, size_t buffer_size) 
	{
		// 1. 校验输入参数
		if (buffer == nullptr || buffer_size < 16) 
		{
			prefix_len = 24;
		}
		if (prefix_len < 1 || prefix_len > 32) 
		{
			prefix_len = 24;
		}

		unsigned char subnet_mask[4] = {0};
		int remaining_bits = prefix_len;

		for (int i = 0; i < 4 && remaining_bits > 0; ++i) {
			int bits_in_current_byte = std::min(remaining_bits, 8);
			subnet_mask[i] = (0xFF << (8 - bits_in_current_byte)) & 0xFF;
			remaining_bits -= bits_in_current_byte;
		}

		// 4. 格式化为字符串
		snprintf(buffer, buffer_size, "%d.%d.%d.%d",
				subnet_mask[0], subnet_mask[1], subnet_mask[2], subnet_mask[3]);

		return 0;
	}

	int onvif_set_ipAdress(ONvifNetworkInfo_S stOnvifInfo)
	{
		if(strlen(stOnvifInfo.achIPAddr) == 0)
		{		
			dlog_error("Ip is NULL, onvif_set_ipAdress failed");
			return -1;
		}

		int nRet = -1;
		char mask[16] = {0};
		Network::Info_S stNetInfo;

		CNetworkManage::instance()->get_system_networkInfo(stNetInfo);

		stNetInfo.stIp.bEnableDhcp = stOnvifInfo.bDhcp;
		/* 子网掩码获取 */
		getSubnetMaskByPrefix(stOnvifInfo.nPrefixlen, mask, sizeof(mask));
		
		stNetInfo.stIp.ipv4Ip = stOnvifInfo.achIPAddr;
		stNetInfo.stIp.ipv4Mask = mask;
		nRet = CNetworkManage::instance()->set_system_networkInfo(stNetInfo);
		return nRet;
	}
	int onvif_get_ipInfo(ONvifNetworkInfo_S *pstOnvifInfo)
	{
		if(pstOnvifInfo == NULL)
		{
			return -1;
		}
		Network::Info_S stNetInfo;

		CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
		/* 子网掩码长度 */
		int nPrefixlen = getPrefixLength(stNetInfo.stIp.ipv4Mask.c_str());

		pstOnvifInfo->nPrefixlen = nPrefixlen;
		pstOnvifInfo->nMtu = stNetInfo.stIp.nMtu;
		pstOnvifInfo->bDhcp = stNetInfo.stIp.bEnableDhcp;
		memcpy(pstOnvifInfo->achMAC, stNetInfo.stIp.physicalAddress.c_str(),
			std::min(sizeof(pstOnvifInfo->achMAC),
				strlen(stNetInfo.stIp.physicalAddress.c_str())));
		
		return 0;
	}

	int onvif_set_Gateway(char *strGateway)
	{
		if(!strGateway)
		{		
			dlog_error("strGateway is NULL, onvif_set_Gateway failed");
			return -1;
		}

		int nRet = -1;
		Network::Info_S stNetInfo;

		nRet = CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
		if (nRet != 0)
		{
			return -1;
		}

		if (stNetInfo.stIp.bEnableDhcp)
		{
			dlog_error("bEnableDhcp = %d, onvif_set_Gateway error", stNetInfo.stIp.bEnableDhcp);
			return -1;
		}

		stNetInfo.stIp.ipv4Gateway = strGateway;
		nRet = CNetworkManage::instance()->set_system_networkInfo(stNetInfo);
		return nRet;
	}

	int onvif_get_ntpServerAddress(char *ntpServerAddress, int nLen)
	{
		if(!ntpServerAddress)
		{
			dlog_error("ntpServerAddress is NULL, onvif_get_ntpServerAddress failed");
			return -1;
		}
		System::TimeInfo_S stTimeInfo;
		CTimeManage::instance()->get_time_info(stTimeInfo);
        nLen = ((size_t) nLen > strlen(stTimeInfo.stNTPInfo.address.c_str()))
                   ? strlen(stTimeInfo.stNTPInfo.address.c_str())
                   : nLen;
        memcpy(ntpServerAddress, stTimeInfo.stNTPInfo.address.c_str(), nLen);
		return 0;
	}

    int onvif_set_system_utc_time(time_t nUtcTime)
    {
        return CTimeManage::instance()->set_system_utc_time(nUtcTime, SystemTimeChangeSource_E::ONVIF);
    }

	int onvif_get_profileParam(OnvifProfile_t *pstProfile, int nStreamNum)
	{
		if(!pstProfile)
		{		
			dlog_error("pstProfile is NULL, onvif_get_profileParam failed");
			return -1;
		}

		/* 获取码流的分辨率大小 */
		std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
		CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

		pstProfile->nWidth = vstVideoConfig[nStreamNum].stVideoResolution.nWidth;
		pstProfile->nHeight = vstVideoConfig[nStreamNum].stVideoResolution.nHeight;
		pstProfile->BitrateLimit = vstVideoConfig[nStreamNum].nBitrateUpperLimit;
		pstProfile->FrameRateLimit = VIDEO_FRAMERATE_MAX; // vstVideoConfig[nStreamNum].nFrameRate;
		pstProfile->Quality = (int)((int)vstVideoConfig[nStreamNum].enImageQuality/20 + 1);
		pstProfile->IFrameInterval = (int)vstVideoConfig[nStreamNum].nIFrameInterval;
		if ((int)vstVideoConfig[nStreamNum].enBitrateType)
		{
			pstProfile->ConstantBitRate = false;
		}
		else
		{
			pstProfile->ConstantBitRate = true;
		}
        
        /* Video Type */
        pstProfile->nVideoType = (int)vstVideoConfig[nStreamNum].enVideoType;

		switch (vstVideoConfig[nStreamNum].enVideoCodec)
		{
		case Video_NS::VideoCodec_E::H264:
			memcpy(pstProfile->VideoEncoderConfiguration_Encoding, VIDEO_CODEC_H264, strlen(VIDEO_CODEC_H264));
			pstProfile->VideoEncoding = 2;
			break;
		case Video_NS::VideoCodec_E::H265:
			memcpy(pstProfile->VideoEncoderConfiguration_Encoding, VIDEO_CODEC_H265, strlen(VIDEO_CODEC_H265));
			pstProfile->VideoEncoding = 3;
			break;
		case Video_NS::VideoCodec_E::MJPEG:
			memcpy(pstProfile->VideoEncoderConfiguration_Encoding, VIDEO_CODEC_MJPEG, strlen(VIDEO_CODEC_MJPEG));
			pstProfile->VideoEncoding = 1;
			break;
		case Video_NS::VideoCodec_E::SVAC3:
			memcpy(pstProfile->VideoEncoderConfiguration_Encoding, VIDEO_CODEC_SVAC3, strlen(VIDEO_CODEC_SVAC3));
			pstProfile->VideoEncoding = 4;
			break;
		default:
			memcpy(pstProfile->VideoEncoderConfiguration_Encoding, "unknown codec", strlen("unknown codec"));
			pstProfile->VideoEncoding = -1;
			break;
		}

		onvif_get_audioParams(&pstProfile->stAudioParam);

		return 0;
	}

	int onvif_get_osdSize()
	{
		int nOsdSize = 0;
		std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
		COsdManage::instance()->get_overplay_info(vecOverplayInfo);
		for (size_t i = 0; i < vecOverplayInfo.size(); i++)
		{
			/* 类型为显示mac和人数的osd不允许用户修改 */
			if (vecOverplayInfo[i].stuOverplay.enElementType != Osd::ELEMENT_TYPE_MAC && vecOverplayInfo[i].stuOverplay.enElementType != Osd::ELEMENT_TYPE_PEOPLE)
			{
				nOsdSize++;
			}
		}
		return nOsdSize;
	}

	int onvif_convert_osd_pos(OnvifOsdCfg_t *pstOsdCfg)
	{
		if(!pstOsdCfg)
		{		
			dlog_error("pstOsdCfg is NULL, onvif_convert_osd_pos failed");
			return -1;
		}

		/* 转换为onvif协议的坐标 */
		switch (pstOsdCfg->eOsdAlign)
		{
		case E_OSDPOSTYPE_TOP_LEFT:
			pstOsdCfg->x = (float)(pstOsdCfg->nHorMargin / (pstOsdCfg->nReferenceWith * 0.5)) - 1.0;
			pstOsdCfg->y = 1.0 - (float)(pstOsdCfg->nVerMargin / (pstOsdCfg->nReferenceHeight * 0.5));
			break;
		case E_OSDPOSTYPE_BOTTOM_LEFT:
			pstOsdCfg->x = (float)(pstOsdCfg->nHorMargin / (pstOsdCfg->nReferenceWith * 0.5) - 1.0);
			pstOsdCfg->y = (float)(pstOsdCfg->nVerMargin / (pstOsdCfg->nReferenceHeight * 0.5) - 1.0);
			break;
		case E_OSDPOSTYPE_TOP_RIGHT:
			pstOsdCfg->x = 1.0 - (float)(pstOsdCfg->nHorMargin / (pstOsdCfg->nReferenceWith * 0.5));
			pstOsdCfg->y = 1.0 - (float)(pstOsdCfg->nVerMargin / (pstOsdCfg->nReferenceHeight * 0.5));
			break;
		case E_OSDPOSTYPE_BOTTOM_RIGHT:
			pstOsdCfg->x = 1.0 - (float)(pstOsdCfg->nHorMargin / (pstOsdCfg->nReferenceWith * 0.5));
			pstOsdCfg->y = (float)(pstOsdCfg->nVerMargin / (pstOsdCfg->nReferenceHeight * 0.5) - 1.0);
			break;
		case E_OSDPOSTYPE_CENTER:
			// pstOsdCfg->x = (int)((*(trt__SetOSD->OSD->Position->Pos->x)) * stOnvifOsdCfgs[nIndex].nReferenceWith / 2);
			// pstOsdCfg->y = (int)((*(trt__SetOSD->OSD->Position->Pos->y)) * stOnvifOsdCfgs[nIndex].nReferenceHeight / 2);
			break;
		default:
			break;
		}

		return 0;
	}

	int onvif_get_osdParam(OnvifOsdCfg_t *pstOsdCfg, int *pnSzie)
	{
		int nCount = 0;
		if(!pstOsdCfg)
		{		
			dlog_error("pstOsdCfg is NULL, onvif_get_osdParam falied");
			return -1;
		}

		Osd::OsdConfig_S stInfo;
		COsdManage::instance()->get_osd_config(stInfo);
		stInfo.init_token();
		/* OSD名称 */
		if(stInfo.stOsdNameInfo.bEnable)
		{
			pstOsdCfg[nCount].bOsdEnable = true;
			pstOsdCfg[nCount].eOsdType = E_OSDTYPE_TEXT;
			pstOsdCfg[nCount].eTextType = E_OSDTYPE_TEXT_NAME;
			memcpy(pstOsdCfg[nCount].TextString_Type, "Plain", strlen("Plain"));
			memcpy(pstOsdCfg[nCount].TextString_PlainText, stInfo.stOsdNameInfo.strName.c_str(), sizeof(pstOsdCfg[nCount].TextString_PlainText));
			memcpy(pstOsdCfg[nCount].Position_Type, ONVIF_TT_POSITION_CUSTOM, strlen(ONVIF_TT_POSITION_CUSTOM));
			if(stInfo.stOsdNameInfo.stOsdAttr.strToken.empty())
			{
				stInfo.stOsdNameInfo.stOsdAttr.strToken = "OsdToken_name";
			}
			snprintf(pstOsdCfg[nCount].token, sizeof(pstOsdCfg[nCount].token), "%s", stInfo.stOsdNameInfo.stOsdAttr.strToken.c_str());
			CommomOsdPos_S stOsdPos;
			stOsdPos.x = stInfo.stOsdNameInfo.stOsdAttr.nX;
			stOsdPos.y = stInfo.stOsdNameInfo.stOsdAttr.nY;
			/* 坐标转换 */
			convert_osd_pos(&pstOsdCfg[nCount].stONvifPos,&stOsdPos,false);

			nCount++;
		}

		/* OSD时间 */
		if(stInfo.stOsdTimeInfo.bEnable)
		{
			pstOsdCfg[nCount].bOsdEnable = true;
			pstOsdCfg[nCount].eOsdType = E_OSDTYPE_TEXT;
			pstOsdCfg[nCount].eTextType = E_OSDTYPE_TEXT_TIME;
			if (stInfo.stOsdTimeInfo.enDateFormat == Osd::OSD_DATE_FORMAT_E::ENGLISH_DDMMYYYY)
			{
				memcpy(pstOsdCfg[nCount].TextString_DateFormat, "dd/MM/yyyy", strlen("dd/MM/yyyy"));
			}
			else if (stInfo.stOsdTimeInfo.enDateFormat == Osd::OSD_DATE_FORMAT_E::ENGLISH_MMDDYYYY)
			{
				memcpy(pstOsdCfg[nCount].TextString_DateFormat, "MM/dd/yyyy", strlen("MM/dd/yyyy"));
			}
			else if (stInfo.stOsdTimeInfo.enDateFormat == Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYY_MM_DD)
			{
				memcpy(pstOsdCfg[nCount].TextString_DateFormat, "yyyy-MM-dd", strlen("yyyy-MM-dd"));
			}
			else
			{
				memcpy(pstOsdCfg[nCount].TextString_DateFormat, "yyyy/MM/dd", strlen("yyyy/MM/dd"));
			}

			if (stInfo.stOsdTimeInfo.enTimeFormat == Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_12)
			{
				memcpy(pstOsdCfg[nCount].TextString_TimeFormat, "hh:mm:ss tt", strlen("hh:mm:ss tt"));
			}
			else
			{
				memcpy(pstOsdCfg[nCount].TextString_TimeFormat, "HH:mm:ss", strlen("HH:mm:ss"));
			}

			if(stInfo.stOsdTimeInfo.stOsdAttr.strToken.empty())
			{
				stInfo.stOsdTimeInfo.stOsdAttr.strToken = "OsdToken_time";
			}
			memcpy(pstOsdCfg[nCount].TextString_Type, "DateAndTime", strlen("DateAndTime"));
			memcpy(pstOsdCfg[nCount].Position_Type, ONVIF_TT_POSITION_CUSTOM, strlen(ONVIF_TT_POSITION_CUSTOM));
			snprintf(pstOsdCfg[nCount].token, sizeof(pstOsdCfg[nCount].token), "%s", stInfo.stOsdTimeInfo.stOsdAttr.strToken.c_str());
			CommomOsdPos_S stOsdPos;
			stOsdPos.x = stInfo.stOsdTimeInfo.stOsdAttr.nX;
			stOsdPos.y = stInfo.stOsdTimeInfo.stOsdAttr.nY;
			convert_osd_pos(&pstOsdCfg[nCount].stONvifPos,&stOsdPos,false);
			nCount++;
		}

		for (size_t i = 0; i < stInfo.vecOsdInfo.size(); i++)
		{
			if(stInfo.vecOsdInfo[i].bEnable)
			{
				pstOsdCfg[nCount].bOsdEnable = true;
				pstOsdCfg[nCount].eOsdType = E_OSDTYPE_TEXT;
				pstOsdCfg[nCount].eTextType = E_OSDTYPE_TEXT_EXTEND;
				memcpy(pstOsdCfg[nCount].TextString_Type, "Plain", strlen("Plain"));
				memcpy(pstOsdCfg[nCount].TextString_PlainText, stInfo.vecOsdInfo[i].strName.c_str(), sizeof(pstOsdCfg[nCount].TextString_PlainText));
				memcpy(pstOsdCfg[nCount].Position_Type, ONVIF_TT_POSITION_CUSTOM, strlen(ONVIF_TT_POSITION_CUSTOM));
				if(stInfo.vecOsdInfo[i].stOsdAttr.strToken.empty())
				{
					stInfo.vecOsdInfo[i].stOsdAttr.strToken = "OsdToken_" + std::to_string(stInfo.vecOsdInfo[i].nId);
				}
				snprintf(pstOsdCfg[nCount].token, sizeof(pstOsdCfg[nCount].token), "%s", stInfo.vecOsdInfo[i].stOsdAttr.strToken.c_str());
				CommomOsdPos_S stOsdPos;
				stOsdPos.x = stInfo.vecOsdInfo[i].stOsdAttr.nX;
				stOsdPos.y = stInfo.vecOsdInfo[i].stOsdAttr.nY;
				/* 坐标转换 */
				convert_osd_pos(&pstOsdCfg[nCount].stONvifPos,&stOsdPos,false);

				nCount++;
			}
		}

		*pnSzie = nCount; 
#if 0
		/* 获取osd信息 */
		std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
		COsdManage::instance()->get_overplay_info(vecOverplayInfo);
		uint32_t unFontColor;
		/* 判断是否存在osd信息 */
		if (!vecOverplayInfo.size())
		{
			return ERR_PARAM_NULL;
		}

		System::DeviceConfig_S stDeviceConfig;
		SystemManage::instance()->get_device_config(stDeviceConfig);
		int nIndex = 0;
		for (int i = 0; i < vecOverplayInfo.size(); i++)
		{
			/* 类型为显示mac和人数的osd不允许用户修改 */
			if (vecOverplayInfo[i].stuOverplay.enElementType != Osd::ELEMENT_TYPE_MAC && vecOverplayInfo[i].stuOverplay.enElementType != Osd::ELEMENT_TYPE_PEOPLE)
			{
				if (nIndex >= nSzie)
				{
					break;
				}
				if(!vecOverplayInfo[i].stuInfo.bEnable || vecOverplayInfo[i].stuOverplay.strCustomize.empty())
				{
					continue;
				}
				pstOsdCfg[nIndex].bOsdEnable = vecOverplayInfo[i].stuInfo.bEnable;

				snprintf(pstOsdCfg[nIndex].token, sizeof(pstOsdCfg[nIndex].token), "osd%d_%s", vecOverplayInfo[i].stuInfo.nID, vecOverplayInfo[i].stuInfo.strName.c_str());

				pstOsdCfg[nIndex].eOsdType = E_OSDTYPE_TEXT;

				pstOsdCfg[nIndex].TextString_FontSize = vecOverplayInfo[i].stuOverplay.nFontSize;
				/* 字体颜色分量 */
				unFontColor = std::stoul(vecOverplayInfo[i].stuOverplay.strFontColor, NULL, 16); /* 16 表示十六进制 */
				pstOsdCfg[nIndex].TextString_Font_R = (unFontColor >> 16) & 0xFF;
				pstOsdCfg[nIndex].TextString_Font_G = (unFontColor >> 8) & 0xFF;
				pstOsdCfg[nIndex].TextString_Font_B = unFontColor & 0xFF;
				pstOsdCfg[nIndex].TextString_FontAlpha = vecOverplayInfo[i].stuOverplay.nFontAlpha;
				/* 背景颜色分量 */
				unFontColor = std::stoul(vecOverplayInfo[i].stuOverplay.strBackColor, NULL, 16); /* 16 表示十六进制 */
				pstOsdCfg[nIndex].TextString_Background_R = (unFontColor >> 16) & 0xFF;
				pstOsdCfg[nIndex].TextString_Background_G = (unFontColor >> 8) & 0xFF;
				pstOsdCfg[nIndex].TextString_Background_B = unFontColor & 0xFF;
				pstOsdCfg[nIndex].TextString_BackgroundAlpha = vecOverplayInfo[i].stuOverplay.nBackAlpha;

				memcpy(pstOsdCfg[nIndex].TextString_PlainText, vecOverplayInfo[i].stuOverplay.strCustomize.c_str(), sizeof(pstOsdCfg[nIndex].TextString_PlainText));
				pstOsdCfg[nIndex].eOsdAlign = (OsdPostionType_e)vecOverplayInfo[i].stuOverplay.enAlign;

				/* 转换为onvif的osd描述字段 */
				switch (vecOverplayInfo[i].stuOverplay.enAlign)
				{
				case Osd::ALIGN_TOP_LEFT:
					memcpy(pstOsdCfg[nIndex].Position_Type, ONVIF_TT_POSITION_UPPER_LEFT, strlen(ONVIF_TT_POSITION_UPPER_LEFT));
					break;
				case Osd::ALIGN_BOTTOM_LEFT:
					memcpy(pstOsdCfg[nIndex].Position_Type, ONVIF_TT_POSITION_LOWER_LEFT, strlen(ONVIF_TT_POSITION_LOWER_LEFT));
					break;
				case Osd::ALIGN_TOP_RIGHT:
					memcpy(pstOsdCfg[nIndex].Position_Type, ONVIF_TT_POSITION_UPPER_RIGHT, strlen(ONVIF_TT_POSITION_UPPER_RIGHT));
					break;
				case Osd::ALIGN_BOTTOM_RIGHT:
					memcpy(pstOsdCfg[nIndex].Position_Type, ONVIF_TT_POSITION_LOWER_RIGHT, strlen(ONVIF_TT_POSITION_LOWER_RIGHT));
					break;
				default:
					memcpy(pstOsdCfg[nIndex].Position_Type, ONVIF_TT_POSITION_CUSTOM, strlen(ONVIF_TT_POSITION_CUSTOM));
					break;
				}

				
				if (vecOverplayInfo[i].stuOverplay.enElementType == Osd::ELEMENT_TYPE_TIME) // 显示时间的osd类型，转换为onvif标准字段
				{

					memcpy(pstOsdCfg[nIndex].TextString_Type, "DateAndTime", strlen("DateAndTime"));

					if(pstOsdCfg[nIndex].bOsdEnable)
					{
						if (stDeviceConfig.enDateFormat == System::DateFormat_E::DDMMYYYY)
						{
							memcpy(pstOsdCfg[nIndex].TextString_DateFormat, "dd/MM/yyyy", strlen("dd/MM/yyyy"));
						}
						else if (stDeviceConfig.enDateFormat == System::DateFormat_E::MMDDYYYY)
						{
							memcpy(pstOsdCfg[nIndex].TextString_DateFormat, "MM/dd/yyyy", strlen("MM/dd/yyyy"));
						}
						else
						{
							memcpy(pstOsdCfg[nIndex].TextString_DateFormat, "yyyy-MM-dd", strlen("yyyy-MM-dd"));
						}

						if (vecOverplayInfo[i].stuOverplay.bEnablePeriod)
						{
							memcpy(pstOsdCfg[nIndex].TextString_TimeFormat, "hh:mm:ss tt", strlen("hh:mm:ss tt"));
						}
						else
						{
							memcpy(pstOsdCfg[nIndex].TextString_TimeFormat, "HH:mm:ss", strlen("HH:mm:ss"));
						}
					} 
				}
				else
				{
					if (vecOverplayInfo[i].stuInfo.bEnable)
					{
						memcpy(pstOsdCfg[nIndex].TextString_Type, "Plain", strlen("Plain"));
					}
				}

				switch (vecOverplayInfo[i].stuInfo.enRefSize) /* 绘画osd的参考尺寸 */
				{
				case Osd::REFERENCE_SIZE_480P:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_640;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_480;
					break;
				case Osd::REFERENCE_SIZE_576P:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_1024;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_576;
					break;
				case Osd::REFERENCE_SIZE_720P:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_1280;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_720;
					break;
				case Osd::REFERENCE_SIZE_1080P:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_1920;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_1080;
					break;
				case Osd::REFERENCE_SIZE_2K:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_2K;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_2K;
					break;
				case Osd::REFERENCE_SIZE_2_5K:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_2_5K;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_2_5K;
					break;
				default:
					pstOsdCfg[nIndex].nReferenceWith = PIXEL_WIDTH_2K;
					pstOsdCfg[nIndex].nReferenceHeight = PIXEL_HEIGHT_2K;
					break;
				}

				/* osd边距 */
				pstOsdCfg[nIndex].nHorMargin = vecOverplayInfo[i].stuOverplay.nHorMargin;
				pstOsdCfg[nIndex].nVerMargin = vecOverplayInfo[i].stuOverplay.nVerMargin;

				/* 转换为适配onvif的坐标 */
				onvif_convert_osd_pos(&pstOsdCfg[nIndex]);

				nIndex++;
			}
		}
#endif
		return 0;
	}

	int onvif_set_osdParam(OnvifOsdCfg_t *pstOsdCfg,char *pToken)
	{
		if(!pstOsdCfg || !pToken)
		{		
			dlog_error("pstOsdCfg is NULL, onvif_set_osdParam falied");
			return -1;
		}

		Osd::OsdConfig_S stInfo;
		COsdManage::instance()->get_osd_config(stInfo);
		stInfo.init_token();
		bool bFound = false;
		if(stInfo.stOsdNameInfo.stOsdAttr.strToken == pToken)
		{
			bFound = true;
			dlog_debug("设置token [%s] 通道名称",pToken);
			stInfo.stOsdNameInfo.strName = pstOsdCfg->TextString_PlainText;
			CommomOsdPos_S stOsdPos;
			/* 坐标转换 */
			convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
			stInfo.stOsdNameInfo.stOsdAttr.nX = stOsdPos.x;
			stInfo.stOsdNameInfo.stOsdAttr.nY = stOsdPos.y;
			stInfo.stOsdNameInfo.stOsdAttr.nW = -1;
			stInfo.stOsdNameInfo.stOsdAttr.nH = -1;
			stInfo.stOsdNameInfo.stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;
		}
		else if(stInfo.stOsdTimeInfo.stOsdAttr.strToken == pToken)
		{
			bFound = true;
			dlog_debug("设置token [%s] 通道时间",pToken);
			if (strcmp(pstOsdCfg->TextString_DateFormat, "dd/MM/yyyy") == 0)
			{
				stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_DDMMYYYY;
			}
			else if (strcmp(pstOsdCfg->TextString_DateFormat, "MM/dd/yyyy") == 0)
			{
				stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_MMDDYYYY;
			}
			else if (strcmp(pstOsdCfg->TextString_DateFormat, "yyyy-MM-dd") == 0)
			{
				stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYY_MM_DD;
			}
			else
			{
				stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYYMMDD;
			}

			if (strcmp(pstOsdCfg->TextString_TimeFormat,  "hh:mm:ss tt") == 0)
			{
				stInfo.stOsdTimeInfo.enTimeFormat = Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_12;
			}
			else
			{
				stInfo.stOsdTimeInfo.enTimeFormat = Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_24;
			}

			CommomOsdPos_S stOsdPos;
			/* 坐标转换 */
			convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
			stInfo.stOsdTimeInfo.stOsdAttr.nX = stOsdPos.x;
			stInfo.stOsdTimeInfo.stOsdAttr.nY = stOsdPos.y;
			stInfo.stOsdTimeInfo.stOsdAttr.nW = -1;
			stInfo.stOsdTimeInfo.stOsdAttr.nH = -1;
			stInfo.stOsdTimeInfo.stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;
		}
		else
		{
			for (size_t i = 0; i < stInfo.vecOsdInfo.size(); i++)
			{
				if(stInfo.vecOsdInfo[i].stOsdAttr.strToken == pToken)
				{
					bFound = true;
					dlog_debug("设置token [%s] 第【%d】个叠加OSD",pToken,i);
					stInfo.vecOsdInfo[i].strName = pstOsdCfg->TextString_PlainText;
					CommomOsdPos_S stOsdPos;
					/* 坐标转换 */
					convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
					stInfo.vecOsdInfo[i].stOsdAttr.nX = stOsdPos.x;
					stInfo.vecOsdInfo[i].stOsdAttr.nY = stOsdPos.y;
					stInfo.vecOsdInfo[i].stOsdAttr.nW = -1;
					stInfo.vecOsdInfo[i].stOsdAttr.nH = -1;
					stInfo.vecOsdInfo[i].stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;
					break;
				}
			}
		}
		
		if(bFound)
		{
			return COsdManage::instance()->set_osd_config(stInfo);
		}
		
		dlog_error("设置osd失败 没有找到对应的token[%s]",pToken);
		return -1;
#if 0
		int nIndex = -1;
		/* 获取osd信息 */
		std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
		COsdManage::instance()->get_overplay_info(vecOverplayInfo);
		uint32_t unFontColor;
		char strToken[32] = {0};
		/* 判断是否存在osd信息 */
		if (!vecOverplayInfo.size())
		{
			return ERR_PARAM_NULL;
		}

		System::DeviceConfig_S stDeviceConfig;
		SystemManage::instance()->get_device_config(stDeviceConfig);

		/* 生成各个osd的唯一token */
		for (int i = 0; i < vecOverplayInfo.size(); i++)
		{
			memset(strToken, 0, sizeof(strToken));
			snprintf(strToken, sizeof(strToken), "osd%d_%s", vecOverplayInfo[i].stuInfo.nID, vecOverplayInfo[i].stuInfo.strName.c_str());
			if (strcmp(strToken, pstOsdCfg->token) == 0)
			{
				nIndex = i;
				break;
			}
		}
		if (nIndex < 0)
		{
			return -1;
		}

		/* 设置日期格式 */
		if (pstOsdCfg->TextString_DateFormat[0] != '\0')
		{
			if (strcmp(pstOsdCfg->TextString_DateFormat, "dd/MM/yyyy") == 0)
			{
				stDeviceConfig.enDateFormat = System::DateFormat_E::DD_MM_YYYY;
			}
			else if (strcmp(pstOsdCfg->TextString_DateFormat, "MM/dd/yyyy") == 0)
			{
				stDeviceConfig.enDateFormat = System::DateFormat_E::MM_DD_YYYY;
			}
			else
			{
				stDeviceConfig.enDateFormat = System::DateFormat_E::YYYY_MM_DD;
			}
		}

		/* 设置时间格式 */
		if (pstOsdCfg->TextString_TimeFormat[0] != '\0')
		{
			if (strcmp(pstOsdCfg->TextString_TimeFormat, "hh:mm:ss tt") == 0)
			{
				vecOverplayInfo[nIndex].stuOverplay.bEnablePeriod = true;
			}
			else
			{
				vecOverplayInfo[nIndex].stuOverplay.bEnablePeriod = false;
			}
		}

		vecOverplayInfo[nIndex].stuOverplay.nHorMargin = pstOsdCfg->nHorMargin;
		vecOverplayInfo[nIndex].stuOverplay.nVerMargin = pstOsdCfg->nVerMargin;

		if (pstOsdCfg->TextString_PlainText[0] != '\0')
		{
			vecOverplayInfo[nIndex].stuOverplay.strCustomize = pstOsdCfg->TextString_PlainText;
		}

		vecOverplayInfo[nIndex].stuInfo.bEnable = pstOsdCfg->bOsdEnable;

		COsdManage::instance()->set_overplay_info(vecOverplayInfo);
		SystemManage::instance()->set_device_config(stDeviceConfig);
#endif
		return 0;
	}

	int onvif_create_osd(OnvifOsdCfg_t *pstOsdCfg,char *pToken)
	{
		if(!pToken || !pstOsdCfg)
		{		
			dlog_error("pToken is NULL, onvif_delete_osd falied");
			return -1;
		}

		Osd::OsdConfig_S stInfo;
		COsdManage::instance()->get_osd_config(stInfo);
		stInfo.init_token();
		bool bFound = false;
		if(stInfo.stOsdNameInfo.stOsdAttr.strToken == pToken && stInfo.stOsdNameInfo.bEnable)
		{
			bFound = true;
		}
		else if(stInfo.stOsdTimeInfo.stOsdAttr.strToken == pToken && stInfo.stOsdTimeInfo.bEnable)
		{
			bFound = true;
		}
		else
		{
			for (size_t i = 0; i < stInfo.vecOsdInfo.size(); i++)
			{
				if(stInfo.vecOsdInfo[i].stOsdAttr.strToken == pToken && stInfo.vecOsdInfo[i].bEnable)
				{
					bFound = true;
					break;
				}
			}
		}
		
		/* 存在toke返回失败 */
		if(bFound)
		{
			dlog_error("token [%s] 已经存在",pToken);
			return -1;
		}
		/* 创建新的OSD token */
		else
		{
			if(pstOsdCfg->eTextType == E_OSDTYPE_TEXT_TIME)
			{
				if(stInfo.stOsdTimeInfo.bEnable)
				{
					return -1;
				}

				stInfo.stOsdTimeInfo.bEnable = true;

				dlog_debug("创建token [%s] 通道时间",stInfo.stOsdTimeInfo.stOsdAttr.strToken.c_str());
				snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.stOsdTimeInfo.stOsdAttr.strToken.c_str());
				if (strcmp(pstOsdCfg->TextString_DateFormat, "dd/MM/yyyy") == 0)
				{
					stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_DDMMYYYY;
				}
				else if (strcmp(pstOsdCfg->TextString_DateFormat, "MM/dd/yyyy") == 0)
				{
					stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_MMDDYYYY;
				}
				else if (strcmp(pstOsdCfg->TextString_DateFormat, "yyyy-MM-dd") == 0)
				{
					stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYY_MM_DD;
				}
				else
				{
					stInfo.stOsdTimeInfo.enDateFormat = Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYYMMDD;
				}

				if (strcmp(pstOsdCfg->TextString_TimeFormat,  "hh:mm:ss tt") == 0)
				{
					stInfo.stOsdTimeInfo.enTimeFormat = Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_12;
				}
				else
				{
					stInfo.stOsdTimeInfo.enTimeFormat = Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_24;
				}
				
				CommomOsdPos_S stOsdPos;
				/* 坐标转换 */
				convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
				stInfo.stOsdTimeInfo.stOsdAttr.nX = stOsdPos.x;
				stInfo.stOsdTimeInfo.stOsdAttr.nY = stOsdPos.y;
				stInfo.stOsdTimeInfo.stOsdAttr.nW = -1;
				stInfo.stOsdTimeInfo.stOsdAttr.nH = -1;
				stInfo.stOsdTimeInfo.stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;

				return COsdManage::instance()->set_osd_config(stInfo);
			}	
			else if(pstOsdCfg->eTextType == E_OSDTYPE_TEXT_NAME)
			{
				if(stInfo.stOsdNameInfo.bEnable)
				{
					return -1;
				}
				dlog_debug("创建token [%s] 通道名称",stInfo.stOsdNameInfo.stOsdAttr.strToken.c_str());
				stInfo.stOsdNameInfo.bEnable = true;
				snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.stOsdNameInfo.stOsdAttr.strToken.c_str());

				stInfo.stOsdNameInfo.strName = pstOsdCfg->TextString_PlainText;
				CommomOsdPos_S stOsdPos;
				/* 坐标转换 */
				convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
				stInfo.stOsdNameInfo.stOsdAttr.nX = stOsdPos.x;
				stInfo.stOsdNameInfo.stOsdAttr.nY = stOsdPos.y;
				stInfo.stOsdNameInfo.stOsdAttr.nW = -1;
				stInfo.stOsdNameInfo.stOsdAttr.nH = -1;
				stInfo.stOsdNameInfo.stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;

				return COsdManage::instance()->set_osd_config(stInfo);
			}
			else
			{
				/* 若通道名称未启用 默认给通道名称 */
				if (false)/*if(!stInfo.stOsdNameInfo.bEnable)*/
				{
					dlog_debug("创建token [%s] 通道名称",pToken);

					stInfo.stOsdNameInfo.bEnable = true;
					
					snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.stOsdNameInfo.stOsdAttr.strToken.c_str());
					
					stInfo.stOsdNameInfo.strName = pstOsdCfg->TextString_PlainText;
					CommomOsdPos_S stOsdPos;
					/* 坐标转换 */
					convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
					stInfo.stOsdNameInfo.stOsdAttr.nX = stOsdPos.x;
					stInfo.stOsdNameInfo.stOsdAttr.nY = stOsdPos.y;

					return COsdManage::instance()->set_osd_config(stInfo);
				}
				else
				{
					for (size_t i = 0; i < stInfo.vecOsdInfo.size(); i++)
					{
						if(stInfo.vecOsdInfo[i].bEnable)
						{
							continue;
						}
						
						dlog_debug("创建token [%s] 第【%d】OSD叠加",pToken,i);
						
						if(strlen(pToken) != 0)
						{
							if(stInfo.vecOsdInfo[i].stOsdAttr.strToken == pToken)
							{
								if(i == stInfo.vecOsdInfo.size() - 1)
								{
									snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.vecOsdInfo[i].stOsdAttr.strToken.c_str());
								}
								else
								{
									snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.vecOsdInfo[i + 1].stOsdAttr.strToken.c_str());
								}

								stInfo.vecOsdInfo[i].bEnable = true;
						
								stInfo.vecOsdInfo[i].strName = pstOsdCfg->TextString_PlainText;
								CommomOsdPos_S stOsdPos;
								/* 坐标转换 */
								convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
								stInfo.vecOsdInfo[i].stOsdAttr.nX = stOsdPos.x;
								stInfo.vecOsdInfo[i].stOsdAttr.nY = stOsdPos.y;
								stInfo.vecOsdInfo[i].stOsdAttr.nW = -1;
								stInfo.vecOsdInfo[i].stOsdAttr.nH = -1;
								stInfo.vecOsdInfo[i].stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;

								return COsdManage::instance()->set_osd_config(stInfo);
							}
							else
							{
								continue;
							}
						}
						/* 返回下一个叠加的token */
						else if(strlen(pToken) == 0)
						{
							if(i == stInfo.vecOsdInfo.size() - 1)
							{
								snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.vecOsdInfo[i].stOsdAttr.strToken.c_str());
							}
							else
							{
								snprintf(pstOsdCfg->token, sizeof(pstOsdCfg->token), "%s", stInfo.vecOsdInfo[i + 1].stOsdAttr.strToken.c_str());
							}

							stInfo.vecOsdInfo[i].bEnable = true;
						
							stInfo.vecOsdInfo[i].strName = pstOsdCfg->TextString_PlainText;
							CommomOsdPos_S stOsdPos;
							/* 坐标转换 */
							convert_osd_pos(&pstOsdCfg->stONvifPos,&stOsdPos,true);
							stInfo.vecOsdInfo[i].stOsdAttr.nX = stOsdPos.x;
							stInfo.vecOsdInfo[i].stOsdAttr.nY = stOsdPos.y;
							stInfo.vecOsdInfo[i].stOsdAttr.nW = -1;
							stInfo.vecOsdInfo[i].stOsdAttr.nH = -1;
							stInfo.vecOsdInfo[i].stOsdAttr.enFontSize = Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE;

							return COsdManage::instance()->set_osd_config(stInfo);
						}
					}
				}
			}
		}
		
		dlog_error("token [%s] 未找到对应类型或者osd叠加已达最大数量",pToken);
		return -1;
	}

	int onvif_delete_osd(char *strOsdToken)
	{
		if(!strOsdToken)
		{		
			dlog_error("strOsdToken is NULL, onvif_delete_osd falied");
			return -1;
		}

		Osd::OsdConfig_S stInfo;
		COsdManage::instance()->get_osd_config(stInfo);
		stInfo.init_token();
		bool bFound = false;
		if(stInfo.stOsdNameInfo.stOsdAttr.strToken == strOsdToken)
		{
			dlog_debug("删除token [%s] 通道名称",strOsdToken);
			stInfo.stOsdNameInfo.bEnable = false;
			bFound = true;
		}
		else if(stInfo.stOsdTimeInfo.stOsdAttr.strToken == strOsdToken)
		{
			dlog_debug("删除token [%s] 通道时间",strOsdToken);
			stInfo.stOsdTimeInfo.bEnable = false;
			bFound = true;
		}
		else
		{
			for (size_t i = 0; i < stInfo.vecOsdInfo.size(); i++)
			{
				if(stInfo.vecOsdInfo[i].stOsdAttr.strToken == strOsdToken)
				{
					dlog_debug("删除token [%s] 第【%d】个OSD叠加",strOsdToken,i );
					stInfo.vecOsdInfo[i].bEnable = false;
					bFound = true;
					break;
				}
			}
		}
		
		if(bFound)
		{
			return COsdManage::instance()->set_osd_config(stInfo);
		}
		
		return -1;
	#if 0
		int nIndex = -1;
		/* 获取osd信息 */
		std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
		COsdManage::instance()->get_overplay_info(vecOverplayInfo);
		char strToken[32] = {0};

		for (int i = 0; i < vecOverplayInfo.size(); i++)
		{
			memset(strToken, 0, sizeof(strToken));
			snprintf(strToken, sizeof(strToken), "osd%d_%s", vecOverplayInfo[i].stuInfo.nID, vecOverplayInfo[i].stuInfo.strName.c_str());
			if (strcmp(strToken, strOsdToken) == 0)
			{
				nIndex = i;
				break;
			}
		}
		if (nIndex < 0)
		{
			return -1;
		}
		vecOverplayInfo[nIndex].stuInfo.bEnable = false;
		COsdManage::instance()->set_overplay_info(vecOverplayInfo);
	#endif
		return 0;
	}

	int onvif_get_imageParam(OnvifImageParam_t *pOnvifImageParam)
	{
		if(!pOnvifImageParam)
		{		
			dlog_error("pOnvifImageParam is NULL, onvif_get_imageParam falied");
			return -1;
		}

		int nRet = 0;
		ISP::ImageParam_S stImage;
		nRet = CIspConfigure::instance()->get_configure(stImage);

		pOnvifImageParam->nBrightness = stImage.nBrightness;
		pOnvifImageParam->nContrast = stImage.nContrast;
		pOnvifImageParam->nSaturation = stImage.nSaturation;
		pOnvifImageParam->nSharpness = stImage.nSharpness;

		return nRet;
	}

	int onvif_set_imageParam(OnvifImageParam_t *pOnvifImageParam)
	{
		if(!pOnvifImageParam)
		{		
			dlog_error("pOnvifImageParam is NULL, onvif_set_imageParam falied");
			return -1;
		}

		int nRet = 0;
		ISP::ImageParam_S stImage;
		CIspConfigure::instance()->get_configure(stImage);

		if(pOnvifImageParam->nBrightness >= BRIGHTNESS_RANGE_MIN && pOnvifImageParam->nBrightness <=  BRIGHTNESS_RANGE_MAX)
		{
			stImage.nBrightness = pOnvifImageParam->nBrightness;
		}
		
		if(pOnvifImageParam->nContrast >= CONTRAST_RANGE_MIN && pOnvifImageParam->nContrast <=  CONTRAST_RANGE_MAX)
		{
			stImage.nContrast = pOnvifImageParam->nContrast;
		}

		if(pOnvifImageParam->nSaturation >=  SATURATION_RANGE_MIN && pOnvifImageParam->nSaturation <=  SATURATION_RANGE_MAX)
		{
			stImage.nSaturation = pOnvifImageParam->nSaturation;
		}

		if(pOnvifImageParam->nSharpness >=  SHARPNESS_RANGE_MIN && pOnvifImageParam->nSharpness <=  SHARPNESS_RANGE_MAX)
		{
			stImage.nSharpness = pOnvifImageParam->nSharpness;
		}

		nRet = CIspConfigure::instance()->set_configure(stImage);
		nRet = CIspManage::instance()->update_config(ISP::PicConfigureType_E::IAMGE);

		return nRet;
	}

	/* 解析视频分辨率 */
	bool parseResolution(const std::string& strResolution, int& width, int& height) 
	{
		char ch;
		std::istringstream iss(strResolution);
		if (iss >> width >> ch >> height) {
			if (ch == 'x' || ch == 'X' || ch == '*') // 支持 "1280x720" 或 "1280*720" 格式
			{ 
				return true;
			}
		}
		return false;
	}

	/* FrameRates Helpers */
    float onvif_framerate_enum_to_float(Video_NS::FrameRate_E enRate) {
        switch(enRate) {
            case Video_NS::FRAME_RATE_1_16: return 1.0f/16.0f;
            case Video_NS::FRAME_RATE_1_8: return 1.0f/8.0f;
            case Video_NS::FRAME_RATE_1_4: return 1.0f/4.0f;
            case Video_NS::FRAME_RATE_1_2: return 1.0f/2.0f;
            case Video_NS::FRAME_RATE_1: return 1.0f;
            case Video_NS::FRAME_RATE_2: return 2.0f;
            case Video_NS::FRAME_RATE_4: return 4.0f;
            case Video_NS::FRAME_RATE_6: return 6.0f;
            case Video_NS::FRAME_RATE_8: return 8.0f;
            case Video_NS::FRAME_RATE_10: return 10.0f;
            case Video_NS::FRAME_RATE_12: return 12.0f;
            case Video_NS::FRAME_RATE_15: return 15.0f;
            case Video_NS::FRAME_RATE_16: return 16.0f;
            case Video_NS::FRAME_RATE_18: return 18.0f;
            case Video_NS::FRAME_RATE_20: return 20.0f;
            case Video_NS::FRAME_RATE_22: return 22.0f;
            case Video_NS::FRAME_RATE_25: return 25.0f;
            case Video_NS::FRAME_RATE_30: return 30.0f;
            default: return 25.0f; // Default
        }
    }

    Video_NS::FrameRate_E onvif_float_to_framerate_enum(float fRate) {
        // Epsilon check? Or simplistic mapping
        if(fRate <= 0.0625f + 0.001f) return Video_NS::FRAME_RATE_1_16;
        if(fRate <= 0.125f + 0.001f) return Video_NS::FRAME_RATE_1_8;
        if(fRate <= 0.25f + 0.001f) return Video_NS::FRAME_RATE_1_4;
        if(fRate <= 0.5f + 0.001f) return Video_NS::FRAME_RATE_1_2;
        if(fRate <= 1.0f + 0.001f) return Video_NS::FRAME_RATE_1;
        if(fRate <= 2.0f + 0.001f) return Video_NS::FRAME_RATE_2;
        if(fRate <= 4.0f + 0.001f) return Video_NS::FRAME_RATE_4;
        if(fRate <= 6.0f + 0.001f) return Video_NS::FRAME_RATE_6;
        if(fRate <= 8.0f + 0.001f) return Video_NS::FRAME_RATE_8;
        if(fRate <= 10.0f + 0.001f) return Video_NS::FRAME_RATE_10;
        if(fRate <= 12.0f + 0.001f) return Video_NS::FRAME_RATE_12;
        if(fRate <= 15.0f + 0.001f) return Video_NS::FRAME_RATE_15;
        if(fRate <= 16.0f + 0.001f) return Video_NS::FRAME_RATE_16;
        if(fRate <= 18.0f + 0.001f) return Video_NS::FRAME_RATE_18;
        if(fRate <= 20.0f + 0.001f) return Video_NS::FRAME_RATE_20;
        if(fRate <= 22.0f + 0.001f) return Video_NS::FRAME_RATE_22;
        if(fRate <= 25.0f + 0.001f) return Video_NS::FRAME_RATE_25;
        // if(fRate <= 30.0f + 0.001f) return Video_NS::FRAME_RATE_30;
        return Video_NS::FRAME_RATE_30;
    }

	int onvif_get_video_capabilities(OnvifVideoParam_t *pstVideoParams, int nStreamNum)
	{
		if(!pstVideoParams)
		{
			dlog_error("pstVideoParams is NULL, onvif_get_video_capabilities falied");
			return -1;
		}

		std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
		CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

		pstVideoParams->nIFrameInterval = (int)vstVideoConfig[nStreamNum].nIFrameInterval;

		/* 设备所有支持切换的分辨率 */
		Video_NS::VideoCapabilitySet_S VideoCapabilitySet;
		Video_NS::VideoCapability_S VideoCapability;
		CAVConfigure::instance()->get_configure(VideoCapabilitySet);

		if(nStreamNum == 0) // 主码流
		{
			pstVideoParams->nSizeResolutionsAvailable = VideoCapabilitySet.stMain.aResolution.size();
			VideoCapability = VideoCapabilitySet.stMain;
		}
		else if(nStreamNum == 1) // 子码流 
		{
			pstVideoParams->nSizeResolutionsAvailable = VideoCapabilitySet.stSub.aResolution.size();
			VideoCapability = VideoCapabilitySet.stSub;
		}

		for (int i = 0; i < pstVideoParams->nSizeResolutionsAvailable; i++)
		{
			parseResolution(VideoCapability.aResolution.at(i).strName, pstVideoParams->nWidths[i], pstVideoParams->nHeights[i]);
		}

		/* 视频质量范围 */
		pstVideoParams->nQualityMin = VIDEO_QUALITY_MIN;
		pstVideoParams->nQualityMax = VIDEO_QUALITY_MAX;

		/* 视频码率范围 */
		pstVideoParams->nBitrateMin = VIDEO_BITRATE_MIN;
		pstVideoParams->nBitrateMax = VIDEO_BITRATE_MAX;

		for (int i = VIDEO_FRAMERATE_MAX; i > 0; i--)
		{
			snprintf(pstVideoParams->FrameRatesSupported + strlen(pstVideoParams->FrameRatesSupported), sizeof(pstVideoParams->FrameRatesSupported), "%d ", i);
		}
		pstVideoParams->FrameRatesSupported[strlen(pstVideoParams->FrameRatesSupported) - 1] = '\0';

		// char *pGovLengthRange = "1 400";
		// snprintf(pstVideoParams->GovLengthRange, sizeof(pstVideoParams->GovLengthRange), pGovLengthRange, strlen(pGovLengthRange));

		/* 转换为onvif标准字段 */
		switch (vstVideoConfig[nStreamNum].enVideoCodec)
		{
		case Video_NS::VideoCodec_E::H264:
			memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_H264, strlen(VIDEO_CODEC_H264));
			break;
		case Video_NS::VideoCodec_E::H265:
			memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_H265, strlen(VIDEO_CODEC_H265));
			break;
		case Video_NS::VideoCodec_E::MJPEG:
			memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_MJPEG, strlen(VIDEO_CODEC_MJPEG));
			break;
		case Video_NS::VideoCodec_E::SVAC3:
			memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_SVAC3, strlen(VIDEO_CODEC_SVAC3));
			break;
		default:
			memcpy(pstVideoParams->strVideoCodec, "unknown codec", strlen("unknown codec"));
			break;
		}

		/*码率类型*/
		if ((int)vstVideoConfig[nStreamNum].enBitrateType)
		{
			pstVideoParams->bConstantBitRate = false;
		}
		else
		{
			pstVideoParams->bConstantBitRate = true;
		}

		switch (vstVideoConfig[nStreamNum].enEncodingComplexity)
		{
		case Video_NS::EncodingComplexity_E::Baseline:
			memcpy(pstVideoParams->strEncodingComplexity, "Baseline", strlen("Baseline"));
			break;
		case Video_NS::EncodingComplexity_E::Main:
			memcpy(pstVideoParams->strEncodingComplexity, "Main", strlen("Main"));
			break;
		case Video_NS::EncodingComplexity_E::High:
			memcpy(pstVideoParams->strEncodingComplexity, "High", strlen("High"));
			break;
		default:
			memcpy(pstVideoParams->strEncodingComplexity, "Main", strlen("Main"));
			break;
		}

        dlog_debug("获取视频能力: 码流号=%d, 编码=%s, 分辨率数量=%d",
            nStreamNum, 
            pstVideoParams->strVideoCodec,
            pstVideoParams->nSizeResolutionsAvailable);
		return 0;
	}

	int onvif_get_supported_codec_count(int nStreamNum)
	{
		Video_NS::VideoCapabilitySet_S VideoCapabilitySet;
		CAVConfigure::instance()->get_configure(VideoCapabilitySet);
		if(nStreamNum == 0)
			return VideoCapabilitySet.stMain.nEncodeTypeNum;
		else if(nStreamNum == 1)
			return VideoCapabilitySet.stSub.nEncodeTypeNum;
		return 0;
	}

	int onvif_get_video_capability_by_index(OnvifVideoParam_t *pstVideoParams, int nStreamNum, int nCodecIndex)
	{
		if(!pstVideoParams)
		{
			dlog_error("pstVideoParams is NULL");
			return -1;
		}

		Video_NS::VideoCapabilitySet_S VideoCapabilitySet;
		Video_NS::VideoCapability_S VideoCapability;
		CAVConfigure::instance()->get_configure(VideoCapabilitySet);

		if(nStreamNum == 0) // 主码流
		{
			VideoCapability = VideoCapabilitySet.stMain;
		}
		else if(nStreamNum == 1) // 子码流 
		{
			VideoCapability = VideoCapabilitySet.stSub;
		}
		else
		{
			return -1;
		}

		if (nCodecIndex < 0 || nCodecIndex >= VideoCapability.aEncodeAbility.size())
		{
			dlog_error("Invalid codec index %d", nCodecIndex);
			return -1;
		}

        std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
		CStreamVideo::instance()->getVideoConfig(vstVideoConfig);
		pstVideoParams->nIFrameInterval = (int)vstVideoConfig[nStreamNum].nIFrameInterval;

		pstVideoParams->nSizeResolutionsAvailable = VideoCapability.aResolution.size();
		for (int i = 0; i < pstVideoParams->nSizeResolutionsAvailable; i++)
		{
			parseResolution(VideoCapability.aResolution.at(i).strName, pstVideoParams->nWidths[i], pstVideoParams->nHeights[i]);
		}

		/* 视频质量范围 */
		pstVideoParams->nQualityMin = VIDEO_QUALITY_MIN;
		pstVideoParams->nQualityMax = VIDEO_QUALITY_MAX;

		/* 视频码率范围 */
		pstVideoParams->nBitrateMin = VIDEO_BITRATE_MIN;
		pstVideoParams->nBitrateMax = VIDEO_BITRATE_MAX;

		/* FrameRate */
        memset(pstVideoParams->FrameRatesSupported, 0, sizeof(pstVideoParams->FrameRatesSupported));
        snprintf(pstVideoParams->FrameRatesSupported, sizeof(pstVideoParams->FrameRatesSupported), 
            "1/16 1/8 1/4 1/2 1 2 4 6 8 10 12 15 16 18 20 22 25 30");

		/* Codec Specific */
		Video_NS::EncodeAbility_S ability = VideoCapability.aEncodeAbility[nCodecIndex];
        
        const char* pCodecName = "unknown";
        if (ability.strVideoCodec == "H.264") pCodecName = VIDEO_CODEC_H264;
        else if (ability.strVideoCodec == "H.265") pCodecName = VIDEO_CODEC_H265;
        else if (ability.strVideoCodec == "MJPEG") pCodecName = VIDEO_CODEC_MJPEG;
        else if (ability.strVideoCodec == "SVAC3") pCodecName = VIDEO_CODEC_SVAC3;
        
        memset(pstVideoParams->strVideoCodec, 0, sizeof(pstVideoParams->strVideoCodec));
		strncpy(pstVideoParams->strVideoCodec, pCodecName, sizeof(pstVideoParams->strVideoCodec)-1);
        
        memcpy(pstVideoParams->strEncodingComplexity, "Main", strlen("Main"));

        dlog_debug("获取视频能力(Index=%d): 码流号=%d, 编码=%s", nCodecIndex, nStreamNum, pstVideoParams->strVideoCodec);

		return 0;
	}

	// Helper to map ONVIF quality (1-6) to internal ImageQuality_E
	int onvif_map_quality_onvif_to_internal(int onvif_quality)
	{
        // Clamp to 1-6
        if(onvif_quality < 1) onvif_quality = 1;
        if(onvif_quality > 6) onvif_quality = 6;

        switch(onvif_quality) {
            case 1: return 1;   // LOWEST
            case 2: return 20;  // LOWER
            case 3: return 40;  // LOW
            case 4: return 60;  // MEDIUM
            case 5: return 80;  // HIGHER
            case 6: return 100; // HIGHEST
            default: return 60; // Should not happen due to clamp
        }
	}

    // Helper to map internal ImageQuality_E to ONVIF quality (1-6)
    int onvif_map_quality_internal_to_onvif(int internal_quality)
    {
        if(internal_quality <= 1) return 1;
        if(internal_quality <= 20) return 2;
        if(internal_quality <= 40) return 3;
        if(internal_quality <= 60) return 4;
        if(internal_quality <= 80) return 5;
        return 6; 
    }

    int onvif_get_videoParams(OnvifVideoParam_t *pstVideoParams, int nStreamNum)
    {
        if(!pstVideoParams)
        {
            dlog_error("pstVideoParams is NULL, onvif_get_videoParams failed");
            return -1;
        }

        std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
        CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

        /* 基础参数 */
        pstVideoParams->nIFrameInterval = (int)vstVideoConfig[nStreamNum].nIFrameInterval;
        pstVideoParams->nWidths[0] = vstVideoConfig[nStreamNum].stVideoResolution.nWidth;
        pstVideoParams->nHeights[0] = vstVideoConfig[nStreamNum].stVideoResolution.nHeight;
        pstVideoParams->nHeights[0] = vstVideoConfig[nStreamNum].stVideoResolution.nHeight;
        pstVideoParams->fCurFrameRate = onvif_framerate_enum_to_float(vstVideoConfig[nStreamNum].enFrameRate);
        pstVideoParams->nCurBitrate = vstVideoConfig[nStreamNum].nBitrateUpperLimit;
        pstVideoParams->nVideoType = (int)vstVideoConfig[nStreamNum].enVideoType; // VideoType
        pstVideoParams->nCurQuality = onvif_map_quality_internal_to_onvif((int)vstVideoConfig[nStreamNum].enImageQuality);

        /* 编码格式 */
        switch (vstVideoConfig[nStreamNum].enVideoCodec)
        {
            case Video_NS::VideoCodec_E::H264:
                memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_H264, strlen(VIDEO_CODEC_H264));
                break;
            case Video_NS::VideoCodec_E::H265:
                memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_H265, strlen(VIDEO_CODEC_H265));
                break;
            case Video_NS::VideoCodec_E::MJPEG:
                memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_MJPEG, strlen(VIDEO_CODEC_MJPEG));
                break;
            case Video_NS::VideoCodec_E::SVAC3:
                memcpy(pstVideoParams->strVideoCodec, VIDEO_CODEC_SVAC3, strlen(VIDEO_CODEC_SVAC3));
                break;
            default:
                memcpy(pstVideoParams->strVideoCodec, "unknown codec", strlen("unknown codec"));
                break;
        }

        /* 码率控制 */
        if (vstVideoConfig[nStreamNum].enBitrateType == Video_NS::BitrateType_E::CBR)
        {
            pstVideoParams->bConstantBitRate = true;
        }
        else
        {
            pstVideoParams->bConstantBitRate = false;
        }
        
        dlog_debug("获取视频参数: 码流号=%d, 编码=%s, 分辨率=%dx%d, 码率=%d, 帧率=%d, I帧间隔=%d",
            nStreamNum, 
            pstVideoParams->strVideoCodec,
            pstVideoParams->nWidths[0], pstVideoParams->nHeights[0],
            pstVideoParams->nCurBitrate,
            pstVideoParams->nCurBitrate,
            pstVideoParams->fCurFrameRate,
            pstVideoParams->nIFrameInterval);

        dlog_debug("获取视频参数(Quality): Internal=%d -> ONVIF=%d", 
            (int)vstVideoConfig[nStreamNum].enImageQuality, 
            pstVideoParams->nCurQuality);

        return 0;
    }

	int onvif_set_videoParams(OnvifVideoParam_t *pstVideoParams, int nStreamNum)
	{
		if(!pstVideoParams)
		{
			dlog_error("pstVideoParams is NULL, onvif_set_videoParams failed");
			return -1;
		}
        dlog_debug("设置视频参数: 码流号=%d, 编码=%s, 分辨率=%dx%d, 码率=%d, 帧率=%d, I帧间隔=%d, 定码率=%d",
            nStreamNum,
            pstVideoParams->strVideoCodec,
            pstVideoParams->nWidths[0], pstVideoParams->nHeights[0],
            pstVideoParams->nCurBitrate,
            pstVideoParams->nCurBitrate,
            pstVideoParams->fCurFrameRate,
            pstVideoParams->nIFrameInterval,
            pstVideoParams->bConstantBitRate);

		std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
		Video_NS::VideoConfig_S stVideoConfig;
		CStreamVideo::instance()->getVideoConfig(vstVideoConfig);
		stVideoConfig = vstVideoConfig[nStreamNum];

		if (pstVideoParams->nIFrameInterval > 0 && pstVideoParams->nIFrameInterval <= VIDEO_I_FRAME_INTERVAL_MAX)
		{
			stVideoConfig.nIFrameInterval = pstVideoParams->nIFrameInterval;
		}

		if (strcmp(pstVideoParams->strVideoCodec, VIDEO_CODEC_H264) == 0)
		{
			stVideoConfig.enVideoCodec = Video_NS::VideoCodec_E::H264;
		}
		else if (strcmp(pstVideoParams->strVideoCodec, VIDEO_CODEC_H265) == 0)
		{
			stVideoConfig.enVideoCodec = Video_NS::VideoCodec_E::H265;
		}
		else if (strcmp(pstVideoParams->strVideoCodec, VIDEO_CODEC_MJPEG) == 0)
		{
			stVideoConfig.enVideoCodec = Video_NS::VideoCodec_E::MJPEG;
		}
		else if (strcmp(pstVideoParams->strVideoCodec, VIDEO_CODEC_SVAC3) == 0)
		{
			stVideoConfig.enVideoCodec = Video_NS::VideoCodec_E::SVAC3;
		}

		if (pstVideoParams->nWidths[0] > 0 && pstVideoParams->nHeights[0] > 0)
		{
			stVideoConfig.stVideoResolution.nWidth = pstVideoParams->nWidths[0];
			stVideoConfig.stVideoResolution.nHeight = pstVideoParams->nHeights[0];
		}
		
		if (!pstVideoParams->bConstantBitRate)
		{
			stVideoConfig.enBitrateType = Video_NS::BitrateType_E::VBR;
			if(pstVideoParams->nCurQuality > 0)
			{
				stVideoConfig.enImageQuality = (Video_NS::ImageQuality_E)onvif_map_quality_onvif_to_internal(pstVideoParams->nCurQuality);
                dlog_debug("设置视频参数(Quality): ONVIF=%d -> Internal=%d", 
                    pstVideoParams->nCurQuality, 
                    (int)stVideoConfig.enImageQuality);
			}
		}
		else 
		{
			stVideoConfig.enBitrateType = Video_NS::BitrateType_E::CBR;
		}

		if(pstVideoParams->nCurBitrate > 0 && pstVideoParams->nCurBitrate <= VIDEO_BITRATE_MAX)
		{
			stVideoConfig.nBitrateUpperLimit = pstVideoParams->nCurBitrate;
		}

        /* 暂时不支持设置分数帧率，默认为1fps */
        Video_NS::FrameRate_E enFrameRate = onvif_float_to_framerate_enum(pstVideoParams->fCurFrameRate);
        stVideoConfig.enFrameRate = enFrameRate < Video_NS::FRAME_RATE_2 ? Video_NS::FRAME_RATE_2 : enFrameRate;

        /* Video Type */
        if(pstVideoParams->nVideoType >= 0) {
            stVideoConfig.enVideoType = (Video_NS::VideoType_E)pstVideoParams->nVideoType;
        }

		CAVConfigure::instance()->set_configure(stVideoConfig);
		return 0;
	}

	int onvif_get_audioParams(OnvifAudioParam_t *pstAudioParams)
	{
		if(!pstAudioParams)
		{
			dlog_error("pstAudioParams is NULL, onvif_get_audioParams failed");
			return -1;
		}

		Audio_NS::AudioConfig_S stAudioConfig;
		memset(&stAudioConfig, 0, sizeof(stAudioConfig));
		CStreamAudio::instance()->getAudioConfig(stAudioConfig);

		pstAudioParams->audioBitrate = (int)stAudioConfig.enBitRate;
		pstAudioParams->audioSampleRate = (int)stAudioConfig.enSampRate;

		/* 转换为onvif支持的标准字段 */
		switch (stAudioConfig.enFormat)
		{
		case Audio_NS::AudioFormat::G711A:
		case Audio_NS::AudioFormat::G711U:
			pstAudioParams->audioFormat = AudioEncoding_G711;
			memcpy(pstAudioParams->audioEncoding, "G711", strlen("G711"));
			break;
		case Audio_NS::AudioFormat::G726:
			pstAudioParams->audioFormat = AudioEncoding_G726;
			memcpy(pstAudioParams->audioEncoding, "G726", strlen("G726"));
			break;
		case Audio_NS::AudioFormat::AAC:
		default:
			pstAudioParams->audioFormat = AudioEncoding_AAC;
			memcpy(pstAudioParams->audioEncoding, "AAC", strlen("AAC"));
			break;
		}

		/* 获取设备所支持的所有采样率 */
		for (int i = 0; i < AUDIO_SAMPRATE_LIST; i++)
		{
			pstAudioParams->SampleRateList[i] = (int)AUDIO_SAMPLERATE_MAP[i];
		}

		/* 获取设备所支持的所有码率 */
		for (int i = 0; i < AUDIO_BITRATE_LIST; i++)
		{
			pstAudioParams->BitrateList[i] = (int)AUDIO_BITRATE_MAP[i];
		}

		return 0;
	}

	int onvif_set_audioParams(OnvifAudioParam_t *pstAudioParams)
	{
		if(!pstAudioParams)
		{
			dlog_error("pstAudioParams is NULL, onvif_set_audioParams failed");
			return -1;
		}

		Audio_NS::AudioConfig_S stAudioConfig;
		memset(&stAudioConfig, 0, sizeof(stAudioConfig));
		CStreamAudio::instance()->getAudioConfig(stAudioConfig);

		/* 查找设置的采样率是否在设备所支持的列表里 */
		for (int i = 0; i < AUDIO_SAMPRATE_LIST; i++)
		{
			if (pstAudioParams->audioSampleRate == (int)AUDIO_SAMPLERATE_MAP[i])
			{
				stAudioConfig.enSampRate = AUDIO_SAMPLERATE_MAP[i];
			}
		}

		/* 查找设置的码率是否在设备所支持的列表里 */
		for (int i = 0; i < AUDIO_BITRATE_LIST; i++)
		{
			if (pstAudioParams->audioBitrate == (int)AUDIO_BITRATE_MAP[i])
			{
				stAudioConfig.enBitRate = AUDIO_BITRATE_MAP[i];
				break;
			}
		}

		if (strcmp(pstAudioParams->audioEncoding, "G711") == 0)
		{
			stAudioConfig.enFormat = Audio_NS::AudioFormat::G711A;
		}
		else if (strcmp(pstAudioParams->audioEncoding, "G726") == 0)
		{
			stAudioConfig.enFormat = Audio_NS::AudioFormat::G726;
		}
		else if (strcmp(pstAudioParams->audioEncoding, "AAC") == 0)
		{
			stAudioConfig.enFormat = Audio_NS::AudioFormat::AAC;
		}

		return CStreamAudio::instance()->setAudioConfig(stAudioConfig);
	}

	int onvif_create_subscription(const char* pAddress) 
    {
        if (!pAddress)
		{
			return -1;
		} 
        return COnvifSubscriptionManager::instance()->addSubscription(pAddress) ? 0 : 1;
    }

	void onvif_destroy_subscription(const char* pAddress) 
    {
        if (pAddress)
		{
			return;
		}
		COnvifSubscriptionManager::instance()->removeSubscription(pAddress);
    }

	int onvif_pull_events(int socket_fd, const char* pAddress, OnvifAlarmEventBatch_S* pBatch, int nTimeoutMs) 
	{
		if (!pAddress || !pBatch) 
		{
			return -1;
		}
		return COnvifSubscriptionManager::instance()->pullEvents(socket_fd, pAddress, pBatch, nTimeoutMs);
	}

	int onvif_get_motion_info(OnvifMotionDetection_S *pInfo)
	{
		if(NULL == pInfo)
		{
			return -1;
		}
		/* 区域网格 */
        using AreaGrid = std::vector<std::vector<unsigned int>>;
		AreaGrid Grid(CELL_MOTION_COLUMNS, std::vector<unsigned int>(CELL_MOTION_ROWS, 0));

		Alarm::MotionDetection_S stMotionAlarm;
		CEventConfigure::instance()->get_configure(stMotionAlarm);
        pInfo->bEnable = stMotionAlarm.bEnable;

		snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stMotionAlarm.stMotionNormalMode.nSensitivity);
		
		auto& varRegion = stMotionAlarm.stMotionNormalMode.varRegion;
		if (std::holds_alternative<AreaGrid>(varRegion))
		{
			Grid = std::get<AreaGrid>(stMotionAlarm.stMotionNormalMode.varRegion);
		}
		/* CELL_MOTION_ROWS:高度 CELL_MOTION_COLUMNS：宽度*/
		for (int i = 0; i < CELL_MOTION_ROWS; ++i) 
		{
        	for (int j = 0; j < CELL_MOTION_COLUMNS; ++j) 
			{
            	int nIndex = i * CELL_MOTION_COLUMNS + j; 
            	if (nIndex < 1024) 
				{ 
                	pInfo->auActiveCell[nIndex] = Grid[i][j];
            	}
        	}
    	}
		/* 将数组转换成BASE编码 */
		ONVIF_MotionArrayToBase64Str(pInfo->auActiveCell,sizeof(pInfo->auActiveCell) / sizeof(pInfo->auActiveCell[0]),pInfo->achBaseStr,CELL_MOTION_COLUMNS,CELL_MOTION_ROWS);

        onvif_serialize_schedule(stMotionAlarm.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
		return 0;
	}

	int onvif_set_motion_analytics(OnvifMotionDetection_S *pInfo)
	{
		if(NULL == pInfo)
		{
			return -1;
		}
		Alarm::MotionDetection_S stMotionAlarm;
		CEventConfigure::instance()->get_configure(stMotionAlarm);
		int nSensitivity = 0; 
		try 
		{
			std::string strSensitivity(pInfo->achSensitivity);
			nSensitivity = std::stoi(strSensitivity);
			dlog_debug("Onvif设置移动侦测灵敏度：%d",nSensitivity);
		}
		catch (const std::invalid_argument& e) 
		{
			dlog_error("转换失败：无效的数字格式");
		}
		catch (const std::out_of_range& e) 
		{
			dlog_error("转换失败：数值超出范围");
		}
		if(nSensitivity < 0 || nSensitivity > 100)
		{
			dlog_error("Onvif设置移动侦测灵敏度失败 超出范围！");
			return -1;
		}

		stMotionAlarm.stMotionNormalMode.nSensitivity = nSensitivity;
        if (nSensitivity == 0)
        {
            stMotionAlarm.bEnable = false;
        }
        else if (pInfo->bEnableSet)
        {
            stMotionAlarm.bEnable = pInfo->bEnable;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stMotionAlarm.aAlarmTime);
        }
    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::MOTION_DETECT;
    stEventSchedule.bStatus = stMotionAlarm.bEnable;
    stEventSchedule.defenseTime = stMotionAlarm.aAlarmTime;
    CEventConfigure::instance()->set_configure(stEventSchedule);

    CEventConfigure::instance()->set_configure(stMotionAlarm);
    CEventManage::instance()->update_event_schedule();
    return 0;
	}
	
	int onvif_set_motion_rule(OnvifMotionDetection_S *pInfo)
	{
		if(NULL == pInfo)
		{
			return -1;
		}
		Alarm::MotionDetection_S stMotionAlarm;
		CEventConfigure::instance()->get_configure(stMotionAlarm);
		std::vector<std::vector<unsigned int>> aNormalArea;
		/* base编码转换成网格 */
		if(ONVIF_MotionBase64StrToArray(pInfo->achBaseStr, pInfo->auActiveCell, sizeof(pInfo->auActiveCell) / sizeof(pInfo->auActiveCell[0]),  CELL_MOTION_COLUMNS, CELL_MOTION_ROWS) != 0)
		{
			return -1;
		}

		for (int i = 0; i < CELL_MOTION_ROWS; ++i) 
		{
			aNormalArea.push_back(std::vector<unsigned int>());
        	for (int j = 0; j < CELL_MOTION_COLUMNS; ++j) 
			{
            	int nIndex = i * CELL_MOTION_COLUMNS + j; 
            	if (nIndex < 1024) 
				{ 
					if(pInfo->auActiveCell[nIndex] != 0 && pInfo->auActiveCell[nIndex] != 1)
					{
						dlog_error("网格编码数组[%d]不符合标准值[%d] ",nIndex,pInfo->auActiveCell[nIndex]);
						return -1;
					}
                	aNormalArea[i].push_back(pInfo->auActiveCell[nIndex]);
            	}
        	}
    	}
		/* 普通模式 */
		stMotionAlarm.enMode = Alarm::MOTION_NORMAL;
		stMotionAlarm.stMotionNormalMode.nRegionType = 1;
		stMotionAlarm.stMotionNormalMode.varRegion = aNormalArea;

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stMotionAlarm.aAlarmTime);
        }
    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::MOTION_DETECT;
    if (stMotionAlarm.stMotionNormalMode.nSensitivity == 0)
    {
        stMotionAlarm.bEnable = false;
    }
    else if (pInfo->bEnableSet)
    {
        stMotionAlarm.bEnable = pInfo->bEnable;
    }
    stEventSchedule.bStatus = stMotionAlarm.bEnable;
    stEventSchedule.defenseTime = stMotionAlarm.aAlarmTime;
    CEventConfigure::instance()->set_configure(stEventSchedule);

    CEventConfigure::instance()->set_configure(stMotionAlarm);
    CEventManage::instance()->update_event_schedule();
    return 0;
	}

	int onvif_get_tamp_info(ONvifTamperDetection_S *pInfo)
	{
		if(NULL == pInfo)
		{
			return -1;
		}
		Alarm::HideAlarm_S stHideAlarm;
		CEventConfigure::instance()->get_configure(stHideAlarm);
        pInfo->bEnable = stHideAlarm.bEnable;
		int nSensitivity = stHideAlarm.nSensitivity * 33;
		snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", nSensitivity);
		/* 遮挡坐标转换 */
		convert_tamper_rect(&stHideAlarm.stRect,pInfo->stPolygon,false);
        onvif_serialize_schedule(stHideAlarm.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
		return 0;
	}
  
	int onvif_set_tamp_analytics(ONvifTamperDetection_S *pInfo)
	{
		if(NULL == pInfo)
		{
			return -1;
		}
		Alarm::HideAlarm_S stHideAlarm;
		CEventConfigure::instance()->get_configure(stHideAlarm);
		int nSensitivity = 0; 
		try 
		{
			std::string strSensitivity(pInfo->achSensitivity);
			nSensitivity = std::stoi(strSensitivity);
			dlog_debug("Onvif设置遮挡报警灵敏度：%d",nSensitivity);
		}
		catch (const std::invalid_argument& e) 
		{
			dlog_error("转换失败：无效的数字格式");
		}
		catch (const std::out_of_range& e) 
		{
			dlog_error("转换失败：数值超出范围");
		}
		if(nSensitivity < 0 || nSensitivity > 100)
		{
			dlog_error("Onvif设置移动侦测灵敏度失败 超出范围！");
			return -1;
		}

		if (nSensitivity == 0)
        {
            stHideAlarm.bEnable = false;
        }
        else if (pInfo->bEnableSet)
        {
            stHideAlarm.bEnable = pInfo->bEnable;
        }


        stHideAlarm.nSensitivity = nSensitivity / 33;
        convert_tamper_rect(&stHideAlarm.stRect,pInfo->stPolygon,false);
        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stHideAlarm.aAlarmTime);
        }
    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::OCCLUSION_DETECT;
    stEventSchedule.bStatus = stHideAlarm.bEnable;
    stEventSchedule.defenseTime = stHideAlarm.aAlarmTime;
    CEventConfigure::instance()->set_configure(stEventSchedule);

    CEventConfigure::instance()->set_configure(stHideAlarm);
    CEventManage::instance()->update_event_schedule();
    return 0;
	}

    int onvif_set_tamp_rule(ONvifTamperDetection_S *pInfo)
	{
		if(NULL == pInfo)
		{
			return -1;
		}
		Alarm::HideAlarm_S stHideAlarm;
		CEventConfigure::instance()->get_configure(stHideAlarm);

		/* 遮挡坐标转换 */
		convert_tamper_rect(&stHideAlarm.stRect,pInfo->stPolygon,true);
        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stHideAlarm.aAlarmTime);
        }
    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::OCCLUSION_DETECT;
    if (stHideAlarm.nSensitivity == 0)
    {
        stHideAlarm.bEnable = false;
    }
    else if (pInfo->bEnableSet)
    {
        stHideAlarm.bEnable = pInfo->bEnable;
    }
    stEventSchedule.bStatus = stHideAlarm.bEnable;
    stEventSchedule.defenseTime = stHideAlarm.aAlarmTime;
    CEventConfigure::instance()->set_configure(stEventSchedule);

    CEventConfigure::instance()->set_configure(stHideAlarm);
    CEventManage::instance()->update_event_schedule();
    return 0;
	}


    /* Helper definitions for coordinate conversion */
    #define SMART_EVENT_ONVIF_WIDTH 1920
    #define SMART_EVENT_ONVIF_HEIGHT 1080
    #define LOCAL_COMMON_WIDTH 1920
    #define LOCAL_COMMON_HEIGHT 1080




    static void convert_region_to_onvif(const Alarm::Region_S& stRegion, OnvifPoint_S* pPolygon, int* pnPointNum)
    {
        int nPoints = stRegion.aPoint.size();
        if (nPoints > ONVIF_ANALYTICS_POLYGON_POINT_NUM) nPoints = ONVIF_ANALYTICS_POLYGON_POINT_NUM;
        
        double x_scale = (double)SMART_EVENT_ONVIF_WIDTH / LOCAL_COMMON_WIDTH;
        double y_scale = (double)SMART_EVENT_ONVIF_HEIGHT / LOCAL_COMMON_HEIGHT;

        for(int i=0; i<nPoints; i++) {
            pPolygon[i].x = (int)(stRegion.aPoint[i].fX * x_scale);
            pPolygon[i].y = (int)(stRegion.aPoint[i].fY * y_scale);
        }
        *pnPointNum = nPoints;
    }

    static void convert_onvif_to_region(const OnvifPoint_S* pPolygon, int nPointNum, Alarm::Region_S& stRegion)
    {
        stRegion.aPoint.clear();
        stRegion.nPointNum = nPointNum;
        
        double x_scale = (double)LOCAL_COMMON_WIDTH / SMART_EVENT_ONVIF_WIDTH;
        double y_scale = (double)LOCAL_COMMON_HEIGHT / SMART_EVENT_ONVIF_HEIGHT;

        for(int i=0; i<nPointNum; i++) {
            Common::PosF_S pos;
            pos.fX = (float)(pPolygon[i].x * x_scale);
            pos.fY = (float)(pPolygon[i].y * y_scale);
            stRegion.aPoint.push_back(pos);
        }
    }

    static void onvif_serialize_detection_target(const std::vector<int>& target, char* buffer, size_t size)
    {
        std::string strTarget = "";
        for(size_t i = 0; i < target.size(); i++)
        {
            char temp[16];
            snprintf(temp, sizeof(temp), "%d", target[i]);
            if(i > 0) strTarget += ",";
            strTarget += temp;
        }
        snprintf(buffer, size, "%s", strTarget.c_str());
    }

    static void onvif_deserialize_detection_target(const char* buffer, std::vector<int>& target)
    {
        target.clear();
        if(buffer == NULL || strlen(buffer) == 0) return;
        std::string strTarget(buffer);
        size_t start = 0, end = 0;
        while ((end = strTarget.find(',', start)) != std::string::npos) {
            try { 
                if(end > start) target.push_back(std::stoi(strTarget.substr(start, end - start))); 
            } catch(...) {}
            start = end + 1;
        }
        try { 
            if(start < strTarget.length()) target.push_back(std::stoi(strTarget.substr(start))); 
        } catch(...) {}
    }

    int onvif_get_enter_region_count()
    {
        Alarm::EntranceDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_enter_region_info(int nIndex, OnvifRegionDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::EntranceDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        onvif_serialize_detection_target(stConf.aRule[nIndex].aDetectionTarget, pInfo->achDetectionTarget, sizeof(pInfo->achDetectionTarget));
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_enter_region_info(int nIndex, OnvifRegionDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::ENTER_REGION, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::EntranceDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::EnterExitIntrusion_S()); 
                 // Initialize defaults if needed
                 stConf.aRule.back().nSensitivity = 50; 
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置进入区域参数错误");
            return -1;
        }
        onvif_deserialize_detection_target(pInfo->achDetectionTarget, stConf.aRule[nIndex].aDetectionTarget);
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置进入区域参数的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::ENTER_REGION;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_leave_region_count()
    {
        Alarm::ExitingDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_leave_region_info(int nIndex, OnvifRegionDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::ExitingDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        onvif_serialize_detection_target(stConf.aRule[nIndex].aDetectionTarget, pInfo->achDetectionTarget, sizeof(pInfo->achDetectionTarget));
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_leave_region_info(int nIndex, OnvifRegionDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::LEAVE_REGION, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::ExitingDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::EnterExitIntrusion_S()); 
                 stConf.aRule.back().nSensitivity = 50;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置离开区域参数错误");
            return -1;
        }
        onvif_deserialize_detection_target(pInfo->achDetectionTarget, stConf.aRule[nIndex].aDetectionTarget);
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置离开区域参数的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::LEAVE_REGION;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_audio_anomaly_count()
    {
        return 1;
    }

    int onvif_get_audio_anomaly_info(int nIndex, OnvifAudioAnomaly_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        Alarm::AudioAnomaly_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        pInfo->bEnable = stConf.bEnable;
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.nUpSensitivity);
        snprintf(pInfo->achThreshold, sizeof(pInfo->achThreshold), "%d", stConf.nUpThreshold);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_audio_anomaly_info(int nIndex, OnvifAudioAnomaly_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::AUDIO_ANOMALY, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::AudioAnomaly_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable;
        }
        stConf.nUpSensitivity = atoi(pInfo->achSensitivity);
        stConf.nUpThreshold = atoi(pInfo->achThreshold);
        /* 参数有效性判断 */
        if (stConf.nUpSensitivity < 1 || stConf.nUpSensitivity > 100 || stConf.nUpThreshold < 1 ||
            stConf.nUpThreshold > 100)
        {
            dlog_error("设置音频异常侦测信息参数错误");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::AUDIO_ANOMALY;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_scene_change_count()
    {
        return 1;
    }

    int onvif_get_scene_change_info(int nIndex, OnvifSceneChange_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        Alarm::SceneChange_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        pInfo->bEnable = stConf.bEnable;
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.nSensitivity);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_scene_change_info(int nIndex, OnvifSceneChange_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::SCENE_CHANGE, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::SceneChange_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable;
        }
        stConf.nSensitivity = atoi(pInfo->achSensitivity);
        /* 参数有效性判断 */
        if (stConf.nSensitivity < 1 || stConf.nSensitivity > 100)
        {
            dlog_error("设置场景变更侦测信息参数错误");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::SCENE_CHANGE;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_face_detect_count()
    {
        return 1;
    }

    int onvif_get_face_detect_info(int nIndex, OnvifFaceDetection_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        Alarm::FaceDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        pInfo->bEnable = stConf.bEnable;
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.nSensitivity);
        convert_region_to_onvif(stConf.stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_face_detect_info(int nIndex, OnvifFaceDetection_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::FACE_DETECT, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::FaceDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable;
        }
        stConf.nSensitivity = atoi(pInfo->achSensitivity);
        /* 参数有效性判断 */
        if (stConf.nSensitivity < 1 || stConf.nSensitivity > 100)
        {
            dlog_error("设置人脸侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.stRegion);
        /* 坐标有效性判断 */
        if (!stConf.stRegion.IsValid())
        {
            dlog_error("设置人脸侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::FACE_DETECT;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_loitering_detect_count()
    {
        Alarm::LoiteringDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_loitering_detect_info(int nIndex, OnvifLoiteringDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::LoiteringDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        snprintf(pInfo->achTimeThreshold, sizeof(pInfo->achTimeThreshold), "%d", stConf.aRule[nIndex].nTimeThreshold);
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_loitering_detect_info(int nIndex, OnvifLoiteringDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::LOITERING_DETECT, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::LoiteringDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::LoiteringRule_S()); 
                 stConf.aRule.back().nSensitivity = 50;
                 stConf.aRule.back().nTimeThreshold = 10;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        stConf.aRule[nIndex].nTimeThreshold = atoi(pInfo->achTimeThreshold);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nTimeThreshold < 0 || stConf.aRule[nIndex].nTimeThreshold > 100 || 
            stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置徘徊侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置徘徊侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::LOITERING_DETECT;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_crowd_gathering_count()
    {
        Alarm::CrowdGathering_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_crowd_gathering_info(int nIndex, OnvifCrowdGathering_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::CrowdGathering_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", 50); 
        snprintf(pInfo->achObjectOccup, sizeof(pInfo->achObjectOccup), "%d", stConf.aRule[nIndex].nObjectOccup);
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_crowd_gathering_info(int nIndex, OnvifCrowdGathering_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::CROWD_GATHERING, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::CrowdGathering_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::CrowdGatheringRule_S()); 
                 stConf.aRule.back().nObjectOccup = 50;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nObjectOccup = atoi(pInfo->achObjectOccup);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nObjectOccup < 1 || stConf.aRule[nIndex].nObjectOccup > 100)
        {
            dlog_error("设置人员聚集侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置人员聚集侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::CROWD_GATHERING;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_parking_detect_count()
    {
        Alarm::ParkingDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_parking_detect_info(int nIndex, OnvifParkingDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::ParkingDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        snprintf(pInfo->achTimeThreshold, sizeof(pInfo->achTimeThreshold), "%d", stConf.aRule[nIndex].nTimeThreshold);
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_parking_detect_info(int nIndex, OnvifParkingDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::PARKING_DETECT, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::ParkingDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::ParkingRule_S()); 
                 stConf.aRule.back().nSensitivity = 50;
                 stConf.aRule.back().nTimeThreshold = 10;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        stConf.aRule[nIndex].nTimeThreshold = atoi(pInfo->achTimeThreshold);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nTimeThreshold < 0 || stConf.aRule[nIndex].nTimeThreshold > 100 || 
            stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置停车侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置停车侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::PARKING_DETECT;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_unattended_object_count()
    {
        Alarm::UnattendedObject_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_unattended_object_info(int nIndex, OnvifUnattendedObject_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::UnattendedObject_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        snprintf(pInfo->achTimeThreshold, sizeof(pInfo->achTimeThreshold), "%d", stConf.aRule[nIndex].nTimeThreshold);
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_unattended_object_info(int nIndex, OnvifUnattendedObject_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::UNATTENDED_OBJECT, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::UnattendedObject_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::UnattendedObjectRule_S()); 
                 stConf.aRule.back().nSensitivity = 50;
                 stConf.aRule.back().nTimeThreshold = 10;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        stConf.aRule[nIndex].nTimeThreshold = atoi(pInfo->achTimeThreshold);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nTimeThreshold < 0 || stConf.aRule[nIndex].nTimeThreshold > 100 || 
            stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置物品遗留侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置物品遗留侦测信息的区域绘制异常");
            return -1;
        }
        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

         /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::UNATTENDED_OBJECT;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_object_removal_count()
    {
        Alarm::ObjectRemoval_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_object_removal_info(int nIndex, OnvifObjectRemoval_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::ObjectRemoval_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        snprintf(pInfo->achTimeThreshold, sizeof(pInfo->achTimeThreshold), "%d", stConf.aRule[nIndex].nTimeThreshold);
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_object_removal_info(int nIndex, OnvifObjectRemoval_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::OBJECT_REMOVAL, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::ObjectRemoval_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::ObjectRemovalRule_S()); 
                 stConf.aRule.back().nSensitivity = 50;
                 stConf.aRule.back().nTimeThreshold = 10;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        stConf.aRule[nIndex].nTimeThreshold = atoi(pInfo->achTimeThreshold);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nTimeThreshold < 0 || stConf.aRule[nIndex].nTimeThreshold > 100 || 
            stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置物品拿取侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置物品拿取侦测信息的区域绘制异常");
            return -1;
        }
        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::OBJECT_REMOVAL;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_pet_recognition_count()
    {
        return 1;
    }

    int onvif_get_pet_recognition_info(int nIndex, OnvifPetRecognition_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        Alarm::PetRecognition_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        pInfo->bEnable = stConf.bEnable;
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.nSensitivity);
        convert_region_to_onvif(stConf.stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_pet_recognition_info(int nIndex, OnvifPetRecognition_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::PET_RECOGNITION, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::PetRecognition_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable;
        }
        stConf.nSensitivity = atoi(pInfo->achSensitivity);
        /* 参数有效性判断 */
        if (stConf.nSensitivity < 1 || stConf.nSensitivity > 100)
        {
            dlog_error("设置宠物识别侦测信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.stRegion);
        /* 坐标有效性判断 */
        if (!stConf.stRegion.IsValid())
        {
            dlog_error("设置宠物识别侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::PET_RECOGNITION;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_face_capture_count()
    {
        return 1;
    }

    int onvif_get_face_capture_info(int nIndex, OnvifFaceCapture_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        Alarm::FaceCapture_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        pInfo->bEnable = stConf.bEnable; /* Check if stRule has bEnable */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.stRule.nSensitivity);
        snprintf(pInfo->achInterval, sizeof(pInfo->achInterval), "%d", stConf.stRule.nInterval);
        convert_region_to_onvif(stConf.stRule.stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_face_capture_info(int nIndex, OnvifFaceCapture_S *pInfo)
    {
        if(!pInfo || nIndex != 0) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::FACE_CAPTURE, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::FaceCapture_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable;
        }
        stConf.stRule.nSensitivity = atoi(pInfo->achSensitivity);
        stConf.stRule.nInterval = atoi(pInfo->achInterval);
        /* 参数有效性判断 */
        if (stConf.stRule.nSensitivity < 1 || stConf.stRule.nSensitivity > 100)
        {
            dlog_error("设置人脸抓拍信息参数错误");
            return -1;
        }
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.stRule.stRegion);
        /* 坐标有效性判断 */
        if (!stConf.stRule.stRegion.IsValid())
        {
            dlog_error("设置人脸抓拍信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::FACE_CAPTURE;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_tripwire_count()
    {
        Alarm::BoundaryDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_tripwire_info(int nIndex, OnvifTripwireDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::BoundaryDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */ 
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        
        if (stConf.aRule[nIndex].enCrossDirection == Alarm::A_TO_B)
            snprintf(pInfo->achDirection, sizeof(pInfo->achDirection), "Left");
        else if (stConf.aRule[nIndex].enCrossDirection == Alarm::B_TO_A)
            snprintf(pInfo->achDirection, sizeof(pInfo->achDirection), "Right");
        else
            snprintf(pInfo->achDirection, sizeof(pInfo->achDirection), "Any");

        Alarm::Region_S stRegion;
        stRegion.aPoint.push_back(stConf.aRule[nIndex].stStartPos);
        stRegion.aPoint.push_back(stConf.aRule[nIndex].stEndPos);
        stRegion.nPointNum = 2;
        
        
        onvif_serialize_detection_target(stConf.aRule[nIndex].aDetectionTarget, pInfo->achDetectionTarget, sizeof(pInfo->achDetectionTarget));
        convert_region_to_onvif(stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_tripwire_info(int nIndex, OnvifTripwireDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::LINE_CROSSING, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::BoundaryDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        
        if (nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::BoundaryPlane_S()); 
                 stConf.aRule.back().nSensitivity = 50;
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置绊线侦测信息参数错误");
            return -1;
        }

        onvif_deserialize_detection_target(pInfo->achDetectionTarget, stConf.aRule[nIndex].aDetectionTarget);
        
        if (strcmp(pInfo->achDirection, "Left") == 0)
            stConf.aRule[nIndex].enCrossDirection = Alarm::A_TO_B;
        else if (strcmp(pInfo->achDirection, "Right") == 0)
            stConf.aRule[nIndex].enCrossDirection = Alarm::B_TO_A;
        else
            stConf.aRule[nIndex].enCrossDirection = Alarm::BOTH_WAYS;

        Alarm::Region_S stRegion;
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stRegion);
        /* 坐标有效性判断 */
        if (stRegion.aPoint.size() < 2)
        {
            dlog_error("设置绊线侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime);
        }
        if(stRegion.aPoint.size() >= 2) {
            stConf.aRule[nIndex].stStartPos = stRegion.aPoint[0];
            stConf.aRule[nIndex].stEndPos = stRegion.aPoint[1];
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::LINE_CROSSING;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_get_intrusion_count()
    {
        Alarm::FieldDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        return (int)stConf.aRule.size();
    }

    int onvif_get_intrusion_info(int nIndex, OnvifFieldDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        Alarm::FieldDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0 || nIndex >= (int)stConf.aRule.size()) return -1;
        
        pInfo->bEnable = stConf.bEnable; /* Global enable mapping */
        snprintf(pInfo->achSensitivity, sizeof(pInfo->achSensitivity), "%d", stConf.aRule[nIndex].nSensitivity);
        snprintf(pInfo->achTimeThreshold, sizeof(pInfo->achTimeThreshold), "%d", stConf.aRule[nIndex].nTimeThreshold);
        onvif_serialize_detection_target(stConf.aRule[nIndex].aDetectionTarget, pInfo->achDetectionTarget, sizeof(pInfo->achDetectionTarget));
        convert_region_to_onvif(stConf.aRule[nIndex].stRegion, pInfo->stPolygon, &pInfo->nPointNum);
        onvif_serialize_schedule(stConf.aAlarmTime, pInfo->achSchedule, sizeof(pInfo->achSchedule));
        return 0;
    }

    int onvif_set_intrusion_info(int nIndex, OnvifFieldDetection_S *pInfo)
    {
        if(!pInfo) return -1;
        if (pInfo->bEnableSet) {
            int ret = check_analytics_resource(Event::Type::INTRUSION, pInfo->bEnable);
            if (ret != 0) return ret;
        }
        Alarm::FieldDetection_S stConf;
        CEventConfigure::instance()->get_configure(stConf);
        if(nIndex < 0) return -1;
        if (nIndex >= (int)stConf.aRule.size()) {
             if (nIndex == (int)stConf.aRule.size()) {
                 stConf.aRule.push_back(Alarm::Intrusion_S()); 
                 stConf.aRule.back().nSensitivity = 50;
                 stConf.aRule.back().nTimeThreshold = 10;
                 // Initialize other default values if needed
             } else {
                 return -1;
             }
        }
        
        if (pInfo->bEnableSet) {
            stConf.bEnable = pInfo->bEnable; /* Global enable mapping */
        }
        stConf.aRule[nIndex].nSensitivity = atoi(pInfo->achSensitivity);
        stConf.aRule[nIndex].nTimeThreshold = atoi(pInfo->achTimeThreshold);
        /* 参数有效性判断 */
        if (stConf.aRule[nIndex].nTimeThreshold < 0 || stConf.aRule[nIndex].nTimeThreshold > 100 || 
            stConf.aRule[nIndex].nSensitivity < 1 || stConf.aRule[nIndex].nSensitivity > 100)
        {
            dlog_error("设置区域入侵侦测信息参数错误");
            return -1;
        }
        onvif_deserialize_detection_target(pInfo->achDetectionTarget, stConf.aRule[nIndex].aDetectionTarget);
        convert_onvif_to_region(pInfo->stPolygon, pInfo->nPointNum, stConf.aRule[nIndex].stRegion);
        /* 坐标有效性判断 */
        if (!stConf.aRule[nIndex].stRegion.IsValid())
        {
            dlog_error("设置区域入侵侦测信息的区域绘制异常");
            return -1;
        }

        if (strlen(pInfo->achSchedule) > 0) {
            onvif_deserialize_schedule(pInfo->achSchedule, stConf.aAlarmTime); // Note: schedule logic not previously present here?
            // Actually it wasn't in the snippet but usually pInfo has it.
            // Wait, previous snippet for Intrusion didn't show schedule deserialization?
            // Let's check existing code.
        }
        
        CEventConfigure::instance()->set_configure(stConf);

        /* 更新事件布防时间 */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::INTRUSION;
        stEventSchedule.bStatus = stConf.bEnable;
        stEventSchedule.defenseTime = stConf.aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
        return 0;
    }

    int onvif_renew_subscription(const char* strAddress, int nDurationSec, int* pCurrentSec)
    {
        if (strAddress == NULL)
        {
            return -1;
        }
        if(COnvifSubscriptionManager::instance()->renewSubscription(strAddress, nDurationSec, pCurrentSec))
        {
            return 0;
        }
        return -1;
    }

}
