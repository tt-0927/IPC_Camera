/**
 * @FilePath     : subject_arbiter.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 14:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 14:10:00
 * @Description  : 主体归属仲裁实现（纯算术 + 定长槽池，逐拍路径无锁、无分配）
 */

#include "subject_arbiter.h"

#include <climits>
#include <cstdio>

#include "dlog.h"
#include "geometry.h"

namespace Classroom
{
namespace
{
/* 位移窗口最长累计到这么多毫秒就强制重启：配置把 patrol_window_ms 调大时，
   一个刚被抢占的槽位不该一次背上来历不明的长距离（换人后位置从画面左跳右）。
   配置层已把该键钳到同一上限，这一道是给**不经 validate 的手搓快照**留的兜底
   （单测里直接构造 CConfigSnapshot_S 是常态） */
#define CLASSROOM_MOVE_WINDOW_MAX (30000)

/* 两帧之间认作"同一次连续移动"的最大间隔倍数：超过就重新起窗，
   不把感知断档前后的两段位移加成一个"走了很远" */
#define CLASSROOM_MOVE_GAP_TIMES (3)

/* 走动保持的窗口倍数（#102 第六刀）：最近一次达门后的这段时间里，bMovedHold 仍为真。
   2×patrol_window_ms（5s 窗 → 10s）。只给 210 巡视用——真实巡视是"走几步、在桌边停
   几秒、再走"，瞬时 5s 窗把每次停驻都判成没走，5s 时长门永远攒不满 */
#define CLASSROOM_PATROL_HOLD_TIMES (2)

/* 位移的单步速度上限（px/s，2026-10-20 第八刀）：跨过漏检拍的那一步只有在这个速度内
   才认作「人走过去」。1080p 画面里教师快走约 150~300px/s，600 留了一倍余量；
   超过它的跳变是换人/换 ID/跟踪跳变，重锚位置、不进位移窗口 */
#define CLASSROOM_MOVE_MAX_SPEED_PX_S (600.0)

const char *subjectName(Subject_E enSubject)
{
    switch (enSubject)
    {
    case Subject_E::SUBJECT_STUDENT:
        return "student";
    case Subject_E::SUBJECT_TEACHER:
        return "teacher";
    case Subject_E::SUBJECT_UNKNOWN:
    default:
        return "unknown";
    }
}
}   // namespace

void CSubjectArbiter::reset()
{
    for (CSlot_S &stSlot : m_astSlot)
    {
        stSlot = CSlot_S();
    }
    /* 位移记忆是按槽位下标复用的环形表（MOVE_MEM_NUM 格），不随槽内容清空。
       上一轮的桶留在这里，下一轮同一槽位第一次接回时会拿它算「走了多远」——
       换班之后第一个人凭空继承上一班某人的位移，bMoved 直接判真。必须一起清 */
    for (CMoveMem_S &stMem : m_aMoveMem)
    {
        stMem = CMoveMem_S();
    }
    m_u32FlipHead = 0;
    m_u32FlipNum = 0;
    m_nTeacherNum = 0;
    m_nCandidateNum = 0;
    m_bPodiumEmpty = true;
    m_bTeacherAreaEmpty = true;
    m_bBoardConfigured = false;
    m_llPodiumWitnessMs = -1;
    for (CReentryMem_S &stMem : m_astReentry)
    {
        stMem = CReentryMem_S();
    }
    publishStat();   /* 统计计数由 resetStat() 负责（契约见 subject_arbiter.h：reset 只清状态） */
}

const CSubjectArbiter::CSlot_S *CSubjectArbiter::findSlot(int32_t nPersonId) const
{
    if (nPersonId < 0)
    {
        return nullptr;
    }
    for (const CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId == nPersonId)
        {
            return &stSlot;
        }
    }
    return nullptr;
}

CSubjectArbiter::CSlot_S *CSubjectArbiter::findSlot(int32_t nPersonId)
{
    return const_cast<CSlot_S *>(static_cast<const CSubjectArbiter *>(this)->findSlot(nPersonId));
}

CSubjectArbiter::CSlot_S &CSubjectArbiter::acquireSlot(int32_t nPersonId, int64_t llFrameMs)
{
    CSlot_S *pSlot = findSlot(nPersonId);
    if (nullptr != pSlot)
    {
        pSlot->llLastSeenMs = llFrameMs;
        return *pSlot;
    }

    int nFree = -1;
    int nOldest = -1;
    int64_t llOldestSeen = 0;
    for (size_t i = 0; i < CLASSROOM_MAX_PERSONS; ++i)
    {
        if (m_astSlot[i].nOwnerId < 0)
        {
            nFree = static_cast<int>(i);
            break;
        }
        /* 找最小值必须扫完整表：早退会让"第一个槽"冒充"最久未见的槽"。
           这里不能早退，而上面找空槽可以——空槽是谁的都一样 */
        if (nOldest < 0 || m_astSlot[i].llLastSeenMs < llOldestSeen)
        {
            nOldest = static_cast<int>(i);
            llOldestSeen = m_astSlot[i].llLastSeenMs;
        }
    }

    if (nFree < 0)
    {
        /* 走到这里说明同一拍里出现了超过槽数的人。回收时机是"本拍没出现即释放"，
           正常情况下不可能满，满了就是上游容量纪律被破坏：宁可有记录地挤掉最久未见的，
           也不要不记录地少认一个人——不记录时现场只会看到"这位老师从来没出现过" */
        ++m_u64SlotSteal;
        nFree = nOldest;
        const CSlot_S &cstVictim = m_astSlot[static_cast<size_t>(nFree)];
        saveMoveMem(cstVictim, llFrameMs);
        if (Subject_E::SUBJECT_TEACHER == cstVictim.enSubject)
        {
            /* 被挤掉的人正挂着教师身份：这条翻转必须发，且要计入 demote。
               他与 retireSlot 那条"人不在了不发翻转"的口径**不是一回事**——人就在本拍的行里
               （只是行序靠后），规则引擎那条"人不见了"的收尾循环不会为他触发，于是他的
               教师行为列等不到 END。挤占本身就是"有记录地少认一个人"，记录不齐等于没记录 */
            ++m_u64Demote;
            pushFlip(cstVictim.nOwnerId, cstVictim.enSubject, Subject_E::SUBJECT_UNKNOWN,
                     llFrameMs);
            /* 与回收循环同一条理：身份随槽走，最后位置留给紧随的重入快轨。
               少了这一笔，被挤掉的教师身份只有"新 ID 从零攒 confirm"一条路，
               而挤占恰恰发生在满座高 churn 的拍上——正是最需要快轨的时候 */
            saveReentryMem(cstVictim.nOwnerId, cstVictim.stLastPos, llFrameMs);
        }
        /* 满员是"上游容量纪律被破坏"的持续状态：每拍一行会把 stream.log 刷成挤占流水，
           计数逐次照记，告警按 1s 窗口限频（与 rule_engine 的 dropLogDue 同口径） */
        if (llFrameMs <= 0 || m_llStealLogMs <= 0 || llFrameMs - m_llStealLogMs >= 1000)
        {
            m_llStealLogMs = llFrameMs;
            dlog_warn("classroom subject: 归属槽位表已满，挤掉最久未出现的人 %d（累计挤出 %llu）",
                      cstVictim.nOwnerId, static_cast<unsigned long long>(m_u64SlotSteal));
        }
    }

    CSlot_S &rstSlot = m_astSlot[static_cast<size_t>(nFree)];
    rstSlot = CSlot_S();
    rstSlot.nOwnerId = nPersonId;
    rstSlot.llLastSeenMs = llFrameMs;
    return rstSlot;
}

