
/***
 * @FilePath     : system_convert.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-09 10:31:43
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-22 14:22:44
 * @Description  :
 */

#pragma once

#include <string>
#include <vector>

#include "Json.h"
#include "system_define.h"

namespace Convert
{
    void deal(Json::Object *pRootJson, System::DeviceInfo_S &stDeviceInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::DeviceConfig_S &stDeviceConfig, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::LoginLock_S &stLoginInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::PwdPolicy_S &stPwdPolicyInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::SshAdmin_S &stSshInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::SecurityServices_S &stSecurityInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::SshCountdown_S &stSshInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::UpgradeMaintain_S &UpgradeMaintain_S, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::NTPInfo_S &stNTPInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::TimeInfo_S &stTimeInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, System::TestNtp_S& stTestInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::SerialInfo_S &stSerialInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::SecurityCert_S &stSecurityCert, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::RealTime_S &stNowTime, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::UpgradeInfo_S &stUpgradeInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::UpgradeStatus_S &stStatus, bool bOutStruct);
    void deal(Json::Object* pRootJson, System::UpgradeVersion_S& stUpgradeVersion, bool bOutStruct);
    void deal(Json::Object* pRootJson, System::LogServerInfo_S& stLogServerInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::AllDeviceParam_S &stAllDeviceParam, bool bOutStruct);
    void deal(Json::Object *pRootJson, System::DeviceParamFile_S &stDeviceParamFile, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<std::string> &vecInfo,bool bOutStruct);
	void deal(Json::Object* pRootJson, System::IpFilterInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<System::IpFilterInfo_S> &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, System::IpFilterConfigInfo_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::string &strIp, bool bOutStruct);
	void deal(Json::Object* pRootJson, System::IpFilterModify_S &stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, System::Peripheral_S &stInfo, bool bOutStruct);

} // namespace Convert