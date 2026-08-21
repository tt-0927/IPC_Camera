/**
 * @file NetTVCapabilityCb.c
 * @author tianl (tianl@kfb.cn)
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
    NET_TV_CB_TYPE_CAP_VIDEO_STREAM = 0,     /* 视频编码能力集 NET_TV_CAP_VIDEO_ENCODE */
    NET_TV_CB_TYPE_CAP_OSD,                  /* OSD参数能力集 NET_TV_CAP_OSD */
    NET_TV_CB_TYPE_CAP_SMART,                /* 智能能力集 NET_TV_CAP_SMART */
    NET_TV_CB_TYPE_CAP_IMAGE,                /* 图像参数能力集 NET_TV_CAP_IMAGE */
    NET_TV_CB_TYPE_CAP_AUDIO,                /* 音频能力集 NET_TV_CAP_AUDIO */
    NET_TV_CB_TYPE_CAP_CHANNELS_ALARM,       /* 通道告警能力集 NET_TV_CAP_CHANNELS_ALARM */
    NET_TV_CB_TYPE_CAP_SYS,                  /* 系统能力集 NET_TV_CAP_SYS */
    NET_TV_CB_TYPE_CAP_USER_MANAGE,          /* 用户管理能力集 NET_TV_CAP_USER_MANAGE */
    NET_TV_CB_TYPE_CAP_MEDIA,                /* 视频通道媒体能力集 NET_TV_CAP_MEDIA */

    NET_TV_CB_TYPE_CAP_MAX               
} Net_TV_CapabilityCb_E;

/**
 * @brief 能力集回调函数联合体定义
 */
typedef union 
{
    NET_TV_COMMON_ECODE_E (*GetVideoEncodeCap)(INT32 dwChannelID, LPNET_TV_VIDEO_ENCODE_CAP_S pCap);
    NET_TV_COMMON_ECODE_E (*GetOsdCap)(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap);
    NET_TV_COMMON_ECODE_E (*GetAudioCap)(INT32 dwChannelID, LPNET_TV_AUDIO_CAP_S pCap);
    // 后续扩展
    // NET_TV_COMMON_ECODE_E (*GetOsdCap)(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap);
    // NET_TV_COMMON_ECODE_E (*GetSmartCap)(INT32 dwChannelID, LPNET_TV_SMART_CAP_S pCap);
    // NET_TV_COMMON_ECODE_E (*GetImageCap)(INT32 dwChannelID, LPNET_TV_IMAGE_CAP_S pCap);
} Net_TV_CapabilityCb_Un;

/**
 * @brief 能力集回调项结构体
 */
typedef struct 
{
    Net_TV_CapabilityCb_E   enType;         // 回调类型
    Net_TV_CapabilityCb_Un  unFunc;         // 回调函数指针（联合体）
    int isRegistered;                        // 注册标记：0=未注册，1=已注册
} NET_TV_Capability_CbItem;

/**
 * @brief 全局能力集回调注册表
 */
static NET_TV_Capability_CbItem g_capCbTable[NET_TV_CB_TYPE_CAP_MAX] = {0};

static INT32 ClampFrameRateNum(INT32 frameRateNum)
{
    if (frameRateNum < 0)
    {
        return 0;
    }
    if (frameRateNum > NET_TV_VIDEO_FRAME_RATE_MAX_NUM)
    {
        return NET_TV_VIDEO_FRAME_RATE_MAX_NUM;
    }
    return frameRateNum;
}

static void NormalizeResolutionFrameRate(LPNET_TV_VIDEO_RESOLUTION_S pResolution)
{
    if (!pResolution)
    {
        return;
    }

    pResolution->dwFrameRateNum = ClampFrameRateNum(pResolution->dwFrameRateNum);
}