void CSubjectArbiter::saveMoveMem(const CSlot_S &rstSlot, int64_t llNowMs)
{
    if (rstSlot.nOwnerId < 0 || rstSlot.llMoveBucketMs <= 0)
    {
        return;
    }
    CMoveMem_S *pEntry = nullptr;
    for (int32_t i = 0; i < MOVE_MEM_NUM; ++i)
    {
        if (m_aMoveMem[i].nOwnerId == rstSlot.nOwnerId)
        {
            pEntry = &m_aMoveMem[i];
            break;
        }
    }
    if (nullptr == pEntry)
    {
        /* 先复用空/过期格（#158 第 24 轮）。现行"接回即失效"语义下，旧实现的环位覆盖
           本来也等于按最旧覆盖；这里把顺序显式写出来，不再让淘汰策略依赖
           "环位顺序 == 年龄序"这条跨函数耦合（失效时机一变，环位就不再等于年龄） */
        for (int32_t i = 0; i < MOVE_MEM_NUM; ++i)
        {
            if (m_aMoveMem[i].nOwnerId < 0 ||
                (llNowMs - m_aMoveMem[i].llSaveMs) > MOVE_MEM_TTL_MS)
            {
                pEntry = &m_aMoveMem[i];
                break;
            }
        }
        if (nullptr == pEntry)
        {
            /* 全表都在期内：只能淘汰一条活记忆。选 llSaveMs 最旧的（并列取表序靠前，
               分配顺序确定、台账可逐格重放），并计数——满座高 churn 下这说明
               位移连续性被打断过，与 210 巡视段被切在一起看 */
            pEntry = &m_aMoveMem[0];
            int64_t llOldestAge = llNowMs - pEntry->llSaveMs;
            for (int32_t i = 1; i < MOVE_MEM_NUM; ++i)
            {
                const int64_t llAge = llNowMs - m_aMoveMem[i].llSaveMs;
                if (llAge > llOldestAge)
                {
                    llOldestAge = llAge;
                    pEntry = &m_aMoveMem[i];
                }
            }
            ++m_u64MoveMemEvict;
        }
    }
    pEntry->nOwnerId = rstSlot.nOwnerId;
    pEntry->llSaveMs = rstSlot.llLastSeenMs;
    pEntry->llBucketMs = rstSlot.llMoveBucketMs;
    pEntry->llWindowMs = rstSlot.llMoveWindowMs;
    pEntry->nBucketIdx = rstSlot.nMoveBucketIdx;
    pEntry->llLastMoveMs = rstSlot.llLastMoveMs;
    pEntry->stLastPos = rstSlot.stLastPos;
    pEntry->nWindowLenMs = rstSlot.nWindowLenMs;
    for (int32_t k = 0; k < CSlot_S::MOVE_BUCKET_NUM; ++k)
    {
        pEntry->afBucket[k] = rstSlot.afMoveBucket[k];
    }
}

void CSubjectArbiter::restoreMoveMem(CSlot_S &rstSlot, int64_t llFrameMs, int32_t nWindowMs)
{
    for (int32_t i = 0; i < MOVE_MEM_NUM; ++i)
    {
        CMoveMem_S &rstMem = m_aMoveMem[i];
        if (rstMem.nOwnerId != rstSlot.nOwnerId)
        {
            continue;
        }
        const int64_t llAgeMs = llFrameMs - rstMem.llSaveMs;
        if (llAgeMs < 0 || llAgeMs > MOVE_MEM_TTL_MS)
        {
            rstMem.nOwnerId = -1;
            return;
        }
        rstSlot.llMoveBucketMs = rstMem.llBucketMs;
        rstSlot.nMoveBucketIdx = rstMem.nBucketIdx;
        rstSlot.llLastMoveMs = rstMem.llLastMoveMs;
        rstSlot.stLastPos = rstMem.stLastPos;
        for (int32_t k = 0; k < CSlot_S::MOVE_BUCKET_NUM; ++k)
        {
            rstSlot.afMoveBucket[k] = rstMem.afBucket[k];
        }
        rstMem.nOwnerId = -1;   /* 记忆只接一次，避免换 ID 后残留 */
        /* 窗口锚保持「上次看见他的那一拍」：接回后的第一拍要用真实缺席时长算速度上限，
           写成 llFrameMs 会让 gap=0、速度闸失效（#102 第八刀的第一次实现就栽在这里） */
        rstSlot.llMoveWindowMs = rstMem.llWindowMs;
        rstSlot.nWindowLenMs = nWindowMs;
        if (rstMem.nWindowLenMs != nWindowMs)
        {
            /* 缺席期间窗宽被热改（#158 第 23 轮，B8 表 211-221 那一行）：桶是按
               **旧**窗宽/10 切出来的，拿新桶长去清「缺席跨过的桶」不是同一把尺——
               多清少清都是错桶数，于是把停之前的位移继续当成「最近一个窗口」的读数。
               方向取保守：不继承累计（当作刚起窗），位置与窗口锚照旧保留，
               下一拍的速度闸仍按真实缺席时长兜住跳变（testPatrolMoveRestoreNoTeleport
               钉的就是保留锚这一格）；新窗口走满后读数会照常长回来 */
            for (int32_t k = 0; k < CSlot_S::MOVE_BUCKET_NUM; ++k)
            {
                rstSlot.afMoveBucket[k] = 0.0;
            }
            rstSlot.nMoveBucketIdx = 0;
            rstSlot.llMoveBucketMs = llFrameMs;
            rstSlot.dfMovePx = 0.0;
            return;
        }
        /* 缺席这段跨过的桶要清掉：那段时间没看见人，不能把停之前的位移算进"最近一个窗口" */
        int64_t llBucket = static_cast<int64_t>(nWindowMs) / CSlot_S::MOVE_BUCKET_NUM;
        if (llBucket < 1)
        {
            llBucket = 1;
        }
        while (llFrameMs - rstSlot.llMoveBucketMs >= llBucket)
        {
            rstSlot.llMoveBucketMs += llBucket;
            rstSlot.nMoveBucketIdx = (rstSlot.nMoveBucketIdx + 1) % CSlot_S::MOVE_BUCKET_NUM;
            rstSlot.afMoveBucket[rstSlot.nMoveBucketIdx] = 0.0;
        }
        double dfSum = 0.0;
        for (int32_t k = 0; k < CSlot_S::MOVE_BUCKET_NUM; ++k)
        {
            dfSum += rstSlot.afMoveBucket[k];
        }
        rstSlot.dfMovePx = dfSum;
        return;
    }
}

void CSubjectArbiter::saveReentryMem(int32_t nOwnerId, const CPointf_S &stPos, int64_t llFrameMs)
{
    if (nOwnerId < 0)
    {
        return;
    }
    int32_t nFree = -1;
    int32_t nOldest = -1;
    int64_t llOldestMs = 0;
    for (int32_t i = 0; i < REENTRY_MEM_NUM; ++i)
    {
        if (m_astReentry[i].nOwnerId < 0)
        {
            nFree = i;
            break;
        }
        /* 找最旧必须扫完整表：早退会让第一个格冒充最旧的格（与 acquireSlot 同一条） */
        if (nOldest < 0 || m_astReentry[i].llRetireMs < llOldestMs)
        {
            nOldest = i;
            llOldestMs = m_astReentry[i].llRetireMs;
        }
    }
    const int32_t nIdx = (nFree >= 0) ? nFree : nOldest;
    CReentryMem_S &rstMem = m_astReentry[static_cast<size_t>(nIdx)];
    rstMem.nOwnerId = nOwnerId;
    rstMem.llRetireMs = llFrameMs;
    rstMem.stPos = stPos;
}

int32_t CSubjectArbiter::findReentryMem(const CPointf_S &stPos, int64_t llFrameMs,
                                        int32_t nWindowMs, int32_t nPx) const
{
    int32_t nBest = -1;
    double dfBest = 0.0;
    for (int32_t i = 0; i < REENTRY_MEM_NUM; ++i)
    {
        const CReentryMem_S &cstMem = m_astReentry[i];
        if (cstMem.nOwnerId < 0 ||
                (llFrameMs - cstMem.llRetireMs) > static_cast<int64_t>(nWindowMs))
        {
            continue;
        }
        const double dfDist = distance2d(cstMem.stPos, stPos);
        if (dfDist > static_cast<double>(nPx))
        {
            continue;
        }
        if (nBest < 0 || dfDist < dfBest)
        {
            nBest = i;
            dfBest = dfDist;
        }
    }
    return nBest;
}

