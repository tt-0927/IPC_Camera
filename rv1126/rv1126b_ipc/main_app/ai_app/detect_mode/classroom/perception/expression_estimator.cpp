/**
 * @FilePath     : expression_estimator.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 16:05:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 16:05:00
 * @Description  : 表情判定与逐人平滑实现
 */

#include "expression_estimator.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "dlog.h"

namespace Classroom
{
/* 出厂那张槽位映射表。与 CExpressionParam_S::anSlotMap 的初值是**两份同样的表**，
   一份给结构体当默认值（那里只能用花括号初值列表），一份留在这里当比对基准。
   不共读的理由写在头文件里；漂了的代价由主机用例逐格钉住（classroom_face_test）
    把模型输出的 int 槽位换成情绪时用的表是
       ANGER, DISGUST, FEAR, JOY, NEUTRAL, SADNESS, SURPRISE
   ——与这颗 112x112 七类 Expression 模型是同一族 */
const Expression_E AEXP_SLOT_MAP_DEFAULT[CLASSROOM_EXPRESSION_SLOT_MAX] = {
    Expression_E::EXP_ANGRY,  Expression_E::EXP_DISGUST, Expression_E::EXP_FEAR,
    Expression_E::EXP_HAPPY,  Expression_E::EXP_NEUTRAL, Expression_E::EXP_SAD,
    Expression_E::EXP_SURPRISE,
    Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN,
    Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN,
    Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN, Expression_E::EXP_UNKNOWN
};

namespace
{
/* 槽位标签是否在语义范围内。EXP_UNKNOWN(-1) 刻意算非法：它出现在映射表里
   等于那一类永远赢（见 expressionParamOk 的注释） */
bool slotLabelOk(Expression_E enLabel)
{
    const int nV = static_cast<int>(enLabel);
    return (nV > static_cast<int>(Expression_E::EXP_UNKNOWN)) && (nV <= static_cast<int>(Expression_E::EXP_DISGUST));
}

/**
 * @brief   : 校验 + softmax + 选优，一次做完
 * @param    {float*} afProb：OUT=长度 nSlotNum 的概率（调用方要给够空间）
 * @return   {bool} 等价于 rstOut.bDecided
 */
bool expressionProbs(const std::vector<float> &rvfLogits, const CExpressionParam_S &rstParam,
                     float *afProb,
                     CExpressionSample_S &rstOut, CExpressionStat_S *pstStat)
{
    rstOut.bDecided = false;
    rstOut.enLabel = Expression_E::EXP_UNKNOWN;
    rstOut.fProb = 0.0f;
    rstOut.fMargin = 0.0f;
    rstOut.enReason = ExprFail_E::EF_OK;
    rstOut.nArgmaxSlot = -1;

    if (nullptr == afProb)
    {
        rstOut.enReason = ExprFail_E::EF_NULL_INPUT;
        return false;
    }
    if (rstParam.nSlotNum <= 0 || rstParam.nSlotNum > CLASSROOM_EXPRESSION_SLOT_MAX)
    {
        rstOut.enReason = ExprFail_E::EF_LENGTH;
        if (nullptr != pstStat)
        {
            ++pstStat->u64LengthBad;
        }
        return false;
    }
    if (rvfLogits.size() != static_cast<size_t>(rstParam.nSlotNum))
    {
        rstOut.enReason = ExprFail_E::EF_LENGTH;
        if (nullptr != pstStat)
        {
            ++pstStat->u64LengthBad;
        }
        return false;
    }
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        if (!slotLabelOk(rstParam.anSlotMap[i]))
        {
            rstOut.enReason = ExprFail_E::EF_SLOT_MAP;
            if (nullptr != pstStat)
            {
                ++pstStat->u64SlotMapBad;
            }
            return false;
        }
    }

    float fMax = rvfLogits[0];
    for (const auto &fV : rvfLogits)
    {
        if (!std::isfinite(fV))
        {
            rstOut.enReason = ExprFail_E::EF_NOT_FINITE;
            if (nullptr != pstStat)
            {
                ++pstStat->u64NotFinite;
            }
            return false;
        }
        if (fV > fMax)
        {
            fMax = fV;
        }
    }

