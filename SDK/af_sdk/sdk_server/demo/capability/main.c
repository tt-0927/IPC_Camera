/**
 * @file main.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 * 
 * @brief SDK服务端 设备能力集Demo
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "NetSdkLog.h"
#include "NetTVSDKServerInterface.h"

/* 日志记录单个日志文件的最大大小 */
#define MAX_LOG_SIZE  (20 * 1024 * 1024)
/* 日志记录最大保留的日志文件数量 */
#define MAX_LOG_FILES (10)
/* 服务端口 */
#define SDKSERVER_PORT 8888

static void FillDemoResolution(LPNET_TV_VIDEO_RESOLUTION_S pResolution,
                               INT32 width,
                               INT32 height,
                               FLOAT frameRateMin,
                               FLOAT frameRateMax,
                               INT32 bitRateMin,
                               INT32 bitRateMax)
{
    static const FLOAT kFrameRates[] = {
        1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 2.0f,
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 8.3f,
        9.0f, 10.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f,
        18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f,
        26.0f, 27.0f, 28.0f, 29.0f, 30.0f
    };
    int i = 0;

    if (!pResolution)
    {
        return;
    }

    memset(pResolution, 0, sizeof(NET_TV_VIDEO_RESOLUTION_S));
    if (frameRateMin > frameRateMax)
    {
        FLOAT tmp = frameRateMin;
        frameRateMin = frameRateMax;
        frameRateMax = tmp;
    }

    pResolution->dwWidth = width;
    pResolution->dwHeight = height;
    pResolution->dwFrameRateMin = frameRateMin;
    pResolution->dwFrameRateMax = frameRateMax;
    pResolution->dwBitRateMin = bitRateMin;
    pResolution->dwBitRateMax = bitRateMax;

    for (i = 0; i < (int)(sizeof(kFrameRates) / sizeof(kFrameRates[0])) &&
                pResolution->dwFrameRateNum < NET_TV_VIDEO_FRAME_RATE_MAX_NUM; ++i)
    {
        FLOAT fps = kFrameRates[i];
        if (fps >= frameRateMin && fps <= frameRateMax)
        {
            pResolution->adwFrameRate[pResolution->dwFrameRateNum++] = kFrameRates[i];
        }
    }
}

/**
 * @brief 视频编码能力集回调实现
 * @note 模拟填充2个码流(主/子码流)的能力集数据
 */
