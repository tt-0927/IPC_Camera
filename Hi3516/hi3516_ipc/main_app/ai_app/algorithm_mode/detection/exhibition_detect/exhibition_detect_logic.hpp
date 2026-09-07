/**
 * @FilePath     : exhibition_detect_logic.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 16:30:00
 * @Description  : 展馆人流统计/人员密度检测配置与规则逻辑
 */

#pragma once

#include <mutex>

#include "alarm_define.h"
#include "algorithm.hpp"

/**
 * @brief   : 展馆人流统计/人员密度检测逻辑
 * @note    : 持有配置（使能、跟踪器参数）与规则状态，不拥有 SDK 句柄、线程与帧。
 *            配置线程调用 apply_algo_config，Worker 线程调用 is_enabled/规则访问等；
 *            内部互斥保护共享状态（参考 CMotionDetectLogic 的 m_configMutex 模式）。
 *            人流规则线/区域/方向变化或密度生效状态翻转时置位跟踪器重建请求，
 *            由模型在 init 时消费，清空旧 Track 状态避免伪轨迹。
 */
class CExhibitionDetectLogic
{
public:
    CExhibitionDetectLogic() = default;
    ~CExhibitionDetectLogic() = default;

    /* 禁止拷贝 */
    CExhibitionDetectLogic(const CExhibitionDetectLogic &) = delete;
    CExhibitionDetectLogic &operator=(const CExhibitionDetectLogic &) = delete;

    /**
     * @brief   : 应用算法总配置（配置线程调用）
     * @param    {const Event::AlgorithmConfig} &stAlgoConfig：算法总配置
     * @note    : 人流统计与 HVF 共用 nEnPeopleFlowStatistics 开关与
     *            Alarm::PeopleFlowStatistics_S 配置；人员密度使用
     *            nEnPeopleDensityDetection 开关与 Alarm::PeopleDensityDetection_S 配置；
     *            人流规则几何/方向变化或密度生效状态翻转时置位跟踪器重建请求
     */
    void apply_algo_config(const Event::AlgorithmConfig &stAlgoConfig);

    /* ---------------- Worker 线程接口 ---------------- */

    /**
     * @brief   : 是否使能（任一算法总开关）
     * @return   {bool} true：人流统计或人员密度总开关任一打开
     * @note    : 引擎懒初始化门控
     */
    bool is_enabled() const;

    /**
     * @brief   : 人流统计是否生效（总开关 + 配置使能）
     * @return   {bool} true：本帧可执行 person 跟踪与统计
     */
    bool is_flow_active() const;

    /**
     * @brief   : 人员密度是否生效（总开关 + 配置使能）
     * @return   {bool} true：本帧可执行 head 跟踪与密度统计
     */
    bool is_density_active() const;

    /**
     * @brief   : person 跟踪器 max_age（丢帧容忍，默认 5）
     * @return   {int} max_age
     */
    int tracker_max_age() const;

    /**
     * @brief   : person 跟踪器 min_hits（防抖，默认 3）
     * @return   {int} min_hits
     */
    int tracker_min_hits() const;

    /**
     * @brief   : person 跟踪器 iou_threshold（匹配阈值，默认 0.3）
     * @return   {float} iou_threshold
     */
    float tracker_iou_threshold() const;

    /**
     * @brief   : head 跟踪器 max_age（丢帧容忍，默认 5）
     * @return   {int} max_age
     */
    int head_tracker_max_age() const;

    /**
     * @brief   : head 跟踪器 min_hits（默认 1）
     * @return   {int} min_hits
     * @note    : 密度为瞬时计数，出现即统计，与旧实现对 NEW 目标计数语义一致
     */
    int head_tracker_min_hits() const;

    /**
     * @brief   : head 跟踪器 iou_threshold（匹配阈值，默认 0.3）
     * @return   {float} iou_threshold
     */
    float head_tracker_iou_threshold() const;

    /**
     * @brief   : 跟踪器轨迹容量上限（person/head 共用，默认 20）
     * @return   {int} 容量上限
     * @note    : 满载时停止创建新轨迹，防止突发检测撑爆轨迹表
     */
    int tracker_max_tracks() const;

    /**
     * @brief   : 消费跟踪器重建请求（仅 Worker 线程调用）
     * @return   {bool} true：存在重建请求（本次调用后清除）
     */
    bool consume_tracker_rebuild_request();

    /**
     * @brief   : 获取当前人流统计完整配置快照
     * @param    {Alarm::PeopleFlowStatistics_S} &stOut：输出配置快照
     */
    void snapshot_config(Alarm::PeopleFlowStatistics_S &stOut) const;

    /**
     * @brief   : 获取当前人员密度完整配置快照
     * @param    {Alarm::PeopleDensityDetection_S} &stOut：输出配置快照
     */
    void snapshot_density_config(Alarm::PeopleDensityDetection_S &stOut) const;

private:
    /**
     * @brief   : 判断人流规则是否发生变化（不加锁版本，调用方必须已持有 m_configMutex）
     * @param    {const Alarm::PeopleFlowStatistics_S} &stNewConfig：新配置
     * @return   {bool} true：规则线/区域/方向变化
     */
    bool is_rule_changed_unlocked(const Alarm::PeopleFlowStatistics_S &stNewConfig) const;

private:
    /* lock: 保护配置/使能/重建请求（配置线程写、Worker 线程读） */
    mutable std::mutex m_configMutex;
    /* 人流统计算法总开关（nEnPeopleFlowStatistics） */
    bool m_bAlgoEnabled = false;
    /* 人员密度算法总开关（nEnPeopleDensityDetection） */
    bool m_bDensityAlgoEnabled = false;
    /* 最近一次人流统计完整配置 */
    Alarm::PeopleFlowStatistics_S m_stConfig;
    /* 最近一次人员密度完整配置 */
    Alarm::PeopleDensityDetection_S m_stDensityConfig;
    /* 规则变化后待消费的跟踪器重建请求 */
    bool m_bTrackerRebuildPending = false;
    /* person 跟踪器丢帧容忍（与 CPeopleFlowProcessor 5 秒 TTL 配合） */
    int m_nTrackerMaxAge = 5;
    /* person 跟踪器防抖：连续命中帧数 */
    int m_nTrackerMinHits = 3;
    /* person 跟踪器 IOU 匹配阈值 */
    float m_fTrackerIouThreshold = 0.3f;
    /* head 跟踪器丢帧容忍 */
    int m_nHeadTrackerMaxAge = 5;
    /* head 跟踪器防抖：密度瞬时计数要求出现即统计 */
    int m_nHeadTrackerMinHits = 1;
    /* head 跟踪器 IOU 匹配阈值 */
    float m_fHeadTrackerIouThreshold = 0.3f;
    /* 跟踪器轨迹容量上限（person/head 共用） */
    int m_nTrackerMaxTracks = 20;
};
