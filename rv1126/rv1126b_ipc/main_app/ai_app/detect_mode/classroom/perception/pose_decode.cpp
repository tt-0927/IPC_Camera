/**
 * @FilePath     : pose_decode.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-15 10:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-15 10:30:00
 * @Description  : 姿态解码与轮转挑选实现
 */

#include "pose_decode.h"

#include <algorithm>
#include <cmath>

#include "dlog.h"

namespace Classroom
{
namespace
{
/* 躯干类关键点的索引区间：双肩(5) 到 臀中点(19)。
   面部 0~4 被排除在有效性闸门之外 */
constexpr int32_t TRUNK_FIRST = static_cast<int32_t>(PosePoint_E::POSE_R_SHOULDER);
constexpr int32_t TRUNK_LAST  = static_cast<int32_t>(PosePoint_E::POSE_HIP);

inline bool isTrunkPoint(int32_t nIdx)
{
    return nIdx >= TRUNK_FIRST && nIdx <= TRUNK_LAST;
}

/* 取缓存里某人的上次姿态时刻；没做过记 0（0 在排序里天然是「最该做」） */
int64_t lastPoseMsOf(const std::vector<CPoseCacheEntry_S> &vecCache, int32_t nTrackId)
{
    const CPoseCacheEntry_S *pstEntry = poseCacheFind(vecCache, nTrackId);
    return (nullptr != pstEntry) ? pstEntry->llPoseMs : 0;
}

/* 规则层紧急度查表：线性扫（≤60 条）。没给表/没这人 = 0（没人等），不是「不知道」 */
int32_t urgencyHintOf(const CPoseSelectParam_S &rstParam, int32_t nPersonId)
{
    if (nullptr == rstParam.pastUrgency || nPersonId < 0)
    {
        return 0;
    }
    int32_t nNum = rstParam.nUrgencyNum;
    if (nNum > CLASSROOM_MAX_PERSONS)
    {
        nNum = CLASSROOM_MAX_PERSONS;
    }
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (rstParam.pastUrgency[i].nPersonId == nPersonId)
        {
            return rstParam.pastUrgency[i].nUrgency;
        }
    }
    return 0;
}

/* 比较器：紧急度降序为主键，同档内最久没做优先，再按体框面积大的优先。
   「档内公平」刻意保留为第二键而不是把面积提前：紧急度是粗分档，
   平时最大的一档就是「没人加急」的全部 60 人，面积一提前后排立刻回到
   整节课没有骨骼的老坑（见 testSelectFairness 守的那条线） */
bool poseCandBefore(const CPoseCand_S &stA, const CPoseCand_S &stB)
{
    if (stA.nUrgency != stB.nUrgency)
    {
        return stA.nUrgency > stB.nUrgency;
    }
    if (stA.llLastMs != stB.llLastMs)
    {
        return stA.llLastMs < stB.llLastMs;
    }
    if (stA.fArea != stB.fArea)
    {
        return stA.fArea > stB.fArea;
    }
    /* 两个键都相同时按行号定序：不确定序的排序在同分人员之间会每拍换人，
       现象是「两个人的骨骼交替出现」，而这正是时长门最讨厌的抖动 */
    return stA.nRow < stB.nRow;
}
}

