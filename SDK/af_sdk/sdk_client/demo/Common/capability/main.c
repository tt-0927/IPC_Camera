/**
 * @file main.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief SDK客户端 设备能力集Demo
 *        支持终端输入命令码获取不同类型的设备能力集
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "NetSdkLog.h"
#include "NetTVSDKClientInterface.h"

/* 日志配置 */
#define MAX_LOG_SIZE  (20 * 1024 * 1024)
#define MAX_LOG_FILES (10)

/* 服务端配置 */
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8888
#define USERNAME    "admin"
#define PASSWORD    "Admin@123456"
#define CLIENT_OSD_ALIGN_MAX_NUM 8

static char g_serverIp[64] = SERVER_IP;
static INT32 g_serverPort = SERVER_PORT;
static char g_username[64] = USERNAME;
static char g_password[64] = PASSWORD;

/* 全局用户句柄 */
static LPVOID g_lpUserID = NULL;

static void CopyString(char* pDst, size_t dstSize, const char* pSrc)
{
    if (!pDst || dstSize == 0)
    {
        return;
    }

    pDst[0] = '\0';
    if (!pSrc)
    {
        return;
    }

    strncpy(pDst, pSrc, dstSize - 1);
    pDst[dstSize - 1] = '\0';
}

static void PrintUsage(const char* pProgram)
{
    printf("Usage: %s [server_ip] [port] [username] [password]\n",
           pProgram ? pProgram : "CapabilityClientDemo");
    printf("Example: %s 172.16.25.199 8888 admin Admin@123456\n",
           pProgram ? pProgram : "CapabilityClientDemo");
}

static void ConfigureByArgs(int argc, char* argv[])
{
    if (argc > 5)
    {
        PrintUsage(argv[0]);
    }

    if (argc > 1)
    {
        CopyString(g_serverIp, sizeof(g_serverIp), argv[1]);
    }

    if (argc > 2)
    {
        int port = atoi(argv[2]);
        if (port > 0)
        {
            g_serverPort = port;
        }
    }

    if (argc > 3)
    {
        CopyString(g_username, sizeof(g_username), argv[3]);
    }

    if (argc > 4)
    {
        CopyString(g_password, sizeof(g_password), argv[4]);
    }
}

/**
 * @brief 打印命令菜单
 */
void PrintMenu()
{
    printf("\n");
    printf("========== 设备能力集获取Demo ==========\n");
    printf("  命令码列表:\n");
    printf("    1 - NET_CAP_VIDEO_ENCODE (视频编码能力集)\n");
    printf("    2 - NET_CAP_OSD          (OSD参数能力集)\n");
    printf("    3 - NET_CAP_SMART        (智能能力集) [未实现]\n");
    printf("    5 - NET_CAP_IMAGE        (图像参数能力集) [未实现]\n");
    printf("    6 - NET_CAP_AUDIO        (音频能力集)\n");
    printf("    0 - 退出\n");
    printf("==========================================\n");
    printf("请输入命令码: ");
}

/**
 * @brief 打印分辨率支持的帧率数组
 */
static void PrintFrameRateList(const char* indent, const NET_VideoResolution_S* pResolution)
{
    int frameRateNum = pResolution->uFrameRateNum;
    if (frameRateNum < 0)
    {
        frameRateNum = 0;
    }
    if (frameRateNum > NET_VIDEO_FRAME_RATE_MAX_NUM)
    {
        frameRateNum = NET_VIDEO_FRAME_RATE_MAX_NUM;
    }

    printf("%s支持帧率fps数组(%d):", indent, frameRateNum);
    if (frameRateNum == 0)
    {
        printf(" 空\n");
        return;
    }

    for (int i = 0; i < frameRateNum; i++)
    {
        printf(" %.4g", pResolution->afFrameRate[i]);
    }
    printf("\n");
}

static void PrintEncodeComplexityList(const char* indent, const NET_VideoEncodeAbility_S* pAbility)
{
    int complexityNum = 0;

    if (!pAbility)
    {
        return;
    }

    complexityNum = pAbility->nEncodeComplexityNum;
    if (complexityNum < 0)
    {
        complexityNum = 0;
    }
    if (complexityNum > NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM)
    {
        complexityNum = NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM;
    }

    printf("%s编码复杂度列表(%d):", indent, complexityNum);
    for (int i = 0; i < complexityNum; ++i)
    {
        printf(" %d", pAbility->anEncodeComplexity[i]);
    }
    printf("\n");
}

