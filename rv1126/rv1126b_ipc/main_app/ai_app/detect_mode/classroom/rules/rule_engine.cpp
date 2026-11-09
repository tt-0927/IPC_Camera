/**
 * @FilePath     : rule_engine.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 14:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 14:57:00
 * @Description  : 规则引擎实现
 */

#include "rule_engine.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "classroom_clock.h"
#include "dlog.h"
#include "geometry.h"

namespace Classroom
{
namespace
{
/* 日志限速（#164 之后改用时间窗，见 m_llDropLogMs）：
   "这一拍该不该落一条环形队列满的 warn"——距上次落日志超过这个窗口才落。
   取 1s 而不是 1000 次：前者让速率与事件密度解耦（重载时几秒内不再连打，
   空场时也不会几千小时没一条），后者两者都不保证。 */
constexpr int64_t DROP_LOG_WINDOW_MS = 1000;

bool dropLogDue(int64_t llNowMs, int64_t &rllLastMs)
{
    if (llNowMs <= 0 || rllLastMs <= 0 || llNowMs - rllLastMs >= DROP_LOG_WINDOW_MS)
    {
        rllLastMs = llNowMs;
        return true;
    }
    return false;
}

/* 班级级（帧级）列：207 指导 / 209 互动 / 210 巡视。
   它们的判定对象是「这一帧的班级状态」，不挂在任何跟踪行上，由 feedClassLevel 用
   哨兵行 CLASSROOM_CLASS_PERSON_ID 喂门。收尾路径（disableColumn/closeActiveAll）
   必须认识这三列，否则哨兵行的事件永远不会结束 */
bool isClassLevel(Behavior_E enBehavior)
{
    return Behavior_E::TEA_GUIDING == enBehavior ||
           Behavior_E::TEA_INTERACT == enBehavior ||
           Behavior_E::TEA_PATROL == enBehavior;
}

/* 存在性判据的三态聚合（207 站 / 209 站或举手共用）：
   任一 TRUE ⇒ 成立；一行都没采到 ⇒ 不知道；每一行都明确 FALSE ⇒ 不成立；
   其余（全 UNKNOWN，或 FALSE 与 UNKNOWN 混杂）⇒ 不知道。
   混杂为什么不判 FALSE：「没测到的那几行里可能正站着人」，把 UNKNOWN 当 FALSE
   正是本子系统反复强调的那类错。门的容忍额度会兜住长期 UNKNOWN（见 duration_gate） */
TriState_E triAnyOf(uint32_t u32Num, uint32_t u32True, uint32_t u32False)
{
    if (u32True > 0)
    {
        return TriState_E::TS_TRUE;
    }
    if (0 == u32Num)
    {
        return TriState_E::TS_UNKNOWN;
    }
    if (u32False == u32Num)
    {
        return TriState_E::TS_FALSE;
    }
    return TriState_E::TS_UNKNOWN;
}
}

bool IBehaviorSampler::anyEnabled(const CConfigSnapshot_S &stSnap) const
{
    const std::vector<Behavior_E> &vecBehaviors = behaviors();
    for (size_t i = 0; i < vecBehaviors.size(); ++i)
    {
        if (stSnap.behaviorOn(vecBehaviors[i]))
        {
            return true;
        }
    }
    return false;
}

/* 采样器读上一拍生效人数：把空表与越界两种情况都归到 0，
   免得每个采样器各写一遍判空——写漏一处就是对空指针取下标 */
int32_t CBehaviorSampleIn_S::prevActiveNum(Behavior_E enBehavior) const
{
    if (nullptr == panActivePrev)
    {
        return 0;
    }
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0 || nCol >= CDurationGate::BEHAVIOR_COLUMN_NUM)
    {
        return 0;
    }
    return panActivePrev[nCol];
}

/* ============================ 装配 ============================ */

bool CRuleEngine::addSampler(IBehaviorSampler *pSampler)
{
    if (nullptr == pSampler)
    {
        return false;
    }
    if (static_cast<int>(m_avecSampler.size()) >= CLASSROOM_MAX_SAMPLERS)
    {
        dlog_error("classroom rule: 采样器容量 %d 已满，拒绝 %s",
                   CLASSROOM_MAX_SAMPLERS, pSampler->name());
        return false;
    }
    for (size_t i = 0; i < m_avecSampler.size(); ++i)
    {
        if (m_avecSampler[i] == pSampler)
        {
            /* 重复注册会让同一个行为被喂两遍：第一次开事件、第二次把中断计数改掉，
               现象是事件时长莫名偏短，比直接报错难查得多，所以在入口就挡掉 */
            dlog_warn("classroom rule: 采样器 %s 重复注册，已忽略", pSampler->name());
            return false;
        }
    }

    m_avecSampler.push_back(pSampler);

    /* 登记这一路喂哪些列：越界的行为号不进位图（采样器声明了一个门表里没有列的 id，
       本身就该由别的判据去发现，这里不能因此写越界） */
    for (const Behavior_E enBehavior : pSampler->behaviors())
    {
        const int nCol = CDurationGate::behaviorColumn(enBehavior);
        if (nCol >= 0 && nCol < CDurationGate::BEHAVIOR_COLUMN_NUM)
        {
            m_u32Declared |= (1U << static_cast<uint32_t>(nCol));
        }
    }

    const size_t nNeed = pSampler->behaviors().size();
    if (m_vecSample.size() < nNeed)
    {
        m_vecSample.resize(nNeed);   /* 只在装配期增长，运行期一次分配都没有 */
    }

    dlog_info("classroom rule: 采样器 %s 已挂接 %zu 个行为", pSampler->name(), nNeed);
    return true;
}

void CRuleEngine::applyConfig(const ConfigPtr &spSnap, int64_t llNowMs)
{
    if (!spSnap)
    {
        return;
    }

    const ConfigPtr spOld = m_spSnap;
    m_spSnap = spSnap;

    /* 换版即复查「开着没人喂」：放在首份快照的早退之前，因为开机第一份配置正是最需要
       这句话被说出来的时刻（出厂默认与运维手改的那份往往不是同一份） */
    checkUnjudgedEnabled();

    if (!spOld)
    {
        return;   /* 首份快照：没有「上一版是开着」可言 */
    }

    /* 逐列比对开关：从开到关的那一列必须当场收尾。不在这儿收尾会一直等到下一帧，
       而「关掉功能」常常正好发生在证据断流的时候（拔线、降级），那一帧可能永远不来 */
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        if (spOld->behaviorOn(enBehavior) && !spSnap->behaviorOn(enBehavior))
        {
            disableColumn(nCol, llNowMs);
        }
    }

    if (spOld->stTeacher.bEnable && !spSnap->stTeacher.bEnable)
    {
        /* 归属总开关关掉：教师那 13 列当场整体收尾。仲裁器自己也会在下一次 update() 里
           收回身份并发翻转（那条路径会再收一遍，但已经收过的是 no-op），可是"下一次 update"
           要等一个新帧——而关掉分析的时候往往正是没有帧的时候。这一处是唯一不依赖帧的收尾点。
           ★ 207/209 不在这里收尾：它们是班级级列（哨兵行、由学生画面驱动），
           只借用教师列的编号段，与「教师归属」这个总开关无关；210 依赖讲台占用（仲裁算的），
           照旧收 */
        for (int nCol = CDurationGate::STUDENT_COLUMN_NUM; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
        {
            Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
            if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
            {
                continue;
            }
            if (Behavior_E::TEA_GUIDING == enBehavior || Behavior_E::TEA_INTERACT == enBehavior)
            {
                continue;
            }
            disableColumn(nCol, llNowMs);
        }
    }
}

void CRuleEngine::verifySamplerAssembly()
{
    m_bAssembled = true;
    /* 班级级三列（207/209/210）由引擎自己喂（feedClassLevel），没有任何采样器声明它们。
       不在这里补进声明位图，checkUnjudgedEnabled 会把「开着且无人判定」的假警报
       挂在三列上——而它们恰恰是唯一由引擎保证每拍都喂的列 */
    for (const Behavior_E enBehavior : { Behavior_E::TEA_GUIDING, Behavior_E::TEA_INTERACT,
                                         Behavior_E::TEA_PATROL })
    {
        const int nCol = CDurationGate::behaviorColumn(enBehavior);
        if (nCol >= 0 && nCol < CDurationGate::BEHAVIOR_COLUMN_NUM)
        {
            m_u32Declared |= (1U << static_cast<uint32_t>(nCol));
        }
    }
    checkUnjudgedEnabled();
}

void CRuleEngine::checkUnjudgedEnabled()
{
    /* 两份输入缺一就是「还不知道」：没快照就不知道哪行开着，没收口就不知道问完了没有 */
    if (!m_spSnap || !m_bAssembled)
    {
        return;
    }

    uint32_t u32Unjudged = 0;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        if (m_spSnap->behaviorOn(enBehavior) && 0 == (m_u32Declared & (1U << static_cast<uint32_t>(nCol))))
        {
            u32Unjudged |= (1U << static_cast<uint32_t>(nCol));
        }
    }
    m_u32Unjudged = u32Unjudged;

    /* 只对新增的位喊、只对消失的位报恢复：配置是 mtime 轮询热生效的，只要这一行还开着，
       不换版也会每拍重算一遍——不加这个沿就变成一条每 100ms 刷一次的告警，
       现场很快就不再看它，而它要说的恰恰是"这个功能其实没在做" */
    const uint32_t u32New  = u32Unjudged & ~m_u32UnjudgedWarned;
    const uint32_t u32Gone = m_u32UnjudgedWarned & ~u32Unjudged;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        const uint32_t u32Bit = (1U << static_cast<uint32_t>(nCol));
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        if (u32New & u32Bit)
        {
            dlog_warn("classroom rule: 行为 %s(id=%d) 已启用但没有采样器判定，"
                      "永远不会出事件",
                      behaviorCnName(enBehavior), static_cast<int>(enBehavior));
        }
        else if (u32Gone & u32Bit)
        {
            /* 两种原因都走这一条：接上了判据，或运维把那行关掉了。日志分不出是哪样也没关系，
               要的是"上面那条告警已经不成立"这句话被说出来 */
            dlog_info("classroom rule: 行为 %s(id=%d) 的无判定告警已解除",
                      behaviorCnName(enBehavior), static_cast<int>(enBehavior));
        }
    }
    m_u32UnjudgedWarned = u32Unjudged;
}

/* ============================ 一拍判定 ============================ */

