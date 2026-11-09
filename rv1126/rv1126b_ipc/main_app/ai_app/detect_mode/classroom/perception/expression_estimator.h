/**
 * @FilePath     : expression_estimator.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 16:05:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 16:05:00
 * @Description  : 表情/情绪判定与逐人平滑
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* 表情模型的输出槽位上限 */
#define CLASSROOM_EXPRESSION_SLOT_MAX (16)

/**
 * @brief   : 判定失败的原因
 */
typedef enum class ExprFail : int
{
    EF_OK = 0,
    EF_NULL_INPUT,      /* 空指针 / 空输出 */
    EF_LENGTH,          /* 浮点个数与配置的类别数不符：后端契约变了，必须当场发现 */
    EF_NOT_FINITE,      /* NaN / inf */
    EF_LOW_PROB,        /* top-1 概率不足 */
    EF_LOW_MARGIN,      /* top1 与 top2 之差不足：这一帧是「猜不出来」而不是「情绪平淡」 */
    EF_SOFTMAX_BAD,     /* 概率归一化失败（全等 -inf 之类），与 LOW_PROB 区别在于分子就没有效 */
    EF_LOW_QUALITY,     /* 人脸质量不足，本拍不该判（调用方传入门限） */
    EF_SLOT_MAP,        /* 槽位映射表非法（含 EXP_UNKNOWN 或越界） */
} ExprFail_E;

/**
 * @brief   : 槽位映射表的**来历**（只记解析那一步知道的事，内容对不对由 expressionParamOk 判，
 *            "有没有人标过"由 expressionSlotMapState 判——那是三个不同的问题）
 */
typedef enum class SlotMapSrc : int
{
    SMS_GUESS = 0,    /* 配置里没这个键 */
    SMS_CFG,          /* 配置给了整张并被采纳 */
    SMS_REJECTED,     /* 配置给了但整张被拒（超长 / 名字不认识 / 元素类型不对 / 压根不是数组），
                         生效的仍是出厂那张。这一档要单独有名字：处置方向是"去查为什么没生效"，
                         与"去标定"相反 */
} SlotMapSrc_E;

/**
 * @brief   : 出厂那张映射表（= 下面 anSlotMap 的初值，单列一份是为了"有没有人动过"有可比对象）
 */
extern const Expression_E AEXP_SLOT_MAP_DEFAULT[CLASSROOM_EXPRESSION_SLOT_MAX];

/**
 * @brief 表情判定参数
 */
typedef struct _ExpressionParam_
{
    /* 模型输出的类别个数，即打平浮点的长度闸门。0 = 该功能未注册，判定层直接拒 */
    int32_t nSlotNum = 7;
    /**
     * 槽位 → 语义标签。默认按「平静/开心/惊讶/悲伤/愤怒/恐惧/厌恶」这个常见 7 类排法，
     * **交付后必须以模型侧给的顺序表为准覆写**。写在这里的意义是它有一个明确默认的形态，
     * 于是"到位只改这张表"是改一行配置而不是一段 if/else
     */
    Expression_E anSlotMap[CLASSROOM_EXPRESSION_SLOT_MAX] = {
        Expression_E::EXP_ANGRY,  Expression_E::EXP_DISGUST, Expression_E::EXP_FEAR,
        Expression_E::EXP_HAPPY,  Expression_E::EXP_NEUTRAL, Expression_E::EXP_SAD,
        Expression_E::EXP_SURPRISE,
        Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN,
        Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN,
        Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN
    };

    SlotMapSrc_E enSlotMapSrc = SlotMapSrc_E::SMS_GUESS;
    /* top-1 概率下限（softmax 之后）。0.45 是「七个类里明显赢的那个」的保守档 */
    float fProbMin = 0.45f;
    /* top1 - top2 的下限。刻意与上一行分开配：情绪模型的绝对概率随温度/归一化习惯漂移，
       而「赢得够不够多」才是可跨模型比较的量 */
    float fMarginMin = 0.12f;
    /* EMA 新样本权重。0.25 ≈ 平滑约 4 帧：表情是一次 1~2s 的事，
       再平滑就把它抹平了；取 0.5 以上则单帧抖动直接进曲线 */
    float fEmaAlpha = 0.25f;
    /* 冷启动最少几帧：不足时曲线值一律不给（给就是拿一帧当稳定值） */
    int32_t nMinFrames = 2;
    /* 曲线输出的最低平滑概率：EMA 向量的 top1 低于此值说明这个人一直在几种情绪之间摇摆，
       此时曲线该断而不是画一条穿中间的值 */
    float fCurveProbMin = 0.40f;
    /* 允许参与表情识别的最大人脸偏转角 */
    float fMaxAngleDeg = 90.0f;
} CExpressionParam_S;

