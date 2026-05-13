/*
 * @FilePath     : VideoAnomalyDetectV1_0.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 17:01:31
 * @Description  :
 */
#pragma once

#include <unordered_map>

#include "VideoAnomalyDetectExt.hpp"
#include "ImageAnomaly.hpp"

namespace VideoAnomalyDetect_NS
{
    class CVideoAnomalyDetectV1_0
    {
    public:

        CVideoAnomalyDetectV1_0(InParam_S stInParam);
        ~CVideoAnomalyDetectV1_0();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param [cv::Mat] inMat: 传入的视频数据
         * @param [AnalyseParam_S] stParam: 分析的参数
         * @param [Result_S&] vecResult: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(InData_S stInData, Result_S& vecResult);



    private:

        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::cImageAnomaly* m_pImageAnomaly = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 512;
        int m_nLimitWidth   = 960;
        int m_nLimitChannel = 3;

    };

}    // namespace VideoAnomalyDetect_NS
