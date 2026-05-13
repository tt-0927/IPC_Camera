/*
 * @FilePath     : AudioAnomalyDetectV1_0.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 17:01:31
 * @Description  :
 */
#pragma once

#include <unordered_map>

#include "AudioAnomalyDetectExt.hpp"
#include "AudioAnomaly.hpp"

namespace AudioAnomalyDetect_NS
{
    class CAudioAnomalyDetectV1_0
    {
    public:

        CAudioAnomalyDetectV1_0(InParam_S stInParam);
        ~CAudioAnomalyDetectV1_0();

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

        Inference_NS::cAudioAnomaly* m_pAudioAnomaly = nullptr;

        /* 上一帧音频的分贝大小 */
        double dLastDB = -1e100;

    };

}    // namespace AudioAnomalyDetect_NS
