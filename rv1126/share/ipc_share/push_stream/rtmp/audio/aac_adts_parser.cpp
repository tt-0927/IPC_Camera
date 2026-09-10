/**
 * @FilePath     : aac_adts_parser.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:34:33
 * @Description  : AAC ADTS解析工具实现
 */

#include "aac_adts_parser.h"

#include "dlog.h"
#include "IpcRet.h"

namespace
{
    /* AAC采样率索引表，索引值来自ADTS标准 */
    const int AAC_SAMPLE_RATES[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000,
        22050, 16000, 12000, 11025, 8000, 7350
    };
}

namespace RtmpAudio_NS
{
    bool parse_adts_header(const uint8_t* pData, int nSize, AacAdtsInfo_S& stInfo)
    {
        if (!pData || nSize < 7)
        {
            /* AAC 解析在逐帧路径上，非 ADTS 或长度不足由调用方按裸 AAC 回退处理。 */
            /* dlog_debug("ADTS解析失败：数据为空或长度不足7，nSize=%d", nSize); */
            return false;
        }

        /* 检查是否为 ADTS 同步字 (0xFF F0)，如果不是则认为是裸 AAC。 */
        if (pData[0] != 0xff || (pData[1] & 0xf0) != 0xf0)
        {
            return false;
        }

        const int nSampleRateIndex = (pData[2] & 0x3c) >> 2;
        const int nSampleRateCount = static_cast<int>(sizeof(AAC_SAMPLE_RATES) / sizeof(AAC_SAMPLE_RATES[0]));
        if (nSampleRateIndex < 0 || nSampleRateIndex >= nSampleRateCount)
        {
            dlog_warn("ADTS采样率索引非法：index=%d, count=%d", nSampleRateIndex, nSampleRateCount);
            return false;
        }

        stInfo.profile = (pData[2] & 0xc0) >> 6;
        stInfo.object_type = stInfo.profile + 1;
        stInfo.sample_rate_index = nSampleRateIndex;
        stInfo.sample_rate = AAC_SAMPLE_RATES[nSampleRateIndex];
        stInfo.channels = ((pData[2] & 0x01) << 2) | ((pData[3] & 0xc0) >> 6);
        stInfo.header_size = (pData[1] & 0x01) ? 7 : 9;

        if (stInfo.object_type <= 0 || stInfo.channels <= 0)
        {
            dlog_warn("ADTS参数异常：object_type=%d, channels=%d", stInfo.object_type, stInfo.channels);
            return false;
        }
        return true;
    }

    int find_sample_rate_index(int nSampleRate)
    {
        const int nCount = static_cast<int>(sizeof(AAC_SAMPLE_RATES) / sizeof(AAC_SAMPLE_RATES[0]));
        for (int i = 0; i < nCount; ++i)
        {
            if (AAC_SAMPLE_RATES[i] == nSampleRate)
            {
                return i;
            }
        }
        return ERR;
    }
}
