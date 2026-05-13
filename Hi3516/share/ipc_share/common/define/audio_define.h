/**
 * @FilePath     : audio_define.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 15:35:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-04 09:34:52
 * @Description  : 音频定义
 */
#pragma once

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>

/*音频定义命名空间*/
namespace Audio_NS
{
    /* 音频输入类型 */
    typedef enum class AudioInputType
    {
        MICIN = 0, /* 麦克风输入 */
        LINEIN,  /* 线路输入 */
    } AudioInputType_E;

    /* 音频输出类型 */
    typedef enum class AudioOutputType
    {
        SPEAKER = 0, /* 扬声器输出 */
        LINEOUT,     /* 线路输出 */
        MUTE         /* 静音 */
    } AudioOutputType_E;

    /**
     * @brief   : 音频输入类型枚举转string
     * @param    {AudioInputType_E} enType 音频输入类型枚举
     * @return   {std::string} 音频输入类型 string 结果
     */
    inline std::string audioInputType_toString(AudioInputType_E enType)
    {
        switch (enType)
        {
        case AudioInputType_E::MICIN: return "MicIn";
        case AudioInputType_E::LINEIN: return "LineIn";
        default:
            return "MicIn";
        }
    }

    /**
     * @brief   : string转音频输入类型枚举
     * @param    {string&} str：音频输入类型 string
     * @return   {AudioInputType_E} 音频输入类型
     */
    inline AudioInputType_E string_toAudioInputType(const std::string& str) 
    {
        if (str == "MicIn") return AudioInputType_E::MICIN;
        if (str == "LineIn") return AudioInputType_E::LINEIN;
        return AudioInputType_E::MICIN;
    }

    /**
     * @brief   : 音频输出类型枚举转string
     * @param    {AudioOutputType_E} enType 音频输出类型枚举
     * @return   {std::string} 音频输出类型 string 结果
     */
    inline std::string audioOutputType_toString(AudioOutputType_E enType)
    {
        switch (enType)
        {
            case AudioOutputType_E::SPEAKER: return "Speaker";
            case AudioOutputType_E::LINEOUT: return "LineOut";
            default:
                return "Speaker";
        }
    }

    /**
    * @brief   : string转音频输出类型枚举
    * @param    {string&} str：音频输出类型 string
    * @return   {AudioOutputType_E} 音频输出类型枚举
    */
    inline AudioOutputType_E string_toAudioOutputType(const std::string& str) 
    {
        if (str == "Speaker") return AudioOutputType_E::SPEAKER;
        if (str == "LineOut") return AudioOutputType_E::LINEOUT;
        return AudioOutputType_E::SPEAKER;
    }

    /* 音频格式 */
    typedef enum class AudioFormat
    {
        G722_1 = 0,
        G711U,
        G711A,
        MP2L2,
        G726,
        AAC,
        PCM,
        MP3,
    } AudioFormat_E;

    /**
     * @brief   : 音频格式枚举转string
     * @param    {AudioFormat_E} enType 音频格式枚举
     * @return   {std::string} 音频格式 string 结果
     */
    inline std::string audioFormat_toString(AudioFormat_E enType)
    {
        switch (enType)
        {
            case AudioFormat_E::G722_1: return "G.722.1";
            case AudioFormat_E::G711U: return "G.711ulaw";
            case AudioFormat_E::G711A: return "G.711alaw";
            case AudioFormat_E::MP2L2: return "MP2L2";
            case AudioFormat_E::G726: return "G.726";
            case AudioFormat_E::AAC: return "AAC";
            case AudioFormat_E::PCM: return "PCM";
            case AudioFormat_E::MP3: return "MP3";
            default:
                return "AAC";
        }
    }

    /**
     * @brief   : string转音频格式枚举
     * @param    {string&} str：音频格式 string
     * @return   {AudioFormat_E} 音频格式枚举
     */
    inline AudioFormat_E string_toAudioFormat(const std::string& str) 
    {
        if (str == "G.722.1") return AudioFormat_E::G722_1;
        if (str == "G.711ulaw") return AudioFormat_E::G711U;
        if (str == "G.711alaw") return AudioFormat_E::G711A;
        if (str == "MP2L2") return AudioFormat_E::MP2L2;
        if (str == "G.726") return AudioFormat_E::G726;
        if (str == "AAC") return AudioFormat_E::AAC;
        if (str == "PCM") return AudioFormat_E::PCM;
        if (str == "MP3") return AudioFormat_E::MP3;
        return AudioFormat_E::AAC;
    }

    /*音频采样率*/
    typedef enum class AudioSamprate
    {
        AUDIO_SAMPRATE_8000     = 8000,
        AUDIO_SAMPRATE_11025    = 11025,
        AUDIO_SAMPRATE_12000    = 12000,
        AUDIO_SAMPRATE_16000    = 16000,
        AUDIO_SAMPRATE_22050    = 22050,
        AUDIO_SAMPRATE_24000    = 24000,
        AUDIO_SAMPRATE_32000    = 32000,
        AUDIO_SAMPRATE_44100    = 44100,
        AUDIO_SAMPRATE_48000    = 48000,
        AUDIO_SAMPRATE_64000    = 64000,
        AUDIO_SAMPRATE_96000    = 96000,
    } AudioSamprate_E;