bool decodePose(const std::vector<float> &vecRaw, const CDetMap &rstMap,
                const CPoseDecodeParam_S &rstParam, CPoseSkeleton_S &rstOut,
                CPoseDecodeStat_S &rstStat)
{
    ++rstStat.u64Decoded;

    rstOut = CPoseSkeleton_S();    /* 整副复位：解不出来时不许留上一次的点 */

    const int32_t nPointNum = rstParam.nKeypointNum;
    if (nPointNum <= 0 || nPointNum > CLASSROOM_POSE_KEYPOINT_NUM)
    {
        /* 容量之外属于装配期缺陷：拓扑是模型定的，配置能改的只有分数门 */
        dlog_error("classroom pose: 关键点容量非法 %d (max %d)", nPointNum,
                   CLASSROOM_POSE_KEYPOINT_NUM);
        ++rstStat.u64TooShort;
        return false;
    }

    const size_t nNeed = static_cast<size_t>(nPointNum) * CLASSROOM_POSE_POINT_FLOAT_NUM;
    if (vecRaw.size() < nNeed)
    {
        /* 后端给浮点数比拓扑要求的少：模型换了输出头，或适配器打平时漏了一路。
           这里绝不解一半算一副——半副骨骼的点会来自两个不同的头 */
        ++rstStat.u64TooShort;
        return false;
    }
    if (!rstMap.isReady())
    {
        ++rstStat.u64MapNotReady;
        return false;
    }

    int32_t nTrunkUsable = 0;
    float   fScoreSum = 0.0f;
    int32_t nScoreNum = 0;

    for (int32_t nIdx = 0; nIdx < nPointNum; ++nIdx)
    {
        const size_t nBase = static_cast<size_t>(nIdx) * CLASSROOM_POSE_POINT_FLOAT_NUM;
        const float fRawX = vecRaw[nBase + 0];
        const float fRawY = vecRaw[nBase + 1];
        const float fScore = vecRaw[nBase + 2];

        if (!std::isfinite(fRawX) || !std::isfinite(fRawY) || !std::isfinite(fScore))
        {
            /* 一次 NaN 会让所有几何判据同时不确定，而它通常说明喂进去的图本身坏了：
               单独计数而不是混进「解不出来」，是为了让「模型输出坏了」这件事有条读数可对 */
            ++rstStat.u64NotFinite;
            return false;
        }

        /* 模型像素 → 分析帧：全链路唯一的换算点（§4.8）。CDetMap 内部会做越界钳制，
           所以画面外的手腕会落在画面边上，而不是造出一个负坐标去骗过几何判据 */
        const CPointf_S stCanvas = rstMap.modelToCanvas(CPointf_S{ fRawX, fRawY });

        rstOut.astPoints[nIdx].fX = stCanvas.fX;
        rstOut.astPoints[nIdx].fY = stCanvas.fY;
        rstOut.astPoints[nIdx].fScore = fScore;

        if (fScore >= rstParam.fKeyScore)
        {
            ++nScoreNum;
            fScoreSum += fScore;
            if (isTrunkPoint(nIdx))
            {
                ++nTrunkUsable;
            }
        }
    }

    rstOut.fScore = (nScoreNum > 0) ? (fScoreSum / static_cast<float>(nScoreNum)) : 0.0f;

    /* 耳证据普查（#65 第一刀，**只计数不改任何判据出口**，理由见 CPoseDecodeStat_S 那两条字段）。
       分母是"点已完整解出"的那些副：三个早退分支（TooShort / MapNotReady / NotFinite）在上面就 return 了，
       它们连点都没有，算进去会让比值失去意义；而躯干不达标（WeakTrunk）的副**要**算，
       因为本数问的从来不是这副骨骼能不能用，是耳点这三下能不能用 */
    if (nPointNum > static_cast<int32_t>(PosePoint_E::POSE_L_EAR))
    {
        const int32_t nNose = static_cast<int32_t>(PosePoint_E::POSE_NOSE);
        const int32_t nREar = static_cast<int32_t>(PosePoint_E::POSE_R_EAR);
        const int32_t nLEar = static_cast<int32_t>(PosePoint_E::POSE_L_EAR);
        if (rstOut.astPoints[nNose].fScore >= rstParam.fKeyScore &&
            rstOut.astPoints[nREar].fScore >= rstParam.fKeyScore &&
            rstOut.astPoints[nLEar].fScore >= rstParam.fKeyScore)
        {
            ++rstStat.u64EarUsable;
            /* 图像坐标 y 向下，"鼻高于双耳"= 鼻的 y 小于两侧耳的 y（严格小于：同高不算仰头，
               正对镜头站直时鼻与耳本就几乎同高，取 <= 会把一整节课的直立人都算成候选） */
            if (rstOut.astPoints[nNose].fY < rstOut.astPoints[nREar].fY &&
                rstOut.astPoints[nNose].fY < rstOut.astPoints[nLEar].fY)
            {
                ++rstStat.u64EarNoseAbove;
            }
        }
    }

    /* 文档口径普查（#65 第三刀，**只计数不改任何判据出口**，字段含义见 CPoseDecodeStat_S）。
       分母同第一刀的道理：三个早退分支在上面已 return；躯干不达标的副**要**算——
       文档几何只用头颈五点，跟躯干够不够没有关系。
       几何完全照文档原文：d1=眼中点到耳线的距离，d2=颈点到耳线的距离，d1/d2>0.2；
       同侧=低头形状，异侧=抬头形状。点到直线的有向距离共享 |B-A| 因子，
       比值直接用叉积模长之比，不用除；侧别看叉积符号，左右耳谁当 A 不影响结论
       （交换 A、B 两条叉积同时变号）。阈值 0.2 是文档自己的默认值
       （原文标注"可以设置的阈值"），普查不为此开配置键。 */
    if (nPointNum > static_cast<int32_t>(PosePoint_E::POSE_NECK))
    {
        const int32_t nLEye = static_cast<int32_t>(PosePoint_E::POSE_L_EYE);
        const int32_t nREye = static_cast<int32_t>(PosePoint_E::POSE_R_EYE);
        const int32_t nREar = static_cast<int32_t>(PosePoint_E::POSE_R_EAR);
        const int32_t nLEar = static_cast<int32_t>(PosePoint_E::POSE_L_EAR);
        const int32_t nNeck = static_cast<int32_t>(PosePoint_E::POSE_NECK);
        if (rstOut.astPoints[nLEye].fScore >= rstParam.fKeyScore &&
            rstOut.astPoints[nREye].fScore >= rstParam.fKeyScore &&
            rstOut.astPoints[nREar].fScore >= rstParam.fKeyScore &&
            rstOut.astPoints[nLEar].fScore >= rstParam.fKeyScore &&
            rstOut.astPoints[nNeck].fScore >= rstParam.fKeyScore)
        {
            ++rstStat.u64EarLineUsable;
            const float fABx = rstOut.astPoints[nREar].fX - rstOut.astPoints[nLEar].fX;
            const float fABy = rstOut.astPoints[nREar].fY - rstOut.astPoints[nLEar].fY;
            /* 简并耳线（双耳几乎重合，坏模型输出那一副）：叉积全长在数值噪声里，
               侧别与 d1/d2 都没有意义 ⇒ 只记分母、形状一律不记 */
            if (fABx * fABx + fABy * fABy > 1.0f)
            {
                const float fEyeMidX = 0.5f * (rstOut.astPoints[nLEye].fX + rstOut.astPoints[nREye].fX);
                const float fEyeMidY = 0.5f * (rstOut.astPoints[nLEye].fY + rstOut.astPoints[nREye].fY);
                const float fCrossEye = fABx * (fEyeMidY - rstOut.astPoints[nLEar].fY) -
                                        fABy * (fEyeMidX - rstOut.astPoints[nLEar].fX);
                const float fCrossNeck = fABx * (rstOut.astPoints[nNeck].fY - rstOut.astPoints[nLEar].fY) -
                                         fABy * (rstOut.astPoints[nNeck].fX - rstOut.astPoints[nLEar].fX);
                /* 侧别容下"正好在耳线上"（叉积=0）：正对镜头的人颈点可能恰好落在线上，
                   那一副既非抬头形状也非低头形状，两边都不记——记进去两边都虚高 */
                const int nEyeSide = (fCrossEye > 0.0f) - (fCrossEye < 0.0f);
                const int nNeckSide = (fCrossNeck > 0.0f) - (fCrossNeck < 0.0f);
                if (0 != nEyeSide && 0 != nNeckSide)
                {
                    if (std::fabs(fCrossEye) / std::fabs(fCrossNeck) > 0.2f)
                    {
                        if (nEyeSide != nNeckSide)
                        {
                            ++rstStat.u64DocUpShape;
                        }
                        else
                        {
                            ++rstStat.u64DocDownShape;
                        }
                    }
                }
            }
        }
    }

    if (nTrunkUsable < rstParam.nMinTrunkPoints)
    {
        /* 点仍然填好（调试绘制要用），只是 bValid=false 让规则层不得据此改状态。
           这一档不是错误：远处后排在逆光下常常就是解不出躯干，属于「证据不足」 */
        ++rstStat.u64WeakTrunk;
        return true;
    }

    rstOut.bValid = true;
    ++rstStat.u64Usable;
    return true;
}

