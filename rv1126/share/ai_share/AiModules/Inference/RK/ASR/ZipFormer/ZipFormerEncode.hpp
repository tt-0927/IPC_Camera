/**
 * @file ZipFormerEncode.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 * 
 * @brief 
 */
#pragma once

#include "AVInferenceRK.hpp"
#include "iostream"

namespace Inference_NS
{

    class CZipFormerEncode : public CAVInferenceRK
    {
    public:
        CZipFormerEncode(std::string strConfigPath);
        ~CZipFormerEncode();

        /**
         * @brief 文字特征提取
         * @return [*]
         * @note
         */
        bool inference();

    private:
        /* 转换NCHW到NHWC */
        void convert_nchw_to_nhwc(float *src, float *dst, int N, int channels, int height, int width) ;

        int nTTT = 0;
    };

} // namespace Inference_NS
