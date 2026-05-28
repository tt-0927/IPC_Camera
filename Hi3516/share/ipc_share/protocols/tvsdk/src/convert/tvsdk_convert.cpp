/**
 * @FilePath     : tvsdk_convert.cpp
 * @Description  : IPC <-> TVSDK 结构体转换实现
 */

#include "tvsdk_convert.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>



#include "dlog.h"
#include "Json.h"
namespace TvSdkConvert
{

template <size_t N>
static void copy_string(CHAR (&dst)[N], const std::string &src)
{
    std::memset(dst, 0, N);
    std::strncpy(dst, src.c_str(), N - 1);
}

static void FillSchedTime(const Common::SchedTime_S &src, NET_TV_SCHED_TIME_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nStartHour   = (INT32)src.stStart.nHour;
    dst.nStartMinute = (INT32)src.stStart.nMinute;
    dst.nEndHour     = (INT32)src.stStop.nHour;
    dst.nEndMinute   = (INT32)src.stStop.nMinute;
    dlog_info("[TVSDK][Sched] FillSchedTime: %02d:%02d -> %02d:%02d",
              (int)dst.nStartHour, (int)dst.nStartMinute,
              (int)dst.nEndHour, (int)dst.nEndMinute);
}

static void ToSchedTime(const NET_TV_SCHED_TIME_S &src, Common::SchedTime_S &dst)
{
    dst.stStart.nHour   = (int)src.nStartHour;
    dst.stStart.nMinute = (int)src.nStartMinute;
    dst.stStart.nSecond = 0;

    dst.stStop.nHour   = (int)src.nEndHour;
    dst.stStop.nMinute = (int)src.nEndMinute;
    dst.stStop.nSecond = 0;
     dlog_info("[TVSDK][Sched] ToSchedTime: %02d:%02d -> %02d:%02d",
              dst.stStart.nHour, dst.stStart.nMinute,
              dst.stStop.nHour, dst.stStop.nMinute);
}

static void FillLinkageList(const Alarm::LinkageList_S &src, NET_TV_LINKAGE_LIST_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));

    // 报警输出
    dst.dwAlarmOutputCount = (INT32)std::min(src.alarmOutput.size(), (size_t)NET_TV_MAX_ALARM_OUT_NUM);
    for (size_t i = 0; i < (size_t)dst.dwAlarmOutputCount; ++i)
    {
        dst.adwAlarmOutput[i] = (INT32)src.alarmOutput[i];
    }

    // 录像通道
    dst.dwRecordChannelCount = (INT32)std::min(src.recordChn.size(), (size_t)NET_TV_CHANNEL_MAX);
    for (size_t i = 0; i < (size_t)dst.dwRecordChannelCount; ++i)
    {
        dst.adwRecordChannel[i] = (INT32)src.recordChn[i];
    }

    // 抓拍通道 (tradition 中与抓拍相关的联动类型映射)
    // tradition 包含 LinkageType_E 枚举值，UPLOAD_PANORAMIC_IMAGE(6)/UPLOAD_TARGET_IMAGE(7) -> 抓拍
    dst.dwSnapshotChannelCount = 0;
    for (size_t i = 0; i < src.tradition.size() && dst.dwSnapshotChannelCount < NET_TV_CHANNEL_MAX; ++i)
    {
        int type = src.tradition[i];
        if (type == Alarm::UPLOAD_PANORAMIC_IMAGE || type == Alarm::UPLOAD_TARGET_IMAGE)
        {
            dst.adwSnapshotChannel[dst.dwSnapshotChannelCount++] = (INT32)type;
        }
    }
}

static void ToLinkageList(const NET_TV_LINKAGE_LIST_S &src, Alarm::LinkageList_S &dst)
{
    dst.alarmOutput.clear();
    dst.recordChn.clear();
    dst.tradition.clear();

    // 报警输出
    for (INT32 i = 0; i < src.dwAlarmOutputCount && i < NET_TV_MAX_ALARM_OUT_NUM; ++i)
    {
        dst.alarmOutput.push_back((int)src.adwAlarmOutput[i]);
    }

    // 录像通道
    for (INT32 i = 0; i < src.dwRecordChannelCount && i < NET_TV_CHANNEL_MAX; ++i)
    {
        dst.recordChn.push_back((int)src.adwRecordChannel[i]);
    }

    // 抓拍 -> tradition
    for (INT32 i = 0; i < src.dwSnapshotChannelCount && i < NET_TV_CHANNEL_MAX; ++i)
    {
        dst.tradition.push_back((int)src.adwSnapshotChannel[i]);
    }
}

void FillDeviceInfo(const ::System::DeviceInfo_S &src, NET_TV_DEVICE_INFO_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.dwDevType        = (INT32)NET_TV_DTYPE_IPC;
    dst.wAlarmInPortNum  = (INT16)src.nAlarmInputCount;
    dst.wAlarmOutPortNum = (INT16)src.nAlarmOutputCount;
    dst.dwChannelNum     = 1;
}

void FillDeviceBasicInfo(const ::System::DeviceInfo_S &src, NET_TV_DEVICE_BASICINFO_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    strncpy(dst.szDevModel, src.strUnitTpye.c_str(), sizeof(dst.szDevModel) - 1);
    strncpy(dst.szDeviceTypeV2, src.strUnitTpye.c_str(), sizeof(dst.szDeviceTypeV2) - 1);
    strncpy(dst.szSerialNum, src.serialNumber.c_str(), sizeof(dst.szSerialNum) - 1);
    strncpy(dst.szFirmwareVersion, src.systemVersion.c_str(), sizeof(dst.szFirmwareVersion) - 1);
    strncpy(dst.szDeviceName, src.deviceName.c_str(), sizeof(dst.szDeviceName) - 1);
    strncpy(dst.szManufacturer, src.strUnitTpye.c_str(), sizeof(dst.szManufacturer) - 1);
}

void ToDeviceInfo(const NET_TV_DEVICE_BASICINFO_S &src, ::System::DeviceInfo_S &dst)
{
    dst.deviceName   = src.szDeviceName;
    dst.strUnitTpye  = src.szDevModel;
    dst.serialNumber = src.szSerialNum;
    dst.systemVersion = src.szFirmwareVersion;
}

void FillNetworkCfg(const Network::Info_S &src, NET_TV_NETWORKCFG_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.dwMTU = (INT32)src.stIp.nMtu;
    dst.bIPv4DHCP = src.stIp.bEnableDhcp ? TRUE : FALSE;
    strncpy(dst.szIpv4Address, src.stIp.ipv4Ip.c_str(), sizeof(dst.szIpv4Address) - 1);
    strncpy(dst.szIPv4GateWay, src.stIp.ipv4Gateway.c_str(), sizeof(dst.szIPv4GateWay) - 1);
    strncpy(dst.szIPv4SubnetMask, src.stIp.ipv4Mask.c_str(), sizeof(dst.szIPv4SubnetMask) - 1);
}

void ToNetworkInfo(const NET_TV_NETWORKCFG_S &src, Network::Info_S &dst)
{
    dst.stIp.bEnableDhcp = (src.bIPv4DHCP == TRUE);
    dst.stIp.ipv4Ip      = src.szIpv4Address;
    dst.stIp.ipv4Gateway = src.szIPv4GateWay;
    dst.stIp.ipv4Mask    = src.szIPv4SubnetMask;
    dst.stIp.nMtu        = (int)src.dwMTU;
}

void FillWifiStaCfg(const Network::WifiStaInfo_S &src, NET_TV_WIFI_STA_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnableWifi = src.bEnableWifi ? TRUE : FALSE;
    dst.bEnableBoost = src.bEnableBoost ? TRUE : FALSE;
}

void ToWifiStaInfo(const NET_TV_WIFI_STA_CFG_S &src, Network::WifiStaInfo_S &dst)
{
    dst.bEnableWifi = (src.bEnableWifi == TRUE);
    dst.bEnableBoost = (src.bEnableBoost == TRUE);
}

void FillWifiStaConnect(const Network::WifiStaConncet_S &src, NET_TV_WIFI_STA_CONNECT_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szSsid, src.ssid.c_str(), sizeof(dst.szSsid) - 1);
    dst.nSecurityMode = (INT32)src.mode;
    std::strncpy(dst.szIpAddress, src.ip_address.c_str(), sizeof(dst.szIpAddress) - 1);
    std::strncpy(dst.szPassword, src.password.c_str(), sizeof(dst.szPassword) - 1);
    std::strncpy(dst.szPairwise, src.pairwise.c_str(), sizeof(dst.szPairwise) - 1);
    dst.nWepKeyLen = src.wep_key_len;
    dst.bWepIsHex = src.wep_is_hex ? TRUE : FALSE;
    std::strncpy(dst.szAuthAlg, src.auth_alg.c_str(), sizeof(dst.szAuthAlg) - 1);

    const size_t nWepKeyCount = std::min<size_t>(src.wep_keys.size(), 4);
    dst.nWepKeyCount = (INT32)nWepKeyCount;
    for (size_t i = 0; i < nWepKeyCount; ++i)
    {
        dst.astWepKeys[i].nIndex = src.wep_keys[i].index;
        std::strncpy(dst.astWepKeys[i].szValue, src.wep_keys[i].value.c_str(), sizeof(dst.astWepKeys[i].szValue) - 1);
    }

    std::strncpy(dst.szEapIdentity, src.eap_identity.c_str(), sizeof(dst.szEapIdentity) - 1);
    std::strncpy(dst.szEapPassword, src.eap_password.c_str(), sizeof(dst.szEapPassword) - 1);
    std::strncpy(dst.szPeapVersion, src.peap_version.c_str(), sizeof(dst.szPeapVersion) - 1);
    std::strncpy(dst.szPhase2, src.phase2.c_str(), sizeof(dst.szPhase2) - 1);
    std::strncpy(dst.szAnonymousIdentity, src.anonymous_identity.c_str(), sizeof(dst.szAnonymousIdentity) - 1);
    std::strncpy(dst.szCaCertPath, src.ca_cert_path.c_str(), sizeof(dst.szCaCertPath) - 1);
    std::strncpy(dst.szPeapLabel, src.peap_label.c_str(), sizeof(dst.szPeapLabel) - 1);

    std::strncpy(dst.szTlsIdentity, src.tls_identity.c_str(), sizeof(dst.szTlsIdentity) - 1);
    std::strncpy(dst.szPrivateKeyPasswd, src.private_key_passwd.c_str(), sizeof(dst.szPrivateKeyPasswd) - 1);
    std::strncpy(dst.szEapolVersion, src.eapol_version.c_str(), sizeof(dst.szEapolVersion) - 1);
    std::strncpy(dst.szClientCertPath, src.client_cert_path.c_str(), sizeof(dst.szClientCertPath) - 1);
    std::strncpy(dst.szPrivateKeyPath, src.private_key_path.c_str(), sizeof(dst.szPrivateKeyPath) - 1);
    std::strncpy(dst.szCtrlInterface, src.ctrl_interface.c_str(), sizeof(dst.szCtrlInterface) - 1);
    std::strncpy(dst.szInterfaceName, src.interface_name.c_str(), sizeof(dst.szInterfaceName) - 1);
}

void ToWifiStaConnect(const NET_TV_WIFI_STA_CONNECT_S &src, Network::WifiStaConncet_S &dst)
{
    dst.ssid = src.szSsid;
    if (src.nSecurityMode >= (INT32)Network::WifiSecurityMode::WPA_PERSONAL &&
        src.nSecurityMode <= (INT32)Network::WifiSecurityMode::EAP_TLS)
    {
        dst.mode = (Network::WifiSecurityMode)src.nSecurityMode;
    }
    else
    {
        dst.mode = Network::WifiSecurityMode::OPEN;
    }

    dst.ip_address = src.szIpAddress;
    dst.password = src.szPassword;
    dst.pairwise = src.szPairwise;
    dst.wep_key_len = src.nWepKeyLen;
    dst.wep_is_hex = (src.bWepIsHex == TRUE);
    dst.auth_alg = src.szAuthAlg;

    dst.wep_keys.clear();
    const int nWepKeyCount = std::max(0, std::min(src.nWepKeyCount, 4));
    for (int i = 0; i < nWepKeyCount; ++i)
    {
        Network::WepKeyConfig stKey;
        stKey.index = src.astWepKeys[i].nIndex;
        stKey.value = src.astWepKeys[i].szValue;
        dst.wep_keys.push_back(stKey);
    }

    dst.eap_identity = src.szEapIdentity;
    dst.eap_password = src.szEapPassword;
    dst.peap_version = src.szPeapVersion;
    dst.phase2 = src.szPhase2;
    dst.anonymous_identity = src.szAnonymousIdentity;
    dst.ca_cert_path = src.szCaCertPath;
    dst.peap_label = src.szPeapLabel;

    dst.tls_identity = src.szTlsIdentity;
    dst.private_key_passwd = src.szPrivateKeyPasswd;
    dst.eapol_version = src.szEapolVersion;
    dst.client_cert_path = src.szClientCertPath;
    dst.private_key_path = src.szPrivateKeyPath;
    dst.ctrl_interface = src.szCtrlInterface;
    dst.interface_name = src.szInterfaceName;
}

void Fill4GInfo(const Network::Network_4G_Config_t &src, NET_TV_4G_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szApn, src.apn.c_str(), sizeof(dst.szApn) - 1);
    std::strncpy(dst.szUserName, src.username.c_str(), sizeof(dst.szUserName) - 1);
    std::strncpy(dst.szPassword, src.password.c_str(), sizeof(dst.szPassword) - 1);
    std::strncpy(dst.szCallNumber, src.call_number.c_str(), sizeof(dst.szCallNumber) - 1);
    dst.nMtu = src.mtu;
    dst.nAuthMode = src.auth_mode;
    dst.nNetworkMode = src.network_mode;
    dst.nDialMode = src.dial_mode;
}

void To4GConfig(const NET_TV_4G_INFO_S &src, Network::Network_4G_Config_t &dst)
{
    dst.apn = src.szApn;
    dst.username = src.szUserName;
    dst.password = src.szPassword;
    dst.call_number = src.szCallNumber;
    dst.mtu = src.nMtu;
    dst.auth_mode = src.nAuthMode;
    dst.network_mode = src.nNetworkMode;
    dst.dial_mode = src.nDialMode;
}

