// 禁用 Windows 的 min/max 宏
#define NOMINMAX
// 然后才是您的兼容性代码
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
#include <limits>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARMER_S& stAlarmInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    
    SDKConvert::CSDKConvert convert(bOutStruct);
    
    // 处理序列号 (BYTE数组转字符串)
    if (bOutStruct)
    {
        // JSON转结构体：字符串转BYTE数组
        std::string serialNumberStr;
        convert.field(pRootJson, "SerialNumber", serialNumberStr);
        size_t copy_len = std::min(serialNumberStr.size(), static_cast<size_t>(NET_TV_LEN_64));
        memcpy(stAlarmInfo.szSerialNumber, serialNumberStr.c_str(), copy_len);
        if (copy_len < NET_TV_LEN_64)
        {
            stAlarmInfo.szSerialNumber[copy_len] = '\0';
        }
    }
    else
    {
        // 结构体转JSON：BYTE数组转字符串
        std::string serialNumberStr(reinterpret_cast<const char*>(stAlarmInfo.szSerialNumber), NET_TV_LEN_64);
        // 去除末尾的空字符
        size_t len = strnlen(serialNumberStr.c_str(), NET_TV_LEN_64);
        serialNumberStr.resize(len);
        Json::add(pRootJson, "SerialNumber", serialNumberStr);
    }
    
    // 处理设备名称 (CHAR数组)
    convert.field(pRootJson, "DeviceName", stAlarmInfo.szDeviceName);
    
    // 处理MAC地址 (BYTE数组转字符串，格式: XX:XX:XX:XX:XX:XX)
    if (bOutStruct)
    {
        // JSON转结构体：字符串转BYTE数组
        std::string macAddrStr;
        convert.field(pRootJson, "MacAddress", macAddrStr);
        // 解析MAC地址字符串 "XX:XX:XX:XX:XX:XX" 或 "XX-XX-XX-XX-XX-XX"
        std::string delimiter = ":";
        if (macAddrStr.find('-') != std::string::npos)
        {
            delimiter = "-";
        }
        
        std::stringstream ss(macAddrStr);
        std::string byteStr;
        int idx = 0;
        while (std::getline(ss, byteStr, delimiter[0]) && idx < NET_TV_LEN_6)
        {
            unsigned int byteVal = 0;
            std::stringstream hexStream(byteStr);
            hexStream >> std::hex >> byteVal;
            stAlarmInfo.byMacAddr[idx++] = static_cast<BYTE>(byteVal & 0xFF);
        }
        // 填充剩余的字节为0
        while (idx < NET_TV_LEN_6)
        {
            stAlarmInfo.byMacAddr[idx++] = 0;
        }
    }
    else
    {
        // 结构体转JSON：BYTE数组转MAC地址字符串
        std::ostringstream oss;
        for (int i = 0; i < NET_TV_LEN_6; ++i)
        {
            if (i > 0) oss << ":";
            oss << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<unsigned int>(stAlarmInfo.byMacAddr[i]);
        }
        Json::add(pRootJson, "MacAddress", oss.str());
    }
    
    // 处理IP地址 (CHAR数组)
    convert.field(pRootJson, "DeviceIP", stAlarmInfo.szDeviceIP);
}

namespace
{
template <size_t N>
void ByteArrayField(Json::Object* pRootJson, const std::string& key, BYTE (&arr)[N], bool bOutStruct)
{
    if (!pRootJson) return;

    if (bOutStruct)
    {
        std::vector<int> vec;
        Json::Object* pArr = Json::get(pRootJson, key);
        if (pArr)
        {
            Json::Array::get(pArr, vec);
        }
        for (size_t i = 0; i < N; ++i)
        {
            int v = (i < vec.size()) ? vec[i] : 0;
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            arr[i] = static_cast<BYTE>(v);
        }
    }
    else
    {
        Json::Object* pArr = Json::Array::init();
        for (size_t i = 0; i < N; ++i)
        {
            Json::Array::add(pArr, static_cast<int>(arr[i]));
        }
        Json::add(pRootJson, key, pArr);
    }
}

template <size_t N>
void CharArrayField(Json::Object* pRootJson, const std::string& key, char (&arr)[N], bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, key, arr);
}

