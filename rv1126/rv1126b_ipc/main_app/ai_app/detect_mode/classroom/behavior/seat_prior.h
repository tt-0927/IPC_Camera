/**
 * @FilePath     : seat_prior.h
 * @Author       : leiyy
 * @Date         : 2026-10-27 13:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-27 13:57:00
 * @Description  : 座位先验（「格子 → 座位」那一层）
 */

#ifndef CLASSROOM_BEHAVIOR_SEAT_PRIOR_H
#define CLASSROOM_BEHAVIOR_SEAT_PRIOR_H

#include <cstdint>
#include <string>

#include "seat_heat.h" 

namespace Classroom
{
/* 一格最多切几个座位。默认档下一格≈1~2 座，取 4 是给"席区画得粗"的机位留余量 */
#define CLASSROOM_SEAT_PRIOR_MAX_PER_CELL (4)
#define CLASSROOM_SEAT_PRIOR_MAX_SLOTS \
    (CLASSROOM_SEAT_MAX_CELLS * CLASSROOM_SEAT_PRIOR_MAX_PER_CELL)

/* 座位号的代次取模。格子号只对"当前这张图"有意义（seatOf 同一条纪律），所以号里必须编进代次，
   否则席区重画之后，新图里的 3 号座会认领旧图里 3 号座那笔还在账本上的历史*/
#define CLASSROOM_SEAT_PRIOR_EPOCH_WRAP (256)


/**
 * @brief 座位先验参数。刻意**不复用**热力的 `n_idle_beats`：那一处踢人是"这格不算他了"，
 *        本处让座是"下一个人可以拿这个号"，后者错一次的代价是名单少一个人，两件事的余量不该互相顶
 */
typedef struct _SeatPriorConf_
{
    /* 这个开关管的是**消费**，不是这张表跑不跑：关掉时引擎照样每采样拍折样、照写在心跳的
       sprior= 段（那才是"该不该开"的证据来源），只是考勤不认这些号，`"seat":N` 退回帧行上那个格子号 */
    bool     bEnable = false;
    /* 离开镜头多久之内再出现，仍按"同一个人坐回原处"认（拍数） */
    int32_t nInheritBeats = 3;
    /* 离开多久之后这个号彻底让出（拍数）。必须**大于** nInheritBeats */
    int32_t nReleaseBeats = 6;
} CSeatPriorConf_S;

/**
 * @brief 一拍的逐人输入。刻意用三个裸字段而不是喂 `CSeatObs_S`：
 *        格子号是热力**折完之后**的结果，位置是折样的那一拍的位置，
 *        让调用方显式把这两样交过来，比在本层再算一次格子少一处会漂移的口径
 */
typedef struct _SeatPriorIn_
{
    int32_t nPersonId = -1;    /* 跟踪 ID，<0 = 空行 */
    int32_t nCell = -1;        /* CSeatHeat::seatOf() 的结果；<0 = 这一拍他没有格子（走道/撤图/没被看着）*/
    float   fNormX = -1.0f;    /* 源帧归一化横坐标；NaN 或 <0 = 没有位置证据 */
} CSeatPriorIn_S;

/**
 * @brief 一拍里发生的一次「就近继承」配对：旧 ID 把这一席交给了新 ID
 */
typedef struct _SeatPriorPair_
{
    int32_t nTrackOld = -1;    /* 这一席上原来挂着的那个人（本拍没被看着） */
    int32_t nTrackNew = -1;    /* 就近认领这一席的新 ID */
} CSeatPriorPair_S;


/**
 * @brief   : 参数自检（形状，不跨块）
 * @return   {bool} false 时 rstrErr 给出原因
 */
bool seatPriorParamOk(const CSeatPriorConf_S &rstConf, std::string &rstrErr);

/**
 * @brief   : 跨块算术：本层的让号速度必须慢过考勤账本的遗忘速度
 * @param    {int32_t} nSampleMs  采样周期（`sample.group_interval_ms`）
 * @param    {int64_t} llRowIdleMs 考勤行的闲置上限（`attendance.row_idle_ms`）
 * @return   {bool} false = 这样配会让"离场的人"与"坐上那个位置的人"共用一个座位号
 */
bool seatPriorIdleOk(int32_t nReleaseBeats, int32_t nSampleMs, int64_t llRowIdleMs,
                     std::string &rstrErr);

/**
 * @brief 座位先验表
 */
class CSeatPrior
{
public:
    CSeatPrior();

