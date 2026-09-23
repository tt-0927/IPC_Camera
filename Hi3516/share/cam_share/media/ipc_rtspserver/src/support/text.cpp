/**
 * @FilePath     : text.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 文本工具实现
 */

#include "support/text.h"

#include <cctype>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
} // namespace

std::string Base64Encode(const std::uint8_t *data, std::size_t len)
{
    std::string out;
    if (data == nullptr || len == 0)
    {
        return out;
    }

    out.reserve(((len + 2u) / 3u) * 4u);

    std::size_t i = 0;
    while (i + 3u <= len)
    {
        const std::uint32_t triple = (static_cast<std::uint32_t>(data[i]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) |
                                     static_cast<std::uint32_t>(data[i + 2]);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3fu]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3fu]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3fu]);
        out.push_back(kBase64Alphabet[triple & 0x3fu]);
        i += 3u;
    }

    const std::size_t rest = len - i;
    if (rest == 1u)
    {
        const std::uint32_t triple = static_cast<std::uint32_t>(data[i]) << 16;
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3fu]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3fu]);
        out.push_back('=');
        out.push_back('=');
    }
    else if (rest == 2u)
    {
        const std::uint32_t triple = (static_cast<std::uint32_t>(data[i]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3fu]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3fu]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3fu]);
        out.push_back('=');
    }

    return out;
}

std::string HexEncode(const std::uint8_t *data, std::size_t len)
{
    static const char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(len * 2u);
    for (std::size_t i = 0; i < len; ++i)
    {
        out[i * 2u] = kHex[data[i] >> 4];
        out[i * 2u + 1u] = kHex[data[i] & 0x0fu];
    }
    return out;
}

bool EqualsIgnoreCase(StringView lhs, StringView rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i)
    {
        const unsigned char a = static_cast<unsigned char>(lhs[i]);
        const unsigned char b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b))
        {
            return false;
        }
    }
    return true;
}

StringView FindParameter(StringView text, StringView name)
{
    std::size_t pos = 0;
    while (pos < text.size())
    {
        const std::size_t found = text.find(name, pos);
        if (found == StringView::npos)
        {
            return {};
        }

        /* 只有位于串首或分隔符之后才算参数名匹配。 */
        const bool at_boundary = (found == 0) || (text[found - 1] == ';') || (text[found - 1] == ' ') || (text[found - 1] == '=');
        if (!at_boundary)
        {
            pos = found + 1;
            continue;
        }

        const std::size_t value_begin = found + name.size();
        const std::size_t value_end = text.find_first_of(";,", value_begin);
        return text.substr(value_begin, (value_end == StringView::npos ? text.size() : value_end) - value_begin);
    }
    return {};
}

StringView Trim(StringView text)
{
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
    {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
    {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string NormalizeUriPath(StringView uri)
{
    const StringView prefix = "rtsp://";
    if (uri.size() > prefix.size() && EqualsIgnoreCase(uri.substr(0, prefix.size()), prefix))
    {
        uri.remove_prefix(prefix.size());
        const std::size_t slash = uri.find('/');
        if (slash == StringView::npos)
        {
            return "/";
        }
        uri.remove_prefix(slash);
    }

    const std::size_t query = uri.find('?');
    if (query != StringView::npos)
    {
        uri = uri.substr(0, query);
    }

    std::string path(uri.data(), uri.size());
    if (path.empty() || path[0] != '/')
    {
        path.insert(path.begin(), '/');
    }
    return path;
}

} // namespace detail
} // namespace ipc_rtsp
