/**
 * @file AlarmInfoConvert.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief 实现告警结构体与 JSON 数据之间的双向转换。
 * 功能说明：
 * 1. 转换基础告警、规则告警和抓拍告警数据。
 * 2. 处理固定数组、Base64 图片和抓拍扩展属性。
 * 3. 保证 JSON 输入数据经过长度和数量限制后再写入 SDK 结构体。
 *
 * @par 修改记录
 * 2026-08-28 qinjt：统一抓拍属性转换辅助函数的命名、注释和内部链接属性。
 * 2026-08-31 qinjt：补充抓拍转换接口的边界和图片生命周期说明。
 */

/* 禁用 Windows 的 min/max 宏。 */
#define NOMINMAX

/* 启用项目兼容性定义。 */
#ifdef _MSC_VER
    #ifndef __func__
        #define __func__ __FUNCTION__
    #endif
    #pragma warning(disable: 4244)
    #pragma warning(disable: 4800)
#endif

#include "AlarmInfoConvert.h"
#include "Base64Util.h"
#include "SDKConvert.h"

#include <algorithm>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>
#include <limits>
#include <new>

void SDKConvert::deal(Json::Object* pRootJson, NET_Alarmer_S& stAlarmInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    /* 处理序列号，将 BYTE 数组转换为字符串或将字符串写入 BYTE 数组。 */
    if (bOutStruct)
    {
        std::string strSerialNumber;
        convert.field(pRootJson, "SerialNumber", strSerialNumber);
        const std::size_t uCopyLength = std::min(
            strSerialNumber.size(), static_cast<std::size_t>(NET_LEN_64));
        memcpy(stAlarmInfo.strSerialNumber, strSerialNumber.c_str(), uCopyLength);
        if (uCopyLength < NET_LEN_64)
        {
            stAlarmInfo.strSerialNumber[uCopyLength] = '\0';
        }
    }
    else
    {
        std::string strSerialNumber(
            reinterpret_cast<const char*>(stAlarmInfo.strSerialNumber), NET_LEN_64);
        const std::size_t uLength =
            strnlen(strSerialNumber.c_str(), NET_LEN_64);
        strSerialNumber.resize(uLength);
        Json::add(pRootJson, "SerialNumber", strSerialNumber);
    }

    /* 处理设备名称字符数组。 */
    convert.field(pRootJson, "DeviceName", stAlarmInfo.strDeviceName);

    /* 处理 MAC 地址，支持冒号和短横线两种分隔格式。 */
    if (bOutStruct)
    {
        std::string strMacAddress;
        convert.field(pRootJson, "MacAddress", strMacAddress);
        std::string strDelimiter = ":";
        if (strMacAddress.find('-') != std::string::npos)
        {
            strDelimiter = "-";
        }

        std::stringstream stStream(strMacAddress);
        std::string strByte;
        int nIndex = 0;
        while (std::getline(stStream, strByte, strDelimiter[0]) &&
               nIndex < NET_LEN_6)
        {
            unsigned int uByteValue = 0;
            std::stringstream stHexStream(strByte);
            stHexStream >> std::hex >> uByteValue;
            stAlarmInfo.byMacAddr[nIndex++] =
                static_cast<BYTE>(uByteValue & 0xFF);
        }
        while (nIndex < NET_LEN_6)
        {
            stAlarmInfo.byMacAddr[nIndex++] = 0;
        }
    }
    else
    {
        std::ostringstream stOutputStream;
        for (int i = 0; i < NET_LEN_6; ++i)
        {
            if (i > 0)
            {
                stOutputStream << ":";
            }
            stOutputStream << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<unsigned int>(stAlarmInfo.byMacAddr[i]);
        }
        Json::add(pRootJson, "MacAddress", stOutputStream.str());
    }

    /* 处理设备 IP 地址字符数组。 */
    convert.field(pRootJson, "DeviceIP", stAlarmInfo.strDeviceIP);
}

namespace
{
/**
 * @brief 在 JSON 数组和固定长度字节数组之间转换。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in] strKey JSON 字段名称。
 * @param [in,out] aByteArray 待转换的字节数组。
 * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
 * @return 无返回值。
 */
template <std::size_t uArrayCapacity>
static void alarm_info_convert_byte_array(
    Json::Object* pRootJson,
    const std::string& strKey,
    BYTE (&aByteArray)[uArrayCapacity],
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        std::vector<int> aValues;
        Json::Object* pJsonArray = Json::get(pRootJson, strKey);
        if (pJsonArray)
        {
            Json::Array::get(pJsonArray, aValues);
        }
        for (std::size_t i = 0; i < uArrayCapacity; ++i)
        {
            int nValue = (i < aValues.size()) ? aValues[i] : 0;
            if (nValue < 0)
            {
                nValue = 0;
            }
            if (nValue > 255)
            {
                nValue = 255;
            }
            aByteArray[i] = static_cast<BYTE>(nValue);
        }
    }
    else
    {
        Json::Object* pJsonArray = Json::Array::init();
        for (std::size_t i = 0; i < uArrayCapacity; ++i)
        {
            Json::Array::add(pJsonArray, static_cast<int>(aByteArray[i]));
        }
        Json::add(pRootJson, strKey, pJsonArray);
    }
}

/**
 * @brief 在 JSON 字符串和固定长度字符数组之间转换。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in] strKey JSON 字段名称。
 * @param [in,out] aCharArray 待转换的字符数组。
 * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
 * @return 无返回值。
 */
template <std::size_t uArrayCapacity>
static void alarm_info_convert_char_array(
    Json::Object* pRootJson,
    const std::string& strKey,
    char (&aCharArray)[uArrayCapacity],
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert stConverter(bOutStruct);
    stConverter.field(pRootJson, strKey, aCharArray);
}

/**
 * @brief 在 JSON Base64 字符串和固定长度图片数组之间转换。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in] strKey JSON 字段名称。
 * @param [in,out] aImageData 图片字节数组。
 * @param [in,out] uImageLength 图片有效长度。
 * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
 * @return 无返回值。
 */