void CSubjectArbiter::retireSlot(CSlot_S &stSlot, int64_t llFrameMs)
{
    /* 人不在了不发翻转：他的行为收尾由 CRuleEngine 那条既有的"人不见了"循环做
       （给名下全部生效行为补 END 再 retirePerson），这里再发一条就是
       "同一个行为两个 END"的来源。身份随之消失：
       跟踪侧换 ID 后是同一个人的新账户，教师身份要重新攒确认时长。
       （本注释曾断言"根因在跟踪不在本类，故不做补偿"——tt15 的 keep_ms A/B 证伪了它：
       trk 新建 -60% 而晋升数不变，churn 的一半就在"缺一拍即收槽"这层。
       补偿由回收循环的 nAbsentHoldMs 承担；本函数保持无差别全清） */
    saveMoveMem(stSlot, llFrameMs);
    stSlot = CSlot_S();
}

bool CSubjectArbiter::inRole(const CRegionTable &stRegion, const std::vector<int32_t> &vecIds,
                             float fCoverage, const CRectf_S &stHeadBox, bool &rbRefError) const
{
    rbRefError = false;
    if (vecIds.empty())
    {
        /* 角色没画区域 = 这个人**不在**该用途里。不能借用 inAnyRegion 的空表语义
           （那里空表表示"不限区域"，是给逐行为区域闸用的），否则没画讲台就等于全员在讲台 */
        return false;
    }
    const bool bIn = stRegion.inAnyRegion(vecIds, fCoverage, stHeadBox, rbRefError);
    /* 不在这里喊：一条区域引用错误在满座教室里每拍能凑 180 行日志，
       把整个 stream.log 变成刷屏（本子系统对日志量的纪律）。由 update() 每拍最多报一次 */
    return bIn;
}

