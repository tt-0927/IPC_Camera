/**
 * @FilePath     : hvf_enter_exit_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 进入/离开区域处理器
 */

#pragma once

#include "algorithm.hpp"
#include "internal/base/hvf_detect_context.hpp"
#include "target_index_manager.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 进入/离开区域后处理器
 */
class CHVFEnterExitProcessor
{
public:
    /**
     * @brief   : 构造进入/离开区域处理器
     */
    CHVFEnterExitProcessor();

    /**
     * @brief   : 析构进入/离开区域处理器
     */
    ~CHVFEnterExitProcessor() = default;

    /**
     * @brief   : 设置进入区域侦测使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEntranceEnabled(bool bEnable);

    /**
     * @brief   : 设置离开区域侦测使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setExitEnabled(bool bEnable);

    /**
     * @brief   : 设置进入区域侦测参数
     * @param    {const Alarm::EntranceDetection_S &} stAlgoCfg：进入区域配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setEntranceAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 设置离开区域侦测参数
     * @param    {const Alarm::ExitingDetection_S &} stAlgoCfg：离开区域配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setExitAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理进入区域结果
     * @param    {SHVFProcessContext &} stContext：单帧处理上下文
     * @return   {void}
     */
    void processEntrance(SHVFProcessContext &stContext);

    /**
     * @brief   : 处理离开区域结果
     * @param    {SHVFProcessContext &} stContext：单帧处理上下文
     * @return   {void}
     */
    void processExit(SHVFProcessContext &stContext);

    /**
     * @brief   : 获取进入区域是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isEntranceEnabled() const;

    /**
     * @brief   : 获取离开区域是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isExitEnabled() const;

private:
    /* 进入区域配置 */
    Alarm::EntranceDetection_S m_stEntranceCfg;
    /* 离开区域配置 */
    Alarm::ExitingDetection_S m_stExitCfg;
    /* 进入区域状态数组 */
    AreaStatus_S m_stEntranceStatus[ENTER_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
    /* 离开区域状态数组 */
    AreaStatus_S m_stExitStatus[EXIT_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
    /* 进入区域目标索引管理器 */
    CTargetIndexManager20 m_enterIndexManager{ "HVFEnterIndex" };
    /* 离开区域目标索引管理器 */
    CTargetIndexManager20 m_exitIndexManager{ "HVFExitIndex" };
    /* 进入区域报警状态机 */
    CAlarmStateMachine m_enterAlarmStateMachine;
    /* 离开区域报警状态机 */
    CAlarmStateMachine m_exitAlarmStateMachine;
};
} // namespace HVFDetectInternal
