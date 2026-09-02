/**
 * @FilePath     : tvsdk_server.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-23 11:17:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-19 16:47:18
 * @Description  : TVSDK 服务端封装实现，对接 NetTVSDKServer.h C 接口；能力集通过 control_manage 命令码获取
 */

#include "tvsdk_server.h"
#include "dlog.h"
#include "user_manage.h"
#include "user_define.h"
#include "action_code.h"
#include "callbacks/tvsdk_callbacks.h"

#include "NetTVSDKServer.h"
#include "av_configure.h"
#include "voice_com_capture_source.h"
#include "system_manage.h"
#include "network_manage.h"
#include "task_manage.h"
#include "convert/tvsdk_convert.h"
#include "convert_interface.h"
#include "Json.h"

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <string>

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
constexpr std::size_t kVoiceComNvrRecvDumpMaxBytes = 5 * 1024 * 1024;
UINT32 g_discoveryHttpPort = IN_CONTROL_SDK_PROT;
std::string g_discoveryInterface = kDefaultDiscoveryInterface;
NET_DiscoveryDeviceInfo_S g_cachedDiscoveryInfo;
std::chrono::steady_clock::time_point g_cachedDiscoveryInfoTime;
bool g_hasCachedDiscoveryInfo = false;
FILE *g_voiceComNvrRecvDumpFp = nullptr;
std::size_t g_voiceComNvrRecvDumpBytes = 0;
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

    std::size_t nWriteSize = size;
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


void STDCALL cb_voice_com_play(const char *data, unsigned int size)
{
    if (!data || size == 0)
        return;

    dump_voice_com_nvr_audio(data, size);

    NET_VoiceComAudioParam_S audioParam;
    fill_default_voice_com_audio_param(audioParam);
    if (NET_serverGetVoiceComAudioParam(&audioParam))
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
    if (!NET_serverRegisterVoiceComPlayCb(cb_voice_com_play))
    {
        dlog_warn("TVSDK VoiceCom register play callback failed");
    }
    else if (!NET_serverRegisterVoiceComCaptureCb(cb_voice_com_capture, nullptr))
    {
        dlog_warn("TVSDK VoiceCom register capture callback failed");
    }
    else if (!NET_serverStartVoiceComServer(kVoiceComPort))
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
    NET_serverStopVoiceComServer();
}

template <std::size_t N>
void copy_text(CHAR (&dst)[N], const std::string &src)
{
    std::snprintf(dst, N, "%s", src.c_str());
}

template <std::size_t N>
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

NET_COMMON_ECODE_E STDCALL cb_set_discovery_network(const NET_PoeNetworkConfig_S *pConfig)
{
    const int nRet = TvSdkCallbacks::apply_discovery_network(pConfig);
    if (nRet == NET_E_SUCCEED)
        g_hasCachedDiscoveryInfo = false;
    return static_cast<NET_COMMON_ECODE_E>(nRet);
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
    CHAR szDeviceName[NET_LEN_132] = "IPC";
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

    System::DeviceInfo_S stDeviceInfo{};
    if (SystemManage::instance()->get_device_info(stDeviceInfo) == OK && !stDeviceInfo.deviceName.empty())
    {
        strncpy(szDeviceName, stDeviceInfo.deviceName.c_str(), sizeof(szDeviceName) - 1);
        szDeviceName[sizeof(szDeviceName) - 1] = '\0';
    }

    BOOL bRet = NET_serverInit(port, szUser, szPass, szDeviceName);
    if (!bRet)
    {
        dlog_error("NET_serverInit failed, port=%u", port);
        return -1;
    }

    /* 设置 TaskManage 并注册所有回调 */
    TvSdkCallbacks::set_task_manage(m_pTaskManage);
    TvSdkCallbacks::register_all();

    start_voice_com_server();

    g_discoveryHttpPort = port;
    g_discoveryInterface = get_discovery_interface();
    /* perf: 将告警设备信息读取放到启动阶段，避免事件线程重复执行网络命令。 */
    prepare_default_alarmer();
    if (!NET_serverRegisterGetDiscoveryDeviceInfoCb(cb_get_discovery_device_info))
    {
        dlog_warn("TVSDK discovery register callback failed");
    }
    else if (!NET_serverRegisterSetNetworkCb(cb_set_discovery_network))
    {
        dlog_warn("TVSDK discovery register set network callback failed");
    }
    else if (!NET_serverStartDiscovery(g_discoveryInterface.c_str()))
    {
        dlog_warn("TVSDK discovery start failed, iface=%s", g_discoveryInterface.c_str());
    }
    else
    {
        dlog_info("TVSDK discovery started, iface=%s", g_discoveryInterface.c_str());
    }

    m_bInit = true;
    subscribe_record_download_progress();
    dlog_info("TVSDK server init ok, port=%u", port);
    return OK;
}

