/**
 * @file ClipTextV2_0.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2026-06-09
 * 
 * @brief 文本检索 ONNX 版本
 */
#pragma once

#include <unordered_map>
#include "TextFeature.hpp"
#include "ClipTextExt.hpp"

namespace ClipText_NS
{
    class CClipTextV2_0
    {
    public:
        CClipTextV2_0(InParam_S stInParam);
        ~CClipTextV2_0();

        /**
         * @brief 初始化
         * @return true
         * @return false
         */
        bool init();

        /**
         * @brief 反初始化
         * @return true
         * @return false
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param stInData 传入的文本数据
         * @param vResult 分析的结果
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<float> &vResult);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CTextFeature *m_pClipText = nullptr;
    };

} // namespace ClipText_NS