template <std::size_t uArrayCapacity>
static void alarm_info_convert_image_base64(
    Json::Object* pRootJson,
    const std::string& strKey,
    BYTE (&aImageData)[uArrayCapacity],
    UINT32& uImageLength,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        std::string strBase64;
        if (Json::get(pRootJson, strKey, strBase64) && !strBase64.empty())
        {
            std::vector<unsigned char> aDecodedData;
            if (SDKConvert::Base64Decode(strBase64, aDecodedData))
            {
                const std::size_t uCopyLength = std::min(
                    aDecodedData.size(), uArrayCapacity);
                if (uCopyLength > 0)
                {
                    std::memcpy(aImageData, aDecodedData.data(), uCopyLength);
                }
                uImageLength = static_cast<UINT32>(uCopyLength);
            }
        }
        return;
    }

    UINT32 uValidImageLength = uImageLength;
    if (uValidImageLength > static_cast<UINT32>(uArrayCapacity))
    {
        uValidImageLength = static_cast<UINT32>(uArrayCapacity);
    }
    if (uValidImageLength > 0)
    {
        const std::string strBase64 = SDKConvert::Base64Encode(
            reinterpret_cast<const unsigned char*>(aImageData),
            static_cast<std::size_t>(uValidImageLength));
        Json::add(pRootJson, strKey, strBase64);
    }
}

/**
 * @brief 在 JSON Base64 字符串和指针图片缓冲区之间转换。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in] strKey JSON 字段名称。
 * @param [in,out] stImage 图片缓冲区描述。
 * @param [in] bOutStruct true 表示 JSON 转结构体并分配内存，false 表示结构体转 JSON。
 * @return 无返回值。
 */
static void alarm_info_convert_image_buffer_base64(
    Json::Object* pRootJson,
    const std::string& strKey,
    NET_ImageBuffer_S& stImage,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        stImage.pData = nullptr;
        stImage.uDataLen = 0;

        std::string strBase64;
        if (!Json::get(pRootJson, strKey, strBase64) || strBase64.empty())
        {
            return;
        }

        std::vector<unsigned char> aDecodedData;
        if (!SDKConvert::Base64Decode(strBase64, aDecodedData) ||
            aDecodedData.empty() ||
            aDecodedData.size() >
                static_cast<std::size_t>(std::numeric_limits<UINT32>::max()))
        {
            return;
        }

        BYTE* pImageData = new (std::nothrow) BYTE[aDecodedData.size()];
        if (!pImageData)
        {
            return;
        }

        std::memcpy(pImageData, aDecodedData.data(), aDecodedData.size());
        stImage.pData = pImageData;
        stImage.uDataLen = static_cast<UINT32>(aDecodedData.size());
        return;
    }

    if (stImage.pData && stImage.uDataLen > 0)
    {
        const std::string strBase64 = SDKConvert::Base64Encode(
            reinterpret_cast<const unsigned char*>(stImage.pData),
            stImage.uDataLen);
        Json::add(pRootJson, strKey, strBase64);
    }
}

/**
 * @brief 在 JSON 对象和抓拍多边形区域之间转换。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in] strKey 多边形区域字段名称。
 * @param [in,out] stPolygon 抓拍多边形区域。
 * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
 * @return 无返回值。
 */
static void alarm_info_convert_capture_polygon(
    Json::Object* pRootJson,
    const std::string& strKey,
    NET_CapturePolygon_S& stPolygon,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        Json::Object* pJsonPolygon = Json::get(pRootJson, strKey);
        if (!pJsonPolygon)
        {
            return;
        }

        UINT32 uPointCount = 0;
        Json::get(pJsonPolygon, "PointCount", uPointCount);
        const UINT32 uMaxPointCount = NET_CAPTURE_REGION_POINT_MAX_NUM;
        const UINT32 uXCount = static_cast<UINT32>(Json::Array::size(
            Json::get(pJsonPolygon, "PointX")));
        const UINT32 uYCount = static_cast<UINT32>(Json::Array::size(
            Json::get(pJsonPolygon, "PointY")));
        uPointCount = std::min(uPointCount, uMaxPointCount);
        uPointCount = std::min(uPointCount, std::min(uXCount, uYCount));

        Json::Object* pJsonXValues = Json::get(pJsonPolygon, "PointX");
        Json::Object* pJsonYValues = Json::get(pJsonPolygon, "PointY");
        for (UINT32 i = 0; i < uPointCount; ++i)
        {
            Json::Object* pJsonX = Json::Array::get(
                pJsonXValues, static_cast<int>(i));
            Json::Object* pJsonY = Json::Array::get(
                pJsonYValues, static_cast<int>(i));
            if (pJsonX && pJsonY)
            {
                double dPointX = 0.0;
                double dPointY = 0.0;
                Json::Value::get(pJsonX, dPointX);
                Json::Value::get(pJsonY, dPointY);
                stPolygon.afPointX[i] = static_cast<FLOAT>(dPointX);
                stPolygon.afPointY[i] = static_cast<FLOAT>(dPointY);
            }
        }
        stPolygon.uPointCount = uPointCount;
        return;
    }

    const UINT32 uPointCount = std::min(
        stPolygon.uPointCount,
        static_cast<UINT32>(NET_CAPTURE_REGION_POINT_MAX_NUM));
    Json::Object* pJsonPolygon = Json::init();
    Json::Object* pJsonXValues = Json::Array::init();
    Json::Object* pJsonYValues = Json::Array::init();
    if (!pJsonPolygon || !pJsonXValues || !pJsonYValues)
    {
        if (pJsonPolygon)
        {
            Json::deinit(pJsonPolygon);
        }
        if (pJsonXValues)
        {
            Json::deinit(pJsonXValues);
        }
        if (pJsonYValues)
        {
            Json::deinit(pJsonYValues);
        }
        return;
    }

    Json::add(pJsonPolygon, "PointCount", uPointCount);
    for (UINT32 i = 0; i < uPointCount; ++i)
    {
        Json::Array::add(pJsonXValues, stPolygon.afPointX[i]);
        Json::Array::add(pJsonYValues, stPolygon.afPointY[i]);
    }
    Json::add(pJsonPolygon, "PointX", pJsonXValues);
    Json::add(pJsonPolygon, "PointY", pJsonYValues);
    Json::add(pRootJson, strKey, pJsonPolygon);
}

/**
 * @brief 在 JSON 对象和抓拍扩展属性之间转换。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in,out] stExtraInfo 抓拍扩展属性结构体。
 * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
 * @return 无返回值。
 */
