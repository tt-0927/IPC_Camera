/**
 * @file CapabilityInfoConvert.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief 能力集结构体JSON转换实现
 */

#include "CapabilityInfoConvert.h"
#include "SDKConvert.h"

#include <cstring>

static INT32 NormalizeEncodeComplexityNum(INT32 complexityNum)
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

static INT32 NormalizeFrameRateNum(INT32 frameRateNum)
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

static void DealFrameRateList(Json::Object* pRootJson, NET_VideoResolution_S& stInfo, bool bOutStruct)
{
    if (bOutStruct)
    {
        std::memset(stInfo.afFrameRate, 0, sizeof(stInfo.afFrameRate));

        Json::Object* pArray = Json::get(pRootJson, "FrameRateList");
        if (pArray)
        {
            int frameRateNum = Json::Array::size(pArray);
            if (frameRateNum > 0)
            {
                stInfo.uFrameRateNum = NormalizeFrameRateNum((INT32)frameRateNum);
                for (INT32 i = 0; i < stInfo.uFrameRateNum; ++i)
                {
                    double frameRate = 0.0;
                    Json::Object* pItem = Json::Array::get(pArray, i);
                    if (pItem && Json::Value::get(pItem, frameRate))
                    {
                        stInfo.afFrameRate[i] = (FLOAT)frameRate;
                    }
                }
                return;
            }
        }

        stInfo.uFrameRateNum = NormalizeFrameRateNum(stInfo.uFrameRateNum);
        return;
    }

    stInfo.uFrameRateNum = NormalizeFrameRateNum(stInfo.uFrameRateNum);
    Json::Object* pArray = Json::Array::init();
    for (INT32 i = 0; i < stInfo.uFrameRateNum; ++i)
    {
        Json::Array::add(pArray, (float)stInfo.afFrameRate[i]);
    }
    Json::add(pRootJson, "FrameRateList", pArray);
}

