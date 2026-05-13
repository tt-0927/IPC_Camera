/**
 * @FilePath     : audio_convert.h
 * @Author       : zhouzirui
 * @Date         : 2025-05-12 14:39:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-26 14:11:15
 * @Description  : 音频配置转换处理 
 */

#pragma once

#include "Json.h"
#include "audio_define.h"

namespace Convert
{
	void deal(Json::Object *pRootJson, Audio_NS::AudioRange_S &stRange, bool bOutStruct);
	void deal(Json::Object *pRootJson, Audio_NS::AudioFormatCapability_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Audio_NS::AudioCapabilitySet_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Audio_NS::AudioConfig_S &stAudioConfig, bool bOutStruct);
}