void CRuleEngine::run(const ConstFeatureFramePtr &spFrame, int64_t llNowMs)
{
    if (!m_spSnap || !m_spSnap->bEnable)
    {
        /* 总开关关闭：不喂样。已生效的事件在 applyConfig 里就收过尾了，
           这里不重复收尾是为了不把「关功能」这件正常事记成一次「感知断档」 */
        /* 关着的这一拍照样要发布：那批补发的 END 是 applyConfig / engine 的 shutdownActive
           在这条 return **之前**写进计数与逐列生效数的，不发布就冻在关闭前的读数上——
           而且只要开关不回开就再也没有一次发布，心跳与 statusJson 于是永远报着
           「关了仍有 N 条行为生效」。下面两条 return 都发布，只有这一条漏了 */
        publishStat();
        return;
    }

    /* 断档收尾：帧时间轴上没有「停摆」这件事，只能用真实时间发现（见 run 的注释） */
    if (m_bFedOnce &&
        (llNowMs - m_llLastRunWallMs) > static_cast<int64_t>(CLASSROOM_FEATURE_TTL_MS) * CLASSROOM_RULE_STALL_TIMES)
    {
        const int32_t nClosed = closeActiveAll(m_llLastRunMs);
        if (nClosed > 0)
        {
            ++m_stStat.u64StallClose;
            dlog_warn("classroom rule: 感知停滞 %lldms，按证据时刻收尾 %d 个进行中事件",
                      static_cast<long long>(llNowMs - m_llLastRunMs), nClosed);
        }
        /* 参照点前推到此刻：不推就会每拍重扫一遍 60×25 的门表且一无所获。
           真正的下一次收尾要再等满一个断档窗口，这正是本规则想要的节奏 */
        m_llLastRunMs = llNowMs;
        m_llLastRunWallMs = llNowMs;
    }

    if (!spFrame || spFrame->llMonotonicMs <= 0)
    {
        /* 没时间基就无法喂门：差值会算出天文数字，一帧的脏数据能让所有事件当场超时结束 */
        publishStat();
        return;
    }
    if (m_bFedOnce && spFrame->u32Seq == m_u32LastSeq)
    {
        /* 同一帧不重喂：节拍 100ms、感知 200ms，多数拍没有新证据。
           重喂不会算错（门有采样间隔挡着），但会把节流计数灌成一坨没有意义的噪声 */
        publishStat();
        return;
    }

    const int64_t llFrameMs = spFrame->llMonotonicMs;
    const int64_t llCostBegin = classroomNowUs();

    ensureRegions(spFrame->stGeom);

    /* 归属先于一切判定：教师那 11 列的总闸就是这个仲裁结果（没有归属就没有教师行为），
       而学生那 5 列也要靠它把「已被认定是老师的人」挡出去。放在区域表重建之后是必须的——
       仲裁读的就是这张表，早一步用的是上一拍的讲台位置 */
    m_stSubject.update(*spFrame, m_stRegion, *m_spSnap, llFrameMs);
    applySubjectFlips(&*spFrame, llFrameMs, spFrame->u32Seq);

    ++m_stStat.u64Runs;
    m_u32LastSeq = spFrame->u32Seq;
    m_bFedOnce = true;
    m_llLastRunMs = llFrameMs;
    m_llLastRunWallMs = llNowMs;
    m_stStat.nRegionNum = m_stRegion.regionNum();

    int32_t nRowNum = (spFrame->nPersonNum > CLASSROOM_MAX_PERSONS)
                      ? CLASSROOM_MAX_PERSONS : spFrame->nPersonNum;
    /* 行数不得超过本帧真正持有的行：同一文件另两处循环已按 size 自钳，归属快照那一段
       原来漏了。畸形帧（nPersonNum > vecPersons.size()）会越界读——读到的是堆上的残留
       对象，于是「上一拍的归属」被当成本拍事实，静默错判。与 subject_arbiter.update 的
       min(nPersonNum, size) 同一口径，不新增拒绝路径、不改报文 */
    const int32_t nVecNum = static_cast<int32_t>(spFrame->vecPersons.size());
    if (nRowNum > nVecNum)
    {
        nRowNum = nVecNum;
    }
    if (nRowNum > m_stStat.nTrackMax)
    {
        m_stStat.nTrackMax = nRowNum;
    }

    m_nSeenNextNum = 0;
    m_vecPhoneRecheck.clear();   /* 空名单也要给：上一拍的低头不能这一拍还在要 NPU */
    m_nPoseUrgNum = 0;           /* 同上：上一拍「在等」的人不能这一拍还在占名额 */

    /* 教师候选与在任教师的姿态加急（#93-5）：身份靠姿态不断重认——姿态一丢就是
       「重认一个确认时长 + 行为列假分裂」。普通人的续期挑选层 EXPIRE 档已经在兜，
       但那档里教师和后排挤同一个 500，插队席一比面积就输——这正是要治的病。
       过 TTL 七成未过期就先挂 TEACHER 档（550），排在普通续期之前；又不压过真正
       在攒分的（600+）与生效中的（1000）。isTeacherCandidate 同时覆盖在任者
       （槽上 enSubject==TEACHER 即真）与攒证据的候选，两边同一档、先到先得。
       必须在这里喂、在逐行判定之前：喂门那一路只会给「有行为在等」的人，候选
       大概率还没有任何行为在攒，等判定完再喂就是空谈。
       只挂有骨骼记忆的（llPoseMs>0）：从没做过的候选冷启动本来就有「最久没做优先」
       兜着，把冷启动也挂成加急，插队限席那一位会被永远轮不到冷启动的人占死 */
    for (int32_t i = 0; i < nRowNum && i < static_cast<int32_t>(spFrame->vecPersons.size()); ++i)
    {
        const CPersonFeature_S &stRow = spFrame->vecPersons[static_cast<size_t>(i)];
        if (stRow.nPersonId < 0 || stRow.llPoseMs <= 0 ||
            !m_stSubject.isTeacherCandidate(stRow.nPersonId))
        {
            continue;
        }
        const int64_t llTtlMs = static_cast<int64_t>(spFrame->poseTtlMs());
        if (llTtlMs > 0 && (llFrameMs - stRow.llPoseMs) >
                               (llTtlMs * CLASSROOM_POSE_URG_STALE_PERMILLE) / 1000)
        {
            mergePoseUrgency(stRow.nPersonId, CLASSROOM_POSE_URG_TEACHER);
        }
    }

    /* 原「巡视/指导的姿态需求」加急段整段删除。207/209 已改为班级级判据
       （学生画面有人站/举手，见 feedClassLevel），210 只看讲台占用——三列都不再需要
       教师行自己的骨骼，继续挂加急只会从真正需要姿态的教师身份重认那里抢名额 */

    /* 逐行归属快照（给要看**别人**那一行的判据用，见 CBehaviorSampleIn_S::penRowSubject）。
       必须排在仲裁与边沿处理之后：applySubjectFlips 会改槽位表，取在它之前会拿到上一拍的归属。
       行数以 nRowSubjectNum 为准，第 nRowNum 行之后的值是上一拍的残留 */
    for (int32_t i = 0; i < nRowNum; ++i)
    {
        const CPersonFeature_S &stRow = spFrame->vecPersons[static_cast<size_t>(i)];
        CSubjectFact_S stRowFact;
        m_anRowSubject[i] = (stRow.nPersonId >= 0 && m_stSubject.factOf(stRow.nPersonId, stRowFact))
                                ? static_cast<int8_t>(stRowFact.enSubject)
                                : static_cast<int8_t>(Subject_E::SUBJECT_UNKNOWN);
    }

    /* 名单这一列同时是 G_OTHER 的唯一需求信号（引擎按它非空与否决定要不要让 PlayPhoneDetect
       驻留），所以"没人要这条复核"必须表现为空名单：两行都关着或现场关了复核时还往名单里写人，
       等于为一条不会上报的行为常驻一整组模型。判据本身照旧跑（104 那条独立行为不受影响），
       只是不再为它花一次裁剪与一次推理。
       106 与 108 **共用这一次复核**：两条的形状完全相同，分家依据只有"图里有没有手机"，
       所以只开 108 也得要复核——否则阅读永远 UNKNOWN，正是 §1.19 那条"配了、开了、永远零事件" */
    /* #187：只开教师玩手机（205）时也得为它花一次复核——否则 205 永远拿不到复核结论，
       只能 fail-open 把所有「低头看教案」判成玩手机。判据本体在 rule_engine.h，
       提出来是为了让主机用例能直接打红（run() 要整台设备才跑得起来） */
    const bool bPhoneWant = CRuleEngine::phoneRecheckWanted(
        m_spSnap->stPhoneRecheck.bEnable,
        m_spSnap->behaviorOn(Behavior_E::STU_PHONE),
        m_spSnap->behaviorOn(Behavior_E::STU_READING),
        m_spSnap->behaviorOn(Behavior_E::TEA_PHONE));

    CBehaviorSampleIn_S stIn;
    stIn.pstFrame = &*spFrame;
    stIn.pstSnap = &*m_spSnap;
    stIn.pstRegion = &m_stRegion;
    stIn.stSubjectView = m_stSubject.view();
    stIn.panActivePrev = m_anActivePrev;
    stIn.penRowSubject = m_anRowSubject;
    stIn.nRowSubjectNum = nRowNum;
    stIn.llNowMs = llFrameMs;

    /* 本拍班级级事实（207/209 的原始计数）：在逐行派发里就地累加，行循环之后聚合 */
    CClassFacts_S stClass;

    for (int32_t i = 0; i < nRowNum && i < static_cast<int32_t>(spFrame->vecPersons.size()); ++i)
    {
        const CPersonFeature_S &stRow = spFrame->vecPersons[static_cast<size_t>(i)];
        if (stRow.nPersonId < 0)
        {
            continue;   /* 空行：感知层没写满，不是「这里有个 0 号人」 */
        }

        markSeen(stRow.nPersonId, stRow.nSeatId, stRow.stHeadBox);
        stIn.pstPerson = &stRow;
        stIn.nSelfIdx = i;

        /* 逐行取归属事实：槽位是本拍第一遍刚认领的，正常一定取得到；
           取不到（行 ID 非法、槽被挤掉）就传 null，由采样器退回帧内那一列 */
        CSubjectFact_S stFact;
        stIn.pstSubject = m_stSubject.factOf(stRow.nPersonId, stFact) ? &stFact : nullptr;

        /* 本行的站立/举手原判先收在这里，行循环末尾统一折算成 207/209 的班级事实：
           两页的区域、停留门、置信门各不相同，落在采样器派发处就地累加会把三套口径
           混成一份计数（同一行在 207 成立、在 209 可能不成立） */
        CBehaviorSample_S stRowStand;
        stRowStand.enReason = SampleReason_E::SR_NO_POSE;
        CBehaviorSample_S stRowRaise;
        stRowRaise.enReason = SampleReason_E::SR_NO_POSE;
        bool bHaveStand = false;
        bool bHaveRaise = false;

        /* 派发不看归属：引擎对每个采样器无条件调用，归属闸由采样器自己的第一道闸执行
           （student_behavior.cpp / teacher_behavior.cpp 的「---- 1) 归属闸」）。
           T18 曾试过在派发处前置同一判据，闸门当场打红——testEngineShutdownClearsState
           钉的就是这份契约：addSampler 是公开扩展点，假采样器对晋升行也必须被调到
           （真实采样器在闸后空转的代价是每行一次虚调用 + 整表铺 UNKNOWN，两个真实采样器
           的归属闸都排在任何骨骼/几何工作之前，本来就没有多少可省） */

        for (size_t s = 0; s < m_avecSampler.size(); ++s)
        {
            IBehaviorSampler *pSampler = m_avecSampler[s];
            const std::vector<Behavior_E> &vecBehaviors = pSampler->behaviors();
            if (vecBehaviors.empty() || !pSampler->anyEnabled(*m_spSnap))
            {
                /* 整路都关着就不进：省一次骨骼遍历。已生效的事件在 applyConfig 换入时就收尾了 */
                continue;
            }

            const size_t nNeed = vecBehaviors.size();
            m_vecSample.resize(nNeed);
            pSampler->samplePerson(stIn, m_vecSample);
            if (m_vecSample.size() != nNeed)
            {
                /* 采样器改写了出参长度＝契约被破坏。补齐成 UNKNOWN 而不是让它少出的那几项
                   静默沿用上一拍的值——沿用会把三拍前的判定当成本拍判定喂进门里 */
                dlog_error("classroom rule: 采样器 %s 返回 %zu/%zu 个样本",
                           pSampler->name(), m_vecSample.size(), nNeed);
                m_vecSample.resize(nNeed);
            }

            for (size_t j = 0; j < nNeed; ++j)
            {
                feedOne(stRow, vecBehaviors[j], m_vecSample[j], llFrameMs, spFrame->u32Seq);

                /* 班级级事实（207/209）：只收 101/102 的**原判**，折算留给本行末尾。
                   刻意不受这两列自己的开关与时长门影响：开关决定的是 101/102 出不出事件，
                   而「这一帧有没有人站着」用的是同一份判据事实（见 feedClassLevel） */
                if (Behavior_E::STU_STAND_UP == vecBehaviors[j])
                {
                    stRowStand = m_vecSample[j];
                    bHaveStand = true;
                }
                else if (Behavior_E::STU_RAISE_HAND == vecBehaviors[j])
                {
                    stRowRaise = m_vecSample[j];
                    bHaveRaise = true;
                }

                /* T39/T40 背身链路探针（只记日志，不改判定）：teacher 背身列在大头框行上落一条。
                   TRUE 与证人否决（SR_MODEL_NEG=否决指纹）逐条落；其余态按 (判,成因) 变化沿
                   + 10s 心跳落。分数全带上：肩是新体门承重证据，
                   眼分与鼻x相对位是 T40 鼻幻觉的判别数据 */
                if (Behavior_E::TEA_TURN_BACK == vecBehaviors[j] &&
                    stRow.stHeadBox.height() >= 60.0f &&
                    nullptr != stIn.pstSubject &&
                    Subject_E::SUBJECT_TEACHER == stIn.pstSubject->enSubject)
                {
                    const CBehaviorSample_S &stTb = m_vecSample[j];
                    const bool bInBoard = (nullptr != stIn.pstSubject) && stIn.pstSubject->bInBoard;
                    const bool bVeto = (SampleReason_E::SR_MODEL_NEG == stTb.enReason);
                    const bool bEdge = (stTb.enState != m_enTbProbeLastState) ||
                                       (stTb.enReason != m_enTbProbeLastReason);
                    if (TriState_E::TS_TRUE == stTb.enState || bVeto || bEdge ||
                        llFrameMs - m_llTbProbeMs >= 10000)
                    {
                        m_llTbProbeMs = llFrameMs;
                        m_enTbProbeLastState = stTb.enState;
                        m_enTbProbeLastReason = stTb.enReason;
                        float fNose = 0.0f, fTop = 0.0f, fLSh = 0.0f, fRSh = 0.0f;
                        float fLEye = 0.0f, fREye = 0.0f;
                        CPointf_S stNose, stTop, stLSh, stRSh, stLEye, stREye;
                        (void)posePoint(&stRow.stSkeleton, PosePoint_E::POSE_NOSE, stNose, fNose);
                        (void)posePoint(&stRow.stSkeleton, PosePoint_E::POSE_HEAD_TOP, stTop, fTop);
                        (void)posePoint(&stRow.stSkeleton, PosePoint_E::POSE_L_SHOULDER, stLSh, fLSh);
                        (void)posePoint(&stRow.stSkeleton, PosePoint_E::POSE_R_SHOULDER, stRSh, fRSh);
                        (void)posePoint(&stRow.stSkeleton, PosePoint_E::POSE_L_EYE, stLEye, fLEye);
                        (void)posePoint(&stRow.stSkeleton, PosePoint_E::POSE_R_EYE, stREye, fREye);
                        /* v3 订正：v2 的「鼻x」实际落的是右眼 x（stPt 被六连调覆写，posePoint
                           每次都写 stOut） */
                        const float fBoxW = stRow.stHeadBox.width();
                        const float fBoxH = stRow.stHeadBox.height();
                        const float fNoseX = (fBoxW > 1.0f && fNose > 0.0f)
                                             ? (stNose.fX - stRow.stHeadBox.fX1) / fBoxW : -1.0f;
                        const float fNoseY = (fBoxH > 1.0f && fNose > 0.0f)
                                             ? (stNose.fY - stRow.stHeadBox.fY1) / fBoxH : -1.0f;
                        const float fEyeLx = (fBoxW > 1.0f && fLEye > 0.0f)
                                             ? (stLEye.fX - stRow.stHeadBox.fX1) / fBoxW : -1.0f;
                        const float fEyeRx = (fBoxW > 1.0f && fREye > 0.0f)
                                             ? (stREye.fX - stRow.stHeadBox.fX1) / fBoxW : -1.0f;
                        const float fEyeY = (fBoxH > 1.0f && fLEye > 0.0f && fREye > 0.0f)
                                            ? ((stLEye.fY + stREye.fY) * 0.5f - stRow.stHeadBox.fY1) / fBoxH : -1.0f;
                        dlog_info("classroom rule: 背身探针 人=%d 头框=[%.0f,%.0f,%.0f,%.0f] 判=%c 成因=%d 在板=%d"
                                  " 骨=%.2f 肩L=%.2f 肩R=%.2f 头顶=%.2f 鼻=%.2f 眼L=%.2f 眼R=%.2f"
                                  " 鼻x=%.2f 鼻y=%.2f 眼Lx=%.2f 眼Rx=%.2f 眼y=%.2f",
                                  stRow.nPersonId,
                                  stRow.stHeadBox.fX1, stRow.stHeadBox.fY1,
                                  stRow.stHeadBox.fX2, stRow.stHeadBox.fY2,
                                  (TriState_E::TS_TRUE == stTb.enState) ? 'T' :
                                  (TriState_E::TS_FALSE == stTb.enState) ? 'F' : 'U',
                                  static_cast<int>(stTb.enReason), bInBoard ? 1 : 0,
                                  stRow.stSkeleton.fScore, fLSh, fRSh, fTop, fNose,
                                  fLEye, fREye, fNoseX, fNoseY, fEyeLx, fEyeRx, fEyeY);
                    }
                }
                /* 低头成立 → 下一拍值得为他裁一张复核「手里是不是手机」。挂在这里而不是
                   isActive(104) 上：104 那一行的开关与门限都不该影响 106 有没有复核 */
                /* #187：教师行这一拍产出的是 203 教师低头，不是 104。名单过去只认 104，
                   于是教师行永远不入队、205 永远走 fail-open。两列都收：同一张"低头×手在
                   身前"的形状，只是主体不同 */
                if (bPhoneWant && CRuleEngine::phoneRecheckColumn(vecBehaviors[j]) &&
                    TriState_E::TS_TRUE == m_vecSample[j].enState &&
                    m_vecPhoneRecheck.size() < static_cast<size_t>(CLASSROOM_MAX_PERSONS))
                {
                    m_vecPhoneRecheck.push_back(stRow.nPersonId);
                }
            }
        }

        accumulateClassFacts(stClass, stRow, stRowStand, bHaveStand,
                             stRowRaise, bHaveRaise, llFrameMs);
    }

    /* 本拍没再出现的停留计时只保留 CLASSROOM_STAY_GAP_MS 的宽限：不按 TTL 过期是因为帧时
       停摆时 TTL 永远不过期，一个人走了十分钟再回来会被当成「一直站着」；但按「本拍必须在场」
       清又会因一次漏检把已经站了几秒的人清零（见 CLASSROOM_STAY_GAP_MS 的注释）。
       时刻倒退的条目一并清掉——那样的 llSinceMs 已经算不出有意义的时长 */
    m_vecStayTrack.erase(std::remove_if(m_vecStayTrack.begin(), m_vecStayTrack.end(),
                                        [llFrameMs](const CStayTrack_S &stTrk) {
                                            return llFrameMs < stTrk.llLastMs ||
                                                   llFrameMs - stTrk.llLastMs > CLASSROOM_STAY_GAP_MS;
                                        }),
                         m_vecStayTrack.end());

    /* 班级级三列（207/209/210）：行循环之后、退役之前。三列的主体是整帧（哨兵行 0），
       207/209 要等本拍所有学生行都判完才能汇总；210 读的讲台占用是仲裁本拍第一遍数的 */
    /* #174：记下「上一次画面里有人」的帧时，供 210 判空场（黑板/遮挡/断流时没人可判） */
    if (nRowNum > 0 && llFrameMs > m_llLastSubjectMs)
    {
        m_llLastSubjectMs = llFrameMs;
    }
    feedClassLevel(stClass, llFrameMs, spFrame->u32Seq);

    retireGone(llFrameMs, spFrame->u32Seq);

    for (int32_t i = 0; i < m_nSeenNextNum; ++i)
    {
        m_astSeen[i] = m_astSeenNext[i];
    }
    m_nSeenNum = m_nSeenNextNum;

    /* 本拍人数最后整体换给下一拍：换点放在 retireGone 之后，是因为收尾也在这里改计数，
       采样器要看到的是「这一拍全部判完」的结果，而不是判到一半的中间态 */
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_anActivePrev[nCol] = m_anActiveNum[nCol];
    }

    m_stStat.llLastCostUs = classroomNowUs() - llCostBegin;
    publishStat();
}