    double dSum = 0.0;
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        /* 减 fMax 再 exp：情绪模型的输出偶尔到 +80 量级，不减就溢出成 inf */
        const double dE = std::exp(static_cast<double>(rvfLogits[i]) - static_cast<double>(fMax));
        afProb[i] = static_cast<float>(dE);
        dSum += dE;
    }
    if (!std::isfinite(dSum) || dSum <= 0.0)
    {
        rstOut.enReason = ExprFail_E::EF_SOFTMAX_BAD;
        if (nullptr != pstStat)
        {
            ++pstStat->u64SoftmaxBad;
        }
        return false;
    }
    const float fInv = static_cast<float>(1.0 / dSum);
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        afProb[i] *= fInv;
    }

    /* top1 / top2 一次扫完。两个 -1 的初值是必须的：初值取 0 时，
       「所有概率都是 0」会被读成「第 0 类以 0 概率胜出」而不是「没有胜出者」 */
    int32_t nBest = -1;
    int32_t nSecond = -1;
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        if (nBest < 0 || afProb[i] > afProb[nBest])
        {
            nSecond = nBest;
            nBest = i;
        }
        else if (nSecond < 0 || afProb[i] > afProb[nSecond])
        {
            nSecond = i;
        }
    }

    const float fP1 = afProb[nBest];
    /* 只有一个槽位时没有第二名，差按满分算：这类模型（二分类/单情绪）不该被差值门挡死 */
    const float fP2 = (nSecond >= 0) ? afProb[nSecond] : 0.0f;

    /* argmax 槽位写在两道门**之前**：门限挡掉的那些帧同样要进槽位分布，
       否则"这台设备的模型到底把哪个槽位当基线"这一读数只在判得出时才存在，
       而判不出的那批（差值门吃掉的多数）正是标定映射表最需要看的那一半 */
    rstOut.nArgmaxSlot = nBest;
    rstOut.fProb = fP1;
    rstOut.fMargin = fP1 - fP2;
    if (nullptr != pstStat && nBest >= 0 && nBest < CLASSROOM_EXPRESSION_SLOT_MAX)
    {
        /* 与上一行同源而不是另找一处：直方图与 argmax 必须永远出自同一次归一化，
           分两处写迟早有一处漏掉某条早退路径，而漏掉的表现是"分布看着很平" */
        ++pstStat->u64SlotHist[nBest];
    }

    if (fP1 < rstParam.fProbMin)
    {
        rstOut.enReason = ExprFail_E::EF_LOW_PROB;
        if (nullptr != pstStat)
        {
            ++pstStat->u64LowProb;
        }
        return false;
    }
    if (rstOut.fMargin < rstParam.fMarginMin)
    {
        rstOut.enReason = ExprFail_E::EF_LOW_MARGIN;
        if (nullptr != pstStat)
        {
            ++pstStat->u64LowMargin;
        }
        return false;
    }

    /* 标签放在两道门之后才写：判不出时它保持入口处的 EXP_UNKNOWN。
       写成「概率最高那个类」看似被 bDecided 兜住了，但漏检 bDecided 的读者迟早会出现
       （OSD 与曲线那条路只读 label），而漏掉时的现象是「这节课所有人都在平静」——
       误标成平静是最贵的一种错法，因为它看起来像正常 */
    rstOut.enLabel = rstParam.anSlotMap[nBest];
    rstOut.bDecided = true;
    if (nullptr != pstStat)
    {
        ++pstStat->u64Decided;
    }
    return true;
}
}