/**
 * @brief 一帧的判定结果（瞬时口径，事件用）
 */
typedef struct _ExpressionSample_
{
    bool bDecided = false;                 /* false = 判不出来，此时以下字段不可信 */
    Expression_E enLabel = Expression_E::EXP_UNKNOWN;
    float fProb = 0.0f;                    /* top-1 的 softmax 概率 */
    float fMargin = 0.0f;                  /* top1 - top2 */
    ExprFail_E enReason = ExprFail_E::EF_OK;
    /**
     * top-1 落在**哪个槽位**（-1 = 连概率都没算出来）
     */
    int32_t nArgmaxSlot = -1;
} CExpressionSample_S;

/**
 * @brief 表情路观测计数（心跳与 statusJson 出口）
 */
typedef struct _ExpressionStat_
{
    uint64_t u64Tried = 0;
    uint64_t u64Decided = 0;
    uint64_t u64LowProb = 0;
    uint64_t u64LowMargin = 0;
    uint64_t u64LengthBad = 0;
    uint64_t u64NotFinite = 0;
    uint64_t u64SoftmaxBad = 0;
    uint64_t u64SlotMapBad = 0;
    uint64_t u64CurveReady = 0;       /* 曲线给得出值的次数（与 u64Tried 之比就是覆盖率） */
    uint64_t u64CurveCold = 0;        /* 因冷启动帧数不足而没给曲线值的次数 */
    uint64_t u64IdFull = 0;           /* 逐人表满：只可能是 ID 洪泛，查跟踪器而不是查表情 */
    /**
     * top-1 落在各**槽位**的累计次数（下标即模型输出的下标，未经 anSlotMap 换语义）
     */
    uint64_t u64SlotHist[CLASSROOM_EXPRESSION_SLOT_MAX] = { 0 };
} CExpressionStat_S;

/**
 * @brief   : 一帧打平输出 → 瞬时判定
 * @param    {const std::vector<float>&} rvfLogits：模型输出，长度须等于 rstParam.nSlotNum
 * @param    {const CExpressionParam_S&} rstParam：门限与槽位映射
 * @param    {CExpressionSample_S&} rstOut：OUT=判定；bDecided=false 时 enReason 说明原因
 * @param    {CExpressionStat_S*} pstStat：可空，累加
 * @return   {bool} 等价于 rstOut.bDecided，单列是为了调用方写 if 时不必先记住读哪个字段
 */
bool decideExpression(const std::vector<float> &rvfLogits, const CExpressionParam_S &rstParam,
                      CExpressionSample_S &rstOut,
                      CExpressionStat_S *pstStat = nullptr);

/**
 * @brief   : 校验槽位映射表（配置层在落配置时调，运行期由 decideExpression 兜）
 * @param    {const CExpressionParam_S&} rstParam
 * @param    {std::string&} rstrErr：OUT=首个问题的一句话说明，供 warn 直接带上
 * @return   {bool} false 表示这张表不能用（越界、含 EXP_UNKNOWN、或 nSlotNum 越界）
 */
bool expressionParamOk(const CExpressionParam_S &rstParam, std::string &rstrErr);

/**
 * @brief   : 标签的文本（配置里的 slot_map、心跳里的 argmax 分布共用这一张表）
 * @return   {const char*} 静态串，永不为空；越界值给 "bad"
 */
const char *expressionName(Expression_E enExpr);

/**
 * @brief   : 标签的中文名（配置键与报文仍走 expressionName 的英文表）
 * @return   {const char*} 静态串，永不为空；越界值给 "表情异常"
 */
const char *expressionCnName(Expression_E enExpr);

/**
 * @brief   : 名字 → 标签
 * @return   {int32_t} 未知名字返回 -1（配置层据此报"值非法"，不静默退回中性）
 */
int32_t expressionOf(const char *pName);

/**
 * @brief   : 整张槽位映射表的一行文本（"neutral|happy|surprise|..."，只打到 nSlotNum）
 * @return   {std::string} 空表 / 槽位数非法时给 "bad"
 */
std::string expressionMapText(const CExpressionParam_S &rstParam);

/**
 * @brief   : 这张表现在**能不能当标定结论用**——心跳与 init 行共用这一个词
 * @return   {const char*} "guess"（生效的就是出厂那张猜的序）/ "declared"（与出厂序不同，
 *             有人为下标语义负过责）/ "rejected"（下发过但没被采纳，出厂序在生效）/ "bad"（越界）
 */
const char *expressionSlotMapState(const CExpressionParam_S &rstParam);

