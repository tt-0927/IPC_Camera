#include <iostream>
#include <algorithm>
#include <cstring>
#include <csignal>
#include <chrono>
#include <fstream>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include "NetTVSDKClientInterface.h"

#ifdef _WIN32
extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#endif

static std::string SanitizeName(const char* value) {
    std::string out = value ? value : "unknown";
    for (char& ch : out) {
        const bool keep = (ch >= '0' && ch <= '9') ||
                          (ch >= 'a' && ch <= 'z') ||
                          (ch >= 'A' && ch <= 'Z') ||
                          ch == '_' || ch == '-';
        if (!keep) {
            ch = '_';
        }
    }
    return out;
}

static std::string SaveAlarmImage(const char* deviceIp,
                                  const char* eventName,
                                  const char* imageKind,
                                  const BYTE* data,
                                  UINT32 len) {
    if (!data || len == 0) {
        return std::string();
    }

    try {
        const std::string dir = "alarm_dump";
#ifdef _WIN32
        _mkdir(dir.c_str());
#else
        mkdir(dir.c_str(), 0755);
#endif

        const auto now = std::chrono::system_clock::now();
        const auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        const std::string filename =
            SanitizeName(deviceIp) + "_" +
            SanitizeName(eventName) + "_" +
            SanitizeName(imageKind) + "_" +
            std::to_string(ts) + ".jpg";
        const std::string filePath = dir + "/" + filename;

        std::ofstream file(filePath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            return std::string();
        }

        file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
        if (!file.good()) {
            return std::string();
        }

        return filePath;
    } catch (...) {
        return std::string();
    }
}

static void SleepSeconds(unsigned int seconds) {
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
}

static const char* GetStatisticsTypeName(UINT32 type) {
    switch (type) {
        case NET_TV_STATISTICS_TYPE_PEOPLE_FLOW: return "PeopleFlow";
        case NET_TV_STATISTICS_TYPE_PEOPLE_DENSITY: return "PeopleDensity";
        default: return "Unknown";
    }
}

static const char* GetAlarmBaseName(INT64 command) {
    switch (command & 0xF000) {
        case NET_TV_ALARM_BASE_BASIC: return "BASIC";
        case NET_TV_ALARM_BASE_RULE: return "RULE";
        case NET_TV_ALARM_BASE_AI: return "AI";
        case NET_TV_ALARM_BASE_TRAFFIC: return "TRAFFIC";
        case NET_TV_ALARM_BASE_EXCEPTION: return "EXCEPTION";
        case NET_TV_ALARM_BASE_STATISTICS: return "STATISTICS";
        default: return "UNKNOWN_BASE";
    }
}

static const char* GetAlarmTypeName(UINT32 alarmType) {
    switch (alarmType) {
        case NET_TV_ALARM_MOTION_DETECT: return "MOTION_DETECT";
        case NET_TV_ALARM_OCCLUSION: return "OCCLUSION";
        case NET_TV_ALARM_LINE_CROSSING: return "LINE_CROSSING";
        case NET_TV_ALARM_INTRUSION: return "INTRUSION";
        case NET_TV_ALARM_ENTER_REGION: return "ENTER_REGION";
        case NET_TV_ALARM_LEAVE_REGION: return "LEAVE_REGION";
        case NET_TV_ALARM_FACE_DETECT: return "FACE_DETECT";
        case NET_TV_ALARM_FACE_CAPTURE: return "FACE_CAPTURE";
        case NET_TV_ALARM_FACE_COMPARE: return "FACE_COMPARE";
        case NET_TV_ALARM_UNATTENDED_OBJECT: return "UNATTENDED_OBJECT";
        case NET_TV_ALARM_OBJECT_REMOVAL: return "OBJECT_REMOVAL";
        case NET_TV_ALARM_CROWD_GATHERING: return "CROWD_GATHERING";
        case NET_TV_ALARM_SLEEP_ON_DUTY: return "SLEEP_ON_DUTY";
        case NET_TV_ALARM_HELMET_MISSING: return "HELMET_MISSING";
        case NET_TV_ALARM_PERSON_FALL: return "PERSON_FALL";
        case NET_TV_ALARM_PHONE_USAGE: return "PHONE_USAGE";
        case NET_TV_ALARM_SMOKING: return "SMOKING";
        case NET_TV_ALARM_SMOKE_FIRE: return "SMOKE_FIRE";
        case NET_TV_ALARM_NO_REFLECTIVE_VEST: return "NO_REFLECTIVE_VEST";
        case NET_TV_ALARM_PLATE_RECOGNITION: return "PLATE_RECOGNITION";
        case NET_TV_ALARM_TRAFFIC_CONGESTION: return "TRAFFIC_CONGESTION";
        case NET_TV_ALARM_VIDEO_LOSS: return "VIDEO_LOSS";
        case NET_TV_ALARM_PEOPLE_FLOW_STATISTICS: return "PEOPLE_FLOW_STATISTICS";
        case NET_TV_ALARM_PEOPLE_DENSITY_STATISTICS: return "PEOPLE_DENSITY_STATISTICS";
        default: return "UNKNOWN_ALARM_TYPE";
    }
}