    /**
     * @brief   : 折一拍，把 (人, 格, 横坐标) 贴到座位表上
     * @param    {const CSeatPriorIn_S*} patIn：本拍的逐人输入，与热力折样同序
     * @param    {int32_t} nNum：行数（0 是合法的"空教室"，负数与空指针按不折处理）
     * @param    {uint64_t} u64GridEpoch：格子图的代次。变了就整表作废——**必须在折样之前取**，
     *             理由见 CSeatHeat::gridEpoch()
     * @param    {float} fCellW：一格的归一化宽度，决定"隔多远就不算坐回同一座"的门限
     *             （门限 = 格宽 ÷ 每格席数 = **一个席宽**，理由见 seat_prior.cpp 的 seatPriorGate）；
     *             <=0 时按"没有可用的格子宽度"处理，只维持已有的号、不发新的
     */
    void fold(const CSeatPriorIn_S *patIn, int32_t nNum, uint64_t u64GridEpoch, float fCellW);

    /**
     * @brief   : 某人当前的座位号，没有先验返回 -1
     */
    int32_t seatIdOf(int32_t nPersonId) const;

    /* 当前有座人数（含"人已走但号还留着"的那些），心跳与主机用例用它查守恒 */
    int32_t seatedNum() const;

    /**
     * @brief   : 取本次 fold 的继承配对表（见 CSeatPriorPair_S：为什么发号之外还要交出这一样）
     * @param    {int32_t&} rnNum：OUT=条数，0 = 这一拍没有第 2 条契约的证据
     * @return   {const CSeatPriorPair_S*} 表首址；**只在本次 fold 之后、下一次 fold 之前有效**
     */
    const CSeatPriorPair_S *inheritPairs(int32_t &rnNum) const;


    /* 心跳一行（每条都是"计数"，一个姓名、一个坐标都不带）：
       `|sprior beat= hold= inherit= new= steal= noslot= nocell= wipe= seated= epoch=`
       读法见 CStat_S 那六项——本层的失效全是"号发错了"这一类，报表上看不出来，只能靠比例 */
    std::string statText() const;

    /* 一节课收尾（与 CSeatHeat::reset 同一处调用点）。清了表也清计数：
       留着上一节的 inherit 数，下一节看着像"这班人爱换座" */
    void reset();

    void setParam(const CSeatPriorConf_S &rstConf);
    const CSeatPriorConf_S &param() const { return m_stConf; }

private:
    /* 一个座位格（格内一席）。nPersonId 有三态，两笔"没有号主"必须分开：
       >=0 = 这个号现在挂在某人身上（他可能在场，也可能刚走——走了也还挂着，见下面的让号期）；
       -1    = 本代次这一席从没被领过（fX 也没有值，不能参与就近判据）；
       -2    = 号主**搬走了**（他在别的格子重新领了号），这一席只留一个横坐标锚点等别人回来坐。
       第三态是必需的：搬走的人那一行在考勤里还活着（他换了个号继续被记），
       而这一席的锚点又得留着给"同格另一个人坐回原位"用——抹成 -1 就丢了锚点，
       留着他的 ID 就会让他同时挂着两个号（seatIdOf 从此要挑一个，而挑哪个都是猜） */
    typedef struct _Slot_
    {
        int32_t  nPersonId = -1;
        float    fX = -1.0f;        /* 上一个号主被看到时的横坐标；-1 = 没有锚点 */
        uint64_t u64LastBeat = 0;   /* 第几拍看到他（拍号，不用时刻——与热力同一口径）*/
    } CSlot_S;

