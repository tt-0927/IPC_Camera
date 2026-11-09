/**
 * @FilePath     : phone_recheck.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 10:06:24
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 10:06:24
 * @Description  : 本文件全部是纯函数，不碰设备、不碰线程、不碰模型
 */

#include "phone_recheck.h"

#include <algorithm>

namespace Classroom
{
namespace
{
bool phoneCandBefore(const CPhoneCand_S &stA, const CPhoneCand_S &stB)
{
    if (stA.llLastMs != stB.llLastMs)
    {
        return stA.llLastMs < stB.llLastMs;
    }
    if (stA.fArea != stB.fArea)
    {
        return stA.fArea > stB.fArea;
    }
    /* 两个键都相同时按行号定序：与姿态段同一理由——不确定序会让同分的人每拍换人，
       而"复核结论一会有一会没"正是时长门最讨厌的抖动形状 */
    return stA.nRow < stB.nRow;
}

bool inIdList(const std::vector<int32_t> &vecIds, int32_t nId)
{
    if (nId < 0)
    {
        return false;
    }
    for (const int32_t nWant : vecIds)
    {
        if (nWant == nId)
        {
            return true;
        }
    }
    return false;
}
}    // namespace

const CPhoneCacheEntry_S *phoneCacheFind(const std::vector<CPhoneCacheEntry_S> &vecCache, int32_t nTrackId)
{
    if (nTrackId < 0)
    {
        return nullptr;
    }
    for (const CPhoneCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId == nTrackId)
        {
            return &stEntry;
        }
    }
    return nullptr;
}

bool phoneCacheUpsert(std::vector<CPhoneCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                      int32_t nTrackId, bool bValid, bool bFound, float fScore, int64_t llPhoneMs)
{
    if (nTrackId < 0)
    {
        return false;
    }

    /* bFound 与 fScore 只在 bValid 时可信：把一次失败的推理记成「没看到手机」是否决一条
       真行为的伪依据，而 bValid=false 的行根本不会被 applyPhoneCache 抄进特征行 */
    const bool bKeepFound = (bValid && bFound);

    for (CPhoneCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId == nTrackId)
        {
            stEntry.bValid = bValid;
            stEntry.bFound = bKeepFound;
            stEntry.fScore = bKeepFound ? fScore : 0.0f;
            stEntry.llPhoneMs = llPhoneMs;
            return true;
        }
    }
    for (CPhoneCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId < 0)
        {
            stEntry.nTrackId = nTrackId;
            stEntry.bValid = bValid;
            stEntry.bFound = bKeepFound;
            stEntry.fScore = bKeepFound ? fScore : 0.0f;
            stEntry.llPhoneMs = llPhoneMs;
            ++rnCapInUse;
            return true;
        }
    }
    return false;
}

int32_t phoneCachePrune(std::vector<CPhoneCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                        int64_t llNowMs, int32_t nTtlMs)
{
    int32_t nCleared = 0;
    for (CPhoneCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId < 0)
        {
            continue;
        }
        if (stEntry.llPhoneMs <= 0 || (llNowMs - stEntry.llPhoneMs) > nTtlMs)
        {
            stEntry = CPhoneCacheEntry_S();
            ++nCleared;
        }
    }

    /* 同 poseCachePrune：计数被外层复位过时取非负兜底，否则「表满」这条判据会被永久关掉 */
    rnCapInUse = (rnCapInUse < nCleared) ? 0 : (rnCapInUse - nCleared);
    return nCleared;
}

int32_t applyPhoneCache(CPersonFeature_S *patRows, int32_t nRowNum,
                        const std::vector<CPhoneCacheEntry_S> &vecCache,
                        int64_t llNowMs, int64_t llTtlMs)
{
    if (nullptr == patRows || nRowNum <= 0)
    {
        return 0;
    }

    int32_t nHit = 0;
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPhoneCacheEntry_S *pstEntry = phoneCacheFind(vecCache, patRows[nRow].nPersonId);
        if (nullptr == pstEntry || !pstEntry->bValid || pstEntry->llPhoneMs <= 0 ||
            (llNowMs - pstEntry->llPhoneMs) > llTtlMs)
        {
            /* 不动这一行：规则层据此走「只有规则证据」那条分支。bValid=false 与「查不到」
               在这里是同一种处理——前者是"试过但没拿到可判的图"，拿它当否证就把它变成了后者
               永远不该承担的角色。这里给 bPhoneObj=false 而不给 llPhoneMs=0 是**反的**做法——
               那会让每一个没轮到复核的人都被判成「模型说他没玩」，106 直接归零 */
            continue;
        }

        patRows[nRow].bPhoneObj = pstEntry->bFound;
        patRows[nRow].fPhoneScore = pstEntry->fScore;
        patRows[nRow].llPhoneMs = pstEntry->llPhoneMs;
        ++nHit;
    }
    return nHit;
}