void FillHotspotInfo(const Network::HotspotConfig &src, NET_TV_HOTSPOT_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnabled = src.enabled ? TRUE : FALSE;
    std::strncpy(dst.szSsid, src.ssid.c_str(), sizeof(dst.szSsid) - 1);
    std::strncpy(dst.szSecurityMode, src.securityMode.c_str(), sizeof(dst.szSecurityMode) - 1);
    std::strncpy(dst.szEncryptionType, src.encryptionType.c_str(), sizeof(dst.szEncryptionType) - 1);
    std::strncpy(dst.szPassword, src.password.c_str(), sizeof(dst.szPassword) - 1);
    std::strncpy(dst.szConfirmPassword, src.confirmPassword.c_str(), sizeof(dst.szConfirmPassword) - 1);
}

void ToHotspotConfig(const NET_TV_HOTSPOT_INFO_S &src, Network::HotspotConfig &dst)
{
    dst.enabled = (src.bEnabled == TRUE);
    dst.ssid = src.szSsid;
    dst.securityMode = src.szSecurityMode;
    dst.encryptionType = src.szEncryptionType;
    dst.password = src.szPassword;
    dst.confirmPassword = src.szConfirmPassword;
}

static bool get_string_alias(Json::Object *pObj, const char *key1, const char *key2, std::string &out)
{
    if (!pObj)
        return false;
    if (key1 && Json::get(pObj, key1, out))
        return true;
    if (key2 && Json::get(pObj, key2, out))
        return true;
    return false;
}

static bool get_int_alias(Json::Object *pObj, const char *key1, const char *key2, int &out)
{
    if (!pObj)
        return false;
    if (key1 && Json::get(pObj, key1, out))
        return true;
    if (key2 && Json::get(pObj, key2, out))
        return true;
    return false;
}

bool FillHotspotConnInfoFromJson(const std::string &srcJson, NET_TV_HOTSPOT_CONN_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    if (srcJson.empty())
        return false;

    Json::Object *pRoot = Json::init(srcJson.c_str());
    if (!pRoot)
        return false;

    Json::Object *pData = Json::get(pRoot, "Data");
    if (!pData)
        pData = pRoot;

    std::string strStatus;
    int nTotal = 0;
    get_string_alias(pData, "status", "Status", strStatus);
    get_int_alias(pData, "total", "Total", nTotal);
    copy_string(dst.szStatus, strStatus);
    dst.nTotal = nTotal;

    Json::Object *pDevices = Json::get(pData, "devices");
    if (!pDevices)
        pDevices = Json::get(pData, "Devices");

    int nSize = pDevices ? Json::Array::size(pDevices) : 0;
    int nCount = std::max(0, std::min(nSize, (int)NET_TV_HOTSPOT_CONN_MAX_NUM));
    dst.nDeviceCount = nCount;
    if (dst.nTotal == 0)
        dst.nTotal = nSize;

    for (int i = 0; i < nCount; ++i)
    {
        Json::Object *pItem = Json::Array::get(pDevices, i);
        if (!pItem)
            continue;

        int nIndex = 0;
        std::string strMac;
        std::string strIp;
        std::string strConnTime;
        get_int_alias(pItem, "index", "Index", nIndex);
        get_string_alias(pItem, "mac", "Mac", strMac);
        get_string_alias(pItem, "ip", "Ip", strIp);
        get_string_alias(pItem, "conn_time", "ConnTime", strConnTime);

        dst.astDevices[i].nIndex = nIndex;
        copy_string(dst.astDevices[i].szMac, strMac);
        copy_string(dst.astDevices[i].szIp, strIp);
        copy_string(dst.astDevices[i].szConnTime, strConnTime);
    }

    Json::deinit(pRoot);
    return true;
}

void FillLoginLock(const ::System::LoginLock_S &src, NET_TV_LOGIN_LOCK_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bIllegalLoginEnable = src.bIllegalLoginEnable ? TRUE : FALSE;
    dst.nCheckInterval = (INT32)src.nCheckInterval;
    dst.nMaxErrorTimes = (INT32)src.nMaxErrorTimes;
    dst.nLockDuration = (INT32)src.nLockDuration;
}

void ToLoginLock(const NET_TV_LOGIN_LOCK_INFO_S &src, ::System::LoginLock_S &dst)
{
    dst.bIllegalLoginEnable = (src.bIllegalLoginEnable == TRUE);
    dst.nCheckInterval = (int)src.nCheckInterval;
    dst.nMaxErrorTimes = (int)src.nMaxErrorTimes;
    dst.nLockDuration = (::System::LockDuration_E)src.nLockDuration;
}

void FillPwdPolicy(const ::System::PwdPolicy_S &src, NET_TV_PWD_POLICY_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bPwdSecurityLevelEnable = src.bPwdSecurityLevelEnable ? TRUE : FALSE;
    dst.bAllowLowLevelPwdLogin = src.bAllowLowLevelPwdLogin ? TRUE : FALSE;
}

void ToPwdPolicy(const NET_TV_PWD_POLICY_INFO_S &src, ::System::PwdPolicy_S &dst)
{
    dst.bPwdSecurityLevelEnable = (src.bPwdSecurityLevelEnable == TRUE);
    dst.bAllowLowLevelPwdLogin = (src.bAllowLowLevelPwdLogin == TRUE);
}

void FillSshAdmin(const ::System::SshAdmin_S &src, NET_TV_SSH_ADMIN_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bSshEnable = src.bSshEnable ? TRUE : FALSE;
    dst.nSshPort = (INT32)src.nSshPort;
    copy_string(dst.szSshStartTime, src.strSshStartTime);
    copy_string(dst.szSshCountdown, src.strSshCountdown);
}

void ToSshAdmin(const NET_TV_SSH_ADMIN_INFO_S &src, ::System::SshAdmin_S &dst)
{
    dst.bSshEnable = (src.bSshEnable == TRUE);
    dst.nSshPort = (int)src.nSshPort;
    dst.strSshStartTime = src.szSshStartTime;
    dst.strSshCountdown = src.szSshCountdown;
}

void FillSecurityServices(const ::System::SecurityServices_S &src, NET_TV_SECURITY_SERVICES_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    FillLoginLock(src.stLoginLock, dst.stLoginLock);
    FillPwdPolicy(src.stPwdPolicy, dst.stPwdPolicy);
    FillSshAdmin(src.stSshAdmin, dst.stSshAdmin);
}

void ToSecurityServices(const NET_TV_SECURITY_SERVICES_INFO_S &src, ::System::SecurityServices_S &dst)
{
    ToLoginLock(src.stLoginLock, dst.stLoginLock);
    ToPwdPolicy(src.stPwdPolicy, dst.stPwdPolicy);
    ToSshAdmin(src.stSshAdmin, dst.stSshAdmin);
}

void FillSshCountdown(const ::System::SshCountdown_S &src, NET_TV_SSH_COUNTDOWN_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    copy_string(dst.szCountdown, src.strCountdown);
}

void FillLogServerInfo(const ::System::LogServerInfo_S &src, NET_TV_LOG_SERVER_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bEnSsl = src.bEnSsl ? TRUE : FALSE;
    copy_string(dst.szServerAddr, src.strServerAddr);
    dst.nPort = (INT32)src.nPort;
}

void ToLogServerInfo(const NET_TV_LOG_SERVER_INFO_S &src, ::System::LogServerInfo_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bEnSsl = (src.bEnSsl == TRUE);
    dst.strServerAddr = src.szServerAddr;
    dst.nPort = (int)src.nPort;
}

void FillPageInfo(const Common::PageInfo_S &src, NET_TV_PAGE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nCurPage = (INT32)src.nCurPage;
    dst.nPageSize = (INT32)src.nPageSize;
    dst.nDataTotal = (INT32)src.nDataTotal;
    dst.nPageTotal = (INT32)src.nPageTotal;
}

void ToPageInfo(const NET_TV_PAGE_INFO_S &src, Common::PageInfo_S &dst)
{
    dst.nCurPage = (int)src.nCurPage;
    dst.nPageSize = (int)src.nPageSize;
    dst.nDataTotal = (int)src.nDataTotal;
    dst.nPageTotal = (int)src.nPageTotal;
}

void ToLogRetrievalCond(const NET_TV_LOG_RETRIEVAL_COND_S &src, Log::RetrievalCond_S &dst)
{
    dst.enType = (Log::Type_E)src.nType;
    dst.enAction = (Log::Action_E)src.nAction;
    dst.startTime = src.szStartTime;
    dst.endTime = src.szEndTime;
}

static void FillLogInfo(const Log::Info_S &src, NET_TV_LOG_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    copy_string(dst.szStartTime, src.startTime);
    dst.nType = (INT32)src.nType;
    dst.nAction = (INT32)src.nAction;
    copy_string(dst.szChnName, src.chnName);
    copy_string(dst.szUser, src.user);
    copy_string(dst.szHost, src.host);
    copy_string(dst.szContext, src.context);
}

void FillLogList(const std::vector<Log::Info_S> &srcLogs, const Common::PageInfo_S &srcPage, NET_TV_LOG_LIST_S &dst)
{
    NET_TV_LOG_RETRIEVAL_COND_S stCond = dst.stCond;
    std::memset(&dst, 0, sizeof(dst));
    dst.stCond = stCond;
    FillPageInfo(srcPage, dst.stPage);

    const int nCount = std::max(0, std::min((int)srcLogs.size(), (int)NET_TV_LOG_QUERY_COND_NUM));
    dst.nLogCount = nCount;
    for (int i = 0; i < nCount; ++i)
    {
        FillLogInfo(srcLogs[(size_t)i], dst.astLogs[i]);
    }
}

void FillRecordInfo(const Record_NS::Info_S &src, NET_TV_RECORD_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nChnId = (INT32)src.nChnId;
    dst.nVideoStatus = (INT32)src.nVideoStatus;
    dst.nAudioStatus = (INT32)src.nAudioStatus;
    dst.nRecordStatus = (INT32)src.nRecordStatus;
    dst.nRecordFormat = (INT32)src.nRecordFormat;
    dst.nEventType = (INT32)src.nEventType;
    copy_string(dst.szPath, src.path);
    copy_string(dst.szRedunPath, src.redunPath);
    copy_string(dst.szRecordName, src.strRecordName);
    copy_string(dst.szRecordTime, src.strRecordTime);
    dst.nStreamType = (INT32)src.nStreamType;
}

void ToRecordInfo(const NET_TV_RECORD_INFO_S &src, Record_NS::Info_S &dst)
{
    dst.nChnId = (int)src.nChnId;
    dst.nVideoStatus = (int)src.nVideoStatus;
    dst.nAudioStatus = (int)src.nAudioStatus;
    dst.nRecordStatus = (int)src.nRecordStatus;
    dst.nRecordFormat = (int)src.nRecordFormat;
    dst.nEventType = (int)src.nEventType;
    dst.path = src.szPath;
    dst.redunPath = src.szRedunPath;
    dst.strRecordName = src.szRecordName;
    dst.strRecordTime = src.szRecordTime;
    dst.nStreamType = (int)src.nStreamType;
}

void FillRecordStatusInfo(const Record_NS::RecordStatusInfo_S &src, NET_TV_RECORD_STATUS_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nStatus = (INT32)src.enStatus;
}

void FillRecordTime(const Record_NS::RecordTime_S &src, NET_TV_RECORD_TIME_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nType = (INT32)src.nType;
    dst.nStartTime = (INT32)src.nStartTime;
    dst.nEndTime = (INT32)src.nEndTime;
}

void ToRecordTime(const NET_TV_RECORD_TIME_S &src, Record_NS::RecordTime_S &dst)
{
    dst.nType = (int)src.nType;
    dst.nStartTime = (int)src.nStartTime;
    dst.nEndTime = (int)src.nEndTime;
}

void FillRecordDaySchedule(const Record_NS::DaySchedule_S &src, NET_TV_RECORD_DAY_SCHEDULE_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nDayOfWeek = (INT32)src.enDayOfWeek;
    const int nCount = std::max(0, std::min((int)src.recordTimes.size(), (int)NET_TV_TIME_DURATION_NUM));
    dst.nRecordTimeCount = nCount;
    for (int i = 0; i < nCount; ++i)
    {
        FillRecordTime(src.recordTimes[(size_t)i], dst.astRecordTimes[i]);
    }
}

void ToRecordDaySchedule(const NET_TV_RECORD_DAY_SCHEDULE_S &src, Record_NS::DaySchedule_S &dst)
{
    dst.enDayOfWeek = (Record_NS::DayOfWeek_E)src.nDayOfWeek;
    dst.recordTimes.clear();
    int nCount = (int)src.nRecordTimeCount;
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nCount > NET_TV_TIME_DURATION_NUM)
    {
        nCount = NET_TV_TIME_DURATION_NUM;
    }
    dst.recordTimes.resize((size_t)nCount);
    for (int i = 0; i < nCount; ++i)
    {
        ToRecordTime(src.astRecordTimes[i], dst.recordTimes[(size_t)i]);
    }
}

void FillRecordSchedule(const Record_NS::Schedule_S &src, NET_TV_RECORD_SCHEDULE_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    const int nCount = std::max(0, std::min((int)src.daySchedules.size(), (int)NET_TV_PLAN_DAY_NUM_AWEEK));
    dst.nDayScheduleCount = nCount;
    for (int i = 0; i < nCount; ++i)
    {
        FillRecordDaySchedule(src.daySchedules[(size_t)i], dst.astDaySchedules[i]);
    }
}

void ToRecordSchedule(const NET_TV_RECORD_SCHEDULE_S &src, Record_NS::Schedule_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.daySchedules.clear();
    int nCount = (int)src.nDayScheduleCount;
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nCount > NET_TV_PLAN_DAY_NUM_AWEEK)
    {
        nCount = NET_TV_PLAN_DAY_NUM_AWEEK;
    }
    dst.daySchedules.resize((size_t)nCount);
    for (int i = 0; i < nCount; ++i)
    {
        ToRecordDaySchedule(src.astDaySchedules[i], dst.daySchedules[(size_t)i]);
    }
}

void FillRecordAdvancedParam(const Record_NS::AdvancedParam_S &src, NET_TV_RECORD_ADVANCED_PARAM_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bLoopWrite = src.bLoopWrite ? TRUE : FALSE;
    dst.nPreTime = (INT32)src.ePreTime;
    dst.nDelayTime = (INT32)src.eDelayTime;
    dst.nStreamType = (INT32)src.nStreamType;
}

void ToRecordAdvancedParam(const NET_TV_RECORD_ADVANCED_PARAM_S &src, Record_NS::AdvancedParam_S &dst)
{
    dst.bLoopWrite = (src.bLoopWrite == TRUE);
    dst.ePreTime = (Record_NS::RecordPreTime_E)src.nPreTime;
    dst.eDelayTime = (Record_NS::RecordDelayTime_E)src.nDelayTime;
    dst.nStreamType = (int)src.nStreamType;
}

