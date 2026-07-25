/**
 * @file NumberOcrV1_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-09
 * 
 * @brief 
 */
#pragma once

#include <unordered_map>
#include "NumberOcr.hpp"
#include "NumberOcrExt.hpp"

namespace NumberOcr_NS
{
    class CNumberOcrV1_0
    {
    public:
        CNumberOcrV1_0(InParam_S stInParam);
        ~CNumberOcrV1_0();

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
         * @param nResult 分析的参数
         * @return true 
         * @return false 
         */
        bool process(InData_S stInData, int &nResult);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CNumberOcr *m_pNumberOcr = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;
    };

} // namespace NumberOcr_NS