/* ============================ 逐 (人 × 行为) 喂门 ============================ */

/* 姿态紧急度分档（P0-1）：生效中的最高档、攒时长接近阈值的按进度给分。
   从 feedOne 抽出来是因为 #92 冻结路径与喂门路径都要它——「在等证据」的拍
   不管有没有喂门，都在为下一个姿态名额定价 */
void CRuleEngine::mergePoseUrgencyOf(const CPersonFeature_S &stRow, const CBehaviorSample_S &stFinal,
                                     const CGateState_S &stBefore, int64_t llNowMs, Behavior_E enBehavior)
{
    int32_t nUrg = 0;
    /* 用喂门前那格状态判档，不能用 getState 的返回值 —— 那是「这人在门里有行」
       而不是「事件正生效」（END 补时长靠 llActiveSinceMs 哨兵兜底，这里没有哨兵可兜，
       直接用会每拍都给满分）。也不用门的 llHeldMs：未生效的尝试它不填（只在生效中
       或刚激活时填），攒时长的进度要用本轮起点 llTrueSinceMs 自己算 */
    if (stBefore.bActive)
    {
        /* 生效中的行为断了证据：事件正靠容忍额度撑，断了就是一次假拆 */
        nUrg = CLASSROOM_POSE_URG_ACTIVE;
    }
    else if (GATE_TIME_NONE != stBefore.llTrueSinceMs)
    {
        /* 未生效但有活着的尝试：离激活阈值越近越优先（600 起步 ~ 999 封顶） */
        const int32_t nMinMs = m_spSnap->gateOf(enBehavior).nMinDurationMs;
        const int64_t llHeld = llNowMs - stBefore.llTrueSinceMs;
        int32_t nPermille = (nMinMs > 0)
                                ? static_cast<int32_t>((llHeld * 1000) / nMinMs)
                                : 1000;
        if (nPermille > 1000)
        {
            nPermille = 1000;
        }
        nUrg = CLASSROOM_POSE_URG_ACC_BASE + (CLASSROOM_POSE_URG_ACC_SPAN * nPermille) / 1000;
    }
    /* 两档都不是（没有尝试也没有事件）：这个人只是“还没做过”，
       不给分——冷启动的公平排序交给挑选层原来的「最久没做优先」 */
    if (nUrg > 0)
    {
        mergePoseUrgency(stRow.nPersonId, nUrg);
    }
}

void CRuleEngine::mergePoseUrgency(int32_t nPersonId, int32_t nUrgency)
{
    if (nPersonId < 0 || nUrgency <= 0)
    {
        return;
    }
    for (int32_t i = 0; i < m_nPoseUrgNum; ++i)
    {
        if (m_astPoseUrg[i].nPersonId == nPersonId)
        {
            if (nUrgency > m_astPoseUrg[i].nUrgency)
            {
                m_astPoseUrg[i].nUrgency = nUrgency;
            }
            return;
        }
    }
    if (m_nPoseUrgNum >= CLASSROOM_MAX_PERSONS)
    {
        return;   /* 条数被人数上限封顶，到不了这里；到得了说明行数契约先坏了 */
    }
    m_astPoseUrg[m_nPoseUrgNum].nPersonId = nPersonId;
    m_astPoseUrg[m_nPoseUrgNum].nUrgency = nUrgency;
    ++m_nPoseUrgNum;
}

