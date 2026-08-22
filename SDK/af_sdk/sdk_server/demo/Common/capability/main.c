/**
 * @file main.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief SDK服务端 设备能力集Demo
 */
#include <stdio.h>
#include <stdlib.h>
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
/* 服务端默认账号 */
#define SDKSERVER_USERNAME "admin"
#define SDKSERVER_PASSWORD "Admin@123456"
/* Demo声明的最大字符叠加数量，需与当前IPC OSD能力保持一致 */
#define SDKSERVER_OSD_MAX_NUM NET_OSD_CUSTOM_MAX_NUM
#define SDKSERVER_OSD_ALIGN_MAX_NUM 8

static INT32 g_serverPort = SDKSERVER_PORT;
static char g_serverUsername[64] = SDKSERVER_USERNAME;
static char g_serverPassword[64] = SDKSERVER_PASSWORD;

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
    printf("Usage: %s [port] [username] [password]\n", pProgram ? pProgram : "CapabilityServerDemo");
    printf("Example: %s 8888 admin Admin@123456\n", pProgram ? pProgram : "CapabilityServerDemo");
}

static void ConfigureByArgs(int argc, char* argv[])
{
    if (argc > 4)
    {
        PrintUsage(argv[0]);
    }

    if (argc > 1)
    {
        int port = atoi(argv[1]);
        if (port > 0)
        {
            g_serverPort = port;
        }
    }

    if (argc > 2)
    {
        CopyString(g_serverUsername, sizeof(g_serverUsername), argv[2]);
    }

    if (argc > 3)
    {
        CopyString(g_serverPassword, sizeof(g_serverPassword), argv[3]);
    }
}

static UINT32 FillUint32List(UINT32* pDst, UINT32 dstCount, const UINT32* pSrc, UINT32 srcCount)
{
    UINT32 i = 0;
    UINT32 count = srcCount;

    if (!pDst || !pSrc || dstCount == 0)
    {
        return 0;
    }

    if (count > dstCount)
    {
        count = dstCount;
    }

    for (i = 0; i < count; ++i)
    {
        pDst[i] = pSrc[i];
    }

    return count;
}

static void PrintUint32List(const char* pName, const UINT32* pList, UINT32 count)
{
    UINT32 i = 0;

    printf("[Server]   %s(%u):", pName, count);
    for (i = 0; pList && i < count; ++i)
    {
        printf(" %u", pList[i]);
    }
    printf("\n");
}

static void FillDemoResolution(pNET_VideoResolution_S pResolution,
                               INT32 width,
                               INT32 height,
                               FLOAT frameRateMin,
                               FLOAT frameRateMax,
                               INT32 bitRateMin,
                               INT32 bitRateMax)
{
    static const FLOAT kFrameRates[] = {
        1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 2.0f,
        1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 15.0f,
        16.0f, 18.0f, 20.0f, 22.0f, 25.0f, 30.0f
    };
    int i = 0;

    if (!pResolution)
    {
        return;
    }

    memset(pResolution, 0, sizeof(NET_VideoResolution_S));
    snprintf(pResolution->szName, sizeof(pResolution->szName), "%d*%d", width, height);
    if (frameRateMin > frameRateMax)
    {
        FLOAT tmp = frameRateMin;
        frameRateMin = frameRateMax;
        frameRateMax = tmp;
    }

    pResolution->uWidth = width;
    pResolution->uHeight = height;
    pResolution->fFrameRateMin = frameRateMin;
    pResolution->fFrameRateMax = frameRateMax;
    pResolution->uBitRateMin = bitRateMin;
    pResolution->uBitRateMax = bitRateMax;

    for (i = 0; i < (int)(sizeof(kFrameRates) / sizeof(kFrameRates[0])) &&
                pResolution->uFrameRateNum < NET_VIDEO_FRAME_RATE_MAX_NUM; ++i)
    {
        FLOAT fps = kFrameRates[i];
        if (fps >= frameRateMin && fps <= frameRateMax)
        {
            pResolution->afFrameRate[pResolution->uFrameRateNum++] = kFrameRates[i];
        }
    }
}

