/**
 * @FilePath     : tvsdk_server.cpp
 * @Description  : TVSDK 服务端封装实现，对接 NetTVSDKServer.h C 接口；能力集通过 control_manage 命令码获取
 */

#include "tvsdk_server.h"
#include "dlog.h"
#include "action_code.h"
#include "user_manage.h"
#include "user_define.h"

#include "callbacks/tvsdk_callbacks.h"

#include "NetTVSDKServer.h"
#include "av_configure.h"
#include "voice_com_capture_source.h"
#include "system_manage.h"
#include "network_manage.h"
#include "task_manage.h"
#include "convert_interface.h"
#include "alarm_convert.h"
#include "convert/tvsdk_convert.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

/* 回调实现/注册已拆分到 callbacks/ 目录 */

namespace
{
const char *kDefaultDiscoveryInterface = "eth0";
const char *kDefaultDiscoveryManufacturer = "ITC";
const char *kVoiceComNvrRecvDumpPath = "/tmp/tvsdk_voicecom_nvr_recv.pcm";
constexpr UINT32 kVoiceComPort = 9006;
constexpr int kNetTvAudioFormatG711U = 1;
constexpr int kNetTvAudioFormatG711A = 2;
constexpr int kNetTvAudioFormatPcm = 6;
constexpr size_t kVoiceComNvrRecvDumpMaxBytes = 5 * 1024 * 1024;
UINT32 g_discoveryHttpPort = IN_CONTROL_SDK_PROT;
std::string g_discoveryInterface = kDefaultDiscoveryInterface;
NET_DiscoveryDeviceInfo_S g_cachedDiscoveryInfo;
std::chrono::steady_clock::time_point g_cachedDiscoveryInfoTime;
bool g_hasCachedDiscoveryInfo = false;
FILE *g_voiceComNvrRecvDumpFp = nullptr;
size_t g_voiceComNvrRecvDumpBytes = 0;
constexpr auto kDiscoveryInfoCacheTtl = std::chrono::seconds(30);

void fill_default_voice_com_audio_param(NET_VoiceComAudioParam_S &audioParam)
{
    std::memset(&audioParam, 0, sizeof(audioParam));
    audioParam.enFormat = kNetTvAudioFormatPcm;
    audioParam.uSampleRate = 16000;
    audioParam.uBitDepth = 16;
    audioParam.uChannels = 1;
    audioParam.uFrameIntervalMs = 20;
    audioParam.uFrameBytes = 640;
    audioParam.uBitRate = 256000;
    audioParam.bLittleEndian = TRUE;
}

Audio_NS::AudioFormat_E voice_com_format_to_ipc(int format)
{
    switch (format)
    {
        case kNetTvAudioFormatG711A:
            return Audio_NS::AudioFormat_E::G711A;
        case kNetTvAudioFormatG711U:
            return Audio_NS::AudioFormat_E::G711U;
        case kNetTvAudioFormatPcm:
        default:
            return Audio_NS::AudioFormat_E::PCM;
    }
}

void dump_voice_com_nvr_audio(const char *data, unsigned int size)
{
    if (!data || size == 0 || g_voiceComNvrRecvDumpBytes >= kVoiceComNvrRecvDumpMaxBytes)
        return;

    if (!g_voiceComNvrRecvDumpFp)
    {
        /*
         * 调试用：保存 NVR 发到 IPC 的原始 VoiceCom 音频。
         * 当前 NVR 按 PCM 对接时，该文件就是 16k/16bit/mono/s16le 裸 PCM，可用 ffplay 直接验证。
         */
        g_voiceComNvrRecvDumpFp = std::fopen(kVoiceComNvrRecvDumpPath, "wb");
        if (!g_voiceComNvrRecvDumpFp)
        {
            dlog_warn("TVSDK VoiceCom open NVR recv dump failed, path=%s", kVoiceComNvrRecvDumpPath);
            g_voiceComNvrRecvDumpBytes = kVoiceComNvrRecvDumpMaxBytes;
            return;
        }
        dlog_info("TVSDK VoiceCom dumping NVR audio to %s", kVoiceComNvrRecvDumpPath);
    }

    size_t nWriteSize = size;
    if (g_voiceComNvrRecvDumpBytes + nWriteSize > kVoiceComNvrRecvDumpMaxBytes)
        nWriteSize = kVoiceComNvrRecvDumpMaxBytes - g_voiceComNvrRecvDumpBytes;

    if (nWriteSize > 0)
    {
        std::fwrite(data, 1, nWriteSize, g_voiceComNvrRecvDumpFp);
        std::fflush(g_voiceComNvrRecvDumpFp);
        g_voiceComNvrRecvDumpBytes += nWriteSize;
    }
}

void close_voice_com_nvr_audio_dump()
{
    if (g_voiceComNvrRecvDumpFp)
    {
        /*
         * 对讲停止时关闭 dump 文件，确保落盘完整，方便把文件取出来单独播放验证。
         */
        std::fclose(g_voiceComNvrRecvDumpFp);
        g_voiceComNvrRecvDumpFp = nullptr;
        dlog_info("TVSDK VoiceCom NVR audio dump saved, path=%s bytes=%zu",
                  kVoiceComNvrRecvDumpPath,
                  g_voiceComNvrRecvDumpBytes);
    }
    g_voiceComNvrRecvDumpBytes = 0;
}

#ifdef SCENE_INTELLIGENCE
/**
 * @brief 从任务发布消息中提取抓拍业务数据。
 * @details TaskPublish 会把业务 JSON 包装在 Data 字段中；为兼容旧发布者，
 *          未包含 Data 时直接使用原始消息。
 * @param [in] pData 任务发布的 JSON 数据指针。
 * @param [in] nDataLength 任务发布数据长度。
 * @return 有效的抓拍业务 JSON；解析失败时返回空字符串。
 */
static std::string ExtractCaptureEventData(const void *pData, int nDataLength)
{
    if (!pData || nDataLength <= 0)
    {
        return std::string();
    }

    const std::string strMessage(static_cast<const char *>(pData), static_cast<size_t>(nDataLength));
    Json::Object *pRootJson = Json::init(strMessage);
    if (!pRootJson)
    {
        return std::string();
    }

    std::string strPayload = strMessage;
    Json::Object *pPayloadJson = Json::get(pRootJson, "Data");
    if (pPayloadJson)
    {
        strPayload = Json::to_string(pPayloadJson);
    }
    Json::deinit(pRootJson);
    return strPayload;
}
#endif

void STDCALL cb_voice_com_play(const char *data, unsigned int size)
{
    if (!data || size == 0)
        return;

    dump_voice_com_nvr_audio(data, size);

    NET_VoiceComAudioParam_S audioParam;
    fill_default_voice_com_audio_param(audioParam);
    if (NET_SERVER_GetVoiceComAudioParam(&audioParam))
    {
        int nRet = CAVConfigure::instance()->setAudioAoSampleRate(
            static_cast<Audio_NS::AudioSamprate_E>(audioParam.uSampleRate));
        if (nRet != OK)
        {
            dlog_warn("TVSDK VoiceCom set AO sample rate failed, sampleRate=%d ret=%d",
                      audioParam.uSampleRate, nRet);
        }
    }

    Audio_NS::AoInfo_S stAoInfo;
    stAoInfo.nChannel = 0;
    stAoInfo.pData = reinterpret_cast<uint8_t *>(const_cast<char *>(data));
    stAoInfo.nLen = size;
    stAoInfo.enAudioFormat = voice_com_format_to_ipc(audioParam.enFormat);
    CAVConfigure::instance()->setAoSpeakInfo(stAoInfo);
}

INT32 STDCALL cb_voice_com_capture(const NET_VoiceComAudioParam_S *pstAudioParam,
                                   CHAR *pBuffer,
                                   UINT32 dwBufferSize,
                                   LPVOID lpUserData)
{
    (void)lpUserData;
    if (!pstAudioParam || !pBuffer || dwBufferSize == 0)
        return 0;

    /*
     * 当前与 NVR 约定 VoiceCom 回传只使用 PCM 16k/16bit/mono。
     * 如果 NVR 后续改成 G711，需要在这里或采集源中补重采样/编码转换。
     */
    const int nExpectedFrameBytes =
        pstAudioParam->uSampleRate *
        pstAudioParam->uChannels *
        (pstAudioParam->uBitDepth / 8) *
        pstAudioParam->uFrameIntervalMs / 1000;
    if (pstAudioParam->enFormat != kNetTvAudioFormatPcm ||
        pstAudioParam->uSampleRate != 16000 ||
        pstAudioParam->uBitDepth != 16 ||
        pstAudioParam->uChannels != 1 ||
        pstAudioParam->bLittleEndian != TRUE ||
        pstAudioParam->uFrameIntervalMs <= 0 ||
        pstAudioParam->uFrameBytes <= 0 ||
        pstAudioParam->uFrameBytes != nExpectedFrameBytes)
    {
        dlog_warn("TVSDK VoiceCom capture only supports PCM/16k/16bit/mono/little-endian now, format=%d sampleRate=%d bitDepth=%d channels=%d littleEndian=%d frameMs=%d frameBytes=%d expected=%d",
                  pstAudioParam->enFormat,
                  pstAudioParam->uSampleRate,
                  pstAudioParam->uBitDepth,
                  pstAudioParam->uChannels,
                  pstAudioParam->bLittleEndian,
                  pstAudioParam->uFrameIntervalMs,
                  pstAudioParam->uFrameBytes,
                  nExpectedFrameBytes);
        return 0;
    }

    int nRead = CVoiceComCaptureSource::instance()->read_pcm_frame(
        pBuffer,
        dwBufferSize,
        pstAudioParam->uFrameBytes);
    if (nRead <= 0)
        return 0;

    return nRead;
}

void start_voice_com_server()
{
    if (!NET_SERVER_RegisterCb_VoiceComPlay(cb_voice_com_play))
    {
        dlog_warn("TVSDK VoiceCom register play callback failed");
    }
    else if (!NET_SERVER_RegisterCb_VoiceComCapture(cb_voice_com_capture, nullptr))
    {
        dlog_warn("TVSDK VoiceCom register capture callback failed");
    }
    else if (!NET_SERVER_StartVoiceComServer(kVoiceComPort))
    {
        dlog_warn("TVSDK VoiceCom server start failed, port=%u", kVoiceComPort);
    }
    else
    {
        dlog_info("TVSDK VoiceCom server started, port=%u", kVoiceComPort);
    }
}

void stop_voice_com_server()
{
    close_voice_com_nvr_audio_dump();
    CVoiceComCaptureSource::instance()->clear();
    NET_SERVER_StopVoiceComServer();
}

template <size_t N>
void copy_text(CHAR (&dst)[N], const std::string &src)
{
    std::snprintf(dst, N, "%s", src.c_str());
}

template <size_t N>
void copy_text(CHAR (&dst)[N], const char *src)
{
    std::snprintf(dst, N, "%s", src ? src : "");
}

std::string first_not_empty(const std::string &first, const std::string &second)
{
    return first.empty() ? second : first;
}

std::string get_discovery_interface()
{
    Network::Info_S stNet;
    std::string iface;
    if (CNetworkManage::instance()->get_system_networkInfo(stNet) == OK)
        iface = stNet.stIp.netName;

    return iface.empty() ? kDefaultDiscoveryInterface : iface;
}

void STDCALL cb_get_discovery_device_info(NET_DiscoveryDeviceInfo_S *pInfo)
{
    if (!pInfo)
        return;

    auto now = std::chrono::steady_clock::now();
    if (g_hasCachedDiscoveryInfo && now - g_cachedDiscoveryInfoTime < kDiscoveryInfoCacheTtl)
    {
        std::memcpy(pInfo, &g_cachedDiscoveryInfo, sizeof(*pInfo));
        return;
    }

    std::memset(pInfo, 0, sizeof(*pInfo));

    System::DeviceInfo_S stDev;
    if (SystemManage::instance()->get_device_info(stDev) == OK)
    {
        copy_text(pInfo->strDeviceName, first_not_empty(stDev.deviceName, "Camera"));
        copy_text(pInfo->strDeviceID, first_not_empty(stDev.serialNumber, std::to_string(stDev.deviceID)));
        copy_text(pInfo->strDeviceType, first_not_empty(stDev.strUnitTpye, "IPC"));
        copy_text(pInfo->strFirmwareVersion, first_not_empty(stDev.systemVersion, stDev.hardwareVersion));
    }
    else
    {
        copy_text(pInfo->strDeviceName, "Camera");
        copy_text(pInfo->strDeviceID, "1");
        copy_text(pInfo->strDeviceType, "IPC");
    }

    Network::Info_S stNet;
    stNet.stIp.netName = g_discoveryInterface.empty() ? kDefaultDiscoveryInterface : g_discoveryInterface;
    if (CNetworkManage::instance()->get_ip_and_dns(stNet) == OK)
    {
        copy_text(pInfo->strIPv4Address, stNet.stIp.ipv4Ip);
        copy_text(pInfo->strIPv4SubnetMask, stNet.stIp.ipv4Mask);
        copy_text(pInfo->strIPv4Gateway, stNet.stIp.ipv4Gateway);

        std::string mac = stNet.stIp.physicalAddress;
        if (mac.empty())
            mac = CNetworkManage::instance()->get_macAddress(stNet.stIp.netName);
        copy_text(pInfo->strMACAddress, mac);
    }

    pInfo->uHttpPort = g_discoveryHttpPort;
    copy_text(pInfo->strManufacturer, kDefaultDiscoveryManufacturer);

    dlog_debug("TVSDK discovery info: name[%s] id[%s] ip[%s] mac[%s] port[%u]",
               pInfo->strDeviceName,
               pInfo->strDeviceID,
               pInfo->strIPv4Address,
               pInfo->strMACAddress,
               pInfo->uHttpPort);

    std::memcpy(&g_cachedDiscoveryInfo, pInfo, sizeof(g_cachedDiscoveryInfo));
    g_cachedDiscoveryInfoTime = now;
    g_hasCachedDiscoveryInfo = true;
}

}

