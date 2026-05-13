/**
 * @file SydneyCartonV1_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 * 
 * @brief 形状检测（只支持多边形检测）
 */
#pragma once

#include <unordered_map>
#include "ShapeDetect.hpp"
#include "SydneyCartonExt.hpp"


namespace SydneyCarton_NS
{
    class CSydneyCartonV1_0
    {
    public:
        CSydneyCartonV1_0(InParam_S stInParam);
        ~CSydneyCartonV1_0();

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

        Inference_NS::CShapeDetect *m_pSydneyCarton = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 256;
        int m_nLimitWidth = 480;
        int m_nLimitChannel = 3;
    };

} // namespace SydneyCarton_NS