/**
 * @brief   : 逐人情绪平滑器（曲线口径）
 */
class CExpressionEstimator
{
public:
    CExpressionEstimator();

    CExpressionEstimator(const CExpressionEstimator &) = delete;
    CExpressionEstimator &operator=(const CExpressionEstimator &) = delete;

    /**
     * @brief   : 换参数（配置换版时调）
     */
    void setParam(const CExpressionParam_S &rstParam);
    const CExpressionParam_S &param() const { return m_stParam; }

    /**
     * @brief   : 吃一帧输出，同时更新瞬时判定与该人的平滑向量
     * @param    {int32_t} nTrackId：跟踪 ID，<0 拒收
     * @param    {rvfLogits / rstParam 见 decideExpression}
     * @param    {int64_t} llNowMs：单调时基
     * @param    {CExpressionSample_S&} rstSample：OUT=本帧瞬时判定（事件用它）
     * @return   {bool} 等价于 rstSample.bDecided
     */
    bool feed(int32_t nTrackId, const std::vector<float> &rvfLogits,
              int64_t llNowMs, CExpressionSample_S &rstSample);

    /**
     * @brief   : 取某人的曲线值（群体情绪曲线用它）
     * @param    {int64_t*} pllLastMs：可空。OUT=这个人最后一次被喂进概率的时刻（单调毫秒）。
     *            单列这一项是因为特征行的情绪列要带自己的时间戳：表情没有自己的节拍，
     *            同一个人在同一帧里"脸是两秒前的、情绪是十六秒前的"是常态，
     *            与 llPoseMs/llPhoneMs 存在的理由完全相同
     * @param    {bool} bCount：要不要把这次问记进 curve_ok/curve_cold
     * @return   {bool} false 表示无行 / 帧数不足 / 平滑概率不足，调用方按「此人本周期无情绪值」处理
     * @note     : 返回 false 时**不改动**出参（label/prob/lastMs 都不动），让调用方保留上一周期
     *            的读数以显式标成 unavailable，而不是静默画一个 0
     */
    bool curveOf(int32_t nTrackId, Expression_E &renLabel, float &rfProb,
                 int64_t *pllLastMs = nullptr, bool bCount = true) const;

    /**
     * @brief   : 取某人**最近一次判得出**的瞬时结论
     * @param    {int64_t*} pllLastMs：OUT=那一次喂入的时刻（不是最后一次喂入的时刻）
     * @return   {bool} false = 这个人从未有过判得出的样本（或行已被清）
     */
    bool latestOf(int32_t nTrackId, Expression_E &renLabel, float &rfProb,
                  int64_t *pllLastMs = nullptr) const;

    /**
     * @brief   : 清掉久没出现的行
     * @return   {int32_t} 被清掉的行数
     */
    int32_t prune(int64_t llNowMs, int64_t llTtlMs);

    void reset();
    CExpressionStat_S stat() const { return m_stStat; }

    /**
     * @brief   : 当前占用的行数（诊断用，与 CLASSROOM_MAX_PERSONS 之比即表占用率）
     */
    int32_t rowInUse() const;

    /**
     * @brief   : 观测读数的一行文本（挂进引擎心跳）
     */
    std::string statText() const;

private:
    /* 一行的平滑状态。概率向量按固定槽位铺在 m_vecEma 里，行号 × 槽位数取偏移，
       这样每人一个 std::vector 的 60 次小分配就没了 */
    struct SRow_S
    {
        int32_t nTrackId = -1;      /* <0 = 空槽 */
        int64_t llLastMs = 0;
        int32_t nFrames = 0;
        /* 最近一次**判得出**的瞬时结论（事件口径，见 latestOf）。与 EMA 分家存：
           EMA 是按固定槽位铺的向量，这里只留赢家；低概率帧不写，所以
           llLastDecidedMs 可能明显早于 llLastMs */
        Expression_E enLastDecided = Expression_E::EXP_UNKNOWN;
        float    fLastDecidedProb = 0.0f;
        int64_t  llLastDecidedMs = 0;
    };

    int32_t findRow(int32_t nTrackId) const;
    int32_t allocRow(int32_t nTrackId, int64_t llNowMs);

    CExpressionParam_S m_stParam;
    std::vector<SRow_S> m_vecRow;
    std::vector<float> m_vecEma;
    /* mutable 是给 curveOf 的：它只读状态，但「曲线给得出值的次数」这一覆盖率
       只能在读侧记，而读侧是个 const 函数。与其在 const 函数里 const_cast 改成员，
       不如把这一项显式声明成「观测计数，不是状态」 */
    mutable CExpressionStat_S m_stStat;
};
}