static const char* GetAlarmCommandName(INT64 command) {
    switch (command) {
        case NET_TV_ALARM_BASE_BASIC: return "ALARM_BASE_BASIC";
        case NET_TV_ALARM_BASE_RULE: return "ALARM_BASE_RULE";
        case NET_TV_ALARM_BASE_AI: return "ALARM_BASE_AI";
        case NET_TV_ALARM_BASE_TRAFFIC: return "ALARM_BASE_TRAFFIC";
        case NET_TV_ALARM_BASE_EXCEPTION: return "ALARM_BASE_EXCEPTION";
        case NET_TV_ALARM_BASE_STATISTICS: return "ALARM_BASE_STATISTICS";
        case NET_TV_ALARM_FACE_DETECT: return "FACE_DETECT";
        case NET_TV_ALARM_FACE_CAPTURE: return "FACE_CAPTURE";
        case NET_TV_ALARM_FACE_COMPARE: return "FACE_COMPARE";
        case NET_TV_ALARM_PEOPLE_FLOW_STATISTICS: return "PEOPLE_FLOW_STATISTICS";
        case NET_TV_ALARM_PEOPLE_DENSITY_STATISTICS: return "PEOPLE_DENSITY_STATISTICS";
        default: return "UNKNOWN";
    }
}

// Global flag to control the main loop
volatile sig_atomic_t g_running = 1;

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\nReceived Ctrl+C. Stopping...\n");
        g_running = 0;
    }
}

