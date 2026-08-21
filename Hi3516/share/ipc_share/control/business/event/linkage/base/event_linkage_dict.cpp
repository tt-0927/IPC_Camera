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
 * @brief   : 创建 TVSDK V2 图片视图，不复制 JPEG 数据
 * @details : 图片所有权仍由事件上下文持有，调用 tvsdk_push_alarm_v2 返回前不得释放上下文。
 */
NET_TV_IMAGE_DATA_S make_tvsdk_image_view(const std::vector<unsigned char> &vecJpeg)
{
    NET_TV_IMAGE_DATA_S stImage = {};
    if (vecJpeg.empty())
    {
        return stImage;
    }

    if (vecJpeg.size() > static_cast<size_t>(std::numeric_limits<UINT32>::max()))
    {
        dlog_warn("TVSDK告警图片超过UINT32长度上限: size[%zu]", vecJpeg.size());
        return stImage;
    }

    stImage.pData = vecJpeg.data();
    stImage.dwLen = static_cast<UINT32>(vecJpeg.size());
    return stImage;
}

NET_TV_IMAGE_DATA_S make_tvsdk_image_view(const EventTvSdkImage_S &stImage)
{
    NET_TV_IMAGE_DATA_S stView = make_tvsdk_image_view(stImage.vecJpeg);
    stView.dwWidth = stImage.nWidth > 0 ? static_cast<UINT32>(stImage.nWidth) : 0;
    stView.dwHeight = stImage.nHeight > 0 ? static_cast<UINT32>(stImage.nHeight) : 0;
    return stView;
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
 * @details : 人脸比对图片没有事件上下文二进制缓存，因此仅在本次同步推送期间由 vecImage 持有。
 */
void load_tvsdk_image_file_v2(const std::string &strPath,
                              std::vector<unsigned char> &vecImage,
                              NET_TV_IMAGE_DATA_S &stImage)
{
    stImage = {};
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
    if (static_cast<unsigned long long>(nSize) >
        static_cast<unsigned long long>(std::numeric_limits<UINT32>::max()))
    {
        dlog_warn("TVSDK人脸比对图片超过UINT32长度上限: %s, size[%lld]",
                  strPath.c_str(),
                  static_cast<long long>(nSize));
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

    stImage = make_tvsdk_image_view(vecImage);
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
    case Event::Type_E::MOTION_DETECT:    return NET_TV_ALARM_MOTION_DETECT;
    case Event::Type_E::OCCLUSION_DETECT: return NET_TV_ALARM_OCCLUSION;
    case Event::Type_E::ANOMALY_ALARM:    return NET_TV_ALARM_ANOMALY;
    case Event::Type_E::ALARM_INPUT:      return NET_TV_ALARM_INPUT;
    case Event::Type_E::PIR_ALARM:        return NET_TV_ALARM_PIR;
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
    case Event::Type_E::LINE_CROSSING:     return NET_TV_ALARM_LINE_CROSSING;
    case Event::Type_E::INTRUSION:         return NET_TV_ALARM_INTRUSION;
    case Event::Type_E::ENTER_REGION:      return NET_TV_ALARM_ENTER_REGION;
    case Event::Type_E::LEAVE_REGION:      return NET_TV_ALARM_LEAVE_REGION;
    case Event::Type_E::OBJECT_REMOVAL:    return NET_TV_ALARM_OBJECT_REMOVAL;
    case Event::Type_E::UNATTENDED_OBJECT: return NET_TV_ALARM_UNATTENDED_OBJECT;
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
    case Event::Type_E::DISK_FULL:         return NET_TV_ALARM_DISK_FULL;
    case Event::Type_E::DISK_ERROR:        return NET_TV_ALARM_DISK_ERROR;
    case Event::Type_E::NET_BROKEN:        return NET_TV_ALARM_NET_BROKEN;
    case Event::Type_E::IP_CONFLICT:       return NET_TV_ALARM_IP_CONFLICT;
    case Event::Type_E::ILLEGAL_ACCESS:    return NET_TV_ALARM_ANOMALY;
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
    case Event::Type_E::LOITERING_DETECT:             return NET_TV_ALARM_LOITERING;
    case Event::Type_E::PARKING_DETECT:               return NET_TV_ALARM_PARKING_DETECT;
    case Event::Type_E::FACE_DETECT:                  return NET_TV_ALARM_FACE_DETECT;
    case Event::Type_E::FACE_CAPTURE:                 return NET_TV_ALARM_FACE_CAPTURE;
    case Event::Type_E::CROWD_GATHERING:              return NET_TV_ALARM_CROWD_GATHERING;
    case Event::Type_E::SLEEP_ON_DUTY:                return NET_TV_ALARM_SLEEP_ON_DUTY;
    case Event::Type_E::LEAVE_POST:                   return NET_TV_ALARM_LEAVE_POST;
    case Event::Type_E::PERSON_FALL_DOWN:             return NET_TV_ALARM_PERSON_FALL;
    case Event::Type_E::FENCE_CLIMBING:               return NET_TV_ALARM_FENCE_CLIMBING;
    case Event::Type_E::SMOKING:                      return NET_TV_ALARM_SMOKING;
    case Event::Type_E::PHONE_USAGE:                  return NET_TV_ALARM_PHONE_USAGE;
    case Event::Type_E::SMOKE_FIRE:                   return NET_TV_ALARM_SMOKE_FIRE;
    case Event::Type_E::OPEN_FLAME:                   return NET_TV_ALARM_OPEN_FLAME;
    case Event::Type_E::MANHOLE_COVER_ABNORMAL:       return NET_TV_ALARM_MANHOLE_COVER_ABNORMAL;
    case Event::Type_E::BARE_SOIL:                    return NET_TV_ALARM_BARE_SOIL;
    case Event::Type_E::HOLE_PROTECTION_BAR:          return NET_TV_ALARM_HOLE_PROTECTION_BAR;
    case Event::Type_E::PEDESTRIAN_INTRUSION:         return NET_TV_ALARM_PEDESTRIAN_INTRUSION;
    case Event::Type_E::PERSON_TRIP:                  return NET_TV_ALARM_PERSON_TRIP;
    case Event::Type_E::SAFETY_HELMET:                return NET_TV_ALARM_HELMET_MISSING;
    case Event::Type_E::REFLECTIVE_CLOTHING:          return NET_TV_ALARM_NO_REFLECTIVE_VEST;
    case Event::Type_E::HIGH_ALTITUDE_SEATBELT:       return NET_TV_ALARM_HIGH_ALTITUDE_SEATBELT;
    case Event::Type_E::CONSTRUCTION_OCCUPY_ROAD:     return NET_TV_ALARM_CONSTRUCTION_OCCUPY_ROAD;
    case Event::Type_E::EMERGENCY_LANE_OCCUPANCY:     return NET_TV_ALARM_EMERGENCY_LANE_OCCUPANCY;
    case Event::Type_E::REVERSE_DIRECTION:            return NET_TV_ALARM_REVERSE_DIRECTION;
    case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION:  return NET_TV_ALARM_NON_MOTOR_VEHICLE_INTRUSION;
    case Event::Type_E::ROAD_PONDING:                 return NET_TV_ALARM_ROAD_PONDING;
    case Event::Type_E::CONGESTION:                   return NET_TV_ALARM_CONGESTION;
    case Event::Type_E::ILLEGAL_LANE_CHANGE:          return NET_TV_ALARM_ILLEGAL_LANE_CHANGE;
    case Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR: return NET_TV_ALARM_ELECTRIC_VEHICLE_IN_ELEVATOR;
    case Event::Type_E::GARBAGE_EXPOSURE:             return NET_TV_ALARM_GARBAGE_EXPOSURE;
    case Event::Type_E::GARBAGE_OVERFLOW:             return NET_TV_ALARM_GARBAGE_OVERFLOW;
    case Event::Type_E::SCENE_CHANGE:                 return NET_TV_ALARM_SCENE_CHANGE;
    case Event::Type_E::AUDIO_ANOMALY:                return NET_TV_ALARM_AUDIO_ANOMALY;
    case Event::Type_E::AUDIO_SUDDEN_RISE:            return NET_TV_ALARM_AUDIO_SUDDEN_RISE;
    case Event::Type_E::AUDIO_SUDDEN_DROP:            return NET_TV_ALARM_AUDIO_SUDDEN_DROP;
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
    case Event::Type_E::PLATE_NUMBER:                 return NET_TV_ALARM_PLATE_RECOGNITION;
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
    NET_TV_ALARM_BASIC_INFO_V2_S stInfo = {};
    stInfo.dwAlarmType = dwAlarmType;
    stInfo.llTimestampMs = get_context_timestamp_ms(stContext);
    // /* perf: 记录推送起始时刻，推送结束后统计耗时 */
    const long long llPushStartMs = TimeUtils_NS::get_currentTimestampMs();

    /* perf: new T() 已完成约1MiB协议结构的零初始化，避免再次清空同一块内存。 */
    std::unique_ptr<NET_TV_ALARM_BASIC_INFO_S> pInfo(new NET_TV_ALARM_BASIC_INFO_S());
    pInfo->dwAlarmType = dwAlarmType;
    pInfo->llTimestampMs = get_context_timestamp_ms(stContext);

    if (stContext.nChnId >= 0 && stContext.nChnId < NET_TV_MAX_ALARM_IN_NUM)
    {
        stInfo.byChannel[stContext.nChnId] = 1;
    }

    if (stContext.pTvSdkPayload)
    {
        stInfo.stPanoramaImg = make_tvsdk_image_view(stContext.pTvSdkPayload->stPanoramaImage);
    }

    const int nRet = ControlManage::instance()->tvsdk_push_alarm_v2(
        static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送普通告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送普通告警成功: cmd[0x%x] timestamp[%lld]",
                  stInfo.dwAlarmType,
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
                  pInfo->dwAlarmType,
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
    NET_TV_ALARM_RULE_INFO_V2_S stInfo = {};
    stInfo.dwAlarmType = dwAlarmType;
    stInfo.dwChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.dwRuleID = static_cast<UINT32>(std::max(0, get_context_attr_int(stContext, "rule_id", 0)));
    stInfo.dwRuleType = stInfo.dwAlarmType;
    stInfo.dwTargetID = static_cast<UINT32>(std::max(0, stContext.nTargetId));
    stInfo.dwObjectType = static_cast<UINT32>(std::max(0, stContext.nObjectType));
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

    stInfo.stPanoramaImg = make_tvsdk_image_view(stPanoramaImage);
    stInfo.stTargetImg = make_tvsdk_image_view(stContext.stTargetImage);

    dlog_info("TVSDK周界告警内容: cmd[0x%x], event[%d], chn[%u], rule[%u], target[%u], objType[%u], timestamp[%lld], "
              "rect[%d,%d,%d,%d], contextPanorama[%zu], contextTarget[%zu], payload[%d], "
              "sendPanorama[%u], sendTarget[%u], structSize[%zu]",
              stInfo.dwAlarmType,
              static_cast<int>(stContext.enEventType),
              stInfo.dwChannel,
              stInfo.dwRuleID,
              stInfo.dwTargetID,
              stInfo.dwObjectType,
              static_cast<long long>(stInfo.llTimestampMs),
              stInfo.nLeft,
              stInfo.nTop,
              stInfo.nRight,
              stInfo.nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size(),
              stContext.pTvSdkPayload ? 1 : 0,
              stInfo.stPanoramaImg.dwLen,
              stInfo.stTargetImg.dwLen,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm_v2(
        static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送周界告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送周界告警成功: cmd[0x%x]", stInfo.dwAlarmType);
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
    NET_TV_ALARM_AI_OBJECT_INFO_V2_S stInfo = {};
    stInfo.dwAlarmType = dwAlarmType;
    stInfo.dwChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.dwObjectType = static_cast<UINT32>(std::max(0, stContext.nObjectType));
    stInfo.fConfidence = stContext.fConfidence;
    stInfo.nLeft = stContext.nLeft;
    stInfo.nTop = stContext.nTop;
    stInfo.nRight = stContext.nRight;
    stInfo.nBottom = stContext.nBottom;
    copy_tvsdk_string(stInfo.szObjectID, std::to_string(stContext.nTargetId));
    stInfo.llTimestampMs = get_context_timestamp_ms(stContext);

    const EventTvSdkImage_S &stPanoramaImage =
        stContext.stPanoramaImage.vecJpeg.empty() && stContext.pTvSdkPayload
            ? stContext.pTvSdkPayload->stPanoramaImage
            : stContext.stPanoramaImage;

    stInfo.stPanoramaImg = make_tvsdk_image_view(stPanoramaImage);
    stInfo.stImgData = make_tvsdk_image_view(stContext.stTargetImage);

    dlog_info("TVSDK AI目标告警内容: cmd[0x%x], event[%d], chn[%u], target[%d], objType[%u], timestamp[%lld], "
              "rect[%d,%d,%d,%d], contextPanorama[%zu], contextTarget[%zu], payload[%d], "
              "sendPanorama[%u], sendTarget[%u], structSize[%zu]",
              stInfo.dwAlarmType,
              static_cast<int>(stContext.enEventType),
              stInfo.dwChannel,
              stContext.nTargetId,
              stInfo.dwObjectType,
              static_cast<long long>(stInfo.llTimestampMs),
              stInfo.nLeft,
              stInfo.nTop,
              stInfo.nRight,
              stInfo.nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size(),
              stContext.pTvSdkPayload ? 1 : 0,
              stInfo.stPanoramaImg.dwLen,
              stInfo.stImgData.dwLen,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm_v2(
        static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送AI目标告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送AI目标告警成功: cmd[0x%x]", stInfo.dwAlarmType);
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
    NET_TV_ALARM_EXCEPTION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.dwAlarmType = dwAlarmType;
    stInfo.dwChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.dwDiskNo = 0;
    stInfo.dwStatus = 1;
    int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送异常告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送异常告警成功: cmd[0x%x]", stInfo.dwAlarmType);
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
    NET_TV_ALARM_FACE_COMPARE_INFO_V2_S stInfo = {};

    int nChannel = stContext.nChnId;
    if (nChannel < 0)
    {
        nChannel = stSrc.stInfo.nChnId;
    }
    if (nChannel < 0)
    {
        nChannel = 0;
    }

    stInfo.dwAlarmType = NET_TV_ALARM_FACE_COMPARE;
    stInfo.dwChannel = static_cast<UINT32>(nChannel);
    stInfo.llTimestampMs = stSrc.stInfo.lTimestamp > 0 ? stSrc.stInfo.lTimestamp : stContext.llTimestamp;
    stInfo.nEventId = stSrc.nEventId;
    stInfo.nCompResult = stSrc.nCompResult;
    stInfo.nSimilarity = stSrc.nSimilarity;
    stInfo.nFaceId = stSrc.nFaceId;
    copy_tvsdk_string(stInfo.szFaceLibName, stSrc.strFaceLibName);
    copy_tvsdk_string(stInfo.szFaceName, stSrc.strFaceName);
    copy_tvsdk_string(stInfo.szLibFacePath, stSrc.strLibFacePath);
    copy_tvsdk_string(stInfo.szCapFacePath, stSrc.strCapFacePath);
    copy_tvsdk_string(stInfo.szCapImagePath, stSrc.strCapImagePath);

    std::vector<unsigned char> vecLibFaceImage;
    std::vector<unsigned char> vecCapFaceImage;
    load_tvsdk_image_file_v2(stSrc.strLibFacePath, vecLibFaceImage, stInfo.stLibFaceImg);
    load_tvsdk_image_file_v2(stSrc.strCapFacePath, vecCapFaceImage, stInfo.stCapFaceImg);

    dlog_info("TVSDK人脸比对告警填充: cmd[0x%x] 通道[%u] 事件ID[%d] 结果[%d] 相似度[%d] "
              "人脸ID[%d] 库[%s] 名称[%s] 库图长度[%u] 抓拍图长度[%u] buf_len[%zu]",
              stInfo.dwAlarmType,
              stInfo.dwChannel,
              stInfo.nEventId,
              stInfo.nCompResult,
              stInfo.nSimilarity,
              stInfo.nFaceId,
              stInfo.szFaceLibName,
              stInfo.szFaceName,
              stInfo.stLibFaceImg.dwLen,
              stInfo.stCapFaceImg.dwLen,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm_v2(
        static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送人脸比对告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送人脸比对告警成功: cmd[0x%x]", stInfo.dwAlarmType);
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
    NET_TV_ALARM_PLATE_INFO_V2_S stInfo = {};
    stInfo.dwAlarmType = dwAlarmType;
    stInfo.dwChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.fConfidence = stContext.fConfidence;

    if (stContext.pTvSdkPayload && stContext.pTvSdkPayload->enType == EventTvSdkPayloadType_E::PLATE)
    {
        const EventTvSdkPlatePayload_S &stPlatePayload = stContext.pTvSdkPayload->stPlate;
        copy_tvsdk_string(stInfo.szPlateNumber, stPlatePayload.strPlateNumber);
        stInfo.dwPlateColor = stPlatePayload.dwPlateColor;
        stInfo.dwVehicleType = stPlatePayload.dwVehicleType;
        stInfo.dwSpeed = stPlatePayload.dwSpeed;
        stInfo.dwLaneNo = stPlatePayload.dwLaneNo;
    }

    stInfo.stPlateImg = make_tvsdk_image_view(stContext.stTargetImage);

    dlog_info("TVSDK车牌识别告警内容: cmd[0x%x], chn[%u], plate[%s], confidence[%.2f], buf_len[%zu]",
              stInfo.dwAlarmType,
              stInfo.dwChannel,
              stInfo.szPlateNumber,
              stInfo.fConfidence,
              sizeof(stInfo));

    const int nRet = ControlManage::instance()->tvsdk_push_alarm_v2(
        static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送车牌识别告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送车牌识别告警成功: cmd[0x%x]", stInfo.dwAlarmType);
    }
}

/**
 * @brief   : 填充统计类目标快照
 * @param    {EventTvSdkTarget_S} &stSrc 源目标快照
 * @param    {NET_TV_ALARM_STATISTICS_TARGET_S} &stDst 目标协议结构
 * @return   {void}
 */
void fill_statistics_target(const EventTvSdkTarget_S &stSrc, NET_TV_ALARM_STATISTICS_TARGET_V2_S &stDst)
{
    stDst.nTrackID = stSrc.nTrackId;
    stDst.dwRuleID = static_cast<UINT32>(std::max(0, stSrc.nRuleId));
    stDst.dwSnapshotType = static_cast<UINT32>(std::max(0, stSrc.nSnapshotType));
    stDst.nLeft = stSrc.nLeft;
    stDst.nTop = stSrc.nTop;
    stDst.nRight = stSrc.nRight;
    stDst.nBottom = stSrc.nBottom;
    stDst.llTimestampMs = stSrc.llTimestampMs;
    stDst.nDirection = stSrc.nDirection;
    stDst.stImgData = make_tvsdk_image_view(stSrc.vecJpeg);
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
    NET_TV_ALARM_STATISTICS_INFO_V2_S stInfo = {};
    stInfo.dwChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    stInfo.dwRuleID = static_cast<UINT32>(std::max(0, stPayload.nRuleId));
    stInfo.llTimestampMs = stPayload.llTimestampMs > 0 ? stPayload.llTimestampMs : stContext.llTimestamp;
    stInfo.dwReportSeq = stPayload.nReportSeq;
    stInfo.dwEnterCount = stPayload.nEnterCount;
    stInfo.dwLeaveCount = stPayload.nLeaveCount;
    stInfo.dwTotalCount = stPayload.nTotalCount;
    stInfo.dwCurrentPeopleCount = stPayload.nCurrentPeopleCount;
    stInfo.dwAverageStayTimeSec = stPayload.nAverageStayTimeSec;

    if (stPayload.nStatisticsType == static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_FLOW))
    {
        stInfo.dwAlarmType = NET_TV_ALARM_PEOPLE_FLOW_STATISTICS;
        stInfo.dwStatisticsType = NET_TV_STATISTICS_TYPE_PEOPLE_FLOW;
    }
    else if (stPayload.nStatisticsType == static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_DENSITY))
    {
        stInfo.dwAlarmType = NET_TV_ALARM_PEOPLE_DENSITY_STATISTICS;
        stInfo.dwStatisticsType = NET_TV_STATISTICS_TYPE_PEOPLE_DENSITY;
    }
    else
    {
        return;
    }

    const size_t nTargetCount = std::min(stPayload.vecTargets.size(),
                                         static_cast<size_t>(NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM));
    stInfo.dwTargetCount = static_cast<UINT32>(nTargetCount);
    for (size_t i = 0; i < nTargetCount; ++i)
    {
        fill_statistics_target(stPayload.vecTargets[i], stInfo.stTargets[i]);
    }
    stInfo.stPanoramaImg = make_tvsdk_image_view(stPayload.stPanoramaImage);

    dlog_info("TVSDK统计告警填充: cmd[0x%x] 通道[%u] 类型[%u] 规则[%u] 时间戳[%lld] 序号[%u] "
              "进入[%u] 离开[%u] 总数[%u] 当前人数[%u] 平均停留[%u] 目标数[%u] buf_len[%zu]",
              stInfo.dwAlarmType,
              stInfo.dwChannel,
              stInfo.dwStatisticsType,
              stInfo.dwRuleID,
              static_cast<long long>(stInfo.llTimestampMs),
              stInfo.dwReportSeq,
              stInfo.dwEnterCount,
              stInfo.dwLeaveCount,
              stInfo.dwTotalCount,
              stInfo.dwCurrentPeopleCount,
              stInfo.dwAverageStayTimeSec,
              stInfo.dwTargetCount,
              sizeof(stInfo));
    const int nRet = ControlManage::instance()->tvsdk_push_alarm_v2(
        static_cast<int>(stInfo.dwAlarmType), &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送统计告警失败: cmd[0x%x] ret[%d]", stInfo.dwAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送统计告警成功: cmd[0x%x]", stInfo.dwAlarmType);
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
