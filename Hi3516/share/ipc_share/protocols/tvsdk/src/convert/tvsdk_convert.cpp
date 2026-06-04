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
namespace TvSdkConvert
{
static constexpr size_t kOsdCustomSlotCount = 4;

static bool HasCustomOsdPayload(const Osd::OsdInfo_S &info)
{
    return info.bEnable ||
           !info.strName.empty() ||
           info.stOsdAttr.nX != 0 ||
           info.stOsdAttr.nY != 0 ||
           info.stOsdAttr.nW != 0 ||
           info.stOsdAttr.nH != 0 ||
           info.stOsdAttr.enAttribute != Osd::OSD_ATTR_ALPHA_N_FLASH_N ||
           info.stOsdAttr.enFontSize != Osd::OSD_FONT_SIZE_ADAPTIVE ||
           info.stOsdAttr.enFontColor != Osd::OSD_COLOR_BLACK;
}

static size_t GetOsdCopyStart(const std::vector<Osd::OsdInfo_S> &infos)
{
    if (infos.size() <= kOsdCustomSlotCount)
    {
        return 0;
    }

    bool bFirstBlockHasPayload = false;
    for (size_t i = 0; i < kOsdCustomSlotCount && i < infos.size(); ++i)
    {
        bFirstBlockHasPayload = bFirstBlockHasPayload || HasCustomOsdPayload(infos[i]);
    }

    bool bSecondBlockHasPayload = false;
    const size_t nSecondEnd = std::min(infos.size(), kOsdCustomSlotCount * 2);
    for (size_t i = kOsdCustomSlotCount; i < nSecondEnd; ++i)
    {
        bSecondBlockHasPayload = bSecondBlockHasPayload || HasCustomOsdPayload(infos[i]);
    }

    return (!bFirstBlockHasPayload && bSecondBlockHasPayload) ? kOsdCustomSlotCount : 0;
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

static INT32 ToSdkVideoCodec(Video_NS::VideoCodec_E src)
{
    switch (src)
    {
        case Video_NS::VideoCodec_E::H264:
            return NET_TV_VIDEO_CODE_H264;
        case Video_NS::VideoCodec_E::H265:
            return NET_TV_VIDEO_CODE_H265;
        case Video_NS::VideoCodec_E::JPEG:
            return NET_TV_VIDEO_CODE_JPEG;
        case Video_NS::VideoCodec_E::MJPEG:
            return NET_TV_VIDEO_CODE_MJPEG;
        case Video_NS::VideoCodec_E::SVAC3:
            return NET_TV_VIDEO_CODE_SVAC3;
        case Video_NS::VideoCodec_E::MPEG4:
            return NET_TV_VIDEO_CODE_MPEG4;
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
        case NET_TV_VIDEO_CODE_JPEG:
            return Video_NS::VideoCodec_E::JPEG;
        case NET_TV_VIDEO_CODE_MJPEG:
            return Video_NS::VideoCodec_E::MJPEG;
        case NET_TV_VIDEO_CODE_SVAC3:
            return Video_NS::VideoCodec_E::SVAC3;
        case NET_TV_VIDEO_CODE_MPEG4:
            return Video_NS::VideoCodec_E::MPEG4;
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

static void FillOsdAttribute(const Osd::OsdAttribute_S &src, OsdAttribute_S &dst)
{
    dst.nX = (INT32)src.nX;
    dst.nY = (INT32)src.nY;
    dst.nW = (INT32)src.nW;
    dst.nH = (INT32)src.nH;
    dst.enAttribute = (OSD_ATTRIBUTE_E)src.enAttribute;
    dst.enFontSize = (OSD_FONT_SIZE_E)src.enFontSize;
    dst.enFontColor = (OSD_COLOR_E)src.enFontColor;
    std::strncpy(dst.strFontColor, src.strFontColor.c_str(), sizeof(dst.strFontColor) - 1);
    std::strncpy(dst.strToken, src.strToken.c_str(), sizeof(dst.strToken) - 1);
}

static void ToOsdAttribute(const OsdAttribute_S &src, Osd::OsdAttribute_S &dst)
{
    dst.nX = (int)src.nX;
    dst.nY = (int)src.nY;
    dst.nW = (int)src.nW;
    dst.nH = (int)src.nH;
    dst.enAttribute = (Osd::OSD_ATTRIBUTE_E)src.enAttribute;
    dst.enFontSize = (Osd::OSD_FONT_SIZE_E)src.enFontSize;
    dst.enFontColor = (Osd::OSD_COLOR_E)src.enFontColor;
    dst.strFontColor = src.strFontColor;
    dst.strToken = src.strToken;
}

void FillOsdConfig(const Osd::OsdConfig_S &src, NET_TV_VIDEO_OSD_CFG_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enAlign = (OSD_ALIGN_E)src.enAlign;

    dst.stOsdNameInfo.bEnable = src.stOsdNameInfo.bEnable ? TRUE : FALSE;
    std::strncpy(dst.stOsdNameInfo.strName, src.stOsdNameInfo.strName.c_str(), sizeof(dst.stOsdNameInfo.strName) - 1);
    FillOsdAttribute(src.stOsdNameInfo.stOsdAttr, dst.stOsdNameInfo.stOsdAttr);

    dst.stOsdTimeInfo.bEnable = src.stOsdTimeInfo.bEnable ? TRUE : FALSE;
    dst.stOsdTimeInfo.bEnableWeek = src.stOsdTimeInfo.bEnableWeek ? TRUE : FALSE;
    dst.stOsdTimeInfo.enTimeFormat = (OSD_TIME_FORMAT_E)src.stOsdTimeInfo.enTimeFormat;
    dst.stOsdTimeInfo.enDateFormat = (OSD_DATE_FORMAT_E)src.stOsdTimeInfo.enDateFormat;
    FillOsdAttribute(src.stOsdTimeInfo.stOsdAttr, dst.stOsdTimeInfo.stOsdAttr);

    const size_t nStart = GetOsdCopyStart(src.vecOsdInfo);
    const size_t nCount = std::min(src.vecOsdInfo.size() - nStart, kOsdCustomSlotCount);
    for (size_t i = 0; i < nCount; ++i)
    {
        const Osd::OsdInfo_S &srcInfo = src.vecOsdInfo[nStart + i];
        dst.OsdInfo[i].nId = (INT32)srcInfo.nId;
        dst.OsdInfo[i].bEnable = srcInfo.bEnable ? TRUE : FALSE;
        std::strncpy(dst.OsdInfo[i].strName, srcInfo.strName.c_str(), sizeof(dst.OsdInfo[i].strName) - 1);
        FillOsdAttribute(srcInfo.stOsdAttr, dst.OsdInfo[i].stOsdAttr);
    }
}

void ToOsdConfig(const NET_TV_VIDEO_OSD_CFG_S &src, Osd::OsdConfig_S &dst)
{
    dst.clear();
    dst.enAlign = (Osd::OSD_ALIGN_E)src.enAlign;

    dst.stOsdNameInfo.bEnable = (src.stOsdNameInfo.bEnable == TRUE);
    dst.stOsdNameInfo.strName = src.stOsdNameInfo.strName;
    ToOsdAttribute(src.stOsdNameInfo.stOsdAttr, dst.stOsdNameInfo.stOsdAttr);

    dst.stOsdTimeInfo.bEnable = (src.stOsdTimeInfo.bEnable == TRUE);
    dst.stOsdTimeInfo.bEnableWeek = (src.stOsdTimeInfo.bEnableWeek == TRUE);
    dst.stOsdTimeInfo.enTimeFormat = (Osd::OSD_TIME_FORMAT_E)src.stOsdTimeInfo.enTimeFormat;
    dst.stOsdTimeInfo.enDateFormat = (Osd::OSD_DATE_FORMAT_E)src.stOsdTimeInfo.enDateFormat;
    ToOsdAttribute(src.stOsdTimeInfo.stOsdAttr, dst.stOsdTimeInfo.stOsdAttr);

    const size_t nCount = std::min(dst.vecOsdInfo.size(), kOsdCustomSlotCount);
    for (size_t i = 0; i < nCount; ++i)
    {
        dst.vecOsdInfo[i].nId = (int)src.OsdInfo[i].nId;
        dst.vecOsdInfo[i].bEnable = (src.OsdInfo[i].bEnable == TRUE);
        dst.vecOsdInfo[i].strName = src.OsdInfo[i].strName;
        ToOsdAttribute(src.OsdInfo[i].stOsdAttr, dst.vecOsdInfo[i].stOsdAttr);
    }

    dst.init_token();
}



void FillImageSetting(const ISP::ImageParam_S &src, NET_TV_IMAGE_SETTING_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nBrightness = (INT32)src.nBrightness;
    dst.nContrast = (INT32)src.nContrast;
    dst.nSaturation = (INT32)src.nSaturation;
    dst.nSharpness = (INT32)src.nSharpness;
}

void ToImageParam(const NET_TV_IMAGE_SETTING_S &src, ISP::ImageParam_S &dst)
{
    dst.nBrightness = (unsigned int)src.nBrightness;
    dst.nContrast = (unsigned int)src.nContrast;
    dst.nSaturation = (unsigned int)src.nSaturation;
    dst.nSharpness = (unsigned int)src.nSharpness;
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

static void FillFrameRateList(FLOAT frameRateMin, FLOAT frameRateMax, NET_TV_VIDEO_RESOLUTION_S &dst)
{
    static const FLOAT kFrameRates[] = {
        1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 2.0f,
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 8.3f,
        9.0f, 10.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f,
        18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f,
        26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 35.0f, 40.0f, 45.0f,
        48.0f, 50.0f, 55.0f, 60.0f, 100.0f, 120.0f
    };

    if (frameRateMin > frameRateMax)
    {
        const FLOAT tmp = frameRateMin;
        frameRateMin = frameRateMax;
        frameRateMax = tmp;
    }

    dst.dwFrameRateNum = 0;
    for (size_t i = 0; i < sizeof(kFrameRates) / sizeof(kFrameRates[0]) &&
                       dst.dwFrameRateNum < NET_TV_VIDEO_FRAME_RATE_MAX_NUM; ++i)
    {
        if (kFrameRates[i] >= frameRateMin && kFrameRates[i] <= frameRateMax)
        {
            dst.adwFrameRate[dst.dwFrameRateNum++] = kFrameRates[i];
        }
    }
}

/**
 * @brief 将 IPC 侧 FrameRate_E 枚举值转换为 SDK 侧的浮点帧率值
 */
static FLOAT FrameRateEnumToFloat(Video_NS::FrameRate_E enFrameRate)
{
    switch (enFrameRate)
    {
        case Video_NS::FRAME_RATE_1_16: return 0.0625f;
        case Video_NS::FRAME_RATE_1_8:  return 0.125f;
        case Video_NS::FRAME_RATE_1_4:  return 0.25f;
        case Video_NS::FRAME_RATE_1_2:  return 0.5f;
        case Video_NS::FRAME_RATE_1:    return 1.0f;
        case Video_NS::FRAME_RATE_2:    return 2.0f;
        case Video_NS::FRAME_RATE_3:    return 3.0f;
        case Video_NS::FRAME_RATE_4:    return 4.0f;
        case Video_NS::FRAME_RATE_5:    return 5.0f;
        case Video_NS::FRAME_RATE_6:    return 6.0f;
        case Video_NS::FRAME_RATE_7:    return 7.0f;
        case Video_NS::FRAME_RATE_8:    return 8.0f;
        case Video_NS::FRAME_RATE_9:    return 9.0f;
        case Video_NS::FRAME_RATE_10:   return 10.0f;
        case Video_NS::FRAME_RATE_12:   return 12.0f;
        case Video_NS::FRAME_RATE_13:   return 13.0f;
        case Video_NS::FRAME_RATE_14:   return 14.0f;
        case Video_NS::FRAME_RATE_15:   return 15.0f;
        case Video_NS::FRAME_RATE_16:   return 16.0f;
        case Video_NS::FRAME_RATE_17:   return 17.0f;
        case Video_NS::FRAME_RATE_18:   return 18.0f;
        case Video_NS::FRAME_RATE_19:   return 19.0f;
        case Video_NS::FRAME_RATE_20:   return 20.0f;
        case Video_NS::FRAME_RATE_21:   return 21.0f;
        case Video_NS::FRAME_RATE_22:   return 22.0f;
        case Video_NS::FRAME_RATE_23:   return 23.0f;
        case Video_NS::FRAME_RATE_24:   return 24.0f;
        case Video_NS::FRAME_RATE_25:   return 25.0f;
        case Video_NS::FRAME_RATE_26:   return 26.0f;
        case Video_NS::FRAME_RATE_27:   return 27.0f;
        case Video_NS::FRAME_RATE_28:   return 28.0f;
        case Video_NS::FRAME_RATE_29:   return 29.0f;
        case Video_NS::FRAME_RATE_30:   return 30.0f;
        case Video_NS::FRAME_RATE_35:   return 35.0f;
        case Video_NS::FRAME_RATE_40:   return 40.0f;
        case Video_NS::FRAME_RATE_45:   return 45.0f;
        case Video_NS::FRAME_RATE_48:   return 48.0f;
        case Video_NS::FRAME_RATE_50:   return 50.0f;
        case Video_NS::FRAME_RATE_55:   return 55.0f;
        case Video_NS::FRAME_RATE_60:   return 60.0f;
        case Video_NS::FRAME_RATE_100:  return 100.0f;
        case Video_NS::FRAME_RATE_120:  return 120.0f;
        case Video_NS::FRAME_RATE_8_3:  return 8.3f;
        default:                        return 30.0f;
    }
}

/**
 * @brief 将 IPC 侧 Resolution_S 转换为 SDK 侧 NET_TV_VIDEO_RESOLUTION_S
 */
static void FillOneResolution(const Video_NS::Resolution_S &src, NET_TV_VIDEO_RESOLUTION_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szName, src.strName.c_str(), sizeof(dst.szName) - 1);
    ParseResolutionName(src.strName, dst);
    dst.dwFrameRateMin = FrameRateEnumToFloat(src.enFrameRateMin);
    dst.dwFrameRateMax = FrameRateEnumToFloat(src.enFrameRateMax);
    FillFrameRateList(dst.dwFrameRateMin, dst.dwFrameRateMax, dst);
    dst.dwBitRateMin = (INT32)src.nBitRateMin;
    dst.dwBitRateMax = (INT32)src.nBitRateMax;
}

static void FillOneEncodeAbility(const Video_NS::EncodeAbility_S &src, NET_TV_VIDEO_ENCODE_ABILITY_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szVideoCodec, src.strVideoCodec.c_str(), sizeof(dst.szVideoCodec) - 1);
    dst.enVideoCodec = ToSdkVideoCodec(Video_NS::string_toVideoCodec(src.strVideoCodec));
    dst.nSupportAdjustComplexity = (INT32)src.nSupportAdjustComplexity;
    dst.nEncodeComplexityNum = (INT32)std::min(src.vEncodeComplexity.size(), (size_t)NET_TV_VIDEO_ENCODE_COMPLEXITY_MAX_NUM);
    for (INT32 i = 0; i < dst.nEncodeComplexityNum; ++i)
    {
        dst.anEncodeComplexity[i] = (INT32)src.vEncodeComplexity[(size_t)i];
    }
    dst.nDefaultComplexity = (UINT32)src.nDefaultComplexity;
    dst.bSupportSVC = (INT32)src.bSupportSVC;
    dst.bSupportStreamSmooth = (INT32)src.bSupportStreamSmooth;
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
        ParseResolutionName(src.aResolution[0].strName, dst.stVideoResolution);
        dst.enFrameRate = (INT32)FrameRateEnumToFloat(src.aResolution[0].enFrameRateMax);
        dst.nAverageBitrate = (INT32)src.aResolution[0].nBitRateMin;
        dst.nBitrateUpperLimit = (INT32)src.aResolution[0].nBitRateMax;
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

    dst.nIFrameInterval = src.nIFrameIntervalMax;
}

static void FillOneStreamCap(const Video_NS::VideoCapability_S &src, NET_TV_VIDEO_STREAM_CAP_S &dst, INT32 streamType)
{
    memset(&dst, 0, sizeof(dst));
    dst.dwStreamType = streamType;
    dst.bSupportMultiStream = (INT32)src.bSupportMultiStream;

    /* 编码能力列表 */
    dst.dwEncodeTypeNum = src.nEncodeTypeNum;
    dst.dwEncodeAbilityNum = (INT32)std::min(src.aEncodeAbility.size(), (size_t)NET_TV_VIDEO_ENCODE_TYPE_MAX);
    if (dst.dwEncodeTypeNum <= 0)
    {
        dst.dwEncodeTypeNum = dst.dwEncodeAbilityNum;
    }

    for (INT32 i = 0; i < dst.dwEncodeAbilityNum; ++i)
    {
        FillOneEncodeAbility(src.aEncodeAbility[(size_t)i], dst.astEncodeAbility[i]);
    }

    dst.dwEncodeCapSize = dst.dwEncodeAbilityNum;
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

    /*
     * IPC 视频能力集只返回码流平滑范围，没有单独的图像质量能力范围。
     * 这里保留质量范围的兼容默认值，同时把真实的 StreamSmooth 范围填到对应字段。
     */
    dst.stQuality.dwMin = 1;
    dst.stQuality.dwMax = 100;
    dst.stStreamSmooth.dwMin = src.nStreamSmoothMin;
    dst.stStreamSmooth.dwMax = src.nStreamSmoothMax;
    dst.dwIFrameIntervalMin = src.nIFrameIntervalMin;
    dst.dwIFrameIntervalMax = src.nIFrameIntervalMax;

    /* 分辨率列表 */
    dst.dwResolutionNum = (INT32)std::min(src.aResolution.size(), (size_t)NET_TV_RESOLUTION_NUM_MAX);
    for (INT32 i = 0; i < dst.dwResolutionNum; ++i)
    {
        FillOneResolution(src.aResolution[i], dst.astResolution[i]);
    }
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
#endif

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

#ifdef SCENE_INTELLIGENCE
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