/**
 * @brief 打印视频编码能力集信息
 */
void PrintVideoEncodeCap(const NET_VideoEncodeCap_S* pCap)
{
    printf("\n[Client] ===== 视频编码能力集 =====\n");
    printf("  码流数量: %d\n", pCap->uStreamCount);

    for (int i = 0; i < pCap->uStreamCount && i < NET_VIDEO_STREAM_MAX; i++)
    {
        const NET_VideoStreamCap_S* pStream = &pCap->astStreamCap[i];
        printf("\n  [码流 %d] StreamType=%d\n", i, pStream->uStreamType);
        printf("    是否支持复合流: %s\n", pStream->bSupportMultiStream ? "是" : "否");
        printf("    编码类型数: %d\n", pStream->uEncodeCapSize);
        printf("    EncodeTypeNum: %d\n", pStream->uEncodeTypeNum);
        printf("    EncodeAbilityNum: %d\n", pStream->uEncodeAbilityNum);
        printf("    I帧间隔范围: %d ~ %d\n", pStream->uIFrameIntervalMin, pStream->uIFrameIntervalMax);
        printf("    图像质量范围: %d ~ %d\n", pStream->stQuality.uMin, pStream->stQuality.uMax);
        printf("    码流平滑范围: %d ~ %d\n", pStream->stStreamSmooth.uMin, pStream->stStreamSmooth.uMax);
        printf("    支持的分辨率数量: %d\n", pStream->uResolutionNum);
        for (int r = 0; r < pStream->uResolutionNum && r < NET_RESOLUTION_NUM_MAX; r++)
        {
            printf("      分辨率[%d]: %dx%d (帧率范围: %.4g~%.4g fps, 码率范围: %d~%d kbps)\n", r,
                   pStream->astResolution[r].uWidth,
                   pStream->astResolution[r].uHeight,
                   pStream->astResolution[r].fFrameRateMin,
                   pStream->astResolution[r].fFrameRateMax,
                   pStream->astResolution[r].uBitRateMin,
                   pStream->astResolution[r].uBitRateMax);
            PrintFrameRateList("        ", &pStream->astResolution[r]);
        }

        for (int j = 0; j < pStream->uEncodeCapSize && j < NET_VIDEO_ENCODE_TYPE_MAX; j++)
        {
            const NET_VideoEncodeOption_S* pEncode = &pStream->astEncodeCap[j];
            printf("    [编码 %d] Id=%d, Codec=%d\n", j, pEncode->nId, pEncode->enVideoCodec);
            printf("      VideoType: %d\n", pEncode->enVideoType);
            printf("      分辨率: %dx%d\n",
                   pEncode->stVideoResolution.uWidth,
                   pEncode->stVideoResolution.uHeight);
            printf("      分辨率能力: 帧率范围=%.4g~%.4g fps, 码率范围=%d~%d kbps\n",
                   pEncode->stVideoResolution.fFrameRateMin,
                   pEncode->stVideoResolution.fFrameRateMax,
                   pEncode->stVideoResolution.uBitRateMin,
                   pEncode->stVideoResolution.uBitRateMax);
            PrintFrameRateList("      ", &pEncode->stVideoResolution);
            printf("      码率类型: %d\n", pEncode->enBitrateType);
            printf("      图像质量: %d\n", pEncode->enImageQuality);
            printf("      帧率: %d\n", pEncode->enFrameRate);
            printf("      码率: average=%d, upper=%d kbps\n",
                   pEncode->nAverageBitrate,
                   pEncode->nBitrateUpperLimit);
            printf("      智能编码: %s\n", pEncode->bSmartEnable ? "是" : "否");
            printf("      编码复杂度: %d\n", pEncode->enEncodingComplexity);
            printf("      I帧间隔: %d\n", pEncode->nIFrameInterval);
            printf("      SVC: %d\n", pEncode->enSvcEnable);
            printf("      码流平滑: %d\n", pEncode->nBitrateSmoothing);
        }

        for (int j = 0; j < pStream->uEncodeAbilityNum && j < NET_VIDEO_ENCODE_TYPE_MAX; j++)
        {
            const NET_VideoEncodeAbility_S* pAbility = &pStream->astEncodeAbility[j];
            printf("    [编码能力 %d] Codec=%s(%d)\n", j, pAbility->szVideoCodec, pAbility->enVideoCodec);
            printf("      支持调整复杂度: %d\n", pAbility->nSupportAdjustComplexity);
            PrintEncodeComplexityList("      ", pAbility);
            printf("      默认复杂度: %u\n", pAbility->nDefaultComplexity);
            printf("      支持SVC: %d\n", pAbility->bSupportSVC);
            printf("      支持码流平滑: %d\n", pAbility->bSupportStreamSmooth);
        }
    }
    printf("===================================\n");
}

