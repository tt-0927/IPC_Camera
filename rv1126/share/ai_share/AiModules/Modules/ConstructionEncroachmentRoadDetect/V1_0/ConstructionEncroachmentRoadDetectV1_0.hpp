/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-17 10:07:33
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-18 08:56:27
 * @FilePath: /1126/share/ai_share/AiModules/Modules/ConstructionEncroachmentRoadDetect/V1_0/ConstructionEncroachmentRoadDetectV1_0.hpp
 * @Description: 施工占道检测
 */

#pragma once

#include <unordered_map>

#include "ConstructionEncroachmentRoadDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define ConstructionEncroachmentRoadDetect_DEBUG 1

namespace ConstructionEncroachmentRoadDetect_NS
{
    class ConstructionEncroachmentRoadDetectV1_0
    {
    public:

        ConstructionEncroachmentRoadDetectV1_0(InParam_S stInParam);
        ~ConstructionEncroachmentRoadDetectV1_0();

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
         * @param [std::vector<Result_S>&] vecResult: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(InData_S stInData, std::vector<Result_S>& vecResult, OutData_S* stOutData = nullptr);

    private:

        /* 初始化参数 */
        InParam_S m_stInParam;

        time_t m_nLastDetectTime;

        Inference_NS::CYoloUltralytics *m_pYoloUltralytics  = nullptr;

        int m_nConstructionEncroachmentRoadFrameCount = 0;   // 施工占道连续检测帧数

        /* 检测类别 */
        enum  TaggetType_E  
        {
            CONICAL_BARREL = 0,              // 锥形桶
            CRASH_BARREL = 1,                // 防撞桶
        };
    };

}    // namespace ConstructionEncroachmentRoadDetect_NS