/* ---------- CTvSdkServer 实现 ---------- */

int CTvSdkServer::init()
{
    if (m_bInit)
    {
        dlog_warn("TVSDK server already inited");
        return OK;
    }

    UINT32 port = (m_udwPort > 0) ? (UINT32)m_udwPort : (UINT32)IN_CONTROL_SDK_PROT;
    CHAR szUser[NET_LEN_132] = {0};
    CHAR szPass[NET_LEN_132] = {0};
    /* 通过用户管理获取管理员用户名和密码 */
    std::string strAdminUser = USER_DEFAULT_NAME;
    std::string strAdminPass = CUserManage::instance()->get_passwd(USER_DEFAULT_NAME);
    if (strAdminPass.empty() && !m_strPassword.empty())
        strAdminPass = m_strPassword;
    if (!m_strUser.empty())
        strAdminUser = m_strUser;
    strncpy(szUser, strAdminUser.c_str(), sizeof(szUser) - 1);
    szUser[sizeof(szUser) - 1] = '\0';
    strncpy(szPass, strAdminPass.c_str(), sizeof(szPass) - 1);
    szPass[sizeof(szPass) - 1] = '\0';

    BOOL bRet = NET_SERVER_Init(port, szUser, szPass);
    if (!bRet)
    {
        dlog_error("NET_SERVER_Init failed, port=%u", port);
        return -1;
    }

    /* 设置 TaskManage 并注册所有回调 */
    TvSdkCallbacks::set_task_manage(m_pTaskManage);
    TvSdkCallbacks::register_all();

    start_voice_com_server();

    g_discoveryHttpPort = port;
    g_discoveryInterface = get_discovery_interface();
    if (!NET_SERVER_RegisterCb_GetDiscoveryDeviceInfo(cb_get_discovery_device_info))
    {
        dlog_warn("TVSDK discovery register callback failed");
    }
    else if (!NET_SERVER_Discovery_Start(g_discoveryInterface.c_str()))
    {
        dlog_warn("TVSDK discovery start failed, iface=%s", g_discoveryInterface.c_str());
    }
    else
    {
        dlog_info("TVSDK discovery started, iface=%s", g_discoveryInterface.c_str());
    }

    m_bInit = true;
#ifdef SCENE_INTELLIGENCE
    register_capture_event_subscribers();
#endif
    dlog_info("TVSDK server init ok, port=%u", port);
    return OK;
}

