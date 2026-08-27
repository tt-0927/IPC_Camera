/**
 * @FilePath     : event_linkage_dict.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 18:12:04
 * @Description  : 事件联动字典与协议映射基础实现
 */

#include "event_linkage_dict.h"

#include "time_utils.h"

#ifdef ENABLE_TVSDK_SRC
#include "control_manage.h"
#include "NetTVSDKServer.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <limits>
#include <new>
#include <vector>
#endif

#ifdef ENABLE_TVSDK_SRC
namespace
{
/* TVSDK告警推送耗时告警阈值(ms)，超过说明推送链路阻塞了事件联动线程 */
constexpr long long TVSDK_PUSH_WARN_MS = 50;

/**
 * @brief   : 从上下文属性中读取整数值
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @param    {std::string} &strKey 属性键
 * @param    {int} nDefault 默认值
 * @return   {int} 属性整数值
 */
int get_context_attr_int(const EventTriggerContext_S &stContext, const std::string &strKey, int nDefault)
{
    const auto it = stContext.mapAttrs.find(strKey);
    if (it == stContext.mapAttrs.end())
    {
        return nDefault;
    }

    return std::atoi(it->second.c_str());
}

long long get_context_timestamp_ms(const EventTriggerContext_S &stContext)
{
    if (stContext.llTimestamp > 0)
    {
        return stContext.llTimestamp;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/**
 * @brief   : 将 IPC JPEG 数据复制到新 SDK 的固定容量图片字段
 * @details : 新 SDK 不再使用 V2 指针图片结构，调用方必须保证长度字段与实际复制字节一致。
 */
template <size_t N>
UINT32 copy_tvsdk_image(BYTE (&byDst)[N], const std::vector<unsigned char> &vecJpeg)
{
    if (vecJpeg.empty())
    {
        return 0;
    }

    const size_t nCopyLen = std::min(vecJpeg.size(), N);
    if (nCopyLen != vecJpeg.size())
    {
        dlog_warn("TVSDK告警图片超过新协议字段上限: source[%zu], limit[%zu]", vecJpeg.size(), N);
    }

    std::memcpy(byDst, vecJpeg.data(), nCopyLen);
    return static_cast<UINT32>(nCopyLen);
}

template <size_t N>
UINT32 copy_tvsdk_image(BYTE (&byDst)[N], const EventTvSdkImage_S &stImage)
{
    return copy_tvsdk_image(byDst, stImage.vecJpeg);
}

/**
 * @brief 将内存中的 JPEG 图片复制到 V1 固定长度图片字段
 * @details V1 接口不接收指针，只接收结构体内的图片数组；图片超过协议上限时拒绝发送，避免产生损坏 JPEG。
 */
bool fill_v1_panorama_image(const EventTvSdkImage_S &stImage,
                            BYTE *pDst,
                            size_t nCapacity,
                            UINT32 &nImageLen)
{
    nImageLen = 0;
    if (stImage.vecJpeg.empty())
    {
        return true;
    }

    if (!pDst || stImage.vecJpeg.size() > nCapacity ||
        stImage.vecJpeg.size() > static_cast<size_t>(std::numeric_limits<UINT32>::max()))
    {
        dlog_warn("V1告警全景图超出协议容量: size[%zu], capacity[%zu]",
                  stImage.vecJpeg.size(), nCapacity);
        return false;
    }

    std::memcpy(pDst, stImage.vecJpeg.data(), stImage.vecJpeg.size());
    nImageLen = static_cast<UINT32>(stImage.vecJpeg.size());
    return true;
}

template <size_t N>
void copy_tvsdk_string(CHAR (&szDst)[N], const std::string &strSrc)
{
    if (N == 0)
    {
        return;
    }

    strncpy(szDst, strSrc.c_str(), N - 1);
    szDst[N - 1] = '\0';
}

/**
 * @brief   : 读取本地 JPEG 文件到动态图片容器
 * @details : 人脸比对图片没有事件上下文二进制缓存，读取后由调用处复制到新 SDK 固定图片字段。
 */
void load_tvsdk_image_file(const std::string &strPath, std::vector<unsigned char> &vecImage)
{
    vecImage.clear();
    if (strPath.empty())
    {
        return;
    }

    std::ifstream file(strPath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        dlog_warn("TVSDK人脸比对图片打开失败: %s", strPath.c_str());
        return;
    }

    const std::streamoff nSize = file.tellg();
    if (nSize <= 0)
    {
        return;
    }
    try
    {
        vecImage.resize(static_cast<size_t>(nSize));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char *>(vecImage.data()), nSize);
        if (!file)
        {
            vecImage.clear();
            dlog_warn("TVSDK人脸比对图片读取失败: %s", strPath.c_str());
            return;
        }
    }
    catch (const std::bad_alloc &e)
    {
        vecImage.clear();
        dlog_warn("TVSDK人脸比对图片内存分配失败: %s, reason[%s]", strPath.c_str(), e.what());
        return;
    }

}

/**
 * @brief   : 获取普通事件对应的 TVSDK 告警命令
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {UINT32} TVSDK 告警命令，0 表示不支持
 */
UINT32 get_basic_alarm_type(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::MOTION_DETECT:    return NET_ALARM_MOTION_DETECT;
    case Event::Type_E::OCCLUSION_DETECT: return NET_ALARM_OCCLUSION;
    case Event::Type_E::ANOMALY_ALARM:    return NET_ALARM_ANOMALY;
    case Event::Type_E::ALARM_INPUT:      return NET_ALARM_INPUT;
    case Event::Type_E::PIR_ALARM:        return NET_ALARM_PIR;
    default:                              return 0;
    }
}

/**
 * @brief   : 获取周界事件对应的 TVSDK 告警命令
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {UINT32} TVSDK 告警命令，0 表示不支持
 */
UINT32 get_rule_alarm_type(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::LINE_CROSSING:     return NET_ALARM_LINE_CROSSING;
    case Event::Type_E::INTRUSION:         return NET_ALARM_INTRUSION;
    case Event::Type_E::ENTER_REGION:      return NET_ALARM_ENTER_REGION;
    case Event::Type_E::LEAVE_REGION:      return NET_ALARM_LEAVE_REGION;
    case Event::Type_E::OBJECT_REMOVAL:    return NET_ALARM_OBJECT_REMOVAL;
    case Event::Type_E::UNATTENDED_OBJECT: return NET_ALARM_UNATTENDED_OBJECT;
    default:                               return 0;
    }
}

