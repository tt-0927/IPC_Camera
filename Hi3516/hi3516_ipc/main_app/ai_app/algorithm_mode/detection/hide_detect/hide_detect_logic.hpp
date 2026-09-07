/**
 * @FilePath     : hide_detect_logic.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 13:50:00
 * @Description  : 遮挡侦测事件判断逻辑（NEW 模式适配器共用，语义与 LEGACY 一致）
 */

#pragma once

#define HIDE_DETECT_LOGIC_HPP

#include <mutex>

#include "algorithm.hpp"
#include "alarm_define.h"
#include "common_define.h"
#include "video_define.h"

/**
 * @brief   : 遮挡侦测事件判断逻辑
 * @note    : 从 CHideDetect 抽取的纯逻辑部分：配置应用（坐标转换、区域校验、
 *            使能判定、重建判定）、遮挡阈值判断与报警状态机。
 *            不拥有 SDK 句柄、线程与缩放/裁剪帧。
 *            配置线程调用 apply_config/set_enabled，Worker 线程调用
 *            is_enabled/process_result 等；内部互斥保护共享状态。
 */
class CHideDetectLogic
{
public:
    CHideDetectLogic();
    ~CHideDetectLogic() = default;

    /* 禁止拷贝 */
    CHideDetectLogic(const CHideDetectLogic &) = delete;
    CHideDetectLogic &operator=(const CHideDetectLogic &) = delete;

    /**
     * @brief   : 应用遮挡侦测配置（配置线程调用）
     * @param    {const Alarm::HideAlarm_S} &stAlgoCfg：遮挡侦测配置
     * @return   {bool} true：需要重新初始化（区域宽高变化），false：无需重建
     * @note    : 语义与 LEGACY CHideDetect::setAlgoParamCfg 完全一致，
     *            包括"区域位置仅在宽高变化时更新"的既有行为
     */
    bool apply_config(const Alarm::HideAlarm_S &stAlgoCfg);

    /**
     * @brief   : 仅更新使能标志（配置线程调用，对应 LEGACY setAlgoEnCfg 关闭分支）
     * @param    {bool} bEnable：是否使能
     */
    void set_enabled(bool bEnable);

    /* ---------------- Worker 线程接口 ---------------- */

    /**
     * @brief   : 是否使能（总开关）
     * @return   {bool} true：使能
     */
    bool is_enabled() const;

    /**
     * @brief   : 侦测区域（算法坐标系 1024×576）
     * @return   {Common::Rect_S} 区域副本
     */
    Common::Rect_S rect() const;

    /**
     * @brief   : 是否需要裁剪（区域尺寸小于算法分辨率，对应 LEGACY m_bIsCrop）
     * @return   {bool} true：需要裁剪
     */
    bool is_crop() const;

    /**
     * @brief   : 遮挡侦测后处理（阈值判断 + 报警状态机）
     * @param    {int} nHideResult：遮挡检测结果值
     * @param    {const SEventProcessContext} &stCtx：事件处理上下文
     * @return   {bool} 是否有报警
     * @note    : 语义与 LEGACY CHideDetect::processHideDetect 完全一致
     */
    bool process_result(int nHideResult, const SEventProcessContext &stCtx);

private:
    /* 以下私有方法仅在持有 m_configMutex 时调用（不加锁，避免重入） */

    /**
     * @brief   : 转换区域坐标并判断是否使能算法（仅 HideAlarm_S 实例化版本）
     * @param    {Alarm::HideAlarm_S} &stConfig：待转换的遮挡侦测配置
     */
    void convert_resolution_and_enable_unlocked(Alarm::HideAlarm_S &stConfig);

    /**
     * @brief   : 是否需要裁剪（不加锁版本，调用方必须已持有 m_configMutex）
     * @return   {bool} true：需要裁剪
     */
    bool is_crop_unlocked() const;

private:
    /* lock: 保护配置/区域（配置线程写、Worker 线程读） */
    mutable std::mutex m_configMutex;
    /* 遮挡侦测配置参数 */
    Alarm::HideAlarm_S m_stAlgoHideDetCfg;
    /* 侦测区域（算法坐标系） */
    Common::Rect_S m_stRect;
    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1024;
    int m_nHeight = PIXEL_HEIGHT_576;
    /* 遮挡侦测报警状态机，判断是否进行报警（仅 Worker 线程访问） */
    CAlarmStateMachine m_hideAlarmStateMachine;
};