void CSubjectArbiter::update(const CFeatureFrame_S &stFrame, const CRegionTable &stRegion,
                             const CConfigSnapshot_S &stSnap, int64_t llFrameMs)
{
    const CTeacherConf_S &stConf = stSnap.stTeacher;
    ++m_u64Updates;

    const int32_t nCap = static_cast<int32_t>(stFrame.vecPersons.size());
    const int32_t nRowNum = (stFrame.nPersonNum < nCap) ? stFrame.nPersonNum : nCap;

    /* 三条角色区域每拍各取一次引用：逐行查角色表等于把同一张表扫 180 遍，
       而"画没画"这件事一拍之内不可能变 */
    const std::vector<int32_t> &vecPodium = stRegion.idsOfRole(RegionRole_E::REGION_ROLE_PODIUM);
    const std::vector<int32_t> &vecBoard = stRegion.idsOfRole(RegionRole_E::REGION_ROLE_BOARD);
    const std::vector<int32_t> &vecStudent = stRegion.idsOfRole(RegionRole_E::REGION_ROLE_STUDENT);
    /* 三条 configured 标志按配置刷新（描述的是"画没画"，与有没有行无关），
       放在开关判断之前：关掉仲裁时运维仍要能看出"讲台是画了的" */
    const bool bPodiumConfigured = !vecPodium.empty();
    const bool bStudentConfigured = !vecStudent.empty();
    m_bPodiumConfigured = bPodiumConfigured;
    m_bStudentRoleConfigured = bStudentConfigured;
    m_bBoardConfigured = !vecBoard.empty();

    if (!stConf.bEnable)
    {
        /* 关掉仲裁：身份一律就地收回并发翻转，**不等放弃时长**。
           不等是因为开关关掉之后不会再有 update()，那条 END 就只能靠时长门的 gap 容忍补，
           而 gap 是"证据断了但人可能还在"的容忍，用它收尾一个明确被关掉的功能，
           等于把"关掉了"写成"他消失了"——平台侧这两个故事不是一回事 */
        for (CSlot_S &stSlot : m_astSlot)
        {
            if (stSlot.nOwnerId < 0)
            {
                continue;
            }
            if (Subject_E::SUBJECT_TEACHER == stSlot.enSubject)
            {
                pushFlip(stSlot.nOwnerId, stSlot.enSubject, Subject_E::SUBJECT_UNKNOWN, llFrameMs);
                ++m_u64Demote;
            }
            retireSlot(stSlot, llFrameMs);
        }
        m_nTeacherNum = 0;
        m_nCandidateNum = 0;
        /* bPodiumEmpty 保持上一拍的值：这一拍没遍历行，说"讲台空"是猜的，
           而 statusJson 里 podium_configured=false 已经足够说明"没在判" */
        publishStat();
        return;
    }

    /* ---------- 第一遍：区域事实 + 位移，并数出讲台里有没有人 ---------- */
    int32_t nPodiumNum = 0;
    int32_t nTeacherAreaNum = 0;
    int32_t nRefError = 0;
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPersonFeature_S &stRow = stFrame.vecPersons[nRow];
        if (stRow.nPersonId < 0)
        {
            continue;
        }
        CSlot_S &stSlot = acquireSlot(stRow.nPersonId, llFrameMs);
        if (stSlot.llMoveBucketMs <= 0)
        {
            /* 刚认领的槽：同 ID 在 TTL 内回来就接上位移记忆（#102 第五刀） */
            restoreMoveMem(stSlot, llFrameMs, stConf.nPatrolWindowMs);
        }
        CSubjectFact_S &stFact = stSlot.stFact;
        bool bRefError = false;

        /* T38 姿态证人（ever-sticky，槽位见 subject_arbiter.h）：讲台/黑板的区域证据
           只认"本轨见过可用骨骼"的行。头检在深色板面上会输出 ~300px 的幻影大头，这类框上姿态模型
           拿不出人；真人恰相反，出场一秒内就有骨骼——sticky 是给真人的：骨骼是
           分时轮转的产物，本拍没轮到不等于没有，见过一次就该继续算数。
           ttl 配 0 = 证人关位，区域证据恢复旧形态（A/B 关位）。 */
        if (stConf.fPoseWitnessTtlMs > 0 &&
            stRow.poseUsable(llFrameMs, static_cast<int64_t>(stConf.fPoseWitnessTtlMs)))
        {
            if (!stSlot.bWitEvalLogged)
            {
                const CPosePoint_S &stNose =
                    stRow.stSkeleton.astPoints[static_cast<int>(PosePoint_E::POSE_NOSE)];
                const CPosePoint_S &stTop =
                    stRow.stSkeleton.astPoints[static_cast<int>(PosePoint_E::POSE_HEAD_TOP)];
                stSlot.bWitEvalLogged = true;
                dlog_info("classroom subject: 证人评估 人=%d 头框=[%.0f,%.0f,%.0f,%.0f] "
                          "鼻=(%.0f,%.0f,%.2f) 头顶=(%.0f,%.0f,%.2f) 骨得分=%.2f 分限=%.2f 余量=%d%%",
                          stRow.nPersonId,
                          stRow.stHeadBox.fX1, stRow.stHeadBox.fY1,
                          stRow.stHeadBox.fX2, stRow.stHeadBox.fY2,
                          stNose.fX, stNose.fY, stNose.fScore,
                          stTop.fX, stTop.fY, stTop.fScore,
                          stRow.stSkeleton.fScore, stConf.fPoseWitMinScore, stConf.nWitOwnHeadPct);
            }
            if (!stSlot.bPoseWitness)
            {
                bool bOwnHead = true;
                if (stConf.nWitOwnHeadPct > 0)
                {
                    const CPosePoint_S &stNose =
                        stRow.stSkeleton.astPoints[static_cast<int>(PosePoint_E::POSE_NOSE)];
                    const CPosePoint_S &stTop =
                        stRow.stSkeleton.astPoints[static_cast<int>(PosePoint_E::POSE_HEAD_TOP)];
                    const float fMx = static_cast<float>(stConf.nWitOwnHeadPct) * 0.01f *
                                      (stRow.stHeadBox.fX2 - stRow.stHeadBox.fX1);
                    const float fMy = static_cast<float>(stConf.nWitOwnHeadPct) * 0.01f *
                                      (stRow.stHeadBox.fY2 - stRow.stHeadBox.fY1);
                    bOwnHead =
                        ((stNose.isValid() &&
                          stNose.fX >= stRow.stHeadBox.fX1 - fMx &&
                          stNose.fX <= stRow.stHeadBox.fX2 + fMx &&
                          stNose.fY >= stRow.stHeadBox.fY1 - fMy &&
                          stNose.fY <= stRow.stHeadBox.fY2 + fMy) ||
                         (stTop.isValid() &&
                          stTop.fX >= stRow.stHeadBox.fX1 - fMx &&
                          stTop.fX <= stRow.stHeadBox.fX2 + fMx &&
                          stTop.fY >= stRow.stHeadBox.fY1 - fMy &&
                          stTop.fY <= stRow.stHeadBox.fY2 + fMy));
                    if (!bOwnHead)
                    {
                        ++m_u64WitHeadMis;
                    }
                }
                bool bScoreOk = true;
                if (stConf.fPoseWitMinScore > 0.0f &&
                    stRow.stSkeleton.fScore < stConf.fPoseWitMinScore)
                {
                    bScoreOk = false;
                    ++m_u64WitScoreRej;
                }
                stSlot.bPoseWitness = (bOwnHead && bScoreOk);
            }
        }
        const bool bPoseWit = (stConf.fPoseWitnessTtlMs <= 0) || stSlot.bPoseWitness;

        const bool bInPodium = inRole(stRegion, vecPodium, stConf.fCoverage, stRow.stHeadBox, bRefError);
        nRefError += bRefError ? 1 : 0;
        const bool bInBoard = inRole(stRegion, vecBoard, stConf.fCoverage, stRow.stHeadBox, bRefError);
        nRefError += bRefError ? 1 : 0;
        if (!bPoseWit)
        {
            /* 计数记的是"区域说在、证人说没有"的原始命中：开着证人时恒 0 说明
               拦错了对象（幻影框上姿态模型反而有输出），要回头查的是证人本身 */
            m_u64PoseWitBlocks += (bInPodium || bInBoard) ? 1 : 0;
        }
        /* 证人拦在事实这一层而不是只拦候选：幻影框不持有教师槽之外，
           也不能以"学生站在讲台"的名义出 110、更不能把讲台数成"有人" */
        const bool bGatedPodium = bInPodium && bPoseWit;
        const bool bGatedBoard = bInBoard && bPoseWit;
        if (bGatedPodium)
        {
            ++nPodiumNum;
            m_llPodiumWitnessMs = llFrameMs;
        }
        if (bGatedPodium || bGatedBoard)
        {
            ++nTeacherAreaNum;
        }
        /* 两条分开留：归属用的是它们的 OR（老师站在黑板前也算在岗），但教师行为问的是具体一块——
           「板书」只要黑板、「进入讲台」只要讲台。合成一个布尔再让下游各自猜，
           等于把这条判据拆到两个文件里写 */
        stFact.bInPodium = bGatedPodium;
        stFact.bInBoard = bGatedBoard;
        stFact.bInTeacherArea = bGatedPodium || bGatedBoard;

        stFact.bInStudentArea =
            inRole(stRegion, vecStudent, stConf.fCoverage, stRow.stHeadBox, bRefError);
        nRefError += bRefError ? 1 : 0;
        if (!bStudentConfigured)
        {
            /* 没画学生席时按「讲台与黑板之外」兜底。它会把过道与后墙也算进来，
               口径比真区域宽——所以观测里单列 b_student_role_configured，
               现场要精确就把学生席画上一条，不必为此拒绝判定 */
            stFact.bInStudentArea = !stFact.bInTeacherArea;
        }


        /* 静止指导的停留锚（#93-4）：进学生区起锚，出学生区就地清零。
           连续性由"锚只在区内保持"这条规则本身给出——中间任何一拍出了区，
           锚归零重新攒，离开半步再回来不会被当成连续停留 */
        if (stFact.bInStudentArea)
        {
            if (stSlot.llStudentStaySinceMs <= 0)
            {
                stSlot.llStudentStaySinceMs = llFrameMs;
            }
        }
        else
        {
            stSlot.llStudentStaySinceMs = 0;
        }

        /* ---- 位移：两帧之间的路径长度按窗口累计 ---- */
        const CPointf_S stPos = stRow.stHeadBox.center();
        /* 本拍桶格点用的窗宽：存入记忆时带走，接回时据此判断旧桶能不能接（#158 第 23 轮） */
        stSlot.nWindowLenMs = stConf.nPatrolWindowMs;
        const int64_t llSpanMs = llFrameMs - stSlot.llMoveWindowMs;
        if (stSlot.llMoveWindowMs <= 0 ||
                llSpanMs > static_cast<int64_t>(stConf.nPatrolWindowMs) * CLASSROOM_MOVE_GAP_TIMES ||
                llSpanMs > CLASSROOM_MOVE_WINDOW_MAX)
        {
            /* 首次见此人的位置，或距上次起窗已经隔了一大段（感知停过、或降级把这几拍挤掉了）。
               重锚必须**位置与窗口一起换**：只换窗口而把"上次位置"留在几十秒前那个点上，
               重锚后的第一拍就会把穿越半个画面的距离当成位移加进去——而巡视判据要的
               恰好是"这段时间确实走动过"，凭空造出来的位移正是它最不该有的假证据 */
            for (int32_t k = 0; k < CSlot_S::MOVE_BUCKET_NUM; ++k)
            {
                stSlot.afMoveBucket[k] = 0.0;
            }
            stSlot.nMoveBucketIdx = 0;
            stSlot.llMoveBucketMs = llFrameMs;
            stSlot.llMoveWindowMs = llFrameMs;
            stSlot.stLastPos = stPos;
            stSlot.dfMovePx = 0.0;
        }
        else
        {
            /* 环形桶：跨过的桶清零后指针前移。桶长取窗口的 1/N，量化误差 ≤ 一个桶；
               跨多桶（感知停过）时那些桶本来也不该把停之前的位移算进「最近一个窗口」 */
            int64_t llBucket = static_cast<int64_t>(stConf.nPatrolWindowMs) / CSlot_S::MOVE_BUCKET_NUM;
            if (llBucket < 1)
            {
                llBucket = 1;
            }
            while (llFrameMs - stSlot.llMoveBucketMs >= llBucket)
            {
                stSlot.llMoveBucketMs += llBucket;
                stSlot.nMoveBucketIdx = (stSlot.nMoveBucketIdx + 1) % CSlot_S::MOVE_BUCKET_NUM;
                stSlot.afMoveBucket[stSlot.nMoveBucketIdx] = 0.0;
            }
            /* 跨过缺席的那一步要过速度闸：
               「上次看到的位置 → 现在的位置」里含着没观测到的那段时间。1~3 拍的漏检
               按位移算是对的（人确实在走，跟踪器只是没输出），但若这段距离对应的人体
               速度超过 CLASSROOM_MOVE_MAX_SPEED_PX_S，就不是走路而是换人/换 ID/跳变，
               只能重锚位置、不能进窗口——否则 210 会凭一次跳变开出一段不存在的巡视 */
            const double dfGap = distance2d(stSlot.stLastPos, stPos);
            const int64_t llGapMs = llFrameMs - stSlot.llMoveWindowMs;
            const double dfMaxGap = CLASSROOM_MOVE_MAX_SPEED_PX_S *
                                    (static_cast<double>(llGapMs > 0 ? llGapMs : 0) / 1000.0);
            /* 时间没前进（同刻两拍）或倒退：这段距离换不出速度预算，**不当作位移**。
               旧码在 llGapMs<=0 时无条件累加——帧时刻倒退的那一拍可以把一次跳变
               （换人/换 ID）直接写进窗口，而巡视判据要的正是"这段时间确实走动过" */
            if (llGapMs > 0 && dfGap <= dfMaxGap)
            {
                stSlot.afMoveBucket[stSlot.nMoveBucketIdx] += dfGap;
            }
            stSlot.stLastPos = stPos;
            stSlot.llMoveWindowMs = llFrameMs;
            double dfSum = 0.0;
            for (int32_t k = 0; k < CSlot_S::MOVE_BUCKET_NUM; ++k)
            {
                dfSum += stSlot.afMoveBucket[k];
            }
            stSlot.dfMovePx = dfSum;
        }
        stFact.dfMovePx = stSlot.dfMovePx;
        stFact.bMoved = (stSlot.dfMovePx >= static_cast<double>(stConf.nPatrolMovePx));
        if (stFact.bMoved)
        {
            stSlot.llLastMoveMs = llFrameMs;
        }
        /* 走动保持（#102 第六刀）：210 用的是"最近这段时间在走动"，不是"这一拍在走动"。
           真实巡视是走几步、在桌边停几秒再走；保持期取 2 个窗口
           （5s 窗 → 10s），只影响 210；207 指导仍用瞬时 bMoved，两条判据保持互补。
           `llLastMoveMs <= llFrameMs` 是必需的：帧时刻倒退时差值为负，负 ≤ 正窗口
           恒真，这个"保持"会在时间倒流后一直挂着 */
        stFact.bMovedHold = (stSlot.llLastMoveMs > 0) &&
                            stSlot.llLastMoveMs <= llFrameMs &&
                            ((llFrameMs - stSlot.llLastMoveMs) <=
                                 static_cast<int64_t>(stConf.nPatrolWindowMs) * CLASSROOM_PATROL_HOLD_TIMES);
    }

    /* ---------- 回收本拍没出现的槽 ----------
       时机与时长门的 retire 完全一致：同一拍里"人不在了"在两层的结论必须相同，
       否则会出现仲裁器还认他是老师、门表却已经收了他的行为这种劈开的状态。
       缺席容忍（T43，nAbsentHoldMs>0 时生效，**只保教师槽**）：跟踪器刻意不输出丢失拍
       （person_tracker「丢失中的不输出」防人数虚高），"一拍没看见"因此分不出
       检测闪断与真人离场；缺一拍即收槽 + 确认时长重攒 = 一次闪断烧掉整段身份
       容忍期内教师槽原样保留——事件连续性由各时长门的 gap 容忍管，本值只保身份不保事件。
       候选槽**不 hold**（当拍照收）：晋升门只看 llCandidateMs 年龄、没有"本拍在场"闸
       超过容忍仍按"人不在"收（旧语义），教师槽走这条路收掉时留一行
       读数供定量缺席 */
    for (CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId < 0 || stSlot.llLastSeenMs == llFrameMs)
        {
            continue;
        }
        const bool bHeld = (stConf.nAbsentHoldMs > 0) &&
                           (Subject_E::SUBJECT_TEACHER == stSlot.enSubject) &&
                           ((llFrameMs - stSlot.llLastSeenMs) <=
                                static_cast<int64_t>(stConf.nAbsentHoldMs));
        if (bHeld)
        {
            continue;
        }
        if (Subject_E::SUBJECT_TEACHER == stSlot.enSubject)
        {
            dlog_info("classroom subject: 教师槽 人=%d 缺席 %lldms 超容忍 %dms，收槽",
                      stSlot.nOwnerId,
                      static_cast<long long>(llFrameMs - stSlot.llLastSeenMs),
                      stConf.nAbsentHoldMs);
            /* T44：教师身份随槽回收而丢。最后位置与时刻留给紧随其后的重入快轨——
               换 ID 后继若带强证据落回这条位置连续窗，当拍继承不重攒确认。
               同拍退多位时各写各的（第 47 轮），不再互相覆盖 */
            saveReentryMem(stSlot.nOwnerId, stSlot.stLastPos, llFrameMs);
        }
        retireSlot(stSlot, llFrameMs);
    }

    /* 讲台"空"的前提是讲台画了：没画时给 false，巡视那条弱判据整条随之关闭。
       证人开着时再加一道保持窗（#175-B）：换槽/丢检造出的 ≤2s 空拍不算空，
       否则 210 会把"这一拍没认到"缝成"讲台真空了"，起点还回推到教师在场时。
       时间倒退时 delta 为负 ⇒ 按"还占着"处理（失败安全方向，不凭空造空） */
    const bool bPodiumHeld = (stConf.fPoseWitnessTtlMs > 0) &&
                             (m_llPodiumWitnessMs >= 0) &&
                             ((llFrameMs - m_llPodiumWitnessMs) < PODIUM_OCCUPANCY_TTL_MS);
    m_bPodiumEmpty = bPodiumConfigured && (0 == nPodiumNum) && !bPodiumHeld;
    /* 同理，"讲台与黑板都没人"这句话在两块都没画时无从谈起——指导判据要的是这条，
       而不是只数讲台的 bPodiumEmpty（老师可能正在黑板前，讲台自然是空的） */
    m_bTeacherAreaEmpty = (bPodiumConfigured || m_bBoardConfigured) && (0 == nTeacherAreaNum);

    if (nRefError > 0)
    {
        /* 一拍最多一条：这条报的是"配置与区域表不是同一份"，真要查的是换快照那一段
           的时序，不是每分钟几百行的重复播报 */
        m_nRoleRefError += nRefError;
        /* 单位是人次不是人：一行最多问讲台+黑板两次，同一个人的两项都会计进来 */
        dlog_warn("classroom subject: 本拍有 %d 人次（行×区域角色）引用了未建成的区域，累计 %d",
                  nRefError, m_nRoleRefError);
    }

    /* 窗口读帧上自带的实测节拍，不读配置（#69-A）：配置里那两个数决定不了
       感知层真正多久轮到一个人，猜出来的窗口会把后排判成"永久没证据" */
    const int32_t nPoseTtlMs = stFrame.poseTtlMs();

    /* ---------- 第二遍：站立事实（归属与行为必须用同一种"站着"）---------- */
    int32_t nStuStandNum = 0;
    /* 本遍已经数进 nStuStandNum 的人（按 ID）：同一帧里同一 ID 出现多行时，
       后面的行照样刷新事实（槽位要的是最后一次读到的那一行），但**不再计数**。
       为什么计数要按 ID 去重：nStuStandNum 是**全拍**事实（207 指导的"独行者"判据
       要求恰好 1 人），而这里是**按行**扫的 —— 帧行表一旦出现同 ID 两行（上游行表
       构造重复、或以后的多源合并），"恰好 1 人"会被数成 2 ⇒ 判据整条失效，
       而现象是"指导从来没成立过"，日志里一个字都不报。
       注意这里只去重计数，不去重事实刷新：两行都读、以后一次为准。 */
    int32_t anCountedId[CLASSROOM_MAX_PERSONS];
    int32_t nCountedNum = 0;
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CPersonFeature_S &stRowDup = stFrame.vecPersons[static_cast<size_t>(nRow)];
        bool bCounted = false;
        for (int32_t k = 0; k < nCountedNum; ++k)
        {
            if (anCountedId[k] == stRowDup.nPersonId)
            {
                bCounted = true;
                break;
            }
        }

        CSlot_S *pSlot = findSlot(stRowDup.nPersonId);
        if (nullptr == pSlot)
        {
            continue;   /* 第一遍没认领到（行 ID 非法），本拍不判 */
        }
        const CPersonFeature_S &stRow = stFrame.vecPersons[static_cast<size_t>(nRow)];
        CSlot_S &stSlot = *pSlot;
        CSubjectFact_S &stFact = stSlot.stFact;

        /* 站立：与采样器同一道证据闸（骨骼在、没过期、没遮挡、没贴边、不太糊），
           判据也复用 geometry 同一个函数与同一份参数——归属与行为必须用同一种"站着"，
           否则会出现"因站着被判成教师，而他的站立行为又因证据不足不成立"的自相矛盾 */
        stFact.bStandKnown = false;
        stFact.bStanding = false;
        if (stFrame.bPoseResident && stRow.poseUsable(llFrameMs, nPoseTtlMs) &&
                !stRow.bOccluded && !stRow.bOnBoundary &&
                !(stRow.fBlurScore > 0.0f &&
                  stRow.fBlurScore < poseBlurMinForHeadPx(stSnap.fPoseBlurMin, stRow.stHeadBox.height())))
        {
            CGeometryInput stInput;
            stInput.pstSkeleton = &stRow.stSkeleton;
            stInput.stHeadBox = stRow.stHeadBox;
            stInput.stBodyBox = stRow.stBodyBox;
            stInput.fShoulderWidth = 0.0f;   /* 站立判据不看肩宽 */
            const TriState_E enStand = judgeStanding(stInput, stSnap.stStand);
            stFact.bStandKnown = (TriState_E::TS_UNKNOWN != enStand);
            stFact.bStanding = (TriState_E::TS_TRUE == enStand);
        }
        else
        {
            /* T25：挡在哪一挡要能从读数上分出来，否则生产 8h 攒下来的 stand_blocked
               只有一个总数，没法回答"贴边/遮挡/过期各占多少、值不值得治理"。
               先命中先记（遮挡优先于贴边：一个被前排挡住的贴边行，根因是遮挡） */
            ++m_u64StandBlocked;
            if (!stFrame.bPoseResident || !stRow.poseUsable(llFrameMs, nPoseTtlMs))
            {
                ++m_u64StandBlkNoPose;
            }
            else if (stRow.bOccluded)
            {
                ++m_u64StandBlkOccl;
            }
            else if (stRow.bOnBoundary)
            {
                ++m_u64StandBlkEdge;
            }
            else
            {
                ++m_u64StandBlkBlur;
            }
        }

        if (stFact.bStandKnown && stFact.bStanding && stFact.bInStudentArea && !bCounted)
        {
            ++nStuStandNum;
            /* 记下已数过的 ID：同帧重复行不再计（只计一次，事实刷新不受影响） */
            if (nCountedNum < CLASSROOM_MAX_PERSONS)
            {
                anCountedId[nCountedNum++] = stRowDup.nPersonId;
            }
        }
    }

    /* ---------- 教师证据与本轮起点 ----------
       挪到站立事实全部出来之后再判：巡视的位移是逐人事实，静止指导的"独行者"
       却是**全拍**事实（学生区站立者恰好 1 人）。在行循环里顺手判等于拿"到当前行为止
       数出来的半份名单"当全拍——行序会把结论翻过来，与文件头"台账可以逐格重放"冲突。
       证据按槽位判（本拍出现过的槽），与按行判等价：本拍没出现的槽不在此列 */
    for (CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId < 0 || stSlot.llLastSeenMs != llFrameMs)
        {
            continue;
        }
        CSubjectFact_S &stFact = stSlot.stFact;

        /* 强证据：人在讲台或黑板里。
           弱证据一（CSV 的巡视口径）：讲台空 + 学生席里站着 + **确实走动过**。
           位移这一条不能省：全班起立问好那 30 秒讲台是空的、30 个人都在学生席站着，
           没有位移就把其中"证据最连续"的一个判成教师，而教师一被安到学生头上，
           那个人的学生行为同时停止记账——恰好在最热闹的那 30 秒丢掉一个学生的全部数据。
           弱证据二（静止指导，#93-4）：巡视漏的是**不走动**的指导（俯身看作业、
           站在小组旁讲解），位移判据对它恒 false。补的判据带三重闸：
           讲台空（讲台有人时不需要这条）、连续停留满 teacher.stay_evidence_ms、
           学生区站立者**恰好 1 人**（同步起立/齐读时人人站立，谁都不算；
           独自站着答题的学生被讲台有人那一关挡住——答题时讲台通常是 occupied）。
           默认 0=关：新判据不上电，现场显式打开才生效 */
        const bool bWeakMove = m_bPodiumEmpty && stFact.bInStudentArea && stFact.bMoved &&
                               !(stFact.bStandKnown && !stFact.bStanding);
        const bool bWeakStay = (stConf.nStayEvidenceMs > 0) && m_bPodiumEmpty &&
                               stFact.bStandKnown && stFact.bStanding && stFact.bInStudentArea &&
                               stSlot.llStudentStaySinceMs > 0 &&
                               (llFrameMs - stSlot.llStudentStaySinceMs) >=
                                   static_cast<int64_t>(stConf.nStayEvidenceMs) &&
                               (1 == nStuStandNum);
        if (bWeakStay)
        {
            ++m_u64StayEvidence;
        }
        const bool bEvidence = stFact.bInTeacherArea || bWeakMove || bWeakStay;

        if (bEvidence)
        {
            if (stSlot.llCandidateMs <= 0)
            {
                stSlot.llCandidateMs = llFrameMs;
            }
            stSlot.llLastEvidenceMs = llFrameMs;
        }
        else if (stSlot.llCandidateMs > 0 &&
                 (llFrameMs - stSlot.llLastEvidenceMs) > static_cast<int64_t>(stConf.nReleaseMs))
        {
            stSlot.llCandidateMs = 0;   /* 断太久，本轮起点作废，重新攒确认时长 */
        }
    }

    /* ---------- 第三遍：先让位再上位（同一拍换人不留空档） ---------- */
    int32_t nTeacherNum = 0;
    for (const CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId >= 0 && Subject_E::SUBJECT_TEACHER == stSlot.enSubject)
        {
            ++nTeacherNum;
        }
    }

    /* ---------- 教师重入快轨（T44）----------
       hold 只保旧槽保不了身份：站立教师被检测漏检，跟踪器换 ID 后继任
       从零攒 confirm_ms，这几秒里他按学生分类站在讲台上出 110（tt19 残余 11 条
       stu_on_podium 假阳性的全部成因）。本段把回收循环与挤占留下的重入记忆兑付掉：
       本拍刚开始攒证据的候选（llCandidateMs==本拍，即换 ID/回归的签名），
       (a) 讲台/黑板强证据在身——弱判据攒的不许快轨，全班起立问好时位移最强的
       学生不能借道上位；(b) 头框落回记忆位置 reentry_px 半径内——"同一个人"的
       必要条件，换了个人在黑板前（Δ≈760px）不继承；(c) 在 reentry_ms 窗口内——
       记忆不是永久豁免，tt19 的 230→242（27s）就该重攒。幽灵交接（旧槽还在
       容忍期内，位置即记忆）优先于记忆路径：旧槽当拍退位（翻转+demote）、
       候选当拍上位，同一拍换人不留空档——交接进出相抵，不受 max_num 约束。
       记忆是多条（同拍多位教师退位各留一条，第 47 轮），兑付按**最近**的位置取，
       每条用后即焚：一次丢失只补偿一次，第二个同位置的候选照常排队 */
    if (stConf.nReentryMs > 0)
    {
        for (CSlot_S &stSlot : m_astSlot)
        {
            if (stSlot.nOwnerId < 0 || Subject_E::SUBJECT_TEACHER == stSlot.enSubject ||
                    stSlot.llLastSeenMs != llFrameMs || stSlot.llCandidateMs != llFrameMs ||
                    !stSlot.stFact.bInTeacherArea)
            {
                continue;
            }
            const CPointf_S stPos = stSlot.stLastPos;
            CSlot_S *pGhost = nullptr;
            for (CSlot_S &stOther : m_astSlot)
            {
                if (stOther.nOwnerId < 0 || &stOther == &stSlot ||
                        Subject_E::SUBJECT_TEACHER != stOther.enSubject ||
                        stOther.llLastSeenMs == llFrameMs)
                {
                    continue;   /* 在场的教师不是幽灵；幽灵=本拍缺席的在任教师槽 */
                }
                if (distance2d(stOther.stLastPos, stPos) <=
                        static_cast<double>(stConf.nReentryPx))
                {
                    pGhost = &stOther;   /* 表序第一个：与晋升同一条确定性取舍 */
                    break;
                }
            }
            int32_t nSrcId = -1;
            int32_t nMemIdx = -1;
            if (nullptr != pGhost)
            {
                nSrcId = pGhost->nOwnerId;
                const Subject_E enFrom = pGhost->enSubject;
                pGhost->enSubject = Subject_E::SUBJECT_UNKNOWN;
                pGhost->llCandidateMs = 0;
                --nTeacherNum;
                ++m_u64Demote;
                pushFlip(pGhost->nOwnerId, enFrom, pGhost->enSubject, llFrameMs);
            }
            else
            {
                /* 记忆路径添一座，受 max_num 约束；交接路径进出相抵不受它约束。
                   同拍可能有多位教师退位，故按位置取**最近**的一条记忆——
                   各自的继任回各自原位时不会被别人的记忆抢走 */
                if (nTeacherNum >= stConf.nMaxNum)
                {
                    continue;
                }
                nMemIdx = findReentryMem(stPos, llFrameMs, stConf.nReentryMs, stConf.nReentryPx);
                if (nMemIdx < 0)
                {
                    continue;
                }
                nSrcId = m_astReentry[static_cast<size_t>(nMemIdx)].nOwnerId;
            }
            stSlot.enSubject = Subject_E::SUBJECT_TEACHER;
            ++nTeacherNum;
            /* 只记 reentry：promote 专属慢轨（正常攒满 confirm 上位）。
               快轨也记 promote 会让"晋升"与"继承"两个读数互相包含，
               心跳里分不清这一个人是攒出来的还是继承来的 */
            ++m_u64Reentry;
            if (nMemIdx >= 0)
            {
                /* 用后即焚：一次丢失只补偿一次继承。幽灵交接没有消费记忆，不清 */
                m_astReentry[static_cast<size_t>(nMemIdx)] = CReentryMem_S();
            }
            pushFlip(stSlot.nOwnerId, Subject_E::SUBJECT_UNKNOWN, stSlot.enSubject, llFrameMs);
            dlog_info("classroom subject: 重入快轨 人=%d 继承教师身份（%s 人=%d）",
                      stSlot.nOwnerId, (nullptr != pGhost) ? "交接自幽灵槽" : "恢复自重入记忆",
                      nSrcId);
        }
    }

    for (CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId < 0 || Subject_E::SUBJECT_TEACHER != stSlot.enSubject)
        {
            continue;
        }
        if ((llFrameMs - stSlot.llLastEvidenceMs) <= static_cast<int64_t>(stConf.nReleaseMs))
        {
            continue;
        }
        const Subject_E enFrom = stSlot.enSubject;
        stSlot.enSubject = Subject_E::SUBJECT_UNKNOWN;
        stSlot.llCandidateMs = 0;
        --nTeacherNum;
        ++m_u64Demote;
        pushFlip(stSlot.nOwnerId, enFrom, stSlot.enSubject, llFrameMs);
    }

    /* ---------- 上限下调对在任者也要生效（只把住入口等于没把住） ----------
       在任教师的身份只由"证据断到 release_ms"这一条路收回，而站在讲台上的人证据不会断：
       运维按 group_stats 那条建议把 max_num 从 4 收回 1 之后，这节课仍会看到 4 个人并行
       计讲台时长（report_format 明写相加就是一个不存在的时间），直到每个人自己走开。
       退的是"最晚开始攒证据"的那批，与晋升同一条优先级（证据年龄），于是同一次下调在
       任意拍序下得到同一个结果；llCandidateMs==0（证据已断到起点作废）视作最晚——
       他本来就在出局路上，先退他不会影响还在讲课的人 */
    while (nTeacherNum > stConf.nMaxNum)
    {
        CSlot_S *pWorst = nullptr;
        int64_t llWorstKey = LLONG_MIN;
        for (CSlot_S &stSlot : m_astSlot)
        {
            if (stSlot.nOwnerId < 0 || Subject_E::SUBJECT_TEACHER != stSlot.enSubject)
            {
                continue;
            }
            const int64_t llKey = (stSlot.llCandidateMs > 0) ? stSlot.llCandidateMs : LLONG_MAX;
            /* 并列取表序靠后的：晋升取"第一个最小"，这里取"最后一个最大"，两半合起来才是
               一个完整的取舍——否则同刻上位的两位教师谁走谁留说不清 */
            if (llKey >= llWorstKey)
            {
                pWorst = &stSlot;
                llWorstKey = llKey;
            }
        }
        if (nullptr == pWorst)
        {
            break;
        }
        const Subject_E enFrom = pWorst->enSubject;
        pWorst->enSubject = Subject_E::SUBJECT_UNKNOWN;
        pWorst->llCandidateMs = 0;
        --nTeacherNum;
        ++m_u64Demote;
        pushFlip(pWorst->nOwnerId, enFrom, pWorst->enSubject, llFrameMs);
    }

    while (nTeacherNum < stConf.nMaxNum)
    {
        /* 候选里取"证据开始最早"的那个。上限小于候选数时必须有个确定且可复现的取舍：
           按证据年龄，而不是按面积（前排大框永远赢）或出现顺序（晚到的老师进不来） */
        CSlot_S *pBest = nullptr;
        for (CSlot_S &stSlot : m_astSlot)
        {
            if (stSlot.nOwnerId < 0 || Subject_E::SUBJECT_TEACHER == stSlot.enSubject)
            {
                continue;
            }
            if (stSlot.llCandidateMs <= 0 ||
                    (llFrameMs - stSlot.llCandidateMs) < static_cast<int64_t>(stConf.nConfirmMs))
            {
                continue;
            }
            if (nullptr == pBest || stSlot.llCandidateMs < pBest->llCandidateMs)
            {
                pBest = &stSlot;
            }
        }
        if (nullptr == pBest)
        {
            break;
        }
        pBest->enSubject = Subject_E::SUBJECT_TEACHER;
        ++nTeacherNum;
        ++m_u64Promote;
        pushFlip(pBest->nOwnerId, Subject_E::SUBJECT_UNKNOWN, pBest->enSubject, llFrameMs);
    }

    int32_t nCandidateNum = 0;
    for (CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId < 0)
        {
            continue;
        }
        stSlot.stFact.enSubject = stSlot.enSubject;
        if (Subject_E::SUBJECT_TEACHER != stSlot.enSubject && stSlot.llCandidateMs > 0)
        {
            ++nCandidateNum;
        }
    }

    m_nTeacherNum = nTeacherNum;
    m_nCandidateNum = nCandidateNum;
    publishStat();
}