/**
 * @brief   : 获取异常事件对应的 TVSDK 告警命令
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {UINT32} TVSDK 告警命令，0 表示不支持
 */
UINT32 get_exception_alarm_type(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::DISK_FULL:         return NET_ALARM_DISK_FULL;
    case Event::Type_E::DISK_ERROR:        return NET_ALARM_DISK_ERROR;
    case Event::Type_E::NET_BROKEN:        return NET_ALARM_NET_BROKEN;
    case Event::Type_E::IP_CONFLICT:       return NET_ALARM_IP_CONFLICT;
    case Event::Type_E::ILLEGAL_ACCESS:    return NET_ALARM_ANOMALY;
    default:                               return 0;
    }
}

/**
 * @brief   : 获取 AI 目标事件对应的 TVSDK 告警命令
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {UINT32} TVSDK 告警命令，0 表示不支持
 */
UINT32 get_ai_object_alarm_type(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::LOITERING_DETECT:             return NET_ALARM_LOITERING;
    case Event::Type_E::PARKING_DETECT:               return NET_ALARM_PARKING_DETECT;
    case Event::Type_E::FACE_DETECT:                  return NET_ALARM_FACE_DETECT;
    case Event::Type_E::FACE_CAPTURE:                 return NET_ALARM_FACE_CAPTURE;
    case Event::Type_E::CROWD_GATHERING:              return NET_ALARM_CROWD_GATHERING;
    case Event::Type_E::SLEEP_ON_DUTY:                return NET_ALARM_SLEEP_ON_DUTY;
    case Event::Type_E::LEAVE_POST:                   return NET_ALARM_LEAVE_POST;
    case Event::Type_E::PERSON_FALL_DOWN:             return NET_ALARM_PERSON_FALL;
    case Event::Type_E::FENCE_CLIMBING:               return NET_ALARM_FENCE_CLIMBING;
    case Event::Type_E::SMOKING:                      return NET_ALARM_SMOKING;
    case Event::Type_E::PHONE_USAGE:                  return NET_ALARM_PHONE_USAGE;
    case Event::Type_E::SMOKE_FIRE:                   return NET_ALARM_SMOKE_FIRE;
    case Event::Type_E::OPEN_FLAME:                   return NET_ALARM_OPEN_FLAME;
    case Event::Type_E::MANHOLE_COVER_ABNORMAL:       return NET_ALARM_MANHOLE_COVER_ABNORMAL;
    case Event::Type_E::BARE_SOIL:                    return NET_ALARM_BARE_SOIL;
    case Event::Type_E::HOLE_PROTECTION_BAR:          return NET_ALARM_HOLE_PROTECTION_BAR;
    case Event::Type_E::PEDESTRIAN_INTRUSION:         return NET_ALARM_PEDESTRIAN_INTRUSION;
    case Event::Type_E::PERSON_TRIP:                  return NET_ALARM_PERSON_TRIP;
    case Event::Type_E::SAFETY_HELMET:                return NET_ALARM_HELMET_MISSING;
    case Event::Type_E::REFLECTIVE_CLOTHING:          return NET_ALARM_NO_REFLECTIVE_VEST;
    case Event::Type_E::HIGH_ALTITUDE_SEATBELT:       return NET_ALARM_HIGH_ALTITUDE_SEATBELT;
    case Event::Type_E::CONSTRUCTION_OCCUPY_ROAD:     return NET_ALARM_CONSTRUCTION_OCCUPY_ROAD;
    case Event::Type_E::EMERGENCY_LANE_OCCUPANCY:     return NET_ALARM_EMERGENCY_LANE_OCCUPANCY;
    case Event::Type_E::REVERSE_DIRECTION:            return NET_ALARM_REVERSE_DIRECTION;
    case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION:  return NET_ALARM_NON_MOTOR_VEHICLE_INTRUSION;
    case Event::Type_E::ROAD_PONDING:                 return NET_ALARM_ROAD_PONDING;
    case Event::Type_E::CONGESTION:                   return NET_ALARM_CONGESTION;
    case Event::Type_E::ILLEGAL_LANE_CHANGE:          return NET_ALARM_ILLEGAL_LANE_CHANGE;
    case Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR: return NET_ALARM_ELECTRIC_VEHICLE_IN_ELEVATOR;
    case Event::Type_E::GARBAGE_EXPOSURE:             return NET_ALARM_GARBAGE_EXPOSURE;
    case Event::Type_E::GARBAGE_OVERFLOW:             return NET_ALARM_GARBAGE_OVERFLOW;
    case Event::Type_E::SCENE_CHANGE:                 return NET_ALARM_SCENE_CHANGE;
    case Event::Type_E::AUDIO_ANOMALY:                return NET_ALARM_AUDIO_ANOMALY;
    case Event::Type_E::AUDIO_SUDDEN_RISE:            return NET_ALARM_AUDIO_SUDDEN_RISE;
    case Event::Type_E::AUDIO_SUDDEN_DROP:            return NET_ALARM_AUDIO_SUDDEN_DROP;
    default:                                          return 0;
    }
}

