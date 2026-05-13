/**
 * @FilePath     : hvf_face_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 人脸侦测处理器
 */

#pragma once

#include "algorithm.hpp"
#include "internal/base/hvf_detect_context.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 人脸侦测后处理器
 */
class CHVFFaceProcessor
{
public:
    /**
     * @brief   : 构造人脸侦测处理器
     */
    CHVFFaceProcessor() = default;

    /**
     * @brief   : 析构人脸侦测处理器
     */
    ~CHVFFaceProcessor() = default;

    /**
     * @brief   : 设置人脸侦测使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置人脸侦测参数
     * @param    {const Alarm::FaceDetection_S &} stAlgoCfg：人脸侦测配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::FaceDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理人脸侦测结果
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
    /* 人脸侦测配置 */
    Alarm::FaceDetection_S m_stFaceDetCfg;
    /* 人脸侦测报警状态机 */
    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace HVFDetectInternal
