/**
 * @file OutDataProcess.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-11-11
 *
 * @brief
 */

#pragma once

#if defined(__AVX2__)
#include <immintrin.h>
#define USE_AVX2 1
#elif defined(__SSE4_1__)
#include <smmintrin.h>
#define USE_SSE4 1
#elif defined(__SSE3__)
#include <pmmintrin.h>
#define USE_SSE3 1
#elif defined(__SSE2__)
#include <emmintrin.h>
#define USE_SSE2 1
#else
#define NO_SIMD 1
#endif

// 检查编译器支持的SIMD指令集
#ifdef __AVX2__
#define USE_AVX2
#elif defined(__SSE4_1__)
#define USE_SSE4
#elif defined(__SSE3__)
#define USE_SSE3
#endif

namespace Inference_NS
{
    // 针对 int8_t 的 SIMD 反量化
    void dequantize_s8_to_f32_simd(const int8_t *src, float *dst, int n,
                                   float inv_scale, int8_t zero_point)
    {
        int i = 0;

#ifdef USE_AVX2
        // AVX2 版本 - 一次处理 16 个元素
        if (zero_point == 0)
        {
            const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
            for (; i <= n - 16; i += 16)
            {
                // 加载 16 个 int8_t
                __m128i int8_vec = _mm_loadu_si128((__m128i *)(src + i));

                // 拆分为两个 8 字节向量并扩展到 32 位
                __m256i int32_vec0 = _mm256_cvtepi8_epi32(int8_vec);
                __m256i int32_vec1 = _mm256_cvtepi8_epi32(_mm_srli_si128(int8_vec, 8));

                // 转换为浮点数
                __m256 float_vec0 = _mm256_cvtepi32_ps(int32_vec0);
                __m256 float_vec1 = _mm256_cvtepi32_ps(int32_vec1);

                // 应用反量化公式: (x + zero_point) * inv_scale
                float_vec0 = _mm256_mul_ps(float_vec0, inv_scale_vec);
                float_vec1 = _mm256_mul_ps(float_vec1, inv_scale_vec);

                // 存储结果
                _mm256_storeu_ps(dst + i, float_vec0);
                _mm256_storeu_ps(dst + i + 8, float_vec1);
            }
        }
        else
        {
            const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
            const __m256 zero_point_vec = _mm256_set1_ps(static_cast<float>(zero_point));

            for (; i <= n - 16; i += 16)
            {
                // 加载 16 个 int8_t
                __m128i int8_vec = _mm_loadu_si128((__m128i *)(src + i));

                // 拆分为两个 8 字节向量并扩展到 32 位
                __m256i int32_vec0 = _mm256_cvtepi8_epi32(int8_vec);
                __m256i int32_vec1 = _mm256_cvtepi8_epi32(_mm_srli_si128(int8_vec, 8));

                // 转换为浮点数
                __m256 float_vec0 = _mm256_cvtepi32_ps(int32_vec0);
                __m256 float_vec1 = _mm256_cvtepi32_ps(int32_vec1);

                // 应用反量化公式: (x + zero_point) * inv_scale
                float_vec0 = _mm256_add_ps(float_vec0, zero_point_vec);
                float_vec0 = _mm256_mul_ps(float_vec0, inv_scale_vec);

                float_vec1 = _mm256_add_ps(float_vec1, zero_point_vec);
                float_vec1 = _mm256_mul_ps(float_vec1, inv_scale_vec);

                // 存储结果
                _mm256_storeu_ps(dst + i, float_vec0);
                _mm256_storeu_ps(dst + i + 8, float_vec1);
            }
        }
#elif defined(USE_SSE4)
        if (zero_point == 0)
        {
            // SSE4 版本 - 一次处理 8 个元素
            const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);

            for (; i <= n - 8; i += 8)
            {
                // 加载 8 个 int8_t
                __m128i int8_vec = _mm_loadl_epi64((__m128i *)(src + i));

                // 扩展到 32 位整数
                __m128i int32_vec0 = _mm_cvtepi8_epi32(int8_vec);
                __m128i int32_vec1 = _mm_cvtepi8_epi32(_mm_srli_si128(int8_vec, 4));

                // 转换为浮点数
                __m128 float_vec0 = _mm_cvtepi32_ps(int32_vec0);
                __m128 float_vec1 = _mm_cvtepi32_ps(int32_vec1);

                // 应用反量化公式
                float_vec0 = _mm_mul_ps(float_vec0, inv_scale_vec);
                float_vec1 = _mm_mul_ps(float_vec1, inv_scale_vec);

                // 存储结果
                _mm_storeu_ps(dst + i, float_vec0);
                _mm_storeu_ps(dst + i + 4, float_vec1);
            }
        }
        else
        {
            // SSE4 版本 - 一次处理 8 个元素
            const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);
            const __m128 zero_point_vec = _mm_set1_ps(static_cast<float>(zero_point));

