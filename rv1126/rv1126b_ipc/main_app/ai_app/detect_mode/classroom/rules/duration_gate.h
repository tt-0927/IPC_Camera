/**
 * @FilePath     : duration_gate.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 15:30:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 15:30:00
 * @Description  : (personId × behavior) 时长门
 */

#pragma once

#include "classroom_define.h"

namespace Classroom
{
/* "从未发生过"的时基哨兵：时基由调用方传入，0 可能是合法时刻，故用负值。
   放在头文件是因为本结构体的默认成员初始化要用它——"默认构造出来的就是一格空账"这条
   不变式必须写在类型上，而不是写在每个把格子清零的函数里（漏一处就把 0 当成起点时刻） */
constexpr int64_t GATE_TIME_NONE = -1;

/**
 * @brief 单条 (personId, behavior) 门的状态
 */
struct CGateState_S
{
    int     nOwnerId = -1;                    /* 占据该槽位的 personId，-1 表示空槽 */
    bool    bActive = false;                  /* 当前是否处于该行为 */
    int64_t llTrueSinceMs = GATE_TIME_NONE;   /* 本轮 TRUE 起点（含容忍中断，不要求严格连续） */
    int64_t llLastTrueMs = GATE_TIME_NONE;    /* 最近一次判为 TRUE 的时刻，用于中断容忍 */
    int64_t llLastSampleMs = GATE_TIME_NONE;  /* 上次被采纳的采样时刻，用于采样间隔节流 */
    int64_t llActiveSinceMs = GATE_TIME_NONE; /* 进入生效的时刻（事件起始时间戳） */
    int     nTrueStreak = 0;                  /* 连续 TRUE 次数，仅用于观测与调参 */
    TriState_E enLastSample = TriState_E::TS_UNKNOWN;
    /* 最后一次被采纳的 TRUE 采样自带的附加读数（#88，112=表情类别），非 TRUE 不更新：
       end 事件要带的类别是「最后维持住事件的那个」，被 NEUTRAL/空窗抹掉就失去可核性 */
    int8_t  nAux = -1;
};

/**
 * @brief 一次喂入的结果
 */
struct CGateResult_S
{
    bool bAccepted = false;      /* 本次采样是否被采纳（未通过采样间隔则为 false） */
    bool bActivated = false;     /* 本次是否刚进入生效态 —— 上层据此发"事件开始" */
    bool bDeactivated = false;   /* 本次是否刚退出 —— 上层据此发"事件结束" */
    bool bActive = false;        /* 当前是否处于该行为 */
    int64_t llHeldMs = 0;        /* 已持续时间（生效中）或本轮尝试持续时长（未生效） */
};

/**
 * @brief 时长门参数，与 Web 下发字段一一对应
 */
struct CGateParam_S
{
    bool  bEnable = false;         /* 功能开关 */
    int   nSampleIntervalMs = 200; /* 状态采样间隔 */
    int   nMinDurationMs = 1000;   /* 最短持续时间 / 进入确认时间 */
    int   nGapToleranceMs = 500;   /* 中断容忍时间 */
    int   nMinTrueCount = 1;       /* 进入所需的最少 TRUE 次数（1 表示只看时长） */
};

/**
 * @brief 时长门表
 */
class CDurationGate
{
public:
    /* 行为列数由 Behavior_E 的两段编号算出，避免魔数与实际枚举漂移 */
    static constexpr int STUDENT_COLUMN_NUM =
        static_cast<int>(Behavior_E::STU_EXPRESSION) - static_cast<int>(Behavior_E::STU_STAND_UP) + 1;
    static constexpr int TEACHER_COLUMN_NUM =
        static_cast<int>(Behavior_E::TEA_IDENTITY) - static_cast<int>(Behavior_E::TEA_STAND_UP) + 1;
    static constexpr int BEHAVIOR_COLUMN_NUM = STUDENT_COLUMN_NUM + TEACHER_COLUMN_NUM;

