/**
 * @file CameraObstructionV1_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-04
 * 
 * @brief 摄像头遮挡算法
 */
#pragma once

#include <unordered_map>

#include "CameraObstructionExt.hpp"
#include "CameraObstruction.hpp"

namespace CameraObstruction_NS
{
    class CCameraObstructionV1_0
    {
    public:

        CCameraObstructionV1_0(InParam_S stInParam);
        ~CCameraObstructionV1_0();

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

        Inference_NS::CComplexityDetect* m_pComplexityDetect = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 160;
        int m_nLimitWidth   = 120;
        int m_nLimitChannel = 3;

    };

}    // namespace CameraObstruction_NS
