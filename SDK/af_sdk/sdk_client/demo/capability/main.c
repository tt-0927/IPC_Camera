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

/* 全局用户句柄 */
static LPVOID g_lpUserID = NULL;

/**
 * @brief 打印命令菜单
 */
void PrintMenu()
{
    printf("\n");
    printf("========== 设备能力集获取Demo ==========\n");
    printf("  命令码列表:\n");
    printf("    1 - NET_TV_CAP_VIDEO_ENCODE (视频编码能力集)\n");
    printf("    2 - NET_TV_CAP_OSD          (OSD参数能力集)\n");
    printf("    3 - NET_TV_CAP_SMART        (智能能力集) [未实现]\n");
    printf("    5 - NET_TV_CAP_IMAGE        (图像参数能力集) [未实现]\n");
    printf("    6 - NET_TV_CAP_AUDIO        (音频能力集)\n");
    printf("    0 - 退出\n");
    printf("==========================================\n");
    printf("请输入命令码: ");
}

/**
 * @brief 打印视频编码能力集信息
 */
void PrintVideoEncodeCap(const NET_TV_VIDEO_ENCODE_CAP_S* pCap)
{
    printf("\n[Client] ===== 视频编码能力集 =====\n");
    printf("  码流数量: %d\n", pCap->dwStreamCount);
    
    for (int i = 0; i < pCap->dwStreamCount && i < NET_TV_VIDEO_STREAM_MAX; i++)
    {
        const NET_TV_VIDEO_STREAM_CAP_S* pStream = &pCap->astStreamCap[i];
        printf("\n  [码流 %d] StreamType=%d\n", i, pStream->dwStreamType);
        printf("    是否支持复合流: %s\n", pStream->bSupportMultiStream ? "是" : "否");
        printf("    编码类型数: %d\n", pStream->dwEncodeCapSize);
        printf("    图像质量范围: %d ~ %d\n", pStream->stQuality.dwMin, pStream->stQuality.dwMax);
        printf("    码流平滑范围: %d ~ %d\n", pStream->stStreamSmooth.dwMin, pStream->stStreamSmooth.dwMax);
        
        for (int j = 0; j < pStream->dwEncodeCapSize && j < NET_TV_VIDEO_ENCODE_TYPE_MAX; j++)
        {
            const NET_TV_VIDEO_ENCODE_OPTION_S* pEncode = &pStream->astEncodeCap[j];
            printf("    [编码 %d] Id=%d, Codec=%d\n", j, pEncode->nId, pEncode->enVideoCodec);
            printf("      VideoType: %d\n", pEncode->enVideoType);
            printf("      分辨率: %dx%d\n",
                   pEncode->stVideoResolution.dwWidth,
                   pEncode->stVideoResolution.dwHeight);
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
    }
    printf("===================================\n");
}

static const char* AudioInputTypeToString(INT32 enType)
{
    switch (enType)
    {
    case NET_TV_AUDIO_INPUT_MICIN:
        return "MICIN";
    case NET_TV_AUDIO_INPUT_LINEIN:
        return "LINEIN";
    default:
        return "UNKNOWN";
    }
}

static const char* AudioOutputTypeToString(INT32 enType)
{
    switch (enType)
    {
    case NET_TV_AUDIO_OUTPUT_SPEAKER:
        return "SPEAKER";
    case NET_TV_AUDIO_OUTPUT_LINEOUT:
        return "LINEOUT";
    case NET_TV_AUDIO_OUTPUT_MUTE:
        return "MUTE";
    default:
        return "UNKNOWN";
    }
}

static const char* AudioFormatToString(INT32 enFormat)
{
    switch (enFormat)
    {
    case NET_TV_AUDIO_FORMAT_G722_1:
        return "G722_1";
    case NET_TV_AUDIO_FORMAT_G711U:
        return "G711U";
    case NET_TV_AUDIO_FORMAT_G711A:
        return "G711A";
    case NET_TV_AUDIO_FORMAT_MP2L2:
        return "MP2L2";
    case NET_TV_AUDIO_FORMAT_G726:
        return "G726";
    case NET_TV_AUDIO_FORMAT_AAC:
        return "AAC";
    case NET_TV_AUDIO_FORMAT_PCM:
        return "PCM";
    case NET_TV_AUDIO_FORMAT_MP3:
        return "MP3";
    default:
        return "UNKNOWN";
    }
}
/**
 * @brief 打印音频编码能力集信息
 */
void PrintAudioEncodeCap(const NET_TV_AUDIO_CAP_S* pCap)
{
    if (pCap == NULL)
    {
        printf("\n[Client] 音频编码能力集为空\n");
        return;
    }

    printf("\n[Client] ===== 音频编码能力集 =====\n");

    printf(" 输入类型数量: %d\n", pCap->dwInputTypeSize);
    printf(" 输入类型列表:");
    for (int i = 0; i < pCap->dwInputTypeSize && i < NET_TV_AUDIO_INPUT_TYPE_MAX; i++)
    {
        printf(" %s(%d)", AudioInputTypeToString(pCap->adwInputType[i]), pCap->adwInputType[i]);
    }
    printf("\n");

    printf(" 输出类型数量: %d\n", pCap->dwOutputTypeSize);
    printf(" 输出类型列表:");
    for (int i = 0; i < pCap->dwOutputTypeSize && i < NET_TV_AUDIO_OUTPUT_TYPE_MAX; i++)
    {
        printf(" %s(%d)", AudioOutputTypeToString(pCap->adwOutputType[i]), pCap->adwOutputType[i]);
    }
    printf("\n");

    printf(" 音频格式数量: %d\n", pCap->dwFormatSize);
    printf(" 音频格式列表:");
    for (int i = 0; i < pCap->dwFormatSize && i < NET_TV_AUDIO_FORMAT_MAX; i++)
    {
        printf(" %s(%d)", AudioFormatToString(pCap->adwFormat[i]), pCap->adwFormat[i]);
    }
    printf("\n");

    printf(" 音频格式详细能力数量: %d\n", pCap->dwFormatDetailSize);

    for (int i = 0; i < pCap->dwFormatDetailSize && i < NET_TV_AUDIO_FORMAT_MAX; i++)
    {
        const NET_TV_AUDIO_FORMAT_CAP_S* pFmt = &pCap->astFormatDetail[i];

        printf("\n [格式能力 %d] Format=%s(%d)\n",
               i,
               AudioFormatToString(pFmt->dwFormat),
               pFmt->dwFormat);

        printf("    采样率数量: %d\n", pFmt->dwSampleRateSize);
        printf("    采样率列表:");
        for (int j = 0; j < pFmt->dwSampleRateSize && j < NET_TV_AUDIO_SAMPRATE_MAX; j++)
        {
            printf(" %d", pFmt->adwSampleRate[j]);
        }
        printf("\n");

        printf("    码率数量: %d\n", pFmt->dwBitRateSize);
        printf("    码率列表:");
        for (int j = 0; j < pFmt->dwBitRateSize && j < NET_TV_AUDIO_BITRATE_MAX; j++)
        {
            printf(" %d", pFmt->adwBitRate[j]);
        }
        printf("\n");

        printf("    采样率范围使能: %s\n", pFmt->stSampleRateRange.bEnable ? "是" : "否");
        printf("    采样率范围: min=%d max=%d step=%d\n",
               pFmt->stSampleRateRange.dwMin,
               pFmt->stSampleRateRange.dwMax,
               pFmt->stSampleRateRange.dwStep);

        printf("    码率范围使能: %s\n", pFmt->stBitRateRange.bEnable ? "是" : "否");
        printf("    码率范围: min=%d max=%d step=%d\n",
               pFmt->stBitRateRange.dwMin,
               pFmt->stBitRateRange.dwMax,
               pFmt->stBitRateRange.dwStep);
    }

    printf("========================================\n");
}

/**
 * @brief 打印OSD能力集信息
 */
void PrintOsdCap(const NET_TV_OSD_CAP_S* pCap)
{
    printf("\n[Client] ===== OSD Ability =====\n");
    
    // 基础能力
    printf("  Support OSD: %s\n", pCap->bSupportOsd ? "Yes" : "No");
    printf("  Support Name: %s\n", pCap->bSupportName ? "Yes" : "No");
    printf("  Support Time: %s\n", pCap->bSupportTime ? "Yes" : "No");
    printf("  Support Week: %s\n", pCap->bSupportWeek ? "Yes" : "No");
    printf("  Support Custom Color: %s\n", pCap->bSupportCustomColor ? "Yes" : "No");
    
    // 字符叠加能力
    printf("  Max OSD Num: %d\n", pCap->udwMaxOsdNum);
    
    // 字体大小能力
    printf("  Supported Font Size Num: %d\n", pCap->udwSupportedFontSizeNum);
    
    // 日期格式能力
    printf("  Supported Date Format Num: %d\n", pCap->udwSupportedDateFormatNum);
    
    // 时间格式能力        
    printf("  Supported Time Format Num: %d\n", pCap->udwSupportedTimeFormatNum);
    
    // 对齐方式能力
    printf("  Supported Align Num: %d\n", pCap->udwSupportedAlignNum);

    printf("===================================\n");
}

/**
 * @brief 获取视频编码能力集
 */
BOOL GetVideoEncodeCap(INT32 dwChannelID)
{
    NET_TV_VIDEO_ENCODE_CAP_S stCap;
    memset(&stCap, 0, sizeof(NET_TV_VIDEO_ENCODE_CAP_S));
    
    INT32 dwBytesReturned = 0;
    
    printf("[Client] 正在获取视频编码能力集, channelID=%d ...\n", dwChannelID);
    
    BOOL bRet = NET_TV_GetDeviceCapability(
        g_lpUserID,
        dwChannelID,
        NET_TV_CAP_VIDEO_ENCODE,
        &stCap,
        sizeof(NET_TV_VIDEO_ENCODE_CAP_S),
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
        printf("[Client] 获取视频编码能力集失败! Error=%d\n", NET_TV_GetLastError());
        return FALSE;
    }
}

/**
 * @brief 获取音频编码能力集
 */
BOOL GetAudioEncodeCap(INT32 dwChannelID)
{
    NET_TV_AUDIO_CAP_S stCap;
    memset(&stCap, 0, sizeof(NET_TV_AUDIO_CAP_S));
    
    INT32 dwBytesReturned = 0;
    
    printf("[Client] 正在获取音频编码能力集, channelID=%d ...\n", dwChannelID);
    
    BOOL bRet = NET_TV_GetDeviceCapability(
        g_lpUserID,
        dwChannelID,
        NET_TV_CAP_AUDIO,
        &stCap,
        sizeof(NET_TV_AUDIO_CAP_S),
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
        printf("[Client] 获取音频编码能力集失败! Error=%d\n", NET_TV_GetLastError());
        return FALSE;
    }
}


/**
 * @brief 获取OSD能力集
 */
BOOL GetOsdCap(INT32 dwChannelID)
{
    NET_TV_OSD_CAP_S stCap;
    memset(&stCap, 0, sizeof(NET_TV_OSD_CAP_S));
    
    INT32 dwBytesReturned = 0;
    
    printf("[Client] Get OSD Cap, channelID=%d ...\n", dwChannelID);
    
    BOOL bRet = NET_TV_GetDeviceCapability(
        g_lpUserID,
        dwChannelID,
        NET_TV_CAP_OSD,
        &stCap,
        sizeof(NET_TV_OSD_CAP_S),
        &dwBytesReturned
    );
    
    if (bRet)
    {
        printf("[Client] Get OSD Cap Success!\n");
        PrintOsdCap(&stCap);
        return TRUE;
    }
    else
    {
        printf("[Client] Get OSD Cap Failed! Error=%d\n", NET_TV_GetLastError());
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
        case 1: // NET_TV_CAP_VIDEO_ENCODE
            GetVideoEncodeCap(dwChannelID);
            break;
            
        case 2: // NET_TV_CAP_OSD
            GetOsdCap(dwChannelID);
            break;
            
        case 3: // NET_TV_CAP_SMART
            printf("[Client] 该能力集类型暂未实现!\n");
            break;
        case 5: // NET_TV_CAP_IMAGE
            printf("[Client] 该能力集类型暂未实现!\n");
            break;
        case 6: // NET_TV_CAP_AUDIO
            GetAudioEncodeCap(dwChannelID);
            break;
            
        default:
            printf("[Client] 无效的命令码: %d\n", cmd);
            break;
    }
}

int main()
{
    printf("=============== SDK Client Capability Demo ================\n");
    
    /* 初始化日志 */
    initSdkLogBySize("CapabilityClientDemo", "/tmp/CapabilityClientDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    syncPrintf(1);
    setLogLevel(NETSDK_LOG_TRACE);
    
    /* 初始化SDK */
    printf("[Client] Initializing SDK...\n");
    if (!NET_TV_Init())
    {
        printf("[Client] NET_TV_Init FAILED!\n");
        return -1;
    }
    printf("[Client] SDK initialized.\n");
    
    /* 登录设备 */
    NET_TV_DEVICE_LOGIN_INFO_S struLoginInfo;
    NET_TV_DEVICE_INFO_S struDeviceInfo;
    memset(&struLoginInfo, 0, sizeof(NET_TV_DEVICE_LOGIN_INFO_S));
    memset(&struDeviceInfo, 0, sizeof(NET_TV_DEVICE_INFO_S));
    
    struLoginInfo.dwPort = SERVER_PORT;
    strncpy(struLoginInfo.szIPAddr, SERVER_IP, sizeof(struLoginInfo.szIPAddr) - 1);
    strncpy(struLoginInfo.szUserName, USERNAME, sizeof(struLoginInfo.szUserName) - 1);
    strncpy(struLoginInfo.szPassword, PASSWORD, sizeof(struLoginInfo.szPassword) - 1);
    
    printf("[Client] Logging in to %s:%d...\n", SERVER_IP, SERVER_PORT);
    g_lpUserID = NET_TV_Login(&struLoginInfo, &struDeviceInfo);
    
    if (!g_lpUserID)
    {
        printf("[Client] Login FAILED! Error=%d\n", NET_TV_GetLastError());
        NET_TV_Cleanup();
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
        NET_TV_Logout(g_lpUserID);
        printf("[Client] Logged out.\n");
    }
    
    /* 清理SDK */
    NET_TV_Cleanup();
    printf("[Client] SDK cleaned up. Bye!\n");
    
    return 0;
}
