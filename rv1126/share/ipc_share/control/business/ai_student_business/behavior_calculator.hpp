/***
 * @FilePath     : behavior_calculator.hpp
 * @Author       : zhengxh (zhengxh@kfb.cn)
 * @Date         : 2026-04-03 14:00:00
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 14:00:00
 * @Description  : 学生行为分析计算器，基于逐帧行为检测结果跟踪各行为类型的持续状态，
 *                 满足时长阈值时触发回调，并记录行为事件时间线与课堂表现统计。
 *                 所有公共接口均线程安全。
 */

#pragma once
#ifdef ENABLE_AI_STUDENT

#include "ai_student_define.h"
#include "StudentBehaviorExt.hpp"
#include <map>
#include <vector>
#include <functional>
#include <shared_mutex>

namespace AiStudentBusiness_NS {

/* 单个行为类型的持续状态 */
struct BehaviorState_S
{
    bool    bActive       = false; /* 当前是否处于持续状态 */
    int64_t nStartTime    = 0;     /* 行为开始时间戳（秒） */
    bool    bTriggered2s  = false; /* 是否已触发 2s 回调 */
    bool    bTriggered10s = false; /* 是否已触发 10s 回调 */
};

struct BehaviorCnt_S
{
    int nBendingHeadCnt = 0; /* 低头人数统计 */
    int nRaisingHeadCnt = 0; /* 抬头人数统计 */
    int nTurningHeadCnt = 0; /* 转头人数统计 */
    int nRaisingHandCnt = 0; /* 举手人数统计 */
    int nStandingCnt    = 0; /* 站立人数统计 */
    int nTurningBodyCnt = 0; /* 转身人数统计 */
    int nLyingOnDeskCnt = 0; /* 趴桌人数统计 */
};

class CBehaviorCalculator {
  public:
    using BehaviorCallback = std::function<void(const BehaviorRecord &)>;

    CBehaviorCalculator() = default;

    /**
     * @brief      : 设置班级应到总人数，用于行为占比计算。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : nTotal  应到总人数，必须大于 0
     */
    void setTotal(int nTotal);

    /**
     * @brief      : 设置当前课程开始时间戳，用于计算行为触发的经过时间。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : nStartTime  课程开始时间，秒级时间戳
     */
    void setCourseStartTime(int64_t nStartTime);

    /**
     * @brief      : 注册行为事件回调，达到持续时长阈值时触发。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : callback  回调函数，参数为触发的行为记录
     */
    void setCallback(BehaviorCallback callback);

    /**
     * @brief      : 处理一帧行为分析结果，更新内部行为持续状态并在满足条件时触发回调。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : vBehaviors  当前帧每个学生的行为检测结果
     * @param [in] : nCount     当前帧检测到的学生人数
     * @return     : true 表示成功，false 表示参数非法
     */
    bool handle(const std::vector<StudentBehavior_NS::Behavior_S> &vBehaviors, int nCount);

    /**
     * @brief       : 获取已记录的行为事件时间线。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : vRecords  行为事件列表
     * @return      : true 表示成功
     */
    bool getBehaviorTimeline(std::vector<BehaviorRecord> &vRecords) const;

    /**
     * @brief       : 获取学生课堂表现统计。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stOut  课堂表现结构体
     * @return      : true 表示成功
     */
    bool getPerformance(StudentPerformance &stOut) const;

    /**
     * @brief  : 重置所有状态，用于新课开始时清零。
     * @author : zhengxh (zhengxh@kfb.cn)
     */
    void reset();

  private:
    /**
     * @brief      : 判定当前帧的主导行为类型（待后续实现具体判断逻辑）。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : vBehaviors  当前帧行为检测结果
     * @param [in] : nCount     当前帧学生人数
     * @return     : 主导行为类型，无有效行为时返回 -1
     */
    int detectDominantBehavior(
        const std::vector<StudentBehavior_NS::Behavior_S> &vBehaviors,
        int                                                nCount) const;

    /**
     * @brief      : 计算课程开始到指定时刻的经过时间字符串。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : nNow  当前时间戳（秒）
     * @return     : 经过时间，格式 HH:MM:SS
     */
    std::string calcElapsedTime(int64_t nNow) const;

    /**
     * @brief      : 激活指定行为的持续状态。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : enType  行为类型
     * @param [in] : nNow   当前时间戳（秒）
     */
    void activateBehavior(PlatformBehaviorType_E enType, int64_t nNow);

    /**
     * @brief      : 检查指定行为是否达到持续时长阈值，满足则触发回调并记录时间线。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : enType  行为类型
     * @param [in] : nNow   当前时间戳（秒）
     */
    void checkBehaviorThreshold(PlatformBehaviorType_E enType, int64_t nNow);

    /**
     * @brief      : 结束指定行为的持续状态，记录结束事件并累计时长。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : enType  行为类型
     * @param [in] : nNow   当前时间戳（秒）
     */
    void endBehavior(PlatformBehaviorType_E enType, int64_t nNow);

    /**
     * @brief      : 是否是听讲状态
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stCnt  行为类型统计
     * @param [in] : nPresentCount  人员总数
     */
    bool isListenToTalk(const BehaviorCnt_S &stCnt, int nPresentCount) const;

    /**
     * @brief      : 是否是实践状态
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stCnt  行为类型统计
     * @param [in] : nPresentCount  人员总数
     */
    bool isPraxis(const BehaviorCnt_S &stCnt, int nPresentCount) const;

    /**
     * @brief      : 是否是演示状态
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stCnt  行为类型统计
     * @param [in] : nPresentCount  人员总数
     */
    bool isDemonstration(const BehaviorCnt_S &stCnt, int nPresentCount) const;

    /**
     * @brief      : 是否是阅读状态
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stCnt  行为类型统计
     * @param [in] : nPresentCount  人员总数
     */
    bool isRead(const BehaviorCnt_S &stCnt, int nPresentCount) const;

    /**
     * @brief      : 是否是讨论状态
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stCnt  行为类型统计
     * @param [in] : nPresentCount  人员总数
     */
    bool isDiscussion(const BehaviorCnt_S &stCnt, int nPresentCount) const;

  private:
    mutable std::shared_mutex m_mutex;

    /* 班级应到总人数 */
    int m_nTotal = 0;
    /* 课程开始时间戳（秒） */
    int64_t m_nCourseStartTime = 0;
    /* 行为事件回调 */
    BehaviorCallback m_callback;

    /* 当前主导行为类型，-1 表示无有效行为 */
    int m_nCurrentDominant = -1;

    /* 每种行为类型的持续状态 */
    std::map<PlatformBehaviorType_E, BehaviorState_S> m_mapBehaviorState;
    /* 已触发的行为事件时间线 */
    std::vector<BehaviorRecord> m_vTimeline;
    /* 各行为类型累计持续时长（秒），用于课堂表现计算 */
    std::map<PlatformBehaviorType_E, int64_t> m_mapDurationAccum;
};

}  // namespace AiStudentBusiness_NS

#endif