/**
 * @brief   : 获取车牌识别事件对应的 TVSDK 告警命令
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {UINT32} TVSDK 告警命令，0 表示不支持
 */
UINT32 get_plate_alarm_type(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::PLATE_NUMBER:                 return NET_ALARM_PLATE_RECOGNITION;
    default:                                          return 0;
    }
}

/**
 * @brief   : 判断是否是统计类事件
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {bool} true：统计类事件 false：非统计类事件
 */
bool is_statistics_event(Event::Type_E enEventType)
{
    switch (enEventType)
    {
#if CAP_AI_PEOPLE_STATISTICS
    case Event::Type_E::PEOPLE_FLOW_STATISTICS:
    case Event::Type_E::PEOPLE_DENSITY_DETECTION:
        return true;
#endif
    default:
        return false;
    }
}

/**
 * @brief   : 推送普通 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @param    {UINT32} dwAlarmType TVSDK 告警命令
 * @return   {void}
 */
void push_basic_alarm(const EventTriggerContext_S &stContext, UINT32 dwAlarmType)
{
    const long long llPushStartMs = TimeUtils_NS::get_currentTimestampMs();

    /* 新 SDK 的图片字段内嵌在告警结构中，使用堆内存避免事件线程栈承载 1 MiB 报文。 */
    std::unique_ptr<NET_AlarmBasicInfo_S> pInfo(new NET_AlarmBasicInfo_S());
    NET_AlarmBasicInfo_S &stInfo = *pInfo;
    stInfo.uAlarmType = dwAlarmType;
    stInfo.llTimestampMs = get_context_timestamp_ms(stContext);

    if (stContext.nChnId >= 0 && stContext.nChnId < NET_MAX_ALARM_IN_NUM)
    {
        pInfo->byChannel[stContext.nChnId] = 1;
    }

    const EventTvSdkImage_S &stPanoramaImage =
        stContext.stPanoramaImage.vecJpeg.empty() && stContext.pTvSdkPayload
            ? stContext.pTvSdkPayload->stPanoramaImage
            : stContext.stPanoramaImage;
    stInfo.uPanoramaImgLen = copy_tvsdk_image(stInfo.byPanoramaImg, stPanoramaImage);

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        static_cast<int>(stInfo.uAlarmType), pInfo.get(), sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送普通告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送普通告警成功: cmd[0x%x] timestamp[%lld]",
                  stInfo.uAlarmType,
                  static_cast<long long>(stInfo.llTimestampMs));
    }

    /*
     * perf: 告警推送耗时探针。超过阈值说明TVSDK链路（含1MB结构拷贝）影响事件线程，
     * 结合RTSP丢帧日志可判断卡顿是否与TVSDK推送相关。
     */
    const long long llPushCostMs = TimeUtils_NS::get_currentTimestampMs() - llPushStartMs;
    if (llPushCostMs > TVSDK_PUSH_WARN_MS)
    {
        dlog_warn("TVSDK告警推送耗时过长: cmd[0x%x] cost[%lldms]",
                  pInfo->uAlarmType,
                  llPushCostMs);
    }
}

/**
 * @brief   : 推送周界 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @param    {UINT32} dwAlarmType TVSDK 告警命令
 * @return   {void}
 */
