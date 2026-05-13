/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 16:28:43
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-20 11:21:30
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group3Detect/V1_0/Group3DetectV1_0.hpp
 * @Description: smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水)
 */

#pragma once

#include <unordered_map>

#include "Group3DetectExt.hpp"
#include "YoloUltralytics.hpp"

#define Group3Detect 0

namespace Group3Detect_NS {
class CGroup3DetectV1_0 {
  public:
    /* 检测类别 */
    enum FireClass
    {
        /* 烟雾 */
        SMOKE,
        /* 火焰 */
        FIRE,
        /* 垃圾满溢 */
        GARBAGEOVER,
        /* 垃圾暴露 */
        GARBAGEEXPOSURE,
        /* 井盖完好 */
        COMPLETE,
        /* 井盖破损 */
        DAMAGED,
        /* 井盖丢失 */
        LOST,
        /* 未盖井盖 */
        UNCOVERED,
        /* 井盖外边沿破损 */
        BREAKOUTOFOUTEREDGE,
        /* 道路积水 */
        WATERACCUMULATION,
        /* 未知类型 */
        UNKOWN
    };

    CGroup3DetectV1_0(InParam_S stInParam);
    ~CGroup3DetectV1_0();

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
    bool process(InData_S stInData, std::vector<Result_S> &vecResult, OutData_S *stOutData = nullptr);

  private:
    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CYoloUltralytics *m_pYoloUltralytics = nullptr;

    int m_nSmokeFrameCount                = 0;  // 烟雾 连续检测帧数
    int m_nFireFrameCount                 = 0;  // 火焰 连续检测帧数
    int m_nGarbageExposureFrameCount      = 0;  // 垃圾暴露 连续检测帧数
    int m_nGarbageOverFrameCount          = 0;  // 垃圾满溢 连续检测帧数
    int m_nManholeCoverAbnormalFrameCount = 0;  // 井盖异常 连续检测帧数
    int m_nRoadPondingFrameCount          = 0;  // 道路积水 连续检测帧数
};

}  // namespace Group3Detect_NS