void CRuleEngine::feedOne(const CPersonFeature_S &stRow, Behavior_E enBehavior,
                          const CBehaviorSample_S &stSample, int64_t llNowMs, uint32_t u32Seq,
                          bool bSkipRegionGate)
{
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0)
    {
        return;   /* 采样器声明了不属于本子系统编号的行为：装配错误，不进门 */
    }
    /* 哨兵行（班级级 207/209/210）：这个人没有骨骼、也不该占姿态名额。
       后两处 mergePoseUrgencyOf 见到它会把 personId=0 塞进下一拍的姿态加急表，
       而调度层不认识 0 号人——名额被一个不存在的对象长期占住 */
    const bool bClassRow = (CLASSROOM_CLASS_PERSON_ID == stRow.nPersonId);
    /* 整列被配置关掉时在这里就返回，不进门也不计 throttled。
       默认配置下学生 10 列有 7 列关、教师还有手机列关，逐列照喂会让 u64Throttled
       （它的本义是"未到采样间隔"）被每拍每行灌一次，读它的人会得出"该调采样间隔"的
       错误结论——而真实原因是"这些列本来就没开"。拆成 u64ColumnOff 之后
       "节拍太快 / 人太多 / 列没开"三种未采纳各有自己的计数。 */
    if (!m_spSnap->gateOf(enBehavior).bEnable)
    {
        ++m_stStat.u64ColumnOff;
        return;
    }

    CBehaviorSample_S stFinal = stSample;

    /* 区域闸在喂门之前，且对 UNKNOWN 同样生效：区域是纯几何事实，
       不需要姿态证据就能成立，反过来「人在区域外」也不该因为证据缺失而被抹掉。
       班级级哨兵行跳过这道闸：它的人头框面积为 0，任何区域都会把它判成 SR_NO_FRAME；
       而这些列要的区域事实（讲台占用 / 整帧站立）在 feedClassLevel 里已经用掉了 */
    const CRegionRef_S &stRef = m_spSnap->regionRefOf(enBehavior);
    /* #86 表情列的角色回落：112 是唯一不吃骨骼证据的学生行为（其余列没姿态全是
       UNKNOWN，鬼影轨天然出不了事件），感知在静物上的幻影脸唯一走得完的链就是它。
       Web 给 112 单独画了区域就按配置走；没画而学生席画了时，头框不在学生席的行
       不得替学生报表情。学生席也没画维持不设闸：「画没画」是现场对精度的显式取舍，
       缺省不得偷偷收紧 */
    const std::vector<int32_t> *pvecRegionIds = &stRef.vecRegionIds;
    if (pvecRegionIds->empty() && Behavior_E::STU_EXPRESSION == enBehavior)
    {
        const std::vector<int32_t> &vecStuIds =
            m_stRegion.idsOfRole(RegionRole_E::REGION_ROLE_STUDENT);
        if (!vecStuIds.empty())
        {
            pvecRegionIds = &vecStuIds;
        }
    }
    if (!bSkipRegionGate && !pvecRegionIds->empty())
    {
        if (stRow.stHeadBox.area() <= 0.0f)
        {
            /* 框本身是坏的（面积为 0）：这时「不在区域内」是框的问题不是人的位置问题，
               记成缺帧而不是区域否决，否则 SR_REGION_MISS 会背下感知层脏数据的锅 */
            stFinal.enState = TriState_E::TS_UNKNOWN;
            stFinal.enReason = SampleReason_E::SR_NO_FRAME;
        }
        else
        {
            bool bConfigError = false;
            if (!m_stRegion.inAnyRegion(*pvecRegionIds, stRef.fMinCoverage, stRow.stHeadBox, bConfigError))
            {
                if (bConfigError)
                {
                    stFinal.enState = TriState_E::TS_UNKNOWN;
                    stFinal.enReason = SampleReason_E::SR_REGION_MISS;
                    ++m_stStat.u64RegionMiss;
                }
                else
                {
                    stFinal.enState = TriState_E::TS_FALSE;
                    stFinal.enReason = SampleReason_E::SR_REGION_MISS;
                    ++m_stStat.u64RegionReject;
                }
            }
        }
    }

    /* 先取本轮起点：同一次 feed 可能既结束旧事件又开启新事件（最短时长为 0 的行为必然如此，
       例如教师跟踪），届时结果里的 llHeldMs 已被新事件覆盖，只有喂之前的状态能算准上一场时长 */
    CGateState_S stBefore;
    const bool bWasActive = m_stGate.getState(stRow.nPersonId, enBehavior, stBefore);

    /* #92 姿态调度空窗冻结：SR_NO_POSE 是「这一拍没有骨骼证据可判」（守护降频、轮转空窗、
       名额没排到），不是「行为不成立」。照旧喂 UNKNOWN 的话，门里的容忍额度会被调度空窗
       一拍拍吃掉——降频期一拍拖长到秒级，500ms 的容忍额度一拍就爆，事件被切成好几段。
       冻结 = 这一拍不喂门，门里的时钟原样停着；真实离场由 retireGone 当拍补 END、
       感知停摆由 run 的断档分支收，两处都有比容忍额度更准的时间常数，不经过这里。
       紧急度照算：正是「在等证据」的这些拍决定他下一个姿态名额值多少分 */
    if (SampleReason_E::SR_NO_POSE == stFinal.enReason)
    {
        /* 冻结时钟 = 距上一次真正被采纳的采样过了多久：连续冻结拍它一直在涨，
           且不需要额外状态（llLastSampleMs 只有喂门才动，本支恰恰不喂） */
        const bool bFreezeOk = !bWasActive ||
                               (GATE_TIME_NONE == stBefore.llLastSampleMs) ||
                               ((llNowMs - stBefore.llLastSampleMs) < CLASSROOM_RULE_POSE_FREEZE_MS);
        if (bFreezeOk)
        {
            ++m_stStat.u64PoseFrozen;
            if (!bClassRow)
            {
                mergePoseUrgencyOf(stRow, stFinal, stBefore, llNowMs, enBehavior);
            }
            return;
        }
        /* 冻结顶到上限：姿态组多半是真死了（驻留再不回来）。按老口径喂一次 UNKNOWN
           交还容忍额度，生效中的事件由此结束；下一次喂门会重置冻结时钟 */
    }

    const CGateResult_S stResult = m_stGate.feed(stRow.nPersonId, enBehavior, stFinal.enState,
                                                 llNowMs, m_spSnap->gateOf(enBehavior), stFinal.nAux);

    if (!stResult.bAccepted)
    {
        ++m_stStat.u64Throttled;   /* 未到采样间隔：本拍什么都不改，包括成因计数 */
        return;
    }

    ++m_stStat.u64Samples;
    if (TriState_E::TS_TRUE == stFinal.enState)
    {
        ++m_stStat.u64True;
    }
    else if (TriState_E::TS_FALSE == stFinal.enState)
    {
        ++m_stStat.u64False;
    }
    else
    {
        ++m_stStat.u64Unknown;
    }
    ++m_stStat.au64Reason[static_cast<int>(stFinal.enReason)];

    /* 姿态紧急度（P0-1 接近激活阈值优先）：采样器说「这次判定缺骨骼证据」时，
       这个人下一个姿态名额值多少分由门状态定。写在这里而不是收尾再扫门表：
       成因、喂门前状态、阈值参数只有这一行同时在手（引擎不需要懂判据，只需要转发「谁在等」）。
       门里已在攒的时长随 UNKNOWN 持续被容忍额度消耗，所以越早补上证据，
       「事件不重启」就越便宜——这就是需求 10 在调度侧的那一半 */
    if (SampleReason_E::SR_NO_POSE == stFinal.enReason && !bClassRow)
    {
        mergePoseUrgencyOf(stRow, stFinal, stBefore, llNowMs, enBehavior);
    }

    if (stResult.bDeactivated)
    {
        CBehaviorEvent_S stEvent;
        stEvent.nPersonId = stRow.nPersonId;
        stEvent.nSeatId = stRow.nSeatId;
        stEvent.enBehavior = enBehavior;
        stEvent.bBegin = false;
        stEvent.llMs = llNowMs;
        /* 起点必须是真实时刻：判据用哨兵而不是「> 0」——时基由调用方传入，回放可以从 0 起，
           拿正负当"有没有起点"会把一节 0 点开始的课算成没有起点，于是改用 feed 给的时间 */
        stEvent.llHeldMs = (bWasActive && GATE_TIME_NONE != stBefore.llActiveSinceMs)
                           ? (llNowMs - stBefore.llActiveSinceMs) : stResult.llHeldMs;
        stEvent.stHeadBox = stRow.stHeadBox;
        stEvent.u32FeatureSeq = u32Seq;
        stEvent.nExprClass = stBefore.nAux;   /* END 带最后维持事件的类别（#88） */
        pushEvent(stEvent);

        if (m_anActiveNum[nCol] > 0)
        {
            --m_anActiveNum[nCol];
        }
        ++m_stStat.u64EventEnd;
    }

    if (stResult.bActivated)
    {
        CBehaviorEvent_S stEvent;
        stEvent.nPersonId = stRow.nPersonId;
        stEvent.nSeatId = stRow.nSeatId;
        stEvent.enBehavior = enBehavior;
        stEvent.bBegin = true;
        stEvent.llMs = llNowMs - stResult.llHeldMs;   /* 回推到本轮第一个 TRUE，与门内口径一致 */
        stEvent.llHeldMs = stResult.llHeldMs;
        stEvent.stHeadBox = stRow.stHeadBox;
        stEvent.u32FeatureSeq = u32Seq;
        stEvent.nExprClass = stFinal.nAux;   /* BEGIN 带开门那次判定的类别（#88） */
        pushEvent(stEvent);

        ++m_anActiveNum[nCol];
        if (m_anActiveNum[nCol] > m_stStat.nActiveMax)
        {
            m_stStat.nActiveMax = m_anActiveNum[nCol];
        }
        ++m_stStat.u64EventBegin;
    }
}

/* ============================ 班级级判定（207/209/210） ============================ */