static void alarm_info_convert_capture_extra_info(
    Json::Object* pRootJson,
    NET_CaptureExtraInfo_S& stExtraInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConverter(bOutStruct);
    stConverter.field(pRootJson, "Male", stExtraInfo.bMale);
    stConverter.field(pRootJson, "AgeLabel", stExtraInfo.nAgeLabel);
    stConverter.field(pRootJson, "Glasses", stExtraInfo.bGlasses);
    stConverter.field(pRootJson, "Beard", stExtraInfo.bBeard);
    stConverter.field(pRootJson, "Mask", stExtraInfo.bMask);
    stConverter.field(pRootJson, "EmotionLabel", stExtraInfo.nEmotionLabel);
    stConverter.field(pRootJson, "Bag", stExtraInfo.bBag);
    stConverter.field(pRootJson, "TopColorLabel", stExtraInfo.nTopColorLabel);
    stConverter.field(
        pRootJson, "BottomColorLabel", stExtraInfo.nBottomColorLabel);
    stConverter.field(pRootJson, "VehicleType", stExtraInfo.nVehicleType);
    stConverter.field(pRootJson, "VehicleColor", stExtraInfo.nVehicleColor);
    alarm_info_convert_char_array(
        pRootJson, "VehicleBrand", stExtraInfo.strVehicleBrand, bOutStruct);
    alarm_info_convert_char_array(pRootJson, "LicensePlateNumber",
                   stExtraInfo.strLicensePlateNumber, bOutStruct);
    alarm_info_convert_char_array(
        pRootJson, "Timestamp", stExtraInfo.strTimestamp, bOutStruct);
    alarm_info_convert_capture_polygon(
        pRootJson, "TargetRegion", stExtraInfo.stTargetRegion, bOutStruct);
}
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmBasicInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "AlarmInputNumber", (int&)stInfo.uAlarmInputNumber);

    alarm_info_convert_byte_array(pRootJson, "AlarmOutputNumber", stInfo.byAlarmOutputNumber, bOutStruct);
    alarm_info_convert_byte_array(pRootJson, "AlarmRelateChannel", stInfo.byAlarmRelateChannel, bOutStruct);
    alarm_info_convert_byte_array(pRootJson, "Channel", stInfo.byChannel, bOutStruct);
    alarm_info_convert_byte_array(pRootJson, "DiskNumber", stInfo.byDiskNumber, bOutStruct);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.uPanoramaImgLen);
    alarm_info_convert_image_base64(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmRuleInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    convert.field(pRootJson, "RuleID", (int&)stInfo.uRuleID);
    convert.field(pRootJson, "RuleType", (int&)stInfo.uRuleType);
    alarm_info_convert_char_array(pRootJson, "RuleName", stInfo.strRuleName, bOutStruct);
    convert.field(pRootJson, "TargetID", (int&)stInfo.uTargetID);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.uObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.uPanoramaImgLen);
    alarm_info_convert_image_base64(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "TargetImgLen", (int&)stInfo.uTargetImgLen);
    alarm_info_convert_image_base64(pRootJson, "TargetImgBase64", stInfo.byTargetImg, stInfo.uTargetImgLen, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmAiObjectInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.uObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    alarm_info_convert_char_array(pRootJson, "ObjectID", stInfo.strObjectID, bOutStruct);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.uPanoramaImgLen);
    alarm_info_convert_image_base64(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "ImgLen", (int&)stInfo.uImgLen);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);

    alarm_info_convert_image_base64(pRootJson, "ImgDataBase64", stInfo.byImgData, stInfo.uImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmFaceCompareInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "EventId", stInfo.nEventId);
    convert.field(pRootJson, "CompareResult", stInfo.nCompResult);
    convert.field(pRootJson, "Similarity", stInfo.nSimilarity);
    convert.field(pRootJson, "FaceID", stInfo.nFaceId);
    alarm_info_convert_char_array(pRootJson, "FaceName", stInfo.strFaceName, bOutStruct);
    alarm_info_convert_char_array(pRootJson, "FaceLibName", stInfo.strFaceLibName, bOutStruct);
    alarm_info_convert_char_array(pRootJson, "LibFacePath", stInfo.strLibFacePath, bOutStruct);
    alarm_info_convert_char_array(pRootJson, "CapFacePath", stInfo.strCapFacePath, bOutStruct);
    alarm_info_convert_char_array(pRootJson, "CapImagePath", stInfo.strCapImagePath, bOutStruct);
    convert.field(pRootJson, "LibFaceImgLen", (int&)stInfo.uLibFaceImgLen);
    convert.field(pRootJson, "CapFaceImgLen", (int&)stInfo.uCapFaceImgLen);
    alarm_info_convert_image_base64(pRootJson, "LibFaceImgBase64", stInfo.byLibFaceImg, stInfo.uLibFaceImgLen, bOutStruct);
    alarm_info_convert_image_base64(pRootJson, "CapFaceImgBase64", stInfo.byCapFaceImg, stInfo.uCapFaceImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmPlateInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    alarm_info_convert_char_array(pRootJson, "PlateNumber", stInfo.strPlateNumber, bOutStruct);
    convert.field(pRootJson, "PlateColor", (int&)stInfo.uPlateColor);
    convert.field(pRootJson, "VehicleType", (int&)stInfo.uVehicleType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Speed", (int&)stInfo.uSpeed);
    convert.field(pRootJson, "LaneNo", (int&)stInfo.uLaneNo);
    convert.field(pRootJson, "PlateImgLen", (int&)stInfo.uPlateImgLen);

    if (bOutStruct)
    {
        std::string b64;
        if (Json::get(pRootJson, "PlateImgBase64", b64) && !b64.empty())
        {
            std::vector<unsigned char> decoded;
            if (SDKConvert::Base64Decode(b64, decoded))
            {
                size_t copyLen = std::min(decoded.size(), (size_t)NET_VEH_PLATE_IMAGE_LEN);
                if (copyLen > 0)
                {
                    std::memcpy(stInfo.byPlateImg, decoded.data(), copyLen);
                }
                stInfo.uPlateImgLen = (UINT32)copyLen;
            }
        }
    }
    else
    {
        UINT32 len = stInfo.uPlateImgLen;
        if (len > (UINT32)NET_VEH_PLATE_IMAGE_LEN) len = (UINT32)NET_VEH_PLATE_IMAGE_LEN;
        if (len > 0)
        {
            std::string b64 = SDKConvert::Base64Encode((const unsigned char*)stInfo.byPlateImg, (size_t)len);
            Json::add(pRootJson, "PlateImgBase64", b64);
        }
    }
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmExceptionInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    convert.field(pRootJson, "DiskNo", (int&)stInfo.uDiskNo);
    convert.field(pRootJson, "Status", (int&)stInfo.uStatus);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmStatisticsTarget_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "TrackID", stInfo.nTrackID);
    convert.field(pRootJson, "RuleID", (int&)stInfo.uRuleID);
    convert.field(pRootJson, "SnapshotType", (int&)stInfo.uSnapshotType);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "Direction", stInfo.nDirection);

    alarm_info_convert_image_base64(pRootJson, "ImgDataBase64", stInfo.byImgData, stInfo.uImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmStatisticsInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    convert.field(pRootJson, "StatisticsType", (int&)stInfo.uStatisticsType);
    convert.field(pRootJson, "RuleID", (int&)stInfo.uRuleID);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "ReportSeq", (int&)stInfo.uReportSeq);
    convert.field(pRootJson, "EnterCount", (int&)stInfo.uEnterCount);
    convert.field(pRootJson, "LeaveCount", (int&)stInfo.uLeaveCount);
    convert.field(pRootJson, "TotalCount", (int&)stInfo.uTotalCount);
    convert.field(pRootJson, "CurrentPeopleCount", (int&)stInfo.uCurrentPeopleCount);
    convert.field(pRootJson, "AverageStayTimeSec", (int&)stInfo.uAverageStayTimeSec);
    convert.field(pRootJson, "TargetCount", (int&)stInfo.uTargetCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Targets");
        int nSize = Json::Array::size(pArray);
        UINT32 count = stInfo.uTargetCount;
        if (count == 0 || count > (UINT32)nSize)
        {
            count = (UINT32)nSize;
        }
        if (count > NET_ALARM_STATISTICS_TARGET_MAX_NUM)
        {
            count = NET_ALARM_STATISTICS_TARGET_MAX_NUM;
        }

        for (UINT32 i = 0; i < count; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, (int)i);
            SDKConvert::deal(pItem, stInfo.stTargets[i], true);
        }
        stInfo.uTargetCount = count;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        UINT32 count = stInfo.uTargetCount;
        if (count > NET_ALARM_STATISTICS_TARGET_MAX_NUM)
        {
            count = NET_ALARM_STATISTICS_TARGET_MAX_NUM;
        }

        for (UINT32 i = 0; i < count; ++i)
        {
            Json::Object* pItem = Json::init();
            SDKConvert::deal(pItem, stInfo.stTargets[i], false);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Targets", pArray);
    }

    if (bOutStruct)
    {
        std::string b64;
        if (Json::get(pRootJson, "PanoramaImgBase64", b64) && !b64.empty())
        {
            std::vector<unsigned char> decoded;
            if (SDKConvert::Base64Decode(b64, decoded))
            {
                size_t copyLen = std::min(decoded.size(), (size_t)NET_PIC_DATA_MAX_LEN);
                if (copyLen > 0)
                {
                    std::memcpy(stInfo.byPanoramaImg, decoded.data(), copyLen);
                }
                stInfo.uPanoramaImgLen = (UINT32)copyLen;
            }
        }
    }
    else
    {
        UINT32 len = stInfo.uPanoramaImgLen;
        if (len > (UINT32)NET_PIC_DATA_MAX_LEN) len = (UINT32)NET_PIC_DATA_MAX_LEN;
        if (len > 0)
        {
            std::string b64 = SDKConvert::Base64Encode((const unsigned char*)stInfo.byPanoramaImg, (size_t)len);
            Json::add(pRootJson, "PanoramaImgBase64", b64);
        }
    }
}