static void FillDemoEncodeAbility(pNET_VideoEncodeAbility_S pAbility,
                                  const char* pCodec,
                                  INT32 enVideoCodec,
                                  INT32 supportAdjustComplexity,
                                  const INT32* pComplexity,
                                  INT32 complexityNum,
                                  UINT32 defaultComplexity,
                                  INT32 supportSvc,
                                  INT32 supportStreamSmooth)
{
    int i = 0;

    if (!pAbility)
    {
        return;
    }

    memset(pAbility, 0, sizeof(NET_VideoEncodeAbility_S));
    CopyString(pAbility->szVideoCodec, sizeof(pAbility->szVideoCodec), pCodec);
    pAbility->enVideoCodec = enVideoCodec;
    pAbility->nSupportAdjustComplexity = supportAdjustComplexity;
    pAbility->nEncodeComplexityNum = complexityNum;
    if (pAbility->nEncodeComplexityNum < 0)
    {
        pAbility->nEncodeComplexityNum = 0;
    }
    if (pAbility->nEncodeComplexityNum > NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM)
    {
        pAbility->nEncodeComplexityNum = NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM;
    }

    for (i = 0; pComplexity && i < pAbility->nEncodeComplexityNum; ++i)
    {
        pAbility->anEncodeComplexity[i] = pComplexity[i];
    }

    pAbility->nDefaultComplexity = defaultComplexity;
    pAbility->bSupportSVC = supportSvc;
    pAbility->bSupportStreamSmooth = supportStreamSmooth;
}

/**
 * @brief 视频编码能力集回调实现
 * @note 模拟填充2个码流(主/子码流)的能力集数据
 */