void CTvSdkServer::deinit()
{
    if (!m_bInit)
        return;
    stop_voice_com_server();
    NET_SERVER_Discovery_Stop();
    NET_SERVER_Cleanup();
    TvSdkCallbacks::clear_task_manage();
    m_bInit = false;
    dlog_info("TVSDK server deinit");
}

void CTvSdkServer::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = std::move(pTaskManage);
    TvSdkCallbacks::set_task_manage(m_pTaskManage);
}

#ifdef SCENE_INTELLIGENCE
/**
 * @brief 注册智能抓拍事件订阅。
 * @details 同一 CTaskManage 只注册一次，避免 TVSDK 重启后重复推送相同抓拍事件。
 * @return 无。
 */
void CTvSdkServer::register_capture_event_subscribers()
{
    if (!m_pTaskManage || m_bCaptureEventSubscribed)
    {
        return;
    }

    const std::vector<int> vecActionCodes = {
        AC_PUSH_FACE_CAPTURE_INFO,
        AC_PUSH_PERSON_CAPTURE_INFO,
        AC_PUSH_MOTORVEHICLE_CAPTURE_INFO,
        AC_PUSH_NONMOTORVEHICLE_CAPTURE_INFO};
    m_pTaskManage->register_subscribe(
        vecActionCodes,
        std::bind(&CTvSdkServer::handle_capture_event,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3,
                  std::placeholders::_4));
    m_bCaptureEventSubscribed = true;
}

