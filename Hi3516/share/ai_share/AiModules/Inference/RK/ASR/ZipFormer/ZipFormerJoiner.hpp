/**
 * @file ZipFormerJoiner.hpp
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

    class CZipFormerJoiner : public CAVInferenceRK
    {
    public:
        CZipFormerJoiner(std::string strConfigPath);
        ~CZipFormerJoiner();

        /**
         * @brief 文字特征提取
         * @return [*]
         * @note
         */
        bool inference();

    };

} // namespace Inference_NS
