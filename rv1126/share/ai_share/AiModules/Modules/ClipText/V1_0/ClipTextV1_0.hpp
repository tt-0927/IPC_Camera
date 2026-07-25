/**
 * @file ClipTextV1_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 * 
 * @brief 图文检索
 */
#pragma once

#include <unordered_map>
#include "TextFeature.hpp"
#include "ClipTextExt.hpp"

namespace ClipText_NS
{
    class CClipTextV1_0
    {
    public:
        CClipTextV1_0(InParam_S stInParam);
        ~CClipTextV1_0();

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
         * @param stInData 传入的视频数据
         * @param vResult 分析的结果
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<float> &vResult);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CTextFeature *m_pClipText = nullptr;

        /* 算法输入参数限制 */
        int m_nSequenceLen = 0;
    };

} // namespace ClipText_NS
