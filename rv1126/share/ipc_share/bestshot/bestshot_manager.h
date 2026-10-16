/**
 * @FilePath     : bestshot_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 抓拍取优模块门面：清晰度评分（可插拔后端）+ 按 trackId 分桶的滞回选优。
 *
 * 评分与取优一体：清晰度是综合评分的主维度，评分后端由装配点注入（CPU 软件 /
 * 平台硬件适配器），模块内部完成"评分 -> 综合评分 -> 滞回选优"，调用方不感知后端。
 *
 * 裁剪约定：不调用 init 即整体旁路——feed 恒返回无 best 结果，调用方
 * （抓拍/事件链路）自动回退当帧出图，事件逻辑不受影响；想用哪个评分后端
 * 就在装配点注入哪个。
 *
 * 资源上界：选优桶数量上限 unMaxTargets（默认 32，每桶约百字节）；达到上限后
 * 新目标不参与取优（feed 返回无 best），已有桶继续正常选优。
 *
 * 线程契约：单线程调用（检测回调线程），模块内部无锁；跨线程调用需调用方保证互斥。
 */

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "best_shot_selector.h"
#include "bestshot_types.h"
#include "sharpness_scorer.h"

namespace Bestshot_NS
{
class CBestShotManager
{
public:
    /* 注入评分后端并启用取优；重复调用会先释放旧后端并清空全部选优桶 */
    void init(std::unique_ptr<ISharpnessScorer> &&pScorer, const BestShotConfig_S &stCfg = {});

    /* 释放后端并清空全部选优桶（幂等，随析构自动执行） */
    void deinit();

    bool isReady() const
    {
        return m_pScorer != nullptr;
    }

    /*
     * 喂入候选帧：模块内部评分并按滞回策略更新该 track 的 best。
     * nTrackId 为 -1 时各目标共用同一桶（兼容未接跟踪的场景）。
     */
    BestShotFeedResult_S feed(int64_t nTrackId, const BestShotFrameView_S &stFrame, const BestShotCandidateInput_S &stInput);

    /* 无候选帧时检查保持窗口，返回过期的 trackId 列表（调用方同步清理关联缓存） */
    std::vector<int64_t> expireAll(long long llNowMs);

    /* 单目标结束（跟踪结束或出图取走 best）：清空该桶 */
    void onTargetEnd(int64_t nTrackId);

    /* 事件结束：清空全部选优桶，下一轮事件重新选优 */
    void clearAll();

private:
    std::unique_ptr<ISharpnessScorer> m_pScorer;
    BestShotConfig_S m_stCfg;
    std::map<int64_t, CBestShotSelector> m_mapSelectors;
};
} // namespace Bestshot_NS
