/**
 * @file NetTVCapabilityCb.c
 * @author chenchl (chenchl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief 能力集回调 - 按照NetTVDeviceCb模式实现
 */

#include <stdio.h>

#include "NetTVCapabilityCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @brief 能力集回调枚举定义
 */
typedef enum
{
    NET_CB_TYPE_CAP_VIDEO_STREAM = 0,     /* 视频编码能力集 NET_CAP_VIDEO_ENCODE */
    NET_CB_TYPE_CAP_OSD,                  /* OSD参数能力集 NET_CAP_OSD */
    NET_CB_TYPE_CAP_SMART,                /* 智能能力集 NET_CAP_SMART */
    NET_CB_TYPE_CAP_IMAGE,                /* 图像参数能力集 NET_CAP_IMAGE */
    NET_CB_TYPE_CAP_AUDIO,                /* 音频能力集 NET_CAP_AUDIO */
    NET_CB_TYPE_CAP_CHANNELS_ALARM,       /* 通道告警能力集 NET_CAP_CHANNELS_ALARM */
    NET_CB_TYPE_CAP_SYS,                  /* 系统能力集 NET_CAP_SYS */
    NET_CB_TYPE_CAP_USER_MANAGE,          /* 用户管理能力集 NET_CAP_USER_MANAGE */
    NET_CB_TYPE_CAP_MEDIA,                /* 视频通道媒体能力集 NET_CAP_MEDIA */

    NET_CB_TYPE_CAP_MAX
} Net_CapabilityCb_E;

/**
 * @brief 能力集回调函数联合体定义
 */
typedef union
{
    NET_COMMON_ECODE_E (*GetVideoEncodeCap)(INT32 nChannelId, pNET_VideoEncodeCap_S pCap);
    NET_COMMON_ECODE_E (*GetOsdCap)(INT32 nChannelId, pNET_OsdCap_S pCap);
    NET_COMMON_ECODE_E (*GetAudioCap)(INT32 nChannelId, pNET_AudioCap_S pCap);
    // 后续扩展
    // NET_COMMON_ECODE_E (*GetOsdCap)(INT32 nChannelId, pNET_OsdCap_S pCap);
    // NET_COMMON_ECODE_E (*GetSmartCap)(INT32 nChannelId, pNET_SmartCap_S pCap);
    // NET_COMMON_ECODE_E (*GetImageCap)(INT32 nChannelId, pNET_ImageCap_S pCap);
} Net_CapabilityCb_Un;

/**
 * @brief 能力集回调项结构体
 */
typedef struct
{
    Net_CapabilityCb_E   enType;         // 回调类型
    Net_CapabilityCb_Un  unFunc;         // 回调函数指针（联合体）
    int isRegistered;                        // 注册标记：0=未注册，1=已注册
} NET_Capability_CbItem;

/**
 * @brief 全局能力集回调注册表
 */
static NET_Capability_CbItem g_capCbTable[NET_CB_TYPE_CAP_MAX] = {0};

static INT32 ClampFrameRateNum(INT32 frameRateNum)
{
    if (frameRateNum < 0)
    {
        return 0;
    }
    if (frameRateNum > NET_VIDEO_FRAME_RATE_MAX_NUM)
    {
        return NET_VIDEO_FRAME_RATE_MAX_NUM;
    }
    return frameRateNum;
}

static void NormalizeResolutionFrameRate(pNET_VideoResolution_S pResolution)
{
    if (!pResolution)
    {
        return;
    }

    pResolution->uFrameRateNum = ClampFrameRateNum(pResolution->uFrameRateNum);
}