bool expressionParamOk(const CExpressionParam_S &rstParam, std::string &rstrErr)
{
    if (rstParam.nSlotNum <= 0 || rstParam.nSlotNum > CLASSROOM_EXPRESSION_SLOT_MAX)
    {
        rstrErr = "slot num out of range";
        return false;
    }
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        if (!slotLabelOk(rstParam.anSlotMap[i]))
        {
            rstrErr = "slot map entry out of range or UNKNOWN";
            return false;
        }
    }
    if (!(rstParam.fProbMin >= 0.0f) || rstParam.fProbMin >= 1.0f)
    {
        rstrErr = "prob_min out of range";
        return false;
    }
    if (!(rstParam.fMarginMin >= 0.0f) || rstParam.fMarginMin >= 1.0f)
    {
        rstrErr = "margin_min out of range";
        return false;
    }
    if (!(rstParam.fEmaAlpha > 0.0f) || rstParam.fEmaAlpha > 1.0f)
    {
        rstrErr = "ema alpha out of range";
        return false;
    }
    /* 角度门取 [1,90]：0 与负值会把每张脸都判成超限 */
    if (!(rstParam.fMaxAngleDeg >= 1.0f) || !(rstParam.fMaxAngleDeg <= 90.0f))
    {
        rstrErr = "max angle out of range";
        return false;
    }
    return true;
}

const char *expressionName(Expression_E enExpr)
{
    switch (enExpr)
    {
        case Expression_E::EXP_UNKNOWN:   return "unknown";
        case Expression_E::EXP_NEUTRAL:   return "neutral";
        case Expression_E::EXP_HAPPY:     return "happy";
        case Expression_E::EXP_SURPRISE:  return "surprise";
        case Expression_E::EXP_SAD:       return "sad";
        case Expression_E::EXP_ANGRY:     return "angry";
        case Expression_E::EXP_FEAR:      return "fear";
        case Expression_E::EXP_DISGUST:   return "disgust";
        default:                          return "bad";
    }
}

const char *expressionCnName(Expression_E enExpr)
{
    /* 与 expressionName 同一对应关系，只服务日志）；配置键与报文仍走英文短名 */
    switch (enExpr)
    {
        case Expression_E::EXP_UNKNOWN:   return "未识别";
        case Expression_E::EXP_NEUTRAL:   return "平静";
        case Expression_E::EXP_HAPPY:     return "开心";
        case Expression_E::EXP_SURPRISE:  return "惊讶";
        case Expression_E::EXP_SAD:       return "悲伤";
        case Expression_E::EXP_ANGRY:     return "愤怒";
        case Expression_E::EXP_FEAR:      return "恐惧";
        case Expression_E::EXP_DISGUST:   return "厌恶";
        default:                          return "表情异常";
    }
}

int32_t expressionOf(const char *pName)
{
    if (nullptr == pName)
    {
        return static_cast<int32_t>(Expression_E::EXP_UNKNOWN);
    }
    /* 名字表与 expressionName 的 switch 是同一套词，改一处就要改另一处：
       刻意不做成一张表让两个函数共读，是因为共读的表要为"大小写/别名"留位置，
       而这里要的恰恰是**只有一个正确写法**（配置里写 "happY" 生效、写 "grin" 不生效，
       比两种都接受更难查） */
    if (0 == strcmp(pName, "unknown"))   { return static_cast<int32_t>(Expression_E::EXP_UNKNOWN); }
    if (0 == strcmp(pName, "neutral"))   { return static_cast<int32_t>(Expression_E::EXP_NEUTRAL); }
    if (0 == strcmp(pName, "happy"))     { return static_cast<int32_t>(Expression_E::EXP_HAPPY); }
    if (0 == strcmp(pName, "surprise"))  { return static_cast<int32_t>(Expression_E::EXP_SURPRISE); }
    if (0 == strcmp(pName, "sad"))       { return static_cast<int32_t>(Expression_E::EXP_SAD); }
    if (0 == strcmp(pName, "angry"))     { return static_cast<int32_t>(Expression_E::EXP_ANGRY); }
    if (0 == strcmp(pName, "fear"))      { return static_cast<int32_t>(Expression_E::EXP_FEAR); }
    if (0 == strcmp(pName, "disgust"))   { return static_cast<int32_t>(Expression_E::EXP_DISGUST); }
    return -1;
}