NET_TV_COMMON_ECODE_E MyVideoEncodeCb(INT32 dwChannelID, LPNET_TV_VIDEO_ENCODE_CAP_S pCap)
{
    if (!pCap)
    {
        return NET_TV_E_INVALID_PARAM;
    }
    
    printf("[Server] GetVideoEncodeCap callback, channelID=%d\n", dwChannelID);

    memset(pCap, 0, sizeof(NET_TV_VIDEO_ENCODE_CAP_S));
    
    // 填充2个码流的能力集
    pCap->dwStreamCount = 2;
    
    // ============ 主码流能力 (索引0) ============
    pCap->astStreamCap[0].dwStreamType = 0;          // NET_TV_LIVE_STREAM_INDEX_MAIN
    pCap->astStreamCap[0].bSupportMultiStream = 1;   // 支持复合流
    pCap->astStreamCap[0].dwEncodeCapSize = 2;
    pCap->astStreamCap[0].stQuality.dwMin = 1;
    pCap->astStreamCap[0].stQuality.dwMax = 6;
    pCap->astStreamCap[0].stStreamSmooth.dwMin = 1;
    pCap->astStreamCap[0].stStreamSmooth.dwMax = 100;
    
    // H.264编码配置示例
    pCap->astStreamCap[0].astEncodeCap[0].nId = NET_TV_LIVE_STREAM_INDEX_MAIN;
    pCap->astStreamCap[0].astEncodeCap[0].enVideoType = 0;
    FillDemoResolution(&pCap->astStreamCap[0].astEncodeCap[0].stVideoResolution, 1920, 1080, 1.0f / 16.0f, 30.0f, 256, 8192);
    pCap->astStreamCap[0].astEncodeCap[0].enBitrateType = 0;
    pCap->astStreamCap[0].astEncodeCap[0].enImageQuality = 60;
    pCap->astStreamCap[0].astEncodeCap[0].enFrameRate = 30;
    pCap->astStreamCap[0].astEncodeCap[0].nBitrateUpperLimit = 8192;
    pCap->astStreamCap[0].astEncodeCap[0].nAverageBitrate = 4096;
    pCap->astStreamCap[0].astEncodeCap[0].enVideoCodec = NET_TV_VIDEO_CODE_H264;
    pCap->astStreamCap[0].astEncodeCap[0].bSmartEnable = FALSE;
    pCap->astStreamCap[0].astEncodeCap[0].enEncodingComplexity = 1;
    pCap->astStreamCap[0].astEncodeCap[0].nIFrameInterval = 50;
    pCap->astStreamCap[0].astEncodeCap[0].enSvcEnable = 1;
    pCap->astStreamCap[0].astEncodeCap[0].nBitrateSmoothing = 50;
    
    // H.265编码配置示例
    pCap->astStreamCap[0].astEncodeCap[1].nId = NET_TV_LIVE_STREAM_INDEX_MAIN;
    pCap->astStreamCap[0].astEncodeCap[1].enVideoType = 0;
    FillDemoResolution(&pCap->astStreamCap[0].astEncodeCap[1].stVideoResolution, 2560, 1440, 1.0f / 16.0f, 25.0f, 512, 16384);
    pCap->astStreamCap[0].astEncodeCap[1].enBitrateType = 0;
    pCap->astStreamCap[0].astEncodeCap[1].enImageQuality = 60;
    pCap->astStreamCap[0].astEncodeCap[1].enFrameRate = 25;
    pCap->astStreamCap[0].astEncodeCap[1].nBitrateUpperLimit = 16384;
    pCap->astStreamCap[0].astEncodeCap[1].nAverageBitrate = 8192;
    pCap->astStreamCap[0].astEncodeCap[1].enVideoCodec = NET_TV_VIDEO_CODE_H265;
    pCap->astStreamCap[0].astEncodeCap[1].bSmartEnable = FALSE;
    pCap->astStreamCap[0].astEncodeCap[1].enEncodingComplexity = 1;
    pCap->astStreamCap[0].astEncodeCap[1].nIFrameInterval = 50;
    pCap->astStreamCap[0].astEncodeCap[1].enSvcEnable = 0;
    pCap->astStreamCap[0].astEncodeCap[1].nBitrateSmoothing = 50;

    // 主码流支持的分辨率列表
    pCap->astStreamCap[0].dwResolutionNum = 4;
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[0], 2560, 1440, 1.0f / 16.0f, 25.0f, 512, 16384);
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[1], 1920, 1080, 1.0f / 16.0f, 30.0f, 256, 8192);
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[2], 1280, 720, 1.0f / 16.0f, 30.0f, 256, 4096);
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[3], 704, 576, 1.0f / 16.0f, 30.0f, 128, 2048);

    // ============ 子码流能力 (索引1) ============
    pCap->astStreamCap[1].dwStreamType = 1;          // NET_TV_LIVE_STREAM_INDEX_SUB
    pCap->astStreamCap[1].bSupportMultiStream = 0;   // 不支持复合流
    pCap->astStreamCap[1].dwEncodeCapSize = 1;
    pCap->astStreamCap[1].stQuality.dwMin = 1;
    pCap->astStreamCap[1].stQuality.dwMax = 6;
    pCap->astStreamCap[1].stStreamSmooth.dwMin = 1;
    pCap->astStreamCap[1].stStreamSmooth.dwMax = 50;
    
    // H.264编码配置示例
    pCap->astStreamCap[1].astEncodeCap[0].nId = NET_TV_LIVE_STREAM_INDEX_AUX;
    pCap->astStreamCap[1].astEncodeCap[0].enVideoType = 1;
    FillDemoResolution(&pCap->astStreamCap[1].astEncodeCap[0].stVideoResolution, 640, 480, 1.0f / 16.0f, 30.0f, 64, 1024);
    pCap->astStreamCap[1].astEncodeCap[0].enBitrateType = 0;
    pCap->astStreamCap[1].astEncodeCap[0].enImageQuality = 60;
    pCap->astStreamCap[1].astEncodeCap[0].enFrameRate = 15;
    pCap->astStreamCap[1].astEncodeCap[0].nBitrateUpperLimit = 1024;
    pCap->astStreamCap[1].astEncodeCap[0].nAverageBitrate = 512;
    pCap->astStreamCap[1].astEncodeCap[0].enVideoCodec = NET_TV_VIDEO_CODE_H264;
    pCap->astStreamCap[1].astEncodeCap[0].bSmartEnable = FALSE;
    pCap->astStreamCap[1].astEncodeCap[0].enEncodingComplexity = 1;
    pCap->astStreamCap[1].astEncodeCap[0].nIFrameInterval = 50;
    pCap->astStreamCap[1].astEncodeCap[0].enSvcEnable = 0;
    pCap->astStreamCap[1].astEncodeCap[0].nBitrateSmoothing = 50;

    // 子码流支持的分辨率列表
    pCap->astStreamCap[1].dwResolutionNum = 3;
    FillDemoResolution(&pCap->astStreamCap[1].astResolution[0], 704, 576, 1.0f / 16.0f, 30.0f, 128, 2048);
    FillDemoResolution(&pCap->astStreamCap[1].astResolution[1], 640, 480, 1.0f / 16.0f, 30.0f, 64, 1024);
    FillDemoResolution(&pCap->astStreamCap[1].astResolution[2], 352, 288, 1.0f / 16.0f, 30.0f, 64, 512);

    printf("[Server] Filled %d streams capability\n", pCap->dwStreamCount);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 音频编码能力集回调实现
 * @note 模拟填充音频编码能力集的能力集数据
 */
