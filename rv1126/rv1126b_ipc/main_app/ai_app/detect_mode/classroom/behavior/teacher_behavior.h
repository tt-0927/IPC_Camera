/**
 * @FilePath     : teacher_behavior.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 15:02:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-24 09:05:00
 * @Description  : 教师行为采样器
 */

#pragma once

#include <vector>

#include "classroom_define.h"
#include "rule_engine.h"

namespace Classroom
{
/**
 * @brief 采样器内部的列号，与 behaviors() 同序
 */
typedef enum TeacherColumn : int
{
    TC_STAND = 0,        /* TEA_STAND_UP      201 */
    TC_HEAD_UP,          /* TEA_HEAD_UP       202 */
    TC_HEAD_DOWN,        /* TEA_HEAD_DOWN     203 */
    TC_TURN_BACK,        /* TEA_TURN_BACK     204 */
    TC_TEACHING,         /* TEA_TEACHING      206 讲授 */
    TC_BOARD_WRITE,      /* TEA_BOARD_WRITE   208 板书 */
    TC_ENTER_PODIUM,     /* TEA_ENTER_PODIUM  211 进入讲台 */
    TC_TRACK,            /* TEA_TRACK         212 教师跟踪 */
    TC_PHONE,            /* TEA_PHONE         205 玩手机：与学生 106 同一形状，见 samplePerson */
    TC_NUM
} TeacherColumn_E;

/**
 * @brief   : 教师行为采样器
 */
class CTeacherBehaviorSampler : public IBehaviorSampler
{
public:
    CTeacherBehaviorSampler();
    ~CTeacherBehaviorSampler() override = default;

    const char *name() const override { return "tea_behavior"; }

    const std::vector<Behavior_E> &behaviors() const override { return m_vecBehavior; }

    void samplePerson(const CBehaviorSampleIn_S &stIn, std::vector<CBehaviorSample_S> &rvOut) override;

private:
    std::vector<Behavior_E> m_vecBehavior;
};
}