/**
 * @brief 视频分辨率结构体转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_VideoResolution_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    if (!bOutStruct)
    {
        stInfo.uFrameRateNum = NormalizeFrameRateNum(stInfo.uFrameRateNum);
    }
    convert.field(pRootJson, "Name", stInfo.szName);
    convert.field(pRootJson, "Width", (int&)stInfo.uWidth);
    convert.field(pRootJson, "Height", (int&)stInfo.uHeight);
    convert.field(pRootJson, "FrameRateMin", stInfo.fFrameRateMin);
    convert.field(pRootJson, "FrameRateMax", stInfo.fFrameRateMax);
    convert.field(pRootJson, "FrameRateNum", (int&)stInfo.uFrameRateNum);
    DealFrameRateList(pRootJson, stInfo, bOutStruct);
    convert.field(pRootJson, "BitRateMin", (int&)stInfo.uBitRateMin);
    convert.field(pRootJson, "BitRateMax", (int&)stInfo.uBitRateMax);
}

/**
 * @brief 取值范围结构体转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_Range_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Min", (int&)stInfo.uMin);
    convert.field(pRootJson, "Max", (int&)stInfo.uMax);
}

/**
 * @brief 视频编码参数配置转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_VideoEncodeOption_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.field(pRootJson, "VideoType", stInfo.enVideoType);
    convert.field(pRootJson, "Width", stInfo.stVideoResolution.uWidth);
    convert.field(pRootJson, "Height", stInfo.stVideoResolution.uHeight);
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
 * @brief 视频编码格式能力转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_VideoEncodeAbility_S& stInfo, bool bOutStruct)
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
                        NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM);
    convert.field(pRootJson, "DefaultComplexity", stInfo.nDefaultComplexity);
    convert.field(pRootJson, "SupportSVC", stInfo.bSupportSVC);
    convert.field(pRootJson, "SupportStreamSmooth", stInfo.bSupportStreamSmooth);

    if (bOutStruct)
    {
        stInfo.nEncodeComplexityNum = NormalizeEncodeComplexityNum(stInfo.nEncodeComplexityNum);
    }
}

/**
 * @brief 视频码流参数能力集转换 (NET_CAP_VIDEO_ENCODE)
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_VideoStreamCap_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StreamType", (int&)stInfo.uStreamType);
    convert.field(pRootJson, "SupportMultiStream", (int&)stInfo.bSupportMultiStream);
    convert.field(pRootJson, "EncodeCapSize", (int&)stInfo.uEncodeCapSize);
    convert.field(pRootJson, "EncodeTypeNum", (int&)stInfo.uEncodeTypeNum);
    convert.field(pRootJson, "EncodeAbilityNum", (int&)stInfo.uEncodeAbilityNum);
    convert.field(pRootJson, "IFrameIntervalMin", (int&)stInfo.uIFrameIntervalMin);
    convert.field(pRootJson, "IFrameIntervalMax", (int&)stInfo.uIFrameIntervalMax);

    // 编码能力数组处理
    if (bOutStruct)
    {
        // JSON -> Struct
        Json::Object* pArray = Json::get(pRootJson, "EncodeCap");
        int nSize = Json::Array::size(pArray);
        int nParsedSize = 0;
        for (int i = 0; i < nSize && i < NET_VIDEO_ENCODE_TYPE_MAX; i++)
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
            stInfo.uEncodeCapSize = nParsedSize;
        }
    }
    else
    {
        // Struct -> JSON
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < stInfo.uEncodeCapSize && i < NET_VIDEO_ENCODE_TYPE_MAX; i++)
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
        for (int i = 0; i < nAbilitySize && i < NET_VIDEO_ENCODE_TYPE_MAX; i++)
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
            stInfo.uEncodeAbilityNum = nParsedAbilitySize;
            if (stInfo.uEncodeTypeNum <= 0)
            {
                stInfo.uEncodeTypeNum = nParsedAbilitySize;
            }
        }
    }
    else
    {
        Json::Object* pAbilityArray = Json::Array::init();
        for (int i = 0; i < stInfo.uEncodeAbilityNum && i < NET_VIDEO_ENCODE_TYPE_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astEncodeAbility[i], bOutStruct);
            Json::Array::add(pAbilityArray, pItem);
        }
        Json::add(pRootJson, "EncodeAbility", pAbilityArray);
    }

    // 图像质量范围
    convert.structure(pRootJson, "Quality", stInfo.stQuality);
    // 码流平滑范围
    convert.structure(pRootJson, "StreamSmooth", stInfo.stStreamSmooth);

    // 分辨率列表
    convert.field(pRootJson, "ResolutionNum", (int&)stInfo.uResolutionNum);
    if (bOutStruct)
    {
        // JSON -> Struct
        Json::Object* pResArray = Json::get(pRootJson, "Resolution");
        int nResSize = Json::Array::size(pResArray);
        int nParsedResSize = 0;
        for (int i = 0; i < nResSize && i < NET_RESOLUTION_NUM_MAX; i++)
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
            stInfo.uResolutionNum = nParsedResSize;
        }
    }
    else
    {
        // Struct -> JSON
        Json::Object* pResArray = Json::Array::init();
        for (int i = 0; i < stInfo.uResolutionNum && i < NET_RESOLUTION_NUM_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astResolution[i], bOutStruct);
            Json::Array::add(pResArray, pItem);
        }
        Json::add(pRootJson, "Resolution", pResArray);
    }
}

static void NormalizeOsdCap(NET_OsdCap_S& stInfo)
{
    stInfo.udwMaxOsdNum = std::min<UINT32>(stInfo.udwMaxOsdNum, NET_OSD_CUSTOM_MAX_NUM);
    stInfo.udwSupportedFontSizeNum = std::min<UINT32>(stInfo.udwSupportedFontSizeNum, NET_OSD_FONT_SIZE_TYPE_MAX_NUM);
    stInfo.udwSupportedDateFormatNum = std::min<UINT32>(stInfo.udwSupportedDateFormatNum, NET_OSD_DATE_FORMAT_MAX_NUM);
    stInfo.udwSupportedTimeFormatNum = std::min<UINT32>(stInfo.udwSupportedTimeFormatNum, NET_OSD_TIME_FORMAT_MAX_NUM);
    stInfo.udwSupportedAlignNum = std::min<UINT32>(stInfo.udwSupportedAlignNum, 8);
}

/**
 * @brief 视频编码能力集转换 (多码流, NET_CAP_VIDEO_ENCODE)
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_VideoEncodeCap_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StreamCount", (int&)stInfo.uStreamCount);

    // 各码流能力数组处理
    if (bOutStruct)
    {
        // JSON -> Struct
        Json::Object* pArray = Json::get(pRootJson, "StreamCap");
        int nSize = Json::Array::size(pArray);
        int nParsedSize = 0;
        for (int i = 0; i < nSize && i < NET_VIDEO_STREAM_MAX; i++)
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
            stInfo.uStreamCount = nParsedSize;
        }
    }
    else
    {
        // Struct -> JSON
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < stInfo.uStreamCount && i < NET_VIDEO_STREAM_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astStreamCap[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "StreamCap", pArray);
    }
}

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
        // JSON -> Struct
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
        // Struct -> JSON
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < (int)dwValidSize && i < (int)dwMaxSize; i++)
        {
            Json::Array::add(pArray, (int)pData[i]);
        }
        Json::add(pRootJson, strKey, pArray);
    }
}


/**
 * @brief 音频范围转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudioRange_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", (int&)stInfo.bEnable);
    convert.field(pRootJson, "Min",    (int&)stInfo.uMin);
    convert.field(pRootJson, "Max",    (int&)stInfo.uMax);
    convert.field(pRootJson, "Step",   (int&)stInfo.uStep);
}


/**
 * @brief 音频格式能力转换
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudioFormatCap_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Format",         (int&)stInfo.uFormat);
    convert.field(pRootJson, "SampleRateSize", (int&)stInfo.uSampleRateSize);
    convert.field(pRootJson, "BitRateSize",    (int&)stInfo.uBitRateSize);

    // 采样率数组
    dealIntArray(pRootJson,
                 "SampleRate",
                 stInfo.auSampleRate,
                 stInfo.uSampleRateSize,
                 NET_AUDIO_SAMPRATE_MAX,
                 bOutStruct);

    // 码率数组
    dealIntArray(pRootJson,
                 "BitRate",
                 stInfo.auBitRate,
                 stInfo.uBitRateSize,
                 NET_AUDIO_BITRATE_MAX,
                 bOutStruct);

    if (bOutStruct)
    {
        // JSON -> Struct
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
        // Struct -> JSON
        Json::Object* pSampleRateRange = Json::init();
        deal(pSampleRateRange, stInfo.stSampleRateRange, bOutStruct);
        Json::add(pRootJson, "SampleRateRange", pSampleRateRange);

        Json::Object* pBitRateRange = Json::init();
        deal(pBitRateRange, stInfo.stBitRateRange, bOutStruct);
        Json::add(pRootJson, "BitRateRange", pBitRateRange);
    }
}


/**
 * @brief 音频编码能力集转换（NET_CAP_AUDIO_ENCODE）
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudioCap_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "InputTypeSize",    (int&)stInfo.uInputTypeSize);
    convert.field(pRootJson, "OutputTypeSize",   (int&)stInfo.uOutputTypeSize);
    convert.field(pRootJson, "FormatSize",       (int&)stInfo.uFormatSize);
    convert.field(pRootJson, "FormatDetailSize", (int&)stInfo.uFormatDetailSize);

    // 输入类型数组
    dealIntArray(pRootJson,
                 "InputType",
                 stInfo.auInputType,
                 stInfo.uInputTypeSize,
                 NET_AUDIO_INPUT_TYPE_MAX,
                 bOutStruct);

    // 输出类型数组
    dealIntArray(pRootJson,
                 "OutputType",
                 stInfo.auOutputType,
                 stInfo.uOutputTypeSize,
                 NET_AUDIO_OUTPUT_TYPE_MAX,
                 bOutStruct);

    // 音频格式数组
    dealIntArray(pRootJson,
                 "Format",
                 stInfo.auFormat,
                 stInfo.uFormatSize,
                 NET_AUDIO_FORMAT_MAX,
                 bOutStruct);

    // 各音频格式详细能力数组
    if (bOutStruct)
    {
        // JSON -> Struct
        Json::Object* pArray = Json::get(pRootJson, "FormatDetail");
        if (!pArray)
        {
            return;
        }

        int nSize = Json::Array::size(pArray);
        for (int i = 0; i < nSize && i < NET_AUDIO_FORMAT_MAX; i++)
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
        // Struct -> JSON
        Json::Object* pArray = Json::Array::init();
        for (int i = 0; i < stInfo.uFormatDetailSize && i < NET_AUDIO_FORMAT_MAX; i++)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astFormatDetail[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "FormatDetail", pArray);
    }
}

/**
 * @brief OSD参数能力集转换 (NET_CAP_OSD)
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_OsdCap_S& stInfo, bool bOutStruct)
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

    // 基础能力
    convert.field(pRootJson, "SupportOsd", (int&)stInfo.bSupportOsd);
    convert.field(pRootJson, "SupportName", (int&)stInfo.bSupportName);
    convert.field(pRootJson, "SupportTime", (int&)stInfo.bSupportTime);
    convert.field(pRootJson, "SupportWeek", (int&)stInfo.bSupportWeek);
    convert.field(pRootJson, "SupportCustomColor", (int&)stInfo.bSupportCustomColor);

    // 字符叠加能力
    convert.field(pRootJson, "MaxOsdNum", (UINT32&)stInfo.udwMaxOsdNum);

    // 字体大小能力
    convert.field(pRootJson, "SupportedFontSizeNum", (UINT32&)stInfo.udwSupportedFontSizeNum);
    convert.field_array(pRootJson, "SupportedFontSizeList", (int*)stInfo.audwSupportedFontSizeList, stInfo.udwSupportedFontSizeNum, NET_OSD_FONT_SIZE_TYPE_MAX_NUM);

    // 日期格式能力
    convert.field(pRootJson, "SupportedDateFormatNum", (UINT32&)stInfo.udwSupportedDateFormatNum);
    convert.field_array(pRootJson, "SupportedDateFormatList", (int*)stInfo.audwSupportedDateFormatList, stInfo.udwSupportedDateFormatNum, NET_OSD_DATE_FORMAT_MAX_NUM);

    // 时间格式能力
    convert.field(pRootJson, "SupportedTimeFormatNum", (UINT32&)stInfo.udwSupportedTimeFormatNum);
    convert.field_array(pRootJson, "SupportedTimeFormatList", (int*)stInfo.audwSupportedTimeFormatList, stInfo.udwSupportedTimeFormatNum, NET_OSD_TIME_FORMAT_MAX_NUM);

    // 对齐方式能力
    convert.field(pRootJson, "SupportedAlignNum", (UINT32&)stInfo.udwSupportedAlignNum);
    convert.field_array(pRootJson, "SupportedAlignList", (int*)stInfo.audwSupportedAlignList, stInfo.udwSupportedAlignNum, 8);

    if (bOutStruct)
    {
        NormalizeOsdCap(stInfo);
    }
}

/* ==================== 从 DeviceInfoConvert 搬运: 视频/图像参数配置 ==================== */

