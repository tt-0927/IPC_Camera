/***
 * @FilePath     : system_convert.cpp
 * @Author       : huangjunda
 * @Date         : 2025-03-28 10:31:43
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 10:32:14
 * @Description  :
 */
#include "system_convert.h"

#include "preview_convert.h"
#include "web_plugin_convert.h"
#include "user_convert.h"
#include "network_convert.h"
#include "alarm_convert.h"
#include "record_convert.h"
#include "capture_convert.h"
#include "storage_manage_convert.h"
#include "video_convert.h"
#include "audio_convert.h"
#include "isp_convert.h"
#include "osd_convert.h"
#include "common_convert.h"

#include "convert.h" /* 这个要放在UserDefineConvert的后面 */

void Convert::deal(Json::Object *pRootJson, System::DeviceInfo_S &stDeviceInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "DeviceName", stDeviceInfo.deviceName);
    convert.field(pRootJson, "DeviceID", stDeviceInfo.deviceID);
    convert.field(pRootJson, "UnitTpye", stDeviceInfo.strUnitTpye);
    convert.field(pRootJson, "SerialNumber", stDeviceInfo.serialNumber);
    convert.field(pRootJson, "HardwareVersion", stDeviceInfo.hardwareVersion);
    convert.field(pRootJson, "SystemVersion", stDeviceInfo.systemVersion);
    convert.field(pRootJson, "PluginVersion", stDeviceInfo.pluginVersion);
    convert.field(pRootJson, "WebVersion", stDeviceInfo.webVersion);
    convert.field(pRootJson, "AlarmInputCount", stDeviceInfo.nAlarmInputCount);
    convert.field(pRootJson, "AlarmOutputCount", stDeviceInfo.nAlarmOutputCount);
}
void Convert::deal(Json::Object *pRootJson, System::DeviceConfig_S &stDeviceConfig, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Language", (int &)stDeviceConfig.enLanguage);
    convert.field(pRootJson, "TimeZone", (int &)stDeviceConfig.enTimeZone);
    convert.field(pRootJson, "DateFormat", (int &)stDeviceConfig.enDateFormat);
    convert.field(pRootJson, "DateTime", stDeviceConfig.strDateTime);
    convert.field(pRootJson, "IsEnableOptPassword", stDeviceConfig.bEnableOptPassword);
    convert.field(pRootJson, "IsEanbleStartupWizard", stDeviceConfig.bEanbleStartupWizard);
    convert.field(pRootJson, "IsEanbleDecodeEnhance", stDeviceConfig.bEanbleDecodeEnhance);
}

void Convert::deal(Json::Object *pRootJson, System::LoginLock_S &stLoginInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "IllegalLoginEnable", stLoginInfo.bIllegalLoginEnable);
    convert.field(pRootJson, "CheckInterval", stLoginInfo.nCheckInterval);
    convert.field(pRootJson, "MaxErrorTimes", stLoginInfo.nMaxErrorTimes);
    convert.field(pRootJson, "LockDuration", (int &)stLoginInfo.nLockDuration);
}

void Convert::deal(Json::Object *pRootJson, System::PwdPolicy_S &stPwdPolicyInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "PwdSecurityLevelEnable", stPwdPolicyInfo.bPwdSecurityLevelEnable);
    convert.field(pRootJson, "AllowLowLevelPwdLogin", stPwdPolicyInfo.bAllowLowLevelPwdLogin);
}

void Convert::deal(Json::Object *pRootJson, System::SshAdmin_S &stSshInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "SshEnable", stSshInfo.bSshEnable);
    convert.field(pRootJson, "SshPort", stSshInfo.nSshPort);
    convert.field(pRootJson, "SshStartTime", stSshInfo.strSshStartTime);
    convert.field(pRootJson, "SshCountdown", stSshInfo.strSshCountdown);
}

