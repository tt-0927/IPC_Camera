/**
 * @file WsQuery.cpp
 * @brief WsQuery.h 的实现
 */

#include "WsQuery.h"

namespace
{

/* 十六进制字符转数值，非法字符返回 -1 */
int hex_value(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

/* %XX 与 '+' 的 urldecode，非法转义原样保留 */
std::string url_decode(const std::string &strInput)
{
    std::string strOut;
    strOut.reserve(strInput.size());
    for (size_t i = 0; i < strInput.size(); ++i)
    {
        if (strInput[i] == '%' && i + 2 < strInput.size())
        {
            int nHigh = hex_value(strInput[i + 1]);
            int nLow = hex_value(strInput[i + 2]);
            if (nHigh >= 0 && nLow >= 0)
            {
                strOut.push_back(static_cast<char>((nHigh << 4) | nLow));
                i += 2;
                continue;
            }
        }
        if (strInput[i] == '+')
        {
            strOut.push_back(' ');
            continue;
        }
        strOut.push_back(strInput[i]);
    }
    return strOut;
}

} /* namespace */

namespace WsQuery
{

std::string extract_query(const std::string &strUri)
{
    const size_t nPos = strUri.find('?');
    if (nPos == std::string::npos)
    {
        return std::string();
    }
    return strUri.substr(nPos + 1);
}

std::map<std::string, std::string> parse(const std::string &strQuery)
{
    std::map<std::string, std::string> mapResult;
    size_t nStart = 0;
    while (nStart <= strQuery.size())
    {
        size_t nEnd = strQuery.find('&', nStart);
        if (nEnd == std::string::npos)
        {
            nEnd = strQuery.size();
        }
        const std::string strPair = strQuery.substr(nStart, nEnd - nStart);
        if (!strPair.empty())
        {
            const size_t nEq = strPair.find('=');
            if (nEq == std::string::npos)
            {
                mapResult[url_decode(strPair)] = std::string();
            }
            else
            {
                mapResult[url_decode(strPair.substr(0, nEq))] = url_decode(strPair.substr(nEq + 1));
            }
        }
        if (nEnd == strQuery.size())
        {
            break;
        }
        nStart = nEnd + 1;
    }
    return mapResult;
}

bool get(const std::map<std::string, std::string> &mapQuery, const std::string &strKey, std::string &strValue)
{
    auto it = mapQuery.find(strKey);
    if (it == mapQuery.end())
    {
        return false;
    }
    strValue = it->second;
    return true;
}

} /* namespace WsQuery */