const CPoseCacheEntry_S *poseCacheFind(const std::vector<CPoseCacheEntry_S> &vecCache, int32_t nTrackId)
{
    if (nTrackId < 0)
    {
        return nullptr;
    }
    for (const CPoseCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId == nTrackId)
        {
            return &stEntry;
        }
    }
    return nullptr;
}

bool poseCacheUpsert(std::vector<CPoseCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                     int32_t nTrackId, const CPoseSkeleton_S &rstSkl, int64_t llPoseMs)
{
    if (nTrackId < 0)
    {
        return false;
    }

    for (CPoseCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId == nTrackId)
        {
            stEntry.stSkl = rstSkl;
            stEntry.stSkl.nPersonId = nTrackId;
            stEntry.llPoseMs = llPoseMs;
            return true;
        }
    }
    for (CPoseCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId < 0)
        {
            stEntry.nTrackId = nTrackId;
            stEntry.stSkl = rstSkl;
            stEntry.stSkl.nPersonId = nTrackId;
            stEntry.llPoseMs = llPoseMs;
            ++rnCapInUse;
            return true;
        }
    }
    return false;
}

int32_t poseCachePrune(std::vector<CPoseCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                       int64_t llNowMs, int32_t nTtlMs)
{
    int32_t nCleared = 0;
    for (CPoseCacheEntry_S &stEntry : vecCache)
    {
        if (stEntry.nTrackId < 0)
        {
            continue;
        }
        if (stEntry.llPoseMs <= 0 || (llNowMs - stEntry.llPoseMs) > nTtlMs)
        {
            stEntry = CPoseCacheEntry_S();
            ++nCleared;
        }
    }

    /* 计数被外层复位过（换场、配置重启）时会比真清掉的少，取非负兜底：
       让已用数翻负就等于把「表满」这条判据永久关掉 */
    rnCapInUse = (rnCapInUse < nCleared) ? 0 : (rnCapInUse - nCleared);
    return nCleared;
}