void push_rule_alarm(const EventTriggerContext_S &stContext, UINT32 dwAlarmType)
{
    std::unique_ptr<NET_AlarmRuleInfo_S> pInfo(new NET_AlarmRuleInfo_S());
    NET_AlarmRuleInfo_S &stInfo = *pInfo;
    stInfo.uAlarmType = dwAlarmType;
    stInfo.uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.uRuleID = static_cast<UINT32>(std::max(0, get_context_attr_int(stContext, "rule_id", 0)));
    stInfo.uRuleType = stInfo.uAlarmType;
    stInfo.uTargetID = static_cast<UINT32>(std::max(0, stContext.nTargetId));
    stInfo.uObjectType = static_cast<UINT32>(std::max(0, stContext.nObjectType));
    stInfo.fConfidence = stContext.fConfidence;
    stInfo.nLeft = stContext.nLeft;
    stInfo.nTop = stContext.nTop;
    stInfo.nRight = stContext.nRight;
    stInfo.nBottom = stContext.nBottom;
    stInfo.llTimestampMs = get_context_timestamp_ms(stContext);

    const EventTvSdkImage_S &stPanoramaImage =
        stContext.stPanoramaImage.vecJpeg.empty() && stContext.pTvSdkPayload
            ? stContext.pTvSdkPayload->stPanoramaImage
            : stContext.stPanoramaImage;

    stInfo.uPanoramaImgLen = copy_tvsdk_image(stInfo.byPanoramaImg, stPanoramaImage);
    stInfo.uTargetImgLen = copy_tvsdk_image(stInfo.byTargetImg, stContext.stTargetImage);

    dlog_info("TVSDK周界告警内容: cmd[0x%x], event[%d], chn[%u], rule[%u], target[%u], objType[%u], timestamp[%lld], "
              "rect[%d,%d,%d,%d], contextPanorama[%zu], contextTarget[%zu], payload[%d], "
              "sendPanorama[%u], sendTarget[%u], structSize[%zu]",
              stInfo.uAlarmType,
              static_cast<int>(stContext.enEventType),
              stInfo.uChannel,
              stInfo.uRuleID,
              stInfo.uTargetID,
              stInfo.uObjectType,
              static_cast<long long>(stInfo.llTimestampMs),
              stInfo.nLeft,
              stInfo.nTop,
              stInfo.nRight,
              stInfo.nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size(),
              stContext.pTvSdkPayload ? 1 : 0,
              stInfo.uPanoramaImgLen,
              stInfo.uTargetImgLen,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        static_cast<int>(stInfo.uAlarmType), pInfo.get(), sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送周界告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送周界告警成功: cmd[0x%x]", stInfo.uAlarmType);
    }
}

/**
 * @brief   : 推送 AI 目标 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @param    {UINT32} dwAlarmType TVSDK 告警命令
 * @return   {void}
 */
void push_ai_object_alarm(const EventTriggerContext_S &stContext, UINT32 dwAlarmType)
{
    std::unique_ptr<NET_AlarmAiObjectInfo_S> pInfo(new NET_AlarmAiObjectInfo_S());
    NET_AlarmAiObjectInfo_S &stInfo = *pInfo;
    stInfo.uAlarmType = dwAlarmType;
    stInfo.uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.uObjectType = static_cast<UINT32>(std::max(0, stContext.nObjectType));
    stInfo.fConfidence = stContext.fConfidence;
    stInfo.nLeft = stContext.nLeft;
    stInfo.nTop = stContext.nTop;
    stInfo.nRight = stContext.nRight;
    stInfo.nBottom = stContext.nBottom;
    copy_tvsdk_string(stInfo.strObjectID, std::to_string(stContext.nTargetId));
    stInfo.llTimestampMs = get_context_timestamp_ms(stContext);

    const EventTvSdkImage_S &stPanoramaImage =
        stContext.stPanoramaImage.vecJpeg.empty() && stContext.pTvSdkPayload
            ? stContext.pTvSdkPayload->stPanoramaImage
            : stContext.stPanoramaImage;

    stInfo.uPanoramaImgLen = copy_tvsdk_image(stInfo.byPanoramaImg, stPanoramaImage);
    stInfo.uImgLen = copy_tvsdk_image(stInfo.byImgData, stContext.stTargetImage);

    dlog_info("TVSDK AI目标告警内容: cmd[0x%x], event[%d], chn[%u], target[%d], objType[%u], timestamp[%lld], "
              "rect[%d,%d,%d,%d], contextPanorama[%zu], contextTarget[%zu], payload[%d], "
              "sendPanorama[%u], sendTarget[%u], structSize[%zu]",
              stInfo.uAlarmType,
              static_cast<int>(stContext.enEventType),
              stInfo.uChannel,
              stContext.nTargetId,
              stInfo.uObjectType,
              static_cast<long long>(stInfo.llTimestampMs),
              stInfo.nLeft,
              stInfo.nTop,
              stInfo.nRight,
              stInfo.nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size(),
              stContext.pTvSdkPayload ? 1 : 0,
              stInfo.uPanoramaImgLen,
              stInfo.uImgLen,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        static_cast<int>(stInfo.uAlarmType), pInfo.get(), sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送AI目标告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送AI目标告警成功: cmd[0x%x]", stInfo.uAlarmType);
    }
}