static const char* AudioInputTypeToString(INT32 enType)
{
    switch (enType)
    {
    case NET_AUDIO_INPUT_MICIN:
        return "MICIN";
    case NET_AUDIO_INPUT_LINEIN:
        return "LINEIN";
    default:
        return "UNKNOWN";
    }
}

static const char* AudioOutputTypeToString(INT32 enType)
{
    switch (enType)
    {
    case NET_AUDIO_OUTPUT_SPEAKER:
        return "SPEAKER";
    case NET_AUDIO_OUTPUT_LINEOUT:
        return "LINEOUT";
    case NET_AUDIO_OUTPUT_MUTE:
        return "MUTE";
    default:
        return "UNKNOWN";
    }
}

static const char* AudioFormatToString(INT32 enFormat)
{
    switch (enFormat)
    {
    case NET_AUDIO_FORMAT_G722_1:
        return "G722_1";
    case NET_AUDIO_FORMAT_G711U:
        return "G711U";
    case NET_AUDIO_FORMAT_G711A:
        return "G711A";
    case NET_AUDIO_FORMAT_MP2L2:
        return "MP2L2";
    case NET_AUDIO_FORMAT_G726:
        return "G726";
    case NET_AUDIO_FORMAT_AAC:
        return "AAC";
    case NET_AUDIO_FORMAT_PCM:
        return "PCM";
    case NET_AUDIO_FORMAT_MP3:
        return "MP3";
    default:
        return "UNKNOWN";
    }
}

static const char* OsdFontSizeToString(UINT32 enValue)
{
    switch (enValue)
    {
    case NET_OSD_FONT_SIZE_ADAPTIVE:
        return "ADAPTIVE";
    case NET_OSD_FONT_SIZE_16:
        return "16x16";
    case NET_OSD_FONT_SIZE_32:
        return "32x32";
    case NET_OSD_FONT_SIZE_48:
        return "48x48";
    case NET_OSD_FONT_SIZE_64:
        return "64x64";
    default:
        return "UNKNOWN";
    }
}

static const char* OsdDateFormatToString(UINT32 enValue)
{
    switch (enValue)
    {
    case NET_OSD_DATE_YYYY_MM_DD:
        return "YYYY-MM-DD";
    case NET_OSD_DATE_MM_DD_YYYY:
        return "MM-DD-YYYY";
    case NET_OSD_DATE_DD_MM_YYYY:
        return "DD-MM-YYYY";
    case NET_OSD_DATE_YYYY_MM_DD_CHN:
        return "YYYY年MM月DD日";
    case NET_OSD_DATE_MM_DD_YYYY_CHN:
        return "MM月DD日YYYY年";
    case NET_OSD_DATE_DD_MM_YYYY_CHN:
        return "DD日MM月YYYY年";
    case NET_OSD_DATE_YYYY_MM_DD_SLASH:
        return "YYYY/MM/DD";
    case NET_OSD_DATE_MM_DD_YYYY_SLASH:
        return "MM/DD/YYYY";
    case NET_OSD_DATE_DD_MM_YYYY_SLASH:
        return "DD/MM/YYYY";
    default:
        return "UNKNOWN";
    }
}

static const char* OsdTimeFormatToString(UINT32 enValue)
{
    switch (enValue)
    {
    case NET_OSD_TIME_FORMAT_24:
        return "24H";
    case NET_OSD_TIME_FORMAT_12:
        return "12H";
    default:
        return "UNKNOWN";
    }
}

static const char* OsdAlignToString(UINT32 enValue)
{
    switch (enValue)
    {
    case NET_OSD_ALIGN_CUSTOMIZE:
        return "CUSTOMIZE";
    case NET_OSD_ALIGN_CHAR_LEFT:
        return "CHAR_LEFT";
    case NET_OSD_ALIGN_CHAR_RIGHT:
        return "CHAR_RIGHT";
    case NET_OSD_ALIGN_ALL_LEFT:
        return "ALL_LEFT";
    case NET_OSD_ALIGN_ALL_RIGHT:
        return "ALL_RIGHT";
    case NET_OSD_ALIGN_GB_MODE:
        return "GB_MODE";
    default:
        return "UNKNOWN";
    }
}