NET_COMMON_ECODE_E MyVideoEncodeCb(INT32 dwChannelID, pNET_VideoEncodeCap_S pCap)
{
    if (!pCap)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[Server] GetVideoEncodeCap callback, channelID=%d\n", dwChannelID);

    memset(pCap, 0, sizeof(NET_VideoEncodeCap_S));

    // 填充2个码流的能力集
    pCap->uStreamCount = 2;

    // ============ 主码流能力 (索引0) ============
    pCap->astStreamCap[0].uStreamType = 0;          // NET_LIVE_STREAM_INDEX_MAIN
    pCap->astStreamCap[0].bSupportMultiStream = 1;   // 支持复合流
    pCap->astStreamCap[0].uEncodeCapSize = 3;
    pCap->astStreamCap[0].uEncodeTypeNum = 3;
    pCap->astStreamCap[0].uEncodeAbilityNum = 3;
    pCap->astStreamCap[0].uIFrameIntervalMin = 1;
    pCap->astStreamCap[0].uIFrameIntervalMax = 400;
    pCap->astStreamCap[0].stQuality.uMin = 1;
    pCap->astStreamCap[0].stQuality.uMax = 100;
    pCap->astStreamCap[0].stStreamSmooth.uMin = 1;
    pCap->astStreamCap[0].stStreamSmooth.uMax = 100;
    {
        const INT32 complexityAll[] = {0, 1, 2};
        const INT32 complexityMain[] = {1};
        FillDemoEncodeAbility(&pCap->astStreamCap[0].astEncodeAbility[0], "H.264", NET_VIDEO_CODE_H264, 1, complexityAll, 3, 0, 1, 1);
        FillDemoEncodeAbility(&pCap->astStreamCap[0].astEncodeAbility[1], "H.265", NET_VIDEO_CODE_H265, 0, complexityMain, 1, 1, 1, 1);
        FillDemoEncodeAbility(&pCap->astStreamCap[0].astEncodeAbility[2], "MJPEG", NET_VIDEO_CODE_MJPEG, 0, complexityMain, 1, 1, 0, 0);
    }

    // H.264编码配置示例
    pCap->astStreamCap[0].astEncodeCap[0].nId = NET_LIVE_STREAM_INDEX_MAIN;
    pCap->astStreamCap[0].astEncodeCap[0].enVideoType = 0;
    FillDemoResolution(&pCap->astStreamCap[0].astEncodeCap[0].stVideoResolution, 1920, 1080, 1.0f / 16.0f, 30.0f, 256, 8192);
    pCap->astStreamCap[0].astEncodeCap[0].enBitrateType = 0;
    pCap->astStreamCap[0].astEncodeCap[0].enImageQuality = 60;
    pCap->astStreamCap[0].astEncodeCap[0].enFrameRate = 30;
    pCap->astStreamCap[0].astEncodeCap[0].nBitrateUpperLimit = 8192;
    pCap->astStreamCap[0].astEncodeCap[0].nAverageBitrate = 4096;
    pCap->astStreamCap[0].astEncodeCap[0].enVideoCodec = NET_VIDEO_CODE_H264;
    pCap->astStreamCap[0].astEncodeCap[0].bSmartEnable = FALSE;
    pCap->astStreamCap[0].astEncodeCap[0].enEncodingComplexity = 1;
    pCap->astStreamCap[0].astEncodeCap[0].nIFrameInterval = 50;
    pCap->astStreamCap[0].astEncodeCap[0].enSvcEnable = 1;
    pCap->astStreamCap[0].astEncodeCap[0].nBitrateSmoothing = 50;

    // H.265编码配置示例
    pCap->astStreamCap[0].astEncodeCap[1].nId = NET_LIVE_STREAM_INDEX_MAIN;
    pCap->astStreamCap[0].astEncodeCap[1].enVideoType = 0;
    FillDemoResolution(&pCap->astStreamCap[0].astEncodeCap[1].stVideoResolution, 2560, 1440, 1.0f / 16.0f, 25.0f, 512, 16384);
    pCap->astStreamCap[0].astEncodeCap[1].enBitrateType = 0;
    pCap->astStreamCap[0].astEncodeCap[1].enImageQuality = 60;
    pCap->astStreamCap[0].astEncodeCap[1].enFrameRate = 25;
    pCap->astStreamCap[0].astEncodeCap[1].nBitrateUpperLimit = 16384;
    pCap->astStreamCap[0].astEncodeCap[1].nAverageBitrate = 8192;
    pCap->astStreamCap[0].astEncodeCap[1].enVideoCodec = NET_VIDEO_CODE_H265;
    pCap->astStreamCap[0].astEncodeCap[1].bSmartEnable = FALSE;
    pCap->astStreamCap[0].astEncodeCap[1].enEncodingComplexity = 1;
    pCap->astStreamCap[0].astEncodeCap[1].nIFrameInterval = 50;
    pCap->astStreamCap[0].astEncodeCap[1].enSvcEnable = 0;
    pCap->astStreamCap[0].astEncodeCap[1].nBitrateSmoothing = 50;

    // MJPEG编码配置示例
    pCap->astStreamCap[0].astEncodeCap[2].nId = NET_LIVE_STREAM_INDEX_MAIN;
    pCap->astStreamCap[0].astEncodeCap[2].enVideoType = 1;
    FillDemoResolution(&pCap->astStreamCap[0].astEncodeCap[2].stVideoResolution, 1920, 1080, 1.0f / 16.0f, 30.0f, 256, 8192);
    pCap->astStreamCap[0].astEncodeCap[2].enBitrateType = 0;
    pCap->astStreamCap[0].astEncodeCap[2].enImageQuality = 60;
    pCap->astStreamCap[0].astEncodeCap[2].enFrameRate = 25;
    pCap->astStreamCap[0].astEncodeCap[2].nBitrateUpperLimit = 8192;
    pCap->astStreamCap[0].astEncodeCap[2].nAverageBitrate = 4096;
    pCap->astStreamCap[0].astEncodeCap[2].enVideoCodec = NET_VIDEO_CODE_MJPEG;
    pCap->astStreamCap[0].astEncodeCap[2].bSmartEnable = FALSE;
    pCap->astStreamCap[0].astEncodeCap[2].enEncodingComplexity = 1;
    pCap->astStreamCap[0].astEncodeCap[2].nIFrameInterval = 50;
    pCap->astStreamCap[0].astEncodeCap[2].enSvcEnable = 0;
    pCap->astStreamCap[0].astEncodeCap[2].nBitrateSmoothing = 0;

    // 主码流支持的分辨率列表
    pCap->astStreamCap[0].uResolutionNum = 4;
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[0], 2560, 1440, 1.0f / 16.0f, 25.0f, 512, 16384);
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[1], 1920, 1080, 1.0f / 16.0f, 30.0f, 256, 8192);
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[2], 1280, 720, 1.0f / 16.0f, 30.0f, 256, 4096);
    FillDemoResolution(&pCap->astStreamCap[0].astResolution[3], 704, 576, 1.0f / 16.0f, 30.0f, 128, 2048);

    // ============ 子码流能力 (索引1) ============
    pCap->astStreamCap[1].uStreamType = 1;          // NET_LIVE_STREAM_INDEX_SUB
    pCap->astStreamCap[1].bSupportMultiStream = 0;   // 不支持复合流
    pCap->astStreamCap[1].uEncodeCapSize = 2;
    pCap->astStreamCap[1].uEncodeTypeNum = 2;
    pCap->astStreamCap[1].uEncodeAbilityNum = 2;
    pCap->astStreamCap[1].uIFrameIntervalMin = 1;
    pCap->astStreamCap[1].uIFrameIntervalMax = 400;
    pCap->astStreamCap[1].stQuality.uMin = 1;
    pCap->astStreamCap[1].stQuality.uMax = 100;
    pCap->astStreamCap[1].stStreamSmooth.uMin = 1;
    pCap->astStreamCap[1].stStreamSmooth.uMax = 50;
    {
        const INT32 complexityAll[] = {0, 1, 2};
        const INT32 complexityMain[] = {1};
        FillDemoEncodeAbility(&pCap->astStreamCap[1].astEncodeAbility[0], "H.264", NET_VIDEO_CODE_H264, 1, complexityAll, 3, 0, 1, 1);
        FillDemoEncodeAbility(&pCap->astStreamCap[1].astEncodeAbility[1], "H.265", NET_VIDEO_CODE_H265, 0, complexityMain, 1, 1, 1, 1);
    }

    // H.264编码配置示例
    pCap->astStreamCap[1].astEncodeCap[0].nId = NET_LIVE_STREAM_INDEX_AUX;
    pCap->astStreamCap[1].astEncodeCap[0].enVideoType = 1;
    FillDemoResolution(&pCap->astStreamCap[1].astEncodeCap[0].stVideoResolution, 640, 480, 1.0f / 16.0f, 30.0f, 64, 1024);
    pCap->astStreamCap[1].astEncodeCap[0].enBitrateType = 0;
    pCap->astStreamCap[1].astEncodeCap[0].enImageQuality = 60;
    pCap->astStreamCap[1].astEncodeCap[0].enFrameRate = 15;
    pCap->astStreamCap[1].astEncodeCap[0].nBitrateUpperLimit = 1024;
    pCap->astStreamCap[1].astEncodeCap[0].nAverageBitrate = 512;
    pCap->astStreamCap[1].astEncodeCap[0].enVideoCodec = NET_VIDEO_CODE_H264;
    pCap->astStreamCap[1].astEncodeCap[0].bSmartEnable = FALSE;
    pCap->astStreamCap[1].astEncodeCap[0].enEncodingComplexity = 1;
    pCap->astStreamCap[1].astEncodeCap[0].nIFrameInterval = 50;
    pCap->astStreamCap[1].astEncodeCap[0].enSvcEnable = 0;
    pCap->astStreamCap[1].astEncodeCap[0].nBitrateSmoothing = 50;

    // H.265编码配置示例
    pCap->astStreamCap[1].astEncodeCap[1].nId = NET_LIVE_STREAM_INDEX_AUX;
    pCap->astStreamCap[1].astEncodeCap[1].enVideoType = 1;
    FillDemoResolution(&pCap->astStreamCap[1].astEncodeCap[1].stVideoResolution, 704, 576, 1.0f / 16.0f, 30.0f, 128, 2048);
    pCap->astStreamCap[1].astEncodeCap[1].enBitrateType = 0;
    pCap->astStreamCap[1].astEncodeCap[1].enImageQuality = 60;
    pCap->astStreamCap[1].astEncodeCap[1].enFrameRate = 15;
    pCap->astStreamCap[1].astEncodeCap[1].nBitrateUpperLimit = 2048;
    pCap->astStreamCap[1].astEncodeCap[1].nAverageBitrate = 1024;
    pCap->astStreamCap[1].astEncodeCap[1].enVideoCodec = NET_VIDEO_CODE_H265;
    pCap->astStreamCap[1].astEncodeCap[1].bSmartEnable = FALSE;
    pCap->astStreamCap[1].astEncodeCap[1].enEncodingComplexity = 1;
    pCap->astStreamCap[1].astEncodeCap[1].nIFrameInterval = 50;
    pCap->astStreamCap[1].astEncodeCap[1].enSvcEnable = 0;
    pCap->astStreamCap[1].astEncodeCap[1].nBitrateSmoothing = 50;

    // 子码流支持的分辨率列表
    pCap->astStreamCap[1].uResolutionNum = 3;
    FillDemoResolution(&pCap->astStreamCap[1].astResolution[0], 704, 576, 1.0f / 16.0f, 30.0f, 128, 2048);
    FillDemoResolution(&pCap->astStreamCap[1].astResolution[1], 640, 480, 1.0f / 16.0f, 30.0f, 64, 1024);
    FillDemoResolution(&pCap->astStreamCap[1].astResolution[2], 352, 288, 1.0f / 16.0f, 30.0f, 64, 512);

    printf("[Server] Filled %d streams capability\n", pCap->uStreamCount);
    return NET_E_SUCCEED;
}

