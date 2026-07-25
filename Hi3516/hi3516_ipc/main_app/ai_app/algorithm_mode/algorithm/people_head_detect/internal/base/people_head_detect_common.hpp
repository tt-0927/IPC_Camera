/**
 * @FilePath     : people_head_detect_common.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 09:13:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:21:53
 * @Description  : 人头检测通用工具
 */

#ifndef PEOPLE_HEAD_DETECT_COMMON_HPP
#define PEOPLE_HEAD_DETECT_COMMON_HPP

#include <string>

#include "people_head_detect_context.hpp"
#if CAP_AI_PEOPLE_DENSITY_LEGACY
#include "event_alarm/statistics/event_statistics_report.hpp"
#include "event_linkage_types.h"
#endif

namespace PeopleHeadDetectInternal
{
/**
 * @brief   : 转换人员聚集区域配置并根据有效区域更新使能
 * @param    {CrowdGathering_S} &stAlgoCfg：人员聚集配置
 * @param    {int} nWidth：算法分辨率宽度
 * @param    {int} nHeight：算法分辨率高度
 * @return   {void}
 */
void convert_crowd_config_and_enable(Alarm::CrowdGathering_S &stAlgoCfg, int nWidth, int nHeight);

#if CAP_AI_PEOPLE_DENSITY_LEGACY
/**
 * @brief   : 转换人员密度区域配置并根据有效区域更新使能
 * @param    {PeopleDensityDetection_S} &stAlgoCfg：人员密度配置
 * @param    {int} nWidth：算法分辨率宽度
 * @param    {int} nHeight：算法分辨率高度
 * @return   {void}
 */
void convert_density_config_and_enable(Alarm::PeopleDensityDetection_S &stAlgoCfg, int nWidth, int nHeight);

/**
 * @brief   : 将人员密度事件转换为等级文本
 * @param    {Event::Type_E} enEventType：人员密度等级事件类型
 * @return   {std::string} 等级文本，未命中等级时返回 none
 */
std::string get_density_level_text(Event::Type_E enEventType);

/**
 * @brief   : 构造人员密度联动上下文
 * @param    {SPeopleHeadProcessContext} &stContext：单帧处理上下文
 * @param    {Event::Type_E} enEventType：联动事件类型
 * @param    {uint32_t} nPeopleCount：当前区域人数
 * @param    {Event::Type_E} enAlarmEventType：当前命中的人员密度等级事件
 * @return   {EventTriggerContext_S} 联动上下文
 */
EventTriggerContext_S build_density_context(const SPeopleHeadProcessContext &stContext,
                                            Event::Type_E enEventType,
                                            uint32_t nPeopleCount,
                                            Event::Type_E enAlarmEventType);

/**
 * @brief   : 构造人员密度目标快照
 * @param    {BoxData_S} &stBoxData：人头目标框
 * @param    {long long} llNowMs：当前时间戳
 * @return   {EventStatistics_NS::TargetSnapshot_S} 目标快照
 */
EventStatistics_NS::TargetSnapshot_S build_density_snapshot(const Inference_NS::BoxData_S &stBoxData,
                                                            long long llNowMs);
#endif
} // namespace PeopleHeadDetectInternal

#endif
