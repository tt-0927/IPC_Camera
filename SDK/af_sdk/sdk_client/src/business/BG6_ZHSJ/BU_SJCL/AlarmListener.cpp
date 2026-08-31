/**
 * @file AlarmListener.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-25
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief 客户端告警管理类实现。
 * 功能说明：
 * 1. 建立 AlarmListen 长连接并解析 multipart 响应。
 * 2. 将 JSON 告警转换为 SDK 结构体并触发客户端回调。
 * 3. 管理抓拍图片缓冲区的回调期生命周期。
 *
 * @par 修改记录
 * 2026-08-28 qinjt：统一抓拍告警解析辅助函数的命名、注释和内部链接属性。
 * 2026-08-31 qinjt：修正监听线程停止、客户端中断和重连并发，避免旧线程访问失效对象。
 */

#include "AlarmListener.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "Json.h"
#include "BG6_ZHSJ/BU_SJCL/AlarmInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/RecordInfoConvert.h"
#include "DeviceInfoConvert.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace
{
/**
 * @brief 删除字符串首尾的 ASCII 空白字符。
 * @param [in] strValue 待处理字符串。
 * @return 删除首尾空白后的字符串。
 */
static std::string alarm_trim_ascii(const std::string& strValue)
{
    std::size_t uBegin = 0;
    while (uBegin < strValue.size() &&
           std::isspace(static_cast<unsigned char>(strValue[uBegin])))
    {
        ++uBegin;
    }

    std::size_t uEnd = strValue.size();
    while (uEnd > uBegin &&
           std::isspace(static_cast<unsigned char>(strValue[uEnd - 1])))
    {
        --uEnd;
    }

    return strValue.substr(uBegin, uEnd - uBegin);
}

/**
 * @brief 不区分大小写比较两个 ASCII 字符串。
 * @param [in] strLeft 左侧字符串。
 * @param [in] strRight 右侧字符串。
 * @return true 表示两个字符串相等，false 表示不相等。
 */
static bool alarm_equals_no_case(
    const std::string& strLeft,
    const std::string& strRight)
{
    if (strLeft.size() != strRight.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < strLeft.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(strLeft[i])) !=
            std::tolower(static_cast<unsigned char>(strRight[i])))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief 判断一个 ASCII 字符串是否包含另一个字符串，比较时忽略大小写。
 * @param [in] strHaystack 待搜索的字符串。
 * @param [in] strNeedle 待查找的字符串。
 * @return true 表示包含，false 表示不包含。
 */
static bool alarm_contains_no_case(
    const std::string& strHaystack,
    const std::string& strNeedle)
{
    if (strNeedle.empty())
    {
        return true;
    }

    const auto stIterator = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(), strNeedle.end(),
        [](char chLeft, char chRight) {
            return std::tolower(static_cast<unsigned char>(chLeft)) ==
                   std::tolower(static_cast<unsigned char>(chRight));
        });
    return stIterator != strHaystack.end();
}

/**
 * @brief 从 multipart 头部文本中提取指定字段值。
 * @param [in] strHeaders multipart 头部文本。
 * @param [in] strHeaderName 待读取的字段名称。
 * @param [out] strValue 输出字段值。
 * @return true 表示找到字段，false 表示未找到字段。
 */
static bool alarm_get_multipart_header_value(
    const std::string& strHeaders,
    const std::string& strHeaderName,
    std::string& strValue)
{
    std::size_t uPosition = 0;
    while (uPosition <= strHeaders.size())
    {
        std::size_t uLineEnd = strHeaders.find("\r\n", uPosition);
        if (uLineEnd == std::string::npos)
        {
            uLineEnd = strHeaders.size();
        }

        const std::string strLine = strHeaders.substr(
            uPosition, uLineEnd - uPosition);
        const std::size_t uColonPosition = strLine.find(':');
        if (uColonPosition != std::string::npos &&
            alarm_equals_no_case(
                alarm_trim_ascii(strLine.substr(0, uColonPosition)),
                strHeaderName))
        {
            strValue = alarm_trim_ascii(strLine.substr(uColonPosition + 1));
            return true;
        }

        if (uLineEnd == strHeaders.size())
        {
            break;
        }
        uPosition = uLineEnd + 2;
    }

    return false;
}

/**
 * @brief 解析 multipart 头部中的 Content-Length 字段。
 * @param [in] strHeaders multipart 头部文本。
 * @param [out] uContentLength 输出内容长度。
 * @return true 表示解析成功，false 表示字段缺失或格式非法。
 */
