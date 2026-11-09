/**
 * @FilePath     : classroom/rules/duration_gate.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 15:30:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 15:30:00
 * @Description  : (personId × behavior) 时长门实现：一行按 personId 现场分配，不做取模
 */

#include "duration_gate.h"

namespace Classroom
{
int CDurationGate::behaviorColumn(Behavior_E enBehavior)
{
    const int nValue = static_cast<int>(enBehavior);

    const int nStuFirst = static_cast<int>(Behavior_E::STU_STAND_UP);
    const int nTeaFirst = static_cast<int>(Behavior_E::TEA_STAND_UP);

    if (nValue >= nStuFirst && nValue < nStuFirst + STUDENT_COLUMN_NUM)
    {
        return nValue - nStuFirst;
    }
    if (nValue >= nTeaFirst && nValue < nTeaFirst + TEACHER_COLUMN_NUM)
    {
        return STUDENT_COLUMN_NUM + (nValue - nTeaFirst);
    }
    return -1;
}

bool CDurationGate::behaviorAtColumn(int nCol, Behavior_E &renBehavior)
{
    const int nStuFirst = static_cast<int>(Behavior_E::STU_STAND_UP);
    const int nTeaFirst = static_cast<int>(Behavior_E::TEA_STAND_UP);

    if (nCol >= 0 && nCol < STUDENT_COLUMN_NUM)
    {
        renBehavior = static_cast<Behavior_E>(nStuFirst + nCol);
        return true;
    }
    if (nCol >= STUDENT_COLUMN_NUM && nCol < BEHAVIOR_COLUMN_NUM)
    {
        renBehavior = static_cast<Behavior_E>(nTeaFirst + (nCol - STUDENT_COLUMN_NUM));
        return true;
    }
    return false;
}

int CDurationGate::columnCount()
{
    return BEHAVIOR_COLUMN_NUM;
}

int CDurationGate::findRow(int nPersonId) const
{
    if (nPersonId < 0)
    {
        return -1;
    }
    for (int nRow = 0; nRow < ROW_NUM; ++nRow)
    {
        if (m_anRowOwner[nRow] == nPersonId)
        {
            return nRow;
        }
    }
    return -1;
}

void CDurationGate::clearRow(int nRow)
{
    if (nRow < 0 || nRow >= ROW_NUM)
    {
        return;
    }
    for (int nCol = 0; nCol < BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        m_astGates[nRow][nCol] = CGateState_S();
    }
    m_anRowOwner[nRow] = -1;
}

int CDurationGate::rowOf(int nPersonId)
{
    if (nPersonId < 0)
    {
        return -1;   /* 调用错误：不占行，feed 里也会在任何状态改动之前挡掉 */
    }

    const int nOwned = findRow(nPersonId);
    if (nOwned >= 0)
    {
        return nOwned;
    }

    for (int nRow = 0; nRow < ROW_NUM; ++nRow)
    {
        if (m_anRowOwner[nRow] < 0)
        {
            m_anRowOwner[nRow] = nPersonId;
            return nRow;
        }
    }

    /* 一行都腾不出来：这一拍同时在门里挂着的人数超过 ROW_NUM。
       只计数不挤人 —— 挤掉的那个人如果正有行为生效，他名下那条 END 就永远补不出来了
       （引擎收尾要先查得到门里的状态才发得出事件），那是错数据；
       而这一个人这一拍没账，是少数据，且 rowDenyNum 看得到 */
    ++m_u64RowDeny;
    return -1;
}

CGateState_S& CDurationGate::mutableState(int nRow, int nCol, int nPersonId)
{
    CGateState_S &stGate = m_astGates[nRow][nCol];

    /* 格子与行主不一致只可能是"这一格还是上一任的"：整格清掉再给他，
       否则新来的人第一次喂入就继承了别人的"低头已攒 1.2 秒"，第一拍就满足最短持续时间。
       清零就是默认构造本身（四个时基字段在类型上已是 GATE_TIME_NONE），这里不再重抄一遍 */
    if (stGate.nOwnerId != nPersonId)
    {
        stGate = CGateState_S();
        stGate.nOwnerId = nPersonId;
    }
    return stGate;
}

CGateResult_S CDurationGate::feed(int nPersonId, Behavior_E enBehavior, TriState_E enSample,
                                  int64_t llNowMs, const CGateParam_S &stParam, int8_t nAux)
{
    CGateResult_S stResult;

    const int nCol = behaviorColumn(enBehavior);
    if (nCol < 0 || nPersonId < 0)
    {
        /* 配置或调用错误：不改动任何状态，直接返回未生效，避免脏数据进统计 */
        return stResult;
    }

    const int nRow = rowOf(nPersonId);
    if (nRow < 0)
    {
        /* 行池已满（计数在 rowOf 里加，这里不重复加）：本次喂入等于没发生过。
           返回的是 bAccepted=false，上层会把它记进 throttled 一类（"这一拍什么都没改"），
           真实原因看 rowDenyNum —— 两条计数同时非 0 才说明不是节拍快而是人太多，
           这与"采样间隔配小了"是两种不同的处置 */
        return stResult;
    }

    CGateState_S &stGate = mutableState(nRow, nCol, nPersonId);

    /* 功能被关掉：正在生效的必须先收尾，否则平台侧会看到永不结束的事件；
       没生效的也要作废——攒到一半的锚点若留着，"关掉这一列"再打开时，
       第一拍 held 就是「now - 关之前那次 TRUE」，最短时长门被一段关着的时间凑满，
       事件起点还回推到开关落下之前。作废与收尾用同一对字段，没有第二种口径 */
    if (!stParam.bEnable)
    {
        if (stGate.bActive)
        {
            stGate.bActive = false;
            stResult.bDeactivated = true;
            stResult.llHeldMs = llNowMs - stGate.llActiveSinceMs;
        }
        stGate.llTrueSinceMs = GATE_TIME_NONE;
        stGate.nTrueStreak = 0;
        stResult.bActive = false;
        return stResult;
    }

    /* 采样间隔节流：未到间隔的帧不改变任何判定，只回报当前状态 */
    if (stGate.llLastSampleMs != GATE_TIME_NONE &&
        (llNowMs - stGate.llLastSampleMs) < stParam.nSampleIntervalMs)
    {
        stResult.bAccepted = false;
        stResult.bActive = stGate.bActive;
        if (stGate.bActive)
        {
            stResult.llHeldMs = llNowMs - stGate.llActiveSinceMs;
        }
        return stResult;
    }

    const int64_t llPrevSampleMs = stGate.llLastSampleMs;   /* 本拍采纳**之前**上一次被采纳的时刻 */
    stGate.llLastSampleMs = llNowMs;
    stGate.enLastSample = enSample;
    stResult.bAccepted = true;

    /* 中断是否已超容忍额度：UNKNOWN 与 FALSE 都算中断，但 FALSE 在"未生效"分支里会额外清零计时 */
    const bool bGapExceeded = (stGate.llLastTrueMs == GATE_TIME_NONE)
                              ? false
                              : ((llNowMs - stGate.llLastTrueMs) >= stParam.nGapToleranceMs);

    /**
     * @brief TRUE 样本用的另一把尺：真正空过了一段，而不是"隔了一个采样周期又来了一次"
     * @note  不能拿 llNowMs-llLastTrueMs 去比容忍额度：连续 TRUE 的相邻样本本来就相隔一个
     *        采样间隔，那段间隔是**名义节拍**不是中断。不扣掉它的后果是
     *        gap_tolerance_ms=0 的行为（教师跟踪不设时长门，容忍额度自然也是 0）
     *        每来一个 TRUE 都"断档"一次：事件每拍结束再开始，一节 40 分钟的课被切成几百段，
     *        而平台按 BEGIN 计次会把「上台一次」报成几百次。
     *        量「上一次被采纳的样本离上次 TRUE 多久」才是要的那件事：中间若全是 TRUE，
     *        这个值是 0；真被 UNKNOWN/FALSE 隔开了，它就是那段中断的长度。
     *        至于"一整段时间根本没有样本"（人离场、感知停摆）不归这里判——
     *        前者由 retireGone 当场补 END（一拍之内就发），后者由引擎的断档收尾负责，
     *        那两处都有比容忍额度更准的时间常数，在这里再判一遍只会把节拍抖动当成断档
     */
    const int64_t llInterruptMs = (GATE_TIME_NONE == stGate.llLastTrueMs || GATE_TIME_NONE == llPrevSampleMs)
                                  ? 0 : (llPrevSampleMs - stGate.llLastTrueMs);
    const bool bTrueGapExceeded = (llInterruptMs > static_cast<int64_t>(stParam.nGapToleranceMs));

    if (TriState_E::TS_TRUE == enSample)
    {
        /* 生效中但证据确实中断过：先补一次结束，再按新一轮重新开始，避免"卡在不生效也不结束" */
        if (stGate.bActive && bTrueGapExceeded)
        {
            stGate.bActive = false;
            stResult.bDeactivated = true;
            stResult.llHeldMs = llNowMs - stGate.llActiveSinceMs;
        }

        /* 本轮计时的起点为空（第一轮、或上一轮已被 FALSE/关闭作废）就必须重新起算。
           只看 llLastTrueMs 会漏掉这一格：作废的那两条路径清的是 llTrueSinceMs，
           而 llLastTrueMs 仍留着最后一个 TRUE——于是 held 是「now - (-1)」，
           被丢掉的那一轮当场生效，最短持续时间形同没有配过。
           llLastTrueMs 只负责回答另一件事：这段中断到底有多长 */
        if (GATE_TIME_NONE == stGate.llTrueSinceMs || bTrueGapExceeded)
        {
            stGate.llTrueSinceMs = llNowMs;
            stGate.nTrueStreak = 0;
        }
        stGate.llLastTrueMs = llNowMs;
        ++stGate.nTrueStreak;
        /* 类别跟着 TRUE 走（#88）：事件存活期间的每拍 TRUE 都可能带来不同类别，
           存最后一个——END 事件要报的是「最后维持住事件的那个表情」 */
        stGate.nAux = nAux;

        if (!stGate.bActive)
        {
            const int64_t llHeld = llNowMs - stGate.llTrueSinceMs;
            if (llHeld >= stParam.nMinDurationMs && stGate.nTrueStreak >= stParam.nMinTrueCount)
            {
                stGate.bActive = true;
                /* 事件起始时间回推到本轮第一个 TRUE，而不是"确认时刻"，与现网事件语义一致 */
                stGate.llActiveSinceMs = stGate.llTrueSinceMs;
                stResult.bActivated = true;
                stResult.llHeldMs = llHeld;
            }
        }
    }
    else
    {
        if (stGate.bActive)
        {
            if (bGapExceeded)
            {
                stGate.bActive = false;
                stResult.bDeactivated = true;
                stResult.llHeldMs = llNowMs - stGate.llActiveSinceMs;
                stGate.llTrueSinceMs = GATE_TIME_NONE;
                stGate.nTrueStreak = 0;
            }
        }
        else if (TriState_E::TS_FALSE == enSample && bGapExceeded)
        {
            /* 明确判为"否"才丢弃本轮计时；UNKNOWN 只是留着等容忍额度内回归 */
            stGate.llTrueSinceMs = GATE_TIME_NONE;
            stGate.nTrueStreak = 0;
        }
    }

    stResult.bActive = stGate.bActive;
    if (stGate.bActive && GATE_TIME_NONE != stGate.llActiveSinceMs)
    {
        stResult.llHeldMs = llNowMs - stGate.llActiveSinceMs;
    }
    return stResult;
}

void CDurationGate::retirePerson(int nPersonId)
{
    const int nRow = findRow(nPersonId);
    if (nRow < 0)
    {
        return;   /* 他一行都没占（退役两次、或从没被喂过）：不必也不能清别人的行 */
    }
    /* 整行放开。取模版本这里只能"挑出属于他的格子清掉"，因为那一行可能同时住着两个人；
       行与一一对应之后，回收才是一行的事而不是逐格的事 */
    clearRow(nRow);
}

bool CDurationGate::isActive(int nPersonId, Behavior_E enBehavior) const
{
    const int nCol = behaviorColumn(enBehavior);
    const int nRow = findRow(nPersonId);
    if (nCol < 0 || nRow < 0)
    {
        return false;
    }
    /* 行是他的，但这一格可能还没被喂过（他这一拍没跑这个行为）。格子上的主人不能省：
       省了之后调用方拿到的是一个全零却"看起来合法"的状态，分不清"这个行为从没为他判过"
       与"判过、结论为假" —— 上层正是靠 getState 的 false 来跳过一条没收尾的账的 */
    const CGateState_S &stGate = m_astGates[nRow][nCol];
    return (stGate.nOwnerId == nPersonId) && stGate.bActive;
}

bool CDurationGate::isKnown(int nPersonId, Behavior_E enBehavior) const
{
    const int nCol = behaviorColumn(enBehavior);
    const int nRow = findRow(nPersonId);
    if (nCol < 0 || nRow < 0)
    {
        return false;
    }
    const CGateState_S &stGate = m_astGates[nRow][nCol];
    return (stGate.nOwnerId == nPersonId) && (TriState_E::TS_UNKNOWN != stGate.enLastSample);
}

bool CDurationGate::getState(int nPersonId, Behavior_E enBehavior, CGateState_S &stOut) const
{
    const int nCol = behaviorColumn(enBehavior);
    const int nRow = findRow(nPersonId);
    if (nCol < 0 || nRow < 0)
    {
        return false;
    }
    if (m_astGates[nRow][nCol].nOwnerId != nPersonId)
    {
        return false;
    }
    stOut = m_astGates[nRow][nCol];
    return true;
}

void CDurationGate::reset()
{
    for (int nRow = 0; nRow < ROW_NUM; ++nRow)
    {
        clearRow(nRow);
    }
    /* 与仲裁器同一口径：拒绝计数跟着表一起归零，读的是"这一节课/这一次开关以来"的溢出。
       不归零的话 reset 之后 statusJson 会一直挂着一个上一轮的数，而那一轮的行池早就空了 */
    m_u64RowDeny = 0;
}

int CDurationGate::occupiedCount() const
{
    int nCount = 0;
    for (int nRow = 0; nRow < ROW_NUM; ++nRow)
    {
        for (int nCol = 0; nCol < BEHAVIOR_COLUMN_NUM; ++nCol)
        {
            if (m_astGates[nRow][nCol].nOwnerId >= 0)
            {
                ++nCount;
            }
        }
    }
    return nCount;
}

int CDurationGate::occupiedRowNum() const
{
    int nCount = 0;
    for (int nRow = 0; nRow < ROW_NUM; ++nRow)
    {
        if (m_anRowOwner[nRow] >= 0)
        {
            ++nCount;
        }
    }
    return nCount;
}
}
