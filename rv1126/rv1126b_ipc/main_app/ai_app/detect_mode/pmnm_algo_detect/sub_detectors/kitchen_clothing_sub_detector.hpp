/*
 * @Author: zhengxh
 * @Date: 2026-10-22
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-22
 * @Description: 后厨穿戴规范子检测器声明（依赖主检测器人员目标输出，裁剪人员小图二次推理判定穿戴）
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <opencv2/opencv.hpp>

#include "alarm_define.h"       /* Alarm::KitchenClothingCompliance_S */
#include "algorithm.hpp"        /* CAlarmStateMachine */
#include "algorithm_define.hpp" /* TrackedTarget_S */

class CKitchenDetector;

/**
 * @brief  : 后厨穿戴规范子检测器
 * @note   : 子检测器依赖主检测器（人车非）输出的人员目标：裁剪人员小图送厨房14类检测，
 *           缺厨师工作服/厨师帽/口罩任一属性判定违规并直接经报警状态机上报；
 *           配置自订阅（updateConfig 自行拉取），模型首次判定时自动加载，
 *           无线程，由算法线程串行驱动
 */
class CKitchenClothingSubDetector
{
public:
    CKitchenClothingSubDetector();
    ~CKitchenClothingSubDetector();

    /* 从事件配置中心拉取配置保存 */
    void updateConfig();

    /**
     * @brief   : 人员目标逐一判定穿戴，命中直接驱动事件上报
     * @param    {cv::Mat} &rgbMat：当前帧 RGB（用于裁剪人员小图）
     * @param    {std::vector<TrackedTarget_S>} &vecPersonTargets：主检测器输出的跟踪人员目标
     * @return   {int} 0：成功 -1：模型未就绪 -2：输入非法
     */
    int process(const cv::Mat &rgbMat, const std::vector<TrackedTarget_S> &vecPersonTargets);

private:
    /* 加载厨房检测模型（失败内部清理，返回 false） */
    bool init();

private:
    /* 厨房检测组件实例（algorithm_mode/kitchen_detect，首次判定时自动加载） */
    std::unique_ptr<CKitchenDetector> m_pKitchenComponent;

    /* 配置快照（updateConfig 来自配置线程，process 在算法线程，读写加锁） */
    std::mutex m_mutexRule;
    Alarm::KitchenClothingCompliance_S m_stConfig; /* 事件配置快照 */

    /* 报警状态机：冷却与重报节奏内置，开始/结束自动走 CEventLinkage */
    CAlarmStateMachine m_alarmStateMachine;
};
