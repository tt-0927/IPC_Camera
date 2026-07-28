/**
 * @file Base64Util.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief Base64Util 模块接口与类型定义
 * 功能说明：
 * 1. 声明 Base64Util 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once

#include <string>
#include <vector>

namespace SDKConvert
{
    /* Base64 encode raw bytes -> string */
    std::string Base64Encode(const unsigned char* bytes, size_t len);

    /* Base64 decode string -> raw bytes (returns false on decode error) */
    bool Base64Decode(const std::string& b64, std::vector<unsigned char>& out);
}