void ToRecordFind(const NET_TV_RECORD_FIND_COND_S &src, Record_NS::Find_S &dst)
{
    dst.nChnId = (int)src.nChnId;
    dst.nType = (int)src.nType;
    dst.year = src.szYear;
    dst.month = src.szMonth;
    dst.date = src.szDate;
    dst.startTime = src.szStartTime;
    dst.endTime = src.szEndTime;
    dst.filename = src.szFilename;
}

void FillRecordVideoTime(const Record_NS::VideoTime_S &src, NET_TV_RECORD_VIDEO_TIME_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nStartTime = (INT32)src.nStartTime;
    dst.nEndTime = (INT32)src.nEndTime;
}

void FillRecordFindResult(const Record_NS::FindResult_S &src, NET_TV_RECORD_FIND_RESULT_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nChnId = (INT32)src.nChnId;
    int nDateCount = std::max(0, std::min((int)src.dates.size(), (int)NET_TV_RECORD_DATE_MAX_NUM));
    dst.nDateCount = nDateCount;
    for (int i = 0; i < nDateCount; ++i)
    {
        std::strncpy(dst.aszDates[i], src.dates[(size_t)i].c_str(), sizeof(dst.aszDates[i]) - 1);
    }
    copy_string(dst.szFilename, src.filename);
    int nVideoTimeCount = std::max(0, std::min((int)src.videoTimes.size(), (int)NET_TV_TIME_DURATION_NUM));
    dst.nVideoTimeCount = nVideoTimeCount;
    for (int i = 0; i < nVideoTimeCount; ++i)
    {
        FillRecordVideoTime(src.videoTimes[(size_t)i], dst.astVideoTimes[i]);
    }
}

void FillRecordFileList(const std::vector<Record_NS::FindResult_S> &src, NET_TV_RECORD_FILE_LIST_S &dst)
{
    NET_TV_RECORD_FIND_COND_S stFind = dst.stFind;
    std::memset(&dst, 0, sizeof(dst));
    dst.stFind = stFind;
    int nCount = std::max(0, std::min((int)src.size(), (int)NET_TV_RECORD_FILE_MAX_NUM));
    dst.nResultCount = nCount;
    for (int i = 0; i < nCount; ++i)
    {
        FillRecordFindResult(src[(size_t)i], dst.astResults[i]);
    }
}

void ToRecordDownloadList(const NET_TV_RECORD_DOWNLOAD_LIST_S &src, std::vector<Record_NS::DownloadInfo_S> &dst)
{
    dst.clear();
    int nCount = (int)src.nDownloadCount;
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
    {
        nCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
    }
    dst.reserve((size_t)nCount);
    for (int i = 0; i < nCount; ++i)
    {
        Record_NS::DownloadInfo_S stInfo;
        stInfo.nChnId = (int)src.astDownloads[i].nChnId;
        stInfo.path = src.astDownloads[i].szPath;
        stInfo.startTime = src.astDownloads[i].szStartTime;
        stInfo.endTime = src.astDownloads[i].szEndTime;
        dst.push_back(stInfo);
    }
}

void FillRecordDownloadProgress(const Record_NS::DownloadProgress_S &src, NET_TV_RECORD_DOWNLOAD_PROGRESS_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    copy_string(dst.szFilename, src.filename);
    dst.nProgress = (INT32)src.nProgress;
}

void FillRecordDownloadListProgress(const std::vector<Record_NS::DownloadProgress_S> &src, NET_TV_RECORD_DOWNLOAD_LIST_S &dst)
{
    int nCount = std::max(0, std::min((int)src.size(), (int)NET_TV_RECORD_DOWNLOAD_MAX_NUM));
    dst.nProgressCount = nCount;
    for (int i = 0; i < nCount; ++i)
    {
        FillRecordDownloadProgress(src[(size_t)i], dst.astProgress[i]);
    }
}

static INT32 ToSdkVideoCodec(Video_NS::VideoCodec_E src)
{
    switch (src)
    {
        case Video_NS::VideoCodec_E::H264:
            return NET_TV_VIDEO_CODE_H264;
        case Video_NS::VideoCodec_E::H265:
            return NET_TV_VIDEO_CODE_H265;
        case Video_NS::VideoCodec_E::MJPEG:
            return NET_TV_VIDEO_CODE_MJPEG;
        default:
            return NET_TV_VIDEO_CODE_INVALID;
    }
}

static Video_NS::VideoCodec_E ToIpcVideoCodec(INT32 src)
{
    switch (src)
    {
        case NET_TV_VIDEO_CODE_H264:
            return Video_NS::VideoCodec_E::H264;
        case NET_TV_VIDEO_CODE_H265:
            return Video_NS::VideoCodec_E::H265;
        case NET_TV_VIDEO_CODE_MJPEG:
            return Video_NS::VideoCodec_E::MJPEG;
        default:
            return Video_NS::VideoCodec_E::H264;
    }
}

void FillVideoEncodeOption(const Video_NS::VideoConfig_S &src, NET_TV_VIDEO_ENCODE_OPTION_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nId = (INT32)src.nId;
    dst.enVideoType = (INT32)src.enVideoType;
    dst.stVideoResolution.dwWidth = (INT32)src.stVideoResolution.nWidth;
    dst.stVideoResolution.dwHeight = (INT32)src.stVideoResolution.nHeight;
    dst.enBitrateType = (INT32)src.enBitrateType;
    dst.enImageQuality = (INT32)src.enImageQuality;
    dst.enFrameRate = (INT32)src.getFrameRateAsInt();
    dst.nBitrateUpperLimit = (INT32)src.nBitrateUpperLimit;
    dst.nAverageBitrate = (INT32)src.nAverageBitrate;
    dst.enVideoCodec = ToSdkVideoCodec(src.enVideoCodec);
    dst.bSmartEnable = src.bSmartEnable ? TRUE : FALSE;
    dst.enEncodingComplexity = (INT32)src.enEncodingComplexity;
    dst.nIFrameInterval = (INT32)src.nIFrameInterval;
    dst.enSvcEnable = (INT32)src.enSvcEnable;
    dst.nBitrateSmoothing = (INT32)src.nBitrateSmoothing;
}

void ToVideoConfig(const NET_TV_VIDEO_ENCODE_OPTION_S &src, Video_NS::VideoConfig_S &dst)
{
    dst.nId = (int)src.nId;
    dst.enVideoType = (Video_NS::VideoType_E)src.enVideoType;
    dst.stVideoResolution.nWidth = (int)src.stVideoResolution.dwWidth;
    dst.stVideoResolution.nHeight = (int)src.stVideoResolution.dwHeight;
    dst.enBitrateType = (Video_NS::BitrateType_E)src.enBitrateType;
    dst.enImageQuality = (Video_NS::ImageQuality_E)src.enImageQuality;
    dst.setFrameRate((int)src.enFrameRate);
    dst.nBitrateUpperLimit = (int)src.nBitrateUpperLimit;
    dst.nAverageBitrate = (int)src.nAverageBitrate;
    dst.enVideoCodec = ToIpcVideoCodec(src.enVideoCodec);
    dst.bSmartEnable = (src.bSmartEnable == TRUE);
    dst.enEncodingComplexity = (Video_NS::EncodingComplexity_E)src.enEncodingComplexity;
    dst.nIFrameInterval = (int)src.nIFrameInterval;
    dst.enSvcEnable = (Video_NS::SvcMode_E)src.enSvcEnable;
    dst.nBitrateSmoothing = (int)src.nBitrateSmoothing;
}



void FillPreviewInfo(const Preview::PreviewInfo_S &src, NET_TV_PREVIEW_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szRtspMainUrl, src.stRtspUrl.strRtspMainUrl.c_str(), sizeof(dst.szRtspMainUrl) - 1);
    std::strncpy(dst.szRtspSubUrl, src.stRtspUrl.strRtspSubUrl.c_str(), sizeof(dst.szRtspSubUrl) - 1);
    dst.nBrightness = src.stImageParam.nBrightness;
    dst.nContrast = src.stImageParam.nContrast;
    dst.nSaturation = src.stImageParam.nSaturation;
    dst.nSharpness = src.stImageParam.nSharpness;
}

void ToPreviewInfo(const NET_TV_PREVIEW_INFO_S &src, Preview::PreviewInfo_S &dst)
{
    dst.stRtspUrl.strRtspMainUrl = src.szRtspMainUrl;
    dst.stRtspUrl.strRtspSubUrl = src.szRtspSubUrl;
    dst.stImageParam.nBrightness = src.nBrightness;
    dst.stImageParam.nContrast = src.nContrast;
    dst.stImageParam.nSaturation = src.nSaturation;
    dst.stImageParam.nSharpness = src.nSharpness;
}

static void ParseResolutionName(const std::string &name, NET_TV_VIDEO_RESOLUTION_S &dst)
{
    int width = 0;
    int height = 0;
    if (std::sscanf(name.c_str(), "%d*%d", &width, &height) == 2 ||
        std::sscanf(name.c_str(), "%dx%d", &width, &height) == 2)
    {
        dst.dwWidth = (INT32)width;
        dst.dwHeight = (INT32)height;
    }
}

static INT32 ToSdkFrameRate(Video_NS::FrameRate_E frameRate)
{
    Video_NS::VideoConfig_S cfg;
    cfg.enFrameRate = frameRate;
    return (INT32)cfg.getFrameRateAsInt();
}

static float ToFloatFrameRate(Video_NS::FrameRate_E frameRate)
{
    Video_NS::VideoConfig_S cfg;
    cfg.enFrameRate = frameRate;
    return cfg.getFrameRateAsFloat();
}

static bool IsValidFrameRate(Video_NS::FrameRate_E frameRate)
{
    return frameRate > Video_NS::FRAME_RATE_ALL && frameRate < Video_NS::FRAME_RATE_TOTAL;
}

static std::vector<Video_NS::FrameRate_E> BuildSupportedFrameRates(const Video_NS::Resolution_S &src)
{
    std::vector<Video_NS::FrameRate_E> frameRates;

    if (!src.aFrameRates.empty())
    {
        for (size_t i = 0; i < src.aFrameRates.size(); ++i)
        {
            if (IsValidFrameRate(src.aFrameRates[i]))
            {
                frameRates.push_back(src.aFrameRates[i]);
            }
        }
    }
    else
    {
        float minFrameRate = ToFloatFrameRate(src.enFrameRateMin);
        float maxFrameRate = ToFloatFrameRate(src.enFrameRateMax);
        if (minFrameRate > maxFrameRate)
        {
            std::swap(minFrameRate, maxFrameRate);
        }

        for (int value = (int)Video_NS::FRAME_RATE_ALL + 1; value < (int)Video_NS::FRAME_RATE_TOTAL; ++value)
        {
            Video_NS::FrameRate_E frameRate = (Video_NS::FrameRate_E)value;
            float currentFrameRate = ToFloatFrameRate(frameRate);
            if (currentFrameRate >= minFrameRate && currentFrameRate <= maxFrameRate)
            {
                frameRates.push_back(frameRate);
            }
        }
    }

    std::sort(frameRates.begin(), frameRates.end(),
              [](Video_NS::FrameRate_E left, Video_NS::FrameRate_E right) {
                  return ToFloatFrameRate(left) < ToFloatFrameRate(right);
              });

    frameRates.erase(std::unique(frameRates.begin(), frameRates.end(),
                                 [](Video_NS::FrameRate_E left, Video_NS::FrameRate_E right) {
                                     return ToSdkFrameRate(left) == ToSdkFrameRate(right);
                                 }),
                     frameRates.end());

    return frameRates;
}

static void FillResolutionCap(const Video_NS::Resolution_S &src, NET_TV_VIDEO_RESOLUTION_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    ParseResolutionName(src.strName, dst);
    dst.dwFrameRateMin = ToSdkFrameRate(src.enFrameRateMin);
    dst.dwFrameRateMax = ToSdkFrameRate(src.enFrameRateMax);
    dst.dwBitRateMin = (INT32)src.nBitRateMin;
    dst.dwBitRateMax = (INT32)src.nBitRateMax;

    std::vector<Video_NS::FrameRate_E> frameRates = BuildSupportedFrameRates(src);
    dst.dwFrameRateNum = (INT32)std::min(frameRates.size(), (size_t)NET_TV_VIDEO_FRAME_RATE_MAX_NUM);
    for (INT32 i = 0; i < dst.dwFrameRateNum; ++i)
    {
        dst.adwFrameRate[i] = ToSdkFrameRate(frameRates[(size_t)i]);
    }
}

static INT32 ClampInt(int value, int minValue, int maxValue)
{
    return (INT32)std::max(minValue, std::min(value, maxValue));
}

static void FillOneEncodeOption(const Video_NS::VideoCapability_S &src,
                                const Video_NS::EncodeAbility_S *ability,
                                NET_TV_VIDEO_ENCODE_OPTION_S &dst,
                                INT32 streamType)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nId = streamType;
    dst.enVideoType = (INT32)Video_NS::VideoType_E::COMPOSITE_STREAM;
    dst.enBitrateType = (INT32)Video_NS::BitrateType_E::CBR;
    dst.enImageQuality = (INT32)Video_NS::ImageQuality_E::MEDIUM;

    if (!src.aResolution.empty())
    {
        FillResolutionCap(src.aResolution[0], dst.stVideoResolution);
        dst.enFrameRate = dst.stVideoResolution.dwFrameRateMax;
        dst.nAverageBitrate = ClampInt(DEFAULTE_BITRATE,
                                       (int)dst.stVideoResolution.dwBitRateMin,
                                       (int)dst.stVideoResolution.dwBitRateMax);
        dst.nBitrateUpperLimit = dst.stVideoResolution.dwBitRateMax;
    }

    if (ability)
    {
        dst.enVideoCodec = ToSdkVideoCodec(Video_NS::string_toVideoCodec(ability->strVideoCodec));
        dst.bSmartEnable = FALSE;
        dst.enEncodingComplexity = (INT32)ability->nDefaultComplexity;
        dst.enSvcEnable = ability->bSupportSVC ? Video_NS::SVC_MODE_ENABLE : Video_NS::SVC_MODE_DISABLE;
        dst.nBitrateSmoothing = ability->bSupportStreamSmooth ? src.nStreamSmoothMax : 0;
    }
    else
    {
        dst.enVideoCodec = NET_TV_VIDEO_CODE_H264;
        dst.enEncodingComplexity = (INT32)Video_NS::EncodingComplexity_E::Main;
        dst.enSvcEnable = Video_NS::SVC_MODE_DISABLE;
    }

    dst.nIFrameInterval = ClampInt(DEFAULTE_GOP, src.nIFrameIntervalMin, src.nIFrameIntervalMax);
}