/**
 * @brief 音频编码能力集回调实现
 * @note 模拟填充音频编码能力集的能力集数据
 */
NET_COMMON_ECODE_E MyAudioEncodeCb(INT32 dwChannelID, pNET_AudioCap_S pCap)
{
    if (!pCap)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[Server] GetAudioEncodeCap callback, channelID=%d\n", dwChannelID);

    memset(pCap, 0, sizeof(NET_AudioCap_S));

    // ================= 输入类型能力 =================
    pCap->uInputTypeSize = 2;
    pCap->auInputType[0] = NET_AUDIO_INPUT_MICIN;
    pCap->auInputType[1] = NET_AUDIO_INPUT_LINEIN;

    // ================= 输出类型能力 =================
    pCap->uOutputTypeSize = 3;
    pCap->auOutputType[0] = NET_AUDIO_OUTPUT_SPEAKER;
    pCap->auOutputType[1] = NET_AUDIO_OUTPUT_LINEOUT;
    pCap->auOutputType[2] = NET_AUDIO_OUTPUT_MUTE;

    // ================= 音频格式能力 =================
    pCap->uFormatSize = 4;
    pCap->auFormat[0] = NET_AUDIO_FORMAT_G711A;
    pCap->auFormat[1] = NET_AUDIO_FORMAT_G711U;
    pCap->auFormat[2] = NET_AUDIO_FORMAT_AAC;
    pCap->auFormat[3] = NET_AUDIO_FORMAT_MP3;

    // 格式详细能力数量
    pCap->uFormatDetailSize = 4;

    // =========================================================
    // G711A 能力
    // =========================================================
    pCap->astFormatDetail[0].uFormat = NET_AUDIO_FORMAT_G711A;

    pCap->astFormatDetail[0].uSampleRateSize = 1;
    pCap->astFormatDetail[0].auSampleRate[0] = NET_AUDIO_SAMPRATE_8000;

    pCap->astFormatDetail[0].uBitRateSize = 1;
    pCap->astFormatDetail[0].auBitRate[0] = NET_AUDIO_BITRATE_64K;

    pCap->astFormatDetail[0].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[0].stSampleRateRange.uMin = 8000;
    pCap->astFormatDetail[0].stSampleRateRange.uMax = 8000;
    pCap->astFormatDetail[0].stSampleRateRange.uStep = 0;

    pCap->astFormatDetail[0].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[0].stBitRateRange.uMin = 64000;
    pCap->astFormatDetail[0].stBitRateRange.uMax = 64000;
    pCap->astFormatDetail[0].stBitRateRange.uStep = 0;

    // =========================================================
    // G711U 能力
    // =========================================================
    pCap->astFormatDetail[1].uFormat = NET_AUDIO_FORMAT_G711U;

    pCap->astFormatDetail[1].uSampleRateSize = 1;
    pCap->astFormatDetail[1].auSampleRate[0] = NET_AUDIO_SAMPRATE_8000;

    pCap->astFormatDetail[1].uBitRateSize = 1;
    pCap->astFormatDetail[1].auBitRate[0] = NET_AUDIO_BITRATE_64K;

    pCap->astFormatDetail[1].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[1].stSampleRateRange.uMin = 8000;
    pCap->astFormatDetail[1].stSampleRateRange.uMax = 8000;
    pCap->astFormatDetail[1].stSampleRateRange.uStep = 0;

    pCap->astFormatDetail[1].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[1].stBitRateRange.uMin = 64000;
    pCap->astFormatDetail[1].stBitRateRange.uMax = 64000;
    pCap->astFormatDetail[1].stBitRateRange.uStep = 0;

    // =========================================================
    // AAC 能力
    // =========================================================
    pCap->astFormatDetail[2].uFormat = NET_AUDIO_FORMAT_AAC;

    pCap->astFormatDetail[2].uSampleRateSize = 4;
    pCap->astFormatDetail[2].auSampleRate[0] = NET_AUDIO_SAMPRATE_16000;
    pCap->astFormatDetail[2].auSampleRate[1] = NET_AUDIO_SAMPRATE_32000;
    pCap->astFormatDetail[2].auSampleRate[2] = NET_AUDIO_SAMPRATE_44100;
    pCap->astFormatDetail[2].auSampleRate[3] = NET_AUDIO_SAMPRATE_48000;

    pCap->astFormatDetail[2].uBitRateSize = 5;
    pCap->astFormatDetail[2].auBitRate[0] = NET_AUDIO_BITRATE_48K;
    pCap->astFormatDetail[2].auBitRate[1] = NET_AUDIO_BITRATE_64K;
    pCap->astFormatDetail[2].auBitRate[2] = NET_AUDIO_BITRATE_96K;
    pCap->astFormatDetail[2].auBitRate[3] = NET_AUDIO_BITRATE_128K;
    pCap->astFormatDetail[2].auBitRate[4] = NET_AUDIO_BITRATE_256K;

    pCap->astFormatDetail[2].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[2].stSampleRateRange.uMin = 16000;
    pCap->astFormatDetail[2].stSampleRateRange.uMax = 48000;
    pCap->astFormatDetail[2].stSampleRateRange.uStep = 0;

    pCap->astFormatDetail[2].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[2].stBitRateRange.uMin = 48000;
    pCap->astFormatDetail[2].stBitRateRange.uMax = 256000;
    pCap->astFormatDetail[2].stBitRateRange.uStep = 0;

    // =========================================================
    // MP3 能力
    // =========================================================
    pCap->astFormatDetail[3].uFormat = NET_AUDIO_FORMAT_MP3;

    pCap->astFormatDetail[3].uSampleRateSize = 3;
    pCap->astFormatDetail[3].auSampleRate[0] = NET_AUDIO_SAMPRATE_32000;
    pCap->astFormatDetail[3].auSampleRate[1] = NET_AUDIO_SAMPRATE_44100;
    pCap->astFormatDetail[3].auSampleRate[2] = NET_AUDIO_SAMPRATE_48000;

    pCap->astFormatDetail[3].uBitRateSize = 6;
    pCap->astFormatDetail[3].auBitRate[0] = NET_AUDIO_BITRATE_32K;
    pCap->astFormatDetail[3].auBitRate[1] = NET_AUDIO_BITRATE_48K;
    pCap->astFormatDetail[3].auBitRate[2] = NET_AUDIO_BITRATE_64K;
    pCap->astFormatDetail[3].auBitRate[3] = NET_AUDIO_BITRATE_96K;
    pCap->astFormatDetail[3].auBitRate[4] = NET_AUDIO_BITRATE_128K;
    pCap->astFormatDetail[3].auBitRate[5] = NET_AUDIO_BITRATE_256K;

    pCap->astFormatDetail[3].stSampleRateRange.bEnable = 1;
    pCap->astFormatDetail[3].stSampleRateRange.uMin = 32000;
    pCap->astFormatDetail[3].stSampleRateRange.uMax = 48000;
    pCap->astFormatDetail[3].stSampleRateRange.uStep = 0;

    pCap->astFormatDetail[3].stBitRateRange.bEnable = 1;
    pCap->astFormatDetail[3].stBitRateRange.uMin = 32000;
    pCap->astFormatDetail[3].stBitRateRange.uMax = 256000;
    pCap->astFormatDetail[3].stBitRateRange.uStep = 0;

    printf("[Server] Filled %d audio formats capability\n", pCap->uFormatDetailSize);
    return NET_E_SUCCEED;
}