std::string expressionMapText(const CExpressionParam_S &rstParam)
{
    /* 192B：16 格 × 最长词 "surprise"(8) 加一个分隔=144B，留一半余量。
       与 statText 同一纪律：宁可整行报 bad，也不给一张被截断的映射表——
       截尾的表读起来像"这模型只有四类"，而那正是本函数要排除的那个误解 */
    char szBuf[192] = {0};
    if (rstParam.nSlotNum <= 0 || rstParam.nSlotNum > CLASSROOM_EXPRESSION_SLOT_MAX)
    {
        return "bad";
    }
    int32_t nOff = 0;
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        const int nAdd = snprintf(szBuf + nOff, sizeof(szBuf) - static_cast<size_t>(nOff),
                                  "%s%s", (0 == i) ? "" : "|", expressionName(rstParam.anSlotMap[i]));
        if (nAdd < 0 || nAdd >= static_cast<int>(sizeof(szBuf) - static_cast<size_t>(nOff)))
        {
            return "bad";
        }
        nOff += nAdd;
    }
    return std::string(szBuf);
}

const char *expressionSlotMapState(const CExpressionParam_S &rstParam)
{
    if (SlotMapSrc_E::SMS_REJECTED == rstParam.enSlotMapSrc)
    {
        /* 排在内容比对之前：被拒的那张表不生效，按内容走必然读成"出厂序在生效"，
           于是"你以为标好了"与"你压根没写"共用一个词——而这两条的处置方向相反
           （一条去查为什么没生效，一条去做标定） */
        return "rejected";
    }
    if (SlotMapSrc_E::SMS_GUESS != rstParam.enSlotMapSrc &&
        SlotMapSrc_E::SMS_CFG != rstParam.enSlotMapSrc)
    {
        /* 越界不并进 "guess"：与 expressionName 给 "bad" 同一条理由——
           出现 bad 才知道是枚举与实现分开改过或内存被写坏，排查方向完全不同 */
        return "bad";
    }
    if (rstParam.nSlotNum <= 0 || rstParam.nSlotNum > CLASSROOM_EXPRESSION_SLOT_MAX)
    {
        return "bad";
    }
    for (int32_t i = 0; i < rstParam.nSlotNum; ++i)
    {
        if (rstParam.anSlotMap[i] != AEXP_SLOT_MAP_DEFAULT[i])
        {
            return "declared";
        }
    }
    /* 到这里是"生效的表与出厂序逐格相同"。它可能是没下发，也可能是下发的就是这七个词
       （出厂样例正是后者）——分不清，所以这一个词刻意不写成"已标定/未标定"那种断言口气。
       报错的方向也要选：把出厂序报成"有负责任的表"会让人拿标签当结论用，
       报成"没标过"最多让人多做一次标定，两者代价不对称 */
    return "guess";
}

bool decideExpression(const std::vector<float> &rvfLogits, const CExpressionParam_S &rstParam,
                      CExpressionSample_S &rstOut, CExpressionStat_S *pstStat)
{
    if (nullptr != pstStat)
    {
        ++pstStat->u64Tried;
    }

    float afProb[CLASSROOM_EXPRESSION_SLOT_MAX] = { 0.0f };
    return expressionProbs(rvfLogits, rstParam, afProb, rstOut, pstStat);
}

CExpressionEstimator::CExpressionEstimator()
{
    /* 构造即定容 */
    m_vecRow.resize(CLASSROOM_MAX_PERSONS);
    m_vecEma.assign(static_cast<size_t>(CLASSROOM_MAX_PERSONS) *
                    static_cast<size_t>(CLASSROOM_EXPRESSION_SLOT_MAX), 0.0f);
}

void CExpressionEstimator::setParam(const CExpressionParam_S &rstParam)
{
    /* 要比的是「槽位数 + 整张映射表」而不是只比槽位数：EMA 是按槽位下标累加的，
       槽位 2 从「惊讶」换成「悲伤」时长度没变、历史却全变了。
       漏掉这一条的后果是配置改完的一分钟内曲线画着上一套语义的值，
       而且没人会报错——只有情绪曲线跟课堂实况对不上 */
    bool bMapChanged = (rstParam.nSlotNum != m_stParam.nSlotNum);
    if (!bMapChanged)
    {
        for (int32_t i = 0; i < rstParam.nSlotNum && i < CLASSROOM_EXPRESSION_SLOT_MAX; ++i)
        {
            if (rstParam.anSlotMap[i] != m_stParam.anSlotMap[i])
            {
                bMapChanged = true;
                break;
            }
        }
    }

    const int32_t nPrevSlotNum = m_stParam.nSlotNum;
    m_stParam = rstParam;
    if (bMapChanged)
    {
        dlog_warn("classroom expression: 槽位映射已变更 (num %d -> %d)，ema 表已重置",
                  nPrevSlotNum, rstParam.nSlotNum);
        reset();
    }
}

