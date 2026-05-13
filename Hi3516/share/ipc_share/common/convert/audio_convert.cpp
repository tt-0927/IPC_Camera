/**
 * @FilePath     : audio_convert.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-12 14:51:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-04 09:09:56
 * @Description  : 音频配置转换处理
 */

#include "audio_convert.h"
#include "convert.h" /* 这个要放在audio_convert.h的后面 */

using namespace Audio_NS;

void Convert::deal(Json::Object *pRootJson, Audio_NS::AudioRange_S &stRange, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Enable", stRange.bEnable);
	convert.field(pRootJson, "Min", stRange.nMin);
	convert.field(pRootJson, "Max", stRange.nMax);
	convert.field(pRootJson, "Step", stRange.nStep);
}

void Convert::deal(Json::Object *pRootJson, Audio_NS::AudioFormatCapability_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Format", stInfo.strFormat);
	convert.field(pRootJson, "SampleRates", stInfo.aSampleRates);
	convert.field(pRootJson, "BitRates", stInfo.aBitRates);
	convert.structure(pRootJson, "SampleRateRange", stInfo.stSampleRateRange);
	convert.structure(pRootJson, "BitRateRange", stInfo.stBitRateRange);
}

void Convert::deal(Json::Object *pRootJson, Audio_NS::AudioCapabilitySet_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "InputTypes", stInfo.aInputTypes);
	convert.field(pRootJson, "OutputTypes", stInfo.aOutputTypes);
	convert.field(pRootJson, "Formats", stInfo.aFormats);
	convert.structure(pRootJson, "FormatDetail", stInfo.aFormatDetail);
}

void Convert::deal(Json::Object *pRootJson, Audio_NS::AudioConfig_S &stAudioConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "AudioSwitch", stAudioConfig.bAudioSwitch);
	std::string strInputType;
	if(bOutStruct)
	{
		convert.field(pRootJson, "InputType", strInputType);
		stAudioConfig.enInputType = Audio_NS::string_toAudioInputType(strInputType);
	}else{
		strInputType = Audio_NS::audioInputType_toString(stAudioConfig.enInputType);
		convert.field(pRootJson, "InputType", strInputType);
	}

	std::string strFormat;
	if(bOutStruct)
	{
		convert.field(pRootJson, "Format", strFormat);
		stAudioConfig.enFormat = Audio_NS::string_toAudioFormat(strFormat);
	}else{
		strFormat = Audio_NS::audioFormat_toString(stAudioConfig.enFormat);
		convert.field(pRootJson, "Format", strFormat);
	}

	convert.field(pRootJson, "SampRate", (int &)stAudioConfig.enSampRate);
	convert.field(pRootJson, "BitRate", (int &)stAudioConfig.enBitRate);
	convert.field(pRootJson, "InputVolume", stAudioConfig.u32InputVolume);
	convert.field(pRootJson, "Denoise", stAudioConfig.bDenoise);

	std::string strOutputType;
	if(bOutStruct)
	{
		convert.field(pRootJson, "OutputType", strOutputType);
		stAudioConfig.enOutputType = Audio_NS::string_toAudioOutputType(strOutputType);
	}else{
		strOutputType = Audio_NS::audioOutputType_toString(stAudioConfig.enOutputType);
		convert.field(pRootJson, "OutputType", strOutputType);
	}
	convert.field(pRootJson, "OutputVolume", stAudioConfig.u32OutputVolume);
}
