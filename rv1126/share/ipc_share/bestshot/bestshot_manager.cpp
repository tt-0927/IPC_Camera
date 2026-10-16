/**
 * @FilePath     : bestshot_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 抓拍取优模块门面实现。
 */

#include "bestshot_manager.h"

namespace Bestshot_NS
{
void CBestShotManager::init(std::unique_ptr<ISharpnessScorer> &&pScorer, const BestShotConfig_S &stCfg)
{
    m_pScorer = std::move(pScorer);
    m_stCfg = stCfg;
    m_mapSelectors.clear();
}

void CBestShotManager::deinit()
{
    m_pScorer.reset();
    m_mapSelectors.clear();
}

BestShotFeedResult_S CBestShotManager::feed(int64_t nTrackId, const BestShotFrameView_S &stFrame, const BestShotCandidateInput_S &stInput)
{
    /* 未注入评分后端：整体旁路（初始化被裁剪时不影响事件链路） */
    BestShotFeedResult_S stResult;
    if (m_pScorer == nullptr)
    {
        return stResult;
    }

    BestShotCandidate_S stCand;
    stCand.stRoi = stInput.stRoi;
    stCand.fDetectScore = stInput.fDetectScore;
    stCand.unArea = stInput.unArea;
    stCand.llTimestamp = stInput.llTimestamp;
    stCand.fBrightness = stInput.fBrightness;
    /* 评分失败（含无效 ROI）保持 -1 哨兵，该帧综合分退化为置信度主导 */
    stCand.fSharpness = m_pScorer->eval(stFrame, stInput.stRoi);

    auto itSelector = m_mapSelectors.find(nTrackId);
    if (itSelector == m_mapSelectors.end())
    {
        /* 桶数量达到上限后不再为新目标建桶（该目标不参与取优，已有桶不受影响），
           防调用方漏调 onTargetEnd/expireAll 时 map 无界增长 */
        if (m_mapSelectors.size() >= m_stCfg.unMaxTargets)
        {
            return stResult;
        }
        itSelector = m_mapSelectors.emplace(nTrackId, CBestShotSelector(m_stCfg)).first;
    }
    CBestShotSelector &stSelector = itSelector->second;
    stResult.fSharpness = stCand.fSharpness;
    /* 亮度门限否决（过暗/过曝）的帧不参与评选：不评分排序、不更新 best */
    stResult.bGated = stSelector.isGated(stCand);
    if (stResult.bGated)
    {
        return stResult;
    }
    /* 首帧保底：桶空且亮度超范围仍入桶（此帧必成 best），标注给调用方日志 */
    stResult.bGateBypassed = !stSelector.hasBest() && stSelector.outOfBrightnessRange(stCand);
    stResult.fTotalScore = stSelector.score(stCand);
    stResult.fBestScore = stSelector.hasBest() ? stSelector.score(stSelector.best()) : -1.0f;
    stResult.bNewBest = stSelector.feed(stCand);
    if (stResult.bNewBest)
    {
        stResult.fBestScore = stResult.fTotalScore;
    }
    return stResult;
}

std::vector<int64_t> CBestShotManager::expireAll(long long llNowMs)
{
    std::vector<int64_t> vecExpired;
    for (auto it = m_mapSelectors.begin(); it != m_mapSelectors.end();)
    {
        if (it->second.expire(llNowMs))
        {
            vecExpired.push_back(it->first);
            it = m_mapSelectors.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return vecExpired;
}

void CBestShotManager::onTargetEnd(int64_t nTrackId)
{
    m_mapSelectors.erase(nTrackId);
}

void CBestShotManager::clearAll()
{
    m_mapSelectors.clear();
}
} // namespace Bestshot_NS