static constexpr int kOsdCustomSlotCount = NET_OSD_CUSTOM_MAX_NUM;


void SDKConvert::deal(Json::Object* pRootJson, NET_VideoOsdCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
    {
        std::memset(stInfo.OsdInfo, 0, sizeof(stInfo.OsdInfo));
    }

#define CONVERT_ENUM_FIELD(jsonKey, enumField, enumType)      \
    do                                                        \
    {                                                         \
        int nTmpValue = (int)(enumField);                     \
        convert.field(pRootJson, jsonKey, nTmpValue);         \
        if (bOutStruct)                                      \
        {                                                     \
            (enumField) = (enumType)nTmpValue;                \
        }                                                     \
    } while (0)

    /* 全局对齐方式 */
    CONVERT_ENUM_FIELD("Align", stInfo.enAlign, OSD_ALIGN_E);

    /* Name OSD */
    convert.field(pRootJson, "NameEnable",      stInfo.stOsdNameInfo.bEnable);
    convert.field(pRootJson, "Name",            stInfo.stOsdNameInfo.strName);
    convert.field(pRootJson, "NameX",           stInfo.stOsdNameInfo.stOsdAttr.nX);
    convert.field(pRootJson, "NameY",           stInfo.stOsdNameInfo.stOsdAttr.nY);
    convert.field(pRootJson, "NameW",           stInfo.stOsdNameInfo.stOsdAttr.nW);
    convert.field(pRootJson, "NameH",           stInfo.stOsdNameInfo.stOsdAttr.nH);
    CONVERT_ENUM_FIELD("NameAttribute", stInfo.stOsdNameInfo.stOsdAttr.enAttribute, OSD_ATTRIBUTE_E);
    CONVERT_ENUM_FIELD("NameFontSize",  stInfo.stOsdNameInfo.stOsdAttr.enFontSize,  OSD_FONT_SIZE_E);
    CONVERT_ENUM_FIELD("NameFontColor", stInfo.stOsdNameInfo.stOsdAttr.enFontColor, OSD_COLOR_E);
    convert.field(pRootJson, "NameCustomColor", stInfo.stOsdNameInfo.stOsdAttr.strFontColor);
    convert.field(pRootJson, "NameToken",       stInfo.stOsdNameInfo.stOsdAttr.strToken);

    /* Time OSD */
    convert.field(pRootJson, "TimeEnable",      stInfo.stOsdTimeInfo.bEnable);
    convert.field(pRootJson, "TimeEnableWeek",  stInfo.stOsdTimeInfo.bEnableWeek);
    CONVERT_ENUM_FIELD("TimeFormat", stInfo.stOsdTimeInfo.enTimeFormat, OSD_TIME_FORMAT_E);
    CONVERT_ENUM_FIELD("DateFormat", stInfo.stOsdTimeInfo.enDateFormat, OSD_DATE_FORMAT_E);
    convert.field(pRootJson, "TimeX",           stInfo.stOsdTimeInfo.stOsdAttr.nX);
    convert.field(pRootJson, "TimeY",           stInfo.stOsdTimeInfo.stOsdAttr.nY);
    convert.field(pRootJson, "TimeW",           stInfo.stOsdTimeInfo.stOsdAttr.nW);
    convert.field(pRootJson, "TimeH",           stInfo.stOsdTimeInfo.stOsdAttr.nH);
    CONVERT_ENUM_FIELD("TimeAttribute", stInfo.stOsdTimeInfo.stOsdAttr.enAttribute, OSD_ATTRIBUTE_E);
    CONVERT_ENUM_FIELD("TimeFontSize",  stInfo.stOsdTimeInfo.stOsdAttr.enFontSize,  OSD_FONT_SIZE_E);
    CONVERT_ENUM_FIELD("TimeFontColor", stInfo.stOsdTimeInfo.stOsdAttr.enFontColor, OSD_COLOR_E);
    convert.field(pRootJson, "TimeCustomColor", stInfo.stOsdTimeInfo.stOsdAttr.strFontColor);
    convert.field(pRootJson, "TimeToken",       stInfo.stOsdTimeInfo.stOsdAttr.strToken);

    /* 当前IPC能力只开放4个自定义字符叠加槽位，结构体保留32槽位用于兼容。 */
    for (int i = 0; i < kOsdCustomSlotCount; ++i)
    {
        char szKey[64] = {0};

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Id", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].nId);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Enable", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].bEnable);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Name", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].strName);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_X", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nX);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Y", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nY);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_W", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nW);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_H", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nH);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Attribute", i);
        {
            int nTmpValue = (int)stInfo.OsdInfo[i].stOsdAttr.enAttribute;
            convert.field(pRootJson, szKey, nTmpValue);
            if (bOutStruct)
            {
                stInfo.OsdInfo[i].stOsdAttr.enAttribute = (OSD_ATTRIBUTE_E)nTmpValue;
            }
        }

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_FontSize", i);
        {
            int nTmpValue = (int)stInfo.OsdInfo[i].stOsdAttr.enFontSize;
            convert.field(pRootJson, szKey, nTmpValue);
            if (bOutStruct)
            {
                stInfo.OsdInfo[i].stOsdAttr.enFontSize = (OSD_FONT_SIZE_E)nTmpValue;
            }
        }

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_FontColor", i);
        {
            int nTmpValue = (int)stInfo.OsdInfo[i].stOsdAttr.enFontColor;
            convert.field(pRootJson, szKey, nTmpValue);
            if (bOutStruct)
            {
                stInfo.OsdInfo[i].stOsdAttr.enFontColor = (OSD_COLOR_E)nTmpValue;
            }
        }

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_CustomColor", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.strFontColor);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Token", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.strToken);
    }

    /* byRes 为保留字段，不做转换 */