static UINT32 ClampUint32(UINT32 value, UINT32 maxValue)
{
    return (value > maxValue) ? maxValue : value;
}

static void PrintNamedUint32List(const char* pName,
                                 const UINT32* pList,
                                 UINT32 count,
                                 const char* (*toString)(UINT32))
{
    UINT32 i = 0;

    printf("  %s(%u):", pName, count);
    for (i = 0; pList && i < count; ++i)
    {
        printf(" %s(%u)", toString ? toString(pList[i]) : "UNKNOWN", pList[i]);
    }
    printf("\n");
}

/**
 * @brief 打印音频编码能力集信息
 */
void PrintAudioEncodeCap(const NET_AudioCap_S* pCap)
{
    if (pCap == NULL)
    {
        printf("\n[Client] 音频编码能力集为空\n");
        return;
    }

    printf("\n[Client] ===== 音频编码能力集 =====\n");

    printf(" 输入类型数量: %d\n", pCap->uInputTypeSize);
    printf(" 输入类型列表:");
    for (int i = 0; i < pCap->uInputTypeSize && i < NET_AUDIO_INPUT_TYPE_MAX; i++)
    {
        printf(" %s(%d)", AudioInputTypeToString(pCap->auInputType[i]), pCap->auInputType[i]);
    }
    printf("\n");

    printf(" 输出类型数量: %d\n", pCap->uOutputTypeSize);
    printf(" 输出类型列表:");
    for (int i = 0; i < pCap->uOutputTypeSize && i < NET_AUDIO_OUTPUT_TYPE_MAX; i++)
    {
        printf(" %s(%d)", AudioOutputTypeToString(pCap->auOutputType[i]), pCap->auOutputType[i]);
    }
    printf("\n");

    printf(" 音频格式数量: %d\n", pCap->uFormatSize);
    printf(" 音频格式列表:");
    for (int i = 0; i < pCap->uFormatSize && i < NET_AUDIO_FORMAT_MAX; i++)
    {
        printf(" %s(%d)", AudioFormatToString(pCap->auFormat[i]), pCap->auFormat[i]);
    }
    printf("\n");

    printf(" 音频格式详细能力数量: %d\n", pCap->uFormatDetailSize);

    for (int i = 0; i < pCap->uFormatDetailSize && i < NET_AUDIO_FORMAT_MAX; i++)
    {
        const NET_AudioFormatCap_S* pFmt = &pCap->astFormatDetail[i];

        printf("\n [格式能力 %d] Format=%s(%d)\n",
               i,
               AudioFormatToString(pFmt->uFormat),
               pFmt->uFormat);

        printf("    采样率数量: %d\n", pFmt->uSampleRateSize);
        printf("    采样率列表:");
        for (int j = 0; j < pFmt->uSampleRateSize && j < NET_AUDIO_SAMPRATE_MAX; j++)
        {
            printf(" %d", pFmt->auSampleRate[j]);
        }
        printf("\n");

        printf("    码率数量: %d\n", pFmt->uBitRateSize);
        printf("    码率列表:");
        for (int j = 0; j < pFmt->uBitRateSize && j < NET_AUDIO_BITRATE_MAX; j++)
        {
            printf(" %d", pFmt->auBitRate[j]);
        }
        printf("\n");

        printf("    采样率范围使能: %s\n", pFmt->stSampleRateRange.bEnable ? "是" : "否");
        printf("    采样率范围: min=%d max=%d step=%d\n",
               pFmt->stSampleRateRange.uMin,
               pFmt->stSampleRateRange.uMax,
               pFmt->stSampleRateRange.uStep);

        printf("    码率范围使能: %s\n", pFmt->stBitRateRange.bEnable ? "是" : "否");
        printf("    码率范围: min=%d max=%d step=%d\n",
               pFmt->stBitRateRange.uMin,
               pFmt->stBitRateRange.uMax,
               pFmt->stBitRateRange.uStep);
    }

    printf("========================================\n");
}

/**
 * @brief 打印OSD能力集信息
 */
