/**
 * @FilePath     : video_convert.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-12 14:39:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-19 19:40:21
 * @Description  : 视频配置转换处理
 */

#include "video_convert.h"
#include "common_convert.h"
#include "convert.h" /* 这个要放在video_convert.h的后面 */

void Convert::deal(Json::Object *pRootJson, Video_NS::VideoConfig_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Id", stInfo.nId);
	convert.field(pRootJson, "VideoType", (int &)stInfo.enVideoType);
	convert.field(pRootJson, "Width", stInfo.stVideoResolution.nWidth);
	convert.field(pRootJson, "Height", stInfo.stVideoResolution.nHeight);
	std::string strBitrateType;
	if(bOutStruct)
	{
		convert.field(pRootJson, "BitrateType", strBitrateType);
		if(strBitrateType == "CBR")
		{
			stInfo.enBitrateType = Video_NS::BitrateType_E::CBR;
		}else if(strBitrateType == "VBR")
		{
			stInfo.enBitrateType = Video_NS::BitrateType_E::VBR;
		}else
		{
			stInfo.enBitrateType  = Video_NS::BitrateType_E::CBR;
		}
	}else{
		if(stInfo.enBitrateType == Video_NS::BitrateType_E::CBR)
		{
			strBitrateType = "CBR";
		}else if(stInfo.enBitrateType == Video_NS::BitrateType_E::VBR)
		{
			strBitrateType = "VBR";
		}else
		{
			strBitrateType = "CBR";
		}
		convert.field(pRootJson, "BitrateType", strBitrateType);
	}
	convert.field(pRootJson, "ImageQuality", (int &)stInfo.enImageQuality);
	convert.field(pRootJson, "FrameRate", (int &)stInfo.enFrameRate);
	// convert.field(pRootJson, "FrameRate", stInfo.nFrameRate);
	convert.field(pRootJson, "BitrateUpperLimit", stInfo.nBitrateUpperLimit);
	convert.field(pRootJson, "AverageBitrate", stInfo.nAverageBitrate);

	std::string strVideoCodec;
	if(bOutStruct)
	{
		convert.field(pRootJson, "VideoCodec", strVideoCodec);
		stInfo.enVideoCodec = Video_NS::string_toVideoCodec(strVideoCodec);
	}else{
		strVideoCodec = Video_NS::videoCodec_toString(stInfo.enVideoCodec);
		convert.field(pRootJson, "VideoCodec", strVideoCodec);
	}

	convert.field(pRootJson, "SmartEnable", stInfo.bSmartEnable);
	convert.field(pRootJson, "EncodingComplexity", (int &)stInfo.enEncodingComplexity);
	convert.field(pRootJson, "IFrameInterval", stInfo.nIFrameInterval);
	convert.field(pRootJson, "SvcEnable", (int &)stInfo.enSvcEnable);
	convert.field(pRootJson, "BitrateSmoothing", stInfo.nBitrateSmoothing);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Video_NS::VideoConfig_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "VideoConfig", stInfo);
}

void Convert::deal(Json::Object *pRootJson, std::set<Video_NS::VideoConfig_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "VideoConfig", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::VideoRoi_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Enable", stInfo.bEnable);
	convert.field(pRootJson, "Idx", stInfo.u32Idx);
	convert.field(pRootJson, "Level", stInfo.u32Level);
	convert.field(pRootJson, "RegionName", stInfo.strRegionName);
	convert.structure(pRootJson, "Rect", stInfo.stRect);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Video_NS::VideoRoi_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "VideoRoi", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::VideoRoiConfig_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Id", stInfo.nId);
	convert.structure(pRootJson, "VideoRoi", stInfo.vstVideoRoi);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Video_NS::VideoRoiConfig_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "VideoRoiConfig", stInfo);
}