void CTvSdkServer::prepare_default_alarmer()
{
    NET_Alarmer_S stAlarmer{};

    /* 设备信息文件读取和 CPU 序列号获取只在服务启动阶段执行一次。 */
    System::DeviceInfo_S stDev{};
    if (SystemManage::instance()->get_device_info(stDev) == OK)
    {
        std::strncpy(reinterpret_cast<char *>(stAlarmer.strSerialNumber),
                     stDev.serialNumber.c_str(),
                     sizeof(stAlarmer.strSerialNumber) - 1);
        std::strncpy(stAlarmer.strDeviceName,
                     stDev.deviceName.c_str(),
                     sizeof(stAlarmer.strDeviceName) - 1);
    }

    /* get_ip_and_dns() 会执行多次 ifconfig/route/ip 查询，禁止在实时告警线程调用。 */
    Network::Info_S stNet{};
    stNet.stIp.netName = g_discoveryInterface.empty() ? kDefaultDiscoveryInterface : g_discoveryInterface;
    if (CNetworkManage::instance()->get_ip_and_dns(stNet) == OK)
    {
        std::strncpy(stAlarmer.strDeviceIP,
                     stNet.stIp.ipv4Ip.c_str(),
                     sizeof(stAlarmer.strDeviceIP) - 1);
    }

    const std::string strMac = CNetworkManage::instance()->get_macAddress(stNet.stIp.netName);
    unsigned int anMac[6] = {0};
    if (std::sscanf(strMac.c_str(), "%x:%x:%x:%x:%x:%x",
                    &anMac[0], &anMac[1], &anMac[2], &anMac[3], &anMac[4], &anMac[5]) == 6)
    {
        for (int nIndex = 0; nIndex < 6; ++nIndex)
        {
            stAlarmer.byMacAddr[nIndex] = static_cast<BYTE>(anMac[nIndex] & 0xFFU);
        }
    }

    {
        /* lock: 发布完整快照后，告警线程只复制固定大小结构体，不再触发系统 I/O。 */
        std::lock_guard<std::mutex> lock(m_mtxDefaultAlarmer);
        m_stDefaultAlarmer = stAlarmer;
        m_bDefaultAlarmerReady = true;
    }
}

void CTvSdkServer::deinit()
{
    if (!m_bInit)
        return;
    stop_voice_com_server();
    NET_serverStopDiscovery();
    NET_serverCleanup();
    TvSdkCallbacks::clear_task_manage();
    {
        /* lock: 清理旧网络快照，下一次初始化必须重新读取当前网卡信息。 */
        std::lock_guard<std::mutex> lock(m_mtxDefaultAlarmer);
        m_stDefaultAlarmer = NET_Alarmer_S{};
        m_bDefaultAlarmerReady = false;
    }
    m_bInit = false;
    dlog_info("TVSDK server deinit");
}

void CTvSdkServer::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = std::move(pTaskManage);
    TvSdkCallbacks::set_task_manage(m_pTaskManage);
    if (m_bInit)
    {
        subscribe_record_download_progress();
    }
}