/*
 * 将抓拍区域顶点数组在 JSON 与 SDK 结构体之间转换。
 * JSON 侧使用 PointX 和 PointY 两个浮点数组，避免顶点结构嵌套带来的兼容性问题。
 */
static void CapturePolygonPointsField(Json::Object* pRootJson,
                                      NET_CapturePolygon_S& stInfo,
                                      bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        int nPointCount = 0;
        Json::get(pRootJson, "PointCount", nPointCount);
        if (nPointCount < 0)
        {
            nPointCount = 0;
        }

        Json::Object* pPointX = Json::get(pRootJson, "PointX");
        Json::Object* pPointY = Json::get(pRootJson, "PointY");
        int nPointXCount = Json::Array::size(pPointX);
        int nPointYCount = Json::Array::size(pPointY);
        int nAvailablePointCount = (std::min)(nPointXCount, nPointYCount);
        int nSafePointCount = (std::min)(nPointCount, nAvailablePointCount);
        nSafePointCount = (std::min)(nSafePointCount, NET_CAPTURE_REGION_POINT_MAX_NUM);
        stInfo.uPointCount = static_cast<UINT32>(nSafePointCount);

        for (int nIndex = 0; nIndex < nSafePointCount; ++nIndex)
        {
            double dPointX = 0.0;
            double dPointY = 0.0;
            Json::Value::get(Json::Array::get(pPointX, nIndex), dPointX);
            Json::Value::get(Json::Array::get(pPointY, nIndex), dPointY);
            stInfo.afPointX[nIndex] = static_cast<FLOAT>(dPointX);
            stInfo.afPointY[nIndex] = static_cast<FLOAT>(dPointY);
        }
        return;
    }

    UINT32 uSafePointCount = stInfo.uPointCount;
    if (uSafePointCount > NET_CAPTURE_REGION_POINT_MAX_NUM)
    {
        uSafePointCount = NET_CAPTURE_REGION_POINT_MAX_NUM;
    }
    Json::add(pRootJson, "PointCount", static_cast<int>(uSafePointCount));
    Json::add(pRootJson, "PointX", Json::Array::init(stInfo.afPointX, static_cast<int>(uSafePointCount)));
    Json::add(pRootJson, "PointY", Json::Array::init(stInfo.afPointY, static_cast<int>(uSafePointCount)));
}

template <size_t N>
void ImageBase64Field(Json::Object* pRootJson,
                      const std::string& key,
                      BYTE (&arr)[N],
                      UINT32& len,
                      bool bOutStruct)
{
    if (!pRootJson) return;

    if (bOutStruct)
    {
        std::string b64;
        if (Json::get(pRootJson, key, b64) && !b64.empty())
        {
            std::vector<unsigned char> decoded;
            if (SDKConvert::Base64Decode(b64, decoded))
            {
                size_t copyLen = std::min(decoded.size(), static_cast<size_t>(N));
                if (copyLen > 0)
                {
                    std::memcpy(arr, decoded.data(), copyLen);
                }
                len = static_cast<UINT32>(copyLen);
            }
        }
        return;
    }

    UINT32 imageLen = len;
    if (imageLen > static_cast<UINT32>(N))
    {
        imageLen = static_cast<UINT32>(N);
    }
    if (imageLen > 0)
    {
        std::string b64 = SDKConvert::Base64Encode(reinterpret_cast<const unsigned char*>(arr),
                                                   static_cast<size_t>(imageLen));
        Json::add(pRootJson, key, b64);
    }
}

/*
 * V2 图片结构只保存视图，不承担内存所有权。服务端序列化时直接读取 pData；
 * 客户端反序列化时由 ClientAlarmManager 持有 vector，并在回调前回填 pData。
 */
void ImageViewBase64Field(Json::Object* pRootJson,
                          const std::string& key,
                          NET_TV_IMAGE_DATA_S& image,
                          bool bOutStruct)
{
    if (!pRootJson || bOutStruct)
    {
        if (bOutStruct)
        {
            image.pData = nullptr;
        }
        return;
    }

    if (image.pData && image.dwLen > 0)
    {
        const std::string base64 = SDKConvert::Base64Encode(
            reinterpret_cast<const unsigned char*>(image.pData), image.dwLen);
        Json::add(pRootJson, key, base64);
    }
}

