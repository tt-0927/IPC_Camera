/**
 * @FilePath     : preview_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-11 14:08:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-28 09:51:26
 * @Description  : 预览定义数据的转换
 */

#include "preview_convert.h"
#include "convert.h"

void Convert::deal(Json::Object *pRootJson, Preview::RtspUrl_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RtspMainUrl", stInfo.strRtspMainUrl);
    convert.field(pRootJson, "RtspSubUrl", stInfo.strRtspSubUrl);
}

void Convert::deal(Json::Object *pRootJson, Preview::ImageParam_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Brightness", stInfo.nBrightness);
    convert.field(pRootJson, "Contrast", stInfo.nContrast);
    convert.field(pRootJson, "Saturation", stInfo.nSaturation);
    convert.field(pRootJson, "Sharpness", stInfo.nSharpness);
}


void Convert::deal(Json::Object *pRootJson, Preview::PreviewInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    // convert.field(pRootJson, "Volume", stInfo.nVolume);
    convert.structure(pRootJson, "RtspUrl", stInfo.stRtspUrl);
    convert.structure(pRootJson, "ImageParam", stInfo.stImageParam);
}

void Convert::deal(Json::Object *pRootJson, Preview::CollectAudioInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.nChn);
    convert.field(pRootJson, "Codec", stInfo.nCodec);
    convert.field(pRootJson, "Format", stInfo.nFormat);
    convert.field(pRootJson, "BitRate", stInfo.nBitRate);
    convert.field(pRootJson, "SampleRate", stInfo.nSampleRate);
}

void Convert::deal(Json::Object *pRootJson, Preview::IntercomInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sdp", stInfo.strSdp);
    convert.field(pRootJson, "Url", stInfo.strUrl);
    convert.field(pRootJson, "LocalIp", stInfo.strLocalIp);
}

void Convert::deal(Json::Object *pRootJson, Preview::BroadcastInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sdp", stInfo.strSdp);
    convert.field(pRootJson, "Url", stInfo.strUrl);
    convert.field(pRootJson, "LocalIp", stInfo.strLocalIp);
}

void Convert::deal(Json::Object *pRootJson, Preview::BeepAlarm_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
}