/**
 * @brief   : 推送异常 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @param    {UINT32} dwAlarmType TVSDK 告警命令
 * @return   {void}
 */
void push_exception_alarm(const EventTriggerContext_S &stContext, UINT32 dwAlarmType)
{
    NET_AlarmExceptionInfo_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.uAlarmType = dwAlarmType;
    stInfo.uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.uDiskNo = 0;
    stInfo.uStatus = 1;
    int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(stInfo.uAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送异常告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送异常告警成功: cmd[0x%x]", stInfo.uAlarmType);
    }
}

/**
 * @brief   : 推送人脸比对 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @return   {void}
 */
void push_face_compare_alarm(const EventTriggerContext_S &stContext)
{
    if (!stContext.pTvSdkPayload || stContext.pTvSdkPayload->enType != EventTvSdkPayloadType_E::FACE_COMPARE)
    {
        return;
    }

    const Event::FaceCompareInfo_S &stSrc = stContext.pTvSdkPayload->stFaceCompare.stFaceCompareInfo;
    std::unique_ptr<NET_AlarmFaceCompareInfo_S> pInfo(new NET_AlarmFaceCompareInfo_S());
    NET_AlarmFaceCompareInfo_S &stInfo = *pInfo;

    int nChannel = stContext.nChnId;
    if (nChannel < 0)
    {
        nChannel = stSrc.stInfo.nChnId;
    }
    if (nChannel < 0)
    {
        nChannel = 0;
    }

    stInfo.uAlarmType = NET_ALARM_FACE_COMPARE;
    stInfo.uChannel = static_cast<UINT32>(nChannel);
    stInfo.llTimestampMs = stSrc.stInfo.lTimestamp > 0 ? stSrc.stInfo.lTimestamp : stContext.llTimestamp;
    stInfo.nEventId = stSrc.nEventId;
    stInfo.nCompResult = stSrc.nCompResult;
    stInfo.nSimilarity = stSrc.nSimilarity;
    stInfo.nFaceId = stSrc.nFaceId;
    copy_tvsdk_string(stInfo.strFaceLibName, stSrc.strFaceLibName);
    copy_tvsdk_string(stInfo.strFaceName, stSrc.strFaceName);
    copy_tvsdk_string(stInfo.strLibFacePath, stSrc.strLibFacePath);
    copy_tvsdk_string(stInfo.strCapFacePath, stSrc.strCapFacePath);
    copy_tvsdk_string(stInfo.strCapImagePath, stSrc.strCapImagePath);

    std::vector<unsigned char> vecLibFaceImage;
    std::vector<unsigned char> vecCapFaceImage;
    load_tvsdk_image_file(stSrc.strLibFacePath, vecLibFaceImage);
    load_tvsdk_image_file(stSrc.strCapFacePath, vecCapFaceImage);
    stInfo.uLibFaceImgLen = copy_tvsdk_image(stInfo.byLibFaceImg, vecLibFaceImage);
    stInfo.uCapFaceImgLen = copy_tvsdk_image(stInfo.byCapFaceImg, vecCapFaceImage);

    dlog_info("TVSDK人脸比对告警填充: cmd[0x%x] 通道[%u] 事件ID[%d] 结果[%d] 相似度[%d] "
              "人脸ID[%d] 库[%s] 名称[%s] 库图长度[%u] 抓拍图长度[%u] buf_len[%zu]",
              stInfo.uAlarmType,
              stInfo.uChannel,
              stInfo.nEventId,
              stInfo.nCompResult,
              stInfo.nSimilarity,
              stInfo.nFaceId,
              stInfo.strFaceLibName,
              stInfo.strFaceName,
              stInfo.uLibFaceImgLen,
              stInfo.uCapFaceImgLen,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        static_cast<int>(stInfo.uAlarmType), pInfo.get(), sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送人脸比对告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送人脸比对告警成功: cmd[0x%x]", stInfo.uAlarmType);
    }
}

/**
 * @brief   : 推送车牌识别 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @param    {UINT32} dwAlarmType TVSDK 告警命令
 * @return   {void}
 */
void push_plate_alarm(const EventTriggerContext_S &stContext, UINT32 dwAlarmType)
{
    std::unique_ptr<NET_AlarmPlateInfo_S> pInfo(new NET_AlarmPlateInfo_S());
    NET_AlarmPlateInfo_S &stInfo = *pInfo;
    stInfo.uAlarmType = dwAlarmType;
    stInfo.uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.fConfidence = stContext.fConfidence;

    if (stContext.pTvSdkPayload && stContext.pTvSdkPayload->enType == EventTvSdkPayloadType_E::PLATE)
    {
        const EventTvSdkPlatePayload_S &stPlatePayload = stContext.pTvSdkPayload->stPlate;
        copy_tvsdk_string(stInfo.strPlateNumber, stPlatePayload.strPlateNumber);
        stInfo.uPlateColor = stPlatePayload.dwPlateColor;
        stInfo.uVehicleType = stPlatePayload.dwVehicleType;
        stInfo.uSpeed = stPlatePayload.dwSpeed;
        stInfo.uLaneNo = stPlatePayload.dwLaneNo;
    }

    stInfo.uPlateImgLen = copy_tvsdk_image(stInfo.byPlateImg, stContext.stTargetImage);

    dlog_info("TVSDK车牌识别告警内容: cmd[0x%x], chn[%u], plate[%s], confidence[%.2f], buf_len[%zu]",
              stInfo.uAlarmType,
              stInfo.uChannel,
              stInfo.strPlateNumber,
              stInfo.fConfidence,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        static_cast<int>(stInfo.uAlarmType), pInfo.get(), sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送车牌识别告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送车牌识别告警成功: cmd[0x%x]", stInfo.uAlarmType);
    }
}