int32_t applyPoseCache(CPersonFeature_S *patRows, int32_t nRowNum,
                       const std::vector<CPoseCacheEntry_S> &vecCache,
                       int64_t llNowMs, int64_t llTtlMs)
{
    if (nullptr == patRows || nRowNum <= 0)
    {
        return 0;
    }

    int32_t nHit = 0;
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPoseCacheEntry_S *pstEntry = poseCacheFind(vecCache, patRows[nRow].nPersonId);
        if (nullptr == pstEntry || pstEntry->llPoseMs <= 0 ||
            (llNowMs - pstEntry->llPoseMs) > llTtlMs)
        {
            /* 不动这一行：槽位借出时已整行清零，此处保持 bValid=false、llPoseMs=0 即可。
               刻意不「留着一副旧的」——规则层与这里的 TTL 口径必须同值，
               留下来的那副就成了只有感知层认、判定层不认的假证据 */
            continue;
        }

        patRows[nRow].stSkeleton = pstEntry->stSkl;
        patRows[nRow].llPoseMs = pstEntry->llPoseMs;
        ++nHit;
    }
    return nHit;
}

size_t selectPoseTargets(const CPersonFeature_S *patRows, int32_t nRowNum,
                         const std::vector<CPoseCacheEntry_S> &vecCache,
                         const CPoseSelectParam_S &rstParam,
                         std::vector<CPoseCand_S> &rstScratch,
                         std::vector<int32_t> &rvecOutRow)
{
    rstScratch.clear();
    rvecOutRow.clear();
    if (nullptr == patRows || nRowNum <= 0 || rstParam.nWantNum <= 0)
    {
        return 0;
    }

    const int64_t llInterval = (rstParam.nIntervalMs > 0) ? rstParam.nIntervalMs : 1;

    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPersonFeature_S &stRow = patRows[nRow];

        if (rstParam.bSkipOccluded && stRow.bOccluded)
        {
            continue;    /* 下游对遮挡行整行给 SR_OCCLUDED，做了也是白做 */
        }
        if (rstParam.fBlurMin > 0.0f && stRow.fBlurScore > 0.0f &&
            stRow.fBlurScore < poseBlurMinForHeadPx(rstParam.fBlurMin, stRow.stHeadBox.height()))
        {
            continue;    /* fBlurScore==0 是「没量出来」而不是「糊到零」，与规则层同一判法；
                            门按头高缩放（#91）：小头框梯度能量天然低，不缩放远带整排被当糊丢掉 */
        }

        CPoseCand_S stCand;
        stCand.nRow = nRow;
        stCand.llLastMs = lastPoseMsOf(vecCache, stRow.nPersonId);
        stCand.fArea = stRow.stBodyBox.area();
        stCand.nUrgency = urgencyHintOf(rstParam, stRow.nPersonId);

        /* 证据临近过期 → 提前续期一档：别等它掉成 SR_NO_POSE 再从「没证据」重新排队，
           那一拍里门里已攒的时长正被容忍额度消耗。只在没人比这更急时生效——
           规则层 600 以上的分已经说明「现在就要」，过期与否不再是主要矛盾。
           过了 TTL 的行也停在 500：已过期的证据该不该补由规则层决定，
           这里只负责「还救得回来」的那一段。
           只抬不压：规则层教师候选档（550）也在 <600 的区间里，压到 500 会让
           「谁先续」在教师与普通人之间重新掷硬币——那 550 就白发了 */
        if (stCand.nUrgency < CLASSROOM_POSE_URG_ACC_BASE &&
            rstParam.nPoseTtlMs > 0 && stCand.llLastMs > 0)
        {
            const int64_t llAgeMs = rstParam.llNowMs - stCand.llLastMs;
            if (llAgeMs > (static_cast<int64_t>(rstParam.nPoseTtlMs) *
                           CLASSROOM_POSE_URG_STALE_PERMILLE) / 1000 &&
                stCand.nUrgency < CLASSROOM_POSE_URG_EXPIRE)
            {
                stCand.nUrgency = CLASSROOM_POSE_URG_EXPIRE;
            }
        }

        /* 单人周期闸：刚做过的人本拍让位。最高一档例外——生效中的行为正靠容忍额度撑，
           等一个周期的代价比多花一次推理大；教师档同享例外（T29）：550 这个提示本身
           就是「骨骼已过 TTL 七成」的产物，sampling.pose_interval_ms 若配得比这年龄大，
           这道闸会把规则层点名最急的人挡在门外——提示与闸互相打架 */
        if (stCand.nUrgency < CLASSROOM_POSE_URG_ACTIVE &&
            CLASSROOM_POSE_URG_TEACHER != stCand.nUrgency &&
            stCand.llLastMs > 0 && rstParam.llNowMs - stCand.llLastMs < llInterval)
        {
            continue;    /* 刚做过，这一拍让给还没做的人 */
        }
        rstScratch.push_back(stCand);
    }

    if (rstScratch.empty())
    {
        return 0;
    }

    /* #92 插队限席：紧急度是插队键（越过「最久没做优先」的公平队列），一拍至多放一人插队。
       降频/卡顿恢复后的第一拍，全场可能几十人同时挂着高分（生效中的都挂 ACTIVE 档），
       不限席则全部名额被他们包场，没挂分的后排又回到整节课没有骨骼的老坑。
       保席者按比较器序取「最急、同档最久、面积大」的那一个，其余人清零回公平队列重排。
       教师席（T29，ttea3 读数：90s 授课素材 206 讲授只出 1 条 5s 事件）：44 行 ÷ 2 席的
       公平轮转 ≈ 6.5s ≫ TTL 2s，单一插队席又被 600+/1000 优先——教师档一让位就要等
       一整圈，骨骼断档远超 TTL，背身判据 UNKNOWN 为主，教师全列（206/207/208/210）
       凑不齐最小时长。给教师档单独一席：本拍至多「插队席 + 教师席」两个保席者，
       防包场的语义不变（多候选时教师席也只保最急的一个，其余清零回公平队列） */
    int32_t nSeat = -1;
    int32_t nLaneSeat = -1;
    for (size_t i = 0; i < rstScratch.size(); ++i)
    {
        if (rstScratch[i].nUrgency <= 0)
        {
            continue;
        }
        if (CLASSROOM_POSE_URG_TEACHER == rstScratch[i].nUrgency)
        {
            if (nLaneSeat < 0 || poseCandBefore(rstScratch[i], rstScratch[static_cast<size_t>(nLaneSeat)]))
            {
                nLaneSeat = static_cast<int32_t>(i);
            }
            continue;
        }
        if (nSeat < 0 || poseCandBefore(rstScratch[i], rstScratch[static_cast<size_t>(nSeat)]))
        {
            nSeat = static_cast<int32_t>(i);
        }
    }
    if (nSeat >= 0 || nLaneSeat >= 0)
    {
        for (size_t i = 0; i < rstScratch.size(); ++i)
        {
            if (static_cast<int32_t>(i) == nSeat || static_cast<int32_t>(i) == nLaneSeat)
            {
                continue;
            }
            rstScratch[i].nUrgency = 0;
        }
    }

    std::sort(rstScratch.begin(), rstScratch.end(), poseCandBefore);

    const size_t nTake = (rstScratch.size() < static_cast<size_t>(rstParam.nWantNum))
                             ? rstScratch.size() : static_cast<size_t>(rstParam.nWantNum);
    for (size_t nIdx = 0; nIdx < nTake; ++nIdx)
    {
        rvecOutRow.push_back(rstScratch[nIdx].nRow);
    }
    return nTake;
}
}