bool CSubjectArbiter::factOf(int32_t nPersonId, CSubjectFact_S &rstOut) const
{
    const CSlot_S *pSlot = findSlot(nPersonId);
    if (nullptr == pSlot)
    {
        return false;
    }
    rstOut = pSlot->stFact;
    return true;
}

Subject_E CSubjectArbiter::subjectOf(int32_t nPersonId) const
{
    const CSlot_S *pSlot = findSlot(nPersonId);
    return (nullptr != pSlot) ? pSlot->enSubject : Subject_E::SUBJECT_UNKNOWN;
}

int32_t CSubjectArbiter::teacherId() const
{
    for (const CSlot_S &stSlot : m_astSlot)
    {
        if (stSlot.nOwnerId >= 0 && Subject_E::SUBJECT_TEACHER == stSlot.enSubject)
        {
            return stSlot.nOwnerId;
        }
    }
    return -1;
}

bool CSubjectArbiter::isTeacherCandidate(int32_t nPersonId) const
{
    const CSlot_S *pSlot = findSlot(nPersonId);
    return (nullptr != pSlot) &&
           (Subject_E::SUBJECT_TEACHER == pSlot->enSubject || pSlot->llCandidateMs > 0);
}

bool CSubjectArbiter::pollFlip(CSubjectFlip_S &rstOut)
{
    if (0 == m_u32FlipNum)
    {
        return false;
    }
    rstOut = m_astFlip[m_u32FlipHead];
    m_astFlip[m_u32FlipHead] = CSubjectFlip_S();
    m_u32FlipHead = (m_u32FlipHead + 1u) % CLASSROOM_SUBJECT_FLIP_RING;
    --m_u32FlipNum;
    return true;
}

