/**
 * @FilePath     : video_convert.h
 * @Author       : zhouzirui
 * @Date         : 2025-05-12 14:39:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-19 13:50:50
 * @Description  : 视频配置转换处理 
 */

#pragma once

#include <set>
#include "Json.h"
#include "video_define.h"

namespace Convert
{
	void deal(Json::Object *pRootJson, Video_NS::VideoConfig_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Video_NS::VideoConfig_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::set<Video_NS::VideoConfig_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Video_NS::VideoRoi_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Video_NS::VideoRoi_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Video_NS::VideoRoiConfig_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Video_NS::VideoRoiConfig_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::set<Video_NS::VideoRoiConfig_S> &stInfo, bool bOutStruct);

	void deal(Json::Object *pRootJson, Video_NS::EncodeAbility_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Video_NS::EncodeAbility_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Video_NS::Resolution_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Video_NS::Resolution_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Video_NS::VideoCapability_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Video_NS::VideoCapabilitySet_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Video_NS::AreaCrop_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::set<Video_NS::AreaCrop_S> &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, std::vector<Video_NS::AreaCrop_S> &stInfo, bool bOutStruct);
}
