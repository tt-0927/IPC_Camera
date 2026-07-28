/**
 * @file CapabilityInfoConvert.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CapabilityInfoConvert 模块实现
 * 功能说明：
 * 1. 实现 CapabilityInfoConvert 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "CapabilityInfoConvert.h"
#include "SDKConvert.h"

#include <cstring>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NormalizeEncodeComplexityNum 对应的处理。
 * @param [in] complexityNum 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static INT32 NormalizeEncodeComplexityNum(INT32 complexityNum)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NormalizeFrameRateNum 对应的处理。
 * @param [in] frameRateNum 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static INT32 NormalizeFrameRateNum(INT32 frameRateNum)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DealFrameRateList 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

static void DealFrameRateList(Json::Object* pRootJson, NET_TV_VIDEO_RESOLUTION_S& stInfo, bool bOutStruct)
{
    if (bOutStruct)
    {
        std::memset(stInfo.adwFrameRate, 0, sizeof(stInfo.adwFrameRate));

        Json::Object* pArray = Json::get(pRootJson, "FrameRateList");
        if (pArray)
        {
            int frameRateNum = Json::Array::size(pArray);
            if (frameRateNum > 0)
            {
                stInfo.dwFrameRateNum = NormalizeFrameRateNum((INT32)frameRateNum);
                for (INT32 i = 0; i < stInfo.dwFrameRateNum; ++i)
                {
                    double frameRate = 0.0;
                    Json::Object* pItem = Json::Array::get(pArray, i);
                    if (pItem && Json::Value::get(pItem, frameRate))
                    {
                        stInfo.adwFrameRate[i] = (FLOAT)frameRate;
                    }
                }
                return;
            }
        }

        stInfo.dwFrameRateNum = NormalizeFrameRateNum(stInfo.dwFrameRateNum);
        return;
    }

    stInfo.dwFrameRateNum = NormalizeFrameRateNum(stInfo.dwFrameRateNum);
    Json::Object* pArray = Json::Array::init();
    for (INT32 i = 0; i < stInfo.dwFrameRateNum; ++i)
    {
        Json::Array::add(pArray, (float)stInfo.adwFrameRate[i]);
    }
    Json::add(pRootJson, "FrameRateList", pArray);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 视频分辨率结构体转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_VIDEO_RESOLUTION_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    if (!bOutStruct)
    {
        stInfo.dwFrameRateNum = NormalizeFrameRateNum(stInfo.dwFrameRateNum);
    }
    convert.field(pRootJson, "Name", stInfo.szName);
    convert.field(pRootJson, "Width", (int&)stInfo.dwWidth);
    convert.field(pRootJson, "Height", (int&)stInfo.dwHeight);
    convert.field(pRootJson, "FrameRateMin", stInfo.dwFrameRateMin);
    convert.field(pRootJson, "FrameRateMax", stInfo.dwFrameRateMax);
    convert.field(pRootJson, "FrameRateNum", (int&)stInfo.dwFrameRateNum);
    DealFrameRateList(pRootJson, stInfo, bOutStruct);
    convert.field(pRootJson, "BitRateMin", (int&)stInfo.dwBitRateMin);
    convert.field(pRootJson, "BitRateMax", (int&)stInfo.dwBitRateMax);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 取值范围结构体转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_RANGE_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Min", (int&)stInfo.dwMin);
    convert.field(pRootJson, "Max", (int&)stInfo.dwMax);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 视频编码参数配置转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_VIDEO_ENCODE_OPTION_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.field(pRootJson, "VideoType", stInfo.enVideoType);
    convert.field(pRootJson, "Width", stInfo.stVideoResolution.dwWidth);
    convert.field(pRootJson, "Height", stInfo.stVideoResolution.dwHeight);
    convert.field(pRootJson, "BitrateType", stInfo.enBitrateType);
    convert.field(pRootJson, "ImageQuality", stInfo.enImageQuality);
    convert.field(pRootJson, "FrameRate", stInfo.enFrameRate);
    convert.field(pRootJson, "BitrateUpperLimit", stInfo.nBitrateUpperLimit);
    convert.field(pRootJson, "AverageBitrate", stInfo.nAverageBitrate);
    convert.field(pRootJson, "VideoCodec", stInfo.enVideoCodec);
    convert.field(pRootJson, "SmartEnable", stInfo.bSmartEnable);
    convert.field(pRootJson, "EncodingComplexity", stInfo.enEncodingComplexity);
    convert.field(pRootJson, "IFrameInterval", stInfo.nIFrameInterval);
    convert.field(pRootJson, "SvcEnable", stInfo.enSvcEnable);
    convert.field(pRootJson, "BitrateSmoothing", stInfo.nBitrateSmoothing);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 视频编码格式能力转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_VIDEO_ENCODE_ABILITY_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (!bOutStruct)
    {
        stInfo.nEncodeComplexityNum = NormalizeEncodeComplexityNum(stInfo.nEncodeComplexityNum);
    }

    convert.field(pRootJson, "VideoCodec", stInfo.szVideoCodec);
    convert.field(pRootJson, "VideoCodecType", stInfo.enVideoCodec);
    convert.field(pRootJson, "SupportAdjustComplexity", stInfo.nSupportAdjustComplexity);
    convert.field(pRootJson, "EncodeComplexityNum", stInfo.nEncodeComplexityNum);
    convert.field_array(pRootJson,
                        "EncodeComplexity",
                        stInfo.anEncodeComplexity,
                        stInfo.nEncodeComplexityNum,
                        NET_TV_VIDEO_ENCODE_COMPLEXITY_MAX_NUM);
    convert.field(pRootJson, "DefaultComplexity", stInfo.nDefaultComplexity);
    convert.field(pRootJson, "SupportSVC", stInfo.bSupportSVC);
    convert.field(pRootJson, "SupportStreamSmooth", stInfo.bSupportStreamSmooth);

    if (bOutStruct)
    {
        stInfo.nEncodeComplexityNum = NormalizeEncodeComplexityNum(stInfo.nEncodeComplexityNum);
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 视频码流参数能力集转换 (NET_TV_CAP_VIDEO_ENCODE)
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_VIDEO_STREAM_CAP_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StreamType", (int&)stInfo.dwStreamType);
    convert.field(pRootJson, "SupportMultiStream", (int&)stInfo.bSupportMultiStream);
    convert.field(pRootJson, "EncodeCapSize", (int&)stInfo.dwEncodeCapSize);
    convert.field(pRootJson, "EncodeTypeNum", (int&)stInfo.dwEncodeTypeNum);
    convert.field(pRootJson, "EncodeAbilityNum", (int&)stInfo.dwEncodeAbilityNum);
    convert.field(pRootJson, "IFrameIntervalMin", (int&)stInfo.dwIFrameIntervalMin);
    convert.field(pRootJson, "IFrameIntervalMax", (int&)stInfo.dwIFrameIntervalMax);

    /* 编码能力数组处理 */
    if (bOutStruct)
    {
        /* JSON -> Struct */
        Json::Object* pArray = Json::get(pRootJson, "EncodeCap");
        int nSize = Json::Array::size(pArray);
        int nParsedSize = 0;
        for (int i = 0; i < nSize && i < NET_TV_VIDEO_ENCODE_TYPE_MAX; i++)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astEncodeCap[i], bOutStruct);
                ++nParsedSize;
            }
        }
        if (nParsedSize > 0)
        {
            stInfo.dwEncodeCapSize = nParsedSize;
        }
    }
    else
    {
        /* Struct -> JSON */
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < stInfo.dwEncodeCapSize && i < NET_TV_VIDEO_ENCODE_TYPE_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astEncodeCap[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "EncodeCap", pArray);
    }

    if (bOutStruct)
    {
        Json::Object* pAbilityArray = Json::get(pRootJson, "EncodeAbility");
        int nAbilitySize = Json::Array::size(pAbilityArray);
        int nParsedAbilitySize = 0;
        for (int i = 0; i < nAbilitySize && i < NET_TV_VIDEO_ENCODE_TYPE_MAX; i++)
        {
            Json::Object* pItem = Json::Array::get(pAbilityArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astEncodeAbility[i], bOutStruct);
                ++nParsedAbilitySize;
            }
        }
        if (nParsedAbilitySize > 0)
        {
            stInfo.dwEncodeAbilityNum = nParsedAbilitySize;
            if (stInfo.dwEncodeTypeNum <= 0)
            {
                stInfo.dwEncodeTypeNum = nParsedAbilitySize;
            }
        }
    }
    else
    {
        Json::Object* pAbilityArray = Json::Array::init();
        for (int i = 0; i < stInfo.dwEncodeAbilityNum && i < NET_TV_VIDEO_ENCODE_TYPE_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astEncodeAbility[i], bOutStruct);
            Json::Array::add(pAbilityArray, pItem);
        }
        Json::add(pRootJson, "EncodeAbility", pAbilityArray);
    }

    /* 图像质量范围 */
    convert.structure(pRootJson, "Quality", stInfo.stQuality);
    /* 码流平滑范围 */
    convert.structure(pRootJson, "StreamSmooth", stInfo.stStreamSmooth);

    /* 分辨率列表 */
    convert.field(pRootJson, "ResolutionNum", (int&)stInfo.dwResolutionNum);
    if (bOutStruct)
    {
        /* JSON -> Struct */
        Json::Object* pResArray = Json::get(pRootJson, "Resolution");
        int nResSize = Json::Array::size(pResArray);
        int nParsedResSize = 0;
        for (int i = 0; i < nResSize && i < NET_TV_RESOLUTION_NUM_MAX; i++)
        {
            Json::Object* pItem = Json::Array::get(pResArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astResolution[i], bOutStruct);
                ++nParsedResSize;
            }
        }
        if (nParsedResSize > 0)
        {
            stInfo.dwResolutionNum = nParsedResSize;
        }
    }
    else
    {
        /* Struct -> JSON */
        Json::Object* pResArray = Json::Array::init();
        for (int i = 0; i < stInfo.dwResolutionNum && i < NET_TV_RESOLUTION_NUM_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astResolution[i], bOutStruct);
            Json::Array::add(pResArray, pItem);
        }
        Json::add(pRootJson, "Resolution", pResArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NormalizeOsdCap 对应的处理。
 * @param [in,out] stInfo 函数处理参数。
 * @return 无返回值。
 */

static void NormalizeOsdCap(NET_TV_OSD_CAP_S& stInfo)
{
    stInfo.udwMaxOsdNum = std::min<UINT32>(stInfo.udwMaxOsdNum, NET_TV_OSD_CUSTOM_MAX_NUM);
    stInfo.udwSupportedFontSizeNum = std::min<UINT32>(stInfo.udwSupportedFontSizeNum, NET_TV_OSD_FONT_SIZE_TYPE_MAX_NUM);
    stInfo.udwSupportedDateFormatNum = std::min<UINT32>(stInfo.udwSupportedDateFormatNum, NET_TV_OSD_DATE_FORMAT_MAX_NUM);
    stInfo.udwSupportedTimeFormatNum = std::min<UINT32>(stInfo.udwSupportedTimeFormatNum, NET_TV_OSD_TIME_FORMAT_MAX_NUM);
    stInfo.udwSupportedAlignNum = std::min<UINT32>(stInfo.udwSupportedAlignNum, 8);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 视频编码能力集转换 (多码流, NET_TV_CAP_VIDEO_ENCODE)
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_VIDEO_ENCODE_CAP_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StreamCount", (int&)stInfo.dwStreamCount);

    /* 各码流能力数组处理 */
    if (bOutStruct)
    {
        /* JSON -> Struct */
        Json::Object* pArray = Json::get(pRootJson, "StreamCap");
        int nSize = Json::Array::size(pArray);
        int nParsedSize = 0;
        for (int i = 0; i < nSize && i < NET_TV_VIDEO_STREAM_MAX; i++)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astStreamCap[i], bOutStruct);
                ++nParsedSize;
            }
        }
        if (nParsedSize > 0)
        {
            stInfo.dwStreamCount = nParsedSize;
        }
    }
    else
    {
        /* Struct -> JSON */
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < stInfo.dwStreamCount && i < NET_TV_VIDEO_STREAM_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astStreamCap[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "StreamCap", pArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 dealIntArray 定义的内部处理。
 * @return 无返回值。
 */

static void dealIntArray(Json::Object* pRootJson,
                         const std::string& strKey,
                         INT32* pData,
                         INT32 dwValidSize,
                         INT32 dwMaxSize,
                         bool bOutStruct)
{
    if (!pRootJson || !pData)
    {
        return;
    }

    if (bOutStruct)
    {
        /* JSON -> Struct */
        Json::Object* pArray = Json::get(pRootJson, strKey);
        if (!pArray)
        {
            return;
        }

        std::vector<int> vValue;
        if (!Json::Array::get(pArray, vValue))
        {
            return;
        }

        for (int i = 0; i < (int)vValue.size() && i < (int)dwMaxSize; i++)
        {
            pData[i] = (INT32)vValue[i];
        }
    }
    else
    {
        /* Struct -> JSON */
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < (int)dwValidSize && i < (int)dwMaxSize; i++)
        {
            Json::Array::add(pArray, (int)pData[i]);
        }
        Json::add(pRootJson, strKey, pArray);
    }
}


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 音频范围转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_AUDIO_RANGE_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", (int&)stInfo.bEnable);
    convert.field(pRootJson, "Min",    (int&)stInfo.dwMin);
    convert.field(pRootJson, "Max",    (int&)stInfo.dwMax);
    convert.field(pRootJson, "Step",   (int&)stInfo.dwStep);
}


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 音频格式能力转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_AUDIO_FORMAT_CAP_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Format",         (int&)stInfo.dwFormat);
    convert.field(pRootJson, "SampleRateSize", (int&)stInfo.dwSampleRateSize);
    convert.field(pRootJson, "BitRateSize",    (int&)stInfo.dwBitRateSize);

    /* 采样率数组 */
    dealIntArray(pRootJson,
                 "SampleRate",
                 stInfo.adwSampleRate,
                 stInfo.dwSampleRateSize,
                 NET_TV_AUDIO_SAMPRATE_MAX,
                 bOutStruct);

    /* 码率数组 */
    dealIntArray(pRootJson,
                 "BitRate",
                 stInfo.adwBitRate,
                 stInfo.dwBitRateSize,
                 NET_TV_AUDIO_BITRATE_MAX,
                 bOutStruct);

    if (bOutStruct)
    {
        /* JSON -> Struct */
        Json::Object* pSampleRateRange = Json::get(pRootJson, "SampleRateRange");
        if (pSampleRateRange)
        {
            deal(pSampleRateRange, stInfo.stSampleRateRange, bOutStruct);
        }

        Json::Object* pBitRateRange = Json::get(pRootJson, "BitRateRange");
        if (pBitRateRange)
        {
            deal(pBitRateRange, stInfo.stBitRateRange, bOutStruct);
        }
    }
    else
    {
        /* Struct -> JSON */
        Json::Object* pSampleRateRange = Json::init();
        deal(pSampleRateRange, stInfo.stSampleRateRange, bOutStruct);
        Json::add(pRootJson, "SampleRateRange", pSampleRateRange);

        Json::Object* pBitRateRange = Json::init();
        deal(pBitRateRange, stInfo.stBitRateRange, bOutStruct);
        Json::add(pRootJson, "BitRateRange", pBitRateRange);
    }
}


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 音频编码能力集转换（NET_TV_CAP_AUDIO_ENCODE）
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_AUDIO_CAP_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "InputTypeSize",    (int&)stInfo.dwInputTypeSize);
    convert.field(pRootJson, "OutputTypeSize",   (int&)stInfo.dwOutputTypeSize);
    convert.field(pRootJson, "FormatSize",       (int&)stInfo.dwFormatSize);
    convert.field(pRootJson, "FormatDetailSize", (int&)stInfo.dwFormatDetailSize);

    /* 输入类型数组 */
    dealIntArray(pRootJson,
                 "InputType",
                 stInfo.adwInputType,
                 stInfo.dwInputTypeSize,
                 NET_TV_AUDIO_INPUT_TYPE_MAX,
                 bOutStruct);

    /* 输出类型数组 */
    dealIntArray(pRootJson,
                 "OutputType",
                 stInfo.adwOutputType,
                 stInfo.dwOutputTypeSize,
                 NET_TV_AUDIO_OUTPUT_TYPE_MAX,
                 bOutStruct);

    /* 音频格式数组 */
    dealIntArray(pRootJson,
                 "Format",
                 stInfo.adwFormat,
                 stInfo.dwFormatSize,
                 NET_TV_AUDIO_FORMAT_MAX,
                 bOutStruct);

    /* 各音频格式详细能力数组 */
    if (bOutStruct)
    {
        /* JSON -> Struct */
        Json::Object* pArray = Json::get(pRootJson, "FormatDetail");
        if (!pArray)
        {
            return;
        }

        int nSize = Json::Array::size(pArray);
        for (int i = 0; i < nSize && i < NET_TV_AUDIO_FORMAT_MAX; i++)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astFormatDetail[i], bOutStruct);
            }
        }
    }
    else
    {
        /* Struct -> JSON */
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < stInfo.dwFormatDetailSize && i < NET_TV_AUDIO_FORMAT_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astFormatDetail[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "FormatDetail", pArray);
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief OSD参数能力集转换 (NET_TV_CAP_OSD)
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_TV_OSD_CAP_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);

    if (!bOutStruct)
    {
        NormalizeOsdCap(stInfo);
    }

    /* 基础能力 */
    convert.field(pRootJson, "SupportOsd", (int&)stInfo.bSupportOsd);
    convert.field(pRootJson, "SupportName", (int&)stInfo.bSupportName);
    convert.field(pRootJson, "SupportTime", (int&)stInfo.bSupportTime);
    convert.field(pRootJson, "SupportWeek", (int&)stInfo.bSupportWeek);
    convert.field(pRootJson, "SupportCustomColor", (int&)stInfo.bSupportCustomColor);

    /* 字符叠加能力 */
    convert.field(pRootJson, "MaxOsdNum", (UINT32&)stInfo.udwMaxOsdNum);

    /* 字体大小能力 */
    convert.field(pRootJson, "SupportedFontSizeNum", (UINT32&)stInfo.udwSupportedFontSizeNum);
    convert.field_array(pRootJson, "SupportedFontSizeList", (int*)stInfo.audwSupportedFontSizeList, stInfo.udwSupportedFontSizeNum, NET_TV_OSD_FONT_SIZE_TYPE_MAX_NUM);

    /* 日期格式能力 */
    convert.field(pRootJson, "SupportedDateFormatNum", (UINT32&)stInfo.udwSupportedDateFormatNum);
    convert.field_array(pRootJson, "SupportedDateFormatList", (int*)stInfo.audwSupportedDateFormatList, stInfo.udwSupportedDateFormatNum, NET_TV_OSD_DATE_FORMAT_MAX_NUM);

    /* 时间格式能力 */
    convert.field(pRootJson, "SupportedTimeFormatNum", (UINT32&)stInfo.udwSupportedTimeFormatNum);
    convert.field_array(pRootJson, "SupportedTimeFormatList", (int*)stInfo.audwSupportedTimeFormatList, stInfo.udwSupportedTimeFormatNum, NET_TV_OSD_TIME_FORMAT_MAX_NUM);

    /* 对齐方式能力 */
    convert.field(pRootJson, "SupportedAlignNum", (UINT32&)stInfo.udwSupportedAlignNum);
    convert.field_array(pRootJson, "SupportedAlignList", (int*)stInfo.audwSupportedAlignList, stInfo.udwSupportedAlignNum, 8);

    if (bOutStruct)
    {
        NormalizeOsdCap(stInfo);
    }
}