            for (; i <= n - 8; i += 8)
            {
                // 加载 8 个 int8_t
                __m128i int8_vec = _mm_loadl_epi64((__m128i *)(src + i));

                // 扩展到 32 位整数
                __m128i int32_vec0 = _mm_cvtepi8_epi32(int8_vec);
                __m128i int32_vec1 = _mm_cvtepi8_epi32(_mm_srli_si128(int8_vec, 4));

                // 转换为浮点数
                __m128 float_vec0 = _mm_cvtepi32_ps(int32_vec0);
                __m128 float_vec1 = _mm_cvtepi32_ps(int32_vec1);

                // 应用反量化公式
                float_vec0 = _mm_add_ps(float_vec0, zero_point_vec);
                float_vec0 = _mm_mul_ps(float_vec0, inv_scale_vec);

                float_vec1 = _mm_add_ps(float_vec1, zero_point_vec);
                float_vec1 = _mm_mul_ps(float_vec1, inv_scale_vec);

                // 存储结果
                _mm_storeu_ps(dst + i, float_vec0);
                _mm_storeu_ps(dst + i + 4, float_vec1);
            }
        }
#endif

        // 处理剩余元素（回退到标量处理）
        if (zero_point == 0)
        {
            for (; i < n; ++i)
            {
                dst[i] = static_cast<float>(src[i]) * inv_scale;
            }
        }
        else
        {
            for (; i < n; ++i)
            {
                dst[i] = (static_cast<float>(src[i]) + zero_point) * inv_scale;
            }
        }
    }

    // 针对 uint8_t 的 SIMD 反量化
    void dequantize_u8_to_f32_simd(const uint8_t *src, float *dst, int n,
                                   float inv_scale, uint8_t zero_point)
    {
        int i = 0;

#ifdef USE_AVX2
        if (zero_point == 0)
        {
            // AVX2 版本
            const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
            for (; i <= n - 16; i += 16)
            {
                __m128i uint8_vec = _mm_loadu_si128((__m128i *)(src + i));

                __m256i uint32_vec0 = _mm256_cvtepu8_epi32(uint8_vec);
                __m256i uint32_vec1 = _mm256_cvtepu8_epi32(_mm_srli_si128(uint8_vec, 8));

                __m256 float_vec0 = _mm256_cvtepi32_ps(uint32_vec0);
                __m256 float_vec1 = _mm256_cvtepi32_ps(uint32_vec1);

                float_vec0 = _mm256_mul_ps(float_vec0, inv_scale_vec);
                float_vec1 = _mm256_mul_ps(float_vec1, inv_scale_vec);

                _mm256_storeu_ps(dst + i, float_vec0);
                _mm256_storeu_ps(dst + i + 8, float_vec1);
            }
        }
        else
        {
            // AVX2 版本
            const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
            const __m256 zero_point_vec = _mm256_set1_ps(static_cast<float>(zero_point));

            for (; i <= n - 16; i += 16)
            {
                __m128i uint8_vec = _mm_loadu_si128((__m128i *)(src + i));

                __m256i uint32_vec0 = _mm256_cvtepu8_epi32(uint8_vec);
                __m256i uint32_vec1 = _mm256_cvtepu8_epi32(_mm_srli_si128(uint8_vec, 8));

                __m256 float_vec0 = _mm256_cvtepi32_ps(uint32_vec0);
                __m256 float_vec1 = _mm256_cvtepi32_ps(uint32_vec1);

                float_vec0 = _mm256_add_ps(float_vec0, zero_point_vec);
                float_vec0 = _mm256_mul_ps(float_vec0, inv_scale_vec);

                float_vec1 = _mm256_add_ps(float_vec1, zero_point_vec);
                float_vec1 = _mm256_mul_ps(float_vec1, inv_scale_vec);

                _mm256_storeu_ps(dst + i, float_vec0);
                _mm256_storeu_ps(dst + i + 8, float_vec1);
            }
        }
#elif defined(USE_SSE4)
        if (zero_point == 0)
        {
            // SSE4 版本
            const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);

            for (; i <= n - 8; i += 8)
            {
                __m128i uint8_vec = _mm_loadl_epi64((__m128i *)(src + i));

                __m128i uint32_vec0 = _mm_cvtepu8_epi32(uint8_vec);
                __m128i uint32_vec1 = _mm_cvtepu8_epi32(_mm_srli_si128(uint8_vec, 4));

                __m128 float_vec0 = _mm_cvtepi32_ps(uint32_vec0);
                __m128 float_vec1 = _mm_cvtepi32_ps(uint32_vec1);

                float_vec0 = _mm_mul_ps(float_vec0, inv_scale_vec);
                float_vec1 = _mm_mul_ps(float_vec1, inv_scale_vec);

                _mm_storeu_ps(dst + i, float_vec0);
                _mm_storeu_ps(dst + i + 4, float_vec1);
            }
        }
        else
        {
            // SSE4 版本
            const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);
            const __m128 zero_point_vec = _mm_set1_ps(static_cast<float>(zero_point));

            for (; i <= n - 8; i += 8)
            {
                __m128i uint8_vec = _mm_loadl_epi64((__m128i *)(src + i));

                __m128i uint32_vec0 = _mm_cvtepu8_epi32(uint8_vec);
                __m128i uint32_vec1 = _mm_cvtepu8_epi32(_mm_srli_si128(uint8_vec, 4));

                __m128 float_vec0 = _mm_cvtepi32_ps(uint32_vec0);
                __m128 float_vec1 = _mm_cvtepi32_ps(uint32_vec1);

                float_vec0 = _mm_add_ps(float_vec0, zero_point_vec);
                float_vec0 = _mm_mul_ps(float_vec0, inv_scale_vec);

                float_vec1 = _mm_add_ps(float_vec1, zero_point_vec);
                float_vec1 = _mm_mul_ps(float_vec1, inv_scale_vec);

                _mm_storeu_ps(dst + i, float_vec0);
                _mm_storeu_ps(dst + i + 4, float_vec1);
            }
        }