    /* 行数 = 两代人数上限。一格里能同时占行的是两类人：
       ① 本拍在帧里的行（跟踪器上限 CLASSROOM_MAX_PERSONS）；
       ② 本拍已不在帧里、但还在 RETIRE_GRACE（2s）宽限其内等它回来的行（名单容量同样是 MAX）。
       两类各自都吃满时就是 2×MAX——旧值 MAX+8 的余量比一整代还小，画面切换/整班换位
       这类一次性 ID 置换会让新一代人的行被拒（u64GateDeny 应恒为 0 的判据当场破），
       被拒的人这两秒里一条行为都不出。容量按实际上限给，再往上（两代以内又整班换一次）
       仍然拒绝：那种洪泛本身就是检测失效，要让它继续以 gate_deny 现形，而不是无限长表。
       代价：120 行 × 25 列 × 56B ≈ 168KB（旧 68 行 ≈ 95KB） */
    static constexpr int ROW_NUM = CLASSROOM_MAX_PERSONS * 2;

    /* 构造即复位：行主登记表要**逐项**填 -1。数组的 `= {-1}` 只填第一项，其余全是 0，
       而"空行"的标记是 -1 不是 0 —— 那样一来除第一行外全表都算已占用，开机第一拍就走进
       "行池已满"，所有人一辈子拿不到槽位（表现为行为判定整体不触发，日志里刷拒绝计数）。
       这类初始化错一次就在构造期定死，比不初始化更难复现，故不靠默认成员初始化。
       格子的空账反而靠默认成员初始化（见 CGateState_S）：结构体成员能逐个带默认值，
       裸数组不能——两处用不同机制是语言限制，不是两种口味 */
    CDurationGate()
    {
        reset();
    }
    ~CDurationGate() = default;

    /**
     * @brief   : 行为枚举 → 表内列下标
     * @return   {int} 越界返回 -1，调用方必须当作配置错误处理
     * @note    : 学生 101~112 与教师 201~213 两段连续编号，故用算术映射而非 map，零堆分配
     */
    static int behaviorColumn(Behavior_E enBehavior);

    /**
     * @brief   : behaviorColumn 的逆映射，列下标 → 行为
     * @param    {int} nCol：列下标，[0, BEHAVIOR_COLUMN_NUM)
     * @param    {Behavior_E&} renBehavior：OUT=该列对应的行为，返回 false 时不被修改
     * @return   {bool} 越界返回 false
     * @note    : 逆映射必须与正映射同处：引擎有几处是「只有列号」（扫全表收尾、逐列报生效人数），
     *            没有它就得在调用方重抄一遍 101~112 / 201~213 的区间，等于多一处会与枚举漂移的地方
     */
    static bool behaviorAtColumn(int nCol, Behavior_E &renBehavior);

    /* 表内行为列数 */
    static int columnCount();

    /**
     * @brief   : 喂入一次几何判定结果，推进该 (personId, behavior) 的状态
     * @param    {int} nPersonId：跟踪 ID（>=0）
     * @param    {Behavior_E} enBehavior：行为
     * @param    {TriState_E} enSample：本次判定，UNKNOWN 表示证据不足
     * @param    {int64_t} llNowMs：单调毫秒时基，由调用方统一提供
     * @param    {CGateParam_S} stParam：该行为的三参数
     * @return   {CGateResult_S}
     * @note    : TS_UNKNOWN 既不算 TRUE 也不算硬中断 —— 它只消耗容忍额度，
     *            否则远焦抖动会让"低头/抬头"每帧重启计时，永远达不到最短持续时间
     */
    CGateResult_S feed(int nPersonId, Behavior_E enBehavior, TriState_E enSample,
                       int64_t llNowMs, const CGateParam_S &stParam, int8_t nAux = -1);

    /**
     * @brief   : personId 退役时回收其全部槽位
     * @note    : 需求 10 的长期稳定关键 —— 吸取 StudentBehavior::unInit() 泄漏教训，
     *            表本身是定长数组，但槽位若不回收会把最后一帧的状态泄漏给后来者
     */
    void retirePerson(int nPersonId);