void CTvSdkServer::subscribe_record_download_progress()
{
    /* 同一 TaskManage 只注册一次；任务层没有反注册接口，服务生命周期内复用该订阅。 */
    if (!m_pTaskManage || m_pRecordDownloadSubscribeManage == m_pTaskManage.get())
    {
        return;
    }

    m_pTaskManage->register_subscribe(
        AC_NOTICE_DOWNLOAD_RECORD_PROGRESS,
        [this](const void *pData, int nDataLen, int nActionCode, void *) -> int
        {
            (void)nActionCode;
            if (!m_bInit || !pData || nDataLen <= 0)
            {
                return 0;
            }

            const std::string strMessage(static_cast<const char *>(pData),
                                         static_cast<size_t>(nDataLen));
            Json::Object *pRoot = Json::init(strMessage.c_str());
            if (!pRoot)
            {
                dlog_warn("TVSDK录像下载进度通知JSON无效");
                return -1;
            }

            /* TaskPublish 包装为 {ActionCode,...,Data}；同时兼容直接发布 Data 的旧路径。 */
            Json::Object *pDataJson = Json::get(pRoot, "Data");
            const std::string strData = pDataJson ? Json::to_string(pDataJson) : strMessage;
            Json::deinit(pRoot);

            Record_NS::DownloadProgress_S stProgress;
            Convert::to_struct(strData, stProgress);
            if (stProgress.filename.empty())
            {
                dlog_warn("TVSDK录像下载进度缺少文件名: %s", strMessage.c_str());
                return -1;
            }

            NET_RecordDownloadProgress_S stSdkProgress{};
            TvSdkConvert::FillRecordDownloadProgress(stProgress, stSdkProgress);
            /* 481 复用 TVSDK 通用告警通道，但载荷是下载进度而非普通智能告警。 */
            const int nRet = push_alarm(nullptr,
                                        NET_NOTICE_DOWNLOAD_RECORD_PROGRESS,
                                        &stSdkProgress,
                                        static_cast<int>(sizeof(stSdkProgress)));
            if (nRet != OK)
            {
                dlog_warn("TVSDK录像下载进度推送失败: file=%s, progress=%d, ret=%d",
                          stProgress.filename.c_str(), stProgress.nProgress, nRet);
            }
            return nRet;
        });
    m_pRecordDownloadSubscribeManage = m_pTaskManage.get();
}

int CTvSdkServer::push_alarm(const void *pAlarmer, int lCommand, const void *pAlarmInfo, int dwBufLen)
{
    if (!m_bInit || !pAlarmInfo || dwBufLen <= 0)
        return -1;
    /* SDK 内部通常要求 pAlarmer 非空；实时告警路径只复制启动期缓存。 */
    NET_Alarmer_S stAlarmer{};
    NET_Alarmer_S *pUseAlarmer = const_cast<NET_Alarmer_S *>(
        static_cast<const NET_Alarmer_S *>(pAlarmer));
    if (!pUseAlarmer)
    {
        bool bDefaultAlarmerReady = false;
        {
            /* lock: 复制固定长度快照后立即释放，不能让 SDK 调用持有本地缓存锁。 */
            std::lock_guard<std::mutex> lock(m_mtxDefaultAlarmer);
            stAlarmer = m_stDefaultAlarmer;
            bDefaultAlarmerReady = m_bDefaultAlarmerReady;
        }
        if (!bDefaultAlarmerReady)
        {
            dlog_warn("TVSDK默认告警设备信息缓存尚未准备完成，当前告警使用空设备信息");
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

    BOOL bRet = NET_serverPushAlarmInfo(
         pUseAlarmer,
        (INT32)lCommand,
        (LPVOID)pAlarmInfo,
        (INT32)dwBufLen);

    if ((lCommand & 0xF000) == NET_ALARM_BASE_STATISTICS)
    {
        dlog_info("[统计推送诊断] push_alarm NET_serverPushAlarmInfo 返回: cmd[0x%x] bRet[%d]", lCommand, bRet);
    }

    return bRet ? OK : -1;
}


int CTvSdkServer::get_client_count() const
{
    if (!m_bInit)
    {
        dlog_error("TVSDK server not inited, get client count failed");
        return -1;
    }

    return (int)NET_serverGetClientCount();
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