// Alarm callback function
void STDCALL AlarmCallBack(OUT INT64 lCommand,
                           OUT NET_TV_ALARMER_S *pAlarmer,
                           OUT CHAR* pAlarmInfo,
                           OUT INT32* dwBufLen,
                           OUT LPVOID lpUserData) {
    printf("[AlarmCallBack] Received Alarm!\n");
    printf("  Command: 0x%llx (%s), Base=%s\n",
           (long long)lCommand,
           GetAlarmCommandName(lCommand),
           GetAlarmBaseName(lCommand));
    if (pAlarmer) {
        printf("  DeviceName: %s\n", pAlarmer->szDeviceName);
        printf("  DeviceIP: %s\n", pAlarmer->szDeviceIP);
    }

    INT64 alarmBase = lCommand & 0xF000;


    if (pAlarmInfo && dwBufLen && *dwBufLen > 0) {
        if (alarmBase == NET_TV_ALARM_BASE_BASIC &&
            *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_BASIC_INFO_S)) {
            auto* info = (NET_TV_ALARM_BASIC_INFO_S*)pAlarmInfo;
            printf("  [BASIC] AlarmType: 0x%x (%s), AlarmInput: %u\n",
                   info->dwAlarmType, GetAlarmTypeName(info->dwAlarmType), info->dwAlarmInputNumber);
        } else if (alarmBase == NET_TV_ALARM_BASE_RULE &&
                   *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_RULE_INFO_S)) {
            auto* info = (NET_TV_ALARM_RULE_INFO_S*)pAlarmInfo;
            printf("  [RULE] AlarmType: 0x%x (%s), Channel: %u, RuleID: %u, RuleType: %u, RuleName: %s, TargetID: %u\n",
                   info->dwAlarmType, GetAlarmTypeName(info->dwAlarmType), info->dwChannel, info->dwRuleID, info->dwRuleType, info->szRuleName, info->dwTargetID);
        } else if (lCommand == NET_TV_ALARM_FACE_COMPARE &&
                   *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_FACE_COMPARE_INFO_S)) {
            auto* info = (NET_TV_ALARM_FACE_COMPARE_INFO_S*)pAlarmInfo;
            printf("  [FACE_COMPARE] AlarmType: 0x%x (%s), Channel: %u, EventId: %d, Result: %d, FaceId: %d, Similarity: %d, FaceName: %s, FaceLibName: %s, CapFaceImgLen: %u, LibImgLen: %u\n",
                   info->dwAlarmType,
                   GetAlarmTypeName(info->dwAlarmType),
                   info->dwChannel,
                   info->nEventId,
                   info->nCompResult,
                   info->nFaceId,
                   info->nSimilarity,
                   info->szFaceName,
                   info->szFaceLibName,
                   info->dwCapFaceImgLen,
                   info->dwLibFaceImgLen);
            if (info->dwCapFaceImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->dwCapFaceImgLen, NET_TV_FACE_IMAGE_MAX_LEN);
                const std::string path = SaveAlarmImage(
                    pAlarmer ? pAlarmer->szDeviceIP : "unknown",
                    "face_compare",
                    "capture",
                    info->byCapFaceImg,
                    imgLen);
                if (!path.empty()) {
                    printf("  [FACE_COMPARE] CaptureFaceSaved: %s\n", path.c_str());
                }
            }
            if (info->dwLibFaceImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->dwLibFaceImgLen, NET_TV_FACE_IMAGE_MAX_LEN);
                const std::string path = SaveAlarmImage(
                    pAlarmer ? pAlarmer->szDeviceIP : "unknown",
                    "face_compare",
                    "library",
                    info->byLibFaceImg,
                    imgLen);
                if (!path.empty()) {
                    printf("  [FACE_COMPARE] LibFaceSaved: %s\n", path.c_str());
                }
            }
        } else if (alarmBase == NET_TV_ALARM_BASE_AI &&
                   *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_AI_OBJECT_INFO_S)) {
            auto* info = (NET_TV_ALARM_AI_OBJECT_INFO_S*)pAlarmInfo;
            printf("  [AI] AlarmType: 0x%x (%s), Channel: %u, ObjectType: %u, Confidence: %.3f, Rect: [%d,%d,%d,%d], ObjectID: %s, ImgLen: %u\n",
                   info->dwAlarmType, GetAlarmTypeName(info->dwAlarmType), info->dwChannel, info->dwObjectType, info->fConfidence,
                   info->nLeft, info->nTop, info->nRight, info->nBottom, info->szObjectID, info->dwImgLen);
            if (lCommand == NET_TV_ALARM_FACE_DETECT ||
                lCommand == NET_TV_ALARM_FACE_CAPTURE) {
                const char* imageKind = info->fConfidence == 1.0f ? "Panorama" : "Thumbnail";
                printf("  [AI] FaceImageKind: %s\n", imageKind);
                printf("  [AI] FaceDetails : dwAlarmType=0x%x channel=%u objectType=%u confidence=%.3f objectId=%s\n",
                       info->dwAlarmType, info->dwChannel, info->dwObjectType, info->fConfidence, info->szObjectID);
                if (info->dwImgLen > 0) {
                    const UINT32 imgLen = std::min<UINT32>(info->dwImgLen, NET_TV_PIC_DATA_MAX_LEN);
                    const std::string path = SaveAlarmImage(
                        pAlarmer ? pAlarmer->szDeviceIP : "unknown",
                        lCommand == NET_TV_ALARM_FACE_CAPTURE ? "face_capture" : "face_detect",
                        imageKind,
                        info->byImgData,
                        imgLen);
                    if (!path.empty()) {
                        printf("  [AI] ImageSaved : %s\n", path.c_str());
                    }
                }
            }
        } else if (alarmBase == NET_TV_ALARM_BASE_TRAFFIC &&
                   *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_PLATE_INFO_S)) {
            auto* info = (NET_TV_ALARM_PLATE_INFO_S*)pAlarmInfo;
            printf("  [TRAFFIC] AlarmType: 0x%x (%s), Channel: %u, Plate: %s, PlateColor: %u, VehicleType: %u, Speed: %u, Lane: %u, PlateImgLen: %u\n",
                   info->dwAlarmType, GetAlarmTypeName(info->dwAlarmType), info->dwChannel, info->szPlateNumber, info->dwPlateColor, info->dwVehicleType, info->dwSpeed, info->dwLaneNo, info->dwPlateImgLen);
            if (info->dwPlateImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->dwPlateImgLen, NET_TV_VEH_PLATE_IMAGE_LEN);
                const std::string path = SaveAlarmImage(
                    pAlarmer ? pAlarmer->szDeviceIP : "unknown",
                    info->dwAlarmType == NET_TV_ALARM_PLATE_RECOGNITION ? "plate_recognition" : "traffic_congestion",
                    "plate",
                    info->byPlateImg,
                    imgLen);
                if (!path.empty()) {
                    printf("  [TRAFFIC] PlateImageSaved: %s\n", path.c_str());
                }
            }
        } else if (alarmBase == NET_TV_ALARM_BASE_EXCEPTION &&
                   *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_EXCEPTION_INFO_S)) {
            auto* info = (NET_TV_ALARM_EXCEPTION_INFO_S*)pAlarmInfo;
            printf("  [EXCEPTION] AlarmType: 0x%x (%s), Channel: %u, DiskNo: %u, Status: %u\n",
                   info->dwAlarmType, GetAlarmTypeName(info->dwAlarmType), info->dwChannel, info->dwDiskNo, info->dwStatus);
        } else if (alarmBase == NET_TV_ALARM_BASE_STATISTICS &&
                   *dwBufLen >= (INT32)sizeof(NET_TV_ALARM_STATISTICS_INFO_S)) {
            auto* info = (NET_TV_ALARM_STATISTICS_INFO_S*)pAlarmInfo;
            printf("  [STATISTICS] AlarmType: 0x%x (%s), Channel: %u, StatisticsType: %u (%s), RuleID: %u, TimestampMs: %lld\n",
                   info->dwAlarmType, GetAlarmTypeName(info->dwAlarmType), info->dwChannel, info->dwStatisticsType,
                   GetStatisticsTypeName(info->dwStatisticsType), info->dwRuleID, (long long)info->llTimestampMs);
            printf("  [STATISTICS] Enter: %u, Leave: %u, Total: %u, "
                   "CurrentPeople: %u, AverageStayTimeSec: %u, TargetCount: "
                   "%u, PanoramaImgLen: %u\n",
                   info->dwEnterCount, info->dwLeaveCount, info->dwTotalCount,
                   info->dwCurrentPeopleCount, info->dwAverageStayTimeSec,
                   info->dwTargetCount, info->dwPanoramaImgLen);
            UINT32 targetCount = info->dwTargetCount;
            if (targetCount > NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM) {
                targetCount = NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM;
            }
            for (UINT32 i = 0; i < targetCount; ++i) {
                const auto& target = info->stTargets[i];
                printf("  [STATISTICS][Target %u] TrackID=%d RuleID=%u SnapshotType=%u Rect=[%d,%d,%d,%d] TimestampMs=%lld Direction=%d\n",
                       i,
                       target.nTrackID,
                       target.dwRuleID,
                       target.dwSnapshotType,
                       target.nLeft,
                       target.nTop,
                       target.nRight,
                       target.nBottom,
                       (long long)target.llTimestampMs,
                       target.nDirection);
            }
            if (info->dwPanoramaImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->dwPanoramaImgLen, NET_TV_PIC_DATA_MAX_LEN);
                const std::string path = SaveAlarmImage(
                    pAlarmer ? pAlarmer->szDeviceIP : "unknown",
                    info->dwStatisticsType == NET_TV_STATISTICS_TYPE_PEOPLE_DENSITY ? "people_density" : "people_flow",
                    "panorama",
                    info->byPanoramaImg,
                    imgLen);
                if (!path.empty()) {
                    printf("  [STATISTICS] PanoramaSaved: %s\n", path.c_str());
                }
            }
        } else {
            // 未知结构：按字符串尝试打印（一般是 JSON 兜底透传）
            printf("  Alarm Info (raw): %s\n", pAlarmInfo);
        }
    }
    printf("----------------------------------------\n");
}