static bool alarm_parse_content_length(
    const std::string& strHeaders,
    std::size_t& uContentLength)
{
    std::string strValue;
    if (!alarm_get_multipart_header_value(
            strHeaders, "Content-Length", strValue) || strValue.empty())
    {
        return false;
    }

    std::size_t uResult = 0;
    for (char chDigit : strValue)
    {
        if (!std::isdigit(static_cast<unsigned char>(chDigit)))
        {
            return false;
        }

        const std::size_t uDigit = static_cast<std::size_t>(chDigit - '0');
        if (uResult > (std::numeric_limits<std::size_t>::max() - uDigit) / 10)
        {
            return false;
        }
        uResult = uResult * 10 + uDigit;
    }

    uContentLength = uResult;
    return true;
}

/**
 * @brief 删除 multipart 内容末尾的回车换行符。
 * @param [in,out] strValue 待处理的 multipart 内容。
 * @return 无返回值。
 */
static void alarm_trim_trailing_crlf(std::string& strValue)
{
    if (strValue.length() >= 2 &&
        strValue.substr(strValue.length() - 2) == "\r\n")
    {
        strValue.resize(strValue.length() - 2);
    }
}

/**
 * @brief 判断响应体是否已经构成完整 JSON 文档。
 * @param [in] strBody 待判断的 JSON 文本。
 * @return true 表示 JSON 完整且可解析，false 表示不完整或非法。
 */
static bool alarm_is_complete_json_body(const std::string& strBody)
{
    Json::Object* pRoot = Json::init(strBody);
    if (!pRoot)
    {
        return false;
    }

    Json::deinit(pRoot);
    return true;
}

/**
 * @brief 释放抓拍告警中由 JSON 转换器分配的图片缓冲区。
 * @param [in,out] stCaptureInfo 待释放图片指针的抓拍告警结构体。
 * @return 无返回值。
 * @note NET_ImageBuffer_S 不拥有内存，图片只保证在本次回调期间有效。
 */
static void alarm_release_capture_images(NET_AlarmCaptureInfo_S& stCaptureInfo)
{
    delete[] stCaptureInfo.stPanoramaImg.pData;
    stCaptureInfo.stPanoramaImg.pData = nullptr;
    stCaptureInfo.stPanoramaImg.uDataLen = 0;

    const UINT32 uCropCount = std::min(
        stCaptureInfo.uCropCount,
        static_cast<UINT32>(NET_CAPTURE_CROP_MAX_NUM));
    for (UINT32 i = 0; i < uCropCount; ++i)
    {
        delete[] stCaptureInfo.stCropImages[i].stImage.pData;
        stCaptureInfo.stCropImages[i].stImage.pData = nullptr;
        stCaptureInfo.stCropImages[i].stImage.uDataLen = 0;
    }
}
}

/**
 * @brief 创建告警监听器并保存设备连接参数。
 * @param [in] strHost 设备主机地址。
 * @param [in] nPort 设备 HTTP 服务端口。
 * @param [in] strUser 设备登录用户名。
 * @param [in] strPass 设备登录密码。
 * @return 无返回值。
 */
CAlarmListener::CAlarmListener(
    const std::string& strHost,
    int nPort,
    const std::string& strUser,
    const std::string& strPass)
    : m_strHost(strHost),
      m_nPort(nPort),
      m_strUsername(strUser),
      m_strPassword(strPass)
{

}

/**
 * @brief 销毁告警监听器并停止所有监听线程。
 * @param 无。
 * @return 无返回值。
 * @details 通过 Stop() 关闭 HTTP 长连接并回收可回收的线程资源。
 */
CAlarmListener::~CAlarmListener()
{
    Stop();
    NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] CAlarmListener destroyed, totalReconnects=%d", m_nReconnectCount.load());
}

/**
 * @brief 启动告警监听和连接健康监控线程。
 * @param [in] pUserHandle 用户登录句柄。
 * @param [in] strSessionId 当前登录会话标识。
 * @return true 表示线程启动成功，false 表示线程创建失败。
 * @details 该函数会先停止旧监听，再使用当前会话建立 AlarmListen 长连接。
 */
bool CAlarmListener::StartListen(
    void* pUserHandle,
    const std::string& strSessionId)
{
    /* 确保旧线程完全停止后再启动新监听线程。 */
    Stop();

    m_hUser = pUserHandle;
    /*
     * 通过统一接口更新会话标识，保证启动线程前后的会话状态访问遵循同一把互斥锁。
     */
    UpdateSessionId(strSessionId);
    m_bRunning = true;

    /* Stop 已经完整回收旧线程，因此此处可以安全复用线程对象。 */
    m_stThread = std::thread(&CAlarmListener::AlarmLoop, this);

    /* 启动健康监控线程，用于检测 read_timeout 失效导致的阻塞连接。 */
    m_lLastDataTimeMilliseconds.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    if (!m_bHealthMonitorRunning.exchange(true))
    {
        if (m_stHealthMonitorThread.joinable())
        {
            m_stHealthMonitorThread.join();
        }
        m_stHealthMonitorThread = std::thread(&CAlarmListener::HealthMonitorLoop, this);
    }

    NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p StartListen: session=%s, starting AlarmLoop+HealthMonitor",
                  m_hUser, strSessionId.c_str());
    return true;
}


