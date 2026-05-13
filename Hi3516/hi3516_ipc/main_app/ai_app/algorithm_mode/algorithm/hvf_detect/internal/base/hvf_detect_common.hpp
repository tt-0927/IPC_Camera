/**
 * @FilePath     : hvf_detect_common.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:42:48
 * @Description  : HVF 公共基础能力声明
 */

#pragma once

#include <set>
#include <string>
#include <vector>

#include "algorithm.hpp"
#include "common_process.h"
#include "target_index_manager.hpp"
#include "hvf_detect_context.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : 判断当前算法类别是否命中业务检测目标配置
 * @param    {const std::vector<int> &} aDetectTarget：检测目标枚举数组
 * @param    {const ot_aidetect_class &} enAiDetectClass：AI 输出目标类别
 * @return   {bool} true：命中 false：未命中
 */
bool is_target_match(const std::vector<int> &aDetectTarget, const ot_aidetect_class &enAiDetectClass);

/**
 * @brief   : 查找指定类别的算法输出
 * @param    {const ot_aidetect_result_array &} stResult：当前帧算法结果
 * @param    {ot_aidetect_class} enClassType：目标类别
 * @return   {const ot_aidetect_object_of_one_class *} 类别结果指针，未找到返回空
 */
const ot_aidetect_object_of_one_class *find_object_class(const ot_aidetect_result_array &stResult, ot_aidetect_class enClassType);

/**
 * @brief   : 收集单个类别结果中的全部 track id
 * @param    {const ot_aidetect_object_of_one_class *} pstObjectClass：类别结果指针
 * @return   {std::set<int>} track id 集合
 */
std::set<int> collect_track_ids(const ot_aidetect_object_of_one_class *pstObjectClass);

/**
 * @brief   : 收集整帧算法结果中的全部 track id
 * @param    {const ot_aidetect_result_array &} stResult：当前帧算法结果
 * @return   {std::set<int>} track id 集合
 */
std::set<int> collect_all_track_ids(const ot_aidetect_result_array &stResult);

/**
 * @brief   : 转换区域坐标并判断是否仍可使能事件
 * @param    {T &} stConfig：带规则数组的事件配置
 * @param    {int} nWidth：算法分辨率宽度
 * @param    {int} nHeight：算法分辨率高度
 * @return   {void}
 */
template <typename T>
void convert_resolution_and_enable(T &stConfig, int nWidth, int nHeight);

/**
 * @brief   : 转换越界警戒线坐标并判断是否仍可使能事件
 * @param    {Alarm::BoundaryDetection_S &} stConfig：越界侦测配置
 * @param    {int} nWidth：算法分辨率宽度
 * @param    {int} nHeight：算法分辨率高度
 * @return   {void}
 */
void convert_boundary_and_enable(Alarm::BoundaryDetection_S &stConfig, int nWidth, int nHeight);

/**
 * @brief   : 重置区域状态二维数组
 * @param    {AreaStatus_S (&)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]} stStatusArray：状态数组
 * @return   {void}
 */
template <size_t MaxRegions>
void reset_area_status_array(AreaStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]);

/**
 * @brief   : 重置越界状态二维数组
 * @param    {BoundaryTrackStatus_S (&)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]} stStatusArray：越界状态数组
 * @return   {void}
 */
template <size_t MaxRegions>
void reset_boundary_status_array(BoundaryTrackStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]);

#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 重置徘徊面板累计时长起点二维数组
 * @param    {double (&)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]} dEnterTimeArray：面板累计起点数组
 * @return   {void}
 */
template <size_t MaxRegions>
void reset_panel_enter_time_array(double (&dEnterTimeArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]);

/**
 * @brief   : 将 AI 目标类别转换为 HVF 面板展示文本
 * @param    {ot_aidetect_class} enClassType：算法目标类别
 * @return   {std::string} 展示文本
 */
std::string get_hvf_target_text(ot_aidetect_class enClassType);

/**
 * @brief   : 将徘徊时长转换为 HVF 面板展示文本
 * @param    {uint32_t} nDurationSec：徘徊时长，单位秒
 * @return   {std::string} 展示文本
 */
std::string get_hvf_loiter_duration_text(uint32_t nDurationSec);

/**
 * @brief   : 构造 HVF 区域类事件面板字段
 * @param    {Event::Type_E} enEventType：事件类型
 * @param    {const ot_aidetect_object_of_one_class *} pstObjectClass：目标类别结果
 * @param    {const ot_aidetect_object &} stObject：当前目标
 * @param    {bool} bAlarm：当前条目是否报警
 * @param    {uint32_t} nDurationSec：当前目标停留时长，单位秒
 * @return   {std::vector<OsdPanel::PanelField_S>} 字段数组
 */
