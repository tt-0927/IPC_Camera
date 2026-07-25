#include <iostream>
#include <algorithm>
#include <cstring>
#include <csignal>
#include <chrono>
#include <fstream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include "NetTVSDKClientInterface.h"
#include <map>
#include <sstream>

#ifdef _WIN32
extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#endif


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
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);
        const auto tm_now = std::localtime(&time_t_now);

        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", tm_now);

        const std::string filename =
            std::string(deviceIp ? deviceIp : "unknown") + "_" +
            std::string(eventName ? eventName : "unknown") + "_" +
            std::string(imageKind ? imageKind : "unknown") + "_" +
            timeBuf + ".jpg";
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

static const char* GetAlarmTypeName(UINT32 alarmType) 
{
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
        case NET_TV_ALARM_LOITERING: return "LOITERING";
        case NET_TV_ALARM_PARKING_DETECT: return "PARKING_DETECT";
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
        case NET_TV_ALARM_LOITERING: return "LOITERING";
        case NET_TV_ALARM_PARKING_DETECT: return "PARKING_DETECT";
        case NET_TV_ALARM_PEOPLE_FLOW_STATISTICS: return "PEOPLE_FLOW_STATISTICS";
        case NET_TV_ALARM_PEOPLE_DENSITY_STATISTICS: return "PEOPLE_DENSITY_STATISTICS";
        default: return "UNKNOWN";
    }
}

// ==================== 异步图片保存 ====================
struct ImageSaveTask {
    std::string deviceIp;
    std::string eventName;
    std::string imageKind;
    std::vector<BYTE> imageData;
};

std::queue<ImageSaveTask> g_saveQueue;
std::mutex g_queueMutex;
std::condition_variable g_queueCV;
bool g_saveThreadRunning = true;

void ImageSaveThread() {
    while (g_saveThreadRunning) {
        std::unique_lock<std::mutex> lock(g_queueMutex);
        g_queueCV.wait(lock, []{ return !g_saveQueue.empty() || !g_saveThreadRunning; });

        if (!g_saveThreadRunning && g_saveQueue.empty()) break;

        while (!g_saveQueue.empty()) {
            ImageSaveTask task = std::move(g_saveQueue.front());
            g_saveQueue.pop();
            lock.unlock();

            const std::string path = SaveAlarmImage(task.deviceIp.c_str(),
                                                    task.eventName.c_str(),
                                                    task.imageKind.c_str(),
                                                    task.imageData.data(),
                                                    (UINT32)task.imageData.size());
            if (!path.empty()) {
                printf("[异步保存/AsyncSave] 图片已保存 / Image saved: %s\n", path.c_str());
            }

            lock.lock();
        }
    }
}

