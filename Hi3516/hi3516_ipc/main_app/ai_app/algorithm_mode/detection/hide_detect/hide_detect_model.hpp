/**
 * @FilePath     : hide_detect_model.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 13:50:00
 * @Description  : 遮挡侦测模型适配器（svp_od，NEW 模式接入统一检测引擎）
 */

#pragma once

#define HIDE_DETECT_MODEL_HPP

#include <atomic>

#include "detection_engine.hpp"
#include "event_manager.hpp"
#include "hide_detect_logic.hpp"

extern "C"
{
    #include "svp_od.h"
}

/**
 * @brief   : 遮挡侦测模型适配器
 * @note    : 持有 svp_od 句柄（HiOd_S*）与 CHideDetectLogic；
 *            SDK 句柄生命周期（init/unInit/process）仅由引擎 Worker 线程访问，
 *            配置线程只通过 setAlgoEnCfg 更新逻辑状态（内部互斥保护）。
 */
class CHideDetectModel : public IDetectionModel
{
public:
    CHideDetectModel() = default;
    ~CHideDetectModel() override = default;

    /* 禁止拷贝 */
    CHideDetectModel(const CHideDetectModel &) = delete;
    CHideDetectModel &operator=(const CHideDetectModel &) = delete;

    const DetectionModelDescriptor_S &descriptor() const override;
    bool isEnabled() const override;
    bool shouldProcess(int nChannelId) override;
    int process(const ot_video_frame_info *pPreparedFrame,
                const MediaData_S &stMediaData) override;
    int init() override;
    void unInit() override;

    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;
    bool needsReinit() const override;
    const Common::Rect_S *crop_rect() const override;

private:
    /* 模型描述（名称/输入分辨率/裁剪需求/执行顺序） */
    static const DetectionModelDescriptor_S ms_stDescriptor;
    /* svp_od 句柄（仅 Worker 线程访问） */
    HiOd_S *m_pHideDetHandle = nullptr;
    /* 检测频率控制（每 500ms 处理一帧，与 LEGACY m_RecvManager 一致） */
    EventManager m_recvManager{500};
    /* 事件判断逻辑（配置/区域/报警状态机） */
    CHideDetectLogic m_logic;
    /* 配置变更后是否需要重建句柄（配置线程置位，Worker 线程 init 时消费） */
    std::atomic<bool> m_bNeedReinit{false};
    /* 裁剪区域快照（仅 Worker 线程访问，crop_rect 返回其指针） */
    mutable Common::Rect_S m_stCropSnapshot;
};
