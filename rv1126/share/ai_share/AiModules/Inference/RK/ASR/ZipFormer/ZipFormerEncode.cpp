/**
 * @file ZipFormerEncode.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 *
 * @brief
 */
#include "ZipFormerEncode.hpp"
#include <memory>
#include <cstring>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

Inference_NS::CZipFormerEncode::CZipFormerEncode(std::string strConfigPath)
    : CAVInferenceRK(strConfigPath)
{
}

Inference_NS::CZipFormerEncode::~CZipFormerEncode()
{
}

// 日志函数实现
void logToFile(const char *filename, const char *format, ...)
{
    FILE *fp = fopen(filename, "a"); // 追加模式打开文件
    if (!fp)
    {
        perror("Error opening log file");
        return;
    }

    // 添加时间戳
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    // 处理可变参数
    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args); // 格式化输出到文件
    va_end(args);

    fprintf(fp, "\n"); // 换行
    fclose(fp);
}

/* 推理数据 */
bool Inference_NS::CZipFormerEncode::inference()
{
    if (!m_pModel)
    {
        return false;
    }

    /* 运行 */
    if (!m_pModel->run(m_pInputs,
                       m_vInputAttrs.size(),
                       m_pOutputs,
                       m_vOutputAttrs.size()))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 转换NCHW到NHWC */
    for (int i = 1; i < m_vInputAttrs.size(); i++)
    {
        if (m_vInputAttrs[i].fmt == RKNN_TENSOR_NHWC)
        {
            int N = m_vInputAttrs[i].dims[0];
            int H = m_vInputAttrs[i].dims[1];
            int W = m_vInputAttrs[i].dims[2];
            int C = m_vInputAttrs[i].dims[3];
            convert_nchw_to_nhwc((float *)m_pOutputs[i].buf, (float *)m_pInputs[i].buf, N, C, H, W);
        }
        else
        {
            memcpy(m_pInputs[i].buf, m_pOutputs[i].buf, m_pInputs[i].size);
        }
        
    }

    return true;
}

/* 转换NCHW到NHWC */
void Inference_NS::CZipFormerEncode::convert_nchw_to_nhwc(float *src, float *dst, int N, int channels, int height, int width)    // 转换NCHW到NHWC
{
    for (int n = 0; n < N; ++n)
    {
        for (int c = 0; c < channels; ++c)
        {
            for (int h = 0; h < height; ++h)
            {
                for (int w = 0; w < width; ++w)
                {
                    dst[n * height * width * channels + h * width * channels + w * channels + c] = src[n * channels * height * width + c * height * width + h * width + w];
                }
            }
        }
    }
}