int32_t CExpressionEstimator::rowInUse() const
{
    int32_t nNum = 0;
    for (const auto &stRow : m_vecRow)
    {
        if (stRow.nTrackId >= 0)
        {
            ++nNum;
        }
    }
    return nNum;
}

int32_t CExpressionEstimator::findRow(int32_t nTrackId) const
{
    if (nTrackId < 0)
    {
        return -1;
    }
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].nTrackId == nTrackId)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

int32_t CExpressionEstimator::allocRow(int32_t nTrackId, int64_t llNowMs)
{
    int32_t nSlot = -1;
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].nTrackId < 0)
        {
            nSlot = static_cast<int32_t>(i);
            break;
        }
    }

    if (nSlot < 0)
    {
        /* 表满：让掉「最久没被喂」的一行，而不是拒绝新 ID */
        int32_t nOldest = 0;
        for (size_t i = 1; i < m_vecRow.size(); ++i)
        {
            if (m_vecRow[i].llLastMs < m_vecRow[static_cast<size_t>(nOldest)].llLastMs)
            {
                nOldest = static_cast<int32_t>(i);
            }
        }
        nSlot = nOldest;
        ++m_stStat.u64IdFull;
    }

    m_vecRow[static_cast<size_t>(nSlot)].nTrackId = nTrackId;
    m_vecRow[static_cast<size_t>(nSlot)].llLastMs = llNowMs;
    m_vecRow[static_cast<size_t>(nSlot)].nFrames = 0;
    m_vecRow[static_cast<size_t>(nSlot)].enLastDecided = Expression_E::EXP_UNKNOWN;
    m_vecRow[static_cast<size_t>(nSlot)].fLastDecidedProb = 0.0f;
    m_vecRow[static_cast<size_t>(nSlot)].llLastDecidedMs = 0;
    float *afEma = &m_vecEma[static_cast<size_t>(nSlot) * CLASSROOM_EXPRESSION_SLOT_MAX];
    for (int32_t s = 0; s < CLASSROOM_EXPRESSION_SLOT_MAX; ++s)
    {
        afEma[s] = 0.0f;
    }
    return nSlot;
}