std::vector<OsdPanel::PanelField_S> build_hvf_region_panel_fields(Event::Type_E enEventType,
                                                                  const ot_aidetect_object_of_one_class *pstObjectClass,
                                                                  const ot_aidetect_object &stObject,
                                                                  bool bAlarm,
                                                                  uint32_t nDurationSec);

/**
 * @brief   : 构造 HVF 区域类事件面板条目
 * @param    {int} nRegionIndex：区域下标
 * @param    {const ot_aidetect_object_of_one_class *} pstObjectClass：目标类别结果
 * @param    {const ot_aidetect_object &} stObject：当前目标
 * @param    {bool} bAlarm：当前条目是否报警
 * @param    {Event::Type_E} enEventType：事件类型
 * @param    {uint32_t} nDurationSec：当前目标停留时长，单位秒
 * @return   {OsdPanel::PanelItem_S} 面板条目
 */
OsdPanel::PanelItem_S build_hvf_region_panel_item(int nRegionIndex,
                                                  const ot_aidetect_object_of_one_class *pstObjectClass,
                                                  const ot_aidetect_object &stObject,
                                                  bool bAlarm,
                                                  Event::Type_E enEventType,
                                                  uint32_t nDurationSec);
#endif

/**
 * @brief   : 通用区域类检测处理
 * @param    {const ot_aidetect_object_of_one_class *} pstObjectClass：当前类别算法结果
 * @param    {const std::vector<RuleType> &} aRules：规则数组
 * @param    {AreaStatus_S (&)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]} stStatusArray：区域状态数组
 * @param    {CTargetIndexManager20 &} indexManager：目标索引管理器
 * @param    {CAlarmStateMachine &} alarmStateMachine：报警状态机
 * @param    {Event::Type_E} enEventType：事件类型
 * @param    {const char *} pszDetectTypeName：检测类型日志名称
 * @param    {std::vector<Common::RectInfo_S> &} vstRectInfo：角框输出数组
 * @param    {OsdPanel::PanelFrame_S *} pstPanelFrame：展会面板结果输出指针
 * @param    {double (*)[SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]} pdPanelEnterTime：徘徊面板累计时长起点数组
 * @return   {bool} true：本帧存在报警 false：本帧无报警
 * @note    : 展会版本下可选输出左上角汇总面板
 */
template <typename RuleType, size_t MaxRegions>
bool process_region_detection(const ot_aidetect_object_of_one_class *pstObjectClass,
                              const std::vector<RuleType> &aRules,
                              AreaStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM],
                              CTargetIndexManager20 &indexManager,
                              CAlarmStateMachine &alarmStateMachine,
                              Event::Type_E enEventType,
                              const char *pszDetectTypeName,
                              std::vector<Common::RectInfo_S> &vstRectInfo
#if CAP_EXHIBITION_OSD_PANEL
                              ,
                              OsdPanel::PanelFrame_S *pstPanelFrame = nullptr,
                              double (*pdPanelEnterTime)[SVP_AIDETECT_MAX_OUTPUT_RECT_NUM] = nullptr
#endif
);

/**
 * @brief   : 通用进入/离开区域检测处理
 * @param    {const ot_aidetect_object_of_one_class *} pstObjectClass：当前类别算法结果
 * @param    {const std::vector<Alarm::EnterExitIntrusion_S> &} aRules：规则数组
 * @param    {AreaStatus_S (&)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]} stStatusArray：区域状态数组
 * @param    {CTargetIndexManager20 &} indexManager：目标索引管理器
 * @param    {CAlarmStateMachine &} alarmStateMachine：报警状态机
 * @param    {Event::Type_E} enEventType：事件类型
 * @param    {const char *} pszDetectTypeName：检测类型日志名称
 * @param    {std::vector<Common::RectInfo_S> &} vstRectInfo：角框输出数组
 * @param    {OsdPanel::PanelFrame_S *} pstPanelFrame：展会面板结果输出指针
 * @return   {bool} true：本帧存在报警 false：本帧无报警
 * @note    : 展会版本下保留面板入参以兼容统一接口，当前默认传空
 */
template <size_t MaxRegions>
bool process_region_enter_exit_detection(const ot_aidetect_object_of_one_class *pstObjectClass,
                                         const std::vector<Alarm::EnterExitIntrusion_S> &aRules,
                                         AreaStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM],
                                         CTargetIndexManager20 &indexManager,
                                         CAlarmStateMachine &alarmStateMachine,
                                         Event::Type_E enEventType,
                                         const char *pszDetectTypeName,
                                         std::vector<Common::RectInfo_S> &vstRectInfo
#if CAP_EXHIBITION_OSD_PANEL
                                         ,
                                         OsdPanel::PanelFrame_S *pstPanelFrame = nullptr
#endif
);
} // namespace HVFDetectInternal

#include "hvf_detect_common_impl.hpp"