#endif
        // 处理剩余元素
        if (zero_point == 0)
        {
            for (; i < n; ++i)
            {
                dst[i] = static_cast<float>(src[i]) * inv_scale;
            }
        }
        else
        {
            for (; i < n; ++i)
            {
                dst[i] = (static_cast<float>(src[i]) + zero_point) * inv_scale;
            }
        }
    }

    // 针对 int16_t 的 SIMD 反量化
    inline void dequantize_s16_to_f32_simd(const int16_t *src, float *dst, int n,
                                           float inv_scale, int16_t zero_point = 0)
    {
        int i = 0;

#ifdef USE_AVX2
        const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
        const __m256i zero_point_vec = _mm256_set1_epi32(zero_point);

        for (; i <= n - 16; i += 16)
        {
            __m256i lo = _mm256_loadu_si256((__m256i const *)(src + i));       // 16×i16
            __m256i hi = _mm256_srli_si256(lo, 8);                             // 高 8 个
            __m256i i32_0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(lo)); // 低 8→i32
            __m256i i32_1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(hi)); // 高 8→i32

            if (zero_point != 0)
            {
                i32_0 = _mm256_sub_epi32(i32_0, zero_point_vec);
                i32_1 = _mm256_sub_epi32(i32_1, zero_point_vec);
            }

            __m256 f0 = _mm256_cvtepi32_ps(i32_0);
            __m256 f1 = _mm256_cvtepi32_ps(i32_1);
            f0 = _mm256_mul_ps(f0, inv_scale_vec);
            f1 = _mm256_mul_ps(f1, inv_scale_vec);

            _mm256_storeu_ps(dst + i, f0);
            _mm256_storeu_ps(dst + i + 8, f1);
        }
#elif defined(USE_SSE4)
        const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);
        const __m128i zero_point_vec = _mm_set1_epi32(zero_point);

        for (; i <= n - 8; i += 8)
        {
            __m128i i16 = _mm_loadu_si128((__m128i const *)(src + i));  // 8×i16
            __m128i i32_0 = _mm_cvtepi16_epi32(i16);                    // 低 4
            __m128i i32_1 = _mm_cvtepi16_epi32(_mm_srli_si128(i16, 8)); // 高 4

            if (zero_point != 0)
            {
                i32_0 = _mm_sub_epi32(i32_0, zero_point_vec);
                i32_1 = _mm_sub_epi32(i32_1, zero_point_vec);
            }

            __m128 f0 = _mm_cvtepi32_ps(i32_0);
            __m128 f1 = _mm_cvtepi32_ps(i32_1);
            f0 = _mm_mul_ps(f0, inv_scale_vec);
            f1 = _mm_mul_ps(f1, inv_scale_vec);

            _mm_storeu_ps(dst + i, f0);
            _mm_storeu_ps(dst + i + 4, f1);
        }