static void FillOneStreamCap(const Video_NS::VideoCapability_S &src, NET_TV_VIDEO_STREAM_CAP_S &dst, INT32 streamType)
{
    memset(&dst, 0, sizeof(dst));
    dst.dwStreamType = streamType;
    dst.bSupportMultiStream = (INT32)src.bSupportMultiStream;
    dst.dwEncodeCapSize = (INT32)std::min(src.aEncodeAbility.size(), (size_t)NET_TV_VIDEO_ENCODE_TYPE_MAX);
    if (dst.dwEncodeCapSize == 0)
    {
        dst.dwEncodeCapSize = 1;
        FillOneEncodeOption(src, nullptr, dst.astEncodeCap[0], streamType);
    }
    else
    {
        for (INT32 i = 0; i < dst.dwEncodeCapSize; ++i)
        {
            FillOneEncodeOption(src, &src.aEncodeAbility[i], dst.astEncodeCap[i], streamType);
        }
    }

    // 填充完整分辨率列表
    dst.dwResolutionNum = (INT32)std::min(src.aResolution.size(),
                                          (size_t)NET_TV_RESOLUTION_NUM_MAX);
    for (INT32 i = 0; i < dst.dwResolutionNum; ++i)
    {
        FillResolutionCap(src.aResolution[i], dst.astResolution[i]);
    }

    dst.stQuality.dwMin = (INT32)Video_NS::ImageQuality_E::LOWEST;
    dst.stQuality.dwMax = (INT32)Video_NS::ImageQuality_E::HIGHEST;
    dst.stStreamSmooth.dwMin = src.nStreamSmoothMin;
    dst.stStreamSmooth.dwMax = src.nStreamSmoothMax;
}

void FillVideoEncodeCap(const Video_NS::VideoCapabilitySet_S &src, NET_TV_VIDEO_ENCODE_CAP_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.dwStreamCount = 0;
    FillOneStreamCap(src.stMain, dst.astStreamCap[0], NET_TV_LIVE_STREAM_INDEX_MAIN);
    dst.dwStreamCount++;
    FillOneStreamCap(src.stSub, dst.astStreamCap[1], NET_TV_LIVE_STREAM_INDEX_AUX);
    dst.dwStreamCount++;
}

static void FillOneAudioFormatCap(const Audio_NS::AudioFormatCapability_S &src,
                                  NET_TV_AUDIO_FORMAT_CAP_S &dst)
{
    memset(&dst, 0, sizeof(dst));

    dst.dwFormat = (INT32)Audio_NS::string_toAudioFormat(src.strFormat);

    for (size_t i = 0; i < src.aSampleRates.size() && i < NET_TV_AUDIO_SAMPRATE_MAX; ++i)
    {
        dst.adwSampleRate[i] = (INT32)src.aSampleRates[i];
        dst.dwSampleRateSize++;
    }

    for (size_t i = 0; i < src.aBitRates.size() && i < NET_TV_AUDIO_BITRATE_MAX; ++i)
    {
        dst.adwBitRate[i] = (INT32)src.aBitRates[i];
        dst.dwBitRateSize++;
    }

    dst.stSampleRateRange.bEnable = src.stSampleRateRange.bEnable ? 1 : 0;
    dst.stSampleRateRange.dwMin   = src.stSampleRateRange.nMin;
    dst.stSampleRateRange.dwMax   = src.stSampleRateRange.nMax;
    dst.stSampleRateRange.dwStep  = src.stSampleRateRange.nStep;

    dst.stBitRateRange.bEnable = src.stBitRateRange.bEnable ? 1 : 0;
    dst.stBitRateRange.dwMin   = src.stBitRateRange.nMin;
    dst.stBitRateRange.dwMax   = src.stBitRateRange.nMax;
    dst.stBitRateRange.dwStep  = src.stBitRateRange.nStep;
}

void FillAudioEncodeCap(const Audio_NS::AudioCapabilitySet_S &src,
                        NET_TV_AUDIO_CAP_S &dst)
{
    memset(&dst, 0, sizeof(dst));

    for (size_t i = 0; i < src.aInputTypes.size() && i < NET_TV_AUDIO_INPUT_TYPE_MAX; ++i)
    {
        dst.adwInputType[i] = (INT32)Audio_NS::string_toAudioInputType(src.aInputTypes[i]);
        dst.dwInputTypeSize++;
    }

    for (size_t i = 0; i < src.aOutputTypes.size() && i < NET_TV_AUDIO_OUTPUT_TYPE_MAX; ++i)
    {
        dst.adwOutputType[i] = (INT32)Audio_NS::string_toAudioOutputType(src.aOutputTypes[i]);
        dst.dwOutputTypeSize++;
    }

    for (size_t i = 0; i < src.aFormats.size() && i < NET_TV_AUDIO_FORMAT_MAX; ++i)
    {
        dst.adwFormat[i] = (INT32)Audio_NS::string_toAudioFormat(src.aFormats[i]);
        dst.dwFormatSize++;
    }

    for (size_t i = 0; i < src.aFormatDetail.size() && i < NET_TV_AUDIO_FORMAT_MAX; ++i)
    {
        FillOneAudioFormatCap(src.aFormatDetail[i], dst.astFormatDetail[i]);
        dst.dwFormatDetailSize++;
    }
}

// --------- Motion (IPC MotionDetection_S <-> SDK NET_TV_MOTION_ALARM_INFO_S) ---------
void FillMotionAlarmInfo(const Alarm::MotionDetection_S &src, NET_TV_MOTION_ALARM_INFO_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bDynamicAnalysisEnable = src.bDynamicAnalysisEnable ? TRUE : FALSE;
    dst.dwMode = (INT32)src.enMode; // 0 normal, 1 expert

    // Normal mode
    dst.stNormalMode.nSensitivity = (INT32)src.stMotionNormalMode.nSensitivity;
    dst.stNormalMode.nRegionType  = (INT32)src.stMotionNormalMode.nRegionType;
    // NOTE:
    // 这里先做最小安全填充：不访问 IPC 侧 variant/grid 具体内容，避免在不同编译宏/ABI 组合下触发崩溃。
    // 若后续确认稳定，可再逐步恢复“矩形/网格区域”的精确映射。
    dst.stNormalMode.nRectLeft = 0;
    dst.stNormalMode.nRectTop = 0;
    dst.stNormalMode.nRectRight = 0;
    dst.stNormalMode.nRectBottom = 0;
    dst.stNormalMode.dwGridWidth = 22;
    dst.stNormalMode.dwGridHeight = 18;
    // 普通模式区域：筒型(Rect) 或 网格(abyGridArea)
    if (dst.stNormalMode.nRegionType == 0)
    {
        if (std::holds_alternative<Common::Rect_S>(src.stMotionNormalMode.varRegion))
        {
            const Common::Rect_S &r = std::get<Common::Rect_S>(src.stMotionNormalMode.varRegion);
            dst.stNormalMode.nRectLeft = r.nX;
            dst.stNormalMode.nRectTop = r.nY;
            dst.stNormalMode.nRectRight = r.nX + r.nWidth;
            dst.stNormalMode.nRectBottom = r.nY + r.nHeight;
        }
    }
    else if (dst.stNormalMode.nRegionType == 1)
    {
        // 默认全 0，只有网格中标记为 1 的宏块才置 1
        std::memset(dst.stNormalMode.abyGridArea, 0, sizeof(dst.stNormalMode.abyGridArea));

        if (std::holds_alternative<Alarm::MotionNormalMode_S::AreaGrid>(src.stMotionNormalMode.varRegion))
        {
            const auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(src.stMotionNormalMode.varRegion);
            int h = (int)std::min<size_t>(grid.size(), 18);
            int w = 0;
            if (h > 0)
                w = (int)std::min<size_t>(grid[0].size(), 22);

            // 尝试按 IPC 网格实际尺寸填充；不足按 18x22 默认
            if (w > 0)
            {
                dst.stNormalMode.dwGridHeight = h;
                dst.stNormalMode.dwGridWidth = w;
                for (int y = 0; y < h; ++y)
                {
                    int rowW = (int)std::min<size_t>(grid[y].size(), (size_t)w);
                    for (int x = 0; x < rowW; ++x)
                    {
                        dst.stNormalMode.abyGridArea[y][x] = (grid[y][x] != 0) ? 1 : 0;
                    }
                }
            }
        }
    }

    // Expert mode: map first 16 regions
    dst.stExpertMode.nExpertDayNightCtrl = (INT32)src.stMotionExpertMode.nExpertDayNightCtrl;
    // stDayTime schedule: SDK uses NET_TV_SCHED_TIME_S, IPC uses Common::SchedTime_S, leave default
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; day++)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
    dst.stExpertMode.dwRegionCount = 0;
    for (size_t i = 0; i < src.stMotionExpertMode.vstMotionRegion.size() && i < 16; ++i)
    {
        const auto &reg = src.stMotionExpertMode.vstMotionRegion[i];
        auto &out = dst.stExpertMode.astRegion[i];
        memset(&out, 0, sizeof(out));
        out.nAreaNo = (INT32)reg.nAreaNo;
        out.nRectLeft   = reg.stRect.nX;
        out.nRectTop    = reg.stRect.nY;
        out.nRectRight  = reg.stRect.nX + reg.stRect.nWidth;
        out.nRectBottom = reg.stRect.nY + reg.stRect.nHeight;
        out.nCloseSensitivity   = (INT32)reg.nCloseSensitivity;
        out.nDaytimeSensitivity = (INT32)reg.nDaytimeSensitivity;
        out.nNightSensitivity   = (INT32)reg.nNightSensitivity;
        dst.stExpertMode.dwRegionCount++;
    }
}