void STDCALL OnChannelStatus(NET_TV_CHANNEL_INFO_S *pInfo, LPVOID pUserData) {
    (void)pUserData;
    if (!pInfo) {
        printf("[ChannelStatus] Received empty channel info.\n");
        printf("----------------------------------------\n");
        return;
    }

    printf("[ChannelStatus] Channel status changed.\n");
    printf("  Channel: %u, Enable: %d, Online: %d, RecordStatus: %d\n",
           pInfo->dwChannel,
           pInfo->byEnable,
           pInfo->byOnline,
           pInfo->nRecordStatus);
    printf("  ChannelName: %s\n", pInfo->szChannelName);
    printf("  DeviceName: %s\n", pInfo->szDevName);
    printf("  DeviceIP: %s\n", pInfo->szDeviceIP);
    printf("  SerialNum: %s\n", pInfo->szSerialNum);
    printf("----------------------------------------\n");
}

int main() {
    // Register signal handler
    signal(SIGINT, signal_handler);

    printf("Starting SDK Client Alarm Demo...\n");

    // Initialize SDK
    if (!NET_TV_Init()) {
        printf("NET_TV_Init failed!\n");
        return -1;
    }
    printf("NET_TV_Init success.\n");

    // Login Information (Hardcoded as per existing demo)
    NET_TV_DEVICE_LOGIN_INFO_S struLoginInfo = {0};
    NET_TV_DEVICE_INFO_S struDeviceInfo = {0};

    struLoginInfo.dwPort = 9019;
    strcpy(struLoginInfo.szIPAddr, "172.16.25.199");
    strcpy(struLoginInfo.szUserName, "admin");
    strcpy(struLoginInfo.szPassword, "sj2@2025");

    printf("Logging in to %s:%d...\n", struLoginInfo.szIPAddr, struLoginInfo.dwPort);
    LPVOID lpUserID = NET_TV_Login(&struLoginInfo, &struDeviceInfo);
    if (lpUserID == NULL) { // Assuming NULL indicates failure, usually API returns handle or ID > 0
        // Note: The API interface says "返回值为用户ID", verify if 0 or NULL checks are appropriate.
        // Usually pointers are checked against NULL.
        printf("NET_TV_Login failed!\n");
        NET_TV_Cleanup();
        return -1;
    }
    printf("Login success. UserID: %p\n", lpUserID);

    // Set Alarm Callback
    if (!NET_TV_SetAlarmCallBack(lpUserID, AlarmCallBack, NULL)) {
        printf("NET_TV_SetAlarmCallBack failed!\n");
        NET_TV_Logout(lpUserID);
        NET_TV_Cleanup();
        return -1;
    }
    printf("SetAlarmCallBack success.\n");

    if (!NET_TV_SetChannelStatusCallBack(lpUserID, OnChannelStatus, NULL)) {
        printf("NET_TV_SetChannelStatusCallBack failed!\n");
        NET_TV_Logout(lpUserID);
        NET_TV_Cleanup();
        return -1;
    }
    printf("SetChannelStatusCallBack success.\n");

    // Start Listening
    if (!NET_TV_StartListen(lpUserID)) {
        printf("NET_TV_StartListen failed!\n");
        NET_TV_Logout(lpUserID);
        NET_TV_Cleanup();
        return -1;
    }
    printf("StartListen success. Waiting for alarms... (Press Ctrl+C to stop)\n");

    // Main loop
    while (g_running) {
        SleepSeconds(1);
    }

    printf("Stopping listen...\n");
    NET_TV_StopListen(lpUserID);

    printf("Logging out...\n");
    NET_TV_Logout(lpUserID);

    printf("Cleaning up SDK...\n");
    NET_TV_Cleanup();

    printf("Exiting.\n");
    return 0;
}
