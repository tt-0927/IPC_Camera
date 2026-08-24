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
        size_t copy_len = std::min(serialNumberStr.size(), static_cast<size_t>(NET_LEN_64));
        memcpy(stAlarmInfo.strSerialNumber, serialNumberStr.c_str(), copy_len);
        if (copy_len < NET_LEN_64)
        {
            stAlarmInfo.strSerialNumber[copy_len] = '\0';
        }
    }
    else
    {
        // 结构体转JSON：BYTE数组转字符串
        std::string serialNumberStr(reinterpret_cast<const char*>(stAlarmInfo.strSerialNumber), NET_LEN_64);
        // 去除末尾的空字符
        size_t len = strnlen(serialNumberStr.c_str(), NET_LEN_64);
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
        while (std::getline(ss, byteStr, delimiter[0]) && idx < NET_LEN_6)
        {
            unsigned int byteVal = 0;
            std::stringstream hexStream(byteStr);
            hexStream >> std::hex >> byteVal;
            stAlarmInfo.byMacAddr[idx++] = static_cast<BYTE>(byteVal & 0xFF);
        }
        // 填充剩余的字节为0
        while (idx < NET_LEN_6)
        {
            stAlarmInfo.byMacAddr[idx++] = 0;
        }
    }
    else
    {
        // 结构体转JSON：BYTE数组转MAC地址字符串
        std::ostringstream oss;
        for (int i = 0; i < NET_LEN_6; ++i)
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

namespace
{
/*
 * 通用抓拍结构使用“指针 + 长度”保存图片。服务端序列化时只读取调用方在本次
 * NET_serverPushAlarmInfo 调用期间提供的内存；客户端反序列化由 AlarmListener
 * 管理图片所有权，因此此处反序列化阶段只恢复长度和元数据，不保存悬空指针。
 */
void CaptureImageField(Json::Object* pRootJson,
                       const char* strName,
                       NET_ImageBuffer_S& stImage,
                       bool bOutStruct)
{
    if (!pRootJson || !strName)
    {
        return;
    }

    const std::string strLengthKey = std::string(strName) + "Len";
    const std::string strBase64Key = std::string(strName) + "Base64";
    if (bOutStruct)
    {
        int nLength = 0;
        Json::get(pRootJson, strLengthKey, nLength);
        stImage.pData = nullptr;
        stImage.uDataLen = static_cast<UINT32>(std::max(0, std::min(nLength, NET_PIC_DATA_MAX_LEN)));
        return;
    }

    UINT32 uLength = stImage.uDataLen;
    if (stImage.pData == nullptr || uLength == 0)
    {
        uLength = 0;
    }
    else if (uLength > NET_PIC_DATA_MAX_LEN)
    {
        /* 单张图片不能突破公开协议的最大长度，避免错误指针或异常长度放大报文。 */
        uLength = NET_PIC_DATA_MAX_LEN;
    }

    Json::add(pRootJson, strLengthKey, static_cast<int>(uLength));
    if (uLength > 0)
    {
        Json::add(pRootJson,
                  strBase64Key,
                  SDKConvert::Base64Encode(stImage.pData, static_cast<size_t>(uLength)));
    }
}

void CaptureCropField(Json::Object* pRootJson, NET_CropImage_S& stCrop, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    int nCropX = static_cast<int>(stCrop.uCropX);
    int nCropY = static_cast<int>(stCrop.uCropY);
    int nCropWidth = static_cast<int>(stCrop.uCropWidth);
    int nCropHeight = static_cast<int>(stCrop.uCropHeight);
    int nTargetType = static_cast<int>(stCrop.uTargetType);
    convert.field(pRootJson, "CropX", nCropX);
    convert.field(pRootJson, "CropY", nCropY);
    convert.field(pRootJson, "CropWidth", nCropWidth);
    convert.field(pRootJson, "CropHeight", nCropHeight);
    convert.field(pRootJson, "TargetType", nTargetType);
    if (bOutStruct)
    {
        stCrop.uCropX = static_cast<UINT32>(std::max(0, nCropX));
        stCrop.uCropY = static_cast<UINT32>(std::max(0, nCropY));
        stCrop.uCropWidth = static_cast<UINT32>(std::max(0, nCropWidth));
        stCrop.uCropHeight = static_cast<UINT32>(std::max(0, nCropHeight));
        stCrop.uTargetType = static_cast<UINT32>(std::max(0, nTargetType));
    }
    convert.field(pRootJson, "Confidence", stCrop.fConfidence);
    convert.field(pRootJson, "TrackID", stCrop.nTrackID);
    CaptureImageField(pRootJson, "Image", stCrop.stImage, bOutStruct);
}

void CapturePolygonField(Json::Object* pRootJson, NET_CapturePolygon_S& stPolygon, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    int nPointCount = static_cast<int>(stPolygon.uPointCount);
    convert.field(pRootJson, "PointCount", nPointCount);
    if (bOutStruct)
    {
        stPolygon.uPointCount = static_cast<UINT32>(std::max(0, std::min(nPointCount, NET_CAPTURE_REGION_POINT_MAX_NUM)));
    }

    Json::Object* pPointX = Json::get(pRootJson, "PointX");
    Json::Object* pPointY = Json::get(pRootJson, "PointY");
    if (bOutStruct)
    {
        const int nPointXSize = pPointX ? Json::Array::size(pPointX) : 0;
        const int nPointYSize = pPointY ? Json::Array::size(pPointY) : 0;
        const UINT32 uPointCount = std::min(stPolygon.uPointCount,
                                            static_cast<UINT32>(std::max(0, std::min(nPointXSize, nPointYSize))));
        for (UINT32 i = 0; i < uPointCount; ++i)
        {
            double fPointX = 0.0;
            double fPointY = 0.0;
            Json::Value::get(Json::Array::get(pPointX, static_cast<int>(i)), fPointX);
            Json::Value::get(Json::Array::get(pPointY, static_cast<int>(i)), fPointY);
            stPolygon.afPointX[i] = static_cast<FLOAT>(fPointX);
            stPolygon.afPointY[i] = static_cast<FLOAT>(fPointY);
        }
        stPolygon.uPointCount = uPointCount;
        return;
    }

    Json::Object* pOutPointX = Json::Array::init();
    Json::Object* pOutPointY = Json::Array::init();
    const UINT32 uPointCount = std::min(stPolygon.uPointCount,
                                        static_cast<UINT32>(NET_CAPTURE_REGION_POINT_MAX_NUM));
    for (UINT32 i = 0; i < uPointCount; ++i)
    {
        Json::Array::add(pOutPointX, static_cast<float>(stPolygon.afPointX[i]));
        Json::Array::add(pOutPointY, static_cast<float>(stPolygon.afPointY[i]));
    }
    Json::add(pRootJson, "PointX", pOutPointX);
    Json::add(pRootJson, "PointY", pOutPointY);
}

void CaptureExtraField(Json::Object* pRootJson, NET_CaptureExtraInfo_S& stExtra, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Male", stExtra.bMale);
    convert.field(pRootJson, "AgeLabel", stExtra.nAgeLabel);
    convert.field(pRootJson, "Glasses", stExtra.bGlasses);
    convert.field(pRootJson, "Beard", stExtra.bBeard);
    convert.field(pRootJson, "Mask", stExtra.bMask);
    convert.field(pRootJson, "EmotionLabel", stExtra.nEmotionLabel);
    convert.field(pRootJson, "Bag", stExtra.bBag);
    convert.field(pRootJson, "TopColorLabel", stExtra.nTopColorLabel);
    convert.field(pRootJson, "BottomColorLabel", stExtra.nBottomColorLabel);
    convert.field(pRootJson, "VehicleType", stExtra.nVehicleType);
    convert.field(pRootJson, "VehicleColor", stExtra.nVehicleColor);
    CharArrayField(pRootJson, "VehicleBrand", stExtra.strVehicleBrand, bOutStruct);
    CharArrayField(pRootJson, "LicensePlateNumber", stExtra.strLicensePlateNumber, bOutStruct);
    CharArrayField(pRootJson, "CaptureTimestamp", stExtra.strTimestamp, bOutStruct);

    Json::Object* pRegion = nullptr;
    if (bOutStruct)
    {
        pRegion = Json::get(pRootJson, "TargetRegion");
        if (pRegion)
        {
            CapturePolygonField(pRegion, stExtra.stTargetRegion, true);
        }
        return;
    }

    pRegion = Json::init();
    CapturePolygonField(pRegion, stExtra.stTargetRegion, false);
    Json::add(pRootJson, "TargetRegion", pRegion);
}
} // namespace

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmCaptureInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AlarmType", (int&)stInfo.uAlarmType);
    convert.field(pRootJson, "Channel", (int&)stInfo.uChannel);
    convert.field(pRootJson, "CaptureType", (int&)stInfo.uCaptureType);
    convert.field(pRootJson, "TimestampMs", stInfo.llTimestampMs);
    convert.field(pRootJson, "PanoramaWidth", (int&)stInfo.uPanoramaWidth);
    convert.field(pRootJson, "PanoramaHeight", (int&)stInfo.uPanoramaHeight);
    CaptureImageField(pRootJson, "PanoramaImg", stInfo.stPanoramaImg, bOutStruct);
    convert.field(pRootJson, "CropCount", (int&)stInfo.uCropCount);
    CaptureExtraField(pRootJson, stInfo.stExtraInfo, bOutStruct);

    if (bOutStruct)
    {
        Json::Object* pCrops = Json::get(pRootJson, "Crops");
        const int nArraySize = pCrops ? Json::Array::size(pCrops) : 0;
        UINT32 uCropCount = std::min(stInfo.uCropCount, static_cast<UINT32>(std::max(0, nArraySize)));
        uCropCount = std::min(uCropCount, static_cast<UINT32>(NET_CAPTURE_CROP_MAX_NUM));
        for (UINT32 i = 0; i < uCropCount; ++i)
        {
            Json::Object* pCrop = Json::Array::get(pCrops, static_cast<int>(i));
            CaptureCropField(pCrop, stInfo.stCropImages[i], true);
        }
        stInfo.uCropCount = uCropCount;
        return;
    }

    Json::Object* pCrops = Json::Array::init();
    const UINT32 uCropCount = std::min(stInfo.uCropCount, static_cast<UINT32>(NET_CAPTURE_CROP_MAX_NUM));
    for (UINT32 i = 0; i < uCropCount; ++i)
    {
        Json::Object* pCrop = Json::init();
        CaptureCropField(pCrop, stInfo.stCropImages[i], false);
        Json::Array::add(pCrops, pCrop);
    }
    Json::add(pRootJson, "Crops", pCrops);
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

/* ==================== 从 DeviceInfoConvert 搬运: 录像/报警/AI/NVR ==================== */

static void JsonToFloatArray(Json::Object* pRootJson, const char* key, FLOAT* values, int maxCount)
{
    Json::Object* pArray = Json::get(pRootJson, key);
    if (!pArray)
    {
        return;
    }

    int nSize = Json::Array::size(pArray);
    for (int i = 0; i < nSize && i < maxCount; i++)
    {
        Json::Object* pItem = Json::Array::get(pArray, i);
        if (pItem)
        {
            double dVal = 0.0;
            Json::Value::get(pItem, dVal);
            values[i] = (FLOAT)dVal;
        }
    }
}

static void FloatArrayToJson(Json::Object* pRootJson, const char* key, const FLOAT* values, int count, int maxCount)
{
    Json::Object* pArray = Json::Array::init();
    if (count > maxCount)
    {
        count = maxCount;
    }
    for (int i = 0; i < count; i++)
    {
        Json::Array::add(pArray, static_cast<float>(values[i]));
    }
    Json::add(pRootJson, key, pArray);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
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

    // 处理每天的时间段数量
    if (bOutStruct)
    {
        // JSON -> 结构体
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

        // 处理每天的时间段数组
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
        // 结构体 -> JSON
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