void CRuleEngine::accumulateClassFacts(CClassFacts_S &rstFacts, const CPersonFeature_S &stRow,
                                       const CBehaviorSample_S &stStand, bool bHaveStand,
                                       const CBehaviorSample_S &stRaise, bool bHaveRaise,
                                       int64_t llFrameMs)
{
    /* 页 14/15 的「绘制区域」：没画 = 不限区域（与区域配置的全局口径一致）；
       框本身无效（面积 0）记 no_frame 而不是「区域外」——那是感知层脏框，不是位置事实；
       区域外记 FALSE，与逐行行为的区域闸同一语义（这一行不在本页关心的地方） */
    auto regionFactOf = [this, &stRow](Behavior_E enBehavior,
                                       const CBehaviorSample_S &stSample) -> CBehaviorSample_S {
        CBehaviorSample_S stOut = stSample;
        const CRegionRef_S &stRef = m_spSnap->regionRefOf(enBehavior);
        if (stRef.vecRegionIds.empty())
        {
            return stOut;
        }
        if (stRow.stHeadBox.area() <= 0.0f)
        {
            stOut.enState = TriState_E::TS_UNKNOWN;
            stOut.enReason = SampleReason_E::SR_NO_FRAME;
            return stOut;
        }
        bool bConfigError = false;
        if (!m_stRegion.inAnyRegion(stRef.vecRegionIds, stRef.fMinCoverage,
                                    stRow.stHeadBox, bConfigError))
        {
            if (bConfigError)
            {
                stOut.enState = TriState_E::TS_UNKNOWN;
                stOut.enReason = SampleReason_E::SR_REGION_MISS;
                ++m_stStat.u64RegionMiss;
            }
            else
            {
                stOut.enState = TriState_E::TS_FALSE;
                stOut.enReason = SampleReason_E::SR_REGION_MISS;
                ++m_stStat.u64RegionReject;
            }
        }
        return stOut;
    };

    auto countInto = [](uint32_t &ru32Num, uint32_t &ru32True, uint32_t &ru32False,
                        SampleReason_E &renUnknown, const CBehaviorSample_S &stSample) {
        ++ru32Num;
        if (TriState_E::TS_TRUE == stSample.enState)
        {
            ++ru32True;
        }
        else if (TriState_E::TS_FALSE == stSample.enState)
        {
            ++ru32False;
        }
        else
        {
            renUnknown = stSample.enReason;   /* 最近一条未知的成因，供探针归因 */
        }
    };

    /* ---- 207 指导：页 14 区域 + stay_s 连续停留 ----
       stay_s 与 min_duration 不是两个同一件事的门：stay_s 判「这一行自己站够了没有」
       （同一 ID 连续 TRUE，换人/离开区域/中断都清零），min_duration 判「班级事实
       持续了多久」。串联的后果是首个事件最早在 stay_s + min_duration 出现，
       换来的是「有人在区域里持续站着」而不是「几个人轮流站一下」——后者正是
       路过、起立问好这类短动作能凑出来的形态 */
    CBehaviorSample_S stGuide = bHaveStand ? regionFactOf(Behavior_E::TEA_GUIDING, stStand)
                                           : stStand;
    CStayTrack_S *pTrk = nullptr;
    for (CStayTrack_S &stTrk : m_vecStayTrack)
    {
        if (stTrk.nPersonId == stRow.nPersonId)
        {
            pTrk = &stTrk;
            break;
        }
    }
    if (TriState_E::TS_TRUE == stGuide.enState)
    {
        const int64_t llStayMs = static_cast<int64_t>(m_spSnap->nGuidingStayMs);
        if (nullptr == pTrk && m_vecStayTrack.size() >= static_cast<size_t>(CLASSROOM_MAX_PERSONS))
        {
            /* 宽限窗里的旧条目占着容量：换掉最久没出现的那一个。否则新来的行拿不到计时槽，
               而拿不到槽在本函数里的语义是「不受停留门」——那是一条不经停留的 TRUE，
               比「晚一个窗口开始计时」错得多 */
            auto itOld = std::min_element(m_vecStayTrack.begin(), m_vecStayTrack.end(),
                                          [](const CStayTrack_S &stL, const CStayTrack_S &stR) {
                                              return stL.llLastMs < stR.llLastMs;
                                          });
            m_vecStayTrack.erase(itOld);
        }
        if (nullptr == pTrk)
        {
            CStayTrack_S stNew;
            stNew.nPersonId = stRow.nPersonId;
            stNew.llSinceMs = llFrameMs;
            stNew.llLastMs = llFrameMs;
            m_vecStayTrack.push_back(stNew);
            pTrk = &m_vecStayTrack.back();
        }
        if (nullptr != pTrk)
        {
            if (llFrameMs < pTrk->llSinceMs)
            {
                /* 时刻倒退的观测（本账本接受）：段起点跟着退，不让时长变成负数 */
                pTrk->llSinceMs = llFrameMs;
            }
            pTrk->llLastMs = llFrameMs;
            if (llStayMs > 0 && (llFrameMs - pTrk->llSinceMs) < llStayMs)
            {
                stGuide.enState = TriState_E::TS_UNKNOWN;
                stGuide.enReason = SampleReason_E::SR_STAY_SHORT;
            }
        }
    }
    else if (TriState_E::TS_FALSE == stGuide.enState && nullptr != pTrk)
    {
        /* 明确不站了/离开区域：连续段当场结束。erase 后 pTrk 失效，不再使用。
           UNKNOWN 不在这里清——「没量到」不是「不站了」，骨骼单拍缺失就把 5s 重新数一遍
           等于白等；这种行本拍不推进 llLastMs，真的断掉时由上面的宽限窗收尾 */
        m_vecStayTrack.erase(m_vecStayTrack.begin() + (pTrk - m_vecStayTrack.data()));
    }
    countInto(rstFacts.u32GuideNum, rstFacts.u32GuideTrue, rstFacts.u32GuideFalse,
              rstFacts.enGuideUnknown, stGuide);

    /* ---- 209 互动：页 15 区域 + 骨骼均分置信门（站立与举手各判各的） ----
       rules.interact.key_score 与 206/208 的 teaching/board_write 同一口径：
       骨骼点平均分低于门限 → UNKNOWN（没量准不是没发生），不写成 FALSE */
    auto interactFact = [this, &regionFactOf, &stRow](Behavior_E enBehavior,
                                                      const CBehaviorSample_S &stSample) {
        CBehaviorSample_S stOut = regionFactOf(enBehavior, stSample);
        const float fGate = m_spSnap->fInteractKeyScore;
        if (TriState_E::TS_TRUE == stOut.enState && fGate > 0.0f &&
            stRow.stSkeleton.bValid && stRow.stSkeleton.fScore < fGate)
        {
            stOut.enState = TriState_E::TS_UNKNOWN;
            stOut.enReason = SampleReason_E::SR_KEY_LOW;
        }
        return stOut;
    };
    if (bHaveStand)
    {
        countInto(rstFacts.u32InteractNum, rstFacts.u32InteractTrue, rstFacts.u32InteractFalse,
                  rstFacts.enInteractUnknown, interactFact(Behavior_E::TEA_INTERACT, stStand));
    }
    if (bHaveRaise)
    {
        countInto(rstFacts.u32InteractNum, rstFacts.u32InteractTrue, rstFacts.u32InteractFalse,
                  rstFacts.enInteractUnknown, interactFact(Behavior_E::TEA_INTERACT, stRaise));
    }
}

void CRuleEngine::feedClassLevel(const CClassFacts_S &stFacts, int64_t llFrameMs, uint32_t u32Seq)
{
    /* 哨兵行：没有具体的人、没有座位、没有头框。ID 0 与跟踪 ID（1 起）不会撞；
       下游只丢 nPersonId<0，所以它能一路走到上报（见 CLASSROOM_CLASS_PERSON_ID） */
    CPersonFeature_S stRow;
    stRow.nPersonId = CLASSROOM_CLASS_PERSON_ID;
    stRow.nSeatId = -1;

    const CSubjectView_S stView = m_stSubject.view();

    /* ---- 210 巡视（教师机）：讲台画了、本拍讲台里一个头框都没有 ⇒ 成立 ----
       （取代原先挂在教师行上的「学生席 + 走动 + 站立」复合判据）：
       讲台区域无人即可。走路、站姿、在哪一侧都不再当前提。
       讲台没画 / 归属总开关关着 ⇒ UNKNOWN（不是 FALSE）：这两种情况下「讲台空」这件事
       根本没人算，写成 FALSE 等于用一次没配置换一段假数据；归属关闭时仲裁器保留上一拍的
       bPodiumEmpty（见 subject_arbiter.cpp），拿它当本拍事实就是陈旧读数 */
    CBehaviorSample_S stPatrol;
    if (!m_spSnap->stTeacher.bEnable)
    {
        stPatrol.enState = TriState_E::TS_UNKNOWN;
        stPatrol.enReason = SampleReason_E::SR_SUBJECT;
    }
    else if (!stView.bPodiumConfigured)
    {
        stPatrol.enState = TriState_E::TS_UNKNOWN;
        stPatrol.enReason = SampleReason_E::SR_REGION_MISS;
    }
    else if (!((m_llLastSubjectMs >= 0) &&
               (llFrameMs - m_llLastSubjectMs) < static_cast<int64_t>(CLASSROOM_SUBJECT_MEMORY_MS)))
    {
        /* #174 空场：黑板/镜头被遮/断流时画面里一个人都没有，「讲台无人」这句话没有主语——
           没人可判，不等于那里真的没人。30s 内见过人仍按原口径判（只有教师的讲台机
           照样出 210）；连续 30s 零主体才认为镜头里没有可判场景 */
        stPatrol.enState = TriState_E::TS_UNKNOWN;
        stPatrol.enReason = SampleReason_E::SR_NO_FRAME;
    }
    else
    {
        stPatrol.enState = stView.bPodiumEmpty ? TriState_E::TS_TRUE : TriState_E::TS_FALSE;
        stPatrol.enReason = SampleReason_E::SR_OK;
    }

    /* 平台查询件的唯一写入点：平台收到学生机 207/209 的事件后会来问「讲台有没有人」，
       答案就是这一格。墙钟取本拍 run() 记下的那个（帧时与墙钟不同基，不能混减）；
       bValid 从第一拍起就是 true——UNKNOWN 也是一种答案（"没人算"），只是默认值不是 */
    m_stPodium.bValid = true;
    m_stPodium.enState = stPatrol.enState;
    m_stPodium.enReason = stPatrol.enReason;
    m_stPodium.llFrameMs = llFrameMs;
    m_stPodium.llWallMs = m_llLastRunWallMs;

    /* ---- 207 指导（学生机）：页 14 框内、连续站够 stay_s 的人 ----
       「一定时间」由 behaviors.207 的 min_duration_ms 承担，这里只出一拍三态。
       stay_s（rules.guiding.stay_ms）为 0 = 不设停留门（临时快照的默认），
       出厂 5s；调大即要求「同一行在区域内连续站着」——路过、起立问好凑不出这一条 */
    const TriState_E enStand = triAnyOf(stFacts.u32GuideNum, stFacts.u32GuideTrue,
                                        stFacts.u32GuideFalse);
    CBehaviorSample_S stGuide;
    stGuide.enState = enStand;
    stGuide.enReason = SampleReason_E::SR_OK;
    if (TriState_E::TS_UNKNOWN == enStand)
    {
        stGuide.enReason = (0 == stFacts.u32GuideNum) ? SampleReason_E::SR_NO_FRAME
                                                      : stFacts.enGuideUnknown;
    }

    /* ---- 209 师生互动（学生机）：页 15 框内有人站立或举手 ----
       站立/举手两列并成一次存在性判断：任一 TRUE 即成立；要判 FALSE 必须两列的
       每一条样本都明确否。置信门已在逐行折算时按 rules.interact.key_score 应用 */
    const uint32_t u32AllNum = stFacts.u32InteractNum;
    const uint32_t u32AllTrue = stFacts.u32InteractTrue;
    const uint32_t u32AllFalse = stFacts.u32InteractFalse;
    const TriState_E enInteract = triAnyOf(u32AllNum, u32AllTrue, u32AllFalse);
    CBehaviorSample_S stInteract;
    stInteract.enState = enInteract;
    stInteract.enReason = SampleReason_E::SR_OK;
    if (TriState_E::TS_UNKNOWN == enInteract)
    {
        if (0 == u32AllNum)
        {
            stInteract.enReason = SampleReason_E::SR_NO_FRAME;
        }
        else
        {
            /* 混杂时给最近一条未知的成因（区域外/骨骼分低/无姿态）——聚合时已记下，
               这里不再按列拆：站立与举手在 209 是同一个存在性命中，拆开只会多一个骗人的数 */
            stInteract.enReason = stFacts.enInteractUnknown;
        }
    }

    /* 关着的班级列不喂门：feedOne 的"关列"分支只记一次 u64ColumnOff 就返回，不做收尾
       （从开到关的收尾在 applyConfig→disableColumn，那条路径对哨兵有虚拟行）。
       这三列是每拍固定喂的、不随行数变化，照喂会让 column_off 变成"每拍 +3"的常量噪声，
       读它的人会以为采样器列也在逐行空转——而它的语义是"每一行每一列被略过一次" */
    auto feedClassCol = [&](Behavior_E enBehavior, const CBehaviorSample_S &stSample)
    {
        if (m_spSnap->gateOf(enBehavior).bEnable)
        {
            feedOne(stRow, enBehavior, stSample, llFrameMs, u32Seq, true);
        }
    };
    feedClassCol(Behavior_E::TEA_PATROL, stPatrol);
    feedClassCol(Behavior_E::TEA_GUIDING, stGuide);
    feedClassCol(Behavior_E::TEA_INTERACT, stInteract);

    /* 探针（只记日志，不改判定）：三列的行是哨兵 0，逐行探针覆盖不到。
       (判,成因) 变化沿 + 10s 心跳——「为什么 210 不出」要先能读出是讲台没画、
       归属没开还是讲台里真的有人 */
    const Behavior_E aenProbe[3] = { Behavior_E::TEA_GUIDING, Behavior_E::TEA_INTERACT, Behavior_E::TEA_PATROL };
    const CBehaviorSample_S astProbe[3] = { stGuide, stInteract, stPatrol };
    for (int i = 0; i < 3; ++i)
    {
        const bool bEdge = (astProbe[i].enState != m_aenClsProbeState[i]) ||
                           (astProbe[i].enReason != m_aenClsProbeReason[i]);
        if (!bEdge && (llFrameMs - m_llClsProbeMs) < 10000)
        {
            continue;
        }
        m_llClsProbeMs = llFrameMs;
        m_aenClsProbeState[i] = astProbe[i].enState;
        m_aenClsProbeReason[i] = astProbe[i].enReason;
        dlog_info("classroom rule: 班级列探针 列=%d 判=%c 成因=%d 讲台配=%d 讲台空=%d"
                  " 导=%u/%u/%u 互=%u/%u/%u",
                  static_cast<int>(aenProbe[i]),
                  (TriState_E::TS_TRUE == astProbe[i].enState) ? 'T' :
                  (TriState_E::TS_FALSE == astProbe[i].enState) ? 'F' : 'U',
                  static_cast<int>(astProbe[i].enReason),
                  stView.bPodiumConfigured ? 1 : 0,
                  stView.bPodiumEmpty ? 1 : 0,
                  stFacts.u32GuideTrue, stFacts.u32GuideFalse, stFacts.u32GuideNum,
                  stFacts.u32InteractTrue, stFacts.u32InteractFalse, stFacts.u32InteractNum);
    }
}

