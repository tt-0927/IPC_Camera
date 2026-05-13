/**
 * @FilePath     : hvf_parking_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 停车侦测处理器
 */

#pragma once

#include "algorithm.hpp"
#include "internal/base/hvf_detect_context.hpp"
#include "target_index_manager.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 停车侦测后处理器
 */
class CHVFParkingProcessor
{
public:
    /**
     * @brief   : 构造停车侦测处理器
     */
    CHVFParkingProcessor();

    /**
     * @brief   : 析构停车侦测处理器
     */
    ~CHVFParkingProcessor() = default;

    /**
     * @brief   : 设置停车侦测使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置停车侦测参数
     * @param    {const Alarm::ParkingDetection_S &} stAlgoCfg：停车侦测配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理停车侦测结果
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
    /* 停车侦测配置 */
    Alarm::ParkingDetection_S m_stAlgoCfg;
    /* 停车侦测状态数组 */
    AreaStatus_S m_stParkingStatus[PARKING_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
    /* 停车侦测目标索引管理器 */
    CTargetIndexManager20 m_indexManager{ "HVFParkingIndex" };
    /* 停车侦测报警状态机 */
    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace HVFDetectInternal