inline void SubmitImageSave(const char* ip, const char* event, const char* kind,
                            const BYTE* data, UINT32 len) {
    if (!data || len == 0) return;
    std::lock_guard<std::mutex> lock(g_queueMutex);
    g_saveQueue.push({ip ? ip : "unknown", event ? event : "unknown",
                      kind ? kind : "unknown", std::vector<BYTE>(data, data + len)});
    g_queueCV.notify_one();
}
// ==================== 异步图片保存结束 ====================

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
                           OUT NET_Alarmer_S *pAlarmer,
                           OUT CHAR* pAlarmInfo,
                           OUT INT32* dwBufLen,
                           OUT LPVOID lpUserData) {

    // 立即刷新stdout，确保日志及时输出
    fflush(stdout);

    // 第一时间打印标记（无任何格式化开销）
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    printf("[%lld] ALARM_CB_ENTRY cmd=0x%llx\n", (long long)now_ms, (long long)lCommand);
    fflush(stdout);

    // 打印当前时间戳
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::time_t t = ms / 1000;
        std::tm* tm_info = std::localtime(&t);
        char timebuf[32];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);
        printf("[%s.%03lld] [AlarmCallBack] 收到告警 / Received Alarm!\n", timebuf, (long long)(ms % 1000));
    }
    printf("  命令 / Command: 0x%llx (%s), Base=%s\n",
           (long long)lCommand,
           GetAlarmCommandName(lCommand),
           GetAlarmBaseName(lCommand));
    if (pAlarmer) {
        printf("  设备名 / DeviceName: %s\n", pAlarmer->szDeviceName);
        printf("  设备IP / DeviceIP: %s\n", pAlarmer->szDeviceIP);
    }

    INT64 alarmBase = lCommand & 0xF000;


    if (pAlarmInfo && dwBufLen && *dwBufLen > 0) {
        if (alarmBase == NET_TV_ALARM_BASE_BASIC &&
            *dwBufLen >= (INT32)sizeof(NET_AlarmBasicInfo_S)) {
            auto* info = (NET_AlarmBasicInfo_S*)pAlarmInfo;
            printf("  [BASIC/基本告警] AlarmType: 0x%x (%s), 告警输入 / AlarmInput: %u, 时间戳 / TimestampMs: %lld\n",
                   info->uAlarmType,
                   GetAlarmTypeName(info->uAlarmType),
                   info->uAlarmInputNumber,
                   (long long)info->llTimestampMs);
        } else if (alarmBase == NET_TV_ALARM_BASE_RULE &&
                   *dwBufLen >= (INT32)sizeof(NET_AlarmRuleInfo_S)) {
            auto* info = (NET_AlarmRuleInfo_S*)pAlarmInfo;
            printf("  [RULE/规则告警] AlarmType: 0x%x (%s), 通道 / Channel: %u, 规则ID / RuleID: %u, 规则类型 / RuleType: %u, 规则名 / RuleName: %s, 目标ID / TargetID: %u, 对象类型 / ObjectType: %u, 时间戳 / TimestampMs: %lld, 置信度 / Confidence: %.3f, 区域 / Rect: [%d,%d,%d,%d], 全景图 / PanoramaLen: %u, 特写图 / TargetLen: %u\n",
                   info->uAlarmType, GetAlarmTypeName(info->uAlarmType), info->uChannel, info->uRuleID, info->uRuleType,
                   info->strRuleName, info->uTargetID, info->uObjectType, (long long)info->llTimestampMs, info->fConfidence,
                   info->nLeft, info->nTop, info->nRight, info->nBottom,
                   info->uPanoramaImgLen, info->uTargetImgLen);
            if (info->uPanoramaImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uPanoramaImgLen, NET_TV_PIC_DATA_MAX_LEN);
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               "rule", "panorama",
                               info->byPanoramaImg, imgLen);
            }
            if (info->uTargetImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uTargetImgLen, NET_TV_PIC_DATA_MAX_LEN);
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               "rule", "target",
                               info->byTargetImg, imgLen);
            }
        } else if (lCommand == NET_TV_ALARM_FACE_COMPARE &&
                   *dwBufLen >= (INT32)sizeof(NET_AlarmFaceCompareInfo_S)) {
            auto* info = (NET_AlarmFaceCompareInfo_S*)pAlarmInfo;
            printf("  [FACE_COMPARE/人脸比对] AlarmType: 0x%x (%s), 通道 / Channel: %u, 事件ID / EventId: %d, 比对结果 / Result: %d, 相似度 / Similarity: %d, 人脸ID / FaceId: %d, 时间戳 / TimestampMs: %lld\n",
                   info->uAlarmType,
                   GetAlarmTypeName(info->uAlarmType),
                   info->uChannel,
                   info->nEventId,
                   info->nCompResult,
                   info->nSimilarity,
                   info->nFaceId,
                   (long long)info->llTimestampMs);
            printf("  [FACE_COMPARE/人脸比对] 库名称 / LibName: %s, 人脸名称 / FaceName: %s\n",
                   info->strFaceLibName,
                   info->strFaceName);
            printf("  [FACE_COMPARE/人脸比对] 库脸路径 / LibFacePath: %s, 抓拍路径 / CapFacePath: %s, 抓拍图片路径 / CapImagePath: %s\n",
                   info->strLibFacePath,
                   info->strCapFacePath,
                   info->strCapImagePath);
            if (info->uLibFaceImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uLibFaceImgLen, NET_TV_FACE_IMAGE_MAX_LEN);
                printf("  [FACE_COMPARE/人脸比对] 库脸图片长度 / LibFaceImgLen: %u (异步保存 / saving async)\n", imgLen);
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               "face_compare", "lib_face",
                               info->byLibFaceImg, imgLen);
            }
            if (info->uCapFaceImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uCapFaceImgLen, NET_TV_FACE_IMAGE_MAX_LEN);
                printf("  [FACE_COMPARE/人脸比对] 抓拍图片长度 / CapFaceImgLen: %u (异步保存 / saving async)\n", imgLen);
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               "face_compare", "capture_face",
                               info->byCapFaceImg, imgLen);
            }
        } else if (alarmBase == NET_TV_ALARM_BASE_AI &&
                   *dwBufLen >= (INT32)sizeof(NET_AlarmAiObjectInfo_S)) {
            auto* info = (NET_AlarmAiObjectInfo_S*)pAlarmInfo;
            printf("  [AI/智能分析] AlarmType: 0x%x (%s), 通道 / Channel: %u, 对象类型 / ObjectType: %u, 时间戳 / TimestampMs: %lld, 置信度 / Confidence: %.3f, 区域 / Rect: [%d,%d,%d,%d], 对象ID / ObjectID: %s, 图片长度 / ImgLen: %u\n",
                   info->uAlarmType, GetAlarmTypeName(info->uAlarmType), info->uChannel, info->uObjectType, (long long)info->llTimestampMs, info->fConfidence,
                   info->nLeft, info->nTop, info->nRight, info->nBottom, info->strObjectID, info->uImgLen);

            if (info->uPanoramaImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uPanoramaImgLen, NET_TV_PIC_DATA_MAX_LEN);
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               "ai_object", "panorama",
                               info->byPanoramaImg, imgLen);
            }

            if (info->uImgLen > 0) {
                bool isJpeg = (info->byImgData[0] == 0xFF && info->byImgData[1] == 0xD8);
                printf("  [AI/智能分析] 图片信息 / ImageInfo: 格式 / Format=%s, 大小 / Size=%u bytes (异步保存 / saving async)\n",
                       isJpeg ? "JPEG" : "Unknown",
                       info->uImgLen);

                const UINT32 imgLen = std::min<UINT32>(info->uImgLen, NET_TV_PIC_DATA_MAX_LEN);
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               "ai_object", "alarm_image",
                               info->byImgData, imgLen);
            }

            if (lCommand == NET_TV_ALARM_FACE_DETECT ||
                lCommand == NET_TV_ALARM_FACE_CAPTURE ||
                lCommand == NET_TV_ALARM_FACE_COMPARE) {
                const bool isFaceCompare = lCommand == NET_TV_ALARM_FACE_COMPARE;
                const char* imageKind = isFaceCompare ? "CompareTarget/比对目标" :
                    (info->fConfidence == 1.0f ? "Panorama/全景" : "Thumbnail/缩略图");
                printf("  [AI/智能分析] 人脸图片类型 / FaceImageKind: %s\n", imageKind);
                printf("  [AI/智能分析] 人脸详情 / FaceDetails: 告警类型 / AlarmType=0x%x, 通道 / channel=%u, 对象类型 / objectType=%u, 置信度 / confidence=%.3f, 对象ID / objectId=%s\n",
                       info->uAlarmType, info->uChannel, info->uObjectType, info->fConfidence, info->strObjectID);
                if (isFaceCompare) {
                    printf("  [AI/智能分析] 人脸比对相似度 / FaceCompareSimilarity: %.3f\n", info->fConfidence);
                }
                if (info->uImgLen > 0) {
                    const UINT32 imgLen = std::min<UINT32>(info->uImgLen, NET_TV_PIC_DATA_MAX_LEN);
                    const char* eventName = isFaceCompare ? "face_compare" :
                        (lCommand == NET_TV_ALARM_FACE_CAPTURE ? "face_capture" : "face_detect");
                    SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                                   eventName, imageKind,
                                   info->byImgData, imgLen);
                }
            }
        } else if (alarmBase == NET_TV_ALARM_BASE_TRAFFIC &&
                   *dwBufLen >= (INT32)sizeof(NET_AlarmPlateInfo_S)) {
            auto* info = (NET_AlarmPlateInfo_S*)pAlarmInfo;
            printf("  [TRAFFIC/交通管理] AlarmType: 0x%x (%s), 通道 / Channel: %u, 车牌 / Plate: %s, 车牌颜色 / PlateColor: %u, 车辆类型 / VehicleType: %u, 速度 / Speed: %u, 车道 / Lane: %u, 车牌图片长度 / PlateImgLen: %u\n",
                   info->uAlarmType, GetAlarmTypeName(info->uAlarmType), info->uChannel, info->strPlateNumber, info->uPlateColor, info->uVehicleType, info->uSpeed, info->uLaneNo, info->uPlateImgLen);
            if (info->uPlateImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uPlateImgLen, NET_TV_VEH_PLATE_IMAGE_LEN);
                printf("  [TRAFFIC/交通管理] 车牌图片长度 / PlateImgLen: %u (异步保存 / saving async)\n", imgLen);
                const char* eventName = info->uAlarmType == NET_TV_ALARM_PLATE_RECOGNITION ? "plate_recognition" : "traffic_congestion";
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               eventName, "plate",
                               info->byPlateImg, imgLen);
            }
        } else if (alarmBase == NET_TV_ALARM_BASE_EXCEPTION &&
                   *dwBufLen >= (INT32)sizeof(NET_AlarmExceptionInfo_S)) {
            auto* info = (NET_AlarmExceptionInfo_S*)pAlarmInfo;
            printf("  [EXCEPTION/异常告警] AlarmType: 0x%x (%s), 通道 / Channel: %u, 硬盘号 / DiskNo: %u, 状态 / Status: %u\n",
                   info->uAlarmType, GetAlarmTypeName(info->uAlarmType), info->uChannel, info->uDiskNo, info->uStatus);
        } else if (alarmBase == NET_TV_ALARM_BASE_STATISTICS &&
                   *dwBufLen >= (INT32)sizeof(NET_AlarmStatisticsInfo_S)) {
            auto* info = (NET_AlarmStatisticsInfo_S*)pAlarmInfo;
            printf("  [STATISTICS/统计告警] AlarmType: 0x%x (%s), 通道 / Channel: %u, 统计类型 / StatisticsType: %u (%s), 规则ID / RuleID: %u, 时间戳 / TimestampMs: %lld\n",
                   info->uAlarmType, GetAlarmTypeName(info->uAlarmType), info->uChannel, info->uStatisticsType, info->uRuleID,
                   GetStatisticsTypeName(info->uStatisticsType), (long long)info->llTimestampMs);
            printf("  [STATISTICS/统计告警] 进入 / Enter: %u, 离开 / Leave: %u, 总计 / Total: %u, "
                   "当前人数 / CurrentPeople: %u, 平均停留时间 / AverageStayTimeSec: %u, 目标数量 / TargetCount: "
                   "%u, 全景图片长度 / PanoramaImgLen: %u\n",
                   info->uEnterCount, info->uLeaveCount, info->uTotalCount,
                   info->uCurrentPeopleCount, info->uAverageStayTimeSec,
                   info->uTargetCount, info->uPanoramaImgLen);
            UINT32 targetCount = info->uTargetCount;
            if (targetCount > NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM) {
                targetCount = NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM;
            }
            // ========== 特写图片信息 / Close-up Image Info ==========
            printf("  [STATISTICS/统计告警][特写图片/Close-up] 目标数量 / TargetCount: %u\n", targetCount);
            for (UINT32 i = 0; i < targetCount; ++i) {
                const auto& target = info->stTargets[i];
                printf("  [STATISTICS/统计告警][特写图片/Close-up][目标%u] 跟踪ID / TrackID: %d, 规则ID / RuleID: %u, 抓拍类型 / SnapshotType: %u, 区域 / Rect: [%d,%d,%d,%d], 时间戳 / TimestampMs: %lld, 方向 / Direction: %d, 图片长度 / ImgLen: %u\n",
                       i,
                       target.nTrackID,
                       target.uRuleID,
                       target.uSnapshotType,
                       target.nLeft,
                       target.nTop,
                       target.nRight,
                       target.nBottom,
                       (long long)target.llTimestampMs,
                       target.nDirection,
                       target.uImgLen
                    );
                if (target.uImgLen > 0) {
                    const UINT32 imgLen = std::min<UINT32>(target.uImgLen, NET_TV_PIC_DATA_MAX_LEN);
                    printf("  [STATISTICS/统计告警][特写图片/Close-up][目标%u] ✅ 有图片数据 / Image available, 长度 / Length: %u bytes (异步保存 / saving async)\n", i, imgLen);
                    const std::string kind = "target_" + std::to_string(i);
                    const char* eventName = info->uStatisticsType == NET_TV_STATISTICS_TYPE_PEOPLE_DENSITY ? "density" : "flow";
                    SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                                   eventName, kind.c_str(),
                                   target.byImgData, imgLen);
                } else {
                    printf("  [STATISTICS/统计告警][特写图片/Close-up][目标%u] ❌ 无图片数据 / No image\n", i);
                }
            }
            // ========== 全景图片信息 / Panorama Image Info ==========
            printf("  [STATISTICS/统计告警][全景图片/Panorama] ");
            if (info->uPanoramaImgLen > 0) {
                const UINT32 imgLen = std::min<UINT32>(info->uPanoramaImgLen, NET_TV_PIC_DATA_MAX_LEN);
                printf("✅ 有图片数据 / Image available, 长度 / Length: %u bytes (异步保存 / saving async)\n", imgLen);
                const char* eventName = info->uStatisticsType == NET_TV_STATISTICS_TYPE_PEOPLE_DENSITY ? "density" : "flow";
                SubmitImageSave(pAlarmer ? pAlarmer->szDeviceIP : nullptr,
                               eventName, "panorama",
                               info->byPanoramaImg, imgLen);
            } else {
                printf("❌ 无图片数据 / No image\n");
            }
        } else {
            // 未知结构：按字符串尝试打印（一般是 JSON 兜底透传）
            printf("  [UNKNOWN/未知类型] Alarm Info (raw): %s\n", pAlarmInfo);
        }
    }
    printf("----------------------------------------\n");
}

