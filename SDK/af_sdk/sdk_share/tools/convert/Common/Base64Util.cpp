/**
 * @file Base64Util.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief Base64Util 模块实现
 * 功能说明：
 * 1. 实现 Base64Util 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "Base64Util.h"

#include <cctype>

namespace SDKConvert
{
static const char kBase64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsBase64Char 对应的数据。
 * @param [in] byCh 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static inline bool IsBase64Char(unsigned char byCh)
{
    return (std::isalnum(byCh) || (byCh == '+') || (byCh == '/') || (byCh == '='));
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 Base64Encode 定义的内部处理。
 * @param [in] pbyBytesToEncode 函数处理参数。
 * @param [in] uInLen 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string Base64Encode(const unsigned char* pbyBytesToEncode, size_t uInLen)
{
    std::string strRet;
    strRet.reserve(((uInLen + 2) / 3) * 4);
    unsigned char byCharArray3[3];
    unsigned char byCharArray4[4];
    int i = 0;

    while (uInLen--)
    {
        byCharArray3[i++] = *(pbyBytesToEncode++);
        if (i == 3)
        {
            byCharArray4[0] = (byCharArray3[0] & 0xfc) >> 2;
            byCharArray4[1] = ((byCharArray3[0] & 0x03) << 4) + ((byCharArray3[1] & 0xf0) >> 4);
            byCharArray4[2] = ((byCharArray3[1] & 0x0f) << 2) + ((byCharArray3[2] & 0xc0) >> 6);
            byCharArray4[3] = byCharArray3[2] & 0x3f;

            for (i = 0; i < 4; i++)
            {
                strRet.push_back(kBase64Table[byCharArray4[i]]);
            }
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; j++) byCharArray3[j] = '\0';

        byCharArray4[0] = (byCharArray3[0] & 0xfc) >> 2;
        byCharArray4[1] = ((byCharArray3[0] & 0x03) << 4) + ((byCharArray3[1] & 0xf0) >> 4);
        byCharArray4[2] = ((byCharArray3[1] & 0x0f) << 2) + ((byCharArray3[2] & 0xc0) >> 6);
        byCharArray4[3] = byCharArray3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
        {
            strRet.push_back(kBase64Table[byCharArray4[j]]);
        }

        while ((i++ < 3))
        {
            strRet.push_back('=');
        }
    }

    return strRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 Base64Decode 定义的内部处理。
 * @param [in] strEncoded 函数处理参数。
 * @param [out] vecOut 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Base64Decode(const std::string& strEncoded, std::vector<unsigned char>& vecOut)
{
    vecOut.clear();
    int nInLen = (int)strEncoded.size();
    int i = 0;
    int nInPos = 0;
    unsigned char byCharArray4[4], byCharArray3[3];

    auto fnDecodeValue = [](unsigned char byCh) -> int {
        if (byCh >= 'A' && byCh <= 'Z') return byCh - 'A';
        if (byCh >= 'a' && byCh <= 'z') return byCh - 'a' + 26;
        if (byCh >= '0' && byCh <= '9') return byCh - '0' + 52;
        if (byCh == '+') return 62;
        if (byCh == '/') return 63;
        return -1;
    };

    while (nInLen-- && (strEncoded[nInPos] != '=') && IsBase64Char((unsigned char)strEncoded[nInPos]))
    {
        unsigned char byCh = (unsigned char)strEncoded[nInPos++];
        if (byCh == '\r' || byCh == '\n' || byCh == ' ' || byCh == '\t') continue;
        byCharArray4[i++] = byCh;

        if (i == 4)
        {
            int nVal0 = fnDecodeValue(byCharArray4[0]);
            int nVal1 = fnDecodeValue(byCharArray4[1]);
            int nVal2 = fnDecodeValue(byCharArray4[2]);
            int nVal3 = fnDecodeValue(byCharArray4[3]);
            if (nVal0 < 0 || nVal1 < 0 || nVal2 < 0 || nVal3 < 0) return false;

            byCharArray3[0] = (unsigned char)((nVal0 << 2) + ((nVal1 & 0x30) >> 4));
            byCharArray3[1] = (unsigned char)(((nVal1 & 0xf) << 4) + ((nVal2 & 0x3c) >> 2));
            byCharArray3[2] = (unsigned char)(((nVal2 & 0x3) << 6) + nVal3);

            vecOut.push_back(byCharArray3[0]);
            vecOut.push_back(byCharArray3[1]);
            vecOut.push_back(byCharArray3[2]);
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 4; j++) byCharArray4[j] = 'A';

        int nVal0 = fnDecodeValue(byCharArray4[0]);
        int nVal1 = fnDecodeValue(byCharArray4[1]);
        int nVal2 = fnDecodeValue(byCharArray4[2]);
        int nVal3 = fnDecodeValue(byCharArray4[3]);
        if (nVal0 < 0 || nVal1 < 0 || nVal2 < 0 || nVal3 < 0) return false;

        byCharArray3[0] = (unsigned char)((nVal0 << 2) + ((nVal1 & 0x30) >> 4));
        byCharArray3[1] = (unsigned char)(((nVal1 & 0xf) << 4) + ((nVal2 & 0x3c) >> 2));
        byCharArray3[2] = (unsigned char)(((nVal2 & 0x3) << 6) + nVal3);

        for (int j = 0; j < i - 1; j++)
        {
            vecOut.push_back(byCharArray3[j]);
        }
    }

    return true;
}
} /* namespace SDKConvert */