/**
 * @brief 停止告警监听和连接健康监控线程。
 * @param 无。
 * @return 无返回值。
 * @details 函数会先停止健康监控，再关闭 HTTP 客户端连接，最后等待监听线程
 *          完整退出。监听线程访问当前对象成员，因此不能使用 detach() 绕过回收。
 */
void CAlarmListener::Stop()
{
    m_bRunning = false;

    /*
     * 先停止当前 HTTP 客户端，立即中断 AlarmLoop 中可能阻塞的网络读取。
     * 健康线程访问客户端也必须经过同一把锁，因此释放成员引用是安全的。
     */
    {
        std::lock_guard<std::mutex> stLock(m_stClientMutex);
        if (m_pClient)
        {
            m_pClient->stop();
        }
        /* 旧 AlarmLoop 持有的共享指针仍可保证客户端对象生命周期。 */
        m_pClient.reset();
    }

    /* 客户端请求已被中断，再回收健康监控线程，避免它继续访问成员对象。 */
    if (m_bHealthMonitorRunning.exchange(false))
    {
        if (m_stHealthMonitorThread.joinable())
        {
            if (std::this_thread::get_id() != m_stHealthMonitorThread.get_id())
            {
                m_stHealthMonitorThread.join();
            }
            else
            {
                m_stHealthMonitorThread.detach();
            }
        }
    }

    /*
     * stop() 会中断当前 HTTP 请求，随后 AlarmLoop 会检测 m_bRunning 并退出。
     * 必须 join 完整回收线程，禁止让仍访问 this 的线程脱离对象生命周期。
     */
    if (m_stThread.joinable())
    {
        if (std::this_thread::get_id() == m_stThread.get_id())
        {
            /*
             * 当前实现的停止调用均来自外部线程；该分支仅用于防止同线程 join
             * 触发标准库异常。调用方必须保证在该线程退出前不销毁监听器对象。
             */
            m_stThread.detach();
            NETSDK_LOG_MESSAGE_WARN(
                "[DIAG-ALARM] User-%p Stop called from AlarmLoop, detached self thread",
                m_hUser);
        }
        else
        {
            /* 等待 AlarmLoop 完整退出，确保 StartListen 可以安全复用线程对象。 */
            m_stThread.join();
            NETSDK_LOG_MESSAGE_INFO(
                "[DIAG-ALARM] User-%p Stopped: AlarmLoop joined normally, totalReconnects=%d",
                m_hUser, m_nReconnectCount.load());
        }
    }
}

/**
 * @brief 执行告警长连接读取、解析、回调和重连循环。
 * @param 无。
 * @return 无返回值。
 * @details 解析 multipart 响应中的 JSON 告警和心跳，处理会话过期状态，
 *          并在网络断开后使用最新会话标识重新连接。
 */