#undef CONVERT_ENUM_FIELD
}


/* ==================== Preview info ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_PreviewRtspUrl_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "RtspMainUrl", stInfo.szRtspMainUrl);
    convert.field(pRootJson, "RtspSubUrl", stInfo.szRtspSubUrl);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_PreviewImageParam_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Brightness", stInfo.nBrightness);
    convert.field(pRootJson, "Contrast", stInfo.nContrast);
    convert.field(pRootJson, "Saturation", stInfo.nSaturation);
    convert.field(pRootJson, "Sharpness", stInfo.nSharpness);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_ImageSetting_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Brightness", stInfo.nBrightness);
    convert.field(pRootJson, "Contrast", stInfo.nContrast);
    convert.field(pRootJson, "Saturation", stInfo.nSaturation);
    convert.field(pRootJson, "Sharpness", stInfo.nSharpness);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_PreviewInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, "RtspUrl", stInfo.stRtspUrl);
    convert.structure(pRootJson, "ImageParam", stInfo.stImageParam);
}


/* ==================== 隐私遮盖配置相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_PrivacyMaskArea_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AreaID", stInfo.nAreaID);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_PrivacyMaskCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AreaCount", stInfo.uAreaCount);

    auto clamp_area_count = [](int nCount) -> int
    {
        if (nCount < 0)
        {
            return 0;
        }
        if (nCount > NET_MAX_PRIVACY_MASK_AREA_NUM)
        {
            return NET_MAX_PRIVACY_MASK_AREA_NUM;
        }
        return nCount;
    };

    /* 逐个转换遮盖区域 */
    if (bOutStruct)
    {
        /* Json -> Struct */
        Json::Object* pAreas = Json::get(pRootJson, "Areas");
        int nSize = pAreas ? Json::Array::size(pAreas) : 0;
        nSize = clamp_area_count(nSize);

        int nCount = clamp_area_count(stInfo.uAreaCount);
        if (nCount == 0 || nCount > nSize)
        {
            nCount = nSize;
        }
        stInfo.uAreaCount = nCount;

        for (int i = 0; i < nCount; i++)
        {
            Json::Object* pItem = Json::Array::get(pAreas, i);
            if (pItem)
            {
                deal(pItem, stInfo.astArea[i], bOutStruct);
            }
        }
    }
    else
    {
        /* Struct -> Json */
        Json::Object* pAreas = Json::Array::init();
        int nCount = clamp_area_count(stInfo.uAreaCount);
        stInfo.uAreaCount = nCount;
        for (int i = 0; i < nCount; i++)
        {
            Json::Object* pItem = Json::init();
            if (pItem)
            {
                deal(pItem, stInfo.astArea[i], bOutStruct);
                Json::Array::add(pAreas, pItem);
            }
        }
        Json::add(pRootJson, "Areas", pAreas);
    }
}