    /* 查询当前是否处于某行为（供群体统计快照用） */
    bool isActive(int nPersonId, Behavior_E enBehavior) const;

    /* 这一格最近一次判定是否有结论（TRUE 或 FALSE 都算，UNKNOWN 不算）。
       与 isActive 是两个问题：isActive 问"现在算不算在发生"，本函数问"能不能下结论"。
       群体时段门的分母必须用这个 —— 拿"有骨骼证据的人数"当听讲的分母，会在没画讲台、
       又没有教师参照的机位上把"整节课判不出"报成"全班没人听讲" */
    bool isKnown(int nPersonId, Behavior_E enBehavior) const;

    /* 读取槽位状态；未占用返回 false */
    bool getState(int nPersonId, Behavior_E enBehavior, CGateState_S &stOut) const;

    /* 全表复位（功能整体关闭 / 配置热切换时调用） */
    void reset();

    /* 当前被占用的槽位数，仅用于自检与日志（按需扫描定长表，不做增量计数） */
    int occupiedCount() const;

    /* 当前占用的行数：与 occupiedCount 是两个问题 —— 前者问"表还剩多少行能给下一个人"，
       后者问"有多少 (人 × 行为) 在本拍有判定"。行池满到要拒人时，只有前者会先跳到上限 */
    int occupiedRowNum() const;

    /* 因行池已满而被拒的喂入次数，供引擎发布进 statusJson 的 rules 段（节拍线程内读，无锁） */
    uint64_t rowDenyNum() const { return m_u64RowDeny; }

private:
    /* 只查不分配：读侧（isActive/getState/retirePerson）用它拿行号，拿不到就是"这人在门里没有账"。
       读侧必须不能改表 —— 否则 statusJson 之类的旁路查询会顺手把一个人的行占住，
       回收时机反而被观测动作推迟了 */
    int findRow(int nPersonId) const;

    /**
     * @brief   : 写侧取行号（本类唯一会改行池的入口）：已有→续用，无→占空行，都没有→拒绝
     * @return   {int} 行号；-1 表示行池已满，本次喂入**不改任何状态**
     * @note    : 满了不挤掉别人：挤一行走会让那人名下已生效的行为连带 bActive 一起消失，
     *            而 END 是引擎在 retireGone 里查门补的 —— 查不到就永远不补，
     *            平台侧留下一条拉到底的事件。宁可让第 ROW_NUM+1 个人这一拍没账（少数据，
     *            有计数可看），也不要让第 3 个人的事件收不了尾（错数据，只有平台知道）
     */
    int rowOf(int nPersonId);

    /* 清一整行（25 列全部归未占用）并把行主登记为"没人" */
    void clearRow(int nRow);

    /**
     * @brief   : 取（行, 列）那一格，并保证它确实属于这个人
     * @note    : 上一任留下的状态必须整格清掉，否则新来的人一开机就继承别人的"低头已攒 1.2 秒"
     */
    CGateState_S& mutableState(int nRow, int nCol, int nPersonId);

    /* 定长二维表：ROW_NUM × 行为列数，构造即存在，运行期不再分配 */
    CGateState_S m_astGates[ROW_NUM][BEHAVIOR_COLUMN_NUM];
    /* 行主登记表：-1=空行，否则该行 25 列全归这个 personId。有了它，"这个人"与"他的那一行"
       才是一对一关系；取模版本里这个关系是隐含的，因而也就是可以被两个人同时满足的 */
    int32_t m_anRowOwner[ROW_NUM];

    /* 被拒绝的喂入次数：非 0 就说明"同时在门里挂着的人数"超过 ROW_NUM。
       除 reset 外只增不减，因为一节课里的偶发溢出正是这里要留下的唯一痕迹。
       本类刻意不打日志（它要能被主机单测直接编译、也要能拿回放复现同一串状态迁移，
       拖上 dlog 栈就得先有一个桩）——这件事由上层取计数后发进 statusJson 的 rules 段 */
    uint64_t m_u64RowDeny = 0;
};
}