static void FillEncodeAbilityFromOption(const NET_VideoEncodeOption_S* pOption,
                                        pNET_VideoEncodeAbility_S pAbility)
{
    if (!pOption || !pAbility)
    {
        return;
    }

    memset(pAbility, 0, sizeof(NET_VideoEncodeAbility_S));
    pAbility->enVideoCodec = pOption->enVideoCodec;
    switch (pOption->enVideoCodec)
    {
        case NET_VIDEO_CODE_H264:
            strncpy(pAbility->szVideoCodec, "H.264", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_VIDEO_CODE_H265:
            strncpy(pAbility->szVideoCodec, "H.265", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_VIDEO_CODE_JPEG:
            strncpy(pAbility->szVideoCodec, "JPEG", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_VIDEO_CODE_MJPEG:
            strncpy(pAbility->szVideoCodec, "MJPEG", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_VIDEO_CODE_SVAC3:
            strncpy(pAbility->szVideoCodec, "SVAC3", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_VIDEO_CODE_MPEG4:
            strncpy(pAbility->szVideoCodec, "MPEG4", sizeof(pAbility->szVideoCodec) - 1);
            break;
        default:
            strncpy(pAbility->szVideoCodec, "UNKNOWN", sizeof(pAbility->szVideoCodec) - 1);
            break;
    }
    pAbility->nEncodeComplexityNum = 1;
    pAbility->anEncodeComplexity[0] = pOption->enEncodingComplexity;
    pAbility->nDefaultComplexity = (UINT32)pOption->enEncodingComplexity;
    pAbility->bSupportSVC = pOption->enSvcEnable;
    pAbility->bSupportStreamSmooth = pOption->nBitrateSmoothing > 0 ? 1 : 0;
}

static INT32 ClampEncodeComplexityNum(INT32 complexityNum)
{
    if (complexityNum < 0)
    {
        return 0;
    }
    if (complexityNum > NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM)
    {
        return NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM;
    }
    return complexityNum;
}

static void NormalizeVideoEncodeCap(pNET_VideoEncodeCap_S pCap)
{
    INT32 i = 0;
    INT32 j = 0;

    if (!pCap)
    {
        return;
    }

    if (pCap->uStreamCount < 0)
    {
        pCap->uStreamCount = 0;
    }
    else if (pCap->uStreamCount > NET_VIDEO_STREAM_MAX)
    {
        pCap->uStreamCount = NET_VIDEO_STREAM_MAX;
    }

    for (i = 0; i < pCap->uStreamCount; ++i)
    {
        pNET_VideoStreamCap_S pStream = &pCap->astStreamCap[i];
        if (pStream->uEncodeCapSize < 0)
        {
            pStream->uEncodeCapSize = 0;
        }
        else if (pStream->uEncodeCapSize > NET_VIDEO_ENCODE_TYPE_MAX)
        {
            pStream->uEncodeCapSize = NET_VIDEO_ENCODE_TYPE_MAX;
        }

        if (pStream->uEncodeAbilityNum < 0)
        {
            pStream->uEncodeAbilityNum = 0;
        }
        else if (pStream->uEncodeAbilityNum > NET_VIDEO_ENCODE_TYPE_MAX)
        {
            pStream->uEncodeAbilityNum = NET_VIDEO_ENCODE_TYPE_MAX;
        }

        if (pStream->uEncodeTypeNum < 0)
        {
            pStream->uEncodeTypeNum = 0;
        }
        else if (pStream->uEncodeTypeNum > NET_VIDEO_ENCODE_TYPE_MAX)
        {
            pStream->uEncodeTypeNum = NET_VIDEO_ENCODE_TYPE_MAX;
        }

        if (pStream->uResolutionNum < 0)
        {
            pStream->uResolutionNum = 0;
        }
        else if (pStream->uResolutionNum > NET_RESOLUTION_NUM_MAX)
        {
            pStream->uResolutionNum = NET_RESOLUTION_NUM_MAX;
        }

        for (j = 0; j < pStream->uEncodeCapSize; ++j)
        {
            NormalizeResolutionFrameRate(&pStream->astEncodeCap[j].stVideoResolution);
        }

        if (pStream->uEncodeAbilityNum == 0 && pStream->uEncodeCapSize > 0)
        {
            pStream->uEncodeAbilityNum = pStream->uEncodeCapSize;
            for (j = 0; j < pStream->uEncodeAbilityNum; ++j)
            {
                FillEncodeAbilityFromOption(&pStream->astEncodeCap[j], &pStream->astEncodeAbility[j]);
            }
        }

        if (pStream->uEncodeTypeNum == 0)
        {
            pStream->uEncodeTypeNum = pStream->uEncodeAbilityNum;
        }

        for (j = 0; j < pStream->uResolutionNum; ++j)
        {
            NormalizeResolutionFrameRate(&pStream->astResolution[j]);
        }

        for (j = 0; j < pStream->uEncodeAbilityNum; ++j)
        {
            pStream->astEncodeAbility[j].nEncodeComplexityNum =
                ClampEncodeComplexityNum(pStream->astEncodeAbility[j].nEncodeComplexityNum);
        }
    }
}

static UINT32 ClampUint32(UINT32 value, UINT32 maxValue)
{
    return (value > maxValue) ? maxValue : value;
}

static void NormalizeOsdCap(pNET_OsdCap_S pCap)
{
    if (!pCap)
    {
        return;
    }

    pCap->udwMaxOsdNum = ClampUint32(pCap->udwMaxOsdNum, NET_OSD_CUSTOM_MAX_NUM);
    pCap->udwSupportedFontSizeNum = ClampUint32(pCap->udwSupportedFontSizeNum,
                                                NET_OSD_FONT_SIZE_TYPE_MAX_NUM);
    pCap->udwSupportedDateFormatNum = ClampUint32(pCap->udwSupportedDateFormatNum,
                                                  NET_OSD_DATE_FORMAT_MAX_NUM);
    pCap->udwSupportedTimeFormatNum = ClampUint32(pCap->udwSupportedTimeFormatNum,
                                                  NET_OSD_TIME_FORMAT_MAX_NUM);
    pCap->udwSupportedAlignNum = ClampUint32(pCap->udwSupportedAlignNum, 8);
}

// ========================== 注册接口实现 ==========================

NET_API BOOL STDCALL NET_serverRegisterGetVideoEncodeCapCb(NET_CB_GetVideoEncodeCap pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    NET_Capability_CbItem* pItem = &g_capCbTable[NET_CB_TYPE_CAP_VIDEO_STREAM];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }

    pItem->enType = NET_CB_TYPE_CAP_VIDEO_STREAM;
    pItem->unFunc.GetVideoEncodeCap = pCb;
    pItem->isRegistered = 1;

    return TRUE;
}

NET_API BOOL STDCALL NET_serverRegisterGetAudioEncodeCapCb(NET_CB_GetAudioEncodeCap pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    NET_Capability_CbItem* pItem = &g_capCbTable[NET_CB_TYPE_CAP_AUDIO];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }

    pItem->enType = NET_CB_TYPE_CAP_AUDIO;
    pItem->unFunc.GetAudioCap = pCb;
    pItem->isRegistered = 1;

    return TRUE;
}

NET_API BOOL STDCALL NET_serverRegisterGetOsdCapCb(NET_CB_GetOsdCap pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    NET_Capability_CbItem* pItem = &g_capCbTable[NET_CB_TYPE_CAP_OSD];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }

    pItem->enType = NET_CB_TYPE_CAP_OSD;
    pItem->unFunc.GetOsdCap = pCb;
    pItem->isRegistered = 1;

    return TRUE;
}

// 后续扩展其他能力集注册接口
// NET_API BOOL STDCALL NET_serverRegisterGetOsdCapCb(...)
// NET_API BOOL STDCALL NET_serverRegisterGetSmartCapCb(...)

// ========================== 执行接口实现 ==========================

int executeGetVideoEncodeCapCb(INT32 nChannelId, pNET_VideoEncodeCap_S pCap)
{
    if (pCap == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    NET_Capability_CbItem* pItem = &g_capCbTable[NET_CB_TYPE_CAP_VIDEO_STREAM];
    if (!pItem->isRegistered)
    {
        return NET_E_NONSUPPORT;
    }

    // 执行对应回调（类型安全）
    int ret = pItem->unFunc.GetVideoEncodeCap(nChannelId, pCap);
    if (ret == NET_E_SUCCEED)
    {
        NormalizeVideoEncodeCap(pCap);
    }
    return ret;
}

int executeGetAudioCapCb(INT32 nChannelId, pNET_AudioCap_S pCap)
{
    if (pCap == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    NET_Capability_CbItem* pItem = &g_capCbTable[NET_CB_TYPE_CAP_AUDIO];
    if (!pItem->isRegistered)
    {
        return NET_E_NONSUPPORT;
    }

    // 执行对应回调（类型安全）
    return pItem->unFunc.GetAudioCap(nChannelId, pCap);
}

int executeGetOsdCapCb(INT32 nChannelId, pNET_OsdCap_S pCap)
{
    if (pCap == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    NET_Capability_CbItem* pItem = &g_capCbTable[NET_CB_TYPE_CAP_OSD];
    if (!pItem->isRegistered)
    {
        return NET_E_NONSUPPORT;
    }

    // 执行对应回调（类型安全）
    int ret = pItem->unFunc.GetOsdCap(nChannelId, pCap);
    if (ret == NET_E_SUCCEED)
    {
        NormalizeOsdCap(pCap);
    }
    return ret;
}

// 后续扩展其他能力集执行接口
// int executeGetOsdCapCb(...) { ... }
// int executeGetSmartCapCb(...) { ... }