void ImageViewLengthField(Json::Object* pRootJson,
                          const std::string& key,
                          NET_TV_IMAGE_DATA_S& image,
                          bool bOutStruct)
{
    INT64 imageLen = static_cast<INT64>(image.dwLen);
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, key, imageLen);
    if (bOutStruct)
    {
        image.dwLen = imageLen > 0 &&
                              imageLen <= static_cast<INT64>(std::numeric_limits<UINT32>::max())
                          ? static_cast<UINT32>(imageLen)
                          : 0;
        image.pData = nullptr;
    }
}

/* 在 JSON Base64 字段与抓拍图片缓存之间转换，超出结构体容量时拒绝复制。 */
template <size_t N>
static void CaptureImageBase64Field(Json::Object* pRootJson,
                                    const std::string& strKey,
                                    BYTE (&abyImage)[N],
                                    UINT32& uImageLen,
                                    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        uImageLen = 0;
        std::string strBase64;
        if (!Json::get(pRootJson, strKey, strBase64) || strBase64.empty())
        {
            return;
        }

        std::vector<unsigned char> vecImage;
        if (!SDKConvert::Base64Decode(strBase64, vecImage) || vecImage.size() > N)
        {
            return;
        }

        if (!vecImage.empty())
        {
            std::memcpy(abyImage, vecImage.data(), vecImage.size());
        }
        uImageLen = static_cast<UINT32>(vecImage.size());
        return;
    }

    if (uImageLen > N)
    {
        uImageLen = 0;
        return;
    }

    if (uImageLen > 0)
    {
        const std::string strBase64 = SDKConvert::Base64Encode(
            reinterpret_cast<const unsigned char*>(abyImage),
            static_cast<size_t>(uImageLen));
        Json::add(pRootJson, strKey, strBase64);
    }
}

} // namespace

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_BASIC_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "AlarmInputNumber", (int&)stInfo.dwAlarmInputNumber);

    ByteArrayField(pRootJson, "AlarmOutputNumber", stInfo.byAlarmOutputNumber, bOutStruct);
    ByteArrayField(pRootJson, "AlarmRelateChannel", stInfo.byAlarmRelateChannel, bOutStruct);
    ByteArrayField(pRootJson, "Channel", stInfo.byChannel, bOutStruct);
    ByteArrayField(pRootJson, "DiskNumber", stInfo.byDiskNumber, bOutStruct);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.dwPanoramaImgLen);
    ImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.dwPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_RULE_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "RuleID", (int&)stInfo.dwRuleID);
    convert.field(pRootJson, "RuleType", (int&)stInfo.dwRuleType);
    CharArrayField(pRootJson, "RuleName", stInfo.szRuleName, bOutStruct);
    convert.field(pRootJson, "TargetID", (int&)stInfo.dwTargetID);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.dwObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.dwPanoramaImgLen);
    ImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.dwPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "TargetImgLen", (int&)stInfo.dwTargetImgLen);
    ImageBase64Field(pRootJson, "TargetImgBase64", stInfo.byTargetImg, stInfo.dwTargetImgLen, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_AI_OBJECT_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.dwObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    CharArrayField(pRootJson, "ObjectID", stInfo.szObjectID, bOutStruct);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.dwPanoramaImgLen);
    ImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.dwPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "ImgLen", (int&)stInfo.dwImgLen);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);

    ImageBase64Field(pRootJson, "ImgDataBase64", stInfo.byImgData, stInfo.dwImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_FACE_COMPARE_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "EventId", stInfo.nEventId);
    convert.field(pRootJson, "CompareResult", stInfo.nCompResult);
    convert.field(pRootJson, "Similarity", stInfo.nSimilarity);
    convert.field(pRootJson, "FaceID", stInfo.nFaceId);
    CharArrayField(pRootJson, "FaceName", stInfo.szFaceName, bOutStruct);
    CharArrayField(pRootJson, "FaceLibName", stInfo.szFaceLibName, bOutStruct);
    CharArrayField(pRootJson, "LibFacePath", stInfo.szLibFacePath, bOutStruct);
    CharArrayField(pRootJson, "CapFacePath", stInfo.szCapFacePath, bOutStruct);
    CharArrayField(pRootJson, "CapImagePath", stInfo.szCapImagePath, bOutStruct);
    convert.field(pRootJson, "LibFaceImgLen", (int&)stInfo.dwLibFaceImgLen);
    convert.field(pRootJson, "CapFaceImgLen", (int&)stInfo.dwCapFaceImgLen);
    ImageBase64Field(pRootJson, "LibFaceImgBase64", stInfo.byLibFaceImg, stInfo.dwLibFaceImgLen, bOutStruct);
    ImageBase64Field(pRootJson, "CapFaceImgBase64", stInfo.byCapFaceImg, stInfo.dwCapFaceImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_CapturePolygon_S& stInfo, bool bOutStruct)
{
    CapturePolygonPointsField(pRootJson, stInfo, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceCapturePushInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "IsMale", stInfo.bMale);
    stConvert.field(pRootJson, "AgeLabel", stInfo.nAgeLabel);
    stConvert.field(pRootJson, "IsGlasses", stInfo.bGlasses);
    stConvert.field(pRootJson, "IsBeard", stInfo.bBeard);
    stConvert.field(pRootJson, "IsMask", stInfo.bMask);
    stConvert.field(pRootJson, "EmotionLabel", stInfo.nEmotionLabel);
    stConvert.structure(pRootJson, "FaceRegion", stInfo.stFaceRegion);
    if (!bOutStruct)
    {
        int nFaceImgLen = static_cast<int>(stInfo.uFaceImgLen);
        int nPanoramaImgLen = static_cast<int>(stInfo.uPanoramaImgLen);
        stConvert.field(pRootJson, "FaceImgLen", nFaceImgLen);
        stConvert.field(pRootJson, "PanoramaImgLen", nPanoramaImgLen);
    }
    CharArrayField(pRootJson, "Timestamp", stInfo.strTimestamp, bOutStruct);
    CaptureImageBase64Field(pRootJson, "FaceImgBase64", stInfo.byFaceImg, stInfo.uFaceImgLen, bOutStruct);
    CaptureImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_PersonCapturePushInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "IsMale", stInfo.bMale);
    stConvert.field(pRootJson, "AgeLabel", stInfo.nAgeLabel);
    stConvert.field(pRootJson, "HasBag", stInfo.bBag);
    stConvert.field(pRootJson, "TopColorLabel", stInfo.nTopColorLabel);
    stConvert.field(pRootJson, "BottomColorLabel", stInfo.nBottomColorLabel);
    if (!bOutStruct)
    {
        int nPersonImgLen = static_cast<int>(stInfo.uPersonImgLen);
        int nPanoramaImgLen = static_cast<int>(stInfo.uPanoramaImgLen);
        stConvert.field(pRootJson, "PersonImgLen", nPersonImgLen);
        stConvert.field(pRootJson, "PanoramaImgLen", nPanoramaImgLen);
    }
    CharArrayField(pRootJson, "Timestamp", stInfo.strTimestamp, bOutStruct);
    CaptureImageBase64Field(pRootJson, "PersonImgBase64", stInfo.byPersonImg, stInfo.uPersonImgLen, bOutStruct);
    CaptureImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_MotorvehicleCapturePushInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    CharArrayField(pRootJson, "VehicleBrand", stInfo.strVehicleBrand, bOutStruct);
    stConvert.field(pRootJson, "VehicleType", stInfo.nVehicleType);
    stConvert.field(pRootJson, "VehicleColor", stInfo.nVehicleColor);
    CharArrayField(pRootJson, "LicensePlateNumber", stInfo.strLicensePlateNumber, bOutStruct);
    if (!bOutStruct)
    {
        int nTargetImgLen = static_cast<int>(stInfo.uTargetImgLen);
        int nPanoramaImgLen = static_cast<int>(stInfo.uPanoramaImgLen);
        stConvert.field(pRootJson, "TargetImgLen", nTargetImgLen);
        stConvert.field(pRootJson, "PanoramaImgLen", nPanoramaImgLen);
    }
    CharArrayField(pRootJson, "Timestamp", stInfo.strTimestamp, bOutStruct);
    CaptureImageBase64Field(pRootJson, "TargetImgBase64", stInfo.byTargetImg, stInfo.uTargetImgLen, bOutStruct);
    CaptureImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_NonMotorvehicleCapturePushInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert stConvert(bOutStruct);
    stConvert.field(pRootJson, "VehicleType", stInfo.nVehicleType);
    stConvert.field(pRootJson, "VehicleColor", stInfo.nVehicleColor);
    if (!bOutStruct)
    {
        int nTargetImgLen = static_cast<int>(stInfo.uTargetImgLen);
        int nPanoramaImgLen = static_cast<int>(stInfo.uPanoramaImgLen);
        stConvert.field(pRootJson, "TargetImgLen", nTargetImgLen);
        stConvert.field(pRootJson, "PanoramaImgLen", nPanoramaImgLen);
    }
    CharArrayField(pRootJson, "Timestamp", stInfo.strTimestamp, bOutStruct);
    CaptureImageBase64Field(pRootJson, "TargetImgBase64", stInfo.byTargetImg, stInfo.uTargetImgLen, bOutStruct);
    CaptureImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_PLATE_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    CharArrayField(pRootJson, "PlateNumber", stInfo.szPlateNumber, bOutStruct);
    convert.field(pRootJson, "PlateColor", (int&)stInfo.dwPlateColor);
    convert.field(pRootJson, "VehicleType", (int&)stInfo.dwVehicleType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Speed", (int&)stInfo.dwSpeed);
    convert.field(pRootJson, "LaneNo", (int&)stInfo.dwLaneNo);
    convert.field(pRootJson, "PlateImgLen", (int&)stInfo.dwPlateImgLen);

    // 严格仿海康：车牌图片随结构体（JSON 里 Base64）
    if (bOutStruct)
    {
        std::string b64;
        if (Json::get(pRootJson, "PlateImgBase64", b64) && !b64.empty())
        {
            std::vector<unsigned char> decoded;
            if (SDKConvert::Base64Decode(b64, decoded))
            {
                size_t copyLen = std::min(decoded.size(), (size_t)NET_TV_VEH_PLATE_IMAGE_LEN);
                if (copyLen > 0)
                {
                    std::memcpy(stInfo.byPlateImg, decoded.data(), copyLen);
                }
                stInfo.dwPlateImgLen = (UINT32)copyLen;
            }
        }
    }
    else
    {
        UINT32 len = stInfo.dwPlateImgLen;
        if (len > (UINT32)NET_TV_VEH_PLATE_IMAGE_LEN) len = (UINT32)NET_TV_VEH_PLATE_IMAGE_LEN;
        if (len > 0)
        {
            std::string b64 = SDKConvert::Base64Encode((const unsigned char*)stInfo.byPlateImg, (size_t)len);
            Json::add(pRootJson, "PlateImgBase64", b64);
        }
    }
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_EXCEPTION_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "DiskNo", (int&)stInfo.dwDiskNo);
    convert.field(pRootJson, "Status", (int&)stInfo.dwStatus);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_TARGET_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "TrackID", stInfo.nTrackID);
    convert.field(pRootJson, "RuleID", (int&)stInfo.dwRuleID);
    convert.field(pRootJson, "SnapshotType", (int&)stInfo.dwSnapshotType);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "Direction", stInfo.nDirection);

    ImageBase64Field(pRootJson, "ImgDataBase64", stInfo.byImgData, stInfo.dwImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_INFO_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "StatisticsType", (int&)stInfo.dwStatisticsType);
    convert.field(pRootJson, "RuleID", (int&)stInfo.dwRuleID);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "ReportSeq", (int&)stInfo.dwReportSeq);
    convert.field(pRootJson, "EnterCount", (int&)stInfo.dwEnterCount);
    convert.field(pRootJson, "LeaveCount", (int&)stInfo.dwLeaveCount);
    convert.field(pRootJson, "TotalCount", (int&)stInfo.dwTotalCount);
    convert.field(pRootJson, "CurrentPeopleCount", (int&)stInfo.dwCurrentPeopleCount);
    convert.field(pRootJson, "AverageStayTimeSec", (int&)stInfo.dwAverageStayTimeSec);
    convert.field(pRootJson, "TargetCount", (int&)stInfo.dwTargetCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Targets");
        int nSize = Json::Array::size(pArray);
        UINT32 count = stInfo.dwTargetCount;
        if (count == 0 || count > (UINT32)nSize)
        {
            count = (UINT32)nSize;
        }
        if (count > NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM)
        {
            count = NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM;
        }

        for (UINT32 i = 0; i < count; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, (int)i);
            SDKConvert::deal(pItem, stInfo.stTargets[i], true);
        }
        stInfo.dwTargetCount = count;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        UINT32 count = stInfo.dwTargetCount;
        if (count > NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM)
        {
            count = NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM;
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
                size_t copyLen = std::min(decoded.size(), (size_t)NET_TV_PIC_DATA_MAX_LEN);
                if (copyLen > 0)
                {
                    std::memcpy(stInfo.byPanoramaImg, decoded.data(), copyLen);
                }
                stInfo.dwPanoramaImgLen = (UINT32)copyLen;
            }
        }
    }
    else
    {
        UINT32 len = stInfo.dwPanoramaImgLen;
        if (len > (UINT32)NET_TV_PIC_DATA_MAX_LEN) len = (UINT32)NET_TV_PIC_DATA_MAX_LEN;
        if (len > 0)
        {
            std::string b64 = SDKConvert::Base64Encode((const unsigned char*)stInfo.byPanoramaImg, (size_t)len);
            Json::add(pRootJson, "PanoramaImgBase64", b64);
        }
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_BASIC_INFO_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "AlarmInputNumber", (int&)stInfo.dwAlarmInputNumber);
    ByteArrayField(pRootJson, "AlarmOutputNumber", stInfo.byAlarmOutputNumber, bOutStruct);
    ByteArrayField(pRootJson, "AlarmRelateChannel", stInfo.byAlarmRelateChannel, bOutStruct);
    ByteArrayField(pRootJson, "Channel", stInfo.byChannel, bOutStruct);
    ByteArrayField(pRootJson, "DiskNumber", stInfo.byDiskNumber, bOutStruct);
    ImageViewLengthField(pRootJson, "PanoramaImgLen", stInfo.stPanoramaImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "PanoramaImgBase64", stInfo.stPanoramaImg, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_RULE_INFO_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "RuleID", (int&)stInfo.dwRuleID);
    convert.field(pRootJson, "RuleType", (int&)stInfo.dwRuleType);
    CharArrayField(pRootJson, "RuleName", stInfo.szRuleName, bOutStruct);
    convert.field(pRootJson, "TargetID", (int&)stInfo.dwTargetID);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.dwObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    ImageViewLengthField(pRootJson, "PanoramaImgLen", stInfo.stPanoramaImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "PanoramaImgBase64", stInfo.stPanoramaImg, bOutStruct);
    ImageViewLengthField(pRootJson, "TargetImgLen", stInfo.stTargetImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "TargetImgBase64", stInfo.stTargetImg, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_AI_OBJECT_INFO_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.dwObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    CharArrayField(pRootJson, "ObjectID", stInfo.szObjectID, bOutStruct);
    ImageViewLengthField(pRootJson, "PanoramaImgLen", stInfo.stPanoramaImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "PanoramaImgBase64", stInfo.stPanoramaImg, bOutStruct);
    ImageViewLengthField(pRootJson, "ImgLen", stInfo.stImgData, bOutStruct);
    ImageViewBase64Field(pRootJson, "ImgDataBase64", stInfo.stImgData, bOutStruct);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_FACE_COMPARE_INFO_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "EventId", stInfo.nEventId);
    convert.field(pRootJson, "CompareResult", stInfo.nCompResult);
    convert.field(pRootJson, "Similarity", (int&)stInfo.nSimilarity);
    convert.field(pRootJson, "FaceID", (int&)stInfo.nFaceId);
    CharArrayField(pRootJson, "FaceLibName", stInfo.szFaceLibName, bOutStruct);
    CharArrayField(pRootJson, "FaceName", stInfo.szFaceName, bOutStruct);
    CharArrayField(pRootJson, "LibFacePath", stInfo.szLibFacePath, bOutStruct);
    CharArrayField(pRootJson, "CapFacePath", stInfo.szCapFacePath, bOutStruct);
    CharArrayField(pRootJson, "CapImagePath", stInfo.szCapImagePath, bOutStruct);
    ImageViewLengthField(pRootJson, "LibFaceImgLen", stInfo.stLibFaceImg, bOutStruct);
    ImageViewLengthField(pRootJson, "CapFaceImgLen", stInfo.stCapFaceImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "LibFaceImgBase64", stInfo.stLibFaceImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "CapFaceImgBase64", stInfo.stCapFaceImg, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_PLATE_INFO_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    CharArrayField(pRootJson, "PlateNumber", stInfo.szPlateNumber, bOutStruct);
    convert.field(pRootJson, "PlateColor", (int&)stInfo.dwPlateColor);
    convert.field(pRootJson, "VehicleType", (int&)stInfo.dwVehicleType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Speed", (int&)stInfo.dwSpeed);
    convert.field(pRootJson, "LaneNo", (int&)stInfo.dwLaneNo);
    ImageViewLengthField(pRootJson, "PlateImgLen", stInfo.stPlateImg, bOutStruct);
    ImageViewBase64Field(pRootJson, "PlateImgBase64", stInfo.stPlateImg, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_TARGET_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "TrackID", stInfo.nTrackID);
    convert.field(pRootJson, "RuleID", (int&)stInfo.dwRuleID);
    convert.field(pRootJson, "SnapshotType", (int&)stInfo.dwSnapshotType);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "Direction", stInfo.nDirection);
    ImageViewBase64Field(pRootJson, "ImgDataBase64", stInfo.stImgData, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_INFO_V2_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.dwAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.dwChannel);
    convert.field(pRootJson, "StatisticsType", (int&)stInfo.dwStatisticsType);
    convert.field(pRootJson, "RuleID", (int&)stInfo.dwRuleID);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "ReportSeq", (int&)stInfo.dwReportSeq);
    convert.field(pRootJson, "EnterCount", (int&)stInfo.dwEnterCount);
    convert.field(pRootJson, "LeaveCount", (int&)stInfo.dwLeaveCount);
    convert.field(pRootJson, "TotalCount", (int&)stInfo.dwTotalCount);
    convert.field(pRootJson, "CurrentPeopleCount", (int&)stInfo.dwCurrentPeopleCount);
    convert.field(pRootJson, "AverageStayTimeSec", (int&)stInfo.dwAverageStayTimeSec);
    convert.field(pRootJson, "TargetCount", (int&)stInfo.dwTargetCount);

    if (!bOutStruct)
    {
        const UINT32 targetCount = std::min<UINT32>(stInfo.dwTargetCount,
                                                     NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM);
        Json::Object* targets = Json::Array::init();
        for (UINT32 i = 0; i < targetCount; ++i)
        {
            Json::Object* target = Json::init();
            SDKConvert::deal(target, stInfo.stTargets[i], false);
            Json::Array::add(targets, target);
        }
        Json::add(pRootJson, "Targets", targets);
    }
    else
    {
        Json::Object* sourceTargets = Json::get(pRootJson, "Targets");
        if (sourceTargets)
        {
            const int nArraySize = Json::Array::size(sourceTargets);
            const UINT32 sourceCount = nArraySize > 0
                                           ? std::min<UINT32>(static_cast<UINT32>(nArraySize),
                                                              NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM)
                                           : 0;
            stInfo.dwTargetCount = sourceCount;
            for (UINT32 i = 0; i < sourceCount; ++i)
            {
                if (Json::Object* target = Json::Array::get(sourceTargets, static_cast<int>(i)))
                {
                    SDKConvert::deal(target, stInfo.stTargets[i], true);
                }
            }
        }
    }

    ImageViewBase64Field(pRootJson, "PanoramaImgBase64", stInfo.stPanoramaImg, bOutStruct);
}