/**
 * @brief 转换并推送一条智能抓拍事件。
 * @param [in] pData 任务发布的 JSON 数据。
 * @param [in] nDataLength JSON 数据长度。
 * @param [in] nActionCode 抓拍事件动作码。
 * @param [in] pUserData 订阅用户数据，当前未使用。
 * @return 推送成功返回 OK，失败返回 ERR。
 */
int CTvSdkServer::handle_capture_event(const void *pData,
                                       int nDataLength,
                                       int nActionCode,
                                       void *pUserData)
{
    (void)pUserData;
    if (!m_bInit)
    {
        return ERR;
    }

    const std::string strCaptureData = ExtractCaptureEventData(pData, nDataLength);
    if (strCaptureData.empty())
    {
        dlog_warn("TVSDK capture event payload is empty, action=%d", nActionCode);
        return ERR;
    }

    BOOL bPushResult = FALSE;
    switch (nActionCode)
    {
        case AC_PUSH_FACE_CAPTURE_INFO:
        {
            Alarm::FaceAlarmInfo_S stSource;
            NET_FaceCapturePushInfo_S stDestination = {};
            Convert::to_struct(strCaptureData, stSource);
            TvSdkConvert::FillFaceCapturePushInfo(stSource, stDestination);
            bPushResult = push_alarm(nullptr,
                                     NET_PUSH_FACE_CAPTURE_INFO,
                                     &stDestination,
                                     static_cast<int>(sizeof(stDestination))) == OK;
            break;
        }
        case AC_PUSH_PERSON_CAPTURE_INFO:
        {
            Alarm::PersonAlarmInfo_S stSource;
            NET_PersonCapturePushInfo_S stDestination = {};
            Convert::to_struct(strCaptureData, stSource);
            TvSdkConvert::FillPersonCapturePushInfo(stSource, stDestination);
            bPushResult = push_alarm(nullptr,
                                     NET_PUSH_PERSON_CAPTURE_INFO,
                                     &stDestination,
                                     static_cast<int>(sizeof(stDestination))) == OK;
            break;
        }
        case AC_PUSH_MOTORVEHICLE_CAPTURE_INFO:
        {
            Alarm::MotorvehicleAlarmInfo_S stSource;
            NET_MotorvehicleCapturePushInfo_S stDestination = {};
            Convert::to_struct(strCaptureData, stSource);
            TvSdkConvert::FillMotorvehicleCapturePushInfo(stSource, stDestination);
            bPushResult = push_alarm(nullptr,
                                     NET_PUSH_MOTORVEHICLE_CAPTURE_INFO,
                                     &stDestination,
                                     static_cast<int>(sizeof(stDestination))) == OK;
            break;
        }
        case AC_PUSH_NONMOTORVEHICLE_CAPTURE_INFO:
        {
            Alarm::NonMotorvehicleAlarmInfo_S stSource;
            NET_NonMotorvehicleCapturePushInfo_S stDestination = {};
            Convert::to_struct(strCaptureData, stSource);
            TvSdkConvert::FillNonMotorvehicleCapturePushInfo(stSource, stDestination);
            bPushResult = push_alarm(nullptr,
                                     NET_PUSH_NONMOTORVEHICLE_CAPTURE_INFO,
                                     &stDestination,
                                     static_cast<int>(sizeof(stDestination))) == OK;
            break;
        }
        default:
        {
            dlog_warn("TVSDK capture event action is unsupported, action=%d", nActionCode);
            return ERR;
        }
    }

    if (!bPushResult)
    {
        dlog_warn("TVSDK capture event push failed, action=%d", nActionCode);
        return ERR;
    }
    return OK;
}
#endif