void STDCALL OnChannelStatus(NET_TV_CHANNEL_INFO_S *pInfo, LPVOID pUserData)
 {
    (void)pUserData;
    if (!pInfo) {
        printf("[通道状态/Channel Status] 收到空的通道信息 / Received empty channel info\n");
        printf("----------------------------------------\n");
        return;
    }

    struct ChnPrev { int online; int devState; int recordStatus; };
    static std::map<UINT32, ChnPrev> s_prev;
    static bool s_first = true;

    UINT32 ch = pInfo->dwChannel;
    auto it = s_prev.find(ch);
    auto onlineStr = [](int v) { return v ? "在线/Online" : "离线/Offline"; };

    if (s_first) {
        printf("[通道状态/Channel Status - 首次/Initial] 通道 / Channel:%u  在线 / Online=%d(%s)  设备状态 / DevState=%d  录像状态 / RecState=%d\n",
               ch, pInfo->byOnline, onlineStr(pInfo->byOnline),
               pInfo->nDevState, pInfo->nRecordStatus);
        printf("  通道名 / ChnName: %s  设备名 / DevName: %s  IP: %s  序列号 / Serial: %s\n",
               pInfo->szChannelName, pInfo->szDevName,
               pInfo->szDeviceIP, pInfo->szSerialNum);
        s_first = false;
    } else if (it == s_prev.end() ||
               it->second.online != pInfo->byOnline ||
               it->second.devState != pInfo->nDevState ||
               it->second.recordStatus != pInfo->nRecordStatus) {

        std::ostringstream oss;
        oss << "[通道状态/Channel Status - 变更/Changed] 通道 / Channel:" << ch << "\n";
        if (it != s_prev.end()) {
            oss << "  在线状态 / Online:  " << it->second.online << "(" << onlineStr(it->second.online)
                << ")  -->  " << (int)pInfo->byOnline << "(" << onlineStr(pInfo->byOnline) << ")\n";
            oss << "  设备状态 / DevState:  " << it->second.devState
                << "  -->  " << (int)pInfo->nDevState << "\n";
            oss << "  录像状态 / RecState:  " << it->second.recordStatus
                << "  -->  " << (int)pInfo->nRecordStatus << "\n";
        } else {
            oss << "  在线状态 / Online:  ???  -->  " << (int)pInfo->byOnline << "(" << onlineStr(pInfo->byOnline) << ")\n";
            oss << "  设备状态 / DevState:  ???  -->  " << (int)pInfo->nDevState << "\n";
            oss << "  录像状态 / RecState:  ???  -->  " << (int)pInfo->nRecordStatus << "\n";
        }
        oss << "  通道名 / ChnName: " << pInfo->szChannelName
            << "  设备名 / DevName: " << pInfo->szDevName
            << "  IP: " << pInfo->szDeviceIP
            << "  序列号 / Serial: " << pInfo->szSerialNum;
        printf("%s\n", oss.str().c_str());
   } else {
        printf("[通道状态/Channel Status] 通道 / Channel:%u  在线 / Online=%d(%s)  设备状态 / DevState=%d  录像状态 / RecState=%d\n",
               ch, pInfo->byOnline, onlineStr(pInfo->byOnline),
               pInfo->nDevState, pInfo->nRecordStatus);
        printf("  通道名 / ChnName: %s  设备名 / DevName: %s  IP: %s  序列号 / Serial: %s\n",
               pInfo->szChannelName, pInfo->szDevName,
               pInfo->szDeviceIP, pInfo->szSerialNum);
    }

    s_prev[ch] = {pInfo->byOnline, pInfo->nDevState, pInfo->nRecordStatus};
    printf("----------------------------------------\n");
}