static void FillEncodeAbilityFromOption(const NET_TV_VIDEO_ENCODE_OPTION_S* pOption,
                                        LPNET_TV_VIDEO_ENCODE_ABILITY_S pAbility)
{
    if (!pOption || !pAbility)
    {
        return;
    }

    memset(pAbility, 0, sizeof(NET_TV_VIDEO_ENCODE_ABILITY_S));
    pAbility->enVideoCodec = pOption->enVideoCodec;
    switch (pOption->enVideoCodec)
    {
        case NET_TV_VIDEO_CODE_H264:
            strncpy(pAbility->szVideoCodec, "H.264", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_TV_VIDEO_CODE_H265:
            strncpy(pAbility->szVideoCodec, "H.265", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_TV_VIDEO_CODE_JPEG:
            strncpy(pAbility->szVideoCodec, "JPEG", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_TV_VIDEO_CODE_MJPEG:
            strncpy(pAbility->szVideoCodec, "MJPEG", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_TV_VIDEO_CODE_SVAC3:
            strncpy(pAbility->szVideoCodec, "SVAC3", sizeof(pAbility->szVideoCodec) - 1);
            break;
        case NET_TV_VIDEO_CODE_MPEG4:
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
    if (complexityNum > NET_TV_VIDEO_ENCODE_COMPLEXITY_MAX_NUM)
    {
        return NET_TV_VIDEO_ENCODE_COMPLEXITY_MAX_NUM;
    }
    return complexityNum;
}

static void NormalizeVideoEncodeCap(LPNET_TV_VIDEO_ENCODE_CAP_S pCap)
{
    INT32 i = 0;
    INT32 j = 0;

    if (!pCap)
    {
        return;
    }

    if (pCap->dwStreamCount < 0)
    {
        pCap->dwStreamCount = 0;
    }
    else if (pCap->dwStreamCount > NET_TV_VIDEO_STREAM_MAX)
    {
        pCap->dwStreamCount = NET_TV_VIDEO_STREAM_MAX;
    }

    for (i = 0; i < pCap->dwStreamCount; ++i)
    {
        LPNET_TV_VIDEO_STREAM_CAP_S pStream = &pCap->astStreamCap[i];
        if (pStream->dwEncodeCapSize < 0)
        {
            pStream->dwEncodeCapSize = 0;
        }
        else if (pStream->dwEncodeCapSize > NET_TV_VIDEO_ENCODE_TYPE_MAX)
        {
            pStream->dwEncodeCapSize = NET_TV_VIDEO_ENCODE_TYPE_MAX;
        }

        if (pStream->dwEncodeAbilityNum < 0)
        {
            pStream->dwEncodeAbilityNum = 0;
        }
        else if (pStream->dwEncodeAbilityNum > NET_TV_VIDEO_ENCODE_TYPE_MAX)
        {
            pStream->dwEncodeAbilityNum = NET_TV_VIDEO_ENCODE_TYPE_MAX;
        }

        if (pStream->dwEncodeTypeNum < 0)
        {
            pStream->dwEncodeTypeNum = 0;
        }
        else if (pStream->dwEncodeTypeNum > NET_TV_VIDEO_ENCODE_TYPE_MAX)
        {
            pStream->dwEncodeTypeNum = NET_TV_VIDEO_ENCODE_TYPE_MAX;
        }

        if (pStream->dwResolutionNum < 0)
        {
            pStream->dwResolutionNum = 0;
        }
        else if (pStream->dwResolutionNum > NET_TV_RESOLUTION_NUM_MAX)
        {
            pStream->dwResolutionNum = NET_TV_RESOLUTION_NUM_MAX;
        }

        for (j = 0; j < pStream->dwEncodeCapSize; ++j)
        {
            NormalizeResolutionFrameRate(&pStream->astEncodeCap[j].stVideoResolution);
        }

        if (pStream->dwEncodeAbilityNum == 0 && pStream->dwEncodeCapSize > 0)
        {
            pStream->dwEncodeAbilityNum = pStream->dwEncodeCapSize;
            for (j = 0; j < pStream->dwEncodeAbilityNum; ++j)
            {
                FillEncodeAbilityFromOption(&pStream->astEncodeCap[j], &pStream->astEncodeAbility[j]);
            }
        }

        if (pStream->dwEncodeTypeNum == 0)
        {
            pStream->dwEncodeTypeNum = pStream->dwEncodeAbilityNum;
        }

        for (j = 0; j < pStream->dwResolutionNum; ++j)
        {
            NormalizeResolutionFrameRate(&pStream->astResolution[j]);
        }

        for (j = 0; j < pStream->dwEncodeAbilityNum; ++j)
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

static void NormalizeOsdCap(LPNET_TV_OSD_CAP_S pCap)
{
    if (!pCap)
    {
        return;
    }

    pCap->udwMaxOsdNum = ClampUint32(pCap->udwMaxOsdNum, NET_TV_OSD_CUSTOM_MAX_NUM);
    pCap->udwSupportedFontSizeNum = ClampUint32(pCap->udwSupportedFontSizeNum,
                                                NET_TV_OSD_FONT_SIZE_TYPE_MAX_NUM);
    pCap->udwSupportedDateFormatNum = ClampUint32(pCap->udwSupportedDateFormatNum,
                                                  NET_TV_OSD_DATE_FORMAT_MAX_NUM);
    pCap->udwSupportedTimeFormatNum = ClampUint32(pCap->udwSupportedTimeFormatNum,
                                                  NET_TV_OSD_TIME_FORMAT_MAX_NUM);
    pCap->udwSupportedAlignNum = ClampUint32(pCap->udwSupportedAlignNum, 8);
}

// ========================== 注册接口实现 ==========================

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(NET_TV_CB_GetVideoEncodeCap pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    } 

    NET_TV_Capability_CbItem* pItem = &g_capCbTable[NET_TV_CB_TYPE_CAP_VIDEO_STREAM];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }
    
    pItem->enType = NET_TV_CB_TYPE_CAP_VIDEO_STREAM;
    pItem->unFunc.GetVideoEncodeCap = pCb;
    pItem->isRegistered = 1;

    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(NET_TV_CB_GetAudioEncodeCap pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    } 

    NET_TV_Capability_CbItem* pItem = &g_capCbTable[NET_TV_CB_TYPE_CAP_AUDIO];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }
    
    pItem->enType = NET_TV_CB_TYPE_CAP_AUDIO;
    pItem->unFunc.GetAudioCap = pCb;
    pItem->isRegistered = 1;

    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCap(NET_TV_CB_GetOsdCap pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    } 

    NET_TV_Capability_CbItem* pItem = &g_capCbTable[NET_TV_CB_TYPE_CAP_OSD];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }
    
    pItem->enType = NET_TV_CB_TYPE_CAP_OSD;
    pItem->unFunc.GetOsdCap = pCb;
    pItem->isRegistered = 1;

    return TRUE;
}

// 后续扩展其他能力集注册接口
// NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCap(...)
// NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmartCap(...)

// ========================== 执行接口实现 ==========================

int NetSDK_ExecuteCb_GetVideoEncodeCap(INT32 dwChannelID, LPNET_TV_VIDEO_ENCODE_CAP_S pCap) 
{
    if (pCap == NULL) 
    {
        return NET_TV_E_INVALID_PARAM;
    }
    
    NET_TV_Capability_CbItem* pItem = &g_capCbTable[NET_TV_CB_TYPE_CAP_VIDEO_STREAM];
    if (!pItem->isRegistered) 
    {
        return NET_TV_E_NONSUPPORT;
    }
    
    // 执行对应回调（类型安全）
    int ret = pItem->unFunc.GetVideoEncodeCap(dwChannelID, pCap);
    if (ret == NET_TV_E_SUCCEED)
    {
        NormalizeVideoEncodeCap(pCap);
    }
    return ret;
}

int NetSDK_ExecuteCb_GetAudioCap(INT32 dwChannelID, LPNET_TV_AUDIO_CAP_S pCap) 
{
    if (pCap == NULL) 
    {
        return NET_TV_E_INVALID_PARAM;
    }
    
    NET_TV_Capability_CbItem* pItem = &g_capCbTable[NET_TV_CB_TYPE_CAP_AUDIO];
    if (!pItem->isRegistered) 
    {
        return NET_TV_E_NONSUPPORT;
    }
    
    // 执行对应回调（类型安全）
    return pItem->unFunc.GetAudioCap(dwChannelID, pCap);
}

int NetSDK_ExecuteCb_GetOsdCap(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap) 
{
    if (pCap == NULL) 
    {
        return NET_TV_E_INVALID_PARAM;
    }
    
    NET_TV_Capability_CbItem* pItem = &g_capCbTable[NET_TV_CB_TYPE_CAP_OSD];
    if (!pItem->isRegistered) 
    {
        return NET_TV_E_NONSUPPORT;
    }
    
    // 执行对应回调（类型安全）
    int ret = pItem->unFunc.GetOsdCap(dwChannelID, pCap);
    if (ret == NET_TV_E_SUCCEED)
    {
        NormalizeOsdCap(pCap);
    }
    return ret;
}

// 后续扩展其他能力集执行接口
// int NetSDK_ExecuteCb_GetOsdCap(...) { ... }
// int NetSDK_ExecuteCb_GetSmartCap(...) { ... }
