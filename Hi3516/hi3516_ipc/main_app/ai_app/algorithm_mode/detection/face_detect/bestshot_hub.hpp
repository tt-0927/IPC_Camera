/**
 * @FilePath     : bestshot_hub.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸链路取优枢纽——CBestShotManager 唯一持有者 + 新 best 事件扇出。
 *
 * 设计定位：取优（评分选优）是链路级能力，抓拍/比对/名单库共用同一份选优桶
 * 与评分后端，经监听器各自消费"新 best"事件——抓拍缓存 best 小图（本期）；
 * 比对最优帧提特征（原生比对迁移时接入）；实时流入库取最优帧（预留接口）。
 * 避免各处理器各持 manager 造成评分重复计算与桶状态不一致。
 *
 * 调用契约：与 bestshot 模块一致，全部接口须在同一 Worker 回调线程调用，
 * 内部无锁。喂入时机分两档：抓拍开启时由抓拍处理器驱动（规则过滤集，含
 * ROI 亮度查询）；抓拍关闭、仅比对时由模型编排驱动（灵敏度过滤集，亮度
 * 按未知评选）。多目标各自独立 best 桶。
 */

#pragma once

#include <memory>
#include <vector>

#include "bestshot_manager.h"
#include "ot_common_video.h"

/* 取优事件监听器：新 best / 目标结束 / 全清，均在 Worker 回调线程回调 */
class IFaceBestShotListener
{
public:
    virtual ~IFaceBestShotListener() = default;

    /*
     * 某目标产生新 best（滞回判定通过）：
     * pFrame/stRoi 为产生本 best 的候选帧与裁剪框（源帧坐标系）。
     * 帧生命周期仅限回调内，需要留存须自行编码缓存，不得保存指针。
     */
    virtual void onNewBest(int nTrackId,
                           const Bestshot_NS::BestShotFeedResult_S &stResult,
                           const ot_video_frame_info *pFrame,
                           const ot_rect &stRoi) = 0;

    /* 目标结束（轨迹回收/出图取走/保持窗口超时），清理该目标关联缓存 */
    virtual void onTrackEnded(int nTrackId) = 0;

    /* 全部取优状态清空（事件结束/链路反初始化），清理全部关联缓存 */
    virtual void onAllCleared() = 0;
};

/* 取优枢纽：单线程契约（Worker 回调线程），由模型持有与装配 */
class CBestShotHub
{
public:
    CBestShotHub();
    ~CBestShotHub();

    /*
     * 装配评分后端并启用取优（唯一初始化点）：注释掉本调用即整体关闭取优，
     * feed 恒无 best，抓拍回退当帧编码出图，事件/联动逻辑不受影响。
     * 须在首次 feed 前调用；监听器须在此之前注册完成。
     */
    void init(std::unique_ptr<Bestshot_NS::ISharpnessScorer> &&pScorer, const Bestshot_NS::BestShotConfig_S &stCfg = {});

    /* 释放评分后端并清空全部桶（扇出 onAllCleared），幂等 */
    void deinit();

    /* 注册监听器（不持有生命周期，不得重复注册同一指针） */
    void registerListener(IFaceBestShotListener *pListener);

    /*
     * 喂入一个候选：内部评分选优，新 best 时扇出 onNewBest。
     * stRoi 为源帧坐标系裁剪框（出图对齐口径）；目标过小无有效裁剪框时
     * 传零尺寸框，仅置信度维度参与选优（与既有取优语义一致）。
     * fBrightness 为 ROI 平均亮度 [0,255]（VGS 批量统计，见抓拍处理器）；
     * 不传或传 -1 表示未知，亮度门限与亮度评分整体跳过。
     */
    Bestshot_NS::BestShotFeedResult_S feed(int nTrackId,
                                           const ot_video_frame_info *pFrame,
                                           const ot_rect &stRoi,
                                           float fDetectScore,
                                           long long llTimestamp,
                                           float fBrightness = -1.0f);

    /* 单目标结束：复位该目标选优桶并扇出 onTrackEnded（出图取走 best / 轨迹结束） */
    void endTarget(int nTrackId);

    /* 保持窗口检查：超时桶失效并逐个扇出 onTrackEnded，返回失效的 trackId 列表 */
    std::vector<int64_t> expireAll(long long llNowMs);

    /* 全清（抓拍事件结束）：清空全部桶并扇出 onAllCleared */
    void clearAll();

private:
    Bestshot_NS::CBestShotManager m_manager;
    std::vector<IFaceBestShotListener *> m_vListeners;
};