void Convert::deal(Json::Object *pRootJson, System::SecurityServices_S &stSecurityInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson,stSecurityInfo.stLoginLock);
    convert.structure(pRootJson,stSecurityInfo.stPwdPolicy);
    convert.structure(pRootJson,stSecurityInfo.stSshAdmin);
}

void Convert::deal(Json::Object *pRootJson, System::SshCountdown_S &stSshInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Countdown", stSshInfo.strCountdown);
}

void Convert::deal(Json::Object *pRootJson, System::UpgradeMaintain_S &stUpgradeMaintain, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "CurrentVersion", stUpgradeMaintain.strCurrentVersion);
    convert.field(pRootJson, "AutoDownPack", stUpgradeMaintain.bAutoDownPack);
    convert.field(pRootJson, "AutoMaintain", stUpgradeMaintain.bAutoMaintain);
    convert.field(pRootJson, "Weekday", (int &)stUpgradeMaintain.enWeek);
    convert.field(pRootJson, "MaintainTime", stUpgradeMaintain.strMaintainTime);
}

void Convert::deal(Json::Object *pRootJson, System::NTPInfo_S &stNTPInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Address", stNTPInfo.address);
    convert.field(pRootJson, "Port", stNTPInfo.nPort);
    convert.field(pRootJson, "SyncInterval", stNTPInfo.nSyncInterval);
}

void Convert::deal(Json::Object *pRootJson, System::TimeInfo_S &stTimeInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "TimeZone", (int &)stTimeInfo.enTimeZone);
    convert.field(pRootJson, "DateFormat", (int &)stTimeInfo.enDateFormat);
    convert.field(pRootJson, "IsEnableNTPSync", stTimeInfo.bEnableNTPSync);
    convert.field(pRootJson, "IsManualSync", stTimeInfo.bManualSync);
    convert.field(pRootJson, "DateTime", stTimeInfo.strDateTime);
    convert.field(pRootJson, "IsSyncWithComputer", stTimeInfo.bIsSyncWithComputer);
    convert.structure(pRootJson, stTimeInfo.stNTPInfo);
}

void Convert::deal(Json::Object* pRootJson, System::TestNtp_S& stTestInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Address", stTestInfo.address);
    convert.field(pRootJson, "Port", stTestInfo.nPort);
}

void Convert::deal(Json::Object *pRootJson, System::SerialInfo_S &stSerialInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stSerialInfo.nChnId);
    convert.field(pRootJson, "BaudRate", stSerialInfo.nBaudRate);
    convert.field(pRootJson, "DataBits", stSerialInfo.nDataBits);
    convert.field(pRootJson, "StopBits", stSerialInfo.nStopBits);
    convert.field(pRootJson, "Parity", (int &)stSerialInfo.enParity);
    convert.field(pRootJson, "FlowControl", (int &)stSerialInfo.enFlowControl);
    convert.field(pRootJson, "CtrlMode", (int &)stSerialInfo.enCtrlMode);
}

void Convert::deal(Json::Object *pRootJson, System::SecurityCert_S &stSecurityCert, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RtspAuth", (int &)stSecurityCert.enRtspAuth);
    convert.field(pRootJson, "RtspDigestAlgorithm", (int &)stSecurityCert.enRtspDigestAlgorithm);
    convert.field(pRootJson, "WebAuth", (int &)stSecurityCert.enWebAuth);
    convert.field(pRootJson, "WebDigestAlgorithm", (int &)stSecurityCert.enWebDigestAlgorithm);
}

void Convert::deal(Json::Object *pRootJson, System::RealTime_S &stNowTime, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RealTime", stNowTime.strNowTime);
}

void Convert::deal(Json::Object *pRootJson, System::UpgradeInfo_S &stUpgradeInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Path", stUpgradeInfo.strUpgradePath);
}

void Convert::deal(Json::Object *pRootJson, System::UpgradeStatus_S &stStatus, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stStatus.nUpgradeStatus);
}