int main() {
    // Register signal handler
    signal(SIGINT, signal_handler);

    printf("Starting SDK Client Alarm Demo...\n");

    // 启动图片异步保存线程
    std::thread saveThread(ImageSaveThread);

    // Initialize SDK
    if (!NET_TV_Init()) {
        printf("NET_TV_Init failed!\n");
        g_saveThreadRunning = false;
        g_queueCV.notify_one();
        if (saveThread.joinable()) saveThread.join();
        return -1;
    }
    printf("NET_TV_Init success.\n");

        // Set log to file
#ifdef _WIN32
    const char* logDir = "root/log";
    _mkdir("root");
    _mkdir(logDir);
#else
    const char* logDir = "/root/log";
    mkdir("root", 0755);
    mkdir(logDir, 0755);
#endif
    if (!NET_TV_SetLogToFile(0, (char*)logDir, 5 * 1024 * 1024, 10)) {
        printf("NET_TV_SetLogToFile failed!\n");
    } else {
        printf("NET_TV_SetLogToFile success, log directory: %s\n", logDir);
    }

    // Login Information (Hardcoded as per existing demo)
    NET_TV_DEVICE_LOGIN_INFO_S struLoginInfo = {0};
    NET_DeviceInfo_S struDeviceInfo = {0};

    struLoginInfo.dwPort = 9019;
    strcpy(struLoginInfo.szIPAddr, "172.16.25.191");
    strcpy(struLoginInfo.szUserName, "admin");
    strcpy(struLoginInfo.szPassword, "itc20232024");

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

    // 停止图片保存线程
    printf("Stopping image save thread...\n");
    g_saveThreadRunning = false;
    g_queueCV.notify_one();
    if (saveThread.joinable()) {
        saveThread.join();
    }

    printf("Exiting.\n");
    return 0;
}
