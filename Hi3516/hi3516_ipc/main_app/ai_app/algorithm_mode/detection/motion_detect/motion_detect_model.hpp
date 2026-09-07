/**
 * @FilePath     : motion_detect_model.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 11:49:38
 * @Description  : 移动侦测模型适配器（svp_md，NEW 模式接入统一检测引擎）
 */

#pragma once

#include <atomic>

#include "detection_engine.hpp"
#include "event_manager.hpp"
#include "motion_detect_logic.hpp"

extern "C"
{
    #include "svp_md.h"
}

/**
 * @brief   : 移动侦测模型适配器
 * @note    : 持有 svp_md 句柄（HiMd_S*）与 CMotionDetectLogic；
 *            SDK 句柄生命周期（init/unInit/process）仅由引擎 Worker 线程访问，
 *            配置线程只通过 setAlgoEnCfg 更新逻辑状态（内部互斥保护）。
 */
class CMotionDetectModel : public IDetectionModel
{
public:
    CMotionDetectModel() = default;
    ~CMotionDetectModel() override = default;

    /* 禁止拷贝 */
    CMotionDetectModel(const CMotionDetectModel &) = delete;
    CMotionDetectModel &operator=(const CMotionDetectModel &) = delete;

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
    /* svp_md 句柄（仅 Worker 线程访问） */
    HiMd_S *m_pMotionDetHandle = nullptr;
    /* 检测频率控制（每 500ms 处理一帧，与 LEGACY m_RecvManager 一致） */
    EventManager m_recvManager{500};
    /* 事件判断逻辑（配置/区域/报警状态机） */
    CMotionDetectLogic m_logic;
    /* 配置变更后是否需要重建句柄（配置线程置位，Worker 线程 init 时消费） */
    std::atomic<bool> m_bNeedReinit{false};
    /* 裁剪区域快照（仅 Worker 线程访问，crop_rect 返回其指针） */
    mutable Common::Rect_S m_stCropSnapshot;
};