/**
 * @brief 在 JSON 与通用抓拍告警结构之间转换。
 * @param [in,out] pRootJson JSON 根对象；根据 bOutStruct 作为输入或输出。
 * @param [in,out] stInfo 通用抓拍告警结构体；根据 bOutStruct 作为输入或输出。
 * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
 * @return 无返回值。
 * @details 图片通过 Base64 传输。JSON 转结构体时由转换器分配图片内存，调用方必须在
 *          回调结束后释放 stPanoramaImg 和各裁剪图中的 pData。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmCaptureInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "CaptureType", stInfo.uCaptureType);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "PanoramaWidth", stInfo.uPanoramaWidth);
    convert.field(pRootJson, "PanoramaHeight", stInfo.uPanoramaHeight);
    convert.field(pRootJson, "PanoramaImgLen", stInfo.stPanoramaImg.uDataLen);
    alarm_info_convert_image_buffer_base64(
        pRootJson, "PanoramaImgBase64", stInfo.stPanoramaImg, bOutStruct);

    if (bOutStruct)
    {
        Json::Object* pCrops = Json::get(pRootJson, "Crops");
        const UINT32 uJsonCropCount = pCrops
            ? static_cast<UINT32>(Json::Array::size(pCrops))
            : 0;
        UINT32 uCropCount = stInfo.uCropCount;
        if (uCropCount == 0 || uCropCount > uJsonCropCount)
        {
            uCropCount = uJsonCropCount;
        }
        uCropCount = std::min(
            uCropCount, static_cast<UINT32>(NET_CAPTURE_CROP_MAX_NUM));

        for (UINT32 i = 0; i < uCropCount; ++i)
        {
            Json::Object* pCrop = Json::Array::get(pCrops, static_cast<int>(i));
            if (!pCrop)
            {
                continue;
            }

            NET_CropImage_S& stCrop = stInfo.stCropImages[i];
            convert.field(pCrop, "CropX", stCrop.uCropX);
            convert.field(pCrop, "CropY", stCrop.uCropY);
            convert.field(pCrop, "CropWidth", stCrop.uCropWidth);
            convert.field(pCrop, "CropHeight", stCrop.uCropHeight);
            convert.field(pCrop, "TargetType", stCrop.uTargetType);
            convert.field(pCrop, "Confidence", stCrop.fConfidence);
            convert.field(pCrop, "TrackID", stCrop.nTrackID);
            convert.field(pCrop, "ImgLen", stCrop.stImage.uDataLen);
            alarm_info_convert_image_buffer_base64(
                pCrop, "ImgBase64", stCrop.stImage, true);
        }
        stInfo.uCropCount = uCropCount;
    }
    else
    {
        stInfo.uCropCount = std::min(
            stInfo.uCropCount,
            static_cast<UINT32>(NET_CAPTURE_CROP_MAX_NUM));
        convert.field(pRootJson, "CropCount", stInfo.uCropCount);
        Json::Object* pCrops = Json::Array::init();
        if (pCrops)
        {
            for (UINT32 i = 0; i < stInfo.uCropCount; ++i)
            {
                NET_CropImage_S& stCrop = stInfo.stCropImages[i];
                Json::Object* pCrop = Json::init();
                if (!pCrop)
                {
                    continue;
                }
                convert.field(pCrop, "CropX", stCrop.uCropX);
                convert.field(pCrop, "CropY", stCrop.uCropY);
                convert.field(pCrop, "CropWidth", stCrop.uCropWidth);
                convert.field(pCrop, "CropHeight", stCrop.uCropHeight);
                convert.field(pCrop, "TargetType", stCrop.uTargetType);
                convert.field(pCrop, "Confidence", stCrop.fConfidence);
                convert.field(pCrop, "TrackID", stCrop.nTrackID);
                convert.field(pCrop, "ImgLen", stCrop.stImage.uDataLen);
                alarm_info_convert_image_buffer_base64(
                    pCrop, "ImgBase64", stCrop.stImage, false);
                Json::Array::add(pCrops, pCrop);
            }
            Json::add(pRootJson, "Crops", pCrops);
        }
    }

    Json::Object* pExtra = Json::get(pRootJson, "ExtraInfo");
    if (bOutStruct)
    {
        if (pExtra)
        {
            alarm_info_convert_capture_extra_info(
                pExtra, stInfo.stExtraInfo, true);
        }
    }
    else
    {
        pExtra = Json::init();
        if (pExtra)
        {
            alarm_info_convert_capture_extra_info(
                pExtra, stInfo.stExtraInfo, false);
            Json::add(pRootJson, "ExtraInfo", pExtra);
        }
    }
}

/* ==================== 从 DeviceInfoConvert 搬运: 录像/报警/AI/NVR ==================== */

