#pragma once

#include <string>
#include <vector>

namespace SDKConvert
{
    // Base64 encode raw bytes -> string
    std::string Base64Encode(const unsigned char* bytes, size_t len);

    // Base64 decode string -> raw bytes (returns false on decode error)
    bool Base64Decode(const std::string& b64, std::vector<unsigned char>& out);
}

