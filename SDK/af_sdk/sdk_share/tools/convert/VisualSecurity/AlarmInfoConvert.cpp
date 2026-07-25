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
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>

void SDKConvert::deal(Json::Object* pRootJson, NET_Alarmer_S& stAlarmInfo, bool bOutStruct)
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
        memcpy(stAlarmInfo.strSerialNumber, serialNumberStr.c_str(), copy_len);
        if (copy_len < NET_TV_LEN_64)
        {
            stAlarmInfo.strSerialNumber[copy_len] = '\0';
        }
    }
    else
    {
        // 结构体转JSON：BYTE数组转字符串
        std::string serialNumberStr(reinterpret_cast<const char*>(stAlarmInfo.strSerialNumber), NET_TV_LEN_64);
        // 去除末尾的空字符
        size_t len = strnlen(serialNumberStr.c_str(), NET_TV_LEN_64);
        serialNumberStr.resize(len);
        Json::add(pRootJson, "SerialNumber", serialNumberStr);
    }
    
    // 处理设备名称 (CHAR数组)
    convert.field(pRootJson, "DeviceName", stAlarmInfo.strDeviceName);
    
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
    convert.field(pRootJson, "DeviceIP", stAlarmInfo.strDeviceIP);
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

} // namespace

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmBasicInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "AlarmInputNumber", (int&)stInfo.uAlarmInputNumber);

    ByteArrayField(pRootJson, "AlarmOutputNumber", stInfo.byAlarmOutputNumber, bOutStruct);
    ByteArrayField(pRootJson, "AlarmRelateChannel", stInfo.byAlarmRelateChannel, bOutStruct);
    ByteArrayField(pRootJson, "Channel", stInfo.byChannel, bOutStruct);
    ByteArrayField(pRootJson, "DiskNumber", stInfo.byDiskNumber, bOutStruct);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.uPanoramaImgLen);
    ImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
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
    CharArrayField(pRootJson, "RuleName", stInfo.strRuleName, bOutStruct);
    convert.field(pRootJson, "TargetID", (int&)stInfo.uTargetID);
    convert.field(pRootJson, "ObjectType", (int&)stInfo.uObjectType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Left", (int&)stInfo.nLeft);
    convert.field(pRootJson, "Top", (int&)stInfo.nTop);
    convert.field(pRootJson, "Right", (int&)stInfo.nRight);
    convert.field(pRootJson, "Bottom", (int&)stInfo.nBottom);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.uPanoramaImgLen);
    ImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "TargetImgLen", (int&)stInfo.uTargetImgLen);
    ImageBase64Field(pRootJson, "TargetImgBase64", stInfo.byTargetImg, stInfo.uTargetImgLen, bOutStruct);
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
    CharArrayField(pRootJson, "ObjectID", stInfo.strObjectID, bOutStruct);
    convert.field(pRootJson, "PanoramaImgLen", (int&)stInfo.uPanoramaImgLen);
    ImageBase64Field(pRootJson, "PanoramaImgBase64", stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, bOutStruct);
    convert.field(pRootJson, "ImgLen", (int&)stInfo.uImgLen);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);

    ImageBase64Field(pRootJson, "ImgDataBase64", stInfo.byImgData, stInfo.uImgLen, bOutStruct);
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
    CharArrayField(pRootJson, "FaceName", stInfo.strFaceName, bOutStruct);
    CharArrayField(pRootJson, "FaceLibName", stInfo.strFaceLibName, bOutStruct);
    CharArrayField(pRootJson, "LibFacePath", stInfo.strLibFacePath, bOutStruct);
    CharArrayField(pRootJson, "CapFacePath", stInfo.strCapFacePath, bOutStruct);
    CharArrayField(pRootJson, "CapImagePath", stInfo.strCapImagePath, bOutStruct);
    convert.field(pRootJson, "LibFaceImgLen", (int&)stInfo.uLibFaceImgLen);
    convert.field(pRootJson, "CapFaceImgLen", (int&)stInfo.uCapFaceImgLen);
    ImageBase64Field(pRootJson, "LibFaceImgBase64", stInfo.byLibFaceImg, stInfo.uLibFaceImgLen, bOutStruct);
    ImageBase64Field(pRootJson, "CapFaceImgBase64", stInfo.byCapFaceImg, stInfo.uCapFaceImgLen, bOutStruct);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmPlateInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    CharArrayField(pRootJson, "PlateNumber", stInfo.strPlateNumber, bOutStruct);
    convert.field(pRootJson, "PlateColor", (int&)stInfo.uPlateColor);
    convert.field(pRootJson, "VehicleType", (int&)stInfo.uVehicleType);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.field(pRootJson, "Speed", (int&)stInfo.uSpeed);
    convert.field(pRootJson, "LaneNo", (int&)stInfo.uLaneNo);
    convert.field(pRootJson, "PlateImgLen", (int&)stInfo.uPlateImgLen);

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
                stInfo.uPlateImgLen = (UINT32)copyLen;
            }
        }
    }
    else
    {
        UINT32 len = stInfo.uPlateImgLen;
        if (len > (UINT32)NET_TV_VEH_PLATE_IMAGE_LEN) len = (UINT32)NET_TV_VEH_PLATE_IMAGE_LEN;
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

    ImageBase64Field(pRootJson, "ImgDataBase64", stInfo.byImgData, stInfo.uImgLen, bOutStruct);
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
        if (count > NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM)
        {
            count = NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM;
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
                stInfo.uPanoramaImgLen = (UINT32)copyLen;
            }
        }
    }
    else
    {
        UINT32 len = stInfo.uPanoramaImgLen;
        if (len > (UINT32)NET_TV_PIC_DATA_MAX_LEN) len = (UINT32)NET_TV_PIC_DATA_MAX_LEN;
        if (len > 0)
        {
            std::string b64 = SDKConvert::Base64Encode((const unsigned char*)stInfo.byPanoramaImg, (size_t)len);
            Json::add(pRootJson, "PanoramaImgBase64", b64);
        }
    }
}
