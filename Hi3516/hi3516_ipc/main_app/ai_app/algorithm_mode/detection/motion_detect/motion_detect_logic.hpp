/**
 * @FilePath     : motion_detect_logic.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 12:56:51
 * @Description  : 移动侦测事件判断逻辑（NEW 模式适配器共用，语义与 LEGACY 一致）
 */

#pragma once

#include <mutex>

#include "algorithm.hpp"
#include "alarm_define.h"
#include "common_define.h"
#include "video_define.h"

extern "C"
{
    /* 显式引入 ot_sample_svp_rect_info/ot_sample_svp（process_result 参数类型），
     * 不依赖 algorithm.hpp -> stream_video.h -> osd_manage.h 的传递包含链 */
    #include "svp_md.h"
}

/**
 * @brief   : 移动侦测事件判断逻辑
 * @note    : 从 CMotionDetect 抽取的纯逻辑部分：配置应用（模式/区域校验、
 *            m_stRect 计算、绘制标志、重建判定）、事件判断（普通/专家模式）、
 *            报警状态机。不拥有 SDK 句柄、线程与缩放/裁剪帧。
 *            配置线程调用 apply_config/set_enabled，Worker 线程调用
 *            is_enabled/process_result 等；内部互斥保护共享状态。
 */
class CMotionDetectLogic
{
public:
    CMotionDetectLogic();
    ~CMotionDetectLogic() = default;

    /* 禁止拷贝 */
    CMotionDetectLogic(const CMotionDetectLogic &) = delete;
    CMotionDetectLogic &operator=(const CMotionDetectLogic &) = delete;

    /**
     * @brief   : 应用移动侦测配置（配置线程调用）
     * @param    {const Alarm::MotionDetection_S} &stAlgoCfg：移动侦测配置
     * @return   {bool} true：需要重新初始化（模式切换或区域宽高变化），
     *                   false：无需重建
     * @note    : 语义与 LEGACY CMotionDetect::setAlgoParamCfg 完全一致
     */
    bool apply_config(const Alarm::MotionDetection_S &stAlgoCfg);

    /**
     * @brief   : 仅更新使能标志（配置线程调用，对应 LEGACY setAlgoEnCfg 关闭分支）
     * @param    {bool} bEnable：是否使能
     */
    void set_enabled(bool bEnable);

    /* ---------------- Worker 线程接口 ---------------- */

    /**
     * @brief   : 是否使能（总开关 + 区域已绘制）
     * @return   {bool} true：使能
     */
    bool is_enabled() const;

    /**
     * @brief   : 是否已绘制侦测区域
     * @return   {bool} true：已绘制
     */
    bool is_draw() const;

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
     * @brief   : 处理移动侦测结果（按模式分发普通/专家处理）
     * @param    {ot_sample_svp_rect_info} &stRectInfo：算法输出结果
     * @param    {const SEventProcessContext} &stCtx：事件处理上下文
     * @note    : 语义与 LEGACY run() 中 sendFrame/getResult 后的处理分支一致，
     *            包含坐标补偿（裁剪时）、灵敏度判断、OSD 联动与报警状态机
     */
    void process_result(ot_sample_svp_rect_info &stRectInfo, const SEventProcessContext &stCtx);

private:
    /* 以下私有方法仅在持有 m_configMutex 时调用（不加锁，避免重入） */

    /**
     * @brief   : 判断当前是否为白天（专家模式用）
     * @return   {bool} true：白天 false：夜晚
     */
    bool is_daytime_unlocked() const;

    /**
     * @brief   : 计算两个矩形的重叠面积
     * @param    {const Common::Rect_S} &rect1：矩形1
     * @param    {const Common::Rect_S} &rect2：矩形2
     * @return   {int} 重叠面积
     */
    int calculate_overlap_area_unlocked(const Common::Rect_S &rect1,
                                        const Common::Rect_S &rect2) const;

    /**
     * @brief   : 将算法输出的点坐标转换为矩形
     * @param    {const ot_sample_svp_rect} &rect：算法输出的矩形点
     * @return   {Common::Rect_S} 转换后的矩形
     */
    Common::Rect_S convert_to_rect_unlocked(const ot_sample_svp_rect &rect) const;

    /**
     * @brief   : 普通模式处理函数
     * @param    {ot_sample_svp_rect_info} &stRectInfo：算法输出结果
     * @param    {const SEventProcessContext} &stCtx：事件处理上下文
     */
    void process_normal_mode_unlocked(ot_sample_svp_rect_info &stRectInfo,
                                      const SEventProcessContext &stCtx);

    /**
     * @brief   : 专家模式处理函数
     * @param    {ot_sample_svp_rect_info} &stRectInfo：算法输出结果
     * @param    {const SEventProcessContext} &stCtx：事件处理上下文
     */
    void process_expert_mode_unlocked(ot_sample_svp_rect_info &stRectInfo,
                                      const SEventProcessContext &stCtx);

    /**
     * @brief   : 是否需要裁剪（不加锁版本，调用方必须已持有 m_configMutex）
     * @return   {bool} true：需要裁剪
     */
    bool is_crop_unlocked() const;

private:
    /* lock: 保护配置/区域/绘制标志（配置线程写、Worker 线程读） */
    mutable std::mutex m_configMutex;
    /* 移动侦测配置参数 */
    Alarm::MotionDetection_S m_stMotionDetCfg;
    /* 侦测区域（算法坐标系） */
    Common::Rect_S m_stRect;
    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1024;
    int m_nHeight = PIXEL_HEIGHT_576;
    /* 是否绘制了区域 */
    bool m_bIsDraw = false;
    /* 移动侦测报警状态机，判断是否进行报警（仅 Worker 线程访问） */
    CAlarmStateMachine m_motionAlarmStateMachine;
};
