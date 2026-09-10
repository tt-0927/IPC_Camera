/**
 * @FilePath     : av_configure_audio_capability.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-03 15:36:42
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-03 16:27:10
 * @Description  : 音频能力集默认填充
 */

#include "av_configure.h"

/* 按编译期能力宏填充音频能力集，供 Web/配置校验统一使用 */
void CAVConfigure::fill_default_audio_capability_set(Audio_NS::AudioCapabilitySet_S &data) const
{
    data = Audio_NS::AudioCapabilitySet_S();

    data.aInputTypes.push_back("MicIn");
#if CAP_AUDIO_INPUT_LINEIN
    data.aInputTypes.push_back("LineIn");
#endif

    data.aOutputTypes.push_back("Speaker");
#if CAP_AUDIO_OUTPUT_LINEOUT
    data.aOutputTypes.push_back("LineOut");
#endif

    /* 同步维护 aFormats 与 aFormatDetail，确保格式与详细约束一一对应 */
    auto add_format_capability = [&data](const std::string &strFormat,
                                         const std::vector<int> &sampleRates,
                                         const std::vector<int> &bitRates,
                                         const Audio_NS::AudioRange_S &sampleRange,
                                         const Audio_NS::AudioRange_S &bitRange)
    {
        data.aFormats.push_back(strFormat);
        Audio_NS::AudioFormatCapability_S stCapability;
        stCapability.strFormat = strFormat;
        stCapability.aSampleRates = sampleRates;
        stCapability.aBitRates = bitRates;
        stCapability.stSampleRateRange = sampleRange;
        stCapability.stBitRateRange = bitRange;
        data.aFormatDetail.push_back(stCapability);
    };

    /* AAC 支持连续范围，前端可优先使用离散列表，后端再做范围兜底校验 */
    Audio_NS::AudioRange_S stAacSampleRange;
    stAacSampleRange.bEnable = true;
    stAacSampleRange.nMin = 8000;
    stAacSampleRange.nMax = 48000;
    stAacSampleRange.nStep = 1;

    Audio_NS::AudioRange_S stAacBitRateRange;
    stAacBitRateRange.bEnable = true;
    stAacBitRateRange.nMin = 16000;
    stAacBitRateRange.nMax = 128000;
    stAacBitRateRange.nStep = 1;

#if CAP_AUDIO_FMT_AAC
    add_format_capability("AAC",
                          { 8000, 16000, 22050, 24000, 32000, 44100, 48000 },
                          { 16000, 32000, 48000, 64000, 96000, 128000 },
                          stAacSampleRange,
                          stAacBitRateRange);
#endif

#if CAP_AUDIO_FMT_G711U
    add_format_capability("G.711ulaw",
                          { 8000 },
                          { 64000 },
                          Audio_NS::AudioRange_S(),
                          Audio_NS::AudioRange_S());
#endif

#if CAP_AUDIO_FMT_G711A
    add_format_capability("G.711alaw",
                          { 8000 },
                          { 64000 },
                          Audio_NS::AudioRange_S(),
                          Audio_NS::AudioRange_S());
#endif

#if CAP_AUDIO_FMT_PCM
    add_format_capability("PCM",
                          { 8000, 16000, 32000, 44100, 48000 },
                          { 128000, 256000 },
                          Audio_NS::AudioRange_S(),
                          Audio_NS::AudioRange_S());
#endif

#if CAP_AUDIO_FMT_G726
    add_format_capability("G.726",
                          { 8000 },
                          { 16000, 24000, 32000, 40000 },
                          Audio_NS::AudioRange_S(),
                          Audio_NS::AudioRange_S());
#endif

    if (data.aFormats.empty())
    {
        add_format_capability("AAC",
                              { 8000, 16000, 22050, 24000, 32000, 44100, 48000 },
                              { 16000, 32000, 48000, 64000, 96000, 128000 },
                              stAacSampleRange,
                              stAacBitRateRange);
        add_format_capability("G.711ulaw",
                              { 8000 },
                              { 64000 },
                              Audio_NS::AudioRange_S(),
                              Audio_NS::AudioRange_S());
        add_format_capability("G.711alaw",
                              { 8000 },
                              { 64000 },
                              Audio_NS::AudioRange_S(),
                              Audio_NS::AudioRange_S());
        add_format_capability("PCM",
                              { 8000, 16000, 32000, 44100, 48000 },
                              { 128000, 256000 },
                              Audio_NS::AudioRange_S(),
                              Audio_NS::AudioRange_S());
    }
}