void PrintOsdCap(const NET_OsdCap_S* pCap)
{
    UINT32 fontSizeNum = 0;
    UINT32 dateFormatNum = 0;
    UINT32 timeFormatNum = 0;
    UINT32 alignNum = 0;

    if (!pCap)
    {
        return;
    }

    fontSizeNum = ClampUint32(pCap->udwSupportedFontSizeNum, NET_OSD_FONT_SIZE_TYPE_MAX_NUM);
    dateFormatNum = ClampUint32(pCap->udwSupportedDateFormatNum, NET_OSD_DATE_FORMAT_MAX_NUM);
    timeFormatNum = ClampUint32(pCap->udwSupportedTimeFormatNum, NET_OSD_TIME_FORMAT_MAX_NUM);
    alignNum = ClampUint32(pCap->udwSupportedAlignNum, CLIENT_OSD_ALIGN_MAX_NUM);

    printf("\n[Client] ===== OSD能力集 =====\n");

    // 基础能力
    printf("  支持OSD: %s\n", pCap->bSupportOsd ? "是" : "否");
    printf("  支持通道名称: %s\n", pCap->bSupportName ? "是" : "否");
    printf("  支持时间: %s\n", pCap->bSupportTime ? "是" : "否");
    printf("  支持星期: %s\n", pCap->bSupportWeek ? "是" : "否");
    printf("  支持自定义颜色: %s\n", pCap->bSupportCustomColor ? "是" : "否");

    // 字符叠加能力
    printf("  最大字符叠加数量: %u\n", pCap->udwMaxOsdNum);

    // 字体大小能力
    printf("  字体大小数量: %u", pCap->udwSupportedFontSizeNum);
    if (fontSizeNum != pCap->udwSupportedFontSizeNum)
    {
        printf("，按数组上限截断为%u", fontSizeNum);
    }
    printf("\n");
    PrintNamedUint32List("字体大小列表",
                         pCap->audwSupportedFontSizeList,
                         fontSizeNum,
                         OsdFontSizeToString);

    // 日期格式能力
    printf("  日期格式数量: %u", pCap->udwSupportedDateFormatNum);
    if (dateFormatNum != pCap->udwSupportedDateFormatNum)
    {
        printf("，按数组上限截断为%u", dateFormatNum);
    }
    printf("\n");
    PrintNamedUint32List("日期格式列表",
                         pCap->audwSupportedDateFormatList,
                         dateFormatNum,
                         OsdDateFormatToString);

    // 时间格式能力
    printf("  时间格式数量: %u", pCap->udwSupportedTimeFormatNum);
    if (timeFormatNum != pCap->udwSupportedTimeFormatNum)
    {
        printf("，按数组上限截断为%u", timeFormatNum);
    }
    printf("\n");
    PrintNamedUint32List("时间格式列表",
                         pCap->audwSupportedTimeFormatList,
                         timeFormatNum,
                         OsdTimeFormatToString);

    // 对齐方式能力
    printf("  对齐方式数量: %u", pCap->udwSupportedAlignNum);
    if (alignNum != pCap->udwSupportedAlignNum)
    {
        printf("，按数组上限截断为%u", alignNum);
    }
    printf("\n");
    PrintNamedUint32List("对齐方式列表",
                         pCap->audwSupportedAlignList,
                         alignNum,
                         OsdAlignToString);

    printf("===================================\n");
}

/**
 * @brief 获取视频编码能力集
 */
BOOL GetVideoEncodeCap(INT32 dwChannelID)
{
    NET_VideoEncodeCap_S stCap;
    memset(&stCap, 0, sizeof(NET_VideoEncodeCap_S));

    INT32 dwBytesReturned = 0;

    printf("[Client] 正在获取视频编码能力集, channelID=%d ...\n", dwChannelID);

    BOOL bRet = NET_GetDeviceCapability(
        g_lpUserID,
        dwChannelID,
        NET_CAP_VIDEO_ENCODE,
        &stCap,
        sizeof(NET_VideoEncodeCap_S),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取视频编码能力集成功!\n");
        PrintVideoEncodeCap(&stCap);
        return TRUE;
    }
    else
    {
        printf("[Client] 获取视频编码能力集失败! Error=%d\n", NET_GetLastError());
        return FALSE;
    }
}

/**
 * @brief 获取音频编码能力集
 */
BOOL GetAudioEncodeCap(INT32 dwChannelID)
{
    NET_AudioCap_S stCap;
    memset(&stCap, 0, sizeof(NET_AudioCap_S));

    INT32 dwBytesReturned = 0;

    printf("[Client] 正在获取音频编码能力集, channelID=%d ...\n", dwChannelID);

    BOOL bRet = NET_GetDeviceCapability(
        g_lpUserID,
        dwChannelID,
        NET_CAP_AUDIO,
        &stCap,
        sizeof(NET_AudioCap_S),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取音频编码能力集成功!\n");
        PrintAudioEncodeCap(&stCap);
        return TRUE;
    }
    else
    {
        printf("[Client] 获取音频编码能力集失败! Error=%d\n", NET_GetLastError());
        return FALSE;
    }
}


