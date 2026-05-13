/**
 * @FilePath     : hvf_loitering_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 徘徊侦测处理器
 */

#pragma once

#include "algorithm.hpp"
#include "internal/base/hvf_detect_context.hpp"
#include "target_index_manager.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 徘徊侦测后处理器
 */
class CHVFLoiteringProcessor
{
public:
    /**
     * @brief   : 构造徘徊侦测处理器
     */
    CHVFLoiteringProcessor();

    /**
     * @brief   : 析构徘徊侦测处理器
     */
    ~CHVFLoiteringProcessor() = default;

    /**
     * @brief   : 设置徘徊侦测使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置徘徊侦测参数
     * @param    {const Alarm::LoiteringDetection_S &} stAlgoCfg：徘徊侦测配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理徘徊侦测结果
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
    /* 徘徊侦测配置 */
    Alarm::LoiteringDetection_S m_stAlgoCfg;
    /* 徘徊侦测状态数组 */
    AreaStatus_S m_stLoiteringStatus[LOITERING_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
#if CAP_EXHIBITION_OSD_PANEL
    /* 徘徊面板累计时长起点，避免面板时间受报警节拍影响回跳 */
    double m_stPanelDurationStartTime[LOITERING_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
#endif
    /* 徘徊侦测目标索引管理器 */
    CTargetIndexManager20 m_indexManager{ "HVFLoiteringIndex" };
    /* 徘徊侦测报警状态机 */
    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace HVFDetectInternal