#endif
        // 标量尾处理
        if (zero_point == 0)
        {
            for (; i < n; ++i)
                dst[i] = static_cast<float>(src[i]) * inv_scale;
        }
        else
        {
            for (; i < n; ++i)
                dst[i] = (static_cast<float>(src[i]) + zero_point) * inv_scale;
        }
    }

    // 针对 uint16_t 的 SIMD 反量化
    inline void dequantize_u16_to_f32_simd(const uint16_t *src, float *dst, int n,
                                           float inv_scale, uint16_t zero_point = 0)
    {
        int i = 0;

#ifdef USE_AVX2
        const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
        const __m256 zero_point_vec = _mm256_set1_ps(static_cast<float>(zero_point));

        for (; i <= n - 16; i += 16)
        {
            // 加载 16 个 uint16_t
            __m256i u16_vec = _mm256_loadu_si256((__m256i const *)(src + i));

            // 拆分为两个 128 位向量
            __m128i u16_vec0 = _mm256_castsi256_si128(u16_vec);
            __m128i u16_vec1 = _mm256_extracti128_si256(u16_vec, 1);

            // 将 uint16_t 零扩展到 uint32_t
            __m256i u32_vec0 = _mm256_cvtepu16_epi32(u16_vec0);
            __m256i u32_vec1 = _mm256_cvtepu16_epi32(u16_vec1);

            // 转换为浮点数
            __m256 float_vec0 = _mm256_cvtepi32_ps(u32_vec0);
            __m256 float_vec1 = _mm256_cvtepi32_ps(u32_vec1);

            if (zero_point != 0)
            {
                // 应用反量化公式: (x - zero_point) * inv_scale
                float_vec0 = _mm256_sub_ps(float_vec0, zero_point_vec);
                float_vec1 = _mm256_sub_ps(float_vec1, zero_point_vec);
            }

            float_vec0 = _mm256_mul_ps(float_vec0, inv_scale_vec);
            float_vec1 = _mm256_mul_ps(float_vec1, inv_scale_vec);

            // 存储结果
            _mm256_storeu_ps(dst + i, float_vec0);
            _mm256_storeu_ps(dst + i + 8, float_vec1);
        }
#elif defined(USE_SSE4)
        const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);
        const __m128 zero_point_vec = _mm_set1_ps(static_cast<float>(zero_point));

        for (; i <= n - 8; i += 8)
        {
            // 加载 8 个 uint16_t
            __m128i u16_vec = _mm_loadu_si128((__m128i const *)(src + i));

            // 将 uint16_t 零扩展到 uint32_t
            __m128i u32_vec0 = _mm_cvtepu16_epi32(u16_vec);
            __m128i u32_vec1 = _mm_cvtepu16_epi32(_mm_srli_si128(u16_vec, 8));

            // 转换为浮点数
            __m128 float_vec0 = _mm_cvtepi32_ps(u32_vec0);
            __m128 float_vec1 = _mm_cvtepi32_ps(u32_vec1);

            if (zero_point != 0)
            {
                // 应用反量化公式: (x - zero_point) * inv_scale
                float_vec0 = _mm_sub_ps(float_vec0, zero_point_vec);
                float_vec1 = _mm_sub_ps(float_vec1, zero_point_vec);
            }

            float_vec0 = _mm_mul_ps(float_vec0, inv_scale_vec);
            float_vec1 = _mm_mul_ps(float_vec1, inv_scale_vec);

            // 存储结果
            _mm_storeu_ps(dst + i, float_vec0);
            _mm_storeu_ps(dst + i + 4, float_vec1);
        }