void CSubjectArbiter::pushFlip(int32_t nPersonId, Subject_E enFrom, Subject_E enTo, int64_t llMs)
{
    if (m_u32FlipNum >= CLASSROOM_SUBJECT_FLIP_RING)
    {
        /* 环满说明一拍里翻的人超过 CLASSROOM_SUBJECT_FLIP_RING 个——判据在整体抖，
           不是偶发换人。容量取宏而注释原先写死"8"：改宏不改注释时，那个数字就和
           代码无关了，读它的人会按错误的桶数去判"这一拍算不算整体抖"。
           丢哪一条都改变不了这个结论，但没有这个计数，现场只会看到"有的门没收尾" */
        ++m_nFlipDrop;
        dlog_warn("classroom subject: 翻转环已满，累计丢弃 %d 条", m_nFlipDrop);
        return;
    }
    if (enFrom != enTo)
    {
        dlog_info("classroom subject: 人 %d 归属由 %s 变为 %s，时刻 %lldms", nPersonId,
                  subjectName(enFrom), subjectName(enTo), static_cast<long long>(llMs));
    }

    CSubjectFlip_S &rstFlip = m_astFlip[(m_u32FlipHead + m_u32FlipNum) % CLASSROOM_SUBJECT_FLIP_RING];
    rstFlip.nPersonId = nPersonId;
    rstFlip.enFrom = enFrom;
    rstFlip.enTo = enTo;
    rstFlip.llMs = llMs;
    ++m_u32FlipNum;
}