size_t selectPhoneTargets(const CPersonFeature_S *patRows, int32_t nRowNum,
                          const std::vector<int32_t> &vecWantIds,
                          const std::vector<CPhoneCacheEntry_S> &vecCache,
                          const CPhoneSelectParam_S &rstParam,
                          std::vector<CPhoneCand_S> &rstScratch,
                          std::vector<int32_t> &rvecOutRow,
                          CPhoneSelectStat_S *pstStat)
{
    rstScratch.clear();
    rvecOutRow.clear();
    /* 每次调用先清零：跨拍累计是调用方的事，这里若累加，heartbeat 的读数会随
       「这段跑了多少圈」漂移，而它要回答的是本拍的名单去哪了 */
    CPhoneSelectStat_S stDiscard;
    CPhoneSelectStat_S &rstStat = (nullptr != pstStat) ? *pstStat : stDiscard;
    rstStat = CPhoneSelectStat_S();
    if (nullptr == patRows || nRowNum <= 0 || rstParam.nWantNum <= 0 || vecWantIds.empty())
    {
        return 0;
    }

    const int64_t llInterval = (rstParam.nIntervalMs > 0) ? rstParam.nIntervalMs : 1;

    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPersonFeature_S &stRow = patRows[nRow];

        if (!inIdList(vecWantIds, stRow.nPersonId))
        {
            continue;    /* 名单是规则层给的，感知层不认识「低头」这个判据 */
        }

        /* 遮挡与贴边的行不做复核：这两类裁剪出来的图**不足以支撑一次否证**。
           判「有手机」时错框最多让一次确认落在邻座身上（还有低头这条合取闸挡着），
           判「没有手机」时却是拿一张缺了半身的图去否决一个人的行为——不对称，
           而否证恰恰是这一步唯一的新增价值，脏了的否证比没有否证更糟 */
        if (stRow.bOccluded || stRow.bOnBoundary)
        {
            ++rstStat.nSkipOccluded;
            continue;
        }

        CPhoneCand_S stCand;
        stCand.nRow = nRow;
        stCand.fArea = stRow.stBodyBox.area();
        if (stCand.fArea <= 0.0f)
        {
            ++rstStat.nSkipBox;
            continue;    /* 没有可用的体框就裁不出图，占着名额也做不成 */
        }

        /* 后排那种十几像素的小框不参与复核：裁出来放大到模型输入那么大的图，模型看到的是糊
           而不是"手里没有东西"，它返回 0 个框就会被记成一条否决——而这一拍唯一的新增价值
           正是否决。拿不定主意时宁可不看（结构体字段注释里有完整推导） */
        if (rstParam.nMinBodyEdge > 0 &&
            (stRow.stBodyBox.width() < static_cast<float>(rstParam.nMinBodyEdge) ||
             stRow.stBodyBox.height() < static_cast<float>(rstParam.nMinBodyEdge)))
        {
            ++rstStat.nSkipBox;
            continue;
        }

        const CPhoneCacheEntry_S *pstEntry = phoneCacheFind(vecCache, stRow.nPersonId);
        stCand.llLastMs = (nullptr != pstEntry) ? pstEntry->llPhoneMs : 0;

        if (stCand.llLastMs > 0 && rstParam.llNowMs - stCand.llLastMs < llInterval)
        {
            ++rstStat.nSkipInterval;
            continue;    /* 刚复核过，这一拍让给还没做过的人 */
        }
        rstScratch.push_back(stCand);
    }

    if (rstScratch.empty())
    {
        return 0;
    }

    std::sort(rstScratch.begin(), rstScratch.end(), phoneCandBefore);

    const size_t nTake = (rstScratch.size() < static_cast<size_t>(rstParam.nWantNum))
                             ? rstScratch.size() : static_cast<size_t>(rstParam.nWantNum);
    for (size_t nIdx = 0; nIdx < nTake; ++nIdx)
    {
        rvecOutRow.push_back(rstScratch[nIdx].nRow);
    }
    return nTake;
}

int32_t phoneYesScoreBucket(float fScore)
{
    if (!(fScore >= 0.3f))   /* NaN 也走这里：比较对 NaN 恒假，坏读数落 b0 而不是消失 */
    {
        return 0;
    }
    if (fScore >= 0.9f)
    {
        return PHONE_YES_BUCKET_NUM - 1;
    }
    return 1 + (static_cast<int32_t>(fScore * 10.0f) - 3);
}
}
