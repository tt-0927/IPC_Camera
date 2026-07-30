/**
 * @FilePath     : event_linkage_dict.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-18 17:41:33
 * @Description  : 事件联动字典与协议映射基础实现
 */

#include "event_linkage_dict.h"

#ifdef ENABLE_TVSDK_SRC
#include "control_manage.h"
#include "NetTVSDKServer.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <new>
#endif

#ifdef ENABLE_TVSDK_SRC
namespace
{
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
 * @brief   : 复制 TVSDK JPEG 图片负载到协议定长缓冲区
 * @param    {EventTvSdkImage_S} &stImage 图片负载
 * @param    {BYTE} *pDst 目标缓冲区
 * @param    {UINT32} &dwDstLen 目标长度字段
 * @param    {size_t} nMaxLen 目标缓冲区最大长度
 * @return   {bool} true：复制成功 false：图片超过协议上限
 */
bool copy_tvsdk_image(const EventTvSdkImage_S &stImage, BYTE *pDst, UINT32 &dwDstLen, size_t nMaxLen)
{
    dwDstLen = 0;
    if (stImage.vecJpeg.empty())
    {
        return true;
    }

    if (stImage.vecJpeg.size() > nMaxLen)
    {
        return false;
    }

    memcpy(pDst, stImage.vecJpeg.data(), stImage.vecJpeg.size());
    dwDstLen = static_cast<UINT32>(stImage.vecJpeg.size());
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
 * @brief   : 读取本地 JPEG 文件到 TVSDK 告警定长缓冲区
 * @param    {std::string} &strPath 图片路径
 * @param    {BYTE} *pDst 目标缓冲区
 * @param    {UINT32} &dwDstLen 目标长度字段
 * @param    {size_t} nMaxLen 目标缓冲区最大长度
 * @return   {void}
 */
void load_tvsdk_image_file(const std::string &strPath, BYTE *pDst, UINT32 &dwDstLen, size_t nMaxLen)
{
    dwDstLen = 0;
    if (strPath.empty() || !pDst)
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
    if (static_cast<size_t>(nSize) > nMaxLen)
    {
        dlog_warn("TVSDK人脸比对图片过大: %s, size[%lld], max[%zu]",
                  strPath.c_str(),
                  static_cast<long long>(nSize),
                  nMaxLen);
        return;
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(pDst), nSize);
    if (!file)
    {
        dlog_warn("TVSDK人脸比对图片读取失败: %s", strPath.c_str());
        return;
    }

    dwDstLen = static_cast<UINT32>(nSize);
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
    case Event::Type_E::DISK_FULL:   return NET_ALARM_DISK_FULL;
    case Event::Type_E::DISK_ERROR:  return NET_ALARM_DISK_ERROR;
    case Event::Type_E::NET_BROKEN:  return NET_ALARM_NET_BROKEN;
    case Event::Type_E::IP_CONFLICT: return NET_ALARM_IP_CONFLICT;
    default:                         return 0;
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
    case Event::Type_E::LOITERING_DETECT: return NET_ALARM_LOITERING;
    case Event::Type_E::PARKING_DETECT:   return NET_ALARM_PARKING_DETECT;
    default:                              return 0;
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
    std::unique_ptr<NET_AlarmBasicInfo_S> pInfo(new NET_AlarmBasicInfo_S());
    memset(pInfo.get(), 0, sizeof(*pInfo));
    pInfo->uAlarmType = dwAlarmType;
    pInfo->llTimestampMs = get_context_timestamp_ms(stContext);

    if (stContext.nChnId >= 0 && stContext.nChnId < NET_MAX_ALARM_IN_NUM)
    {
        pInfo->byChannel[stContext.nChnId] = 1;
    }

    if (stContext.pTvSdkPayload && !copy_tvsdk_image(stContext.pTvSdkPayload->stPanoramaImage,
                                                     pInfo->byPanoramaImg,
                                                     pInfo->uPanoramaImgLen,
                                                     sizeof(pInfo->byPanoramaImg)))
    {
        return;
    }

    int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->uAlarmType), pInfo.get(), sizeof(*pInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送普通告警失败: cmd[0x%x] ret[%d]", pInfo->uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送普通告警成功: cmd[0x%x] timestamp[%lld]",
                  pInfo->uAlarmType,
                  static_cast<long long>(pInfo->llTimestampMs));
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
    memset(pInfo.get(), 0, sizeof(*pInfo));
    pInfo->uAlarmType = dwAlarmType;
    pInfo->uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    pInfo->uRuleID = static_cast<UINT32>(std::max(0, get_context_attr_int(stContext, "rule_id", 0)));
    pInfo->uRuleType = pInfo->uAlarmType;
    pInfo->uTargetID = static_cast<UINT32>(std::max(0, stContext.nTargetId));
    pInfo->uObjectType = static_cast<UINT32>(std::max(0, stContext.nObjectType));
    pInfo->fConfidence = stContext.fConfidence;
    pInfo->nLeft = stContext.nLeft;
    pInfo->nTop = stContext.nTop;
    pInfo->nRight = stContext.nRight;
    pInfo->nBottom = stContext.nBottom;
    pInfo->llTimestampMs = get_context_timestamp_ms(stContext);

    const EventTvSdkImage_S &stPanoramaImage =
        stContext.stPanoramaImage.vecJpeg.empty() && stContext.pTvSdkPayload
            ? stContext.pTvSdkPayload->stPanoramaImage
            : stContext.stPanoramaImage;

    if (!copy_tvsdk_image(stPanoramaImage,
                          pInfo->byPanoramaImg,
                          pInfo->uPanoramaImgLen,
                          sizeof(pInfo->byPanoramaImg)))
    {
        dlog_warn("TVSDK周界告警全景图超过协议上限: cmd[0x%x]", pInfo->uAlarmType);
        return;
    }

    if (!copy_tvsdk_image(stContext.stTargetImage,
                          pInfo->byTargetImg,
                          pInfo->uTargetImgLen,
                          sizeof(pInfo->byTargetImg)))
    {
        dlog_warn("TVSDK周界告警特写图超过协议上限: cmd[0x%x]", pInfo->uAlarmType);
        return;
    }

    dlog_info("TVSDK周界告警内容: cmd[0x%x], event[%d], chn[%u], rule[%u], target[%u], objType[%u], timestamp[%lld], "
              "rect[%d,%d,%d,%d], contextPanorama[%zu], contextTarget[%zu], payload[%d], "
              "sendPanorama[%u], sendTarget[%u], structSize[%zu]",
              pInfo->uAlarmType,
              static_cast<int>(stContext.enEventType),
              pInfo->uChannel,
              pInfo->uRuleID,
              pInfo->uTargetID,
              pInfo->uObjectType,
              static_cast<long long>(pInfo->llTimestampMs),
              pInfo->nLeft,
              pInfo->nTop,
              pInfo->nRight,
              pInfo->nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size(),
              stContext.pTvSdkPayload ? 1 : 0,
              pInfo->uPanoramaImgLen,
              pInfo->uTargetImgLen,
              sizeof(*pInfo));

    int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->uAlarmType), pInfo.get(), sizeof(*pInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送周界告警失败: cmd[0x%x] ret[%d]", pInfo->uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送周界告警成功: cmd[0x%x]", pInfo->uAlarmType);
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
    std::unique_ptr<NET_AlarmAiObjectInfo_S> pInfo(new (std::nothrow) NET_AlarmAiObjectInfo_S);
    if (!pInfo)
    {
        dlog_warn("TVSDK AI目标告警内存分配失败，丢弃本次推送: buf_len[%zu]", sizeof(NET_AlarmAiObjectInfo_S));
        return;
    }