/**
 * @brief 将 JSON 浮点数组转换为固定容量的浮点数组。
 * @param [in] pRootJson JSON 根对象。
 * @param [in] pstrKey JSON 数组字段名称。
 * @param [out] pValues 输出浮点数组。
 * @param [in] nMaxCount 输出数组最大元素数量。
 * @return 无返回值。
 */
static void alarm_info_json_to_float_array(
    Json::Object* pRootJson,
    const char* pstrKey,
    FLOAT* pValues,
    int nMaxCount)
{
    Json::Object* pArray = Json::get(pRootJson, pstrKey);
    if (!pArray)
    {
        return;
    }

    const int nSize = Json::Array::size(pArray);
    for (int i = 0; i < nSize && i < nMaxCount; ++i)
    {
        Json::Object* pItem = Json::Array::get(pArray, i);
        if (pItem)
        {
            double dValue = 0.0;
            Json::Value::get(pItem, dValue);
            pValues[i] = static_cast<FLOAT>(dValue);
        }
    }
}

/**
 * @brief 将固定容量的浮点数组转换为 JSON 数组。
 * @param [in,out] pRootJson JSON 根对象。
 * @param [in] pstrKey JSON 数组字段名称。
 * @param [in] pValues 待转换的浮点数组。
 * @param [in] nCount 有效元素数量。
 * @param [in] nMaxCount 数组最大元素数量。
 * @return 无返回值。
 */
static void alarm_info_float_array_to_json(
    Json::Object* pRootJson,
    const char* pstrKey,
    const FLOAT* pValues,
    int nCount,
    int nMaxCount)
{
    Json::Object* pArray = Json::Array::init();
    if (nCount > nMaxCount)
    {
        nCount = nMaxCount;
    }
    for (int i = 0; i < nCount; ++i)
    {
        Json::Array::add(pArray, static_cast<float>(pValues[i]));
    }
    Json::add(pRootJson, pstrKey, pArray);
}


/**
 * @brief 告警联动通道数组转换参数。
 */
typedef struct AlarmCopyToConvertParam_S
{
    const char* pJsonKey;
    INT32& nCopyToCount;
    INT32* pCopyTo;
    INT32 nCapacity;
    bool bOutStruct;
} AlarmCopyToConvertParam_S;

/**
 * @brief 将数组元素数量限制在 SDK 二进制接口定义的最大容量内。
 * @author ITC
 * @param [in] nCount 待限制的数组元素数量。
 * @param [in] nMaximum 数组允许的最大元素数量。
 * @return 返回位于零和 nMaximum 之间的元素数量。
 */
static INT32 clamp_alarm_config_count(INT32 nCount, INT32 nMaximum)
{
    if (nCount < 0)
    {
        return 0;
    }
    if (nCount > nMaximum)
    {
        return nMaximum;
    }
    return nCount;
}

/**
 * @brief 在 JSON 与 SDK 固定长度联动通道数组之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 stConvertParam.bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stConvertParam 固定长度联动通道数组的转换参数。
 * @return 无。
 */
static void deal_alarm_copy_to(Json::Object* pRootJson,
                               AlarmCopyToConvertParam_S& stConvertParam)
{
    if (!pRootJson || !stConvertParam.pJsonKey || !stConvertParam.pCopyTo ||
        stConvertParam.nCapacity <= 0)
    {
        return;
    }

    if (stConvertParam.bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, stConvertParam.pJsonKey);
        const INT32 nArrayCount = pArray ? (INT32)Json::Array::size(pArray) : 0;
        stConvertParam.nCopyToCount = clamp_alarm_config_count(nArrayCount,
                                                                stConvertParam.nCapacity);
        for (INT32 nIndex = 0; nIndex < stConvertParam.nCopyToCount; ++nIndex)
        {
            Json::Object* pItem = Json::Array::get(pArray, nIndex);
            if (pItem)
            {
                Json::Value::get(pItem, stConvertParam.pCopyTo[nIndex]);
            }
        }
        return;
    }

    stConvertParam.nCopyToCount = clamp_alarm_config_count(stConvertParam.nCopyToCount,
                                                            stConvertParam.nCapacity);
    Json::Object* pArray = Json::Array::init();
    if (!pArray)
    {
        return;
    }
    for (INT32 nIndex = 0; nIndex < stConvertParam.nCopyToCount; ++nIndex)
    {
        Json::Array::add(pArray, stConvertParam.pCopyTo[nIndex]);
    }
    Json::add(pRootJson, stConvertParam.pJsonKey, pArray);
}