int CTvSdkServer::push_alarm(const void *pAlarmer, int lCommand, const void *pAlarmInfo, int dwBufLen)
{
    if (!m_bInit || !pAlarmInfo || dwBufLen <= 0)
        return -1;
     // SDK 内部通常要求 pAlarmer 非空，这里按需填默认告警设备信息
    NET_Alarmer_S stAlarmer;
    NET_Alarmer_S *pUseAlarmer = (NET_Alarmer_S *)pAlarmer;
    if (!pUseAlarmer)
    {
        memset(&stAlarmer, 0, sizeof(stAlarmer));

        // device info
        ::System::DeviceInfo_S stDev;
        if (SystemManage::instance()->get_device_info(stDev) == 0)
        {
            strncpy((char *)stAlarmer.strSerialNumber, stDev.serialNumber.c_str(), sizeof(stAlarmer.strSerialNumber) - 1);
            strncpy(stAlarmer.strDeviceName, stDev.deviceName.c_str(), sizeof(stAlarmer.strDeviceName) - 1);
        }

        // ip + mac
        Network::Info_S stNet;
        if (CNetworkManage::instance()->get_ip_and_dns(stNet) == 0)
        {
            strncpy(stAlarmer.strDeviceIP, stNet.stIp.ipv4Ip.c_str(), sizeof(stAlarmer.strDeviceIP) - 1);
        }
        std::string mac = CNetworkManage::instance()->get_macAddress("eth0");
        unsigned int b[6] = {0};
        if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6)
        {
            for (int i = 0; i < 6; ++i)
                stAlarmer.byMacAddr[i] = (BYTE)(b[i] & 0xFF);
        }

        pUseAlarmer = &stAlarmer;
    }
    if ((lCommand & 0xF000) == NET_ALARM_BASE_BASIC)
    {
        dlog_info("[基础告警推送诊断] push_alarm 进入TVSDK层: cmd[0x%x] buf_len[%d] expect_size[%zu]",
                  lCommand,
                  dwBufLen,
                  sizeof(NET_AlarmBasicInfo_S));
        if (dwBufLen >= static_cast<int>(sizeof(NET_AlarmBasicInfo_S)))
        {
            const NET_AlarmBasicInfo_S *pBasic =
                static_cast<const NET_AlarmBasicInfo_S *>(pAlarmInfo);
            dlog_info("[基础告警推送诊断] push_alarm 基础内容: alarm_type[0x%x] 时间戳[%lld] 全景图长度[%u]",
                      pBasic->uAlarmType,
                      static_cast<long long>(pBasic->llTimestampMs),
                      pBasic->uPanoramaImgLen);
        }
        else
        {
            dlog_warn("[基础告警推送诊断] push_alarm 缓冲区过小: cmd[0x%x] buf_len[%d] expect_size[%zu]",
                      lCommand,
                      dwBufLen,
                      sizeof(NET_AlarmBasicInfo_S));
        }
    }
    else if ((lCommand & 0xF000) == NET_ALARM_BASE_STATISTICS)
    {
        dlog_info("[统计推送诊断] push_alarm 进入TVSDK层: cmd[0x%x] buf_len[%d] expect_size[%zu]", lCommand, dwBufLen,
                  sizeof(NET_AlarmStatisticsInfo_S));
        if (dwBufLen >= static_cast<int>(sizeof(NET_AlarmStatisticsInfo_S)))
        {
            const NET_AlarmStatisticsInfo_S *pStatistics =
                static_cast<const NET_AlarmStatisticsInfo_S *>(pAlarmInfo);
            dlog_info("[统计推送诊断] push_alarm 统计内容: alarm_type[0x%x] 通道[%u] 类型[%u] 规则[%u] 时间戳[%lld] 序号[%u] "
                      "进入[%u] 离开[%u] 总数[%u] 当前人数[%u] 目标数[%u] 全景图长度[%u]",
                      pStatistics->uAlarmType,
                      pStatistics->uChannel,
                      pStatistics->uStatisticsType,
                      pStatistics->uRuleID,
                      static_cast<long long>(pStatistics->llTimestampMs),
                      pStatistics->uReportSeq,
                      pStatistics->uEnterCount,
                      pStatistics->uLeaveCount,
                      pStatistics->uTotalCount,
                      pStatistics->uCurrentPeopleCount,
                      pStatistics->uTargetCount,
                      pStatistics->uPanoramaImgLen);
        }
        else
        {
            dlog_warn("[统计推送诊断] push_alarm 缓冲区过小: cmd[0x%x] buf_len[%d] expect_size[%zu]", lCommand, dwBufLen,
                      sizeof(NET_AlarmStatisticsInfo_S));
        }
    }
    else if ((lCommand & 0xF000) == NET_ALARM_BASE_RULE)
    {
        dlog_info("[周界推送诊断] push_alarm 进入TVSDK层: cmd[0x%x] buf_len[%d] expect_size[%zu]",
                  lCommand,
                  dwBufLen,
                  sizeof(NET_AlarmRuleInfo_S));
        if (dwBufLen >= static_cast<int>(sizeof(NET_AlarmRuleInfo_S)))
        {
            const NET_AlarmRuleInfo_S *pRule =
                static_cast<const NET_AlarmRuleInfo_S *>(pAlarmInfo);
            dlog_info("[周界推送诊断] push_alarm 周界内容: alarm_type[0x%x] 通道[%u] 规则[%u] 目标[%u] "
                      "类型[%u] 时间戳[%lld] 框[%d,%d,%d,%d] 全景图长度[%u] 特写图长度[%u]",
                      pRule->uAlarmType,
                      pRule->uChannel,
                      pRule->uRuleID,
                      pRule->uTargetID,
                      pRule->uObjectType,
                      static_cast<long long>(pRule->llTimestampMs),
                      pRule->nLeft,
                      pRule->nTop,
                      pRule->nRight,
                      pRule->nBottom,
                      pRule->uPanoramaImgLen,
                      pRule->uTargetImgLen);
        }
        else
        {
            dlog_warn("[周界推送诊断] push_alarm 缓冲区过小: cmd[0x%x] buf_len[%d] expect_size[%zu]",
                      lCommand,
                      dwBufLen,
                      sizeof(NET_AlarmRuleInfo_S));
        }
    }
    else if ((lCommand & 0xF000) == NET_ALARM_BASE_AI)
    {
        dlog_info("[AI目标推送诊断] push_alarm 进入TVSDK层: cmd[0x%x] buf_len[%d] expect_size[%zu]",
                  lCommand,
                  dwBufLen,
                  sizeof(NET_AlarmAiObjectInfo_S));
        if (dwBufLen >= static_cast<int>(sizeof(NET_AlarmAiObjectInfo_S)))
        {
            const NET_AlarmAiObjectInfo_S *pObject =
                static_cast<const NET_AlarmAiObjectInfo_S *>(pAlarmInfo);
            dlog_info("[AI目标推送诊断] push_alarm AI目标内容: alarm_type[0x%x] 通道[%u] 目标[%s] "
                      "类型[%u] 时间戳[%lld] 框[%d,%d,%d,%d] 全景图长度[%u] 特写图长度[%u]",
                      pObject->uAlarmType,
                      pObject->uChannel,
                      pObject->strObjectID,
                      pObject->uObjectType,
                      static_cast<long long>(pObject->llTimestampMs),
                      pObject->nLeft,
                      pObject->nTop,
                      pObject->nRight,
                      pObject->nBottom,
                      pObject->uPanoramaImgLen,
                      pObject->uImgLen);
        }
        else
        {
            dlog_warn("[AI目标推送诊断] push_alarm 缓冲区过小: cmd[0x%x] buf_len[%d] expect_size[%zu]",
                      lCommand,
                      dwBufLen,
                      sizeof(NET_AlarmAiObjectInfo_S));
        }
    }

    BOOL bRet = NET_SERVER_PushAlarmInfo(
         pUseAlarmer,
        (INT32)lCommand,
        (LPVOID)pAlarmInfo,
        (INT32)dwBufLen);

    if ((lCommand & 0xF000) == NET_ALARM_BASE_STATISTICS)
    {
        dlog_info("[统计推送诊断] push_alarm NET_SERVER_PushAlarmInfo 返回: cmd[0x%x] bRet[%d]", lCommand, bRet);
    }

    return bRet ? OK : -1;
}