void Convert::deal(Json::Object* pRootJson, System::UpgradeVersion_S& stUpgradeVersion, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Version", stUpgradeVersion.strVersion);
}

void Convert::deal(Json::Object* pRootJson, System::LogServerInfo_S& stLogServerInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stLogServerInfo.bEnable);
    convert.field(pRootJson, "EnSsl", stLogServerInfo.bEnSsl);
    convert.field(pRootJson, "ServerAddr", stLogServerInfo.strServerAddr);
    convert.field(pRootJson, "Port", stLogServerInfo.nPort);
}

void Convert::deal(Json::Object *pRootJson, System::AllDeviceParam_S &stAllDeviceParam, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Password", stAllDeviceParam.strPassword);                        /* 密码 */
    /* 预览 */
    convert.structure(pRootJson, "DeviceInfo", stAllDeviceParam.stDeviceInfo);                 /* 预览 */
    /* 回放 */
    /* 图片 */
    /* 配置 */
    /* 本地设置 */
    convert.structure(pRootJson, "Param", stAllDeviceParam.stParam);                           /* 本地设置 */
    /* 系统配置 */
    convert.structure(pRootJson, "DeviceConfig", stAllDeviceParam.stDeviceConfig);             /* 系统设置-基本配置 */
    convert.structure(pRootJson, "DeviceInfo", stAllDeviceParam.stDeviceInfo);                 /* 系统设置-基本信息 */
    convert.structure(pRootJson, "TimeInfo", stAllDeviceParam.stTimeInfo);                     /* 系统设置-时间配置 */
                                                                                               /* 系统设置-智能资源分配 */
    convert.structure(pRootJson, "UpgradeMaintain", stAllDeviceParam.stUpgradeMaintain);       /* 系统维护-升级维护 */
    convert.structure(pRootJson, "LogServerInfo", stAllDeviceParam.stLogServerInfo);           /* 系统维护-安全审计日志 */
    convert.structure(pRootJson, "SecurityCert", stAllDeviceParam.stSecurityCert);             /* 安全管理-认证方式 */
    convert.structure(pRootJson, "IpFilterConfigInfo", stAllDeviceParam.stIpFilterConfigInfo); /* 安全管理-IP地址过滤 */
    convert.structure(pRootJson, "SecurityServices", stAllDeviceParam.stSecurityServices);     /* 安全管理-安全服务 */
                                                                                               /* 安全管理-客户端证书 */
                                                                                               /* 安全管理-CA证书 */
    convert.structure(pRootJson, "UserInfo", stAllDeviceParam.vecUserInfo);                    /* 用户管理-用户管理 */
                                                                                               /* 用户管理-在线用户 */
    /* 网络配置 */
    convert.structure(pRootJson, "NetInfo", stAllDeviceParam.stNetInfo);                       /* 基本配置-TCP/IP */
    convert.structure(pRootJson, "PortConfig", stAllDeviceParam.stPortConfig);                 /* 基本配置-端口 */
    convert.structure(pRootJson, "UpnpConfigInfo", stAllDeviceParam.stPortMapConfig);          /* 基本配置-端口映射 */
    convert.structure(pRootJson, "SnmpConfig", stAllDeviceParam.stSnmpConfig);                 /* 高级配置-SNMP */
    convert.structure(pRootJson, "EmailInfo", stAllDeviceParam.stEmailInfo);                   /* 高级配置-Email */
    convert.structure(pRootJson, "GB28181Client", stAllDeviceParam.stGB28181Client);           /* 高级配置-平台接入 */
    convert.structure(pRootJson, "GmCertFileInfo", stAllDeviceParam.stGmCertFileInfo);         /* 高级配置-国标证书管理 */
    convert.structure(pRootJson, "HttpsConfigInfo", stAllDeviceParam.stHttpsConfigInfo);       /* 高级配置-HTTPS */
    convert.structure(pRootJson, "QosConfigInfo", stAllDeviceParam.stQosConfigInfo);           /* 高级配置-Qos */
    convert.structure(pRootJson, "OnvifConfigInfo", stAllDeviceParam.stOnvifConfigInfo);       /* 高级配置-集成协议 */
                                                                                               /* 高级配置-网络服务 */
    /* 事件配置 */
    convert.structure(pRootJson, "MotionDetection", stAllDeviceParam.stMotionDetection);       /* 普通事件-移动侦测 */
    convert.structure(pRootJson, "HideAlarm", stAllDeviceParam.stHideAlarm);                   /* 普通事件-遮挡报警 */
    convert.structure(pRootJson, "AbnormalDetection", stAllDeviceParam.stAbnormalDetection);   /* 普通事件-异常 */
    convert.structure(pRootJson, "SoundOutputAlarm", stAllDeviceParam.stSoundOutputAlarm);     /* 普通事件-声音报警输出 */
    convert.structure(pRootJson, "IoInputInfo", stAllDeviceParam.stIoInputInfo);               /* 普通事件-报警输入 */
    convert.structure(pRootJson, "IoOutputInfo", stAllDeviceParam.stIoOutputInfo);             /* 普通事件-报警输出 */
    convert.structure(pRootJson, "FlashInfo", stAllDeviceParam.stFlashInfo);                   /* 普通事件-闪光灯报警输出 */
    convert.structure(pRootJson, "PirAlarmInfo", stAllDeviceParam.stPirAlarmInfo);             /* 普通事件-PIR报警 */
    convert.structure(pRootJson, "BoundaryDetection", stAllDeviceParam.stBoundaryDetection);   /* 周界事件-越界侦测 */
    convert.structure(pRootJson, "FieldDetection", stAllDeviceParam.stFieldDetection);         /* 周界事件-区域入侵侦测 */
    convert.structure(pRootJson, "EntranceDetection", stAllDeviceParam.stEntranceDetection);   /* 周界事件-进入区域侦测 */
    convert.structure(pRootJson, "ExitingDetection", stAllDeviceParam.stExitingDetection);     /* 周界事件-离开区域侦测 */
                                                                                               /* 场景智能-翻阅围栏识别 */
                                                                                               /* 场景智能-离岗识别 */
                                                                                               /* 场景智能-违规变道识别 */
                                                                                               /* 场景智能-逆行识别 */
                                                                                               /* 场景智能-非机动车闯入识别 */
                                                                                               /* 场景智能-应急车道占用识别 */
                                                                                               /* 场景智能-行人闯入识别 */
                                                                                               /* 场景智能-其他智能事件 */
    convert.structure(pRootJson, "AudioAnomaly", stAllDeviceParam.stAudioAnomaly);             /* Smart事件-音频异常侦测 */
    convert.structure(pRootJson, "SceneChange", stAllDeviceParam.stSceneChange);               /* Smart事件-场景变更侦测 */
    convert.structure(pRootJson, "FaceDetection", stAllDeviceParam.stFaceDetection);           /* Smart事件-人脸侦测 */
    convert.structure(pRootJson, "LoiteringDetection", stAllDeviceParam.stLoiteringDetection); /* Smart事件-徘徊侦测 */
    convert.structure(pRootJson, "CrowdGathering", stAllDeviceParam.stCrowdGathering);         /* Smart事件-人员聚集侦测 */
    convert.structure(pRootJson, "ParkingDetection", stAllDeviceParam.stParkingDetection);     /* Smart事件-停车侦测 */
    convert.structure(pRootJson, "UnattendedObject", stAllDeviceParam.stUnattendedObject);     /* Smart事件-物品遗留侦测 */
    convert.structure(pRootJson, "ObjectRemoval", stAllDeviceParam.stObjectRemoval);           /* Smart事件-物品拿取侦测 */
    convert.structure(pRootJson, "PetRecognition", stAllDeviceParam.stPetRecognition);         /* Smart事件-宠物侦测 */
    /* 存储管理 */
    convert.structure(pRootJson, "Schedule", stAllDeviceParam.stSchedule);                     /* 计划配置-录像计划 */
    convert.structure(pRootJson, "AdvancedParamParam", stAllDeviceParam.stAdvancedParamParam); /* 计划配置-录像计划-录像参数 */
    convert.structure(pRootJson, "CapturePlan", stAllDeviceParam.stCapturePlan);               /* 计划配置-抓图计划 */
    convert.structure(pRootJson, "CaptureParam", stAllDeviceParam.stCaptureParam);             /* 计划配置-抓图计划-抓图参数 */
    convert.structure(pRootJson, "StorageManage", stAllDeviceParam.stStorageManage);           /* 存储管理-轻存储 */
    /* 视音频 */
    convert.structure(pRootJson, "VideoConfig", stAllDeviceParam.vecVideoConfig);              /* 视频 */
    convert.structure(pRootJson, "AudioConfig", stAllDeviceParam.stAudioConfig);               /* 音频 */
    convert.structure(pRootJson, "VideoRoi", stAllDeviceParam.vecVideoRoi);                    /* ROI */
    convert.structure(pRootJson, "AreaCrop", stAllDeviceParam.vecAreaCrop);                    /* 区域裁剪 */
    /* 人脸抓拍 */
    convert.structure(pRootJson, "FaceCapture", stAllDeviceParam.stFaceCapture);               /* 人脸抓拍 */
    /* 图像管理 */
    convert.structure(pRootJson, "AllSceneParams", stAllDeviceParam.stAllSceneParams);         /* 显示设置 */
    convert.structure(pRootJson, "OsdConfig", stAllDeviceParam.stOsdConfig);                   /* OSD设置 */
    convert.structure(pRootJson, "CoverConfig", stAllDeviceParam.stCoverConfig);               /* 视频遮盖 */
    convert.structure(pRootJson, "SceneSchedule", stAllDeviceParam.stSceneSchedule);           /* 图像参数切换 */
    /* 校验码 */
    convert.structure(pRootJson, "CheckCode", stAllDeviceParam.strCheckCode);
}