#endif

        // 标量尾处理
        if (zero_point == 0)
        {
            for (; i < n; ++i)
                dst[i] = static_cast<float>(src[i]) * inv_scale;
        }
        else
        {
            for (; i < n; ++i)
                dst[i] = (static_cast<float>(src[i]) - zero_point) * inv_scale;
        }
    }

    // 针对 float16 的 SIMD 转换到 float32
    // 针对 float16 的 SIMD 反量化转换到 float32
    inline void dequantize_f16_to_f32_simd(const uint16_t *src, float *dst, int n,
                                           float inv_scale, uint16_t zero_point = 0)
    {
        int i = 0;

        // 将 zero_point 从 uint16_t (float16 格式) 转换为 float32
        float zero_point_f32 = 0.0f;
        if (zero_point != 0)
        {
            // 简单的 float16 到 float32 标量转换
            uint16_t h = zero_point;
            uint32_t sign = (h & 0x8000) << 16;
            uint32_t exponent = (h & 0x7C00) >> 10;
            uint32_t mantissa = (h & 0x03FF);

            if (exponent == 0)
            {
                if (mantissa == 0)
                {
                    zero_point_f32 = *reinterpret_cast<float *>(&sign);
                }
                else
                {
                    // 非规约数处理
                    uint32_t f32_mantissa = mantissa << 13;
                    uint32_t f32_exponent = 112;
                    while ((f32_mantissa & 0x7F800000) == 0)
                    {
                        f32_mantissa <<= 1;
                        f32_exponent--;
                    }
                    f32_mantissa &= 0x007FFFFF;
                    uint32_t f32 = sign | (f32_exponent << 23) | f32_mantissa;
                    zero_point_f32 = *reinterpret_cast<float *>(&f32);
                }
            }
            else if (exponent == 0x1F)
            {
                uint32_t f32 = sign | 0x7F800000 | (mantissa << 13);
                zero_point_f32 = *reinterpret_cast<float *>(&f32);
            }
            else
            {
                uint32_t f32 = sign | ((exponent + 112) << 23) | (mantissa << 13);
                zero_point_f32 = *reinterpret_cast<float *>(&f32);
            }
        }

#if defined(USE_AVX2) && defined(__F16C__)
        // AVX2 + F16C 版本 - 一次处理 16 个元素
        const __m256 inv_scale_vec = _mm256_set1_ps(inv_scale);
        const __m256 zero_point_vec = _mm256_set1_ps(zero_point_f32);

        for (; i <= n - 16; i += 16)
        {
            // 加载 16 个 float16
            __m256i f16_vec = _mm256_loadu_si256((__m256i const *)(src + i));

            // 使用 F16C 指令将 float16 转换为 float32
            __m256 f32_vec = _mm256_cvtph_ps(f16_vec);

            // 应用反量化公式: (x - zero_point) * inv_scale
            if (zero_point != 0)
            {
                f32_vec = _mm256_sub_ps(f32_vec, zero_point_vec);
            }
            f32_vec = _mm256_mul_ps(f32_vec, inv_scale_vec);

            // 存储结果
            _mm256_storeu_ps(dst + i, f32_vec);
        }
#elif defined(USE_SSE4) && defined(__F16C__)
        // SSE4 + F16C 版本 - 一次处理 8 个元素
        const __m128 inv_scale_vec = _mm_set1_ps(inv_scale);
        const __m128 zero_point_vec = _mm_set1_ps(zero_point_f32);

        for (; i <= n - 8; i += 8)
        {
            // 加载 8 个 float16
            __m128i f16_vec = _mm_loadu_si128((__m128i const *)(src + i));

            // 使用 F16C 指令将 float16 转换为 float32
            __m128 f32_vec = _mm_cvtph_ps(f16_vec);

            // 应用反量化公式: (x - zero_point) * inv_scale
            if (zero_point != 0)
            {
                f32_vec = _mm_sub_ps(f32_vec, zero_point_vec);
            }
            f32_vec = _mm_mul_ps(f32_vec, inv_scale_vec);

            // 存储结果
            _mm_storeu_ps(dst + i, f32_vec);
        }
#endif

        // 标量回退处理
        for (; i < n; ++i)
        {
            // float16 到 float32 转换
            uint16_t h = src[i];
            float temp = 0.0f;

            // 提取符号位、指数位和尾数位
            uint32_t sign = (h & 0x8000) << 16;
            uint32_t exponent = (h & 0x7C00) >> 10;
            uint32_t mantissa = (h & 0x03FF);

            if (exponent == 0)
            {
                if (mantissa == 0)
                {
                    temp = *reinterpret_cast<float *>(&sign);
                }
                else
                {
                    // 非规约数
                    uint32_t f32_mantissa = mantissa << 13;
                    uint32_t f32_exponent = 112;
                    while ((f32_mantissa & 0x7F800000) == 0)
                    {
                        f32_mantissa <<= 1;
                        f32_exponent--;
                    }
                    f32_mantissa &= 0x007FFFFF;
                    uint32_t f32 = sign | (f32_exponent << 23) | f32_mantissa;
                    temp = *reinterpret_cast<float *>(&f32);
                }
            }
            else if (exponent == 0x1F)
            {
                uint32_t f32 = sign | 0x7F800000 | (mantissa << 13);
                temp = *reinterpret_cast<float *>(&f32);
            }
            else
            {
                uint32_t f32 = sign | ((exponent + 112) << 23) | (mantissa << 13);
                temp = *reinterpret_cast<float *>(&f32);
            }

            // 应用反量化
            if (zero_point != 0)
            {
                dst[i] = (temp - zero_point_f32) * inv_scale;
            }
            else
            {
                dst[i] = temp * inv_scale;
            }
        }
    }
}