/**
 * @brief 在 JSON 与 SDK 自定义声音告警音频信息之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标自定义音频结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudibleAlarmCustomAudio_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "Selected", stInfo.bSelected);
    stConvert.field(pRootJson, "Name", stInfo.strName);
    stConvert.field(pRootJson, "Path", stInfo.strPath);
}


/**
 * @brief 在 JSON 与 SDK 声音告警配置之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标声音告警配置结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudibleAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "SoundType", stInfo.enSoundType);
    stConvert.field(pRootJson, "AlertSound", stInfo.enAlertSound);
    stConvert.field(pRootJson, "Times", stInfo.nTimes);
    stConvert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "CustomAudios");
        const INT32 nArrayCount = pArray ? (INT32)Json::Array::size(pArray) : 0;
        stInfo.nCustomAudioCount = clamp_alarm_config_count(nArrayCount,
                                                             NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM);
        for (INT32 nIndex = 0; nIndex < stInfo.nCustomAudioCount; ++nIndex)
        {
            Json::Object* pItem = Json::Array::get(pArray, nIndex);
            if (pItem)
            {
                deal(pItem, stInfo.astCustomAudios[nIndex], bOutStruct);
            }
        }
        return;
    }

    stInfo.nCustomAudioCount = clamp_alarm_config_count(stInfo.nCustomAudioCount,
                                                         NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM);
    stConvert.field(pRootJson, "CustomAudioCount", stInfo.nCustomAudioCount);
    Json::Object* pArray = Json::Array::init();
    if (!pArray)
    {
        return;
    }
    for (INT32 nIndex = 0; nIndex < stInfo.nCustomAudioCount; ++nIndex)
    {
        Json::Object* pItem = Json::init();
        if (!pItem)
        {
            continue;
        }
        deal(pItem, stInfo.astCustomAudios[nIndex], bOutStruct);
        Json::Array::add(pArray, pItem);
    }
    Json::add(pRootJson, "CustomAudios", pArray);
}


/**
 * @brief 在 JSON 与 SDK 单路报警输入配置之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输入配置结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmInputInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "AlarmNumber", stInfo.nAlarmNumber);
    stConvert.field(pRootJson, "AlarmAddress", stInfo.strAlarmAddress);
    stConvert.field(pRootJson, "AlarmName", stInfo.strAlarmName);
    stConvert.field(pRootJson, "NormallyOpen", stInfo.bNormallyOpen);
    stConvert.field(pRootJson, "DealType", stInfo.nDealType);
    stConvert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    stConvert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
    AlarmCopyToConvertParam_S stCopyToConvertParam = {
        "CopyTo", stInfo.nCopyToCount, stInfo.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM, bOutStruct};
    deal_alarm_copy_to(pRootJson, stCopyToConvertParam);
}


/**
 * @brief 在 JSON 与 SDK 报警输入配置集合之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输入配置集合结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmInputInfoList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "AlarmInputs");
        const INT32 nArrayCount = pArray ? (INT32)Json::Array::size(pArray) : 0;
        stInfo.nAlarmInputCount = clamp_alarm_config_count(nArrayCount, NET_MAX_ALARM_IN_NUM);
        for (INT32 nIndex = 0; nIndex < stInfo.nAlarmInputCount; ++nIndex)
        {
            Json::Object* pItem = Json::Array::get(pArray, nIndex);
            if (pItem)
            {
                deal(pItem, stInfo.astAlarmInputs[nIndex], bOutStruct);
            }
        }
        return;
    }

    stInfo.nAlarmInputCount = clamp_alarm_config_count(stInfo.nAlarmInputCount,
                                                        NET_MAX_ALARM_IN_NUM);
    stConvert.field(pRootJson, "AlarmInputCount", stInfo.nAlarmInputCount);
    Json::Object* pArray = Json::Array::init();
    if (!pArray)
    {
        return;
    }
    for (INT32 nIndex = 0; nIndex < stInfo.nAlarmInputCount; ++nIndex)
    {
        Json::Object* pItem = Json::init();
        if (!pItem)
        {
            continue;
        }
        deal(pItem, stInfo.astAlarmInputs[nIndex], bOutStruct);
        Json::Array::add(pArray, pItem);
    }
    Json::add(pRootJson, "AlarmInputs", pArray);
}


/**
 * @brief 在 JSON 与 SDK 单路报警输出配置之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输出配置结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmOutputInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "AlarmNumber", stInfo.nAlarmNumber);
    stConvert.field(pRootJson, "AlarmAddress", stInfo.strAlarmAddress);
    stConvert.field(pRootJson, "AlarmName", stInfo.strAlarmName);
    stConvert.field(pRootJson, "DelayTime", stInfo.nDelayTime);
    stConvert.field(pRootJson, "State", stInfo.enState);
    stConvert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    AlarmCopyToConvertParam_S stCopyToConvertParam = {
        "CopyTo", stInfo.nCopyToCount, stInfo.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM, bOutStruct};
    deal_alarm_copy_to(pRootJson, stCopyToConvertParam);
}


/**
 * @brief 在 JSON 与 SDK 报警输出配置集合之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输出配置集合结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmOutputInfoList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "AlarmOutputs");
        const INT32 nArrayCount = pArray ? (INT32)Json::Array::size(pArray) : 0;
        stInfo.nAlarmOutputCount = clamp_alarm_config_count(nArrayCount,
                                                             NET_MAX_ALARM_OUT_NUM);
        for (INT32 nIndex = 0; nIndex < stInfo.nAlarmOutputCount; ++nIndex)
        {
            Json::Object* pItem = Json::Array::get(pArray, nIndex);
            if (pItem)
            {
                deal(pItem, stInfo.astAlarmOutputs[nIndex], bOutStruct);
            }
        }
        return;
    }

    stInfo.nAlarmOutputCount = clamp_alarm_config_count(stInfo.nAlarmOutputCount,
                                                         NET_MAX_ALARM_OUT_NUM);
    stConvert.field(pRootJson, "AlarmOutputCount", stInfo.nAlarmOutputCount);
    Json::Object* pArray = Json::Array::init();
    if (!pArray)
    {
        return;
    }
    for (INT32 nIndex = 0; nIndex < stInfo.nAlarmOutputCount; ++nIndex)
    {
        Json::Object* pItem = Json::init();
        if (!pItem)
        {
            continue;
        }
        deal(pItem, stInfo.astAlarmOutputs[nIndex], bOutStruct);
        Json::Array::add(pArray, pItem);
    }
    Json::add(pRootJson, "AlarmOutputs", pArray);
}


/**
 * @brief 在 JSON 与 SDK 闪光灯告警配置之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标闪光灯告警配置结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_FlashingLightAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "FlashTime", stInfo.nFlashTime);
    stConvert.field(pRootJson, "FlashFrequency", stInfo.enFlashFrequency);
    stConvert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    AlarmCopyToConvertParam_S stCopyToConvertParam = {
        "CopyTo", stInfo.nCopyToCount, stInfo.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM, bOutStruct};
    deal_alarm_copy_to(pRootJson, stCopyToConvertParam);
}


/**
 * @brief 在 JSON 与 SDK PIR 告警配置之间转换。
 * @author ITC
 * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
 * @param [in,out] stInfo 根据 bOutStruct 作为源或目标 PIR 告警配置结构体。
 * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
 * @return 无。
 */