void SDKConvert::deal(Json::Object *pRootJson, NET_ExposureInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "ExpTime", stInfo.enExpTime);
    convert.field(pRootJson, "AntiBanding", stInfo.bAntiBanding);
}


void SDKConvert::deal(Json::Object *pRootJson, NET_DayNightInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "DayNightMode", stInfo.enDayNightMode);
    convert.field(pRootJson, "BeginHour", stInfo.nBeginHour);
    convert.field(pRootJson, "BeginMinute", stInfo.nBeginMinute);
    convert.field(pRootJson, "BeginSecond", stInfo.nBeginSecond);
    convert.field(pRootJson, "BeginMilliSec", stInfo.nBeginMilliSec);
    convert.field(pRootJson, "EndHour", stInfo.nEndHour);
    convert.field(pRootJson, "EndMinute", stInfo.nEndMinute);
    convert.field(pRootJson, "EndSecond", stInfo.nEndSecond);
    convert.field(pRootJson, "EndMilliSec", stInfo.nEndMilliSec);
    convert.field(pRootJson, "SensitivityLevel", stInfo.nSensitivityLevel);
    convert.field(pRootJson, "FilterTime", stInfo.nFilterTime);
    convert.field(pRootJson, "FillLightExp", stInfo.bFillLightExp);
    convert.field(pRootJson, "LightMode", stInfo.enLightMode);
    convert.field(pRootJson, "LightType", stInfo.enLightType);
    convert.field(pRootJson, "WhiteLightEnable", stInfo.bWhiteLightEnable);
    convert.field(pRootJson, "WhiteLightLevel", stInfo.nWhiteLightLevel);
    convert.field(pRootJson, "RedLightEnable", stInfo.bRedLightEnable);
    convert.field(pRootJson, "RedLightLevel", stInfo.nRedLightLevel);
}


void SDKConvert::deal(Json::Object *pRootJson, NET_BackLightInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "BackLightArea", stInfo.enBackLightArea);
    convert.field(pRootJson, "WdrEnable", stInfo.bWdrEnable);
    convert.field(pRootJson, "WdrLevel", stInfo.nWdrLevel);
    convert.field(pRootJson, "HlsEnable", stInfo.bHlsEnable);
    convert.field(pRootJson, "HlsLevel", stInfo.nHlsLevel);
    printf("\n Test \n");
}


void SDKConvert::deal(Json::Object *pRootJson, NET_DenoiseInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "DnrMode", stInfo.enDnrMode);
    convert.field(pRootJson, "DnrLevel", stInfo.nDnrLevel);
    convert.field(pRootJson, "SnrLevel", stInfo.nSnrLevel);
    convert.field(pRootJson, "TnrLevel", stInfo.nTnrLevel);
}


void SDKConvert::deal(Json::Object *pRootJson, NET_WhiteBalanceInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "AwbMode", stInfo.enAwbMode);
    convert.field(pRootJson, "RGain", stInfo.nRGain);
    convert.field(pRootJson, "BGain", stInfo.nBGain);
}
