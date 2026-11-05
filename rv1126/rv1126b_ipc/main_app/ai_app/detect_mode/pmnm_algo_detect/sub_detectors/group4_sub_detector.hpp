/**
 * @Author: xushou
 * @Date: 2026-10-28
 * @Description: Group4 行为检测子检测器声明（抽烟/睡岗/玩手机/摔倒）
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <opencv2/opencv.hpp>

#include "group4_detect.hpp"
#include "algorithm.hpp"
#include "algorithm_define.hpp"

/**
 * @brief  : Group4 行为检测子检测器
 * @note   : 依赖主检测器（人车非）输出的人员目标：
 *           - 抽烟/玩手机：裁剪人员框送 Group4 模型检测
 *           - 睡岗/摔倒：整帧送 Group4 模型检测
 *           配置自订阅（updateConfig 自行拉取），模型首次判定时自动加载，
 *           无线程，由算法线程串行驱动
 */
class CGroup4SubDetector
{
public:
    CGroup4SubDetector();
    ~CGroup4SubDetector();

    /* 从事件配置中心拉取配置保存 */
    void updateConfig();

    /**
     * @brief   : 处理一帧，检测抽烟/睡岗/玩手机/摔倒
     * @param    {cv::Mat} &rgbMat：当前帧 RGB
     * @param    {std::vector<TrackedTarget_S>} &vecPersonTargets：主检测器输出的跟踪人员目标
     * @return   {int} 0：成功 -1：模型未就绪 -2：输入非法
     */
    int process(const cv::Mat &rgbMat, const std::vector<TrackedTarget_S> &vecPersonTargets);

private:
    /* 加载 Group4 检测模型 */
    bool init();

private:
    /* Group4 检测组件实例 */
    std::unique_ptr<Group4Detect_NS::CGroup4Detect> m_pGroup4Handle;

    /* 配置快照 */
    std::mutex m_mutexConfig;
    struct Config_S
    {
        bool bSmokingEnable = false;
        bool bSleepEnable = false;
        bool bPhoneEnable = false;
        bool bTripEnable = false;

        unsigned int nSmokingSensitivity = 50;
        unsigned int nSleepSensitivity = 50;
        unsigned int nPhoneSensitivity = 50;
        unsigned int nTripSensitivity = 50;
    };
    Config_S m_stConfig;

    /* 报警状态机 */
    CAlarmStateMachine m_smokingStateMachine;
    CAlarmStateMachine m_sleepOnDutyStateMachine;
    CAlarmStateMachine m_phoneUsageStateMachine;
    CAlarmStateMachine m_tripStateMachine;
};