    int32_t slotIndexOf(int32_t nCell, int32_t nOrdinal) const;
    /* 反查一个槽位属于哪一格（表就是按 nCell*每格席数 铺的，不另存一份格子号） */
    int32_t slotCellOf(int32_t nSlot) const;
    int32_t indexOf(int32_t nPersonId) const;
    void    wipe();

    CSeatPriorConf_S m_stConf;
    /* 本次 fold 的继承配对（见 inheritPairs）。与表本体同一口径定长、构造期定容，
       每拍 fold 开头清零——它只活一拍，所以不需要老化、不需要随代次作废 */
    CSeatPriorPair_S m_astPair[CLASSROOM_MAX_PERSONS];
    int32_t m_nPairNum = 0;

    /* 定长数组而不是 vector：8 KB 的常驻换来"本表容量是一个可以在心跳里念出来的常数" */
    CSlot_S m_astSlot[CLASSROOM_SEAT_PRIOR_MAX_SLOTS];
    uint64_t m_u64Beat = 0;
    uint64_t m_u64Epoch = 0;
    bool     m_bEpochSeen = false;

    /**
     * @brief 六个计数，各自挡一种"静默错账"。本层的失效没有一条会崩，
     *        只会让某个人领到别人的号（名单少一个）或领不到号（复用失效，名单多一个）
     */
    typedef struct _Stat_
    {
        uint64_t u64Hold = 0;      /* 号主本拍还在自己那格原位（最常见的一条路，量它是为了看比例）*/
        uint64_t u64Inherit = 0;   /* 换 ID 的人在**一个席宽**之内就近认领一个还没过让号期的空号
                                      —— 第 2 条契约在生效。这个数是本层的收益：它每 +1，
                                      考勤就少起一行 */
        uint64_t u64New = 0;       /* 领了一个本代次从没被占过的座位（表在随教室人数长）*/
        uint64_t u64Steal = 0;     /* 领了一个**已过让号期**的号（同席回收与远处借用两档都算在这里，
                                      第 3 条契约的门放行之后才允许）—— 课间换人坐同一席是它的正当用法；
                                      它若在一节课中间猛涨，说明跟踪 ID 在频繁重建而锚点没接住
                                      （席宽门限太紧，或者一次组轮转把空窗拉过了 inherit_beats）*/
        uint64_t u64NoSlot = 0;    /* 一格里挤不下（每格上限 CLASSROOM_SEAT_PRIOR_MAX_PER_CELL），
                                      这一拍发不出号：宁可 -1，绝不重号。持续非零 = 席区画太粗，
                                      把三排人圈进了一格 */
        uint64_t u64NoCell = 0;    /* 有身份但没格子：撤图 / 坐过道 / 热力还没认下他。
                                      非零不是错，但它决定 none 的比例有多高 */
        uint64_t u64EpochWipe = 0; /* 图换代、整表作废的次数（+1 之后所有号都换了一批含义）*/
    } CStat_S;
    CStat_S m_stStat;
};

class CAttendanceLedger;    /* 前向声明：只有下面这个消费出口用到，不在头文件里引账本 */

/**
 * @brief   : 把本次 fold 的继承证据交给考勤
 * @param    {CSeatPrior&} stPrior 刚折过样的那张表
 * @param    {bool} bConsume  false = 先验只在测量不发言：一条证据也不送
 * @param    {CAttendanceLedger&} stLedger 要并行的那本账
 * @return   {int32_t} 真的并掉一行的次数（0 是常态：没有证据、或证据里的行已不在表上）
 */
int32_t applySeatPriorLinks(const CSeatPrior &stPrior, bool bConsume, CAttendanceLedger &stLedger);


}    /* namespace Classroom */

#endif    /* CLASSROOM_BEHAVIOR_SEAT_PRIOR_H */
