/**
 * @FilePath     : hvf_intrusion_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 区域入侵处理器
 */

#pragma once

#include "algorithm.hpp"
#include "internal/base/hvf_detect_context.hpp"
#include "target_index_manager.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 区域入侵后处理器
 */
class CHVFIntrusionProcessor
{
public:
    /**
     * @brief   : 构造区域入侵处理器
     */
    CHVFIntrusionProcessor();

    /**
     * @brief   : 析构区域入侵处理器
     */
    ~CHVFIntrusionProcessor() = default;

    /**
     * @brief   : 设置区域入侵使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置区域入侵参数
     * @param    {const Alarm::FieldDetection_S &} stAlgoCfg：区域入侵配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理区域入侵结果
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
    /* 区域入侵配置 */
    Alarm::FieldDetection_S m_stAlgoCfg;
    /* 区域入侵状态数组 */
    AreaStatus_S m_stFieldStatus[FIELD_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
    /* 区域入侵目标索引管理器 */
    CTargetIndexManager20 m_indexManager{ "HVFIntrusionIndex" };
    /* 区域入侵报警状态机 */
    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace HVFDetectInternal