/* ============================ 事件环 ============================ */

void CRuleEngine::pushEvent(const CBehaviorEvent_S &stEvent)
{
    if (m_u32EventNum >= CLASSROOM_RULE_EVENT_RING)
    {
        /* 丢最旧而不是丢最新：下游取不走时，保留的时间轴尾部更接近「现在」。
           代价是可能丢掉一条 BEGIN 而留下它的 END —— P3 侧必须容忍没有配对的结束事件
           （它的聚合只按 BEGIN 计次、按 BEGIN/END 配对算时长，多出的 END 只会被丢弃并计数） */
        m_u32EventHead = (m_u32EventHead + 1) % CLASSROOM_RULE_EVENT_RING;
        --m_u32EventNum;
        ++m_stStat.u64EventDrop;
        /* 用这条**事件**的时刻而不是墙钟：环形队列满这件事发生在帧时间轴上，
           而帧时间轴才是"这节课里什么时候出的问题"该读的那把尺 */
        if (dropLogDue(stEvent.llMs, m_llDropLogMs))
        {
            dlog_warn("classroom rule: 事件环形队列已满，丢弃最旧 (dropped=%llu)",
                      static_cast<unsigned long long>(m_stStat.u64EventDrop));
        }
    }

    const uint32_t u32Tail = (m_u32EventHead + m_u32EventNum) % CLASSROOM_RULE_EVENT_RING;
    m_astEventRing[u32Tail] = stEvent;
    ++m_u32EventNum;
}

bool CRuleEngine::pollEvent(CBehaviorEvent_S &stOut)
{
    if (0 == m_u32EventNum)
    {
        return false;
    }
    stOut = m_astEventRing[m_u32EventHead];
    m_u32EventHead = (m_u32EventHead + 1) % CLASSROOM_RULE_EVENT_RING;
    --m_u32EventNum;
    return true;
}

/* ============================ 收尾与退役 ============================ */

void CRuleEngine::disableColumn(int nCol, int64_t llNowMs)
{
    Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
    if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
    {
        return;
    }

    /* 用「喂一次 bEnable=false」收尾而不是清表：门自己实现了关闭语义（生效中则补一次退出，
       没生效的把攒到一半的计时作废），参数里其余字段都取当前值，只有 bEnable 强制 false。
       这一拍不该产生新事件，所以最短时长/容忍额度实际用不上，唯一起作用的是那两行作废。
       条件只看"这个人这一列有没有格子"，不看"生效没有"：挡掉未生效的槽位，
       就等于让攒到一半的锚点跨过一次关→开活下来（平台上最常用的正是那个动作） */
    CGateParam_S stOff = m_spSnap->gateOf(enBehavior);
    stOff.bEnable = false;
    stOff.nSampleIntervalMs = 0;   /* 收尾不受采样间隔节流，否则可能整拍被拒、事件关不掉 */

    int32_t nClosed = 0;
    /* P13：宽限名单里的人也要收——关列时他们仍有生效中的事件，却已不在本拍在场表里。
       漏掉他们的现象是「这一列已经关了，平台上还在跑」。
       ★ 班级级列（207/209/210）的行是哨兵 0，两张在场表里都没有它：
       在末尾补一格虚拟的「在场记录」，让收尾只走一份事件构造 */
    const int32_t nClassRow = isClassLevel(enBehavior) ? 1 : 0;
    const int32_t nSeenNum = m_nSeenNum + m_nGoneNum;
    for (int32_t i = 0; i < nSeenNum + nClassRow; ++i)
    {
        CSeen_S stClass;
        const CSeen_S *pstSeen = nullptr;
        if (i < m_nSeenNum)
        {
            pstSeen = &m_astSeen[i];
        }
        else if (i < nSeenNum)
        {
            pstSeen = &m_astGone[i - m_nSeenNum];
        }
        else
        {
            stClass.nPersonId = CLASSROOM_CLASS_PERSON_ID;
            stClass.nSeatId = -1;
            pstSeen = &stClass;
        }
        const CSeen_S &stSeen = *pstSeen;
        CGateState_S stBefore;
        if (!m_stGate.getState(stSeen.nPersonId, enBehavior, stBefore))
        {
            continue;   /* 没有格子：不喂，喂会让 mutableState 为他从没占过的列分配一行 */
        }

        const CGateResult_S stResult = m_stGate.feed(stSeen.nPersonId, enBehavior, TriState_E::TS_UNKNOWN,
                                                    llNowMs, stOff);
        if (!stResult.bDeactivated)
        {
            continue;
        }

        CBehaviorEvent_S stEvent;
        stEvent.nPersonId = stSeen.nPersonId;
        stEvent.nSeatId = stSeen.nSeatId;
        stEvent.enBehavior = enBehavior;
        stEvent.bBegin = false;
        stEvent.llMs = llNowMs;
        stEvent.llHeldMs = (GATE_TIME_NONE != stBefore.llActiveSinceMs)
                               ? (llNowMs - stBefore.llActiveSinceMs) : 0;
        stEvent.stHeadBox = stSeen.stHeadBox;
        stEvent.nExprClass = stBefore.nAux;   /* 关列收尾同样带类别（#88） */
        pushEvent(stEvent);

        if (m_anActiveNum[nCol] > 0)
        {
            --m_anActiveNum[nCol];
        }
        ++m_stStat.u64EventEnd;
        ++nClosed;
    }

    if (nClosed > 0)
    {
        dlog_info("classroom rule: 行为 %s 已关闭，收尾 %d 个事件",
                  behaviorCnName(enBehavior), nClosed);
    }
}

int32_t CRuleEngine::closeActiveAll(int64_t llNowMs)
{
    int32_t nClosed = 0;

    /* P13：宽限名单里的人也必须收。旧口径的隐含前提是「门里有状态的人一定在本拍在场表里」，
       退役宽限把这条打破了——人在宽限期里仍有生效中的事件，却已不在 m_astSeen。
       漏掉他们的后果不是少收一条 END：下面 m_stGate.reset() 会把整表清空，
       他们的 kCounter 还在（m_anActiveNum 没减），于是 activeNum 永久虚高、
       isActive 全部为假 —— fuzz 套称这种现象为「引擎与账本对不上」。 */
    for (int32_t i = 0; i < m_nSeenNum + m_nGoneNum; ++i)
    {
        const CSeen_S &stSeen = (i < m_nSeenNum) ? m_astSeen[i] : m_astGone[i - m_nSeenNum];

        for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
        {
            Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
            if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
            {
                continue;
            }

            CGateState_S stBefore;
            if (!m_stGate.getState(stSeen.nPersonId, enBehavior, stBefore) || !stBefore.bActive)
            {
                continue;
            }

            CBehaviorEvent_S stEvent;
            stEvent.nPersonId = stSeen.nPersonId;
            stEvent.nSeatId = stSeen.nSeatId;
            stEvent.enBehavior = enBehavior;
            stEvent.bBegin = false;
            stEvent.llMs = llNowMs;
            stEvent.llHeldMs = (GATE_TIME_NONE != stBefore.llActiveSinceMs)
                                   ? (llNowMs - stBefore.llActiveSinceMs) : 0;
            stEvent.stHeadBox = stSeen.stHeadBox;
            stEvent.nExprClass = stBefore.nAux;   /* 停摆/关总开关收尾同样带类别（#88） */
            pushEvent(stEvent);

            if (m_anActiveNum[nCol] > 0)
            {
                --m_anActiveNum[nCol];
            }
            ++m_stStat.u64EventEnd;
            ++nClosed;
        }
    }

    /* 班级级列的行是哨兵 0，不在上面两张表里。漏掉它，下面的 m_stGate.reset()
       会把 bActive 清掉却不发 END —— 平台侧留下一条拉到底的指导/互动/巡视。
       复用 closeAndRetirePerson：它只收「生效中」的格子，收完连同未生效的锚点一起回收 */
    CSeen_S stClass;
    stClass.nPersonId = CLASSROOM_CLASS_PERSON_ID;
    stClass.nSeatId = -1;
    nClosed += closeAndRetirePerson(stClass, llNowMs, 0);

    /* 清表而不是逐槽改写，且**不看 nClosed**：收尾之后这一批人是否还在场已经不确定，
       留着旧状态让下一拍从零开始判，比继承一场「已经结束的持续态」安全。
       原先挂着 `if (nClosed > 0)`，而"没关掉任何东西"恰恰是最需要清表的那种局——
       一条都没生效不等于门表是空的：还在攒最短时长、没攒满的那些格子（`llTrueSinceMs` 锚点）
       一条也关不掉，于是全表原封不动带进下一轮。表现是三处都能撞到的同一个错：
       关总开关再开（`reset`）、进应急（`shutdownActive`）、感知停摆（run 的断档分支）之后，
       旧锚点还在走表——重启第一拍 held 就 >= min，最短持续门当场被判满足，
       而事件的起点被回推到开关落下之前（`llActiveSinceMs = llTrueSinceMs`）。
       代价说清：`m_stGate.reset()` 连带把行池溢出计数 `gate_deny` 归零，
       它的口径是"这一节课/这一次开关以来"，现在多了三个归零点（收尾、应急、停摆）。
       换一个方向读：那本来就是"这一轮重开"的意思，读的人要的是分轮，不是跨轮累计 */
    m_stGate.reset();

    /* 清表 = 这一轮没有生效中的行为。计数一并归零（上面那圈已逐个减过；这里是第二道，
       防的是「某条路径漏减」变成永久虚高）。宽限名单同样清：表都空了，
       名单里的人再留着只会在下一次 retireGone 里对着空表补 END、白记一次 retired */
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_anActiveNum[nCol] = 0;
    }
    m_nGoneNum = 0;
    return nClosed;
}