void CAlarmListener::AlarmLoop()
{
    std::string strBoundary;
    std::string strBuffer;

    /* 返回 false 表示当前 AlarmListen 应断开，例如客户端主动停止监听。 */
    auto fnDispatchAlarm = [&](const std::string& strJsonBody) -> bool
    {
        Json::Object* pRoot = Json::init(strJsonBody);
        if (!pRoot)
        {
            NETSDK_LOG_MESSAGE_WARN(
                "[DIAG-ALARM] User-%p JSON parse failed, len=%zu",
                m_hUser, strJsonBody.size());
            /* JSON 解析失败不是致命错误，继续处理后续数据。 */
            return true;
        }

        long long lCommand = 0;
        Json::get(pRoot, "Command", lCommand);

        std::string strEventType;
        Json::get(pRoot, "Event", strEventType);

        /* 过滤服务端心跳包，不将其传递给上层业务回调。 */
        std::string strMessageType;
        Json::get(pRoot, "type", strMessageType);
        if (strMessageType == "heartbeat")
        {
            m_nReceivedHeartbeatCount++;
            /* 心跳日志每三十条输出一次，避免长连接运行时刷屏。 */
            const int nHeartbeatCount = m_nReceivedHeartbeatCount.load();
            if (nHeartbeatCount % 30 == 0)
            {
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p heartbeat #%d, alarms=%d, conn_alive",
                              m_hUser, nHeartbeatCount, m_nReceivedAlarmCount.load());
            }
            Json::deinit(pRoot);
            return true;
        }
        m_nReceivedAlarmCount++;
        m_stLastAlarmTime = std::chrono::steady_clock::now();

        /* 提取入队时间戳并计算告警从服务端入队到客户端接收的延迟。 */
        long long lEnqueueTimestampMs = 0;
        Json::get(pRoot, "enqueue_ts", lEnqueueTimestampMs);
        if (lEnqueueTimestampMs > 0)
        {
            const long long lNowTimestampMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p alarm received: cmd=0x%llX, event=%s, e2e_delay_ms=%lld",
                          m_hUser, lCommand, strEventType.c_str(),
                          lNowTimestampMs - lEnqueueTimestampMs);
        }
        else
        {
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p alarm received: cmd=0x%llX, event=%s",
                          m_hUser, lCommand, strEventType.c_str());
        }

        if (strEventType == "ChannelStatus" || lCommand == NET_NOTIFY_CHANNEL_STATUS)
        {
            NET_ChannelInfo_S stChannelInfo = {0};
            bool bParseSuccess = false;

            if (auto* pChannelObject = Json::get(pRoot, "ChannelInfo"))
            {
                SDKConvert::deal(pChannelObject, stChannelInfo, true);
                bParseSuccess = true;
            }

            if (bParseSuccess && m_fnChannelStatusCallback)
            {
                m_fnChannelStatusCallback(&stChannelInfo, m_pChannelStatusUserData);
            }

            Json::deinit(pRoot);
            return true;
        }

        if (!m_fnAlarmCallback)
        {
            Json::deinit(pRoot);
            return true;
        }

        /* 与服务端一致按高字节识别基类，避免 0x6100 抓拍被识别为 0x6000 统计。 */
        const INT32 nAlarmBase = static_cast<INT32>(lCommand) & 0xFF00;

        NET_Alarmer_S stAlarmer = {0};
        if (auto* pAlarmerObject = Json::get(pRoot, "Alarmer"))
        {
            SDKConvert::deal(pAlarmerObject, stAlarmer, true);
        }

        Json::Object* pAlarmInfoObject = Json::get(pRoot, "AlarmInfo");
        if (!pAlarmInfoObject)
        {
            std::vector<char> aJsonBuffer(
                strJsonBody.begin(), strJsonBody.end());
            aJsonBuffer.push_back('\0');
            INT32 nCallbackBufferLength = static_cast<INT32>(
                aJsonBuffer.size() - 1);
            m_fnAlarmCallback(
                lCommand, &stAlarmer, aJsonBuffer.data(),
                &nCallbackBufferLength, m_pAlarmUserData);
            Json::deinit(pRoot);
            return true;
        }

        if (lCommand == NET_ALARM_FACE_COMPARE)
        {
            std::unique_ptr<NET_AlarmFaceCompareInfo_S> pstFaceCompareInfo(
                new NET_AlarmFaceCompareInfo_S());
            SDKConvert::deal(pAlarmInfoObject, *pstFaceCompareInfo, true);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(*pstFaceCompareInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)pstFaceCompareInfo.get(),
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (nAlarmBase == NET_ALARM_BASE_BASIC)
        {
            NET_AlarmBasicInfo_S stBasicInfo = {0};
            SDKConvert::deal(pAlarmInfoObject, stBasicInfo, true);
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p basic parsed: cmd=0x%llX, alarmType=0x%X, timestamp=%lld, panoramaLen=%u",
                          m_hUser,
                          lCommand,
                          stBasicInfo.uAlarmType,
                          static_cast<long long>(stBasicInfo.llTimestampMs),
                          stBasicInfo.uPanoramaImgLen);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(stBasicInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)&stBasicInfo,
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (nAlarmBase == NET_ALARM_BASE_RULE)
        {
            auto pstRuleInfo = std::make_unique<NET_AlarmRuleInfo_S>();
            SDKConvert::deal(pAlarmInfoObject, *pstRuleInfo, true);
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p rule parsed: cmd=0x%llX, alarmType=0x%X, channel=%u, rule=%u, "
                          "target=%u, objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, targetLen=%u, "
                          "jsonHasPanoramaB64=%d, jsonHasTargetB64=%d",
                          m_hUser,
                          lCommand,
                          pstRuleInfo->uAlarmType,
                          pstRuleInfo->uChannel,
                          pstRuleInfo->uRuleID,
                          pstRuleInfo->uTargetID,
                          pstRuleInfo->uObjectType,
                          static_cast<long long>(pstRuleInfo->llTimestampMs),
                          pstRuleInfo->nLeft,
                          pstRuleInfo->nTop,
                          pstRuleInfo->nRight,
                          pstRuleInfo->nBottom,
                          pstRuleInfo->uPanoramaImgLen,
                          pstRuleInfo->uTargetImgLen,
                          strJsonBody.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                          strJsonBody.find("TargetImgBase64") != std::string::npos ? 1 : 0);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(*pstRuleInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)pstRuleInfo.get(),
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (nAlarmBase == NET_ALARM_BASE_AI)
        {
            auto pstAiObjectInfo = std::make_unique<NET_AlarmAiObjectInfo_S>();
            SDKConvert::deal(pAlarmInfoObject, *pstAiObjectInfo, true);
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p ai object parsed: cmd=0x%llX, alarmType=0x%X, channel=%u, object=%s, "
                          "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, imgLen=%u, "
                          "jsonHasPanoramaB64=%d, jsonHasImgDataB64=%d",
                          m_hUser,
                          lCommand,
                          pstAiObjectInfo->uAlarmType,
                          pstAiObjectInfo->uChannel,
                          pstAiObjectInfo->strObjectID,
                          pstAiObjectInfo->uObjectType,
                          static_cast<long long>(pstAiObjectInfo->llTimestampMs),
                          pstAiObjectInfo->nLeft,
                          pstAiObjectInfo->nTop,
                          pstAiObjectInfo->nRight,
                          pstAiObjectInfo->nBottom,
                          pstAiObjectInfo->uPanoramaImgLen,
                          pstAiObjectInfo->uImgLen,
                          strJsonBody.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                          strJsonBody.find("ImgDataBase64") != std::string::npos ? 1 : 0);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(*pstAiObjectInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)pstAiObjectInfo.get(),
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (nAlarmBase == NET_ALARM_BASE_CAPTURE)
        {
            auto pstCaptureInfo = std::make_unique<NET_AlarmCaptureInfo_S>();
            SDKConvert::deal(pAlarmInfoObject, *pstCaptureInfo, true);
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p capture parsed: cmd=0x%llX, alarmType=0x%X, "
                          "channel=%u, captureType=%u, timestamp=%lld, panorama=%ux%u/%u, cropCount=%u, "
                          "faceAttrs={male=%d,age=%d,glasses=%d,beard=%d,mask=%d,emotion=%d}, "
                          "jsonHasPanoramaB64=%d, jsonHasCropB64=%d",
                          m_hUser,
                          lCommand,
                          pstCaptureInfo->uAlarmType,
                          pstCaptureInfo->uChannel,
                          pstCaptureInfo->uCaptureType,
                          (long long)pstCaptureInfo->llTimestampMs,
                          pstCaptureInfo->uPanoramaWidth,
                          pstCaptureInfo->uPanoramaHeight,
                          pstCaptureInfo->stPanoramaImg.uDataLen,
                          pstCaptureInfo->uCropCount,
                          pstCaptureInfo->stExtraInfo.bMale,
                          pstCaptureInfo->stExtraInfo.nAgeLabel,
                          pstCaptureInfo->stExtraInfo.bGlasses,
                          pstCaptureInfo->stExtraInfo.bBeard,
                          pstCaptureInfo->stExtraInfo.bMask,
                          pstCaptureInfo->stExtraInfo.nEmotionLabel,
                          strJsonBody.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                          strJsonBody.find("\"ImgBase64\"") != std::string::npos ? 1 : 0);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(*pstCaptureInfo));
            /* 图片指针仅保证在本次回调执行期间有效，避免回调方保存悬空指针。 */
            m_fnAlarmCallback(lCommand, &stAlarmer, (CHAR*)pstCaptureInfo.get(),
                 &nCallbackBufferLength, m_pAlarmUserData);
            alarm_release_capture_images(*pstCaptureInfo);
        }
        else if (nAlarmBase == NET_ALARM_BASE_TRAFFIC)
        {
            NET_AlarmPlateInfo_S stPlateInfo = {0};
            SDKConvert::deal(pAlarmInfoObject, stPlateInfo, true);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(stPlateInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)&stPlateInfo,
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (nAlarmBase == NET_ALARM_BASE_EXCEPTION)
        {
            NET_AlarmExceptionInfo_S stExceptionInfo = {0};
            SDKConvert::deal(pAlarmInfoObject, stExceptionInfo, true);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(stExceptionInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)&stExceptionInfo,
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (nAlarmBase == NET_ALARM_BASE_STATISTICS)
        {
            auto pstStatisticsInfo =
                std::make_unique<NET_AlarmStatisticsInfo_S>();
            SDKConvert::deal(pAlarmInfoObject, *pstStatisticsInfo, true);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(*pstStatisticsInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)pstStatisticsInfo.get(),
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else if (lCommand == NET_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            NET_RecordDownloadProgress_S stRecordProgressInfo = {0};
            SDKConvert::deal(
                pAlarmInfoObject, stRecordProgressInfo, true);
            INT32 nCallbackBufferLength = static_cast<INT32>(
                sizeof(stRecordProgressInfo));
            m_fnAlarmCallback(
                lCommand, &stAlarmer, (CHAR*)&stRecordProgressInfo,
                &nCallbackBufferLength, m_pAlarmUserData);
        }
        else
        {
            std::vector<char> aJsonBuffer(
                strJsonBody.begin(), strJsonBody.end());
            aJsonBuffer.push_back('\0');
            INT32 nCallbackBufferLength = static_cast<INT32>(
                aJsonBuffer.size() - 1);
            m_fnAlarmCallback(
                lCommand, &stAlarmer, aJsonBuffer.data(),
                &nCallbackBufferLength, m_pAlarmUserData);
        }

        Json::deinit(pRoot);
        return true;
    };

    int nLoopCount = 0;
    while (m_bRunning)
    {
        ++nLoopCount;
        const std::chrono::steady_clock::time_point stBeforeConnectTime =
            std::chrono::steady_clock::now();

        /*
         * 在锁内重建客户端。AlarmLoop 和 HealthMonitorLoop 通过共享指针
         * 持有各自的客户端对象，避免替换成员客户端时产生悬空引用。
         */
        std::shared_ptr<httplib::Client> pstLocalClient;
        {
            std::lock_guard<std::mutex> stLock(m_stClientMutex);
            /*
             * Stop() 可能已经在循环条件检查后将监听置为停止状态。
             * 在同一把客户端锁内再次确认，禁止停止流程之后重新创建连接。
             */
            if (!m_bRunning)
            {
                break;
            }

            m_pClient = std::make_shared<httplib::Client>(m_strHost, m_nPort);
            m_pClient->set_digest_auth(m_strUsername.c_str(), m_strPassword.c_str());
            m_pClient->set_read_timeout(75);
            m_pClient->set_keep_alive(true);
            pstLocalClient = m_pClient;
        }

        /* 重置当前连接的健康统计数据。 */
        m_nReceivedHeartbeatCount = 0;
        m_nReceivedAlarmCount = 0;
        m_bFirstDataReceived = false;
        m_stLastStatisticTime = std::chrono::steady_clock::time_point{};
        m_stConnectionStartTime = std::chrono::steady_clock::now();
        m_stLastAlarmTime = std::chrono::steady_clock::now();
        /* 更新时间戳，防止健康监控线程在连接建立阶段误判为假死。 */
        m_lLastDataTimeMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        /* 每次重连时读取最新的 sessionId，登录线程可能已更新该值。 */
        std::string strCurrentSessionId;
        {
            std::lock_guard<std::mutex> stLock(m_stSessionIdMutex);
            strCurrentSessionId = m_strSessionId;
        }

        m_nReconnectCount++;
        const std::string strUrl =
            std::string(NET_API_PATH_ALARMEVENT_LISTEN) +
            "?session_id=" + strCurrentSessionId;
        NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p [CONNECT #%d] url=%s, host=%s:%d",
                      m_hUser, m_nReconnectCount.load(), strUrl.c_str(),
                      m_strHost.c_str(), m_nPort);

        bool bGot401 = false;
        auto stResult = pstLocalClient->Get(
            strUrl.c_str(), [&](const httplib::Response& stResponse)
        {
            NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p [RESPONSE] status=%d, ct=%s",
                          m_hUser, stResponse.status,
                          stResponse.get_header_value("Content-Type").c_str());
            if (stResponse.status == 401)
            {
                bGot401 = true;
                /* 不读取 401 响应体，httplib 会将结果标记为 Canceled。 */
                return false;
            }
            if (stResponse.status != 200)
            {
                return false;
            }
            /* HTTP 200 响应表示长连接已经建立，更新健康时间戳。 */
            m_lLastDataTimeMilliseconds.store(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            const std::string strContentType =
                stResponse.get_header_value("Content-Type");
            const std::size_t uBoundaryPosition =
                strContentType.find("boundary=");
            if (uBoundaryPosition != std::string::npos)
            {
                strBoundary = "--" +
                    strContentType.substr(uBoundaryPosition + 9);
            }
            return true;
        }, [&](const char* pData, std::size_t uDataLength)
        {
            if (!m_bRunning)
            {
                return false;
            }

            const std::chrono::steady_clock::time_point stNow =
                std::chrono::steady_clock::now();
            m_lLastDataTimeMilliseconds.store(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    stNow.time_since_epoch()).count());
            m_bFirstDataReceived = true;

            strBuffer.append(pData, uDataLength);

            /* 每六十秒输出一次数据接收统计，避免日志过度增长。 */
            const long long lStatisticElapsedSeconds =
                std::chrono::duration_cast<std::chrono::seconds>(
                    stNow - m_stLastStatisticTime).count();
            if (m_stLastStatisticTime == std::chrono::steady_clock::time_point{} ||
                lStatisticElapsedSeconds >= 60)
            {
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p [DATA] buffer=%zu bytes, hb=%d, alarms=%d",
                              m_hUser, strBuffer.size(),
                              m_nReceivedHeartbeatCount.load(), m_nReceivedAlarmCount.load());
                m_stLastStatisticTime = stNow;
            }

            if (strBoundary.empty())
            {
                return true;
            }
            while (true)
            {
                const std::size_t uBoundaryOffset =
                    strBuffer.find(strBoundary);
                if (uBoundaryOffset == std::string::npos)
                {
                    if (strBuffer.size() > strBoundary.size())
                    {
                        strBuffer.erase(
                            0, strBuffer.size() - strBoundary.size());
                    }
                    break;
                }

                if (uBoundaryOffset > 0)
                {
                    strBuffer.erase(0, uBoundaryOffset);
                    continue;
                }

                if (strBuffer.size() < strBoundary.size() + 2)
                {
                    break;
                }

                if (strBuffer.compare(strBoundary.size(), 2, "--") == 0)
                {
                    strBuffer.erase(0, strBoundary.size() + 2);
                    break;
                }

                if (strBuffer.compare(strBoundary.size(), 2, "\r\n") != 0)
                {
                    strBuffer.erase(0, strBoundary.size());
                    continue;
                }

                const std::size_t uHeadersStart = strBoundary.size() + 2;
                const std::size_t uHeaderEnd = strBuffer.find(
                    "\r\n\r\n", uHeadersStart);
                if (uHeaderEnd == std::string::npos)
                {
                    break;
                }

                const std::string strHeaders = strBuffer.substr(
                    uHeadersStart, uHeaderEnd - uHeadersStart);
                const std::size_t uBodyStart = uHeaderEnd + 4;
                std::size_t uContentLength = 0;
                std::size_t uConsumeEnd = std::string::npos;
                std::string strBody;

                if (alarm_parse_content_length(strHeaders, uContentLength))
                {
                    if (uContentLength > strBuffer.size() - uBodyStart)
                    {
                        break;
                    }

                    strBody = strBuffer.substr(uBodyStart, uContentLength);
                    uConsumeEnd = uBodyStart + uContentLength;
                    if (strBuffer.size() >= uConsumeEnd + 2 &&
                        strBuffer.compare(uConsumeEnd, 2, "\r\n") == 0)
                    {
                        uConsumeEnd += 2;
                    }
                }
                else
                {
                    const std::size_t uNextBoundaryOffset =
                        strBuffer.find(strBoundary, uBodyStart);
                    if (uNextBoundaryOffset == std::string::npos)
                    {
                        if (!alarm_contains_no_case(
                                strHeaders, "application/json"))
                        {
                            break;
                        }

                        strBody = strBuffer.substr(uBodyStart);
                        alarm_trim_trailing_crlf(strBody);
                        if (!alarm_is_complete_json_body(strBody))
                        {
                            break;
                        }
                        uConsumeEnd = strBuffer.size();
                    }
                    else
                    {
                        strBody = strBuffer.substr(
                            uBodyStart, uNextBoundaryOffset - uBodyStart);
                        alarm_trim_trailing_crlf(strBody);
                        uConsumeEnd = uNextBoundaryOffset;
                    }
                }

                /* 处理 JSON 数据。 */
                if (alarm_contains_no_case(strHeaders, "application/json"))
                {
                    if (!fnDispatchAlarm(strBody))
                    {
                        /* 回调要求当前 AlarmListen 断开。 */
                        return false;
                    }
                }
                /* 图片附件由 JSON 中的 Base64 字段承载，此处无需单独处理。 */
                else if (alarm_contains_no_case(strHeaders, "image"))
                {
                }

                if (uConsumeEnd == std::string::npos)
                {
                    break;
                }
                strBuffer.erase(0, uConsumeEnd);
            }
            return true;
        });

        if (!m_bRunning)
        {
            break;
        }

        /* 长连接断开后记录连接时长、响应状态和当前会话信息。 */
        const long long lConnectionDurationSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() -
                m_stConnectionStartTime).count();
        const long long lTotalElapsedSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() -
                stBeforeConnectTime).count();

        if (stResult)
        {
            if (stResult->status == NET_HTTP_RESP_CODE_UNAUTHORIZED ||
                stResult->status == 401)
            {
                NETSDK_LOG_MESSAGE_ERROR(
                    "[DIAG-ALARM] User-%p [DISCONNECT] 401 Unauthorized: "
                    "conn=%llds, hb=%d, alarms=%d, triggering re-login",
                    m_hUser, lConnectionDurationSeconds,
                    m_nReceivedHeartbeatCount.load(),
                    m_nReceivedAlarmCount.load());
                if (m_fnSessionExpiredCallback)
                {
                    m_fnSessionExpiredCallback();
                }
                /* 由上层重新登录后重新启动 AlarmListen。 */
                break;
            }

            /* 记录服务端返回的其他 HTTP 错误，例如 500 或 503。 */
            NETSDK_LOG_MESSAGE_WARN(
                "[DIAG-ALARM] User-%p [DISCONNECT] HTTP status=%d: "
                "conn=%llds, hb=%d, alarms=%d",
                m_hUser, stResult->status, lConnectionDurationSeconds,
                m_nReceivedHeartbeatCount.load(),
                m_nReceivedAlarmCount.load());
        }
        else
        {
            /* 无响应通常表示服务端关闭 TCP 连接、网络中断或读取超时。 */
            const auto enError = stResult.error();

            /* 读取当前会话标识，便于诊断重连使用的会话是否发生变化。 */
            std::string strCurrentSessionIdLog;
            {
                std::lock_guard<std::mutex> stLock(m_stSessionIdMutex);
                strCurrentSessionIdLog = m_strSessionId;
            }

            /* 401 被 httplib 中止后可能表现为 Canceled，此时仍按会话过期处理。 */
            if (bGot401)
            {
                NETSDK_LOG_MESSAGE_ERROR(
                    "[DIAG-ALARM] User-%p [DISCONNECT] 401 Unauthorized "
                    "(Canceled): conn=%llds, hb=%d, alarms=%d, session=%s, "
                    "triggering re-login",
                    m_hUser, lConnectionDurationSeconds,
                    m_nReceivedHeartbeatCount.load(),
                    m_nReceivedAlarmCount.load(),
                    strCurrentSessionIdLog.c_str());
                if (m_fnSessionExpiredCallback)
                {
                    m_fnSessionExpiredCallback();
                }
                /* 由上层重新登录后重新启动 AlarmListen。 */
                break;
            }

            NETSDK_LOG_MESSAGE_WARN(
                "[DIAG-ALARM] User-%p [DISCONNECT] No response (err=%d): "
                "conn=%llds, total=%llds, hb=%d, alarms=%d, session=%s, "
                "reconnecting...",
                m_hUser, static_cast<int>(enError),
                lConnectionDurationSeconds, lTotalElapsedSeconds,
                m_nReceivedHeartbeatCount.load(),
                m_nReceivedAlarmCount.load(),
                strCurrentSessionIdLog.c_str());
        }

        /* 清空当前响应缓存和边界标识，避免下一次连接解析到旧数据。 */
        strBuffer.clear();
        strBoundary.clear();

        /* 统一等待半秒后再次建立监听连接。 */
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p AlarmLoop EXIT: totalLoops=%d, totalReconnects=%d",
                  m_hUser, nLoopCount, m_nReconnectCount.load());
}