static void FillDemoOsdCap(pNET_OsdCap_S pCap)
{
    static const UINT32 kFontSizeList[] = {
        NET_OSD_FONT_SIZE_ADAPTIVE,
        NET_OSD_FONT_SIZE_16,
        NET_OSD_FONT_SIZE_32,
        NET_OSD_FONT_SIZE_48
    };
    static const UINT32 kDateFormatList[] = {
        NET_OSD_DATE_YYYY_MM_DD,
        NET_OSD_DATE_MM_DD_YYYY,
        NET_OSD_DATE_DD_MM_YYYY,
        NET_OSD_DATE_YYYY_MM_DD_CHN,
        NET_OSD_DATE_MM_DD_YYYY_CHN,
        NET_OSD_DATE_DD_MM_YYYY_CHN,
        NET_OSD_DATE_YYYY_MM_DD_SLASH,
        NET_OSD_DATE_MM_DD_YYYY_SLASH,
        NET_OSD_DATE_DD_MM_YYYY_SLASH
    };
    static const UINT32 kTimeFormatList[] = {
        NET_OSD_TIME_FORMAT_24,
        NET_OSD_TIME_FORMAT_12
    };
    static const UINT32 kAlignList[] = {
        NET_OSD_ALIGN_CUSTOMIZE,
        NET_OSD_ALIGN_CHAR_LEFT,
        NET_OSD_ALIGN_CHAR_RIGHT,
        NET_OSD_ALIGN_ALL_LEFT,
        NET_OSD_ALIGN_ALL_RIGHT,
        NET_OSD_ALIGN_GB_MODE
    };

    if (!pCap)
    {
        return;
    }

    memset(pCap, 0, sizeof(NET_OsdCap_S));

    pCap->bSupportOsd = TRUE;
    pCap->bSupportName = TRUE;
    pCap->bSupportTime = TRUE;
    pCap->bSupportWeek = TRUE;
    pCap->bSupportCustomColor = TRUE;
    pCap->udwMaxOsdNum = SDKSERVER_OSD_MAX_NUM;

    pCap->udwSupportedFontSizeNum =
        FillUint32List(pCap->audwSupportedFontSizeList,
                       NET_OSD_FONT_SIZE_TYPE_MAX_NUM,
                       kFontSizeList,
                       (UINT32)(sizeof(kFontSizeList) / sizeof(kFontSizeList[0])));
    pCap->udwSupportedDateFormatNum =
        FillUint32List(pCap->audwSupportedDateFormatList,
                       NET_OSD_DATE_FORMAT_MAX_NUM,
                       kDateFormatList,
                       (UINT32)(sizeof(kDateFormatList) / sizeof(kDateFormatList[0])));
    pCap->udwSupportedTimeFormatNum =
        FillUint32List(pCap->audwSupportedTimeFormatList,
                       NET_OSD_TIME_FORMAT_MAX_NUM,
                       kTimeFormatList,
                       (UINT32)(sizeof(kTimeFormatList) / sizeof(kTimeFormatList[0])));
    pCap->udwSupportedAlignNum =
        FillUint32List(pCap->audwSupportedAlignList,
                       SDKSERVER_OSD_ALIGN_MAX_NUM,
                       kAlignList,
                       (UINT32)(sizeof(kAlignList) / sizeof(kAlignList[0])));
}