void Convert::deal(Json::Object *pRootJson, std::set<Video_NS::VideoRoiConfig_S> &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "VideoRoiConfig", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::EncodeAbility_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "VideoCodec", stInfo.strVideoCodec);
    convert.field(pRootJson, "SupportAdjustComplexity", stInfo.nSupportAdjustComplexity);
    convert.field(pRootJson, "EncodeComplexity", stInfo.vEncodeComplexity);
    convert.field(pRootJson, "EncodeComplexityNum", stInfo.nEncodeComplexityNum);
    convert.field(pRootJson, "DefaultComplexity", stInfo.nDefaultComplexity);
    convert.field(pRootJson, "SupportSVC", stInfo.bSupportSVC);
    convert.field(pRootJson, "SupportStreamSmooth", stInfo.bSupportStreamSmooth);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Video_NS::EncodeAbility_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "EncodeAbility", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::Resolution_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Name", stInfo.strName);
    convert.field(pRootJson, "FrameRateMin", (int &)stInfo.enFrameRateMin);
    convert.field(pRootJson, "FrameRateMax", (int &)stInfo.enFrameRateMax);
    if (bOutStruct)
    {
        std::vector<int> frameRates;
        convert.field(pRootJson, "FrameRates", frameRates);
        stInfo.aFrameRates.clear();
        for (size_t i = 0; i < frameRates.size(); ++i)
        {
            Video_NS::VideoConfig_S stVideoConfig;
            stVideoConfig.setFrameRate(frameRates[i]);
            if (stVideoConfig.getFrameRateAsInt() == frameRates[i])
            {
                stInfo.aFrameRates.push_back(stVideoConfig.enFrameRate);
            }
        }
    }
    else
    {
        std::vector<int> frameRates;
        frameRates.reserve(stInfo.aFrameRates.size());
        for (size_t i = 0; i < stInfo.aFrameRates.size(); ++i)
        {
            Video_NS::VideoConfig_S stVideoConfig;
            stVideoConfig.enFrameRate = stInfo.aFrameRates[i];
            frameRates.push_back(stVideoConfig.getFrameRateAsInt());
        }
        convert.field(pRootJson, "FrameRates", frameRates);
    }
    convert.field(pRootJson, "BitRateMin", stInfo.nBitRateMin);
    convert.field(pRootJson, "BitRateMax", stInfo.nBitRateMax);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Video_NS::Resolution_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Resolution", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::VideoCapability_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.field(pRootJson, "SupportMultiStream", stInfo.bSupportMultiStream);
    convert.structure(pRootJson, "Resolution", stInfo.aResolution);
    convert.field(pRootJson, "ResolutionNum", stInfo.nResolutionNum);
    convert.structure(pRootJson, "EncodeAbility", stInfo.aEncodeAbility);
    convert.field(pRootJson, "EncodeTypeNum", stInfo.nEncodeTypeNum);
    convert.field(pRootJson, "IFrameIntervalMin", stInfo.nIFrameIntervalMin);
    convert.field(pRootJson, "IFrameIntervalMax", stInfo.nIFrameIntervalMax);
    convert.field(pRootJson, "StreamSmoothMin", stInfo.nStreamSmoothMin);
    convert.field(pRootJson, "StreamSmoothMax", stInfo.nStreamSmoothMax);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::VideoCapabilitySet_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Main", stInfo.stMain);
    convert.structure(pRootJson, "Sub", stInfo.stSub);
}

void Convert::deal(Json::Object *pRootJson, Video_NS::AreaCrop_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Id", stInfo.nId);
	std::string strResolution;
	if(bOutStruct)
	{
		convert.field(pRootJson, "Resolution", strResolution);
		stInfo.stResolution.parse_string(strResolution);
	}else {
		strResolution = stInfo.stResolution.to_string();
		convert.field(pRootJson, "Resolution", strResolution);
	}
	convert.structure(pRootJson, "Rect", stInfo.stRect);
}

void Convert::deal(Json::Object *pRootJson, std::set<Video_NS::AreaCrop_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "AreaCrop", stInfo);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Video_NS::AreaCrop_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "AreaCrop", stInfo);
}