/**
 * @brief 执行告警连接健康监控循环。
 * @param 无。
 * @return 无返回值。
 * @details 定时检查 AlarmLoop 是否收到数据；发现底层读取阻塞超过阈值时，
 *          仅中断当前 AlarmListen 连接，让监听循环重新建立连接。
 */
void CAlarmListener::HealthMonitorLoop()
{
    NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p HealthMonitor started: checkInterval=5s, "
                  "timeoutInitial=30s, timeoutDataLoss=60s",
                  m_hUser);

    while (m_bHealthMonitorRunning.load())
    {
        for (int i = 0;
             i < 5 && m_bHealthMonitorRunning.load();
             ++i)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!m_bHealthMonitorRunning.load())
        {
            break;
        }

        const long long lNowTimestampMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const long long lLastDataTimestampMilliseconds =
            m_lLastDataTimeMilliseconds.load();
        const long long lElapsedSeconds =
            (lLastDataTimestampMilliseconds > 0) ?
            (lNowTimestampMilliseconds - lLastDataTimestampMilliseconds) /
            1000 : 0;

        /* 分级判断连接超时：首次收不到数据等待三十秒，已有数据后等待六十秒。 */
        const long long lTimeoutSeconds = m_bFirstDataReceived.load() ? 60 : 30;

        if (lLastDataTimestampMilliseconds > 0 &&
            lElapsedSeconds > lTimeoutSeconds)
        {
            NETSDK_LOG_MESSAGE_ERROR("[DIAG-ALARM] User-%p [HEALTH] NO DATA for %llds (threshold=%llds, "
                          "hadData=%d). Connection is STUCK in recv(). "
                          "Requesting AlarmListen reconnect with the current session.",
                          m_hUser, lElapsedSeconds, lTimeoutSeconds,
                          static_cast<int>(m_bFirstDataReceived.load()));

            /* 仅中断当前 AlarmListen，外层循环会使用同一会话重新建立长连接；
             * 无数据不等于登录会话失效，因此不能触发 Basic/Login。 */
            {
                std::lock_guard<std::mutex> stLock(m_stClientMutex);
                if (m_pClient)
                {
                    m_pClient->stop();
                }
            }

            /* 防止当前 Get() 尚未退出时每个监控周期重复触发恢复；
             * stop() 生效后 AlarmLoop 会进入下一轮连接并刷新该时间戳。 */
            m_lLastDataTimeMilliseconds.store(lNowTimestampMilliseconds);
        }
    }
    NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] User-%p HealthMonitor EXIT", m_hUser);
}