void CRuleEngine::shutdownActive(int64_t llNowMs)
{
    const int32_t nClosed = closeActiveAll(llNowMs);
    m_nSeenNum = 0;
    m_nSeenNextNum = 0;
    m_nGoneNum = 0;   /* 宽限名单随停机一起清：停机已经给所有人补过 END，留着会二次收尾 */

    /* 归属随之清零：closeActiveAll 已经把每条行为都收尾了，所以这里不需要翻转来补发 END，
       但**必须**清掉身份——留着的话总开关重新打开的第一拍，那个人还是老师，
       而仲裁器的确认时长计数也停在上一轮，等于"没攒时间就直接上岗" */
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_anActivePrev[nCol] = 0;
    }
    m_stSubject.reset();

    dlog_info("classroom rule: 停机，收尾 %d 个进行中事件（时刻 %lldms）",
              nClosed, static_cast<long long>(llNowMs));
}

void CRuleEngine::reset(int64_t llNowMs)
{
    /* 先收尾：门表槽位是按 id 取模复用的，留着「生效中」的僵尸状态，
       下一轮占到此 id 的人一上来就带着上一轮的持续时间（最短持续门当场被判满足） */
    shutdownActive(llNowMs);

    /* 收尾产出的那批 END 在这里抹掉：这一次它们没有读者。
       不抹的话，新一轮第一次取数会先吐出一批上一轮的事件，时间戳还是上一轮的时钟 */
    m_u32EventHead = 0;
    m_u32EventNum = 0;

    m_u32LastSeq = 0;
    m_bFedOnce = false;
    m_llLastRunMs = 0;
    m_llLastRunWallMs = 0;
    m_llLastSubjectMs = -1;   /* #174：新一轮从「还没见过人」起算，不借上一轮的主体记忆 */

    /* 区域重建判据必须一起清：重启后快照版本号重新从 1 开始，与上一轮恰好相等时
       「版本没变」这句话会跳过重建，于是新画布尺寸下判定继续用旧区域表 */
    m_llRegionVersion = -1;
    m_bRegionGeomReady = false;

    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_anActiveNum[nCol] = 0;   /* 正常路径上收尾已逐条减到 0，这里是不信任前面的加减 */
    }

    m_stSubject.resetStat();   /* 归属计数同理：新一轮的 promote/demote 不能含上一轮的总量 */
    resetStat();
}

void CRuleEngine::retireGone(int64_t llFrameMs, uint32_t u32Seq)
{
    /* P13：一帧丢检不是离场。先处理宽限名单——回来的销案，过期的才补 END。
       顺序不能反：先把「本拍新消失」加进名单、再判过期，会把刚记的 0ms 当场当成
       已过期（或者要额外减一次），两条路径的口径就分叉了 */
    for (int32_t i = 0; i < m_nGoneNum; )
    {
        CSeen_S &stGone = m_astGone[i];

        bool bBack = false;
        for (int32_t j = 0; j < m_nSeenNextNum; ++j)
        {
            if (m_astSeenNext[j].nPersonId == stGone.nPersonId)
            {
                bBack = true;
                break;
            }
        }
        if (bBack)
        {
            /* 宽限期内回来：门状态这一段时间一直冻着（没有喂过 UNKNOWN），
               原事件继续，不重开。挪尾再退一格，避免把后面的条目跳过 */
            stGone = m_astGone[m_nGoneNum - 1];
            --m_nGoneNum;
            continue;
        }

        if ((llFrameMs - stGone.llMissingSinceMs) >= CLASSROOM_RULE_RETIRE_GRACE_MS)
        {
            closeAndRetirePerson(stGone, llFrameMs, u32Seq);
            ++m_stStat.u64Retired;
            stGone = m_astGone[m_nGoneNum - 1];
            --m_nGoneNum;
            continue;
        }
        ++i;
    }

    /* 本拍新消失的人：只记起点，不收尾。已经在名单里的不重复计时——
       名单就是「消失时长」的唯一账本，m_astSeen 下一拍就换掉了 */
    for (int32_t i = 0; i < m_nSeenNum; ++i)
    {
        const CSeen_S &stOld = m_astSeen[i];

        bool bStill = false;
        for (int32_t j = 0; j < m_nSeenNextNum; ++j)
        {
            if (m_astSeenNext[j].nPersonId == stOld.nPersonId)
            {
                bStill = true;
                break;
            }
        }
        if (bStill)
        {
            continue;
        }

        bool bKnown = false;
        for (int32_t j = 0; j < m_nGoneNum; ++j)
        {
            if (m_astGone[j].nPersonId == stOld.nPersonId)
            {
                bKnown = true;
                break;
            }
        }
        if (bKnown)
        {
            continue;
        }

        if (m_nGoneNum < CLASSROOM_MAX_PERSONS)
        {
            CSeen_S &stGone = m_astGone[m_nGoneNum];
            stGone = stOld;
            stGone.llMissingSinceMs = llFrameMs;
            ++m_nGoneNum;
        }
        else
        {
            /* 名单满：退回老口径当场收尾。宁可切一段，也不让门状态在无人认领的名单里泄漏 */
            closeAndRetirePerson(stOld, llFrameMs, u32Seq);
            ++m_stStat.u64Retired;
        }
    }
}

int32_t CRuleEngine::closeAndRetirePerson(const CSeen_S &stAt, int64_t llNowMs, uint32_t u32Seq,
                                          bool bSubjectCut)
{
    /* 人不见了、或他刚换了身份：先给名下所有生效中的行为补 END，再回收槽位。
       反过来的话 retirePerson 会把 bActive 一起清掉，END 事件就再也发不出来了 ——
       现象正是「人一走，他的低头事件在平台上永远不结束」 */
    int32_t nClosed = 0;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }

        CGateState_S stBefore;
        if (!m_stGate.getState(stAt.nPersonId, enBehavior, stBefore) || !stBefore.bActive)
        {
            continue;
        }

        CBehaviorEvent_S stEvent;
        stEvent.nPersonId = stAt.nPersonId;
        stEvent.nSeatId = stAt.nSeatId;
        stEvent.enBehavior = enBehavior;
        stEvent.bBegin = false;
        stEvent.llMs = llNowMs;
        stEvent.llHeldMs = (GATE_TIME_NONE != stBefore.llActiveSinceMs)
                               ? (llNowMs - stBefore.llActiveSinceMs) : 0;
        stEvent.stHeadBox = stAt.stHeadBox;
        stEvent.u32FeatureSeq = u32Seq;
        stEvent.nExprClass = stBefore.nAux;   /* 人离场补 END 同样带类别（#88） */
        stEvent.bSubjectCut = bSubjectCut;    /* 翻转收尾：合并器不得与随后的 BEGIN 并段（P14） */
        pushEvent(stEvent);

        if (m_anActiveNum[nCol] > 0)
        {
            --m_anActiveNum[nCol];
        }
        ++m_stStat.u64EventEnd;
        ++nClosed;
    }

    /* 连未生效的挂起态一起清（retirePerson 是整格回收）：翻转之后这个人要换一本账，
       留着上一族的"已攒 1.2 秒"会让新身份第一拍就满足最短时长，并把事件起点回推到换身份之前 */
    m_stGate.retirePerson(stAt.nPersonId);
    return nClosed;
}

void CRuleEngine::locatePerson(const CFeatureFrame_S *pstFrame, int32_t nPersonId, CSeen_S &rstOut) const
{
    rstOut = CSeen_S();
    rstOut.nPersonId = nPersonId;

    if (nullptr != pstFrame)
    {
        const CPersonFeature_S *pRow = pstFrame->findPerson(nPersonId);
        if (nullptr != pRow)
        {
            rstOut.nSeatId = pRow->nSeatId;
            rstOut.stHeadBox = pRow->stHeadBox;
            return;
        }
    }

    for (int32_t i = 0; i < m_nSeenNum; ++i)
    {
        if (m_astSeen[i].nPersonId == nPersonId)
        {
            rstOut = m_astSeen[i];
            return;
        }
    }
    /* 两处都没有：事件照发，只是没有位置。丢掉整条 END 的后果（时间轴拉到底）
       比一条没有矩形的位置事件更糟 */
}

void CRuleEngine::applySubjectFlips(const CFeatureFrame_S *pstFrame, int64_t llNowMs, uint32_t u32Seq)
{
    m_nFlipIdNum = 0;
    CSubjectFlip_S stFlip;
    while (m_stSubject.pollFlip(stFlip))
    {
        if (m_nFlipIdNum < CLASSROOM_MAX_PERSONS)
        {
            m_anFlipIds[m_nFlipIdNum++] = stFlip.nPersonId;
        }
        CSeen_S stAt;
        locatePerson(pstFrame, stFlip.nPersonId, stAt);

        const int32_t nClosed = closeAndRetirePerson(stAt, llNowMs, u32Seq, true);
        if (nClosed > 0)
        {
            /* 翻转本身由仲裁器报（它那里有主体名），这条只补它答不出的那半边：
               这次换身份切掉了几条正在记的行为。没有切掉东西的翻转不值得刷屏 */
            m_stStat.u64SubjectClose += static_cast<uint64_t>(nClosed);
            dlog_warn("classroom rule: 人员 %d 主体切换提前截断 %d 个事件（时刻 %lldms）",
                      stFlip.nPersonId, nClosed, static_cast<long long>(llNowMs));
        }
    }
}

void CRuleEngine::markSeen(int32_t nPersonId, int32_t nSeatId, const CRectf_S &stHeadBox)
{
    if (m_nSeenNextNum >= CLASSROOM_MAX_PERSONS)
    {
        return;   /* 上游已按容量截断，这里只是防止越界写成为可能的 second-order 后果 */
    }
    /* 同一个 ID 在本拍只记一行（后面一行覆盖前一行的头框/座位，
       不新开一条）。理由与仲裁器的 anCountedId 同源：在场表是"这一拍有哪些人"的台账，
       重复行会让 closeActiveAll / disableColumn 对同一个人补两条 END、m_anActiveNum
       二次自减，并且多占一格把另一个真人挤出在场表——他的 END 只能等宽限路径补，
       而宽限名单的容量也是 CLASSROOM_MAX_PERSONS，挤出去的那一格是真实的信息损失。
       今天不可达（跟踪器 ID 唯一、感知层每轨一行），但与仲裁器那条是同一形状的两个
       孪生修补：只补一侧，将来任一侧上游改成多源合并就会静默错账。 */
    for (int32_t i = 0; i < m_nSeenNextNum; ++i)
    {
        if (m_astSeenNext[i].nPersonId == nPersonId)
        {
            m_astSeenNext[i].nSeatId = nSeatId;
            m_astSeenNext[i].stHeadBox = stHeadBox;
            return;   /* 覆盖而不是追加：后读到的那一行为准 */
        }
    }

    CSeen_S &stSeen = m_astSeenNext[m_nSeenNextNum];
    stSeen.nPersonId = nPersonId;
    stSeen.nSeatId = nSeatId;
    stSeen.stHeadBox = stHeadBox;
    ++m_nSeenNextNum;
}

/* ============================ 区域表 ============================ */