/**
 * @brief 获取OSD能力集
 */
BOOL GetOsdCap(INT32 dwChannelID)
{
    NET_OsdCap_S stCap;
    memset(&stCap, 0, sizeof(NET_OsdCap_S));

    INT32 dwBytesReturned = 0;

    printf("[Client] 正在获取OSD能力集, channelID=%d, command=%d ...\n",
           dwChannelID,
           NET_CAP_OSD);

    BOOL bRet = NET_GetDeviceCapability(
        g_lpUserID,
        dwChannelID,
        NET_CAP_OSD,
        &stCap,
        sizeof(NET_OsdCap_S),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取OSD能力集成功! bytesReturned=%d\n", dwBytesReturned);
        PrintOsdCap(&stCap);
        return TRUE;
    }
    else
    {
        printf("[Client] 获取OSD能力集失败! Error=%d\n", NET_GetLastError());
        return FALSE;
    }
}

/**
 * @brief 处理用户命令
 */
void ProcessCommand(int cmd)
{
    INT32 dwChannelID = 1; // 默认通道号

    switch (cmd)
    {
        case 1: // NET_CAP_VIDEO_ENCODE
            GetVideoEncodeCap(dwChannelID);
            break;

        case 2: // NET_CAP_OSD
            GetOsdCap(dwChannelID);
            break;

        case 3: // NET_CAP_SMART
            printf("[Client] 该能力集类型暂未实现!\n");
            break;
        case 5: // NET_CAP_IMAGE
            printf("[Client] 该能力集类型暂未实现!\n");
            break;
        case 6: // NET_CAP_AUDIO
            GetAudioEncodeCap(dwChannelID);
            break;

        default:
            printf("[Client] 无效的命令码: %d\n", cmd);
            break;
    }
}

int main(int argc, char* argv[])
{
    printf("=============== SDK Client Capability Demo ================\n");
    ConfigureByArgs(argc, argv);

    /* 初始化日志 */
    initSdkLogBySize("CapabilityClientDemo", "/tmp/CapabilityClientDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    syncPrintf(1);
    setLogLevel(NETSDK_LOG_TRACE);

    /* 初始化SDK */
    printf("[Client] Initializing SDK...\n");
    if (!NET_Init())
    {
        printf("[Client] NET_Init FAILED!\n");
        return -1;
    }
    printf("[Client] SDK initialized.\n");

    /* 登录设备 */
    NET_DeviceLoginInfo_S struLoginInfo;
    NET_DeviceInfo_S struDeviceInfo;
    memset(&struLoginInfo, 0, sizeof(NET_DeviceLoginInfo_S));
    memset(&struDeviceInfo, 0, sizeof(NET_DeviceInfo_S));

    struLoginInfo.uPort = g_serverPort;
    strncpy(struLoginInfo.szIPAddr, g_serverIp, sizeof(struLoginInfo.szIPAddr) - 1);
    strncpy(struLoginInfo.szUserName, g_username, sizeof(struLoginInfo.szUserName) - 1);
    strncpy(struLoginInfo.szPassword, g_password, sizeof(struLoginInfo.szPassword) - 1);

    printf("[Client] Logging in to %s:%d, username=%s...\n",
           g_serverIp,
           g_serverPort,
           g_username);
    g_lpUserID = NET_Login(&struLoginInfo, &struDeviceInfo);

    if (!g_lpUserID)
    {
        printf("[Client] Login FAILED! Error=%d\n", NET_GetLastError());
        NET_Cleanup();
        return -1;
    }
    printf("[Client] Login SUCCESS! UserID=%p\n", g_lpUserID);

    /* 主循环 - 处理用户输入 */
    int cmd = -1;
    while (1)
    {
        PrintMenu();

        if (scanf("%d", &cmd) != 1)
        {
            // 清除输入缓冲
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("[Client] 输入无效，请输入数字!\n");
            continue;
        }

        if (cmd == 0)
        {
            printf("[Client] 退出程序...\n");
            break;
        }

        ProcessCommand(cmd);
    }

    /* 登出 */
    if (g_lpUserID)
    {
        NET_Logout(g_lpUserID);
        printf("[Client] Logged out.\n");
    }

    /* 清理SDK */
    NET_Cleanup();
    printf("[Client] SDK cleaned up. Bye!\n");

    return 0;
}