void SDKConvert::deal(Json::Object* pRootJson, NET_PirAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "Enable", stInfo.bEnable);
    stConvert.field(pRootJson, "AlarmName", stInfo.strAlarmName);
    stConvert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    stConvert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
    AlarmCopyToConvertParam_S stCopyToConvertParam = {
        "CopyTo", stInfo.nCopyToCount, stInfo.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM, bOutStruct};
    deal_alarm_copy_to(pRootJson, stCopyToConvertParam);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LinkageList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    if (bOutStruct)
    {
        convert.field(pRootJson, "AlarmOutputCount", stInfo.uAlarmOutputCount);
        convert.field_array(pRootJson, "AlarmOutput", stInfo.auAlarmOutput,
                           stInfo.uAlarmOutputCount, NET_MAX_ALARM_OUT_NUM);
        convert.field(pRootJson, "RecordChannelCount", stInfo.uRecordChannelCount);
        convert.field_array(pRootJson, "RecordChannel", stInfo.auRecordChannel,
                           stInfo.uRecordChannelCount, NET_CHANNEL_MAX);
        convert.field(pRootJson, "SnapshotChannelCount", stInfo.uSnapshotChannelCount);
        convert.field_array(pRootJson, "SnapshotChannel", stInfo.auSnapshotChannel,
                           stInfo.uSnapshotChannelCount, NET_CHANNEL_MAX);
    }
    else
    {
        convert.field(pRootJson, "AlarmOutputCount", stInfo.uAlarmOutputCount);
        convert.field_array(pRootJson, "AlarmOutput", stInfo.auAlarmOutput,
                           stInfo.uAlarmOutputCount, NET_MAX_ALARM_OUT_NUM);
        convert.field(pRootJson, "RecordChannelCount", stInfo.uRecordChannelCount);
        convert.field_array(pRootJson, "RecordChannel", stInfo.auRecordChannel,
                           stInfo.uRecordChannelCount, NET_CHANNEL_MAX);
        convert.field(pRootJson, "SnapshotChannelCount", stInfo.uSnapshotChannelCount);
        convert.field_array(pRootJson, "SnapshotChannel", stInfo.auSnapshotChannel,
                           stInfo.uSnapshotChannelCount, NET_CHANNEL_MAX);
    }
}