    /*音频码率*/
    typedef enum class AudioBitrate
    {
        AUDIO_BITRATE_16K = 16000,
        AUDIO_BITRATE_32K = 32000,
        AUDIO_BITRATE_48K = 48000,
        AUDIO_BITRATE_64K = 64000,
        AUDIO_BITRATE_96K = 96000,
        AUDIO_BITRATE_128K = 128000,
        AUDIO_BITRATE_256K = 256000,
    } AudioBitrate_E;

    /* 数值范围 */
    typedef struct AudioRange
    {
        bool bEnable; /* 是否启用范围约束，false 表示不校验范围 */
        int nMin;     /* 允许的最小值（闭区间） */
        int nMax;     /* 允许的最大值（闭区间） */
        int nStep;    /* 步长，<=0 表示不做步长离散校验 */

        AudioRange() :
            bEnable(false),
            nMin(0),
            nMax(0),
            nStep(0)
        {
        }
    } AudioRange_S;

    /* 音频格式能力 */
    typedef struct AudioFormatCapability
    {
        std::string strFormat;           /* 音频格式字符串 */
        std::vector<int> aSampleRates;   /* 支持的采样率列表 */
        std::vector<int> aBitRates;      /* 支持的码率列表 */
        AudioRange_S stSampleRateRange;  /* 采样率范围（预留） */
        AudioRange_S stBitRateRange;     /* 码率范围（预留） */

        AudioFormatCapability()
        {
            strFormat.clear();
            aSampleRates.clear();
            aBitRates.clear();
        }
    } AudioFormatCapability_S;

    /* 音频能力集 */
    typedef struct AudioCapabilitySet
    {
        std::vector<std::string> aInputTypes;               /* 支持的输入类型字符串列表（如 MicIn/LineIn） */
        std::vector<std::string> aOutputTypes;              /* 支持的输出类型字符串列表（如 Speaker/LineOut） */
        std::vector<std::string> aFormats;                  /* 支持的音频编码格式列表（与 strFormat 对应） */
        std::vector<AudioFormatCapability_S> aFormatDetail; /* 每种编码格式的采样率/码率细粒度能力约束 */

        AudioCapabilitySet()
        {
            aInputTypes.clear();
            aOutputTypes.clear();
            aFormats.clear();
            aFormatDetail.clear();
        }
    } AudioCapabilitySet_S;

    /*音频配置*/
    typedef struct AudioConfig
    {
        bool bAudioSwitch; /* 音频开关 */
        /* 音频输入 */
        AudioInputType_E enInputType; /* 输入类型 */
        AudioFormat_E enFormat;       /* 音频格式 */
        AudioSamprate_E enSampRate;   /* 采样率 */
        AudioBitrate_E enBitRate;     /* 码率 */
        uint32_t u32InputVolume;      /* 输入音量：0-100db */
        bool bDenoise;                /* 降噪开关 */
        /* 音频输出 */
        AudioOutputType_E enOutputType; /* 输出类型 */
        uint32_t u32OutputVolume;       /* 输出音量：0-100db */

        AudioConfig() :
            bAudioSwitch(true),
            enInputType(AudioInputType_E::MICIN),
            enFormat(AudioFormat_E::AAC),
            enSampRate(AudioSamprate_E::AUDIO_SAMPRATE_16000),
            enBitRate(AudioBitrate_E::AUDIO_BITRATE_48K),
            u32InputVolume(50),
            bDenoise(true),
            enOutputType(AudioOutputType_E::SPEAKER),
            u32OutputVolume(50)
        {
        }

        void print()
        {
            std::cout << "音频配置:" << std::endl;
            std::cout << "音频输入:" << std::endl;
            std::cout << "音频开关:" << bAudioSwitch << std::endl;
            std::cout << "输入类型:" << static_cast<int>(enInputType) << std::endl;
            std::cout << "音频格式:" << static_cast<int>(enFormat) << std::endl;
            std::cout << "采样率:" << static_cast<int>(enSampRate) << std::endl;
            std::cout << "码率:" << static_cast<int>(enBitRate) << std::endl;
            std::cout << "输入音量:" << u32InputVolume << std::endl;
            std::cout << "降噪开关:" << bDenoise << std::endl;
            std::cout << "音频输出:" << std::endl;
            std::cout << "输出类型:" << static_cast<int>(enOutputType) << std::endl;
            std::cout << "输出音量:" << u32InputVolume << std::endl;
            std::cout << std::endl;
        }
    } AudioConfig_S;

    /* 统一的音频帧结构 */
    typedef struct
    {
        AudioFormat_E enFormat; /* 音频格式 */
        int nLen;               /* 音频数据长度 */
        uint8_t pData[];        /* 柔性数组，音频数据紧跟在结构体后面 */
        // uint8_t *pData;             /* 音频数据指针 */
    } AudioFrame_S;

    /* 送到ao的信息 */
    typedef struct _AoInfo_S_
    {
        int nChannel;                 /* ao通道 */
        int nVolume = 50;             /* 音量 */
        uint8_t *pData;               /* 送给ao的音频数据 */
        uint32_t nLen;                /* 送给ao的音频数据长度 */
        AudioFormat_E enAudioFormat;  /* 音频编码格式 */
    }AoInfo_S;
};  // namespace Audio_NS