static void PrintDemoOsdCap(const NET_OsdCap_S* pCap)
{
    if (!pCap)
    {
        return;
    }

    printf("[Server] OSD cap prepared:\n");
    printf("[Server]   SupportOsd=%d, SupportName=%d, SupportTime=%d, SupportWeek=%d, SupportCustomColor=%d\n",
           pCap->bSupportOsd,
           pCap->bSupportName,
           pCap->bSupportTime,
           pCap->bSupportWeek,
           pCap->bSupportCustomColor);
    printf("[Server]   MaxOsdNum=%u\n", pCap->udwMaxOsdNum);
    PrintUint32List("SupportedFontSizeList",
                    pCap->audwSupportedFontSizeList,
                    pCap->udwSupportedFontSizeNum);
    PrintUint32List("SupportedDateFormatList",
                    pCap->audwSupportedDateFormatList,
                    pCap->udwSupportedDateFormatNum);
    PrintUint32List("SupportedTimeFormatList",
                    pCap->audwSupportedTimeFormatList,
                    pCap->udwSupportedTimeFormatNum);
    PrintUint32List("SupportedAlignList",
                    pCap->audwSupportedAlignList,
                    pCap->udwSupportedAlignNum);
}

/**
 * @brief OSD能力集回调实现
 */
NET_COMMON_ECODE_E MyOsdCapCb(INT32 dwChannelID, pNET_OsdCap_S pCap)
{
    if (!pCap)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[Server] GetOsdCap callback, channelID=%d\n", dwChannelID);

    FillDemoOsdCap(pCap);
    PrintDemoOsdCap(pCap);

    return NET_E_SUCCEED;
}