/**
 * @brief   : 填充统计类目标快照
 * @param    {EventTvSdkTarget_S} &stSrc 源目标快照
 * @param    {NET_AlarmStatisticsTarget_S} &stDst 目标协议结构
 * @return   {void}
 */
void fill_statistics_target(const EventTvSdkTarget_S &stSrc, NET_AlarmStatisticsTarget_S &stDst)
{
    stDst.nTrackID = stSrc.nTrackId;
    stDst.uRuleID = static_cast<UINT32>(std::max(0, stSrc.nRuleId));
    stDst.uSnapshotType = static_cast<UINT32>(std::max(0, stSrc.nSnapshotType));
    stDst.nLeft = stSrc.nLeft;
    stDst.nTop = stSrc.nTop;
    stDst.nRight = stSrc.nRight;
    stDst.nBottom = stSrc.nBottom;
    stDst.llTimestampMs = stSrc.llTimestampMs;
    stDst.nDirection = stSrc.nDirection;
    stDst.uImgLen = copy_tvsdk_image(stDst.byImgData, stSrc.vecJpeg);
}

/**
 * @brief   : 推送统计类 TVSDK 告警
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @return   {void}
 */
void push_statistics_alarm(const EventTriggerContext_S &stContext)
{
    if (!stContext.pTvSdkPayload || stContext.pTvSdkPayload->enType != EventTvSdkPayloadType_E::STATISTICS)
    {
        return;
    }

    const EventTvSdkStatisticsPayload_S &stPayload = stContext.pTvSdkPayload->stStatistics;
    std::unique_ptr<NET_AlarmStatisticsInfo_S> pInfo(new NET_AlarmStatisticsInfo_S());
    NET_AlarmStatisticsInfo_S &stInfo = *pInfo;
    stInfo.uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.uRuleID = static_cast<UINT32>(std::max(0, stPayload.nRuleId));
    stInfo.llTimestampMs = stPayload.llTimestampMs > 0 ? stPayload.llTimestampMs : stContext.llTimestamp;
    stInfo.uReportSeq = stPayload.nReportSeq;
    stInfo.uEnterCount = stPayload.nEnterCount;
    stInfo.uLeaveCount = stPayload.nLeaveCount;
    stInfo.uTotalCount = stPayload.nTotalCount;
    stInfo.uCurrentPeopleCount = stPayload.nCurrentPeopleCount;
    stInfo.uAverageStayTimeSec = stPayload.nAverageStayTimeSec;

    if (stPayload.nStatisticsType == static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_FLOW))
    {
        stInfo.uAlarmType = NET_ALARM_PEOPLE_FLOW_STATISTICS;
        stInfo.uStatisticsType = NET_STATISTICS_TYPE_PEOPLE_FLOW;
    }
    else if (stPayload.nStatisticsType == static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_DENSITY))
    {
        stInfo.uAlarmType = NET_ALARM_PEOPLE_DENSITY_STATISTICS;
        stInfo.uStatisticsType = NET_STATISTICS_TYPE_PEOPLE_DENSITY;
    }
    else
    {
        return;
    }

    const size_t nTargetCount = std::min(stPayload.vecTargets.size(),
                                         static_cast<size_t>(NET_ALARM_STATISTICS_TARGET_MAX_NUM));
    stInfo.uTargetCount = static_cast<UINT32>(nTargetCount);
    for (size_t i = 0; i < nTargetCount; ++i)
    {
        fill_statistics_target(stPayload.vecTargets[i], stInfo.stTargets[i]);
    }
    stInfo.uPanoramaImgLen = copy_tvsdk_image(stInfo.byPanoramaImg, stPayload.stPanoramaImage);

    dlog_info("TVSDK统计告警填充: cmd[0x%x] 通道[%u] 类型[%u] 规则[%u] 时间戳[%lld] 序号[%u] "
              "进入[%u] 离开[%u] 总数[%u] 当前人数[%u] 平均停留[%u] 目标数[%u] buf_len[%zu]",
              stInfo.uAlarmType,
              stInfo.uChannel,
              stInfo.uStatisticsType,
              stInfo.uRuleID,
              static_cast<long long>(stInfo.llTimestampMs),
              stInfo.uReportSeq,
              stInfo.uEnterCount,
              stInfo.uLeaveCount,
              stInfo.uTotalCount,
              stInfo.uCurrentPeopleCount,
              stInfo.uAverageStayTimeSec,
              stInfo.uTargetCount,
              sizeof(stInfo));
    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        static_cast<int>(stInfo.uAlarmType), pInfo.get(), sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送统计告警失败: cmd[0x%x] ret[%d]", stInfo.uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送统计告警成功: cmd[0x%x]", stInfo.uAlarmType);
    }
}
} // namespace
#endif

