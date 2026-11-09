/**
 * @FilePath     : classroom_fmt.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 10:56:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 10:56:00
 * @Description  : appendFmt / fmtMetricLine 的两趟实现
 */

#include "classroom_fmt.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace Classroom
{

void appendFmt(std::string &rstrJson, const char *pFormat, ...)
{
    char astBuf[512];

    va_list stArgs;
    va_start(stArgs, pFormat);
    const int nLen = vsnprintf(astBuf, sizeof(astBuf), pFormat, stArgs);
    va_end(stArgs);

    if (nLen < 0)
    {
        return;
    }
    if (static_cast<size_t>(nLen) < sizeof(astBuf))
    {
        rstrJson.append(astBuf, static_cast<size_t>(nLen));
        return;
    }

    /* 第二趟按实长拿全量。第二趟的实参是同一批对象，长度可复现；
       万一不可复现，vsnprintf 自己按 size 截到 resize 出来的容量，
       不会留半个 NUL——但仍**完整落一段可判读的前缀**，绝不静吞 */
    std::string strChunk(static_cast<size_t>(nLen) + 1, '\0');
    va_start(stArgs, pFormat);
    const int nLen2 = vsnprintf(&strChunk[0], strChunk.size(), pFormat, stArgs);
    va_end(stArgs);
    if (nLen2 > 0)
    {
        strChunk.resize(static_cast<size_t>(std::min(nLen2, static_cast<int>(strChunk.size()) - 1)));
        rstrJson.append(strChunk);
    }
}

std::string fmtMetricLine(const char *pFormat, ...)
{
    char astBuf[4096];
    int nLen = 0;

    va_list stArgs;
    va_start(stArgs, pFormat);
    nLen = vsnprintf(astBuf, sizeof(astBuf), pFormat, stArgs);
    va_end(stArgs);

    std::string strOut;
    if (nLen < 0)
    {
        return strOut;
    }
    if (static_cast<size_t>(nLen) < sizeof(astBuf))
    {
        strOut.assign(astBuf, static_cast<size_t>(nLen));
        return strOut;
    }

    strOut.resize(static_cast<size_t>(nLen) + 1);
    va_start(stArgs, pFormat);
    /* 第二趟的实参是同一批对象（本拍内不会变），所以长度可复现；
       万一不可复现，vsnprintf 自己按 size 截，resize 到实长即可，不会留半个 NUL */
    nLen = vsnprintf(&strOut[0], strOut.size(), pFormat, stArgs);
    va_end(stArgs);
    strOut.resize((nLen < 0) ? 0 : std::min(static_cast<size_t>(nLen), strOut.size() - 1));
    return strOut;
}

std::string truncateUtf8(const std::string &strIn, int32_t nMaxBytes, bool &rbTruncated)
{
    rbTruncated = false;
    if (nMaxBytes <= 0 || static_cast<int32_t>(strIn.size()) <= nMaxBytes)
    {
        return strIn;
    }
    size_t nCut = static_cast<size_t>(nMaxBytes);
    /* 回退到本字符起始处：UTF-8 续字节形如 10xxxxxx（0x80~0xBF） */
    while (nCut > 0 && (static_cast<unsigned char>(strIn[nCut]) & 0xC0U) == 0x80U)
    {
        --nCut;
    }
    rbTruncated = true;
    return strIn.substr(0, nCut);
}

} /* namespace Classroom */
