/**
 * @FilePath     : bestshot_hub.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸链路取优枢纽实现。
 */

#include "bestshot_hub.hpp"

CBestShotHub::CBestShotHub() = default;

CBestShotHub::~CBestShotHub()
{
    deinit();
}

void CBestShotHub::init(std::unique_ptr<Bestshot_NS::ISharpnessScorer> &&pScorer, const Bestshot_NS::BestShotConfig_S &stCfg)
{
    m_manager.init(std::move(pScorer), stCfg);
}

void CBestShotHub::deinit()
{
    m_manager.deinit();
    for (IFaceBestShotListener *pListener : m_vListeners)
    {
        pListener->onAllCleared();
    }
}

void CBestShotHub::registerListener(IFaceBestShotListener *pListener)
{
    if (pListener == nullptr)
    {
        return;
    }
    for (IFaceBestShotListener *pRegistered : m_vListeners)
    {
        if (pRegistered == pListener)
        {
            return;
        }
    }
    m_vListeners.push_back(pListener);
}

Bestshot_NS::BestShotFeedResult_S CBestShotHub::feed(int nTrackId,
                                                     const ot_video_frame_info *pFrame,
                                                     const ot_rect &stRoi,
                                                     float fDetectScore,
                                                     long long llTimestamp,
                                                     float fBrightness)
{
    if (pFrame == nullptr)
    {
        return {};
    }

    /* 帧视图（CPU 后端读虚拟地址，硬件后端用物理地址）+ 候选输入，评分在模块内部执行 */
    Bestshot_NS::BestShotFrameView_S stView;
    stView.pYVirAddr = static_cast<const uint8_t *>(pFrame->video_frame.virt_addr[0]);
    stView.unYPhysAddr = static_cast<uint64_t>(pFrame->video_frame.phys_addr[0]);
    stView.unStride = pFrame->video_frame.stride[0];
    Bestshot_NS::BestShotCandidateInput_S stInput;
    stInput.stRoi.nX = stRoi.x;
    stInput.stRoi.nY = stRoi.y;
    stInput.stRoi.unWidth = stRoi.width;
    stInput.stRoi.unHeight = stRoi.height;
    stInput.fDetectScore = fDetectScore;
    stInput.unArea = static_cast<uint64_t>(stRoi.width) * stRoi.height;
    stInput.llTimestamp = llTimestamp;
    stInput.fBrightness = fBrightness;

    const Bestshot_NS::BestShotFeedResult_S stResult = m_manager.feed(nTrackId, stView, stInput);
    if (stResult.bNewBest)
    {
        for (IFaceBestShotListener *pListener : m_vListeners)
        {
            pListener->onNewBest(nTrackId, stResult, pFrame, stRoi);
        }
    }
    return stResult;
}

void CBestShotHub::endTarget(int nTrackId)
{
    m_manager.onTargetEnd(nTrackId);
    for (IFaceBestShotListener *pListener : m_vListeners)
    {
        pListener->onTrackEnded(nTrackId);
    }
}

std::vector<int64_t> CBestShotHub::expireAll(long long llNowMs)
{
    const std::vector<int64_t> vecExpired = m_manager.expireAll(llNowMs);
    for (const int64_t nTrackId : vecExpired)
    {
        for (IFaceBestShotListener *pListener : m_vListeners)
        {
            pListener->onTrackEnded(static_cast<int>(nTrackId));
        }
    }
    return vecExpired;
}

void CBestShotHub::clearAll()
{
    m_manager.clearAll();
    for (IFaceBestShotListener *pListener : m_vListeners)
    {
        pListener->onAllCleared();
    }
}
