/**
 * @FilePath     : isp_scene_schedule_policy.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:45:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-14 14:25:34
 * @Description  : 场景计划校验纯策略声明
 */

#pragma once

#include "isp_define.h"
#include "isp_capability_profile.h"

/**
 * @brief 场景计划校验纯策略集合。
 */
namespace IspSceneSchedulePolicy_NS
{

/**
 * @brief   : 校验场景计划的合法性
 * @param    {ISP::SceneSchedule_S&} stConfig：场景计划配置入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
 * @return   {int} OK：合法，ERR_PARAM：时间段或月份非法，ERR_UNSUPPORT：计划或场景不支持
 * @note    : 只计算和校验，不读配置、不启动线程。禁用计划允许空月份集合。
 */
int normalize_scene_schedule(ISP::SceneSchedule_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

} // namespace IspSceneSchedulePolicy_NS