bool CExpressionEstimator::feed(int32_t nTrackId, const std::vector<float> &rvfLogits,
                                int64_t llNowMs, CExpressionSample_S &rstSample)
{
    ++m_stStat.u64Tried;

    float afProb[CLASSROOM_EXPRESSION_SLOT_MAX] = { 0.0f };
    const bool bOk = expressionProbs(rvfLogits, m_stParam, afProb,
                                     rstSample, &m_stStat);

    if (nTrackId < 0)
    {
        return bOk;
    }

    /* 「概率不足 / 差不足」是真证据（模糊本身就是这一帧的证据），要进平滑；
       「长度错 / NaN / 槽位表非法」是无证据，不进平滑——
       把无证据的帧按 0 概率写进 EMA，等于用一段坏数据把这条曲线压向平均值，
       而那正是"曲线看起来很平"的另一个来源 */
    const bool bEvidence = bOk ||
                           (ExprFail_E::EF_LOW_PROB == rstSample.enReason) ||
                           (ExprFail_E::EF_LOW_MARGIN == rstSample.enReason);
    if (!bEvidence)
    {
        return bOk;
    }

    int32_t nRow = findRow(nTrackId);
    if (nRow < 0)
    {
        nRow = allocRow(nTrackId, llNowMs);
    }
    if (nRow < 0)
    {
        return bOk;
    }

    m_vecRow[nRow].llLastMs = llNowMs;
    ++m_vecRow[nRow].nFrames;
    /* 事件口径的瞬时结论：只收"判得出"的帧。低概率/低差值不是"中性"，
       不覆盖上一次真结论；调用方用 event_ttl 让旧结论自己过期 */
    if (rstSample.bDecided)
    {
        m_vecRow[nRow].enLastDecided = rstSample.enLabel;
        m_vecRow[nRow].fLastDecidedProb = rstSample.fProb;
        m_vecRow[nRow].llLastDecidedMs = llNowMs;
    }

    float *afEma = &m_vecEma[static_cast<size_t>(nRow) * CLASSROOM_EXPRESSION_SLOT_MAX];
    const float fAlpha = m_stParam.fEmaAlpha;
    for (int32_t i = 0; i < m_stParam.nSlotNum; ++i)
    {
        afEma[i] = afEma[i] * (1.0f - fAlpha) + afProb[i] * fAlpha;
    }

    /* 覆盖率的记账点是「一次喂入」，不是「一拍读一次」：读侧每拍按在场人数问一遍
       （bCount=false），记进去会让 ok/(ok+cold) 变成节拍频率与在场人数的函数；
       而喂入由取特征段的轮转决定，是这条链上唯一稳定的口径。判定与读侧同一把尺
       （curveOf），所以 cold 的含义就是"这一喂之后还出不了曲线（行不在/帧数不够）"。
       此前两列在现网恒为 0：唯一的读侧调用点传了 bCount=false，而这里不记账 */
    Expression_E enCurve = Expression_E::EXP_UNKNOWN;
    float fCurve = 0.0f;
    (void)curveOf(nTrackId, enCurve, fCurve);
    return bOk;
}

bool CExpressionEstimator::curveOf(int32_t nTrackId, Expression_E &renLabel, float &rfProb,
                                   int64_t *pllLastMs, bool bCount) const
{
    const int32_t nRow = findRow(nTrackId);
    if (nRow < 0)
    {
        if (bCount)
        {
            ++m_stStat.u64CurveCold;
        }
        return false;
    }
    if (m_vecRow[nRow].nFrames < m_stParam.nMinFrames)
    {
        /* 冷启动：一帧就出曲线等于把模型的单帧抖动原样画出来，
           需求里「情绪曲线」的读者会把它当成情绪波动 */
        if (bCount)
        {
            ++m_stStat.u64CurveCold;
        }
        return false;
    }

    const float *afEma = &m_vecEma[static_cast<size_t>(nRow) * CLASSROOM_EXPRESSION_SLOT_MAX];
    int32_t nBest = -1;
    for (int32_t i = 0; i < m_stParam.nSlotNum; ++i)
    {
        if (nBest < 0 || afEma[i] > afEma[nBest])
        {
            nBest = i;
        }
    }
    if (nBest < 0 || !slotLabelOk(m_stParam.anSlotMap[nBest]))
    {
        return false;
    }
    if (afEma[nBest] < m_stParam.fCurveProbMin)
    {
        /* 平滑后仍不突出：这个人一直在几种情绪之间摇摆。此时曲线该断，
           断档在平台上是「没有值」，而取一个中间值画上去是「情绪平平」——
           两者对老师的含义相反 */
        return false;
    }

    renLabel = m_stParam.anSlotMap[nBest];
    rfProb = afEma[nBest];
    if (nullptr != pllLastMs)
    {
        *pllLastMs = m_vecRow[nRow].llLastMs;
    }
    if (bCount)
    {
        ++m_stStat.u64CurveReady;
    }
    return true;
}

bool CExpressionEstimator::latestOf(int32_t nTrackId, Expression_E &renLabel, float &rfProb,
                                    int64_t *pllLastMs) const
{
    const int32_t nRow = findRow(nTrackId);
    if (nRow < 0)
    {
        return false;
    }
    const SRow_S &stRow = m_vecRow[static_cast<size_t>(nRow)];
    if (Expression_E::EXP_UNKNOWN == stRow.enLastDecided || 0 == stRow.llLastDecidedMs)
    {
        return false;
    }
    renLabel = stRow.enLastDecided;
    rfProb = stRow.fLastDecidedProb;
    if (nullptr != pllLastMs)
    {
        *pllLastMs = stRow.llLastDecidedMs;
    }
    return true;
}

