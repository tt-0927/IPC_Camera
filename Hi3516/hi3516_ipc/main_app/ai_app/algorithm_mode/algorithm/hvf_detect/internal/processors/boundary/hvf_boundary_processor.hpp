/**
 * @FilePath     : hvf_boundary_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 越界侦测处理器
 */

#pragma once

#include "algorithm.hpp"
#include "internal/base/hvf_detect_context.hpp"
#include "target_index_manager.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 越界侦测后处理器
 */
class CHVFBoundaryProcessor
{
public:
    /**
     * @brief   : 构造越界侦测处理器
     */
    CHVFBoundaryProcessor();

    /**
     * @brief   : 析构越界侦测处理器
     */
    ~CHVFBoundaryProcessor() = default;

    /**
     * @brief   : 设置越界侦测使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置越界侦测参数
     * @param    {const Alarm::BoundaryDetection_S &} stAlgoCfg：越界侦测配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理越界侦测结果
     * @param    {SHVFProcessContext &} stContext：单帧处理上下文
     * @return   {void}
     */
    void process(SHVFProcessContext &stContext);

    /**
     * @brief   : 获取当前是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isEnabled() const;

private:
    /* 越界侦测配置 */
    Alarm::BoundaryDetection_S m_stAlgoCfg;
    /* 越界侦测状态数组 */
    BoundaryTrackStatus_S m_stBoundaryStatus[BOUND_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
    /* 越界侦测目标索引管理器 */
    CTargetIndexManager20 m_indexManager{ "HVFBoundaryIndex" };
    /* 越界侦测报警状态机 */
    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace HVFDetectInternal
