
#pragma once

#include "base_type.hpp"
#include "dcl.h"
#include <functional>

typedef enum
{
    IMAGE_COLOR_BGR888_TO_BGR888_PLANAR = 0,
    IMAGE_COLOR_BGR888_TO_RGB888_PLANAR,
    IMAGE_COLOR_BGR888_TO_RGB888,
    IMAGE_COLOR_BGR888_TO_BGR888
} colorSpace_t;

namespace dcl
{
    static int bgr888ToBgr888_Planar(const dcl::Mat &src, dcl::Mat &dst)
    {
        const int c = src.c();
        const int h = src.h();
        const int w = src.w();
        size_t size = c * h * w;
        for (int i = 0; i < size; ++i)
        {
            int dc = i % c;
            int dh = i / c / w;
            int dw = i / c % w;
            dst.data[dc * h * w + dh * w + dw] = src.data[i];
        }
        dst.pixelFormat = DCL_PIXEL_FORMAT_BGR_888_PLANAR;
        return 0;
    }

    static int bgr888ToRgb888_Planar(const dcl::Mat &src, dcl::Mat &dst)
    {
        const int c = src.c();
        const int h = src.h();
        const int w = src.w();
        size_t size = c * h * w;
        for (int i = 0; i < size; ++i)
        {
            int dc = i % c;
            int dh = i / c / w;
            int dw = i / c % w;
            dst.data[(c - dc - 1) * h * w + dh * w + dw] = src.data[i];
        }
        dst.pixelFormat = DCL_PIXEL_FORMAT_RGB_888_PLANAR;
        return 0;
    }

    static int cvtColor(const dcl::Mat &src, dcl::Mat &dst,
                        colorSpace_t colorSpace)
    {
        if (dst.empty())
        {
            DCL_APP_LOG(DCL_ERROR, "dst is empty");
            return -1;
        }

        switch (colorSpace)
        {
        case IMAGE_COLOR_BGR888_TO_BGR888_PLANAR:
            return bgr888ToBgr888_Planar(src, dst);
        case IMAGE_COLOR_BGR888_TO_RGB888_PLANAR:
            return bgr888ToRgb888_Planar(src, dst);
        default:
            return -2;
        }
    }

    static unsigned int hash_to_rgb(const std::string &str)
    {
        // 使用标准库的 hash 函数对字符串进行哈希
        unsigned int hash = std::hash<std::string>{}(str);
        return hash;
    }

    static Color get_rgb_from_hash(const std::string &str)
    {
        unsigned int hash = hash_to_rgb(str);
        // 将哈希值分解为 RGB 颜色
        int r = (hash >> 16) & 0xFF; // 提取红色分量
        int g = (hash >> 8) & 0xFF;  // 提取绿色分量
        int b = hash & 0xFF;         // 提取蓝色分量
        return Color(b, g, r);
    }
} // namespace dcl