void CRuleEngine::ensureRegions(const CFrameGeometry &stGeom)
{
    if (!m_spSnap)
    {
        return;
    }

    const bool bGeomReady = stGeom.isReady();
    const int nMirror = static_cast<int>(stGeom.mirrorOf());
    const int nRotate = stGeom.rotationOf();

    const bool bGeomChanged = (m_bRegionGeomReady != bGeomReady) ||
                              (m_nRegionSrcW != stGeom.srcWidth()) ||
                              (m_nRegionSrcH != stGeom.srcHeight()) ||
                              (m_nRegionDstW != stGeom.dstWidth()) ||
                              (m_nRegionDstH != stGeom.dstHeight()) ||
                              (m_nRegionRotate != nRotate) ||
                              (m_nRegionMirror != nMirror);

    /* 版本单独判：换配置时哪怕几何没变也必须重算，否则「把讲台往左挪 20 像素」会不生效 */
    if (!bGeomChanged && m_llRegionVersion == m_spSnap->llVersion)
    {
        return;
    }

    const int32_t nBuilt = m_stRegion.build(m_spSnap->vecRegions, m_spSnap->vecRegionRoles,
                                            m_spSnap->enRegionSpace,
                                            m_spSnap->nRegionRefWidth, m_spSnap->nRegionRefHeight,
                                            stGeom);
    m_bRegionGeomReady = bGeomReady;
    m_nRegionSrcW = stGeom.srcWidth();
    m_nRegionSrcH = stGeom.srcHeight();
    m_nRegionDstW = stGeom.dstWidth();
    m_nRegionDstH = stGeom.dstHeight();
    m_nRegionRotate = nRotate;
    m_nRegionMirror = nMirror;
    m_llRegionVersion = m_spSnap->llVersion;

    dlog_info("classroom rule: 区域已重建, built=%d desc=%zu geom_ready=%d src=%dx%d dst=%dx%d"
              " rotate=%d mirror=%d config_ver=%lld",
              nBuilt, m_spSnap->vecRegions.size(), bGeomReady ? 1 : 0,
              m_nRegionSrcW, m_nRegionSrcH, m_nRegionDstW, m_nRegionDstH, nRotate, nMirror,
              static_cast<long long>(m_llRegionVersion));

    if (!m_spSnap->vecRegions.empty() && 0 == nBuilt)
    {
        /* 配了区域却一条都没建成：此时所有带区域引用的行为都在被判「查不到区域」，
           现象是这些行为整体不出事件。把因果直接写进日志，别让人去反推 SR_REGION_MISS */
        dlog_error("classroom rule: 配置了 %zu 个区域但一条都没建成，"
                   "带区域门控的行为将一直停在 unknown",
                   m_spSnap->vecRegions.size());
    }
}

/* ============================ 观测 ============================ */

bool CRuleEngine::isActive(int32_t nPersonId, Behavior_E enBehavior) const
{
    return m_stGate.isActive(nPersonId, enBehavior);
}

bool CRuleEngine::isKnown(int32_t nPersonId, Behavior_E enBehavior) const
{
    return m_stGate.isKnown(nPersonId, enBehavior);
}

int32_t CRuleEngine::activeNum(Behavior_E enBehavior) const
{
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0)
    {
        return 0;
    }
    return m_anActiveNum[nCol];
}

void CRuleEngine::publishStat()
{
    m_stStat.nRegionNum = m_stRegion.regionNum();
    /* 门自己的账只有节拍线程读得到（它无锁），而 publishStat 正跑在同一线程 —— 取到这里
       再随快照发布，statusJson 才有它。放到别处读就是让第二个线程去翻一张正在改的表 */
    m_stStat.u64GateDeny = m_stGate.rowDenyNum();
    /* 配置条数只有和「建成条数」并排看才有意义：region=0/3 是换算没成（必须查），
       region=0/0 是根本没画区域（正常）。publishStat 跑在节拍线程，读快照是安全的 */
    m_stStat.nRegionConfigured = m_spSnap ? static_cast<int32_t>(m_spSnap->vecRegions.size()) : 0;
    /* 「开着但没人喂」的行数：只报数量，具体是哪几行在 applyConfig 换版那刻已经逐条喊过。
       放这儿是因为它是 m_u32Unjudged 唯一的消费者，而那个成员只在节拍线程被写 */
    m_stStat.nUnjudged = static_cast<int32_t>(__builtin_popcount(m_u32Unjudged));

    std::lock_guard<std::mutex> stLock(m_mtxPub);
    m_stStatPub = m_stStat;
    /* 讲台状态与计数快照同一次持锁换出：平台拿到的 age_ms 与 behaviors 账对得上同一拍 */
    m_stPodiumPub = m_stPodium;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_anActiveNumPub[nCol] = m_anActiveNum[nCol];
    }
}

CClassPodiumState_S CRuleEngine::podiumState() const
{
    std::lock_guard<std::mutex> stLock(m_mtxPub);
    return m_stPodiumPub;
}


CRuleEngineStat_S CRuleEngine::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtxPub);
    return m_stStatPub;
}

void CRuleEngine::resetStat()
{
    /* 两份都清：只清工作份的话，发布件里的旧读数会在下一次 publish 之前被 statusJson 读到，
       「已经归零」这句话就假了半个拍周期——而调这个接口的人正是拿它在测什么 */
    m_stStat = CRuleEngineStat_S();
    std::lock_guard<std::mutex> stLock(m_mtxPub);
    m_stStatPub = CRuleEngineStat_S();
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_anActiveNumPub[nCol] = 0;
    }
}

const char *CRuleEngine::reasonName(SampleReason_E enReason)
{
    return sampleReasonName(enReason);
}

std::string CRuleEngine::statLine() const
{
    const CRuleEngineStat_S stSnap = stat();   /* 一次取表：逐个字段取会反复加锁 */

    /* 缓冲按字段数与最大串长估：10 个成因名 ≈ 100B，其余 15 个键名与分隔 ≈ 200B，
       数值全按 20 位整数留 320B，取 1024 是余量——截断的读数比缺失的读数更危险 */
    char szBuf[1024] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "runs=%llu rows=%d evt=%llu/%llu drop=%llu retired=%llu deny=%llu stall=%llu frz=%llu"
                              /* act_max 是单列峰值（某一行为同时生效人数）而非全班总量 */
                              " act_max=%d region=%d/%d unjudged=%d cost=%lldus"
                              " |samp=%llu thr=%llu coff=%llu T=%llu F=%llu U=%llu rej=%llu miss=%llu"
                              " |why",
                              static_cast<unsigned long long>(stSnap.u64Runs),
                              stSnap.nTrackMax,
                              static_cast<unsigned long long>(stSnap.u64EventBegin),
                              static_cast<unsigned long long>(stSnap.u64EventEnd),
                              static_cast<unsigned long long>(stSnap.u64EventDrop),
                              static_cast<unsigned long long>(stSnap.u64Retired),
                              static_cast<unsigned long long>(stSnap.u64GateDeny),
                              static_cast<unsigned long long>(stSnap.u64StallClose),
                              static_cast<unsigned long long>(stSnap.u64PoseFrozen),
                              stSnap.nActiveMax,
                              stSnap.nRegionNum, stSnap.nRegionConfigured,
                              stSnap.nUnjudged,
                              static_cast<long long>(stSnap.llLastCostUs),
                              static_cast<unsigned long long>(stSnap.u64Samples),
                              static_cast<unsigned long long>(stSnap.u64Throttled),
                              static_cast<unsigned long long>(stSnap.u64ColumnOff),
                              static_cast<unsigned long long>(stSnap.u64True),
                              static_cast<unsigned long long>(stSnap.u64False),
                              static_cast<unsigned long long>(stSnap.u64Unknown),
                              static_cast<unsigned long long>(stSnap.u64RegionReject),
                              static_cast<unsigned long long>(stSnap.u64RegionMiss));

    if (nLen < 0 || static_cast<size_t>(nLen) >= sizeof(szBuf))
    {
        return std::string("rules=fmt_error");
    }

    /* 成因逐项跟在后面：现场真正要看的是「UNKNOWN 是被什么挡住的」，
       所以按 SR_NUM 全打、0 也打——缺一项与该项为 0 是两种不同的信息（枚举加一项这里自动多一项） */
    std::string strOut(szBuf);
    for (int i = 0; i < static_cast<int>(SampleReason_E::SR_NUM); ++i)
    {
        char szItem[64] = {0};
        snprintf(szItem, sizeof(szItem), " %s=%llu", reasonName(static_cast<SampleReason_E>(i)),
                 static_cast<unsigned long long>(stSnap.au64Reason[i]));
        strOut += szItem;
    }
    return strOut;
}

std::string CRuleEngine::statJson() const
{
    /* 两份发布件必须在同一次持锁里取走：分开取会拿到「计数是第 N 拍、生效人数是第 N+1 拍」
       的组合，看板上就是一条永远对不上的账 */
    CRuleEngineStat_S stSnap;
    int32_t anActive[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    {
        std::lock_guard<std::mutex> stLock(m_mtxPub);
        stSnap = m_stStatPub;
        for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
        {
            anActive[nCol] = m_anActiveNumPub[nCol];
        }
    }

    std::string str = "{";
    str += "\"runs\":" + std::to_string(static_cast<long long>(stSnap.u64Runs));
    str += ",\"samples\":" + std::to_string(static_cast<long long>(stSnap.u64Samples));
    str += ",\"throttled\":" + std::to_string(static_cast<long long>(stSnap.u64Throttled));
    str += ",\"column_off\":" + std::to_string(static_cast<long long>(stSnap.u64ColumnOff));
    str += ",\"t\":" + std::to_string(static_cast<long long>(stSnap.u64True));
    str += ",\"f\":" + std::to_string(static_cast<long long>(stSnap.u64False));
    str += ",\"u\":" + std::to_string(static_cast<long long>(stSnap.u64Unknown));
    str += ",\"region_reject\":" + std::to_string(static_cast<long long>(stSnap.u64RegionReject));
    str += ",\"region_miss\":" + std::to_string(static_cast<long long>(stSnap.u64RegionMiss));
    str += ",\"evt_begin\":" + std::to_string(static_cast<long long>(stSnap.u64EventBegin));
    str += ",\"evt_end\":" + std::to_string(static_cast<long long>(stSnap.u64EventEnd));
    str += ",\"evt_drop\":" + std::to_string(static_cast<long long>(stSnap.u64EventDrop));
    str += ",\"retired\":" + std::to_string(static_cast<long long>(stSnap.u64Retired));
    str += ",\"gate_deny\":" + std::to_string(static_cast<long long>(stSnap.u64GateDeny));
    str += ",\"stall_close\":" + std::to_string(static_cast<long long>(stSnap.u64StallClose));
    str += ",\"pose_frozen\":" + std::to_string(static_cast<long long>(stSnap.u64PoseFrozen));
    str += ",\"subject_close\":" + std::to_string(static_cast<long long>(stSnap.u64SubjectClose));
    str += ",\"rows_max\":" + std::to_string(stSnap.nTrackMax);
    str += ",\"active_max\":" + std::to_string(stSnap.nActiveMax);
    str += ",\"region_built\":" + std::to_string(stSnap.nRegionNum);
    str += ",\"region_configured\":" + std::to_string(stSnap.nRegionConfigured);
    str += ",\"unjudged\":" + std::to_string(stSnap.nUnjudged);
    str += ",\"cost_us\":" + std::to_string(static_cast<long long>(stSnap.llLastCostUs));

    str += ",\"reason\":{";
    for (int i = 0; i < static_cast<int>(SampleReason_E::SR_NUM); ++i)
    {
        if (i > 0)
        {
            str += ",";
        }
        str += std::string("\"") + reasonName(static_cast<SampleReason_E>(i)) + "\":" +
               std::to_string(static_cast<long long>(stSnap.au64Reason[i]));
    }
    str += "}";

    /* 每列行为的生效人数：只列非 0 项，一条都不活动时是空对象。
       这里不读快照也不读门表（都在判定线程被改），只用发布件，跨线程才有保证 */
    str += ",\"active\":{";
    bool bFirst = true;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        if (anActive[nCol] <= 0)
        {
            continue;
        }
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        if (!bFirst)
        {
            str += ",";
        }
        bFirst = false;
        str += std::string("\"") + behaviorName(enBehavior) + "\":" + std::to_string(anActive[nCol]);
    }
    str += "}}";

    return str;
}
}