    memset(pInfo.get(), 0, sizeof(*pInfo));
    pInfo->uAlarmType = dwAlarmType;
    pInfo->uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
    pInfo->uObjectType = static_cast<UINT32>(std::max(0, stContext.nObjectType));
    pInfo->fConfidence = stContext.fConfidence;
    pInfo->nLeft = stContext.nLeft;
    pInfo->nTop = stContext.nTop;
    pInfo->nRight = stContext.nRight;
    pInfo->nBottom = stContext.nBottom;
    copy_tvsdk_string(pInfo->strObjectID, std::to_string(stContext.nTargetId));
    pInfo->llTimestampMs = get_context_timestamp_ms(stContext);

    const EventTvSdkImage_S &stPanoramaImage =
        stContext.stPanoramaImage.vecJpeg.empty() && stContext.pTvSdkPayload
            ? stContext.pTvSdkPayload->stPanoramaImage
            : stContext.stPanoramaImage;

    if (!copy_tvsdk_image(stPanoramaImage,
                          pInfo->byPanoramaImg,
                          pInfo->uPanoramaImgLen,
                          sizeof(pInfo->byPanoramaImg)))
    {
        dlog_warn("TVSDK AI目标告警全景图超过协议上限: cmd[0x%x]", pInfo->uAlarmType);
        return;
    }

    if (!copy_tvsdk_image(stContext.stTargetImage,
                          pInfo->byImgData,
                          pInfo->uImgLen,
                          sizeof(pInfo->byImgData)))
    {
        dlog_warn("TVSDK AI目标告警特写图超过协议上限: cmd[0x%x]", pInfo->uAlarmType);
        return;
    }