void ToMotionDetection(const NET_TV_MOTION_ALARM_INFO_S &src, Alarm::MotionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bDynamicAnalysisEnable = (src.bDynamicAnalysisEnable == TRUE);
    dst.enMode = (Alarm::MotionType_E)src.dwMode;

    dst.stMotionNormalMode.nSensitivity = (unsigned int)src.stNormalMode.nSensitivity;
    dst.stMotionNormalMode.nRegionType  = (unsigned int)src.stNormalMode.nRegionType;

    // 只做最小安全回写：矩形区域可回写，网格区域暂不回写（保持 IPC 默认）
    if (dst.stMotionNormalMode.nRegionType == 0)
    {
        Common::Rect_S r;
        r.nX = src.stNormalMode.nRectLeft;
        r.nY = src.stNormalMode.nRectTop;
        r.nWidth  = src.stNormalMode.nRectRight - src.stNormalMode.nRectLeft;
        r.nHeight = src.stNormalMode.nRectBottom - src.stNormalMode.nRectTop;
        dst.stMotionNormalMode.varRegion = r;
    }
    else if (dst.stMotionNormalMode.nRegionType == 1)
    {
        // 网格区域：从 SDK abyGridArea 还原为 IPC AreaGrid（默认 18x22，按 dwGrid* 裁剪）
        int h = src.stNormalMode.dwGridHeight;
        int w = src.stNormalMode.dwGridWidth;
        if (h <= 0 || h > 18)
            h = 18;
        if (w <= 0 || w > 22)
            w = 22;

        Alarm::MotionNormalMode_S::AreaGrid grid((size_t)h, std::vector<unsigned int>((size_t)w, 0));
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                grid[y][x] = (src.stNormalMode.abyGridArea[y][x] != 0) ? 1U : 0U;
            }
        }
        dst.stMotionNormalMode.varRegion = grid;
    }
    // 布防时间：用 SDK 一周×8 段回写 IPC 的 aAlarmTime（只填有值的时间段）
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- Tamper (IPC HideAlarm_S <-> SDK NET_TV_TAMPER_ALARM_INFO_S) ---------
void FillTamperAlarmInfo(const Alarm::HideAlarm_S &src, NET_TV_TAMPER_ALARM_INFO_S &dst)
{
     memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwSensitivity = (INT32)src.nSensitivity;
    dst.nRectLeft   = src.stRect.nX;
    dst.nRectTop    = src.stRect.nY;
    dst.nRectRight  = src.stRect.nX + src.stRect.nWidth;
    dst.nRectBottom = src.stRect.nY + src.stRect.nHeight;

    // 布防时间：一周 × 最多 8 段
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; day++)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToHideAlarm(const NET_TV_TAMPER_ALARM_INFO_S &src, Alarm::HideAlarm_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.dwSensitivity;
    dst.stRect.nX = src.nRectLeft;
    dst.stRect.nY = src.nRectTop;
    dst.stRect.nWidth  = src.nRectRight - src.nRectLeft;
    dst.stRect.nHeight = src.nRectBottom - src.nRectTop;

    // 布防时间回写
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- CrossLine (IPC BoundaryDetection_S <-> SDK NET_TV_CROSS_LINE_ALARM_INFO_S) ---------
void FillCrossLineAlarmInfo(const Alarm::BoundaryDetection_S &src, NET_TV_CROSS_LINE_ALARM_INFO_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.fStartPosX = r.stStartPos.fX;
        out.fStartPosY = r.stStartPos.fY;
        out.fEndPosX   = r.stEndPos.fX;
        out.fEndPosY   = r.stEndPos.fY;
        out.enCrossDirection = (INT32)r.enCrossDirection;
        out.nSensitivity = (INT32)r.nSensitivity;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
            out.adwDetectionTarget[j] = r.aDetectionTarget[j];
        dst.dwRuleCount++;
    }

     // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToBoundaryDetection(const NET_TV_CROSS_LINE_ALARM_INFO_S &src, Alarm::BoundaryDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::BoundaryPlane_S out;
        out.stStartPos = { r.fStartPosX, r.fStartPosY };
        out.stEndPos   = { r.fEndPosX, r.fEndPosY };
        out.enCrossDirection = (Alarm::CrossDirection_E)r.enCrossDirection;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- Intrusion (IPC FieldDetection_S <-> SDK NET_TV_INTRUSION_ALARM_INFO_S) ---------
void FillIntrusionAlarmInfo(const Alarm::FieldDetection_S &src, NET_TV_INTRUSION_ALARM_INFO_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.dwPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.dwPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity   = (INT32)r.nSensitivity;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
            out.adwDetectionTarget[j] = r.aDetectionTarget[j];
        dst.dwRuleCount++;
    }

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToFieldDetection(const NET_TV_INTRUSION_ALARM_INFO_S &src, Alarm::FieldDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::Intrusion_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.dwPointCount;
        for (int p = 0; p < r.dwPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({ r.afPointX[p], r.afPointY[p] });
        }
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- Loitering (IPC LoiteringDetection_S <-> SDK NET_TV_LOITERING_ALARM_INFO_S) ---------
void FillLoiteringAlarmInfo(const Alarm::LoiteringDetection_S &src, NET_TV_LOITERING_ALARM_INFO_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.dwPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.dwPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity   = (INT32)r.nSensitivity;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
            out.adwDetectionTarget[j] = r.aDetectionTarget[j];
        dst.dwRuleCount++;
    }

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToLoiteringDetection(const NET_TV_LOITERING_ALARM_INFO_S &src, Alarm::LoiteringDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::LoiteringRule_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.dwPointCount;
        for (int p = 0; p < r.dwPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({ r.afPointX[p], r.afPointY[p] });
        }
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

static void FillPolygonPoints(const Alarm::Region_S &src, INT32 &pointCount, FLOAT pointX[32], FLOAT pointY[32])
{
    pointCount = (INT32)std::min<size_t>(src.aPoint.size(), 32);
    for (int p = 0; p < pointCount; ++p)
    {
        pointX[p] = src.aPoint[p].fX;
        pointY[p] = src.aPoint[p].fY;
    }
}

static void ToRegionFromPolygon(INT32 pointCount, const FLOAT pointX[32], const FLOAT pointY[32], Alarm::Region_S &dst)
{
    int cnt = pointCount;
    if (cnt < 0)
        cnt = 0;
    if (cnt > 32)
        cnt = 32;

    dst.aPoint.clear();
    dst.nPointNum = (unsigned int)cnt;
    for (int p = 0; p < cnt; ++p)
    {
        Common::PosF_S pt;
        pt.fX = pointX[p];
        pt.fY = pointY[p];
        dst.aPoint.push_back(pt);
    }
}

// --------- SceneChange (IPC SceneChange_S <-> SDK NET_TV_SCENE_CHANGE_ALARM_INFO_S) ---------
void FillSceneChangeAlarmInfo(const Alarm::SceneChange_S &src, NET_TV_SCENE_CHANGE_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToSceneChange(const NET_TV_SCENE_CHANGE_ALARM_INFO_S &src, Alarm::SceneChange_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- CrowdGathering (IPC CrowdGathering_S <-> SDK NET_TV_CROWD_GATHERING_ALARM_INFO_S) ---------
void FillCrowdGatheringAlarmInfo(const Alarm::CrowdGathering_S &src, NET_TV_CROWD_GATHERING_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nObjectOccup = (INT32)r.nObjectOccup;
        dst.dwRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToCrowdGathering(const NET_TV_CROWD_GATHERING_ALARM_INFO_S &src, Alarm::CrowdGathering_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::CrowdGatheringRule_S out;
        ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nObjectOccup = (unsigned int)r.nObjectOccup;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT

// --------- GarbageExposure (IPC Alarm::GarbageExposureDetection_S <-> SDK NET_TV_GARBAGE_EXPOSURE_CFG_S) ---------
void FillGarbageExposureCfg(const Alarm::GarbageExposureDetection_S &src, NET_TV_GARBAGE_EXPOSURE_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillPolygonPoints(src.stRule.stRegion, dst.stRule.dwPointCount, dst.stRule.afPointX, dst.stRule.afPointY);

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }

    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToGarbageExposure(const NET_TV_GARBAGE_EXPOSURE_CFG_S &src, Alarm::GarbageExposureDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToRegionFromPolygon(src.stRule.dwPointCount, src.stRule.afPointX, src.stRule.afPointY, dst.stRule.stRegion);

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }

    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

// --------- GarbageOverflow (IPC Alarm::GarbageOverflowDetection_S <-> SDK NET_TV_GARBAGE_OVERFLOW_CFG_S) ---------
void FillGarbageOverflowCfg(const Alarm::GarbageOverflowDetection_S &src, NET_TV_GARBAGE_OVERFLOW_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillPolygonPoints(src.stRule.stRegion, dst.stRule.dwPointCount, dst.stRule.afPointX, dst.stRule.afPointY);

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }

    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToGarbageOverflow(const NET_TV_GARBAGE_OVERFLOW_CFG_S &src, Alarm::GarbageOverflowDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToRegionFromPolygon(src.stRule.dwPointCount, src.stRule.afPointX, src.stRule.afPointY, dst.stRule.stRegion);

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }

    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

#endif

static void FillSingleRuleAlarmSchedule(const Alarm::DefenseTime &src, NET_TV_ALARM_SCHEDULE_S &dst)
{
    if (!src.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.size())
                break;

            const auto &vecDay = src[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.astTimeSection[day][seg]);
            }
        }
    }
}

static void ToSingleRuleAlarmSchedule(const NET_TV_ALARM_SCHEDULE_S &src, Alarm::DefenseTime &dst)
{
    dst.clear();
    dst.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;

        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.astTimeSection[day][seg], dst[day][seg]);
        }
    }
}

#ifdef SCENE_INTELLIGENCE
void FillManholeCoverAbnormalCfg(const Alarm::ManholeCoverAbnormalDetection_S &src, NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToManholeCoverAbnormal(const NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S &src, Alarm::ManholeCoverAbnormalDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSleepOnDutyCfg(const Alarm::SleepOnDutyDetection_S &src, NET_TV_SLEEP_ON_DUTY_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSleepOnDuty(const NET_TV_SLEEP_ON_DUTY_CFG_S &src, Alarm::SleepOnDutyDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillElectricVehicleInElevatorCfg(const Alarm::ElectricScooterDetection_S &src, NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToElectricVehicleInElevator(const NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S &src, Alarm::ElectricScooterDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPersonFallDownCfg(const Alarm::PersonFallDownDetection_S &src, NET_TV_PERSON_FALL_DOWN_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPersonFallDown(const NET_TV_PERSON_FALL_DOWN_CFG_S &src, Alarm::PersonFallDownDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillConstructionOccupyRoadCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &src, NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToConstructionOccupyRoad(const NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S &src, Alarm::ConstructionEncroachmentRoadDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillCongestionCfg(const Alarm::CongestionDetection_S &src, NET_TV_CONGESTION_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToCongestion(const NET_TV_CONGESTION_CFG_S &src, Alarm::CongestionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillLicensePlateRecognitionCfg(const Alarm::LicensePlateCognitionDetection_S &src, NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToLicensePlateRecognition(const NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S &src, Alarm::LicensePlateCognitionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillHighAltitudeSeatbeltCfg(const Alarm::HighAltitudeSeatbeltDetection_S &src, NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToHighAltitudeSeatbelt(const NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S &src, Alarm::HighAltitudeSeatbeltDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSafetyHelmetCfg(const Alarm::SafetyHelmetDection_S &src, NET_TV_SAFETY_HELMET_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSafetyHelmet(const NET_TV_SAFETY_HELMET_CFG_S &src, Alarm::SafetyHelmetDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPersonFallCfg(const Alarm::TripDetection_S &src, NET_TV_PERSON_FALL_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPersonFall(const NET_TV_PERSON_FALL_CFG_S &src, Alarm::TripDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPhoneUsageCfg(const Alarm::PhoneUsageDetection_S &src, NET_TV_PHONE_USAGE_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPhoneUsage(const NET_TV_PHONE_USAGE_CFG_S &src, Alarm::PhoneUsageDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSmokingCfg(const Alarm::SmokingDection_S &src, NET_TV_SMOKING_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSmoking(const NET_TV_SMOKING_CFG_S &src, Alarm::SmokingDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillOpenFlameCfg(const Alarm::OpenFlameDetection_S &src, NET_TV_OPEN_FLAME_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToOpenFlame(const NET_TV_OPEN_FLAME_CFG_S &src, Alarm::OpenFlameDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillBareSoilCfg(const Alarm::BareSoiletDection_S &src, NET_TV_BARE_SOIL_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToBareSoil(const NET_TV_BARE_SOIL_CFG_S &src, Alarm::BareSoiletDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillHoleProtectionBarCfg(const Alarm::HoleProtectionBarDection_S &src, NET_TV_HOLE_PROTECTION_BAR_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToHoleProtectionBar(const NET_TV_HOLE_PROTECTION_BAR_CFG_S &src, Alarm::HoleProtectionBarDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillReflectiveClothingCfg(const Alarm::ReflectiveClothingDection_S &src, NET_TV_REFLECTIVE_CLOTHING_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToReflectiveClothing(const NET_TV_REFLECTIVE_CLOTHING_CFG_S &src, Alarm::ReflectiveClothingDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPetRecognitionInfo(const Alarm::PetRecognition_S &src, NET_TV_PET_RECOGNITION_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bDynamicAnalysisEnable = src.bDynamicAnalysisEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;
    FillPolygonPoints(src.stRegion, dst.stRegion.dwPointCount, dst.stRegion.afPointX, dst.stRegion.afPointY);
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPetRecognition(const NET_TV_PET_RECOGNITION_INFO_S &src, Alarm::PetRecognition_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bDynamicAnalysisEnable = (src.bDynamicAnalysisEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;
    ToRegionFromPolygon(src.stRegion.dwPointCount, src.stRegion.afPointX, src.stRegion.afPointY, dst.stRegion);
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillClimbFenceInfo(const Alarm::FenceClimbingDetection_S &src, NET_TV_CLIMB_FENCE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_REGION_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
        {
            out.adwDetectionTarget[j] = (INT32)r.aDetectionTarget[j];
        }
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToClimbFence(const NET_TV_CLIMB_FENCE_INFO_S &src, Alarm::FenceClimbingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_REGION_RULE_S &r = src.astRule[i];
        Alarm::FenceClimbingRule_S out;
        ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillDimissionInfo(const Alarm::LeavePostDetection_S &src, NET_TV_DIMISSION_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_REGION_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
        {
            out.adwDetectionTarget[j] = (INT32)r.aDetectionTarget[j];
        }
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToDimission(const NET_TV_DIMISSION_INFO_S &src, Alarm::LeavePostDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_REGION_RULE_S &r = src.astRule[i];
        Alarm::LeavePostRule_S out;
        ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillIllegalLaneInfo(const Alarm::IllegalLaneChangeDetection_S &src, NET_TV_ILLEGAL_LANE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_LINE_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.fStartPosX = r.stStartPos.fX;
        out.fStartPosY = r.stStartPos.fY;
        out.fEndPosX = r.stEndPos.fX;
        out.fEndPosY = r.stEndPos.fY;
        out.enCrossDirection = (INT32)r.enCrossDirection;
        out.nSensitivity = (INT32)r.nSensitivity;
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToIllegalLane(const NET_TV_ILLEGAL_LANE_INFO_S &src, Alarm::IllegalLaneChangeDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_LINE_RULE_S &r = src.astRule[i];
        Alarm::IllegalLaneChangeRule_S out;
        out.stStartPos = {r.fStartPosX, r.fStartPosY};
        out.stEndPos = {r.fEndPosX, r.fEndPosY};
        out.enCrossDirection = (Alarm::CrossDirection_E)r.enCrossDirection;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillRetrogradeInfo(const Alarm::DrivingAgainstTrafficDetection_S &src, NET_TV_RETROGRADE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_LINE_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.fStartPosX = r.stStartPos.fX;
        out.fStartPosY = r.stStartPos.fY;
        out.fEndPosX = r.stEndPos.fX;
        out.fEndPosY = r.stEndPos.fY;
        out.enCrossDirection = (INT32)r.enCrossDirection;
        out.nSensitivity = (INT32)r.nSensitivity;
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToRetrograde(const NET_TV_RETROGRADE_INFO_S &src, Alarm::DrivingAgainstTrafficDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_LINE_RULE_S &r = src.astRule[i];
        Alarm::DrivingAgainstTrafficRule_S out;
        out.stStartPos = {r.fStartPosX, r.fStartPosY};
        out.stEndPos = {r.fEndPosX, r.fEndPosY};
        out.enCrossDirection = (Alarm::CrossDirection_E)r.enCrossDirection;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillNonmotorVehicleIntrusionInfo(const Alarm::NonMotorVehicleIntrusionDetection_S &src, NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_REGION_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
        {
            out.adwDetectionTarget[j] = (INT32)r.aDetectionTarget[j];
        }
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToNonmotorVehicleIntrusion(const NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S &src, Alarm::NonMotorVehicleIntrusionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_REGION_RULE_S &r = src.astRule[i];
        Alarm::NonMotorVehicleIntrusionRule_S out;
        ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillOccupationEmergencyInfo(const Alarm::EmergencyLaneOccupancyDetection_S &src, NET_TV_OCCUPATION_EMERGENCY_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_REGION_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
        {
            out.adwDetectionTarget[j] = (INT32)r.aDetectionTarget[j];
        }
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToOccupationEmergency(const NET_TV_OCCUPATION_EMERGENCY_INFO_S &src, Alarm::EmergencyLaneOccupancyDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_REGION_RULE_S &r = src.astRule[i];
        Alarm::EmergencyLaneOccupancyRule_S out;
        ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPedestrianIntrusionInfo(const Alarm::PedestrianIntrusionDetection_S &src, NET_TV_PEDESTRIAN_INTRUSION_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_TV_SMART_REGION_RULE_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
        {
            out.adwDetectionTarget[j] = (INT32)r.aDetectionTarget[j];
        }
        dst.dwRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPedestrianIntrusion(const NET_TV_PEDESTRIAN_INTRUSION_INFO_S &src, Alarm::PedestrianIntrusionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const NET_TV_SMART_REGION_RULE_S &r = src.astRule[i];
        Alarm::PedestrianIntrusionRule_S out;
        ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSmokeFireCfg(const Alarm::SmokeFireDetection_S &src, NET_TV_SMOKE_FIRE_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSmokeFire(const NET_TV_SMOKE_FIRE_CFG_S &src, Alarm::SmokeFireDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillRoadPondingCfg(const Alarm::RoadPondingDetection_S &src, NET_TV_ROAD_PONDING_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToRoadPonding(const NET_TV_ROAD_PONDING_CFG_S &src, Alarm::RoadPondingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

#endif

// --------- AudioAnomaly (IPC AudioAnomaly_S <-> SDK NET_TV_AUDIO_ANOMALY_ALARM_INFO_S) ---------
void FillAudioAnomalyAlarmInfo(const Alarm::AudioAnomaly_S &src, NET_TV_AUDIO_ANOMALY_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bAudioInputAnomaly = src.bAudioInputAnomaly ? TRUE : FALSE;
    dst.bUpEnable = src.bUpEnable ? TRUE : FALSE;
    dst.nUpSensitivity = (INT32)src.nUpSensitivity;
    dst.nUpThreshold = (INT32)src.nUpThreshold;
    dst.bDownEnable = src.bDownEnable ? TRUE : FALSE;
    dst.nDownSensitivity = (INT32)src.nDownSensitivity;

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToAudioAnomaly(const NET_TV_AUDIO_ANOMALY_ALARM_INFO_S &src, Alarm::AudioAnomaly_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bAudioInputAnomaly = (src.bAudioInputAnomaly == TRUE);
    dst.bUpEnable = (src.bUpEnable == TRUE);
    dst.nUpSensitivity = (unsigned int)src.nUpSensitivity;
    dst.nUpThreshold = (unsigned int)src.nUpThreshold;
    dst.bDownEnable = (src.bDownEnable == TRUE);
    dst.nDownSensitivity = (unsigned int)src.nDownSensitivity;

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}



#if CAP_AI_PEOPLE_STATISTICS
// --------- PeopleFlowStatistics (IPC Alarm::PeopleFlowStatistics_S <-> SDK NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S) ---------
static void FillPeopleAlarmRule(const Alarm::PopulationAlarmRule_S &src, NET_TV_PEOPLE_ALARM_RULE_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nThreshold = (INT32)src.nThreshold;
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

static void ToPeopleAlarmRule(const NET_TV_PEOPLE_ALARM_RULE_S &src, Alarm::PopulationAlarmRule_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nThreshold = (unsigned int)src.nThreshold;
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPeopleFlowStatisticsCfg(const Alarm::PeopleFlowStatistics_S &src, NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;

    // 规则线
    dst.stRuleLine.fStartPointX = src.stRuleLine.stStartPos.fX;
    dst.stRuleLine.fStartPointY = src.stRuleLine.stStartPos.fY;
    dst.stRuleLine.fEndPointX = src.stRuleLine.stEndPos.fX;
    dst.stRuleLine.fEndPointY = src.stRuleLine.stEndPos.fY;
    dst.stRuleLine.nDirection = (INT32)src.stRuleLine.enDirection;

    // 检测区域
    FillPolygonPoints(src.stDetectRegion, dst.dwPointCount, dst.afPointX, dst.afPointY);

    dst.nReportInterval = (INT32)src.nReportInterval;
    dst.enStatisticsType = (INT32)src.enStatisticsType;

    // 定时清零
    dst.stTimedReset.bEnable = src.stTimedReset.bEnable ? TRUE : FALSE;
    dst.stTimedReset.nHour = (INT32)src.stTimedReset.stExecuteTime.nHour;
    dst.stTimedReset.nMinute = (INT32)src.stTimedReset.stExecuteTime.nMinute;

    // 三级报警
    FillPeopleAlarmRule(src.stStayAlarm.stNormal, dst.stStayAlarm.stNormal);
    FillPeopleAlarmRule(src.stStayAlarm.stMedium, dst.stStayAlarm.stMedium);
    FillPeopleAlarmRule(src.stStayAlarm.stSevere, dst.stStayAlarm.stSevere);

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToPeopleFlowStatistics(const NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S &src, Alarm::PeopleFlowStatistics_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;

    // 规则线
    dst.stRuleLine.stStartPos.fX = src.stRuleLine.fStartPointX;
    dst.stRuleLine.stStartPos.fY = src.stRuleLine.fStartPointY;
    dst.stRuleLine.stEndPos.fX = src.stRuleLine.fEndPointX;
    dst.stRuleLine.stEndPos.fY = src.stRuleLine.fEndPointY;
    dst.stRuleLine.enDirection = (Alarm::CrossDirection_E)src.stRuleLine.nDirection;

    // 检测区域
    ToRegionFromPolygon(src.dwPointCount, src.afPointX, src.afPointY, dst.stDetectRegion);

    dst.nReportInterval = (unsigned int)src.nReportInterval;
    dst.enStatisticsType = (Alarm::PeopleFlowStatisticsType_E)src.enStatisticsType;

    // 定时清零
    dst.stTimedReset.bEnable = (src.stTimedReset.bEnable == TRUE);
    dst.stTimedReset.stExecuteTime.nHour = (int)src.stTimedReset.nHour;
    dst.stTimedReset.stExecuteTime.nMinute = (int)src.stTimedReset.nMinute;
    dst.stTimedReset.stExecuteTime.nSecond = 0;

    // 三级报警
    ToPeopleAlarmRule(src.stStayAlarm.stNormal, dst.stStayAlarm.stNormal);
    ToPeopleAlarmRule(src.stStayAlarm.stMedium, dst.stStayAlarm.stMedium);
    ToPeopleAlarmRule(src.stStayAlarm.stSevere, dst.stStayAlarm.stSevere);

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- PeopleDensityDetection (IPC Alarm::PeopleDensityDetection_S <-> SDK NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S) ---------
void FillPeopleDensityDetectionCfg(const Alarm::PeopleDensityDetection_S &src, NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;

    // 检测区域
    FillPolygonPoints(src.stDetectRegion, dst.dwPointCount, dst.afPointX, dst.afPointY);

    dst.nReportInterval = (INT32)src.nReportInterval;

    // 三级报警
    FillPeopleAlarmRule(src.stDensityAlarm.stNormal, dst.stDensityAlarm.stNormal);
    FillPeopleAlarmRule(src.stDensityAlarm.stMedium, dst.stDensityAlarm.stMedium);
    FillPeopleAlarmRule(src.stDensityAlarm.stSevere, dst.stDensityAlarm.stSevere);

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToPeopleDensityDetection(const NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S &src, Alarm::PeopleDensityDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;

    // 检测区域
    ToRegionFromPolygon(src.dwPointCount, src.afPointX, src.afPointY, dst.stDetectRegion);

    dst.nReportInterval = (unsigned int)src.nReportInterval;

    // 三级报警
    ToPeopleAlarmRule(src.stDensityAlarm.stNormal, dst.stDensityAlarm.stNormal);
    ToPeopleAlarmRule(src.stDensityAlarm.stMedium, dst.stDensityAlarm.stMedium);
    ToPeopleAlarmRule(src.stDensityAlarm.stSevere, dst.stDensityAlarm.stSevere);

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}
#endif

void ToUpgradeInfo(const NET_TV_UPGRADE_INFO_S &src, ::System::UpgradeInfo_S &dst)
{
    dst.strUpgradePath = src.szUpgradePath;
}

void FillUpgradeStatus(const ::System::UpgradeStatus_S &src, NET_TV_UPGRADE_STATUS_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nUpgradeStatus = (INT32)src.nUpgradeStatus;
}

void FillUpgradeVersion(const ::System::UpgradeVersion_S &src, NET_TV_UPGRADE_VERSION_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szVersion, src.strVersion.c_str(), sizeof(dst.szVersion) - 1);
    dst.szVersion[sizeof(dst.szVersion) - 1] = '\0';
}

static void FillOneCaptureConfig(const Capture_NS::CaptureConfig_S &src, NET_TV_CAPTURE_CONFIG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.enPictureFormat = (INT32)src.enPictureFormat;
    dst.nWidth = (INT32)src.stVideoResolution.nWidth;
    dst.nHeight = (INT32)src.stVideoResolution.nHeight;
    dst.enImageQuality = (INT32)src.enImageQuality;
    dst.unInterval = src.stTimeInterval.unInterval;
    dst.enTimeUnit = (INT32)src.stTimeInterval.enTimeUnit;
    dst.unNumber = src.unNumber;
}

static void ToOneCaptureConfig(const NET_TV_CAPTURE_CONFIG_S &src, Capture_NS::CaptureConfig_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.enPictureFormat = (Capture_NS::PictureFormat_E)src.enPictureFormat;
    dst.stVideoResolution.nWidth = src.nWidth;
    dst.stVideoResolution.nHeight = src.nHeight;
    dst.enImageQuality = (Capture_NS::ImageQuality_E)src.enImageQuality;
    dst.stTimeInterval.unInterval = src.unInterval;
    dst.stTimeInterval.enTimeUnit = (Capture_NS::TimeUnit_E)src.enTimeUnit;
    dst.unNumber = src.unNumber;
}

void FillCapturePlan(const Capture_NS::CapturePlan_S &src, NET_TV_CAPTURE_PLAN_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    for (size_t i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
    {
        dst.astDaySchedules[i].nDayOfWeek = (INT32)(i + 1);
        dst.astDaySchedules[i].udwTimeCount = 1;
        dst.astDaySchedules[i].astTimes[0].nStartTime = 0;
        dst.astDaySchedules[i].astTimes[0].nEndTime = 24 * 60 * 60;
    }

    const size_t dayCount = src.vstDaySchedules.size();
    for (size_t i = 0; i < dayCount; ++i)
    {
        const Capture_NS::DaySchedule_S &day = src.vstDaySchedules[i];
        int nDayOfWeek = (int)day.enDayOfWeek;
        if (nDayOfWeek < 1 || nDayOfWeek > (int)NET_TV_PLAN_DAY_NUM_AWEEK)
            continue;
        NET_TV_CAPTURE_DAY_SCHEDULE_S &outDay = dst.astDaySchedules[(size_t)nDayOfWeek - 1];
        outDay.nDayOfWeek = (INT32)nDayOfWeek;

        const size_t timeCount = day.captureTimes.size();
        const size_t n = (timeCount < NET_TV_PLAN_TIME_SECTION_NUM_ADAY) ? timeCount : NET_TV_PLAN_TIME_SECTION_NUM_ADAY;
        outDay.udwTimeCount = (UINT32)n;
        if (n == 0)
            continue;

        for (size_t j = 0; j < n; ++j)
        {
            outDay.astTimes[j].nStartTime = (INT32)day.captureTimes[j].nStartTime;
            outDay.astTimes[j].nEndTime = (INT32)day.captureTimes[j].nEndTime;
        }
    }
}

void ToCapturePlan(const NET_TV_CAPTURE_PLAN_INFO_S &src, Capture_NS::CapturePlan_S &dst)
{
    dst.init_weekSchedule();
    for (size_t i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
    {
        const NET_TV_CAPTURE_DAY_SCHEDULE_S &inDay = src.astDaySchedules[i];
        int nDayOfWeek = inDay.nDayOfWeek;
        if (nDayOfWeek < 1 || nDayOfWeek > 7)
            nDayOfWeek = (int)i + 1;
        Capture_NS::DaySchedule_S &outDay = dst.vstDaySchedules[(size_t)nDayOfWeek - 1];
        outDay.enDayOfWeek = (Capture_NS::DayOfWeek_E)nDayOfWeek;

        outDay.captureTimes.clear();
        size_t n = (size_t)inDay.udwTimeCount;
        if (n > NET_TV_PLAN_TIME_SECTION_NUM_ADAY)
            n = NET_TV_PLAN_TIME_SECTION_NUM_ADAY;

        if (n == 0)
        {
            Capture_NS::CaptureTime_S t;
            t.nStartTime = 0;
            t.nEndTime = 24 * 60 * 60;
            outDay.captureTimes.push_back(t);
            continue;
        }

        outDay.captureTimes.resize(n);
        for (size_t j = 0; j < n; ++j)
        {
            outDay.captureTimes[j].nStartTime = inDay.astTimes[j].nStartTime;
            outDay.captureTimes[j].nEndTime = inDay.astTimes[j].nEndTime;
        }
    }
}

void FillCaptureParam(const Capture_NS::CaptureParam_S &src, NET_TV_CAPTURE_PARAM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    FillOneCaptureConfig(src.stCaptureTimingConfig, dst.stCaptureTimingConfig);
    FillOneCaptureConfig(src.stCaptureEventConfig, dst.stCaptureEventConfig);
}

void ToCaptureParam(const NET_TV_CAPTURE_PARAM_INFO_S &src, Capture_NS::CaptureParam_S &dst)
{
    ToOneCaptureConfig(src.stCaptureTimingConfig, dst.stCaptureTimingConfig);
    ToOneCaptureConfig(src.stCaptureEventConfig, dst.stCaptureEventConfig);
}
} // namespace TvSdkConvert

void TvSdkConvert::FillExposureInfo(const ISP::ExposureAttr_S &src, NET_TV_EXPOSURE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enExpTime = (INT32)src.enExpTime;
    dst.bAntiBanding = src.bAntiBanding ? TRUE : FALSE;
}

void TvSdkConvert::ToExposureAttr(const NET_TV_EXPOSURE_INFO_S &src, ISP::ExposureAttr_S &dst)
{
    dst.enExpTime = (ISP::ExpTimeMode_E)src.enExpTime;
    dst.bAntiBanding = (src.bAntiBanding == TRUE);
}

void TvSdkConvert::FillDayNightInfo(const ISP::DayNightAttr_S &src, NET_TV_DAYNIGHT_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enDayNightMode = (INT32)src.enDayNightMode;
    dst.nBeginHour = (INT32)src.stBeginTime.nHour;
    dst.nBeginMinute = (INT32)src.stBeginTime.nMinute;
    dst.nBeginSecond = (INT32)src.stBeginTime.nSecond;
    dst.nBeginMilliSec = (INT32)src.stBeginTime.nMilliSec;
    dst.nEndHour = (INT32)src.stEndTime.nHour;
    dst.nEndMinute = (INT32)src.stEndTime.nMinute;
    dst.nEndSecond = (INT32)src.stEndTime.nSecond;
    dst.nEndMilliSec = (INT32)src.stEndTime.nMilliSec;
    dst.nSensitivityLevel = src.nSensitivityLevel;
    dst.nFilterTime = src.nFilterTime;
    dst.bFillLightExp = src.bFillLightExp ? TRUE : FALSE;
    dst.enLightMode = (INT32)src.enLightMode;
    dst.enLightType = (INT32)src.stFillLight.enLightType;
    dst.bWhiteLightEnable = src.stFillLight.stWhiteAttr.bEnable ? TRUE : FALSE;
    dst.nWhiteLightLevel = src.stFillLight.stWhiteAttr.nLightLevel;
    dst.bRedLightEnable = src.stFillLight.stRedAttr.bEnable ? TRUE : FALSE;
    dst.nRedLightLevel = src.stFillLight.stRedAttr.nLightLevel;
}

void TvSdkConvert::ToDayNightAttr(const NET_TV_DAYNIGHT_INFO_S &src, ISP::DayNightAttr_S &dst)
{
    dst.enDayNightMode = (ISP::DayNightMode_E)src.enDayNightMode;
    dst.stBeginTime.nHour = (unsigned int)src.nBeginHour;
    dst.stBeginTime.nMinute = (unsigned int)src.nBeginMinute;
    dst.stBeginTime.nSecond = (unsigned int)src.nBeginSecond;
    dst.stBeginTime.nMilliSec = (unsigned int)src.nBeginMilliSec;
    dst.stEndTime.nHour = (unsigned int)src.nEndHour;
    dst.stEndTime.nMinute = (unsigned int)src.nEndMinute;
    dst.stEndTime.nSecond = (unsigned int)src.nEndSecond;
    dst.stEndTime.nMilliSec = (unsigned int)src.nEndMilliSec;
    dst.nSensitivityLevel = src.nSensitivityLevel;
    dst.nFilterTime = src.nFilterTime;
    dst.bFillLightExp = (src.bFillLightExp == TRUE);
    dst.enLightMode = (ISP::LightBrightMode_E)src.enLightMode;
    dst.stFillLight.enLightType = (ISP::LightType_E)src.enLightType;
    dst.stFillLight.stWhiteAttr.bEnable = (src.bWhiteLightEnable == TRUE);
    dst.stFillLight.stWhiteAttr.nLightLevel = src.nWhiteLightLevel;
    dst.stFillLight.stRedAttr.bEnable = (src.bRedLightEnable == TRUE);
    dst.stFillLight.stRedAttr.nLightLevel = src.nRedLightLevel;
}

void TvSdkConvert::FillBackLightInfo(const ISP::BackLightArrt_S &src, NET_TV_BACKLIGHT_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enBackLightArea = (INT32)src.enBackLightArea;
    dst.bWdrEnable = src.stWdrAttr.bEnable ? TRUE : FALSE;
    dst.nWdrLevel = src.stWdrAttr.nWdrLevel;
    dst.bHlsEnable = src.stHlsAttr.bEnable ? TRUE : FALSE;
    dst.nHlsLevel = src.stHlsAttr.nHlsLevel;
}

void TvSdkConvert::ToBackLightAttr(const NET_TV_BACKLIGHT_INFO_S &src, ISP::BackLightArrt_S &dst)
{
    dst.enBackLightArea = (ISP::BackLightArea_E)src.enBackLightArea;
    dst.stWdrAttr.bEnable = (src.bWdrEnable == TRUE);
    dst.stWdrAttr.nWdrLevel = src.nWdrLevel;
    dst.stHlsAttr.bEnable = (src.bHlsEnable == TRUE);
    dst.stHlsAttr.nHlsLevel = src.nHlsLevel;
}

void TvSdkConvert::FillDenoiseInfo(const ISP::DnrAttr_S &src, NET_TV_DENOISE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enDnrMode = (INT32)src.enDnrMode;
    dst.nDnrLevel = src.nDnrLevel;
    dst.nSnrLevel = src.nSnrLevel;
    dst.nTnrLevel = src.nTnrLevel;
}

void TvSdkConvert::ToDnrAttr(const NET_TV_DENOISE_INFO_S &src, ISP::DnrAttr_S &dst)
{
    dst.enDnrMode = (ISP::DnrMode_E)src.enDnrMode;
    dst.nDnrLevel = src.nDnrLevel;
    dst.nSnrLevel = src.nSnrLevel;
    dst.nTnrLevel = src.nTnrLevel;
}

void TvSdkConvert::FillWhiteBalanceInfo(const ISP::AwbAttr_S &src, NET_TV_WHITEBALANCE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enAwbMode = (INT32)src.enAwbMode;
    dst.nRGain = src.nRGain;
    dst.nBGain = src.nBGain;
}

void TvSdkConvert::ToAwbAttr(const NET_TV_WHITEBALANCE_INFO_S &src, ISP::AwbAttr_S &dst)
{
    dst.enAwbMode = (ISP::AwbMode_E)src.enAwbMode;
    dst.nRGain = src.nRGain;
    dst.nBGain = src.nBGain;
}


void TvSdkConvert::FillTalkbackStateInfo(const Preview::IntercomInfo_S &src, NET_TV_TALKBACK_STATE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    std::strncpy(dst.szSdp, src.strSdp.c_str(), sizeof(dst.szSdp) - 1);
    std::strncpy(dst.szUrl, src.strUrl.c_str(), sizeof(dst.szUrl) - 1);
    std::strncpy(dst.szLocalIP, src.strLocalIp.c_str(), sizeof(dst.szLocalIP) - 1);
}

void TvSdkConvert::ToIntercomInfo(const NET_TV_TALKBACK_STATE_INFO_S &src, Preview::IntercomInfo_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.strSdp = src.szSdp;
    dst.strUrl = src.szUrl;
    dst.strLocalIp = src.szLocalIP;
}

void TvSdkConvert::FillTalkbackStreamInfo(const Replay::Stream::Info_S &src, NET_TV_TALKBACK_STREAM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szHost, src.host.c_str(), sizeof(dst.szHost) - 1);
    dst.nPort = src.nPort;
    dst.nChnId = src.nChnId;
    dst.nUserID = src.nUserId;
    dst.bMainStream = src.bMainStream ? TRUE : FALSE;
    std::strncpy(dst.szProtocol, src.protocol.c_str(), sizeof(dst.szProtocol) - 1);
    std::strncpy(dst.szStartTime, src.startTime.c_str(), sizeof(dst.szStartTime) - 1);
    std::strncpy(dst.szEndTime, src.endTime.c_str(), sizeof(dst.szEndTime) - 1);
    std::strncpy(dst.szFileName, src.filename.c_str(), sizeof(dst.szFileName) - 1);
}

void TvSdkConvert::ToReplayStreamInfo(const NET_TV_TALKBACK_STREAM_INFO_S &src, Replay::Stream::Info_S &dst)
{
    dst.host = src.szHost;
    dst.nPort = src.nPort;
    dst.nChnId = src.nChnId;
    dst.nUserId = src.nUserID;
    dst.bMainStream = (src.bMainStream == TRUE);
    dst.protocol = src.szProtocol;
    dst.startTime = src.szStartTime;
    dst.endTime = src.szEndTime;
    dst.filename = src.szFileName;
}

void TvSdkConvert::FillReplayTalkbackInfo(const Replay::Stream::ReplayRtpInfo_S &src, NET_TV_REPLAY_TALKBACK_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szNvrIp, src.nvrIp.c_str(), sizeof(dst.szNvrIp) - 1);
    std::strncpy(dst.szRemoteIp, src.remoteIp.c_str(), sizeof(dst.szRemoteIp) - 1);
    FillTalkbackStreamInfo(src.ipcInfo, dst.stIPCInfo);
}

void TvSdkConvert::ToReplayRtpInfo(const NET_TV_REPLAY_TALKBACK_INFO_S &src, Replay::Stream::ReplayRtpInfo_S &dst)
{
    dst.nvrIp = src.szNvrIp;
    dst.remoteIp = src.szRemoteIp;
    TvSdkConvert::ToReplayStreamInfo(src.stIPCInfo, dst.ipcInfo);
}

static void FillPolygonPoints(const Alarm::Region_S &src, INT32 &pointCount, FLOAT pointX[32], FLOAT pointY[32])
{
    pointCount = (INT32)std::min<size_t>(src.aPoint.size(), 32);
    for (int p = 0; p < pointCount; ++p)
    {
        pointX[p] = src.aPoint[p].fX;
        pointY[p] = src.aPoint[p].fY;
    }
}

// --------- ParkingDetect (IPC ParkingDetection_S <-> SDK NET_TV_PARKING_ALARM_INFO_S) ---------

void TvSdkConvert::FillParkingDetectAlarmInfo(const Alarm::ParkingDetection_S &src, NET_TV_PARKING_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        TvSdkConvert::FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        dst.dwRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToParkingDetection(const NET_TV_PARKING_ALARM_INFO_S &src, Alarm::ParkingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::ParkingRule_S out;
        TvSdkConvert::ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- UnattendedObject (IPC UnattendedObject_S <-> SDK NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S) ---------
void TvSdkConvert::FillUnattendedObjectAlarmInfo(const Alarm::UnattendedObject_S &src, NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        TvSdkConvert::FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        dst.dwRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
               TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToUnattendedObject(const NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S &src, Alarm::UnattendedObject_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::UnattendedObjectRule_S out;
        TvSdkConvert::ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- ObjectRemoval (IPC ObjectRemoval_S <-> SDK NET_TV_OBJECT_REMOVAL_ALARM_INFO_S) ---------
void TvSdkConvert::FillObjectRemovalAlarmInfo(const Alarm::ObjectRemoval_S &src, NET_TV_OBJECT_REMOVAL_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        TvSdkConvert::FillPolygonPoints(r.stRegion, out.dwPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        dst.dwRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToObjectRemoval(const NET_TV_OBJECT_REMOVAL_ALARM_INFO_S &src, Alarm::ObjectRemoval_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::ObjectRemovalRule_S out;
        TvSdkConvert::ToRegionFromPolygon(r.dwPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

void TvSdkConvert::FillAudioCfg(const Audio_NS::AudioConfig_S &src, NET_TV_AUDIO_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bAudioSwitch = src.bAudioSwitch ? TRUE : FALSE;
    dst.enInputType = (INT32)src.enInputType;
    dst.enFormat = (INT32)src.enFormat;
    dst.enSampRate = (INT32)src.enSampRate;
    dst.enBitRate = (INT32)src.enBitRate;
    dst.u32InputVolume = src.u32InputVolume;
    dst.bDenoise = src.bDenoise ? TRUE : FALSE;
    dst.enOutputType = (INT32)src.enOutputType;
    dst.u32OutputVolume = src.u32OutputVolume;
}

void TvSdkConvert::ToAudioConfig(const NET_TV_AUDIO_CFG_S &src, Audio_NS::AudioConfig_S &dst)
{
    dst.bAudioSwitch = (src.bAudioSwitch == TRUE);
    dst.enInputType = (Audio_NS::AudioInputType_E)src.enInputType;
    dst.enFormat = (Audio_NS::AudioFormat_E)src.enFormat;
    dst.enSampRate = (Audio_NS::AudioSamprate_E)src.enSampRate;
    dst.enBitRate = (Audio_NS::AudioBitrate_E)src.enBitRate;
    dst.u32InputVolume = src.u32InputVolume;
    dst.bDenoise = (src.bDenoise == TRUE);
    dst.enOutputType = (Audio_NS::AudioOutputType_E)src.enOutputType;
    dst.u32OutputVolume = src.u32OutputVolume;
}


// --------- EnterRegion (IPC EntranceDetection_S <-> SDK NET_TV_ENTER_REGION_ALARM_INFO_S) ---------
void TvSdkConvert::FillEnterRegionAlarmInfo(const Alarm::EntranceDetection_S &src, NET_TV_ENTER_REGION_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.dwPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.dwPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity = (INT32)r.nSensitivity;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
            out.adwDetectionTarget[j] = r.aDetectionTarget[j];
        dst.dwRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToEntranceDetection(const NET_TV_ENTER_REGION_ALARM_INFO_S &src, Alarm::EntranceDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::EnterExitIntrusion_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.dwPointCount;
        for (int p = 0; p < r.dwPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({r.afPointX[p], r.afPointY[p]});
        }
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- LeaveRegion (IPC ExitingDetection_S <-> SDK NET_TV_LEAVE_REGION_ALARM_INFO_S) ---------
void TvSdkConvert::FillLeaveRegionAlarmInfo(const Alarm::ExitingDetection_S &src, NET_TV_LEAVE_REGION_ALARM_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.dwRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.dwPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.dwPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity = (INT32)r.nSensitivity;
        out.dwDetectionTargetCount = (INT32)std::min<size_t>(r.aDetectionTarget.size(), 8);
        for (int j = 0; j < out.dwDetectionTargetCount; ++j)
            out.adwDetectionTarget[j] = r.aDetectionTarget[j];
        dst.dwRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToExitingDetection(const NET_TV_LEAVE_REGION_ALARM_INFO_S &src, Alarm::ExitingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.dwRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::EnterExitIntrusion_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.dwPointCount;
        for (int p = 0; p < r.dwPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({r.afPointX[p], r.afPointY[p]});
        }
        out.aDetectionTarget.assign(r.adwDetectionTarget, r.adwDetectionTarget + std::min(r.dwDetectionTargetCount, 8));
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- FaceCapture (IPC FaceCapture_S <-> SDK NET_TV_FACE_CAPTURE_INFO_S) ---------
void TvSdkConvert::FillFaceCaptureInfo(const Alarm::FaceCapture_S &src, NET_TV_FACE_CAPTURE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;

    dst.stRule.stRegion.dwPointCount = (INT32)std::min<size_t>(src.stRule.stRegion.aPoint.size(), 32);
    for (int p = 0; p < dst.stRule.stRegion.dwPointCount; ++p)
    {
        dst.stRule.stRegion.afPointX[p] = src.stRule.stRegion.aPoint[p].fX;
        dst.stRule.stRegion.afPointY[p] = src.stRule.stRegion.aPoint[p].fY;
    }

    dst.stRule.dwShieldRegionCount = (INT32)std::min<size_t>(src.stRule.vstShieldedRegion.size(), 4);
    for (int i = 0; i < dst.stRule.dwShieldRegionCount; ++i)
    {
        const auto &reg = src.stRule.vstShieldedRegion[i];
        auto &out = dst.stRule.astShieldRegion[i];
        out.dwPointCount = (INT32)std::min<size_t>(reg.aPoint.size(), 32);
        for (int p = 0; p < out.dwPointCount; ++p)
        {
            out.afPointX[p] = reg.aPoint[p].fX;
            out.afPointY[p] = reg.aPoint[p].fY;
        }
    }

    dst.stRule.nMinIpdRectLeft = src.stRule.stMinIpdRect.nX;
    dst.stRule.nMinIpdRectTop = src.stRule.stMinIpdRect.nY;
    dst.stRule.nMinIpdRectRight = src.stRule.stMinIpdRect.nX + src.stRule.stMinIpdRect.nWidth;
    dst.stRule.nMinIpdRectBottom = src.stRule.stMinIpdRect.nY + src.stRule.stMinIpdRect.nHeight;
    dst.stRule.nMinWidth = src.stRule.nMinWidth;
    dst.stRule.nMinHeight = src.stRule.nMinHeight;
    dst.stRule.nMaxWidth = src.stRule.nMaxWidth;
    dst.stRule.nMaxHeight = src.stRule.nMaxHeight;
    dst.stRule.nInterval = src.stRule.nInterval;

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_TV_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.dwTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToFaceCapture(const NET_TV_FACE_CAPTURE_INFO_S &src, Alarm::FaceCapture_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);

    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;

    dst.stRule.stRegion.aPoint.clear();
    int pointCnt = std::max(0, std::min(src.stRule.stRegion.dwPointCount, 32));
    dst.stRule.stRegion.nPointNum = (unsigned int)pointCnt;
    for (int p = 0; p < pointCnt; ++p)
    {
        Common::PosF_S pt;
        pt.fX = src.stRule.stRegion.afPointX[p];
        pt.fY = src.stRule.stRegion.afPointY[p];
        dst.stRule.stRegion.aPoint.push_back(pt);
    }

    dst.stRule.vstShieldedRegion.clear();
    int shieldCnt = std::max(0, std::min(src.stRule.dwShieldRegionCount, 4));
    for (int i = 0; i < shieldCnt; ++i)
    {
        const auto &inReg = src.stRule.astShieldRegion[i];
        Alarm::Region_S outReg;
        outReg.aPoint.clear();
        int shieldPointCnt = std::max(0, std::min(inReg.dwPointCount, 32));
        outReg.nPointNum = (unsigned int)shieldPointCnt;
        for (int p = 0; p < shieldPointCnt; ++p)
        {
            Common::PosF_S pt;
            pt.fX = inReg.afPointX[p];
            pt.fY = inReg.afPointY[p];
            outReg.aPoint.push_back(pt);
        }
        dst.stRule.vstShieldedRegion.push_back(outReg);
    }

    dst.stRule.stMinIpdRect.nX = src.stRule.nMinIpdRectLeft;
    dst.stRule.stMinIpdRect.nY = src.stRule.nMinIpdRectTop;
    dst.stRule.stMinIpdRect.nWidth = src.stRule.nMinIpdRectRight - src.stRule.nMinIpdRectLeft;
    dst.stRule.stMinIpdRect.nHeight = src.stRule.nMinIpdRectBottom - src.stRule.nMinIpdRectTop;
    dst.stRule.nMinWidth = src.stRule.nMinWidth;
    dst.stRule.nMinHeight = src.stRule.nMinHeight;
    dst.stRule.nMaxWidth = src.stRule.nMaxWidth;
    dst.stRule.nMaxHeight = src.stRule.nMaxHeight;
    dst.stRule.nInterval = src.stRule.nInterval;

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.dwTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_TV_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// ---------  FaceLib / FaceInfo ---------
void TvSdkConvert::FillFaceLibInfo(const Event::FaceLibInfo_S &src, NET_TV_FACE_LIB_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    copy_string(dst.szFaceLibName, src.strFaceLibName);
    dst.nTotalFace = (INT32)src.nTotalFace;
    dst.nNormalNum = (INT32)src.nNormalNum;
    dst.nAbnormalNum = (INT32)src.nAbnormalNum;
}

void TvSdkConvert::ToFaceCompare(const NET_TV_FACE_COMPARE_INFO_S &src, Alarm::FaceCompare_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageListSuccess, dst.stLinkageListSuccess);
    ToLinkageList(src.stLinkageListFail, dst.stLinkageListFail);
}

void TvSdkConvert::ToFaceLibInfo(const NET_TV_FACE_LIB_INFO_S &src, Event::FaceLibInfo_S &dst)
{
    dst.strFaceLibName = src.szFaceLibName;
    dst.nTotalFace = (int)src.nTotalFace;
    dst.nNormalNum = (int)src.nNormalNum;
    dst.nAbnormalNum = (int)src.nAbnormalNum;
}

void TvSdkConvert::FillFaceLibList(const std::vector<Event::FaceLibInfo_S> &src, NET_TV_FACE_LIB_LIST_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    size_t nCount = std::min(src.size(), (size_t)NET_TV_FACE_LIB_MAX_NUM);
    dst.nTargetLibCount = (INT32)nCount;
    for (size_t i = 0; i < nCount; ++i)
    {
        FillFaceLibInfo(src[i], dst.astTargetLibInfos[i]);
    }
}

void TvSdkConvert::ToFaceIdInfo(const NET_TV_FACE_ID_INFO_S &src, Event::FaceIdInfo_S &dst)
{
    dst.ids.clear();
    int nCount = std::max(0, std::min(src.nIdCount, (INT32)NET_TV_FACE_ID_MAX_NUM));
    for (int i = 0; i < nCount; ++i)
    {
        dst.ids.push_back((int)src.anIds[i]);
    }
}

void TvSdkConvert::FillFaceInfo(const Event::FaceInfo_S &src, NET_TV_FACE_INFO_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nId = (INT32)src.nId;
    copy_string(dst.szFaceLibName, src.strFaceLibName);
    copy_string(dst.szName, src.strName);
    copy_string(dst.szPhoneNum, src.strPhoneNum);
    copy_string(dst.szPicPath, src.strPicPath);
    copy_string(dst.szBinPath, src.BinPath);
    copy_string(dst.szPicType, src.strPicType);
    dst.nPicSize = (INT32)src.nPicSize;
    copy_string(dst.szPicDate, src.strPicDate);
    dst.nModelState = (INT32)src.nModelState;
    dst.nRatingLevel = (INT32)src.nRatingLevel;
}

void TvSdkConvert::ToFaceInfo(const NET_TV_FACE_INFO_S &src, Event::FaceInfo_S &dst)
{
    dst.nId = (int)src.nId;
    dst.strFaceLibName = src.szFaceLibName;
    dst.strName = src.szName;
    dst.strPhoneNum = src.szPhoneNum;
    dst.strPicPath = src.szPicPath;
    dst.BinPath = src.szBinPath;
    dst.strPicType = src.szPicType;
    dst.nPicSize = (int)src.nPicSize;
    dst.strPicDate = src.szPicDate;
    dst.nModelState = (int)src.nModelState;
    dst.nRatingLevel = (int)src.nRatingLevel;
}

void TvSdkConvert::FillFaceInfoList(const std::vector<Event::FaceInfo_S> &src, NET_TV_FACE_INFO_LIST_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    size_t nCount = std::min(src.size(), (size_t)NET_TV_FACE_INFO_MAX_NUM);
    dst.nFaceInfoCount = (INT32)nCount;
    for (size_t i = 0; i < nCount; ++i)
    {
        FillFaceInfo(src[i], dst.astFaceInfos[i]);
    }
}

/* 将内部 "0xRRGGBB" 格式转换为 SDK "#RRGGBB" 格式 */
static void convert_font_color(char *dst, size_t dstSize, const std::string &src)
{
    std::memset(dst, 0, dstSize);
    if (src.empty())
        return;
    if (src.size() >= 2 && src[0] == '0' && (src[1] == 'x' || src[1] == 'X'))
        std::snprintf(dst, dstSize, "#%s", src.c_str() + 2);
    else
        std::strncpy(dst, src.c_str(), dstSize - 1);
}

static void fill_osd_attr(const Osd::OsdAttribute_S &src, OsdAttribute_S &dst)
{
    dst.nX          = (INT32)src.nX;
    dst.nY          = (INT32)src.nY;
    dst.nW          = (INT32)src.nW;
    dst.nH          = (INT32)src.nH;
    dst.enAttribute = (OSD_ATTRIBUTE_E)src.enAttribute;
    dst.enFontSize  = (OSD_FONT_SIZE_E)src.enFontSize;
    dst.enFontColor = (OSD_COLOR_E)src.enFontColor;
    convert_font_color(dst.strFontColor, sizeof(dst.strFontColor), src.strFontColor);
    TvSdkConvert::copy_string(dst.strToken, src.strToken);
}

void TvSdkConvert::FillOsdConfig(const Osd::OsdConfig_S &src, NET_TV_VIDEO_OSD_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));

    dst.enAlign = (OSD_ALIGN_E)src.enAlign;

    /* 名称信息 */
    dst.stOsdNameInfo.bEnable = src.stOsdNameInfo.bEnable ? TRUE : FALSE;
    copy_string(dst.stOsdNameInfo.strName, src.stOsdNameInfo.strName);
    fill_osd_attr(src.stOsdNameInfo.stOsdAttr, dst.stOsdNameInfo.stOsdAttr);

    /* 时间信息 */
    dst.stOsdTimeInfo.bEnable     = src.stOsdTimeInfo.bEnable ? TRUE : FALSE;
    dst.stOsdTimeInfo.bEnableWeek = src.stOsdTimeInfo.bEnableWeek ? TRUE : FALSE;
    dst.stOsdTimeInfo.enTimeFormat = (OSD_TIME_FORMAT_E)src.stOsdTimeInfo.enTimeFormat;
    dst.stOsdTimeInfo.enDateFormat = (OSD_DATE_FORMAT_E)src.stOsdTimeInfo.enDateFormat;
    fill_osd_attr(src.stOsdTimeInfo.stOsdAttr, dst.stOsdTimeInfo.stOsdAttr);

    /* 字符叠加信息 */
    size_t nCount = std::min(src.vecOsdInfo.size(), (size_t)32);
    for (size_t i = 0; i < nCount; ++i)
    {
        dst.OsdInfo[i].nId    = (INT32)src.vecOsdInfo[i].nId;
        dst.OsdInfo[i].bEnable = src.vecOsdInfo[i].bEnable ? TRUE : FALSE;
        copy_string(dst.OsdInfo[i].strName, src.vecOsdInfo[i].strName);
        fill_osd_attr(src.vecOsdInfo[i].stOsdAttr, dst.OsdInfo[i].stOsdAttr);
    }
}

/* 将 SDK "#RRGGBB" 格式转换为内部 "0xRRGGBB" 格式 */
static void revert_font_color(std::string &dst, const char *src)
{
    if (!src || src[0] == '\0')
    {
        dst.clear();
        return;
    }
    if (src[0] == '#')
        dst = std::string("0x") + (src + 1);
    else
        dst = src;
}

static void to_osd_attr(const OsdAttribute_S &src, Osd::OsdAttribute_S &dst)
{
    dst.nX          = (int)src.nX;
    dst.nY          = (int)src.nY;
    dst.nW          = (int)src.nW;
    dst.nH          = (int)src.nH;
    dst.enAttribute = (Osd::OSD_ATTRIBUTE_E)src.enAttribute;
    dst.enFontSize  = (Osd::OSD_FONT_SIZE_E)src.enFontSize;
    dst.enFontColor = (Osd::OSD_COLOR_E)src.enFontColor;
    revert_font_color(dst.strFontColor, src.strFontColor);
    dst.strToken    = src.strToken;
}

void TvSdkConvert::ToOsdConfig(const NET_TV_VIDEO_OSD_CFG_S &src, Osd::OsdConfig_S &dst)
{
    dst.enAlign = (Osd::OSD_ALIGN_E)src.enAlign;

    /* 名称信息 */
    dst.stOsdNameInfo.bEnable  = (src.stOsdNameInfo.bEnable == TRUE);
    dst.stOsdNameInfo.strName  = src.stOsdNameInfo.strName;
    to_osd_attr(src.stOsdNameInfo.stOsdAttr, dst.stOsdNameInfo.stOsdAttr);

    /* 时间信息 */
    dst.stOsdTimeInfo.bEnable      = (src.stOsdTimeInfo.bEnable == TRUE);
    dst.stOsdTimeInfo.bEnableWeek  = (src.stOsdTimeInfo.bEnableWeek == TRUE);
    dst.stOsdTimeInfo.enTimeFormat = (Osd::OSD_TIME_FORMAT_E)src.stOsdTimeInfo.enTimeFormat;
    dst.stOsdTimeInfo.enDateFormat = (Osd::OSD_DATE_FORMAT_E)src.stOsdTimeInfo.enDateFormat;
    to_osd_attr(src.stOsdTimeInfo.stOsdAttr, dst.stOsdTimeInfo.stOsdAttr);

    /* 字符叠加信息 */
    dst.vecOsdInfo.resize(32);
    for (size_t i = 0; i < 32; ++i)
    {
        dst.vecOsdInfo[i].nId     = (int)src.OsdInfo[i].nId;
        dst.vecOsdInfo[i].bEnable = (src.OsdInfo[i].bEnable == TRUE);
        dst.vecOsdInfo[i].strName = src.OsdInfo[i].strName;
        to_osd_attr(src.OsdInfo[i].stOsdAttr, dst.vecOsdInfo[i].stOsdAttr);
    }
}

void TvSdkConvert::FillPrivacyMaskCfg(const Osd::CoverConfig_S &src, NET_TV_PRIVACY_MASK_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    size_t nCount = std::min(src.vecCoverAttr.size(), (size_t)NET_TV_MAX_PRIVACY_MASK_AREA_NUM);
    dst.dwAreaCount = (INT32)nCount;
    for (size_t i = 0; i < nCount; ++i)
    {
        dst.astArea[i].nAreaID    = src.vecCoverAttr[i].nId - 1; /* 内部1-based → SDK 0-based */
        dst.astArea[i].bEnable    = src.vecCoverAttr[i].bEnable ? TRUE : FALSE;
        dst.astArea[i].nRectLeft  = src.vecCoverAttr[i].nX;
        dst.astArea[i].nRectTop   = src.vecCoverAttr[i].nY;
        dst.astArea[i].nRectRight = src.vecCoverAttr[i].nX + src.vecCoverAttr[i].nWidth;
        dst.astArea[i].nRectBottom = src.vecCoverAttr[i].nY + src.vecCoverAttr[i].nHeight;
    }
}

void TvSdkConvert::ToPrivacyMaskCfg(const NET_TV_PRIVACY_MASK_CFG_S &src, Osd::CoverConfig_S &dst)
{
    dst.clear();
    dst.bEnable = (src.bEnable == TRUE);
    INT32 nCount = std::max<INT32>(0, std::min(src.dwAreaCount, (INT32)NET_TV_MAX_PRIVACY_MASK_AREA_NUM));
    size_t nUpdateCount = std::min((size_t)nCount, dst.vecCoverAttr.size());
    for (size_t i = 0; i < nUpdateCount; ++i)
    {
        dst.vecCoverAttr[i].nId      = src.astArea[i].nAreaID + 1; /* SDK 0-based → 内部1-based */
        dst.vecCoverAttr[i].bEnable  = (src.astArea[i].bEnable == TRUE);
        dst.vecCoverAttr[i].nX       = src.astArea[i].nRectLeft;
        dst.vecCoverAttr[i].nY       = src.astArea[i].nRectTop;
        dst.vecCoverAttr[i].nWidth   = src.astArea[i].nRectRight - src.astArea[i].nRectLeft;
        dst.vecCoverAttr[i].nHeight  = src.astArea[i].nRectBottom - src.astArea[i].nRectTop;
    }
}
