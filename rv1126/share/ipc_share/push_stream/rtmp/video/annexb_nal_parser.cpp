/**
 * @FilePath     : annexb_nal_parser.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:37:30
 * @Description  : AnnexB NAL解析工具实现
 */

#include "annexb_nal_parser.h"

namespace
{
    /**
     * @brief 判断当前位置是否为AnnexB起始码
     * @param pData 数据指针
     * @param nSize 数据长度
     * @param nPos 检查位置
     * @param nStartCodeLen 输出起始码长度
     * @return true：当前位置是起始码，false：不是起始码
     */
    bool is_start_code(const uint8_t* pData, int nSize, int nPos, int& nStartCodeLen)
    {
        if (nPos + 3 < nSize && pData[nPos] == 0x00 && pData[nPos + 1] == 0x00 && pData[nPos + 2] == 0x01)
        {
            nStartCodeLen = 3;
            return true;
        }
        if (nPos + 4 < nSize && pData[nPos] == 0x00 && pData[nPos + 1] == 0x00 &&
            pData[nPos + 2] == 0x00 && pData[nPos + 3] == 0x01)
        {
            nStartCodeLen = 4;
            return true;
        }
        return false;
    }

    /**
     * @brief 查找下一个AnnexB起始码
     * @param pData 数据指针
     * @param nSize 数据长度
     * @param nPos 起始查找位置
     * @return 起始码位置，未找到返回nSize
     */
    int find_next_start_code(const uint8_t* pData, int nSize, int nPos)
    {
        int nStartCodeLen = 0;
        for (int i = nPos; i < nSize - 3; ++i)
        {
            if (is_start_code(pData, nSize, i, nStartCodeLen))
            {
                return i;
            }
        }
        return nSize;
    }
}

namespace RtmpVideo_NS
{
    bool extract_h264_parameter_sets(const uint8_t* pData, int nSize, VideoParameterSets_S& stSets)
    {
        if (!pData || nSize <= 4)
        {
            return false;
        }

        stSets = VideoParameterSets_S();
        int nPos = 0;
        while (nPos < nSize - 4)
        {
            int nStartCodeLen = 0;
            if (!is_start_code(pData, nSize, nPos, nStartCodeLen))
            {
                ++nPos;
                continue;
            }

            const int nNalStart = nPos + nStartCodeLen;
            const int nNextStart = find_next_start_code(pData, nSize, nNalStart);
            const int nNalLen = nNextStart - nNalStart;
            if (nNalLen > 0)
            {
                const int nNalType = pData[nNalStart] & 0x1f;
                if (nNalType == 7)
                {
                    stSets.sps.assign(pData + nNalStart, pData + nNalStart + nNalLen);
                }
                else if (nNalType == 8)
                {
                    stSets.pps.assign(pData + nNalStart, pData + nNalStart + nNalLen);
                }
            }
            nPos = nNextStart;
        }

        return !stSets.sps.empty() && !stSets.pps.empty();
    }

    bool extract_h265_parameter_sets(const uint8_t* pData, int nSize, VideoParameterSets_S& stSets)
    {
        if (!pData || nSize <= 5)
        {
            return false;
        }

        stSets = VideoParameterSets_S();
        int nPos = 0;
        while (nPos < nSize - 5)
        {
            int nStartCodeLen = 0;
            if (!is_start_code(pData, nSize, nPos, nStartCodeLen))
            {
                ++nPos;
                continue;
            }

            const int nNalStart = nPos + nStartCodeLen;
            const int nNextStart = find_next_start_code(pData, nSize, nNalStart);
            const int nNalLen = nNextStart - nNalStart;
            if (nNalLen > 1)
            {
                const int nNalType = (pData[nNalStart] >> 1) & 0x3f;
                if (nNalType == 32)
                {
                    stSets.vps.assign(pData + nNalStart, pData + nNalStart + nNalLen);
                }
                else if (nNalType == 33)
                {
                    stSets.sps.assign(pData + nNalStart, pData + nNalStart + nNalLen);
                }
                else if (nNalType == 34)
                {
                    stSets.pps.assign(pData + nNalStart, pData + nNalStart + nNalLen);
                }
            }
            nPos = nNextStart;
        }

        return !stSets.vps.empty() && !stSets.sps.empty() && !stSets.pps.empty();
    }

    bool has_key_frame_nal(const uint8_t* pData, int nSize, Video_NS::VideoCodec_E enCodec)
    {
        if (!pData || nSize < 5)
        {
            return false;
        }

        int nPos = 0;
        while (nPos < nSize - 4)
        {
            int nStartCodeLen = 0;
            if (!is_start_code(pData, nSize, nPos, nStartCodeLen))
            {
                ++nPos;
                continue;
            }

            const int nNalStart = nPos + nStartCodeLen;
            if (nNalStart >= nSize)
            {
                break;
            }

            if (enCodec == Video_NS::VideoCodec_E::H264)
            {
                const int nNalType = pData[nNalStart] & 0x1f;
                if (nNalType == 5) /* IDR */
                {
                    return true;
                }
            }
            else if (enCodec == Video_NS::VideoCodec_E::H265)
            {
                const int nNalType = (pData[nNalStart] >> 1) & 0x3f;
                if (nNalType == 19 || nNalType == 20 || nNalType == 21) /* IDR_W_RADL / IDR_N_LP / CRA */
                {
                    return true;
                }
            }

            /* 跳到下一个起始码，避免在同一个NAL内部逐字节重复识别。 */
            nPos = find_next_start_code(pData, nSize, nNalStart);
        }

        return false;
    }
}