std::string EventLinkageDict::get_event_name(Event::Type_E enType)
{
    switch (enType)
    {
    case Event::Type_E::MOTION_DETECT:
        return "移动侦测";
    case Event::Type_E::OCCLUSION_DETECT:
        return "遮挡侦测";
    case Event::Type_E::ANOMALY_ALARM:
        return "异常报警";
    case Event::Type_E::DISK_FULL:
        return "异常报警-硬盘满";
    case Event::Type_E::DISK_ERROR:
        return "异常报警-硬盘错误";
    case Event::Type_E::NET_BROKEN:
        return "异常报警-网络断开";
    case Event::Type_E::IP_CONFLICT:
        return "异常报警-IP冲突";
    case Event::Type_E::ILLEGAL_ACCESS:
        return "异常报警-非法访问";
    case Event::Type_E::RECORD_ABNORMAL:
        return "异常报警-录像异常";
    case Event::Type_E::HARDWARE_ABNORMAL:
        return "异常报警-配件版异常";
    case Event::Type_E::IP_CHANNEL_CONFLICT:
        return "异常报警-IP通道冲突";
    case Event::Type_E::STREAM_RESOLUTION_LIMIT:
        return "异常报警-子码流分辨率/码率超限";
    case Event::Type_E::VIDEO_SIGNAL_LOSS:
        return "异常报警-视频信号丢失";
    case Event::Type_E::AUDIO_ALARM:
        return "声音报警";
    case Event::Type_E::ALARM_INPUT:
        return "报警输入";
    case Event::Type_E::PIR_ALARM:
        return "PIR红外感应报警";
    case Event::Type_E::MANUAL_SOUND_LIGHT_ALARM:
        return "手动声光报警";
    case Event::Type_E::LINE_CROSSING:
        return "越界侦测";
    case Event::Type_E::INTRUSION:
        return "区域入侵";
    case Event::Type_E::ENTER_REGION:
        return "进入区域";
    case Event::Type_E::LEAVE_REGION:
        return "离开区域";
    case Event::Type_E::AUDIO_ANOMALY:
        return "音频异常侦测";
    case Event::Type_E::AUDIO_SUDDEN_RISE:
        return "音频异常侦测-声强陡升检测";
    case Event::Type_E::AUDIO_SUDDEN_DROP:
        return "音频异常侦测-声强陡降检测";
    case Event::Type_E::SCENE_CHANGE:
        return "场景变更";
    case Event::Type_E::FACE_DETECT:
        return "人脸侦测";
    case Event::Type_E::LOITERING_DETECT:
        return "徘徊侦测";
    case Event::Type_E::CROWD_GATHERING:
        return "人员聚集";
    case Event::Type_E::PARKING_DETECT:
        return "停车侦测";
    case Event::Type_E::UNATTENDED_OBJECT:
        return "物品遗留";
    case Event::Type_E::OBJECT_REMOVAL:
        return "物品拿取";
    case Event::Type_E::PET_RECOGNITION:
        return "宠物识别";
    case Event::Type_E::FACE_CAPTURE:
        return "人脸抓拍";
    case Event::Type_E::FACE_COMPARE_SUCCESS:
        return "人脸比对事件触发，抓拍人脸与目标库匹配";
    case Event::Type_E::FACE_COMPARE_FAIL:
        return "人脸比对事件触发，抓拍人脸与目标库不匹配";
#ifdef SCENE_INTELLIGENCE
    case Event::Type_E::SLEEP_ON_DUTY:
        return "睡岗识别";
    case Event::Type_E::LEAVE_POST:
        return "离岗识别";
    case Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR:
        return "电瓶车进电梯识别";
    case Event::Type_E::PERSON_FALL_DOWN:
        return "人员倒地识别";
    case Event::Type_E::FENCE_CLIMBING:
        return "翻越围栏识别";
    case Event::Type_E::SMOKING:
        return "抽烟识别";
    case Event::Type_E::PHONE_USAGE:
        return "玩手机识别";
    case Event::Type_E::SMOKE_FIRE:
        return "烟火识别";
    case Event::Type_E::OPEN_FLAME:
        return "明火识别";
    case Event::Type_E::MANHOLE_COVER_ABNORMAL:
        return "井盖异常检测";
    case Event::Type_E::BARE_SOIL:
        return "黄土裸露识别";
    case Event::Type_E::HOLE_PROTECTION_BAR:
        return "洞口防护栏识别";
    case Event::Type_E::PEDESTRIAN_INTRUSION:
        return "行人闯入识别";
    case Event::Type_E::PERSON_TRIP:
        return "摔倒识别";
    case Event::Type_E::SAFETY_HELMET:
        return "安全帽识别";
    case Event::Type_E::REFLECTIVE_CLOTHING:
        return "反光衣识别";
    case Event::Type_E::HIGH_ALTITUDE_SEATBELT:
        return "高空安全带识别";
    case Event::Type_E::CONSTRUCTION_OCCUPY_ROAD:
        return "施工占道识别";
    case Event::Type_E::EMERGENCY_LANE_OCCUPANCY:
        return "应急车道占用识别";
    case Event::Type_E::REVERSE_DIRECTION:
        return "逆行识别";
    case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION:
        return "非机动车闯入识别";
    case Event::Type_E::ROAD_PONDING:
        return "道路积水识别";
    case Event::Type_E::CONGESTION:
        return "拥堵识别";
    case Event::Type_E::ILLEGAL_PARKING:
        return "违规停车识别";
    case Event::Type_E::ILLEGAL_LANE_CHANGE:
        return "违规变道识别";
    case Event::Type_E::PLATE_NUMBER:
        return "车牌识别";
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    case Event::Type_E::GARBAGE_EXPOSURE:
        return "垃圾暴露识别";
    case Event::Type_E::GARBAGE_OVERFLOW:
        return "垃圾满溢识别";
#endif
#if CAP_AI_PEOPLE_STATISTICS
    case Event::Type_E::PEOPLE_FLOW_STATISTICS:
        return "人流统计";
    case Event::Type_E::PEOPLE_DENSITY_DETECTION:
        return "人员密度检测";
    case Event::Type_E::PEOPLE_FLOW_STAY_NORMAL:
        return "人流统计-滞留人数普通报警";
    case Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM:
        return "人流统计-滞留人数中度报警";
    case Event::Type_E::PEOPLE_FLOW_STAY_SEVERE:
        return "人流统计-滞留人数严重报警";
    case Event::Type_E::PEOPLE_DENSITY_NORMAL:
        return "人员密度检测-普通报警";
    case Event::Type_E::PEOPLE_DENSITY_MEDIUM:
        return "人员密度检测-中度报警";
    case Event::Type_E::PEOPLE_DENSITY_SEVERE:
        return "人员密度检测-严重报警";
#endif
#ifdef SCENE_INTELLIGENT_ANALYSIS
    case Event::Type_E::IMAGE_ANALYSIS:
        return "AI图像分析";
    case Event::Type_E::TEXT_PRESET:
        return "AI文本预设任务";
    case Event::Type_E::REAL_ALARM:
        return "AI实时预警";
#endif
    default:
        return "未知事件";
    }
}

