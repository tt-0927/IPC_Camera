/**
 * @FilePath     : flv_packet_converter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:36:01
 * @Description  : FLV写包前码流格式转换工具实现
 */

#include "flv_packet_converter.h"

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
}

namespace RtmpFlv_NS
{
    bool annexb_to_mp4(const uint8_t* pData, int nSize, std::vector<uint8_t>& vMp4Data)
    {
        if (!pData || nSize <= 4)
        {
            return false;
        }

        vMp4Data.clear();
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

            int nNextStart = nSize;
            int nNextStartCodeLen = 0;
            for (int i = nNalStart; i < nSize - 3; ++i)
            {
                if (is_start_code(pData, nSize, i, nNextStartCodeLen))
                {
                    nNextStart = i;
                    break;
                }
            }

            const int nNalLen = nNextStart - nNalStart;
            if (nNalLen > 0)
            {
                /* 写入4字节长度前缀（大端序），供 FFmpeg HEVC FLV 写包逻辑直接使用。 */
                vMp4Data.push_back(static_cast<uint8_t>((nNalLen >> 24) & 0xff));
                vMp4Data.push_back(static_cast<uint8_t>((nNalLen >> 16) & 0xff));
                vMp4Data.push_back(static_cast<uint8_t>((nNalLen >> 8) & 0xff));
                vMp4Data.push_back(static_cast<uint8_t>(nNalLen & 0xff));
                vMp4Data.insert(vMp4Data.end(), pData + nNalStart, pData + nNalStart + nNalLen);
            }

            nPos = nNextStart;
        }

        return !vMp4Data.empty();
    }
}