NET_TV_COMMON_ECODE_E MyAudioEncodeCb(INT32 dwChannelID, LPNET_TV_AUDIO_CAP_S pCap)
{
    if (!pCap)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    printf("[Server] GetAudioEncodeCap callback, channelID=%d\n", dwChannelID);

    memset(pCap, 0, sizeof(NET_TV_AUDIO_CAP_S));

    // ================= 输入类型能力 =================
    pCap->dwInputTypeSize = 2;
    pCap->adwInputType[0] = NET_TV_AUDIO_INPUT_MICIN;
    pCap->adwInputType[1] = NET_TV_AUDIO_INPUT_LINEIN;

    // ================= 输出类型能力 =================
    pCap->dwOutputTypeSize = 3;
    pCap->adwOutputType[0] = NET_TV_AUDIO_OUTPUT_SPEAKER;
    pCap->adwOutputType[1] = NET_TV_AUDIO_OUTPUT_LINEOUT;
    pCap->adwOutputType[2] = NET_TV_AUDIO_OUTPUT_MUTE;

    // ================= 音频格式能力 =================
    pCap->dwFormatSize = 4;
    pCap->adwFormat[0] = NET_TV_AUDIO_FORMAT_G711A;
    pCap->adwFormat[1] = NET_TV_AUDIO_FORMAT_G711U;
    pCap->adwFormat[2] = NET_TV_AUDIO_FORMAT_AAC;
    pCap->adwFormat[3] = NET_TV_AUDIO_FORMAT_MP3;

    // 格式详细能力数量
    pCap->dwFormatDetailSize = 4;

    // =========================================================
    // G711A 能力
    // =========================================================
    pCap->astFormatDetail[0].dwFormat = NET_TV_AUDIO_FORMAT_G711A;

    pCap->astFormatDetail[0].dwSampleRateSize = 1;
    pCap->astFormatDetail[0].adwSampleRate[0] = NET_TV_AUDIO_SAMPRATE_8000;

    pCap->astFormatDetail[0].dwBitRateSize = 1;
    pCap->astFormatDetail[0].adwBitRate[0] = NET_TV_AUDIO_BITRATE_64K;

    pCap->astFormatDetail[0].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[0].stSampleRateRange.dwMin = 8000;
    pCap->astFormatDetail[0].stSampleRateRange.dwMax = 8000;
    pCap->astFormatDetail[0].stSampleRateRange.dwStep = 0;

    pCap->astFormatDetail[0].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[0].stBitRateRange.dwMin = 64000;
    pCap->astFormatDetail[0].stBitRateRange.dwMax = 64000;
    pCap->astFormatDetail[0].stBitRateRange.dwStep = 0;

    // =========================================================
    // G711U 能力
    // =========================================================
    pCap->astFormatDetail[1].dwFormat = NET_TV_AUDIO_FORMAT_G711U;

    pCap->astFormatDetail[1].dwSampleRateSize = 1;
    pCap->astFormatDetail[1].adwSampleRate[0] = NET_TV_AUDIO_SAMPRATE_8000;

    pCap->astFormatDetail[1].dwBitRateSize = 1;
    pCap->astFormatDetail[1].adwBitRate[0] = NET_TV_AUDIO_BITRATE_64K;

    pCap->astFormatDetail[1].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[1].stSampleRateRange.dwMin = 8000;
    pCap->astFormatDetail[1].stSampleRateRange.dwMax = 8000;
    pCap->astFormatDetail[1].stSampleRateRange.dwStep = 0;

    pCap->astFormatDetail[1].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[1].stBitRateRange.dwMin = 64000;
    pCap->astFormatDetail[1].stBitRateRange.dwMax = 64000;
    pCap->astFormatDetail[1].stBitRateRange.dwStep = 0;

    // =========================================================
    // AAC 能力
    // =========================================================
    pCap->astFormatDetail[2].dwFormat = NET_TV_AUDIO_FORMAT_AAC;

    pCap->astFormatDetail[2].dwSampleRateSize = 4;
    pCap->astFormatDetail[2].adwSampleRate[0] = NET_TV_AUDIO_SAMPRATE_16000;
    pCap->astFormatDetail[2].adwSampleRate[1] = NET_TV_AUDIO_SAMPRATE_32000;
    pCap->astFormatDetail[2].adwSampleRate[2] = NET_TV_AUDIO_SAMPRATE_44100;
    pCap->astFormatDetail[2].adwSampleRate[3] = NET_TV_AUDIO_SAMPRATE_48000;

    pCap->astFormatDetail[2].dwBitRateSize = 5;
    pCap->astFormatDetail[2].adwBitRate[0] = NET_TV_AUDIO_BITRATE_48K;
    pCap->astFormatDetail[2].adwBitRate[1] = NET_TV_AUDIO_BITRATE_64K;
    pCap->astFormatDetail[2].adwBitRate[2] = NET_TV_AUDIO_BITRATE_96K;
    pCap->astFormatDetail[2].adwBitRate[3] = NET_TV_AUDIO_BITRATE_128K;
    pCap->astFormatDetail[2].adwBitRate[4] = NET_TV_AUDIO_BITRATE_256K;

    pCap->astFormatDetail[2].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[2].stSampleRateRange.dwMin = 16000;
    pCap->astFormatDetail[2].stSampleRateRange.dwMax = 48000;
    pCap->astFormatDetail[2].stSampleRateRange.dwStep = 0;

    pCap->astFormatDetail[2].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[2].stBitRateRange.dwMin = 48000;
    pCap->astFormatDetail[2].stBitRateRange.dwMax = 256000;
    pCap->astFormatDetail[2].stBitRateRange.dwStep = 0;

    // =========================================================
    // MP3 能力
    // =========================================================
    pCap->astFormatDetail[3].dwFormat = NET_TV_AUDIO_FORMAT_MP3;

    pCap->astFormatDetail[3].dwSampleRateSize = 3;
    pCap->astFormatDetail[3].adwSampleRate[0] = NET_TV_AUDIO_SAMPRATE_32000;
    pCap->astFormatDetail[3].adwSampleRate[1] = NET_TV_AUDIO_SAMPRATE_44100;
    pCap->astFormatDetail[3].adwSampleRate[2] = NET_TV_AUDIO_SAMPRATE_48000;

    pCap->astFormatDetail[3].dwBitRateSize = 6;
    pCap->astFormatDetail[3].adwBitRate[0] = NET_TV_AUDIO_BITRATE_32K;
    pCap->astFormatDetail[3].adwBitRate[1] = NET_TV_AUDIO_BITRATE_48K;
    pCap->astFormatDetail[3].adwBitRate[2] = NET_TV_AUDIO_BITRATE_64K;
    pCap->astFormatDetail[3].adwBitRate[3] = NET_TV_AUDIO_BITRATE_96K;
    pCap->astFormatDetail[3].adwBitRate[4] = NET_TV_AUDIO_BITRATE_128K;
    pCap->astFormatDetail[3].adwBitRate[5] = NET_TV_AUDIO_BITRATE_256K;

    pCap->astFormatDetail[3].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[3].stSampleRateRange.dwMin = 32000;
    pCap->astFormatDetail[3].stSampleRateRange.dwMax = 48000;
    pCap->astFormatDetail[3].stSampleRateRange.dwStep = 0;

    pCap->astFormatDetail[3].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[3].stBitRateRange.dwMin = 32000;
    pCap->astFormatDetail[3].stBitRateRange.dwMax = 256000;
    pCap->astFormatDetail[3].stBitRateRange.dwStep = 0;

    printf("[Server] Filled %d audio formats capability\n", pCap->dwFormatDetailSize);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief OSD能力集回调实现
 */
NET_TV_COMMON_ECODE_E MyOsdCapCb(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap)
{
    if (!pCap)
    {
        return NET_TV_E_INVALID_PARAM;
    }
    
    printf("[Server] GetOsdCap callback, channelID=%d\n", dwChannelID);
    
    // 基础能力
    pCap->bSupportOsd = TRUE;
    pCap->bSupportName = TRUE;
    pCap->bSupportTime = TRUE;
    pCap->bSupportWeek = TRUE;
    pCap->bSupportCustomColor = TRUE;
    
    // 字符叠加能力
    pCap->udwMaxOsdNum = NET_TV_OSD_CUSTOM_MAX_NUM;
    
    // 字体大小能力
    pCap->udwSupportedFontSizeNum = 5;
    pCap->audwSupportedFontSizeList[0] = NET_TV_OSD_FONT_SIZE_ADAPTIVE;
    pCap->audwSupportedFontSizeList[1] = NET_TV_OSD_FONT_SIZE_16;
    pCap->audwSupportedFontSizeList[2] = NET_TV_OSD_FONT_SIZE_32;
    pCap->audwSupportedFontSizeList[3] = NET_TV_OSD_FONT_SIZE_48;
    
    // 日期格式能力
    pCap->udwSupportedDateFormatNum = 9;
    pCap->audwSupportedDateFormatList[0] = NET_TV_OSD_DATE_YYYY_MM_DD;
    pCap->audwSupportedDateFormatList[1] = NET_TV_OSD_DATE_MM_DD_YYYY;
    pCap->audwSupportedDateFormatList[2] = NET_TV_OSD_DATE_DD_MM_YYYY;
    pCap->audwSupportedDateFormatList[3] = NET_TV_OSD_DATE_YYYY_MM_DD_CHN;
    pCap->audwSupportedDateFormatList[4] = NET_TV_OSD_DATE_MM_DD_YYYY_CHN;
    pCap->audwSupportedDateFormatList[5] = NET_TV_OSD_DATE_DD_MM_YYYY_CHN;
    pCap->audwSupportedDateFormatList[6] = NET_TV_OSD_DATE_YYYY_MM_DD_SLASH;
    pCap->audwSupportedDateFormatList[7] = NET_TV_OSD_DATE_MM_DD_YYYY_SLASH;
    pCap->audwSupportedDateFormatList[8] = NET_TV_OSD_DATE_DD_MM_YYYY_SLASH;
    
    // 时间格式能力
    pCap->udwSupportedTimeFormatNum = 2;
    pCap->audwSupportedTimeFormatList[0] = NET_TV_OSD_TIME_FORMAT_24;
    pCap->audwSupportedTimeFormatList[1] = NET_TV_OSD_TIME_FORMAT_12;
    
    // 对齐方式能力
    pCap->udwSupportedAlignNum = 6;
    pCap->audwSupportedAlignList[0] = NET_TV_OSD_ALIGN_CUSTOMIZE;
    pCap->audwSupportedAlignList[1] = NET_TV_OSD_ALIGN_CHAR_LEFT;
    pCap->audwSupportedAlignList[2] = NET_TV_OSD_ALIGN_CHAR_RIGHT;
    pCap->audwSupportedAlignList[3] = NET_TV_OSD_ALIGN_ALL_LEFT;
    pCap->audwSupportedAlignList[4] = NET_TV_OSD_ALIGN_ALL_RIGHT;
    pCap->audwSupportedAlignList[5] = NET_TV_OSD_ALIGN_GB_MODE;
    
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设备信息回调实现
 */
NET_TV_COMMON_ECODE_E MyDeviceInfoCb(LPNET_TV_DEVICE_INFO_S pInfo)
{
    if (!pInfo)
    {
        return NET_TV_E_INVALID_PARAM;
    }
    
    printf("[Server] GetDeviceInfo callback\n");
    
    // 填充设备信息
    pInfo->dwDevType = 0;           // 设备类型
    pInfo->wAlarmInPortNum = 4;     // 报警输入端口数
    pInfo->wAlarmOutPortNum = 2;    // 报警输出端口数
    pInfo->dwChannelNum = 4;        // 通道数
    
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 注册回调函数
 */
void AddRegisterCb()
{
    // 注册设备信息回调
    if (NET_TV_SERVER_RegisterCb_GetDeviceInfo(MyDeviceInfoCb))
    {
        printf("[Server] RegisterCb_GetDeviceInfo SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetDeviceInfo FAILED\n");
    }

    // 注册视频编码能力集回调
    if (NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(MyVideoEncodeCb))
    {
        printf("[Server] RegisterCb_GetVideoEncodeCap SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetVideoEncodeCap FAILED\n");
    }

    // 注册音频编码能力集回调
    if (NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(MyAudioEncodeCb))
    {
        printf("[Server] RegisterCb_GetAudioEncodeCap SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetAudioEncodeCap FAILED\n");
    }

    // 注册OSD能力集回调
    if (NET_TV_SERVER_RegisterCb_GetOsdCap(MyOsdCapCb))
    {
        printf("[Server] RegisterCb_GetOsdCap SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetOsdCap FAILED\n");
    }
}

int main()
{
    printf("=============== SDK Server Capability Demo ================\n");
    
    /* 初始化日志 */
    initSdkLogBySize("CapabilityDemo", "/opt/course/CapabilityDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    syncPrintf(1);
    setLogLevel(NETSDK_LOG_TRACE);
    
    /* 注册回调 */
    AddRegisterCb();
    
    /* 启动服务 */
    printf("[Server] Starting on port %d...\n", SDKSERVER_PORT);
    if (NET_TV_SERVER_Init(SDKSERVER_PORT, "admin", "Admin@123456"))
    {
        printf("[Server] Server started successfully!\n");
    }
    else
    {
        printf("[Server] Server start FAILED!\n");
        return -1;
    }
    
    printf("[Server] Waiting for client requests...\n");
    printf("Press Ctrl+C to stop.\n");
    
    while (1)
    {
        sleep(1);
    }
    
    NET_TV_SERVER_Cleanup();
    return 0;
}
