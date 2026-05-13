#include "Base64Util.h"

#include <cctype>

namespace SDKConvert
{
static const char kBase64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline bool IsBase64Char(unsigned char c)
{
    return (std::isalnum(c) || (c == '+') || (c == '/') || (c == '='));
}

std::string Base64Encode(const unsigned char* bytes_to_encode, size_t in_len)
{
    std::string ret;
    ret.reserve(((in_len + 2) / 3) * 4);
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int i = 0;

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
            {
                ret.push_back(kBase64Table[char_array_4[i]]);
            }
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; j++) char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
        {
            ret.push_back(kBase64Table[char_array_4[j]]);
        }

        while ((i++ < 3))
        {
            ret.push_back('=');
        }
    }

    return ret;
}

bool Base64Decode(const std::string& encoded, std::vector<unsigned char>& out)
{
    out.clear();
    int in_len = (int)encoded.size();
    int i = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];

    auto decode_value = [](unsigned char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };

    while (in_len-- && (encoded[in_] != '=') && IsBase64Char((unsigned char)encoded[in_]))
    {
        unsigned char c = (unsigned char)encoded[in_++];
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') continue;
        char_array_4[i++] = c;

        if (i == 4)
        {
            int v0 = decode_value(char_array_4[0]);
            int v1 = decode_value(char_array_4[1]);
            int v2 = decode_value(char_array_4[2]);
            int v3 = decode_value(char_array_4[3]);
            if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;

            char_array_3[0] = (unsigned char)((v0 << 2) + ((v1 & 0x30) >> 4));
            char_array_3[1] = (unsigned char)(((v1 & 0xf) << 4) + ((v2 & 0x3c) >> 2));
            char_array_3[2] = (unsigned char)(((v2 & 0x3) << 6) + v3);

            out.push_back(char_array_3[0]);
            out.push_back(char_array_3[1]);
            out.push_back(char_array_3[2]);
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 4; j++) char_array_4[j] = 'A';

        int v0 = decode_value(char_array_4[0]);
        int v1 = decode_value(char_array_4[1]);
        int v2 = decode_value(char_array_4[2]);
        int v3 = decode_value(char_array_4[3]);
        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;

        char_array_3[0] = (unsigned char)((v0 << 2) + ((v1 & 0x30) >> 4));
        char_array_3[1] = (unsigned char)(((v1 & 0xf) << 4) + ((v2 & 0x3c) >> 2));
        char_array_3[2] = (unsigned char)(((v2 & 0x3) << 6) + v3);

        for (int j = 0; j < i - 1; j++)
        {
            out.push_back(char_array_3[j]);
        }
    }

    return true;
}
} // namespace SDKConvert