void Convert::deal(Json::Object *pRootJson, System::DeviceParamFile_S &stDeviceParamFile, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Password", stDeviceParamFile.password);
    convert.field(pRootJson, "Path", stDeviceParamFile.path);
}

/*转换IP List*/
void Convert::deal(Json::Object *pRootJson,std::vector<std::string> &vecInfo,bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson,"IpList",vecInfo);
}

void Convert::deal(Json::Object* pRootJson, System::IpFilterInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Enable", stInfo.bEnable);
	convert.field(pRootJson, "IP", stInfo.strIp);
}

void Convert::deal(Json::Object* pRootJson, std::vector<System::IpFilterInfo_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "VecIps", stInfo);
}

void Convert::deal(Json::Object* pRootJson, System::IpFilterConfigInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Enable", stInfo.bEnable);
	convert.field(pRootJson, "Mode", (int &)stInfo.eMode);
	convert.structure(pRootJson, "VecIps", stInfo.vecIps);
}

void Convert::deal(Json::Object* pRootJson, std::string &strIp, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "IP", strIp);
}

void Convert::deal(Json::Object* pRootJson, System::IpFilterModify_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "OldIP", stInfo.strOldIp);
	convert.field(pRootJson, "NewIP", stInfo.strNewIp);
}

void Convert::deal(Json::Object* pRootJson, System::Peripheral_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Enable", stInfo.bEnable);
	convert.field(pRootJson, "Level", stInfo.nLevel);
    convert.field(pRootJson, "Mode", stInfo.nLightMode);
    convert.structure(pRootJson,"LightTime", stInfo.stLightTime);
}