CSubjectArbiterStat_S CSubjectArbiter::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtxPub);
    return m_stStatPub;
}

void CSubjectArbiter::resetStat()
{
    m_u64Updates = 0;
    m_u64Promote = 0;
    m_u64Demote = 0;
    m_u64Reentry = 0;
    m_u64SlotSteal = 0;
    m_u64MoveMemEvict = 0;
    m_u64StandBlocked = 0;
    m_u64StandBlkNoPose = 0;
    m_u64StandBlkOccl = 0;
    m_u64StandBlkEdge = 0;
    m_u64StandBlkBlur = 0;
    m_u64StayEvidence = 0;
    m_u64PoseWitBlocks = 0;
    m_u64WitHeadMis = 0;
    m_u64WitScoreRej = 0;
    m_nFlipDrop = 0;
    m_nRoleRefError = 0;
    publishStat();
}

void CSubjectArbiter::publishStat()
{
    CSubjectArbiterStat_S stOut;
    stOut.u64Updates = m_u64Updates;
    stOut.u64Promote = m_u64Promote;
    stOut.u64Demote = m_u64Demote;
    stOut.u64Reentry = m_u64Reentry;
    stOut.u64SlotSteal = m_u64SlotSteal;
    stOut.u64MoveMemEvict = m_u64MoveMemEvict;
    stOut.u64StandBlocked = m_u64StandBlocked;
    stOut.u64StandBlkNoPose = m_u64StandBlkNoPose;
    stOut.u64StandBlkOccl = m_u64StandBlkOccl;
    stOut.u64StandBlkEdge = m_u64StandBlkEdge;
    stOut.u64StandBlkBlur = m_u64StandBlkBlur;
    stOut.u64StayEvidence = m_u64StayEvidence;
    stOut.u64PoseWitBlocks = m_u64PoseWitBlocks;
    stOut.u64WitHeadMis = m_u64WitHeadMis;
    stOut.u64WitScoreRej = m_u64WitScoreRej;
    stOut.nTeacherNum = m_nTeacherNum;
    stOut.nCandidateNum = m_nCandidateNum;
    stOut.nFlipDrop = m_nFlipDrop;
    stOut.nRoleRefError = m_nRoleRefError;
    stOut.bPodiumEmpty = m_bPodiumEmpty;
    stOut.bPodiumConfigured = m_bPodiumConfigured;
    stOut.bBoardConfigured = m_bBoardConfigured;
    stOut.bTeacherAreaEmpty = m_bTeacherAreaEmpty;
    stOut.bStudentRoleConfigured = m_bStudentRoleConfigured;

    std::lock_guard<std::mutex> stLock(m_mtxPub);
    m_stStatPub = stOut;
}