int32_t CExpressionEstimator::prune(int64_t llNowMs, int64_t llTtlMs)
{
    int32_t nCleared = 0;
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].nTrackId < 0)
        {
            continue;
        }
        if ((llNowMs - m_vecRow[i].llLastMs) > llTtlMs)
        {
            m_vecRow[i].nTrackId = -1;
            m_vecRow[i].nFrames = 0;
            m_vecRow[i].llLastMs = 0;
            m_vecRow[i].enLastDecided = Expression_E::EXP_UNKNOWN;
            m_vecRow[i].fLastDecidedProb = 0.0f;
            m_vecRow[i].llLastDecidedMs = 0;
            float *afEma = &m_vecEma[i * CLASSROOM_EXPRESSION_SLOT_MAX];
            for (int32_t s = 0; s < CLASSROOM_EXPRESSION_SLOT_MAX; ++s)
            {
                afEma[s] = 0.0f;
            }
            ++nCleared;
        }
    }
    return nCleared;
}

void CExpressionEstimator::reset()
{
    for (auto &stRow : m_vecRow)
    {
        stRow.nTrackId = -1;
        stRow.nFrames = 0;
        stRow.llLastMs = 0;
        stRow.enLastDecided = Expression_E::EXP_UNKNOWN;
        stRow.fLastDecidedProb = 0.0f;
        stRow.llLastDecidedMs = 0;
    }
    for (auto &fV : m_vecEma)
    {
        fV = 0.0f;
    }
}

std::string CExpressionEstimator::statText() const
{
    /* 640B，按最坏情况算而不是按今天的实际值算 */
    char szBuf[640] = {0};
    int nLen = snprintf(szBuf, sizeof(szBuf),
                        "expr try=%llu ok=%llu lowp=%llu lowm=%llu len=%llu nan=%llu sm=%llu map=%llu slotmap=%s |curve ok=%llu cold=%llu rows=%d idfull=%llu",
                        static_cast<unsigned long long>(m_stStat.u64Tried),
                        static_cast<unsigned long long>(m_stStat.u64Decided),
                        static_cast<unsigned long long>(m_stStat.u64LowProb),
                        static_cast<unsigned long long>(m_stStat.u64LowMargin),
                        static_cast<unsigned long long>(m_stStat.u64LengthBad),
                        static_cast<unsigned long long>(m_stStat.u64NotFinite),
                        static_cast<unsigned long long>(m_stStat.u64SoftmaxBad),
                        static_cast<unsigned long long>(m_stStat.u64SlotMapBad),
                        expressionSlotMapState(m_stParam),
                        static_cast<unsigned long long>(m_stStat.u64CurveReady),
                        static_cast<unsigned long long>(m_stStat.u64CurveCold),
                        rowInUse(),
                        static_cast<unsigned long long>(m_stStat.u64IdFull));
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        return "expr stat=truncated";
    }

    /* 槽位分布：这是 slot_map 的标定出口，改一次映射表之前唯一要看的读数。
       含 0 的那些槽位照打——"哪个槽位从来不动"和"哪个槽位在笑的时候抬起来"
       是同一个问题的两半，省略 0 会让前者读不出来 */
    const int32_t nSlot = clampValue(m_stParam.nSlotNum, 0, CLASSROOM_EXPRESSION_SLOT_MAX);
    for (int32_t i = 0; i < nSlot; ++i)
    {
        const int nAdd = snprintf(szBuf + nLen, sizeof(szBuf) - static_cast<size_t>(nLen),
                                   "%s%d=%llu", (0 == i) ? " |slot" : " ", i,
                                   static_cast<unsigned long long>(m_stStat.u64SlotHist[i]));
        if (nAdd < 0 || nAdd >= static_cast<int>(sizeof(szBuf) - static_cast<size_t>(nLen)))
        {
            return "expr stat=truncated";
        }
        nLen += nAdd;
    }
    return std::string(szBuf);
}
}
