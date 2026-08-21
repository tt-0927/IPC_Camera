/**
 * @FilePath     : isp_daynight_policy.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : 日夜与补光选择规则声明
 */

#pragma once

#include "isp_capability_profile.h"
#include "isp_runtime_decision.h"

/**
 * @brief 日夜与补光选择规则。
 */
namespace IspDayNightPolicy_NS
{

/**
 * @brief   : 选择智能补光灯型
 * @param    {ISP::IspCapabilityProfile_S} stProfile：平台功能和参数范围
 * @return   {ISP::IspSmartLightDecision_E} 补光选择结果
 */
ISP::IspSmartLightDecision_E decide_smart_light(const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验平台是否支持日夜场景
 * @param    {ISP::IspCapabilityProfile_S} stProfile：平台功能和参数范围
 * @param    {ISP::IspRuntimeScene_E} enScene：待校验日夜场景
 * @return   {int} OK：支持，ERR_UNSUPPORT：不支持
 */
int ensure_scene_supported(const ISP::IspCapabilityProfile_S &stProfile, ISP::IspRuntimeScene_E enScene);

/**
 * @brief   : 根据日夜结果、用户补光偏好和平台能力选择日夜场景与硬件设置
 * @param    {bool} bIsNight：是否夜间
 * @param    {ISP::DayNightAttr_S} stConfig：日夜配置
 * @param    {ISP::IspCapabilityProfile_S} stProfile：平台功能和参数范围
 * @param    {ISP::IspRuntimeDecision_S} stDecision：输出选择结果
 * @return   {int} OK：成功，ERR_UNSUPPORT：能力不支持，ERR_PARAM：参数非法
 */
int decide_runtime_scene(bool bIsNight,
                         const ISP::DayNightAttr_S &stConfig,
                         const ISP::IspCapabilityProfile_S &stProfile,
                         ISP::IspRuntimeDecision_S &stDecision);

/**
 * @brief   : 定时模式判定当前时刻是否处于夜晚区间
 * @param    {ISP::DayNightAttr_S} stConfig：日夜配置，包含开始和结束时间
 * @param    {int} nNowSecOfDay：当前时刻的当日秒数
 * @return   {bool} true：夜晚，false：白天
 */
bool is_night_by_time_range(const ISP::DayNightAttr_S &stConfig, int nNowSecOfDay);

/**
 * @brief   : 判断夜间补光设置是否变化
 * @param    {ISP::DayNightAttr_S} stOld：旧日夜配置
 * @param    {ISP::DayNightAttr_S} stNew：新日夜配置
 * @return   {bool} true：需要重新应用，false：无需重新应用
 */
bool has_night_light_runtime_changed(const ISP::DayNightAttr_S &stOld, const ISP::DayNightAttr_S &stNew);

} // namespace IspDayNightPolicy_NS