/* ==================== 移动侦测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_MotionRegion_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AreaNo", stInfo.nAreaNo);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
    convert.field(pRootJson, "CloseSensitivity", stInfo.nCloseSensitivity);
    convert.field(pRootJson, "DaytimeSensitivity", stInfo.nDaytimeSensitivity);
    convert.field(pRootJson, "NightSensitivity", stInfo.nNightSensitivity);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_MotionExpertMode_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ExpertDayNightCtrl", stInfo.nExpertDayNightCtrl);
    convert.structure(pRootJson, "DayTime", stInfo.stDayTime);
    convert.field(pRootJson, "RegionCount", stInfo.uRegionCount);

    if (bOutStruct)
    {
        Json::Object* pRegions = Json::get(pRootJson, "Regions");
        if (pRegions)
        {
            int count = stInfo.uRegionCount;
            if (count > 16) count = 16;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRegion = Json::get(pRegions, key);
                if (pRegion)
                {
                    deal(pRegion, stInfo.astRegion[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRegions = Json::init();
        int count = stInfo.uRegionCount;
        if (count > 16) count = 16;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRegion = Json::init();
            deal(pRegion, stInfo.astRegion[i], bOutStruct);
            Json::add(pRegions, std::to_string(i).c_str(), pRegion);
        }
        Json::add(pRootJson, "Regions", pRegions);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_MotionNormalMode_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "RegionType", stInfo.nRegionType);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
    convert.field(pRootJson, "GridWidth", stInfo.uGridWidth);
    convert.field(pRootJson, "GridHeight", stInfo.uGridHeight);

    if (bOutStruct)
    {
        Json::Object* pGridArea = Json::get(pRootJson, "GridArea");
        if (pGridArea)
        {
            for (int i = 0; i < 18; i++)
            {
                std::string rowKey = std::to_string(i);
                Json::Object* pRow = Json::get(pGridArea, rowKey);
                if (pRow)
                {
                    for (int j = 0; j < 22; j++)
                    {
                        std::string colKey = std::to_string(j);
                        int val = 0;
                        Json::get(pRow, colKey, val);
                        stInfo.abyGridArea[i][j] = (BYTE)val;
                    }
                }
            }
        }
    }
    else
    {
        Json::Object* pGridArea = Json::init();
        for (int i = 0; i < 18; i++)
        {
            Json::Object* pRow = Json::init();
            for (int j = 0; j < 22; j++)
            {
                Json::add(pRow, std::to_string(j).c_str(), (int)stInfo.abyGridArea[i][j]);
            }
            Json::add(pGridArea, std::to_string(i).c_str(), pRow);
        }
        Json::add(pRootJson, "GridArea", pGridArea);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_MotionAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "DynamicAnalysisEnable", stInfo.bDynamicAnalysisEnable);
    convert.field(pRootJson, "Mode", stInfo.uMode);
    convert.structure(pRootJson, "NormalMode", stInfo.stNormalMode);
    convert.structure(pRootJson, "ExpertMode", stInfo.stExpertMode);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ==================== 遮挡报警相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_TamperAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.uSensitivity);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ==================== 越界检测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_BoundaryPlane_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);

    if (bOutStruct)
    {
        double dVal = 0.0;
        if (Json::get(pRootJson, "StartPosX", dVal))
        {
            stInfo.fStartPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "StartPosY", dVal))
        {
            stInfo.fStartPosY = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosX", dVal))
        {
            stInfo.fEndPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosY", dVal))
        {
            stInfo.fEndPosY = (FLOAT)dVal;
        }
    }
    else
    {
        Json::add(pRootJson, "StartPosX", (double)stInfo.fStartPosX);
        Json::add(pRootJson, "StartPosY", (double)stInfo.fStartPosY);
        Json::add(pRootJson, "EndPosX", (double)stInfo.fEndPosX);
        Json::add(pRootJson, "EndPosY", (double)stInfo.fEndPosY);
    }

    convert.field(pRootJson, "CrossDirection", stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget,
                       stInfo.uDetectionTargetCount, 8);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_CrossLineAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ==================== 入侵检测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_IntrusionRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointX");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointX[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointX[i]));
        }
        Json::add(pRootJson, "PointX", pArray);
    }

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointY");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointY[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointY[i]));
        }
        Json::add(pRootJson, "PointY", pArray);
    }

    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget,
                       stInfo.uDetectionTargetCount, 8);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_IntrusionAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ==================== 徘徊侦测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_LoiteringRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointX");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointX[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointX[i]));
        }
        Json::add(pRootJson, "PointX", pArray);
    }

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointY");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointY[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointY[i]));
        }
        Json::add(pRootJson, "PointY", pArray);
    }

    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget,
                       stInfo.uDetectionTargetCount, 8);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LoiteringAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SceneChangeAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_CrowdGatheringRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }

    convert.field(pRootJson, "ObjectOccup", stInfo.nObjectOccup);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_CrowdGatheringAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.astRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.astRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_ParkingRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }

    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_ParkingAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 8) count = 8;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.astRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 8) count = 8;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.astRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_UnattendedObjectRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }

    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_UnattendedObjectAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_ObjectRemovalRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }

    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_ObjectRemovalAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ===================== 垃圾暴露检测配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageExposureRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageExposureCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ===================== 垃圾满溢检测配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageOverflowRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageOverflowCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ===================== 单规则智能检测配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_AiSimpleRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}


/* ===================== 智能事件配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_SmartRegion_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);
    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SmartRegionRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);
    if (bOutStruct)
    {
        alarm_info_json_to_float_array(pRootJson, "PointX", stInfo.afPointX, 32);
        alarm_info_json_to_float_array(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        alarm_info_float_array_to_json(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        alarm_info_float_array_to_json(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget, stInfo.uDetectionTargetCount, 8);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SmartLineRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    if (bOutStruct)
    {
        double dVal = 0.0;
        if (Json::get(pRootJson, "StartPosX", dVal))
        {
            stInfo.fStartPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "StartPosY", dVal))
        {
            stInfo.fStartPosY = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosX", dVal))
        {
            stInfo.fEndPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosY", dVal))
        {
            stInfo.fEndPosY = (FLOAT)dVal;
        }
    }
    else
    {
        Json::add(pRootJson, "StartPosX", (double)stInfo.fStartPosX);
        Json::add(pRootJson, "StartPosY", (double)stInfo.fStartPosY);
        Json::add(pRootJson, "EndPosX", (double)stInfo.fEndPosX);
        Json::add(pRootJson, "EndPosY", (double)stInfo.fEndPosY);
    }
    convert.field(pRootJson, "CrossDirection", stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}


/* ===================== Audio anomaly alarm config ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudioAnomalyAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AudioInputAnomaly", stInfo.bAudioInputAnomaly);
    convert.field(pRootJson, "UpEnable", stInfo.bUpEnable);
    convert.field(pRootJson, "UpSensitivity", stInfo.nUpSensitivity);
    convert.field(pRootJson, "UpThreshold", stInfo.nUpThreshold);
    convert.field(pRootJson, "DownEnable", stInfo.bDownEnable);
    convert.field(pRootJson, "DownSensitivity", stInfo.nDownSensitivity);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_EnterRegionAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LeaveRegionAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}

/* ==================== 布防时间和联动相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_SchedTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartHour", stInfo.nStartHour);
    convert.field(pRootJson, "StartMinute", stInfo.nStartMinute);
    convert.field(pRootJson, "EndHour", stInfo.nEndHour);
    convert.field(pRootJson, "EndMinute", stInfo.nEndMinute);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmSchedule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    /* 处理每天的时间段数量。 */
    if (bOutStruct)
    {
        /* 将 JSON 转换为结构体。 */
        Json::Object* pTimeSectionCount = Json::get(pRootJson, "TimeSectionCount");
        if (pTimeSectionCount)
        {
            for (int day = 0; day < 7; day++)
            {
                std::string key = std::to_string(day);
                int count = 0;
                Json::get(pTimeSectionCount, key, count);
                stInfo.uTimeSectionCount[day] = count;
            }
        }

        /* 处理每天的时间段数组。 */
        Json::Object* pTimeSections = Json::get(pRootJson, "TimeSections");
        if (pTimeSections)
        {
            for (int day = 0; day < 7; day++)
            {
                std::string dayKey = std::to_string(day);
                Json::Object* pDaySections = Json::get(pTimeSections, dayKey);
                if (pDaySections)
                {
                    int count = stInfo.uTimeSectionCount[day];
                    if (count > NET_PLAN_SECTION_NUM) count = NET_PLAN_SECTION_NUM;

                    for (int i = 0; i < count; i++)
                    {
                        std::string idxKey = std::to_string(i);
                        Json::Object* pTimeItem = Json::get(pDaySections, idxKey);
                        if (pTimeItem)
                        {
                            deal(pTimeItem, stInfo.astTimeSection[day][i], bOutStruct);
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* 将结构体转换为 JSON。 */
        Json::Object* pTimeSectionCount = Json::init();
        for (int day = 0; day < 7; day++)
        {
            Json::add(pTimeSectionCount, std::to_string(day).c_str(), stInfo.uTimeSectionCount[day]);
        }
        Json::add(pRootJson, "TimeSectionCount", pTimeSectionCount);

        Json::Object* pTimeSections = Json::init();
        for (int day = 0; day < 7; day++)
        {
            int count = stInfo.uTimeSectionCount[day];
            if (count > NET_PLAN_SECTION_NUM) count = NET_PLAN_SECTION_NUM;

            Json::Object* pDaySections = Json::init();
            for (int i = 0; i < count; i++)
            {
                Json::Object* pTimeItem = Json::init();
                deal(pTimeItem, stInfo.astTimeSection[day][i], bOutStruct);
                Json::add(pDaySections, std::to_string(i).c_str(), pTimeItem);
            }
            Json::add(pTimeSections, std::to_string(day).c_str(), pDaySections);
        }
        Json::add(pRootJson, "TimeSections", pTimeSections);
    }
}
