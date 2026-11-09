/**
 * @FilePath     : face_feature.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-21 9:33:06
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 9:33:06
 * @Description  : 人脸特征表实现（纯函数，见 face_feature.h 的文件头纪律）
 */

#include "face_feature.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Classroom
{
namespace
{
/* 候选排序：先「最久没做」，同一次数里再「脸大的优先」。
   写成比较函数而不是 lambda 是给后面读代码的人一个可 grep 的名字（与 pose_decode 同法） */
bool featCandBefore(const CFaceFeatCand_S &stA, const CFaceFeatCand_S &stB)
{
    if (stA.llLastMs != stB.llLastMs)
    {
        return stA.llLastMs < stB.llLastMs;
    }
    return stA.fArea > stB.fArea;
}

/* 轮转键：上次「试过给他取特征」的时刻。0 = 从没做过（排在最前，这是冷启动能铺开的原因）。
   刻意取 llTryMs 而不是 llFeatMs：一个总在裁剪/对齐就失败的人如果不推进这个时刻，
   就会永久占住一个配额 */
int64_t lastTryMsOf(const std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId)
{
    const int32_t nIdx = faceFeatIndexOf(vecTab, nTrackId);
    return (nIdx < 0) ? 0 : vecTab[static_cast<size_t>(nIdx)].llTryMs;
}
}

int32_t faceFeatIndexOf(const std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId)
{
    if (nTrackId < 0)
    {
        return -1;    /* <0 是"没有跟踪 ID"的约定值，与空槽同键：让它命中空槽等于把无主特征当有主 */
    }
    for (size_t i = 0; i < vecTab.size(); ++i)
    {
        if (vecTab[i].nTrackId == nTrackId)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

bool faceFeatUpsert(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                    const float *pafFeat, int32_t nDim, int64_t llFeatMs,
                    CFaceFeatStat_S &rstStat)
{
    if (nullptr == pafFeat || nDim <= 0 || nTrackId < 0)
    {
        ++rstStat.u64RejectNull;
        return false;
    }
    if (nDim > CLASSROOM_FACE_FEAT_MAX_DIM)
    {
        /* 整行不写而不是截断：截断后的向量与底库全维度点积算出来的是一个"看着像相似度"的数，
           它会以略低于门限的样子出现在读数里，让人去调门限而不是去改容量 */
        ++rstStat.u64RejectWide;
        return false;
    }

    int32_t nIdx = faceFeatIndexOf(vecTab, nTrackId);
    const bool bHadRow = (nIdx >= 0);
    if (nIdx < 0)
    {
        /* 先复用最靠前的空槽。装不下就计数不扩容：表容量等于配置的人数上限，
           涨满本身就是 ID 洪泛的读数，悄悄长表会把这件事永久藏起来 */
        for (size_t i = 0; i < vecTab.size(); ++i)
        {
            if (vecTab[i].nTrackId < 0)
            {
                nIdx = static_cast<int32_t>(i);
                break;
            }
        }
    }
    if (nIdx < 0)
    {
        ++rstStat.u64RejectFull;
        return false;
    }

    CFaceFeatEntry_S &stEntry = vecTab[static_cast<size_t>(nIdx)];
    /* 覆盖式重置：比对侧残留一份旧结论就等于"新脸配旧名字" */
    stEntry.bHasMatch = false;
    stEntry.bMatchTaken = false;
    stEntry.llMatchMs = 0;
    stEntry.stMatch = CFaceMatch_S();

    stEntry.nTrackId = nTrackId;
    stEntry.llFeatMs = llFeatMs;
    /* 轮转键与证据时刻在同一次成功里一起推进：只写 llFeatMs 的话这个人下一拍又被选中，
       配额白烧一次，而底库多一次拿同一张脸凑出第二票的机会（见文件头的两票口径） */
    stEntry.llTryMs = llFeatMs;
    /* 尝试次数与轮转键在同一次成功里一起推进（饱和不回绕：真到 INT32_MAX 时
       这个人早已不是"没铺开"的候选，回绕反而会让他重新触发预热全速） */
    stEntry.nTryNum = bHadRow ? ((stEntry.nTryNum < INT32_MAX) ? stEntry.nTryNum + 1 : INT32_MAX) : 1;
    stEntry.nDim = nDim;
    std::copy(pafFeat, pafFeat + nDim, stEntry.afFeat);
    ++rstStat.u64Wrote;
    return true;
}

bool faceFeatStampAttempt(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                          int64_t llNowMs, CFaceFeatStat_S &rstStat)
{
    if (nTrackId < 0)
    {
        ++rstStat.u64RejectNull;    /* 没有 ID 就没有"下次再轮到他"这件事可记 */
        return false;
    }

    const int32_t nIdx = faceFeatIndexOf(vecTab, nTrackId);
    if (nIdx >= 0)
    {
        /* 只推轮转键与尝试次数：动 nDim 或动比对侧等于把"这一拍没做成"升级成
           "丢掉一份已经拿到的证据" */
        CFaceFeatEntry_S &stEntry = vecTab[static_cast<size_t>(nIdx)];
        stEntry.llTryMs = llNowMs;
        if (stEntry.nTryNum < INT32_MAX)
        {
            ++stEntry.nTryNum;
        }
        ++rstStat.u64Stamped;
        return true;
    }

    for (size_t i = 0; i < vecTab.size(); ++i)
    {
        CFaceFeatEntry_S &stEntry = vecTab[i];
        if (stEntry.nTrackId >= 0)
        {
            continue;
        }
        stEntry.nTrackId = nTrackId;
        stEntry.llFeatMs = 0;
        stEntry.llTryMs = llNowMs;
        stEntry.nTryNum = 1;
        stEntry.nDim = 0;
        /* 新槽必须和 upsert 一样清洗比对侧：这张表是复用的，不清洗等于把上一位的名字
           交给新来的人，而 faceFeatTake 看不出名字是谁给的 */
        stEntry.bHasMatch = false;
        stEntry.bMatchTaken = false;
        stEntry.llMatchMs = 0;
        stEntry.stMatch = CFaceMatch_S();
        ++rstStat.u64Stamped;
        return true;
    }
    /* 装不下就不记：这一拍之后他仍是"从没做过"、仍排队首。看起来是坏结果，但它正是
       表满这件事唯一的读数出口——悄悄把标记丢掉会让 ID 洪泛表现为"人脸段在跑但没名字" */
    ++rstStat.u64RejectFull;
    return false;
}

int32_t faceFeatPrune(std::vector<CFaceFeatEntry_S> &vecTab, int64_t llNowMs, int32_t nTtlMs,
                      CFaceFeatStat_S &rstStat)
{
    if (nTtlMs <= 0)
    {
        return 0;    /* 配 0 = 不清表（合法但危险：表会长满旧 ID，故上限仍由容量兜住） */
    }
    int32_t nCut = 0;
    for (size_t i = 0; i < vecTab.size(); ++i)
    {
        CFaceFeatEntry_S &stEntry = vecTab[i];
        if (stEntry.nTrackId < 0)
        {
            continue;
        }
        /* 新鲜度基准按行取：有特征的行按 llFeatMs（这份证据还能不能用），只有尝试记录的行按
           llTryMs。统一按 llFeatMs 会让纯标记行因时刻为 0 在下一轮就被清掉——防饿死那笔白记，
           这个人又回到队首，读起来像"TTL 设得太短" */
        const int64_t llBase = (stEntry.nDim > 0) ? stEntry.llFeatMs : stEntry.llTryMs;
        /* 时刻倒退（NTP 校时、单调时钟本身不回退，但表可能来自上一份配置的新表）时
           差值为负，`(now-ts) > ttl` 因此判"没过期"——宁留一行旧特征，也不在时钟异常时清空整表。
           判据由 `>=` 改成 `>`，与消费方的 `faceUsable()` 对齐。
           `CPersonFeature_S::faceUsable()`（bus/feature_frame.h:129）判"这份脸还能不能用"
           用的是 `(now - llFaceMs) <= ttl` ⇒ 恰好等于 TTL 时它仍然可用。
           而这里原来用 `>= ttl` 清表 ⇒ 同一拍上"规则层还能用、数据已经没了"：
           考勤拿到一个刚被清掉的人脸，读数上是 pend 或 noface 多一次，
           而现场按 TTL 反推只会得出"TTL 设短了"的错误结论。
           现在是"恰好等于 TTL 仍可用，超过才清"，与 phoneUsable/phoneCachePrune 那对同口径。 */
        if (!((llNowMs - llBase) > nTtlMs))
        {
            continue;
        }
        /* 结论没被账本取走就随行清掉：这部分计入 u64PrunedPending 单独归因——
           u64MatchSet−u64Taken 的差既可能是"还在等"，也可能是"等的时候被 TTL 清了"，
           两条路在现场的处置完全不同（前者查节拍，后者查 TTL/人脸段是否停滞） */
        if (stEntry.bHasMatch && !stEntry.bMatchTaken)
        {
            ++rstStat.u64PrunedPending;
        }
        stEntry.nTrackId = -1;
        stEntry.llFeatMs = 0;
        stEntry.llTryMs = 0;
        stEntry.nTryNum = 0;
        stEntry.nDim = 0;
        stEntry.bHasMatch = false;
        stEntry.bMatchTaken = false;
        stEntry.llMatchMs = 0;
        stEntry.stMatch = CFaceMatch_S();
        ++nCut;
    }
    rstStat.u64Pruned += static_cast<uint64_t>(nCut);
    return nCut;
}

bool faceFeatNeedsMatch(const CFaceFeatEntry_S &stEntry)
{
    return (stEntry.nTrackId >= 0 && stEntry.nDim > 0 && !stEntry.bHasMatch);
}

bool faceFeatSetMatch(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                      const CFaceMatch_S &rstMatch, int64_t llNowMs, CFaceFeatStat_S &rstStat)
{
    const int32_t nIdx = faceFeatIndexOf(vecTab, nTrackId);
    if (nIdx < 0)
    {
        /* 比完回来发现行没了：只可能是这一拍之间特征被 TTL 清掉或表被换过。
           计数不重试——重试会在节拍里循环遍历底库 */
        ++rstStat.u64MatchMissRow;
        return false;
    }
    CFaceFeatEntry_S &stEntry = vecTab[static_cast<size_t>(nIdx)];
    stEntry.stMatch = rstMatch;
    stEntry.bHasMatch = true;
    stEntry.bMatchTaken = false;
    stEntry.llMatchMs = llNowMs;
    ++rstStat.u64MatchSet;
    return true;
}

bool faceFeatTake(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                  CFaceMatch_S &rstOut, CFaceFeatStat_S &rstStat)
{
    rstOut = CFaceMatch_S();
    const int32_t nIdx = faceFeatIndexOf(vecTab, nTrackId);
    if (nIdx < 0)
    {
        return false;
    }
    CFaceFeatEntry_S &stEntry = vecTab[static_cast<size_t>(nIdx)];
    if (!stEntry.bHasMatch || stEntry.bMatchTaken)
    {
        return false;
    }
    stEntry.bMatchTaken = true;
    rstOut = stEntry.stMatch;
    ++rstStat.u64Taken;
    return true;
}

int32_t faceFeatNeedMatchNum(const std::vector<CFaceFeatEntry_S> &vecTab)
{
    int32_t nNeed = 0;
    for (size_t i = 0; i < vecTab.size(); ++i)
    {
        if (faceFeatNeedsMatch(vecTab[i]))
        {
            ++nNeed;
        }
    }
    return nNeed;
}

int32_t faceFeatWantNum(bool bPerBeatThrottle, int32_t nPerBeatPersons, int32_t nMaxPersons,
                        const std::vector<CFaceDet_S> &vecFace,
                        const std::vector<int32_t> &vecTrackId,
                        const std::vector<CFaceFeatEntry_S> &vecTab)
{
    const int32_t nCap = clampValue(nMaxPersons, 1, CLASSROOM_MAX_PERSONS);
    const int32_t nPerBeat = clampValue(nPerBeatPersons, 1, CLASSROOM_MAX_PERSONS);
    /* 稳态值不许越过容量口径：配置层本来就钳 per_beat ≤ max_persons，这里再收一次口，
       免得运行期换了表之后节流档比不节流还多要（"节流反而更贵"这种读数最难解释） */
    const int32_t nSteady = (bPerBeatThrottle && nPerBeat < nCap) ? nPerBeat : nCap;
    if (nSteady >= nCap || vecFace.size() != vecTrackId.size())
    {
        /* 没开节流（稳态就是容量）⇒ 预热无事可做；两表不等是上游写错，
           挑选处会整批不做，这里跟着退回稳态，不额外发钱 */
        return nSteady;
    }

    /* 空槽是"这一拍真能给无行的人铺开一行"的前提。满表时不把无行的人算进 warm（见头注） */
    bool bHasFreeSlot = false;
    for (size_t i = 0; i < vecTab.size(); ++i)
    {
        if (vecTab[i].nTrackId < 0)
        {
            bHasFreeSlot = true;
            break;
        }
    }

    for (size_t f = 0; f < vecFace.size(); ++f)
    {
        const int32_t nTrackId = vecTrackId[f];
        if (nTrackId < 0)
        {
            continue;    /* 与挑选处同一口径：没配到人身上的脸不取特征 */
        }
        const int32_t nIdx = faceFeatIndexOf(vecTab, nTrackId);
        if (nIdx < 0)
        {
            if (bHasFreeSlot)
            {
                return nCap;
            }
            continue;
        }
        if (vecTab[static_cast<size_t>(nIdx)].nTryNum < CLASSROOM_FACE_WARM_TRIES)
        {
            return nCap;
        }
    }
    return nSteady;
}

size_t selectFaceFeatTargets(const std::vector<CFaceDet_S> &vecFace,
                             const std::vector<int32_t> &vecTrackId,
                             const std::vector<CFaceFeatEntry_S> &vecTab,
                             const CFaceFeatSelectParam_S &rstParam,
                             std::vector<CFaceFeatCand_S> &rstScratch,
                             std::vector<int32_t> &rvecOutIdx)
{
    rstScratch.clear();
    rvecOutIdx.clear();
    if (rstParam.nWantNum <= 0 || vecFace.size() != vecTrackId.size())
    {
        /* 两张表长度不等是上游写错了（人脸表与配行表不同步）。这里返回 0 而不是按下标少的那个走：
           少一个人脸只是这一拍不做考勤，错配一个 ID 会把张三的特征挂到李四头上 */
        return 0;
    }

    const int64_t llInterval = (rstParam.nMinIntervalMs > 0) ? rstParam.nMinIntervalMs : 0;

    for (size_t f = 0; f < vecFace.size(); ++f)
    {
        const int32_t nTrackId = vecTrackId[f];
        if (nTrackId < 0)
        {
            continue;    /* 没配到人身上的脸：取特征只能得到一份"不知道是谁"的向量，白烧一次 NPU */
        }

        CFaceFeatCand_S stCand;
        stCand.nFaceIdx = static_cast<int32_t>(f);
        stCand.llLastMs = lastTryMsOf(vecTab, nTrackId);
        stCand.fArea = vecFace[f].stBox.area();

        if (stCand.llLastMs > 0 && llInterval > 0 &&
            rstParam.llNowMs - stCand.llLastMs < llInterval)
        {
            continue;    /* 刚做过，这一拍让给还没做的人 */
        }
        rstScratch.push_back(stCand);
    }

    if (rstScratch.empty())
    {
        return 0;
    }

    std::sort(rstScratch.begin(), rstScratch.end(), featCandBefore);

    const size_t nTake = (rstScratch.size() < static_cast<size_t>(rstParam.nWantNum))
                             ? rstScratch.size() : static_cast<size_t>(rstParam.nWantNum);
    for (size_t nIdx = 0; nIdx < nTake; ++nIdx)
    {
        rvecOutIdx.push_back(rstScratch[nIdx].nFaceIdx);
    }
    return nTake;
}

const char *faceFeatStatText(const std::vector<CFaceFeatEntry_S> &vecTab,
                             const CFaceFeatStat_S &rstStat, char *pchBuf, int32_t nBufSize)
{
    if (nullptr == pchBuf || nBufSize <= 0)
    {
        return "";
    }
    int32_t nLive = 0;
    int32_t nPending = 0;
    int32_t nDimSeen = 0;
    for (size_t i = 0; i < vecTab.size(); ++i)
    {
        if (vecTab[i].nTrackId >= 0)
        {
            ++nLive;
            if (0 == nDimSeen && vecTab[i].nDim > 0)
            {
                /* 取第一个**真有特征**的行的维度。以前取第一个非空行就够（非空即有特征），
                   加了尝试标记行之后那些行 nDim==0，会把"维度到底是几百"报成 0——
                   而这条读数唯一要说的事就是它，报 0 会把人引去查模型而不是查表 */
                nDimSeen = vecTab[i].nDim;
            }
        }
        if (faceFeatNeedsMatch(vecTab[i]))
        {
            ++nPending;
        }
    }
    /* 串首不带段落标签：本函数是拼进心跳里 "|feat …" 那一段尾巴的，
       自带一个 feat 会让读的人先看到两个 feat 再看到 live */
    snprintf(pchBuf, static_cast<size_t>(nBufSize),
             "live=%d pend=%d dim=%d w=%llu try=%llu null=%llu wide=%llu full=%llu"
             " pruned=%llu pdrop=%llu set=%llu gone=%llu taken=%llu",
             nLive, nPending, nDimSeen,
             static_cast<unsigned long long>(rstStat.u64Wrote),
             static_cast<unsigned long long>(rstStat.u64Stamped),
             static_cast<unsigned long long>(rstStat.u64RejectNull),
             static_cast<unsigned long long>(rstStat.u64RejectWide),
             static_cast<unsigned long long>(rstStat.u64RejectFull),
             static_cast<unsigned long long>(rstStat.u64Pruned),
             static_cast<unsigned long long>(rstStat.u64PrunedPending),
             static_cast<unsigned long long>(rstStat.u64MatchSet),
             static_cast<unsigned long long>(rstStat.u64MatchMissRow),
             static_cast<unsigned long long>(rstStat.u64Taken));
    return pchBuf;
}

int32_t faceFeatIntervalMs(int32_t nConfigMs, int32_t nIntervalMs, int32_t nWantNum, int32_t nFaceNum)
{
    if (nConfigMs > 0)
    {
        return nConfigMs;
    }
    if (nIntervalMs <= 0 || nWantNum <= 0 || nFaceNum <= nWantNum)
    {
        /* 本拍就做得完（或这段压根没周期），不需要间隔：让挑选处按"无单人周期"处理 */
        return 0;
    }
    /* 轮转一圈所需时间 = 检测段周期 × 圈数。少这一条就会变成"前排每拍重做、后排永远排不到"，
       而它在读数上只是"名单里就那么几个人"，是考勤最难看出来的覆盖率问题 */
    const int32_t nRounds = (nFaceNum + nWantNum - 1) / nWantNum;
    const int64_t llMs = static_cast<int64_t>(nIntervalMs) * nRounds;
    return (llMs > INT32_MAX) ? INT32_MAX : static_cast<int32_t>(llMs);
}
}