void EventLinkageDict::push_tvsdk_event_alarm(Event::Type_E enEventType, bool bEventEnded, int nChnId)
{
    EventTriggerContext_S stContext;
    stContext.enEventType = enEventType;
    stContext.bEventEnded = bEventEnded;
    stContext.nChnId = nChnId;
    push_tvsdk_event_alarm(stContext);
}

void EventLinkageDict::push_tvsdk_event_alarm(const EventTriggerContext_S &stContext)
{
#ifndef ENABLE_TVSDK_SRC
    (void)stContext;
    return;
#else
    if (stContext.bEventEnded || ControlManage::instance()->tvsdk_get_client_count() <= 0)
    {
        return;
    }

    if (stContext.enEventType == Event::Type_E::FACE_COMPARE ||
        stContext.enEventType == Event::Type_E::FACE_COMPARE_SUCCESS ||
        stContext.enEventType == Event::Type_E::FACE_COMPARE_FAIL)
    {
        push_face_compare_alarm(stContext);
        return;
    }

    const UINT32 dwBasicAlarmType = get_basic_alarm_type(stContext.enEventType);
    if (dwBasicAlarmType != 0)
    {
        push_basic_alarm(stContext, dwBasicAlarmType);
        return;
    }

    const UINT32 dwRuleAlarmType = get_rule_alarm_type(stContext.enEventType);
    if (dwRuleAlarmType != 0)
    {
        push_rule_alarm(stContext, dwRuleAlarmType);
        return;
    }

    const UINT32 dwExceptionAlarmType = get_exception_alarm_type(stContext.enEventType);
    if (dwExceptionAlarmType != 0)
    {
        push_exception_alarm(stContext, dwExceptionAlarmType);
        return;
    }

    const UINT32 dwAiObjectAlarmType = get_ai_object_alarm_type(stContext.enEventType);
    if (dwAiObjectAlarmType != 0)
    {
        push_ai_object_alarm(stContext, dwAiObjectAlarmType);
        return;
    }

    const UINT32 dwPlateAlarmType = get_plate_alarm_type(stContext.enEventType);
    if (dwPlateAlarmType != 0)
    {
        push_plate_alarm(stContext, dwPlateAlarmType);
        return;
    }

    if (is_statistics_event(stContext.enEventType))
    {
        push_statistics_alarm(stContext);
    }
#endif
}