CSubjectView_S CSubjectArbiter::view() const
{
    CSubjectView_S stView;
    stView.bPodiumConfigured = m_bPodiumConfigured;
    stView.bBoardConfigured = m_bBoardConfigured;
    stView.bStudentConfigured = m_bStudentRoleConfigured;
    stView.bTeacherAreaKnown = m_bPodiumConfigured || m_bBoardConfigured;
    stView.bPodiumEmpty = m_bPodiumEmpty;
    stView.bTeacherAreaEmpty = m_bTeacherAreaEmpty;
    return stView;
}

std::string CSubjectArbiter::statLine() const
{
    const CSubjectArbiterStat_S stPub = stat();
    char szBuf[256] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "tea=%d cand=%d podium=%s cfg=%d/%d pm=%llu dm=%llu re=%llu sb=%llu st=%llu stay=%llu pw=%llu drop=%d",
                              stPub.nTeacherNum, stPub.nCandidateNum,
                              stPub.bPodiumConfigured ? (stPub.bPodiumEmpty ? "empty" : "busy") : "none",
                              stPub.bPodiumConfigured ? 1 : 0, stPub.bStudentRoleConfigured ? 1 : 0,
                              static_cast<unsigned long long>(stPub.u64Promote),
                              static_cast<unsigned long long>(stPub.u64Demote),
                              static_cast<unsigned long long>(stPub.u64Reentry),
                              static_cast<unsigned long long>(stPub.u64StandBlocked),
                              static_cast<unsigned long long>(stPub.u64SlotSteal),
                              static_cast<unsigned long long>(stPub.u64StayEvidence),
                              static_cast<unsigned long long>(stPub.u64PoseWitBlocks),
                              stPub.nFlipDrop);
    /* 截断的读数比缺失的读数危险：宁可给一行明确的 fmt_error，也不给半截数字 */
    if (nLen < 0 || static_cast<size_t>(nLen) >= sizeof(szBuf))
    {
        return std::string("subj=fmt_error");
    }
    return std::string(szBuf);
}

std::string CSubjectArbiter::statJson() const
{
    const CSubjectArbiterStat_S stPub = stat();
    std::string strJson = "{";
    strJson += "\"teacher_num\":" + std::to_string(stPub.nTeacherNum);
    strJson += ",\"candidate_num\":" + std::to_string(stPub.nCandidateNum);
    strJson += ",\"podium\":\"";
    strJson += stPub.bPodiumConfigured ? (stPub.bPodiumEmpty ? "empty" : "busy") : "none";
    strJson += "\"";
    strJson += ",\"podium_configured\":";
    strJson += stPub.bPodiumConfigured ? "true" : "false";
    strJson += ",\"board_configured\":";
    strJson += stPub.bBoardConfigured ? "true" : "false";
    strJson += ",\"teacher_area_empty\":";
    strJson += stPub.bTeacherAreaEmpty ? "true" : "false";
    strJson += ",\"student_role_configured\":";
    strJson += stPub.bStudentRoleConfigured ? "true" : "false";
    strJson += ",\"promote\":" + std::to_string(static_cast<long long>(stPub.u64Promote));
    strJson += ",\"demote\":" + std::to_string(static_cast<long long>(stPub.u64Demote));
    strJson += ",\"reentry\":" + std::to_string(static_cast<long long>(stPub.u64Reentry));
    strJson += ",\"slot_steal\":" + std::to_string(static_cast<long long>(stPub.u64SlotSteal));
    strJson += ",\"move_mem_evict\":" + std::to_string(static_cast<long long>(stPub.u64MoveMemEvict));
    strJson += ",\"stand_blocked\":" + std::to_string(static_cast<long long>(stPub.u64StandBlocked));
    strJson += ",\"stand_blk_nopose\":" + std::to_string(static_cast<long long>(stPub.u64StandBlkNoPose));
    strJson += ",\"stand_blk_occl\":" + std::to_string(static_cast<long long>(stPub.u64StandBlkOccl));
    strJson += ",\"stand_blk_edge\":" + std::to_string(static_cast<long long>(stPub.u64StandBlkEdge));
    strJson += ",\"stand_blk_blur\":" + std::to_string(static_cast<long long>(stPub.u64StandBlkBlur));
    strJson += ",\"stay_evidence\":" + std::to_string(static_cast<long long>(stPub.u64StayEvidence));
    strJson += ",\"pose_wit_blocks\":" + std::to_string(static_cast<long long>(stPub.u64PoseWitBlocks));
    strJson += ",\"wit_head_mis\":" + std::to_string(static_cast<long long>(stPub.u64WitHeadMis));
    strJson += ",\"wit_score_rej\":" + std::to_string(static_cast<long long>(stPub.u64WitScoreRej));
    strJson += ",\"flip_drop\":" + std::to_string(stPub.nFlipDrop);
    strJson += ",\"role_ref_error\":" + std::to_string(stPub.nRoleRefError);
    strJson += "}";
    return strJson;
}
}
