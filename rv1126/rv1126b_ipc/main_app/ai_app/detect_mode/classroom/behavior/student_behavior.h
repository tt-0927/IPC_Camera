/**
 * @FilePath     : student_behavior.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 10:07:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 10:07:00
 * @Description  : 学生姿态行为采样器
 *                 把"单个人的姿态 + 几何特征"翻译成 11 个行为的三态判定结果。
 *                 采样器只回答"这一帧看起来像不像"，不做时间平滑、不产生事件；
 *                 时长累积与 begin/end 事件由规则引擎的 CDurationGate 负责。
 */

#pragma once

#include <vector>

#include "classroom_define.h"
#include "rule_engine.h"

namespace Classroom
{
/* 采样结果数组的下标列
 * 必须与 student_behavior.cpp 中的 astBehavior 查表严格同序（那里有 static_assert 兜底），
 * 因为规则引擎是按 behaviors() 的顺序把结果分发给行为列的，这里的顺序即对外 ABI 约定 */
typedef enum StudentColumn : int
{
    SC_STAND = 0,     // 站立
    SC_RAISE_HAND,    // 举手
    SC_HEAD_UP,       // 抬头
    SC_HEAD_DOWN,     // 低头
    SC_TURN_BACK,     // 回头
    SC_PHONE,         // 玩手机（低头 + 手部/手机形状）
    SC_LISTEN,        // 听讲（面向讲台 + 抬头 + 未回头）
    SC_READING,       // 阅读（低头形状，且手机模型未确认）
    SC_DISCUSS,       // 讨论（面向邻座）
    SC_PODIUM,        // 位于讲台区域
    SC_PRESENT,       // 上台演示（讲台 + 站立 + 面向班级）
    SC_EXPRESSION,     /* STU_EXPRESSION 表情识别 */
    SC_NUM
} StudentColumn_E;

/* 学生姿态采样器
 * 实现约束：samplePerson() 处于"每帧 x 每人"的热路径上，
 *           内部只做纯计算，不分配堆内存、不加锁、不累加统计。 */
class CStudentBehaviorSampler : public IBehaviorSampler
{
public:
    CStudentBehaviorSampler();
    ~CStudentBehaviorSampler() override = default;

    /* 采样器标识，出现在日志与"行为无人判定"告警中，用于定位来源 */
    const char *name() const override { return "stu_pose"; }

    /* 输出列顺序，长度恒为 SC_NUM；规则引擎据此把结果映射到行为列 */
    const std::vector<Behavior_E> &behaviors() const override { return m_vecBehavior; }

    /* 对一个人做一次全量采样
     * @param stIn  本帧上下文：帧、该人特征、配置快照、区域表、主体事实、同行他人身份
     * @param rvOut 输出缓冲，长度须为 SC_NUM，元素为 三态 + 原因码
     * @note rvOut 尺寸不符时直接返回并把全部结果留在 UNKNOWN/NO_FRAME，属于调用方错误 */

    void samplePerson(const CBehaviorSampleIn_S &stIn, std::vector<CBehaviorSample_S> &rvOut) override;

private:
    /* 只读的行为列映射表，构造时一次性填好后再不改动，可被多线程安全共享读 */
    std::vector<Behavior_E> m_vecBehavior;
};
}