/**
 * @brief 推送包含动态图片的 V2 告警。
 * @details V2 图片指针只在本次同步调用期间读取，调用者必须保证返回前内存有效。
 * @param [in] pAlarmer 告警设备信息，为空时自动填充本机信息。
 * @param [in] lCommand 告警命令码。
 * @param [in] pAlarmInfo V2 告警结构体。
 * @param [in] dwBufLen V2 告警结构体长度。
 * @return 成功返回 OK，失败返回 ERR。
 */
int CTvSdkServer::push_alarm_v2(const void *pAlarmer,
                                int lCommand,
                                const void *pAlarmInfo,
                                int dwBufLen)
{
    if (!m_bInit || !pAlarmInfo || dwBufLen <= 0)
    {
        return ERR;
    }

    NET_Alarmer_S stAlarmer = {};
    NET_Alarmer_S *pUseAlarmer = static_cast<NET_Alarmer_S *>(const_cast<void *>(pAlarmer));
    if (!pUseAlarmer)
    {
        System::DeviceInfo_S stDeviceInfo;
        if (SystemManage::instance()->get_device_info(stDeviceInfo) == OK)
        {
            std::strncpy(reinterpret_cast<char *>(stAlarmer.strSerialNumber),
                         stDeviceInfo.serialNumber.c_str(),
                         sizeof(stAlarmer.strSerialNumber) - 1);
            std::strncpy(stAlarmer.strDeviceName,
                         stDeviceInfo.deviceName.c_str(),
                         sizeof(stAlarmer.strDeviceName) - 1);
        }

        Network::Info_S stNetworkInfo;
        if (CNetworkManage::instance()->get_ip_and_dns(stNetworkInfo) == OK)
        {
            std::strncpy(stAlarmer.strDeviceIP,
                         stNetworkInfo.stIp.ipv4Ip.c_str(),
                         sizeof(stAlarmer.strDeviceIP) - 1);
        }

        const std::string strMacAddress = CNetworkManage::instance()->get_macAddress("eth0");
        unsigned int auMacBytes[6] = {};
        if (std::sscanf(strMacAddress.c_str(),
                        "%x:%x:%x:%x:%x:%x",
                        &auMacBytes[0], &auMacBytes[1], &auMacBytes[2],
                        &auMacBytes[3], &auMacBytes[4], &auMacBytes[5]) == 6)
        {
            for (int i = 0; i < 6; ++i)
            {
                stAlarmer.byMacAddr[i] = static_cast<BYTE>(auMacBytes[i] & 0xFF);
            }
        }
        pUseAlarmer = &stAlarmer;
    }

    return NET_SERVER_PushAlarmInfoV2(pUseAlarmer,
                                      static_cast<INT32>(lCommand),
                                      const_cast<void *>(pAlarmInfo),
                                      static_cast<INT32>(dwBufLen)) ? OK : ERR;
}

int CTvSdkServer::get_client_count() const
{
    if (!m_bInit)
    {
        dlog_error("TVSDK server not inited, get client count failed");
        return -1;
    }

    return (int)NET_SERVER_GetClientCount();
}

void CTvSdkServer::set_port(unsigned int port)
{
    m_udwPort = port;
}

void CTvSdkServer::set_user_passwd(const std::string &user, const std::string &passwd)
{
    m_strUser = user;
    m_strPassword = passwd;
}