    dlog_info("TVSDK AI目标告警内容: cmd[0x%x], event[%d], chn[%u], target[%d], objType[%u], timestamp[%lld], "
              "rect[%d,%d,%d,%d], contextPanorama[%zu], contextTarget[%zu], payload[%d], "
              "sendPanorama[%u], sendTarget[%u], structSize[%zu]",
              pInfo->uAlarmType,
              static_cast<int>(stContext.enEventType),
              pInfo->uChannel,
              stContext.nTargetId,
              pInfo->uObjectType,
              static_cast<long long>(pInfo->llTimestampMs),
              pInfo->nLeft,
              pInfo->nTop,
              pInfo->nRight,
              pInfo->nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size(),
              stContext.pTvSdkPayload ? 1 : 0,
              pInfo->uPanoramaImgLen,
              pInfo->uImgLen,
              sizeof(*pInfo));

    int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->uAlarmType), pInfo.get(), sizeof(*pInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送AI目标告警失败: cmd[0x%x] ret[%d]", pInfo->uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送AI目标告警成功: cmd[0x%x]", pInfo->uAlarmType);
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
    std::unique_ptr<NET_AlarmFaceCompareInfo_S> pInfo(new (std::nothrow) NET_AlarmFaceCompareInfo_S);
    if (!pInfo)
    {
        dlog_warn("TVSDK人脸比对告警内存分配失败，丢弃本次推送: buf_len[%zu]", sizeof(NET_AlarmFaceCompareInfo_S));
        return;
    }

    memset(pInfo.get(), 0, sizeof(*pInfo));

    int nChannel = stContext.nChnId;
    if (nChannel < 0)
    {
        nChannel = stSrc.stInfo.nChnId;
    }
    if (nChannel < 0)
    {
        nChannel = 0;
    }

    pInfo->uAlarmType = NET_ALARM_FACE_COMPARE;
    pInfo->uChannel = static_cast<UINT32>(nChannel);
    pInfo->llTimestampMs = stSrc.stInfo.lTimestamp > 0 ? stSrc.stInfo.lTimestamp : stContext.llTimestamp;
    pInfo->nEventId = stSrc.nEventId;
    pInfo->nCompResult = stSrc.nCompResult;
    pInfo->nSimilarity = stSrc.nSimilarity;
    pInfo->nFaceId = stSrc.nFaceId;
    copy_tvsdk_string(pInfo->strFaceLibName, stSrc.strFaceLibName);
    copy_tvsdk_string(pInfo->strFaceName, stSrc.strFaceName);
    copy_tvsdk_string(pInfo->strLibFacePath, stSrc.strLibFacePath);
    copy_tvsdk_string(pInfo->strCapFacePath, stSrc.strCapFacePath);
    copy_tvsdk_string(pInfo->strCapImagePath, stSrc.strCapImagePath);
    load_tvsdk_image_file(stSrc.strLibFacePath,
                          pInfo->byLibFaceImg,
                          pInfo->uLibFaceImgLen,
                          sizeof(pInfo->byLibFaceImg));
    load_tvsdk_image_file(stSrc.strCapFacePath,
                          pInfo->byCapFaceImg,
                          pInfo->uCapFaceImgLen,
                          sizeof(pInfo->byCapFaceImg));

    dlog_info("TVSDK人脸比对告警填充: cmd[0x%x] 通道[%u] 事件ID[%d] 结果[%d] 相似度[%d] "
              "人脸ID[%d] 库[%s] 名称[%s] 库图长度[%u] 抓拍图长度[%u] buf_len[%zu]",
              pInfo->uAlarmType,
              pInfo->uChannel,
              pInfo->nEventId,
              pInfo->nCompResult,
              pInfo->nSimilarity,
              pInfo->nFaceId,
              pInfo->strFaceLibName,
              pInfo->strFaceName,
              pInfo->uLibFaceImgLen,
              pInfo->uCapFaceImgLen,
              sizeof(*pInfo));

    int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->uAlarmType), pInfo.get(), sizeof(*pInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送人脸比对告警失败: cmd[0x%x] ret[%d]", pInfo->uAlarmType, nRet);
    }
    else
    {
        dlog_info("TVSDK推送人脸比对告警成功: cmd[0x%x]", pInfo->uAlarmType);
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
    EventTvSdkImage_S stTargetImage;
    stTargetImage.vecJpeg = stSrc.vecJpeg;
    if (!copy_tvsdk_image(stTargetImage,
                          stDst.byImgData,
                          stDst.uImgLen,
                          sizeof(stDst.byImgData)))
    {
        stDst.uImgLen = 0;
    }
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

    try
    {
        const EventTvSdkStatisticsPayload_S &stPayload = stContext.pTvSdkPayload->stStatistics;
        std::unique_ptr<NET_AlarmStatisticsInfo_S> pInfo(new (std::nothrow) NET_AlarmStatisticsInfo_S);
        if (!pInfo)
        {
            dlog_warn("TVSDK统计告警内存分配失败，丢弃本次推送: buf_len[%zu]", sizeof(NET_AlarmStatisticsInfo_S));
            return;
        }

        memset(pInfo.get(), 0, sizeof(*pInfo));
        pInfo->uChannel = static_cast<UINT32>(stContext.nChnId < 0 ? 0 : stContext.nChnId);
        pInfo->uRuleID = static_cast<UINT32>(std::max(0, stPayload.nRuleId));
        pInfo->llTimestampMs = stPayload.llTimestampMs > 0 ? stPayload.llTimestampMs : stContext.llTimestamp;
        pInfo->uReportSeq = stPayload.nReportSeq;
        pInfo->uEnterCount = stPayload.nEnterCount;
        pInfo->uLeaveCount = stPayload.nLeaveCount;
        pInfo->uTotalCount = stPayload.nTotalCount;
        pInfo->uCurrentPeopleCount = stPayload.nCurrentPeopleCount;
        const UINT32 dwAverageStayTimeSec = static_cast<UINT32>(stPayload.nAverageStayTimeSec);
        pInfo->uAverageStayTimeSec = dwAverageStayTimeSec;

        if (stPayload.nStatisticsType == static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_FLOW))
        {
            pInfo->uAlarmType = NET_ALARM_PEOPLE_FLOW_STATISTICS;
            pInfo->uStatisticsType = NET_STATISTICS_TYPE_PEOPLE_FLOW;
        }
        else if (stPayload.nStatisticsType == static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_DENSITY))
        {
            pInfo->uAlarmType = NET_ALARM_PEOPLE_DENSITY_STATISTICS;
            pInfo->uStatisticsType = NET_STATISTICS_TYPE_PEOPLE_DENSITY;
        }
        else
        {
            return;
        }

        const size_t nTargetCount = std::min(stPayload.vecTargets.size(),
                                             static_cast<size_t>(NET_ALARM_STATISTICS_TARGET_MAX_NUM));
        pInfo->uTargetCount = static_cast<UINT32>(nTargetCount);
        for (size_t i = 0; i < nTargetCount; ++i)
        {
            fill_statistics_target(stPayload.vecTargets[i], pInfo->stTargets[i]);
        }

        if (!copy_tvsdk_image(stPayload.stPanoramaImage,
                              pInfo->byPanoramaImg,
                              pInfo->uPanoramaImgLen,
                              sizeof(pInfo->byPanoramaImg)))
        {
            dlog_warn("TVSDK统计告警全景图超过协议上限，丢弃图片但保留统计数据");
            pInfo->uPanoramaImgLen = 0;
        }

        dlog_info("TVSDK统计告警填充: cmd[0x%x] 通道[%u] 类型[%u] 规则[%u] 时间戳[%lld] 序号[%u] "
                  "进入[%u] 离开[%u] 总数[%u] 当前人数[%u] 平均停留[%u] 目标数[%u] buf_len[%zu]",
                  pInfo->uAlarmType,
                  pInfo->uChannel,
                  pInfo->uStatisticsType,
                  pInfo->uRuleID,
                  static_cast<long long>(pInfo->llTimestampMs),
                  pInfo->uReportSeq,
                  pInfo->uEnterCount,
                  pInfo->uLeaveCount,
                  pInfo->uTotalCount,
                  pInfo->uCurrentPeopleCount,
                  dwAverageStayTimeSec,
                  pInfo->uTargetCount,
                  sizeof(*pInfo));
        int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->uAlarmType), pInfo.get(), sizeof(*pInfo));
        if (nRet < 0)
        {
            dlog_warn("TVSDK推送统计告警失败: cmd[0x%x] ret[%d]", pInfo->uAlarmType, nRet);
        }
        else
        {
            dlog_info("TVSDK推送统计告警成功: cmd[0x%x]", pInfo->uAlarmType);
        }
    }
    catch (const std::bad_alloc &e)
    {
        dlog_warn("TVSDK统计告警内存不足，丢弃本次推送: %s", e.what());
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

    if (is_statistics_event(stContext.enEventType))
    {
        push_statistics_alarm(stContext);
    }
#endif
}