/**
 * @brief 设备信息回调实现
 */
NET_COMMON_ECODE_E MyDeviceInfoCb(pNET_DeviceInfo_S pInfo)
{
    if (!pInfo)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[Server] GetDeviceInfo callback\n");

    // 填充设备信息
    pInfo->uDevType = 0;           // 设备类型
    pInfo->uAlarmInPortNum = 4;     // 报警输入端口数
    pInfo->uAlarmOutPortNum = 2;    // 报警输出端口数
    pInfo->uChannelNum = 4;        // 通道数

    return NET_E_SUCCEED;
}

/**
 * @brief 注册回调函数
 */
void AddRegisterCb()
{
    // 注册设备信息回调
    if (NET_serverRegisterGetDeviceInfoCb(MyDeviceInfoCb))
    {
        printf("[Server] RegisterCb_GetDeviceInfo SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetDeviceInfo FAILED\n");
    }

    // 注册视频编码能力集回调
    if (NET_serverRegisterGetVideoEncodeCapCb(MyVideoEncodeCb))
    {
        printf("[Server] RegisterCb_GetVideoEncodeCap SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetVideoEncodeCap FAILED\n");
    }

    // 注册音频编码能力集回调
    if (NET_serverRegisterGetAudioEncodeCapCb(MyAudioEncodeCb))
    {
        printf("[Server] RegisterCb_GetAudioEncodeCap SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetAudioEncodeCap FAILED\n");
    }

    // 注册OSD能力集回调
    if (NET_serverRegisterGetOsdCapCb(MyOsdCapCb))
    {
        printf("[Server] RegisterCb_GetOsdCap SUCCESS\n");
    }
    else
    {
        printf("[Server] RegisterCb_GetOsdCap FAILED\n");
    }
}

int main(int argc, char* argv[])
{
    printf("=============== SDK Server Capability Demo ================\n");
    ConfigureByArgs(argc, argv);

    /* 初始化日志 */
    initSdkLogBySize("CapabilityDemo", "/opt/course/CapabilityDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    syncPrintf(1);
    setLogLevel(NETSDK_LOG_TRACE);

    /* 注册回调 */
    AddRegisterCb();

    /* 启动服务 */
    printf("[Server] Starting on port %d, username=%s...\n", g_serverPort, g_serverUsername);
    if (NET_serverInit(g_serverPort, g_serverUsername, g_serverPassword, NULL))
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

    NET_serverCleanup();
    return 0;
}
