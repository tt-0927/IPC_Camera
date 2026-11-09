/**
 * @FilePath     : report_format.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 16:40:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 16:40:00
 * @Description  : 报表素材 → 报文文本
*/

#include "report_format.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace Classroom
{
namespace
{
/* 墙钟早于这个点就认定"没校过时"（2020-01-01） */
constexpr int64_t WALL_SYNCED_MIN_MS = 1577836800000LL;

/* 与 interface 层同名函数 */
void appendFmt(std::string &rstrOut, const char *pFormat, ...)
{
    char astBuf[256];

    va_list stArgs;
    va_start(stArgs, pFormat);
    const int nLen = vsnprintf(astBuf, sizeof(astBuf), pFormat, stArgs);
    va_end(stArgs);

    if (nLen > 0)
    {
        rstrOut.append(astBuf,
                       static_cast<size_t>(std::min(nLen, static_cast<int>(sizeof(astBuf)) - 1)));
    }
}

/**
 * @brief 逗号归"项与项之间"管，不归"谁是最后一项"管
 */
class CSep_S
{
public:
    const char *next()
    {
        const bool bFirst = m_bFirst;
        m_bFirst = false;
        return bFirst ? "" : ",";
    }

private:
    bool m_bFirst = true;
};

/**
 * @brief   : 单调时刻 → 墙钟时刻
 */
int64_t toWallMs(int64_t llMonoMs, int64_t llWallMs, int64_t llAnchorMonoMs)
{
    if (llMonoMs <= 0)
    {
        /* 0 在结构体里的含义是"没有这个时刻"。直接做减法会得到 1970 附近一个合法数字，
           那是把"不知道"伪装成"知道"，比缺字段更坏，所以原样传出 0 让下游判 */
        return 0;
    }
    return llWallMs + (llMonoMs - llAnchorMonoMs);
}

/* 整数读数。<0 出 null 而不是出 -1：与 appendRate 同一条纪律——"不知道"不能伪装成
   一个看起来合法的数。考勤那四个数里 -1 是有含义的（应到没下发、底库没交付），
   原样传下去平台会画成"缺勤 -1 人" */
void appendIntOrNull(std::string &rstrOut, CSep_S &rstSep, const char *pszKey, int32_t nVal)
{
    if (nVal < 0)
    {
        appendFmt(rstrOut, "%s\"%s\":null", rstSep.next(), pszKey);
        return;
    }
    appendFmt(rstrOut, "%s\"%s\":%d", rstSep.next(), pszKey, nVal);
}

/**
 * @brief   : 带转义的字符串字段
 */
void appendJsonString(std::string &rstrOut, CSep_S &rstSep, const char *pszKey,
                      const std::string &strVal)
{
    appendFmt(rstrOut, "%s\"%s\":\"", rstSep.next(), pszKey);
    for (const char c : strVal)
    {
        const unsigned char uc = static_cast<unsigned char>(c);
        switch (c)
        {
        case '"':  rstrOut += "\\\""; break;
        case '\\': rstrOut += "\\\\"; break;
        case '\n': rstrOut += "\\n";  break;
        case '\r': rstrOut += "\\r";  break;
        case '\t': rstrOut += "\\t";  break;
        default:
            /* 其余控制字符（0x00~0x1F）走 \uXXXX：姓名里出现它们只可能是上游脏数据，
               原样写出会让整份报文不可解析，丢掉一个字符比丢掉一份报表好 */
            if (uc < 0x20U)
            {
                appendFmt(rstrOut, "\\u%04x", static_cast<int>(uc));
            }
            else
            {
                rstrOut += c;
            }
            break;
        }
    }
    rstrOut += "\"";
}

/* 比率：结构体里 UNKNOWN 是 -1.0f，报文里没有 -1 这个语义——
   -1 会被平台画成一条负增长曲线，比少一个点更糟。于是换 JSON null，让折线在这里断开，
   同时 metric/timeline 都并排给出 evidence 数：断点处到底是"没证据"还是"真没人"，
   看分母就能分开，不需要下游再猜一次 */
void appendRate(std::string &rstrOut, CSep_S &rstSep, const char *pszKey, float fRate)
{
    if (fRate < 0.0f)
    {
        appendFmt(rstrOut, "%s\"%s\":null", rstSep.next(), pszKey);
        return;
    }
    appendFmt(rstrOut, "%s\"%s\":%.4f", rstSep.next(), pszKey, static_cast<double>(fRate));
}

/* 三态读数：与 appendRate 同一条纪律——UNKNOWN 出 null，不出 false。
   时段那一层尤其要紧：姿态时间片每 20s 有一次空窗，空窗里"没达到人数阈值"是**没测**，
   报成 false 会让平台的课时统计把这几拍当成"全班不听讲"计进去，而这类偏差在报表上不自洽 */
void appendTriState(std::string &rstrOut, CSep_S &rstSep, const char *pszKey, TriState_E enState)
{
    if (TriState_E::TS_TRUE == enState)
    {
        appendFmt(rstrOut, "%s\"%s\":true", rstSep.next(), pszKey);
    }
    else if (TriState_E::TS_FALSE == enState)
    {
        appendFmt(rstrOut, "%s\"%s\":false", rstSep.next(), pszKey);
    }
    else
    {
        appendFmt(rstrOut, "%s\"%s\":null", rstSep.next(), pszKey);
    }
}

/* 逐列出人数。报文里用名字而不用列号：列号有 101~112 / 201~213 两段，
   平台侧照号拼 key 迟早拼错，而名字是 behaviorName() 那一份权威表出来的唯一口径。
   未开启的行为照发 0——与"开了但没人做"同为 0 是 group_stats 里就定下的口径，
   区分两者看配置快照，本报文不重复携带开关状态 */
void appendActiveNum(std::string &rstrOut, const int32_t *pnCol)
{
    CSep_S stSep;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        appendFmt(rstrOut, "%s\"%s\":%d", stSep.next(), behaviorName(enBehavior), pnCol[nCol]);
    }
}

/* 会话账本同样逐列，但换算是"人·秒"：结构体积的是人·毫秒，
   与 statJson 里 stand_person_s 同一个口径，两处都给整数秒，不给会掉精度的浮点 */
void appendActivePersonSec(std::string &rstrOut, const int64_t *pllCol)
{
    CSep_S stSep;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        appendFmt(rstrOut, "%s\"%s\":%lld", stSep.next(), behaviorName(enBehavior),
                  static_cast<long long>(pllCol[nCol] / 1000));
    }
}

/* 一个时间轴点。刻意比 metric 段瘦：只给趋势看得出来的那几项
   （人数 + 分母 + 七个学生比率），教师比率与行为明细只在 metric 段给一次。
   理由是 192 点会把任何"多加一个字段"放大 192 倍，而面板上多一个字段与日志里
   多一处可查的东西，这两件事的代价不对等 */
void appendTimelinePoint(std::string &rstrOut, const char *pszArrSep, const CGroupMetric_S &stM,
                         int64_t llWallMs, int64_t llAnchorMs)
{
    /* 数组侧的分隔符由调用方取好传进来：一个 CSep_S 只管一个 JSON 对象。
       拿同一个实例既管点与点、又管点内的字段，第二个点的 {"t" 就会变成 {,"t" */
    rstrOut += pszArrSep;
    rstrOut += "{";
    CSep_S rstSep;
    appendFmt(rstrOut, "%s\"t\":%lld", rstSep.next(),
              static_cast<long long>(toWallMs(stM.llSampleMs, llWallMs, llAnchorMs)));
    appendFmt(rstrOut, "%s\"person\":%d", rstSep.next(), stM.nPersonNum);
    appendFmt(rstrOut, "%s\"evidence\":%d", rstSep.next(), stM.nStuEvidence);
    appendRate(rstrOut, rstSep, "head_up_rate", stM.fStuHeadUpRate);
    appendRate(rstrOut, rstSep, "head_down_rate", stM.fStuHeadDownRate);
    appendRate(rstrOut, rstSep, "stand_rate", stM.fStuStandRate);
    appendRate(rstrOut, rstSep, "raise_rate", stM.fStuRaiseRate);
    appendRate(rstrOut, rstSep, "turn_back_rate", stM.fStuTurnBackRate);
    appendRate(rstrOut, rstSep, "attention_rate", stM.fStuAttention);
    appendRate(rstrOut, rstSep, "sit_rate", stM.fStuSitRate);
    rstrOut += "}";
}

/* 源帧归一化坐标（0~1）。-1 出 null 而不是出 -1：与比率同一套纪律——
   画轨迹的人拿到 -1 会在画面外左上角落一个点，那看着像"教师从门口进来"，
   而真值是"这一拍没有位置证据" */
void appendNorm(std::string &rstrOut, CSep_S &rstSep, const char *pszKey, float fNorm)
{
    if (fNorm < 0.0f)
    {
        appendFmt(rstrOut, "%s\"%s\":null", rstSep.next(), pszKey);
        return;
    }
    appendFmt(rstrOut, "%s\"%s\":%.3f", rstSep.next(), pszKey, static_cast<double>(fNorm));
}

/* 一个轨迹点）。为什么带 id：归属会换手（跟踪 ID 退役、新人接管），
   一节课里同一个人被换成新 ID 是常态而不是异常*/
void appendTrackPoint(std::string &rstrOut, const char *pszArrSep, const CGroupMetric_S &stM,
                      int64_t llWallMs, int64_t llAnchorMs)
{
    rstrOut += pszArrSep;
    rstrOut += "{";
    CSep_S rstSep;
    appendFmt(rstrOut, "%s\"t\":%lld", rstSep.next(),
              static_cast<long long>(toWallMs(stM.llSampleMs, llWallMs, llAnchorMs)));
    appendFmt(rstrOut, "%s\"tea_id\":%d", rstSep.next(), stM.nTeaPersonId);
    appendNorm(rstrOut, rstSep, "x", stM.fTeaX);
    appendNorm(rstrOut, rstSep, "y", stM.fTeaY);
    /* 区域给文本不给编号：0 在枚举里是"讲台区"这个真值，让平台记住一张数字表
       不如让它直接读到一个能显示的词（口径表在 teacherAreaName，全子系统唯一一份） */
    appendFmt(rstrOut, "%s\"area\":\"%s\"", rstSep.next(), teacherAreaName(stM.nTeaArea));
    rstrOut += "}";
}

/* 按行为取会话"人·秒"。列号取不到时给 0 而不是读越界：与 group_stats 里 activeAt 同一条防御，
   代价都是"少一个读数"，好处是不会因为枚举与表被分开改过一次而把整份报表读飞 */
int64_t sessionSecAt(const int64_t *pllCol, Behavior_E enBehavior)
{
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0 || nCol >= CDurationGate::BEHAVIOR_COLUMN_NUM)
    {
        return 0;
    }
    return pllCol[nCol] / 1000;
}
}    /* namespace */

/* 即时事件推送（，SPOOL_TYPE_EVENT）：事件发生那一刻就发一条单事件报文，
   不等最长 60s 的周期报表。周期报表仍是权威全集（断网滞留/重传语义不变），
   两边都带 ev_seq，平台先到为准去重。kind=event 与报表的 periodic/session_close 分流 */
std::string eventPushJson(const CBehaviorEvent_S &stEvent, int64_t llWallMs, int64_t llMonoMs)
{
    std::string strOut;
    strOut.reserve(256);
    strOut += "{";
    appendFmt(strOut, "\"kind\":\"event\"");
    appendFmt(strOut, ",\"ts\":%lld", static_cast<long long>(llWallMs));
    appendFmt(strOut, ",\"clock_sync\":%d", (llWallMs >= WALL_SYNCED_MIN_MS) ? 1 : 0);
    appendFmt(strOut, ",\"t\":%lld",
              static_cast<long long>(toWallMs(stEvent.llMs, llWallMs, llMonoMs)));
    appendFmt(strOut, ",\"action\":\"%s\"", stEvent.bBegin ? "begin" : "end");
    appendFmt(strOut, ",\"behavior\":\"%s\"", behaviorName(stEvent.enBehavior));
    appendFmt(strOut, ",\"person\":%s,\"seat\":%s",
              (stEvent.nPersonId < 0) ? "null" : std::to_string(stEvent.nPersonId).c_str(),
              (stEvent.nSeatId < 0) ? "null" : std::to_string(stEvent.nSeatId).c_str());
    appendFmt(strOut, ",\"held_s\":%lld", static_cast<long long>(stEvent.llHeldMs / 1000));
    appendFmt(strOut, ",\"frame_seq\":%u", stEvent.u32FeatureSeq);
    appendFmt(strOut, ",\"ev_seq\":%llu", static_cast<unsigned long long>(stEvent.u64EventSeq));
    appendFmt(strOut, ",\"expr_class\":%d", static_cast<int>(stEvent.nExprClass));
    strOut += "}";
    return strOut;
}

/* 讲台状态段 */
std::string podiumJson(const CClassPodiumState_S &stPodium, int64_t llAgeMs)
{
    const char *szState = "unknown";
    const char *szReason = "no_run";
    if (stPodium.bValid)
    {
        if (TriState_E::TS_TRUE == stPodium.enState)
        {
            szState = "empty";
            szReason = "ok";
        }
        else if (TriState_E::TS_FALSE == stPodium.enState)
        {
            szState = "occupied";
            szReason = "ok";
        }
        else if (SampleReason_E::SR_REGION_MISS == stPodium.enReason)
        {
            /* 讲台没画：本机确实没有这个答案，不是"讲台里没人" */
            szReason = "unconfigured";
        }
        else if (SampleReason_E::SR_SUBJECT == stPodium.enReason)
        {
            /* 归属总开关关着：仲裁器不数讲台，保留的是上一拍的旧值，不能当本拍事实 */
            szReason = "off";
        }
        else
        {
            szReason = sampleReasonName(stPodium.enReason);
        }
    }

    std::string strOut;
    strOut.reserve(64);
    appendFmt(strOut, "{\"state\":\"%s\",\"reason\":\"%s\",\"age_ms\":%lld}",
              szState, szReason, static_cast<long long>(llAgeMs));
    return strOut;
}

namespace
{
/* 面板的一个行为分组 */
void appendPanelBehaviorGroup(std::string &rstrOut, const char *pszGroupKey,
                              const CPanelReport_S &stPanel, int nFirstBehavior, int nLastBehavior)
{
    appendFmt(rstrOut, ",\"%s\":{", pszGroupKey);
    CSep_S stSep;
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        if (!CDurationGate::behaviorAtColumn(nCol, enBehavior))
        {
            continue;
        }
        const int nValue = static_cast<int>(enBehavior);
        if (nValue < nFirstBehavior || nValue > nLastBehavior)
        {
            continue;
        }
        appendFmt(rstrOut, "%s\"%s\":%llu", stSep.next(), behaviorName(enBehavior),
                  static_cast<unsigned long long>(stPanel.au64BehaviorBegin[nCol]));
    }
    rstrOut += "}";
}
}

std::string panelReportJson(const CPanelReport_S &stPanel)
{
    /* 情绪键名是**报文词汇**，与 expressionName() 同一组词、同一顺序（Expression_E 0..6）。
       这里没有 include 感知层那张表：egress 不依赖 perception（分层闸门会红），
       而且那张表还带着模型侧的名字。两处是否漂移由用例钉住（键名逐条比） */
    static const char *const aszExprKey[] = {
        "neutral", "happy", "surprise", "sad", "angry", "fear", "disgust",
    };
    static_assert(sizeof(aszExprKey) / sizeof(aszExprKey[0]) == CLASSROOM_EXPRESSION_CLASS_NUM,
                  "情绪键表与 Expression_E 的类别数不一致");
    static_assert(static_cast<int>(Behavior_E::STU_STAND_UP) == 101 &&
                  static_cast<int>(Behavior_E::STU_PHONE) == 106 &&
                  static_cast<int>(Behavior_E::STU_PRESENT) == 111 &&
                  static_cast<int>(Behavior_E::TEA_STAND_UP) == 201,
                  "行为号段变了：面板的三组边界要跟着改");

    std::string strOut;
    strOut.reserve(1024);

    strOut += "{\"attendance\":{";
    CSep_S stAttSep;
    /* -1 出 null：应到没下发、实到没在考勤，网页显示"--"；出 0 会被读成"缺勤全班" */
    appendIntOrNull(strOut, stAttSep, "expected", stPanel.nExpectedNum);
    appendIntOrNull(strOut, stAttSep, "present", stPanel.nPresentNum);
    strOut += "}";

    /* 三组行为都是**本会话累计的 BEGIN 条数**：进行中的那一段算 1 次（还没结束） */
    appendPanelBehaviorGroup(strOut, "teacher", stPanel,
                             static_cast<int>(Behavior_E::TEA_STAND_UP), 299);
    appendPanelBehaviorGroup(strOut, "student_basic", stPanel,
                             static_cast<int>(Behavior_E::STU_STAND_UP),
                             static_cast<int>(Behavior_E::STU_PHONE));
    appendPanelBehaviorGroup(strOut, "student_learning", stPanel,
                             static_cast<int>(Behavior_E::STU_LISTEN),
                             static_cast<int>(Behavior_E::STU_PRESENT));

    strOut += ",\"emotion\":{";
    CSep_S stExprSep;
    for (int nExpr = 0; nExpr < CLASSROOM_EXPRESSION_CLASS_NUM; ++nExpr)
    {
        appendFmt(strOut, "%s\"%s\":%llu", stExprSep.next(), aszExprKey[nExpr],
                  static_cast<unsigned long long>(stPanel.au64ExprNum[nExpr]));
    }
    strOut += "}}";
    return strOut;
}

std::string reportBundleJson(const CReportBundle_S &stBundle, int64_t llWallMs, int64_t llMonoMs)
{
    std::string strOut;
    /* 预留一个常见量级而不是按上限预留：报表节奏是分钟级，多长几次不心疼，
       反过来按 64KB 预留会让"没人在教室"的那份空报表也占一次峰值分配 */
    strOut.reserve(8192);

    const CGroupMetric_S &stM = stBundle.stLatest;
    const CSessionAccum_S &stS = stBundle.stSession;
    CSep_S stTop;

    strOut += "{";
    /* 本报文覆盖的时间跨度由时间轴首末点算，而不是用上报周期：
       周期是"本该多久报一次"，跨度是"这份实际报了多少"——断网重启、总开关中途开合，
       两者会明显不等，而平台拿周期去补时间轴就会凭空造出一段没证据的平坦线 */
    int64_t llSpanMs = 0;
    const size_t uTimelineNum = stBundle.vecTimeline.size();
    if (uTimelineNum >= 2)
    {
        llSpanMs = stBundle.vecTimeline[uTimelineNum - 1].llSampleMs
                   - stBundle.vecTimeline[0].llSampleMs;
    }

    appendFmt(strOut, "%s\"kind\":\"%s\"", stTop.next(),
              stBundle.bSessionReset ? "session_close" : "periodic");
    appendFmt(strOut, "%s\"ts\":%lld", stTop.next(), static_cast<long long>(llWallMs));
    appendFmt(strOut, "%s\"clock_sync\":%d", stTop.next(),
              (llWallMs >= WALL_SYNCED_MIN_MS) ? 1 : 0);
    appendFmt(strOut, "%s\"span_s\":%lld", stTop.next(),
              static_cast<long long>((llSpanMs > 0) ? (llSpanMs / 1000) : 0));

    /* ---- 最新一条样本：平台面板上的"当前值" ---- */
    appendFmt(strOut, "%s\"metric\":{", stTop.next());
    CSep_S stMetric;
    appendFmt(strOut, "%s\"t\":%lld", stMetric.next(),
              static_cast<long long>(toWallMs(stM.llSampleMs, llWallMs, llMonoMs)));
    /* 帧时刻单独给：llFrameMs 为 0 表示"这一拍没有新帧、沿用了上一帧的证据"。
       只给样本时刻的话，降级掉帧期间这份报表看起来跟正常报表一模一样 */
    appendFmt(strOut, "%s\"frame_t\":%lld", stMetric.next(),
              static_cast<long long>(toWallMs(stM.llFrameMs, llWallMs, llMonoMs)));
    appendFmt(strOut, "%s\"frame_seq\":%u", stMetric.next(), stM.u32FeatureSeq);
    appendFmt(strOut, "%s\"person\":%d", stMetric.next(), stM.nPersonNum);
    appendFmt(strOut, "%s\"student\":%d", stMetric.next(), stM.nStudentNum);
    appendFmt(strOut, "%s\"teacher\":%d", stMetric.next(), stM.nTeacherNum);
    appendFmt(strOut, "%s\"evidence_stu\":%d", stMetric.next(), stM.nStuEvidence);
    appendFmt(strOut, "%s\"evidence_tea\":%d", stMetric.next(), stM.nTeaEvidence);
    appendFmt(strOut, "%s\"no_evidence\":%d", stMetric.next(), stM.nStuNoEvidence);
    appendFmt(strOut, "%s\"truncated\":%d", stMetric.next(), stM.bTruncated ? 1 : 0);
    appendRate(strOut, stMetric, "head_up_rate", stM.fStuHeadUpRate);
    appendRate(strOut, stMetric, "head_down_rate", stM.fStuHeadDownRate);
    appendRate(strOut, stMetric, "stand_rate", stM.fStuStandRate);
    appendRate(strOut, stMetric, "raise_rate", stM.fStuRaiseRate);
    appendRate(strOut, stMetric, "turn_back_rate", stM.fStuTurnBackRate);
    appendRate(strOut, stMetric, "attention_rate", stM.fStuAttention);
    appendRate(strOut, stMetric, "sit_rate", stM.fStuSitRate);
    appendRate(strOut, stMetric, "tea_head_up_rate", stM.fTeaHeadUpRate);
    appendRate(strOut, stMetric, "tea_stand_rate", stM.fTeaStandRate);
    /* 时段读数。只加在 metric 段、不进时间轴点：
       一个字段 192 份的代价见 appendTimelinePoint 的注释，而"这节课有没有讨论时段"
       在面板上是一个点，不是一条曲线 */
    appendTriState(strOut, stMetric, "listen_period", stM.enListenPeriod);
    appendTriState(strOut, stMetric, "reading_period", stM.enReadingPeriod);
    appendTriState(strOut, stMetric, "discuss_period", stM.enDiscussPeriod);
    appendFmt(strOut, "%s\"tea_id\":%d", stMetric.next(), stM.nTeaPersonId);
    appendFmt(strOut, "%s\"tea_area\":\"%s\"", stMetric.next(), teacherAreaName(stM.nTeaArea));
    appendNorm(strOut, stMetric, "tea_x", stM.fTeaX);
    appendNorm(strOut, stMetric, "tea_y", stM.fTeaY);
    appendFmt(strOut, "%s\"active\":{", stMetric.next());
    appendActiveNum(strOut, stM.nActive);
    strOut += "}}";

    /* ---- 时间轴 ----
       超限按步长抽稀、且**保留末点**：整段丢掉后半截会让"这节课最后十分钟全班趴下"
       这种最需要看的形状消失，而均匀抽稀只是降低分辨率。
       每点都自带 t，所以抽稀后平台仍能算出真实间距，不需要额外解释 */
    size_t uStep = 1;
    if (uTimelineNum > static_cast<size_t>(CLASSROOM_REPORT_TIMELINE_MAX))
    {
        uStep = (uTimelineNum + CLASSROOM_REPORT_TIMELINE_MAX - 1) / CLASSROOM_REPORT_TIMELINE_MAX;
    }
    appendFmt(strOut, "%s\"timeline\":{\"stride\":%zu,\"point\":[", stTop.next(), uStep);
    CSep_S stPoint;
    size_t uLast = 0;
    for (size_t i = 0; i < uTimelineNum; i += uStep)
    {
        appendTimelinePoint(strOut, stPoint.next(), stBundle.vecTimeline[i], llWallMs, llMonoMs);
        uLast = i;
    }
    if (uTimelineNum > 0 && uLast != uTimelineNum - 1)
    {
        appendTimelinePoint(strOut, stPoint.next(), stBundle.vecTimeline[uTimelineNum - 1],
                            llWallMs, llMonoMs);
    }
    strOut += "]}";

    /* ---- 轨迹打点---- */
    size_t uTrackStep = 1;
    if (uTimelineNum > static_cast<size_t>(CLASSROOM_REPORT_TRACK_MAX))
    {
        uTrackStep = (uTimelineNum + CLASSROOM_REPORT_TRACK_MAX - 1) / CLASSROOM_REPORT_TRACK_MAX;
    }
    appendFmt(strOut, "%s\"track\":{\"stride\":%zu,\"point\":[", stTop.next(), uTrackStep);
    CSep_S stTrack;
    size_t uTrackLast = 0;
    for (size_t i = 0; i < uTimelineNum; i += uTrackStep)
    {
        uTrackLast = i;
        if (stBundle.vecTimeline[i].nTeaPersonId >= 0)
        {
            appendTrackPoint(strOut, stTrack.next(), stBundle.vecTimeline[i], llWallMs, llMonoMs);
        }
    }
    if (uTimelineNum > 0 && uTrackLast != uTimelineNum - 1 &&
        stBundle.vecTimeline[uTimelineNum - 1].nTeaPersonId >= 0)
    {
        appendTrackPoint(strOut, stTrack.next(), stBundle.vecTimeline[uTimelineNum - 1],
                         llWallMs, llMonoMs);
    }
    strOut += "]}";

    /* ---- 学生机位轨迹 ---- */
    const CStuTrackSnap_S &stStu = stBundle.stStuTrack;
    appendFmt(strOut, "%s\"stu_track\":{\"gate\":\"%s\",\"evict\":%d,\"thin\":%d,\"nopos\":%d,\"person\":[",
              stTop.next(), stuTrackGateName(stStu.nGate), stStu.nEvictNum, stStu.nThinNum,
              stStu.nNoPosNum);
    CSep_S stStuRow;
    /* 上限取数组的真实长度而不是 nPersonNum：这个结构体是跨层拷来的，
       越界读不会在设备上留下任何痕迹，只会把内存里的噪声画成一条轨迹（与 seat 的 cell 同一条防御） */
    const int32_t nStuNum = std::min(stStu.nPersonNum,
                                     static_cast<int32_t>(CLASSROOM_STU_TRACK_PERSON_MAX));
    for (int32_t i = 0; i < nStuNum; ++i)
    {
        const CStuTrackPerson_S &stPerson = stStu.astPerson[static_cast<size_t>(i)];
        appendFmt(strOut, "%s{\"id\":%d,\"why\":[", stStuRow.next(), stPerson.nPersonId);
        CSep_S stWhy;
        for (uint32_t u32Bit = 0; u32Bit < 2; ++u32Bit)
        {
            const uint32_t u32Mask = (1U << u32Bit);
            if (stPerson.u32Why & u32Mask)
            {
                appendFmt(strOut, "%s\"%s\"", stWhy.next(), stuTrackWhyName(u32Mask));
            }
        }
        strOut += "],\"pt\":[";
        CSep_S stPt;
        const int32_t nPtNum = std::min(stPerson.nPtNum,
                                        static_cast<int32_t>(CLASSROOM_STU_TRACK_POINT_MAX));
        for (int32_t j = 0; j < nPtNum; ++j)
        {
            const CStuTrackPoint_S &stPtRef = stPerson.astPt[static_cast<size_t>(j)];
            /* 每个点都带绝对时刻：抽稀过的线上，间距自己会说话（平台算得出稀到哪一档），
               不需要另加一个"抽稀倍数"字段——那是一份会与真实时刻分叉的第二套账 */
            appendFmt(strOut, "%s{\"t\":%lld,\"x\":%.3f,\"y\":%.3f}", stPt.next(),
                      static_cast<long long>(toWallMs(stPtRef.llMonoMs, llWallMs, llMonoMs)),
                      static_cast<double>(stPtRef.fNormX), static_cast<double>(stPtRef.fNormY));
        }
        strOut += "]}";
    }
    strOut += "]}";

    /* ---- 事件明细 ----
       不带 stHeadBox：那是**分析帧坐标**，直接发出去等于让平台画到一个它不知道存在过的
       画幅上。镜像/旋转/缩放的逆运算归 CFrameGeometry，谁持有几何谁负责换算，
       本文件没有几何信息，也就没有把坐标发对的资格 */
    appendFmt(strOut, "%s\"events\":[", stTop.next());
    CSep_S stEvt;
    for (size_t i = 0; i < stBundle.vecEvents.size(); ++i)
    {
        const CBehaviorEvent_S &stE = stBundle.vecEvents[i];
        /* ev_seq（#95）：事件唯一序号。即时事件通道会先发一份同事件的推送报文，
           平台按它去重（先到为准）；0 = 该事件没进过聚合环，没有可去重的凭据 */
        appendFmt(strOut,
                  "%s{\"t\":%lld,\"action\":\"%s\",\"behavior\":\"%s\","
                  "\"person\":%s,\"seat\":%s,\"held_s\":%lld,\"frame_seq\":%u,\"ev_seq\":%llu,"
                  "\"expr_class\":%d}",
                  stEvt.next(),
                  static_cast<long long>(toWallMs(stE.llMs, llWallMs, llMonoMs)),
                  stE.bBegin ? "begin" : "end", behaviorName(stE.enBehavior),
                  (stE.nPersonId < 0) ? "null" : std::to_string(stE.nPersonId).c_str(),
                  (stE.nSeatId < 0) ? "null" : std::to_string(stE.nSeatId).c_str(),
                  static_cast<long long>(stE.llHeldMs / 1000), stE.u32FeatureSeq,
                  static_cast<unsigned long long>(stE.u64EventSeq),
                  static_cast<int>(stE.nExprClass));
    }
    strOut += "]";

    /* ---- 会话账本：一次课的收尾报与周期报共用同一段，差别只在 kind ---- */
    appendFmt(strOut, "%s\"session\":{", stTop.next());
    CSep_S stSess;
    appendFmt(strOut, "%s\"begin_t\":%lld", stSess.next(),
              static_cast<long long>(toWallMs(stS.llBeginMs, llWallMs, llMonoMs)));
    appendFmt(strOut, "%s\"present_s\":%lld", stSess.next(),
              static_cast<long long>(stS.llPresentMs / 1000));
    appendFmt(strOut, "%s\"person_max\":%d", stSess.next(), stS.nPersonMax);
    appendFmt(strOut, "%s\"samples\":%d", stSess.next(), stS.nSampleNum);
    appendFmt(strOut, "%s\"truncated\":%d", stSess.next(), stS.bTruncatedSeen ? 1 : 0);
    appendFmt(strOut, "%s\"event_begin\":%llu", stSess.next(),
              static_cast<unsigned long long>(stS.u64BeginEvt));
    appendFmt(strOut, "%s\"event_end\":%llu", stSess.next(),
              static_cast<unsigned long long>(stS.u64EndEvt));
    appendFmt(strOut, "%s\"person_s\":{", stSess.next());
    appendActivePersonSec(strOut, stS.llActiveMs);
    strOut += "}";
    const int nPatrolCol = CDurationGate::behaviorColumn(Behavior_E::TEA_PATROL);
    appendFmt(strOut, "%s\"teacher\":{", stSess.next());
    CSep_S stTeaSess;
    appendFmt(strOut, "%s\"podium_s\":%lld", stTeaSess.next(),
              static_cast<long long>(sessionSecAt(stS.llActiveMs, Behavior_E::TEA_ENTER_PODIUM)));
    appendFmt(strOut, "%s\"patrol_s\":%lld", stTeaSess.next(),
              static_cast<long long>(sessionSecAt(stS.llActiveMs, Behavior_E::TEA_PATROL)));
    appendFmt(strOut, "%s\"patrol_times\":%llu", stTeaSess.next(),
              static_cast<unsigned long long>(
                  (nPatrolCol >= 0 && nPatrolCol < CDurationGate::BEHAVIOR_COLUMN_NUM)
                      ? stS.u64BeginAt[nPatrolCol] : 0));
    appendFmt(strOut, "%s\"track_s\":%lld}", stTeaSess.next(),
              static_cast<long long>(sessionSecAt(stS.llActiveMs, Behavior_E::TEA_TRACK)));
    strOut += "}";

    /* 两个"没报出去"分开给，口径见 data_center.h 里 CReportBundle_S 的注释：
       deferred 是本轮没带走、下次会带走；lost 是被环覆盖且从未取走，永远拿不到。
       合成一个数就是把延迟谎报成丢失 */
    appendFmt(strOut, "%s\"coverage\":{\"event_deferred\":%d,\"event_lost\":%d}", stTop.next(),
              stBundle.nEventDropped, stBundle.nEventLost);
    /* ---- 考勤（CSV 25~30：到课人数 / 应到差异 / 比对 / 身份展示）---- */
    const CAttendSnap_S &stA = stBundle.stAttend;
    appendFmt(strOut, "%s\"attendance\":{", stTop.next());
    CSep_S stAtt;
    appendFmt(strOut, "%s\"enable\":%s", stAtt.next(), stA.bEnable ? "true" : "false");
    if (stA.bEnable)
    {
        appendIntOrNull(strOut, stAtt, "present", stA.nPresentNum);
        appendIntOrNull(strOut, stAtt, "named", stA.nNamedNum);
        appendIntOrNull(strOut, stAtt, "expected", stA.nExpectedNum);
        /* absent 是**有符号**差异：>0 缺勤，<0 超员（旁听、走错教室、以及人数被 ID 抖动撑大）。
           未知与否看 bAbsentKnown 而不是看符号：-1 在这里是一个真值（超员一人），
           拿"负数即未知"那一档处理它，等于把超员这条线索从报文里删掉 */
        if (stA.bAbsentKnown)
        {
            appendFmt(strOut, "%s\"absent\":%d", stAtt.next(), stA.nAbsentNum);
        }
        else
        {
            appendFmt(strOut, "%s\"absent\":null", stAtt.next());
        }
        appendFmt(strOut, "%s\"closed\":%d", stAtt.next(), stA.bClosed ? 1 : 0);
        appendFmt(strOut, "%s\"roster\":[", stAtt.next());

        bool bFirstRec = true;
        size_t uOmitted = 0;
        size_t uEmitted = 0;
        for (const CAttendRec_S &stRec : stA.vecRoster)
        {
            if (static_cast<int32_t>(uEmitted) >= CLASSROOM_REPORT_ROSTER_MAX)
            {
                ++uOmitted;
                continue;
            }
            ++uEmitted;
            strOut += (bFirstRec ? "" : ",");
            bFirstRec = false;
            strOut += "{";
            CSep_S stRsep;    /* 一个 JSON 对象一个分隔符实例：跨对象复用会让首项带逗号 */
            appendJsonString(strOut, stRsep, "person_id", stRec.strPersonId);
            appendJsonString(strOut, stRsep, "name", stRec.strName);
            appendFmt(strOut, "%s\"first_t\":%lld", stRsep.next(),
                      static_cast<long long>(toWallMs(stRec.llFirstMs, llWallMs, llMonoMs)));
            appendFmt(strOut, "%s\"last_t\":%lld", stRsep.next(),
                      static_cast<long long>(toWallMs(stRec.llLastMs, llWallMs, llMonoMs)));
            appendFmt(strOut, "%s\"seen_s\":%lld", stRsep.next(),
                      static_cast<long long>(stRec.llSeenMs / 1000));
            appendFmt(strOut, "%s\"score\":%.3f", stRsep.next(), static_cast<double>(stRec.fBestScore));
            appendIntOrNull(strOut, stRsep, "seat", stRec.nSeatId);
            strOut += "}";
        }
        strOut += "]";
        /* 被省下的条数是**对象**的键，必须落在数组外面：写在 ] 之前会让 roster 数组里
           混进一个键值对，整份报文在平台侧直接解析失败 */
        appendFmt(strOut, "%s\"roster_omitted\":%zu", stAtt.next(), uOmitted);
    }
    else
    {
        /* 关掉考勤：四个数给 null。报 0 会被平台当成"这节课 0 人到课"入库，
           而那是一句从设备上发出去的假话 */
        appendFmt(strOut, "%s\"present\":null", stAtt.next());
        appendFmt(strOut, "%s\"named\":null", stAtt.next());
        appendFmt(strOut, "%s\"expected\":null", stAtt.next());
        appendFmt(strOut, "%s\"absent\":null", stAtt.next());
        appendFmt(strOut, "%s\"roster\":[]", stAtt.next());
    }
    strOut += "}";

    /* ---- 座位热力 ---- */
    const CSeatSnap_S &stSeat = stBundle.stSeat;
    appendFmt(strOut, "%s\"seat\":{", stTop.next());
    CSep_S stSeatSep;
    appendFmt(strOut, "%s\"enable\":%s", stSeatSep.next(), stSeat.bEnable ? "true" : "false");
    if (stSeat.bEnable && stSeat.bAreaKnown)
    {
        appendFmt(strOut, "%s\"cols\":%d", stSeatSep.next(), stSeat.nCols);
        appendFmt(strOut, "%s\"rows\":%d", stSeatSep.next(), stSeat.nRows);
        appendFmt(strOut, "%s\"box\":[%.3f,%.3f,%.3f,%.3f]", stSeatSep.next(),
                  static_cast<double>(stSeat.fX0), static_cast<double>(stSeat.fY0),
                  static_cast<double>(stSeat.fX1), static_cast<double>(stSeat.fY1));
        appendFmt(strOut, "%s\"cell\":[", stSeatSep.next());
        /* 上限取快照数组的真实长度而不是 cols*rows：这个结构体是跨层拷来的，
           越界读不会在设备上留下任何痕迹，只会让最后几格的值来自数组之后的内存 */
        const int32_t nCellNum = std::min(stSeat.nCols * stSeat.nRows,
                                          static_cast<int32_t>(CLASSROOM_SEAT_MAX_CELLS));
        CSep_S stCell;
        for (int32_t i = 0; i < nCellNum; ++i)
        {
            const int32_t nSec = stSeat.anSec[static_cast<size_t>(i)];
            if (nSec < 0)
            {
                strOut += stCell.next();
                strOut += "null";
                continue;
            }
            appendFmt(strOut, "%s%d", stCell.next(), nSec);
        }
        strOut += "]";
        /* 这两个诊断数让"图是空的"可判定：per=0 是这一周期没人被看着，
           per=40 而 nopos=40 是位置换算链断了。只给图的话现场只能从头猜 */
        appendFmt(strOut, "%s\"per\":%d", stSeatSep.next(), stSeat.nPersonNum);
        appendFmt(strOut, "%s\"nopos\":%d", stSeatSep.next(), stSeat.nNoPosNum);
    }
    else
    {
        appendFmt(strOut, "%s\"cell\":null", stSeatSep.next());
    }
    strOut += "}";

    /* ---- 板书记录---- */
    const bool bBoardOn = stBundle.bBoardEnable;
    appendFmt(strOut, "%s\"board\":{", stTop.next());
    CSep_S stBoardSep;
    appendFmt(strOut, "%s\"enable\":%s", stBoardSep.next(), bBoardOn ? "true" : "false");
    if (bBoardOn)
    {
        appendFmt(strOut, "%s\"st\":%d", stBoardSep.next(), stBundle.nBoardState);
        appendFmt(strOut, "%s\"item\":[", stBoardSep.next());
        CSep_S stItemSep;
        for (const CBoardItem_S &stItem : stBundle.vecBoard)
        {
            strOut += stItemSep.next();
            strOut += "{";
            CSep_S stField;
            appendFmt(strOut, "%s\"seq\":%u", stField.next(), stItem.u32Seq);
            appendFmt(strOut, "%s\"t\":%lld", stField.next(),
                      static_cast<long long>(toWallMs(stItem.llTsMs, llWallMs, llMonoMs)));
            /* 这一段的五个值全是整数：没有字符串就没有转义，
               一段不可信的外部文本不再有机会把整份报表变成非法 JSON */
            appendFmt(strOut, "%s\"conf\":%.3f", stField.next(), static_cast<double>(stItem.fConfidence));
            appendFmt(strOut, "%s\"lines\":%d", stField.next(), stItem.nLineNum);
            appendFmt(strOut, "%s\"bytes\":%d", stField.next(), stItem.nTextBytes);
            strOut += "}";
        }
        strOut += "]";
    }
    else
    {
        appendFmt(strOut, "%s\"item\":null", stBoardSep.next());
    }
    strOut += "}";

    /* ---- 课堂场景时段----
       正在延长的那一段每次都带、且**没有 end_t**：写一个假的结束时刻会让平台把"现在"
       当成"已经过去了"来累计占比。闭合段是队列语义（取走就不再出现），所以这里给全量而不加游标。
       整段关掉时 segment 给 null——空数组是"这节课一次都没切过段"，那是真读数不是关闭状态 */
    appendFmt(strOut, "%s\"scene\":{", stTop.next());
    CSep_S stSceneSep;
    appendFmt(strOut, "%s\"enable\":%s", stSceneSep.next(), stBundle.bSceneEnable ? "true" : "false");
    if (stBundle.bSceneEnable)
    {
        appendFmt(strOut, "%s\"cur\":{", stSceneSep.next());
        CSep_S stCurSep;
        appendFmt(strOut, "%s\"type\":\"%s\"", stCurSep.next(),
                  CSceneInfer::sceneName(stBundle.stOpenScene.enScene));
        appendFmt(strOut, "%s\"begin_t\":%lld", stCurSep.next(),
                  static_cast<long long>(toWallMs(stBundle.stOpenScene.llBeginMs, llWallMs, llMonoMs)));
        appendFmt(strOut, "%s\"board\":%d", stCurSep.next(), stBundle.stOpenScene.nBoardNum);
        appendFmt(strOut, "%s\"person\":%d", stCurSep.next(), stBundle.stOpenScene.nPersonNum);
        strOut += "}";
        appendFmt(strOut, "%s\"segment\":[", stSceneSep.next());
        CSep_S stSegSep;
        for (const CSceneSegment_S &stSeg : stBundle.vecScenes)
        {
            strOut += stSegSep.next();
            strOut += "{";
            CSep_S stField;
            appendFmt(strOut, "%s\"type\":\"%s\"", stField.next(),
                      CSceneInfer::sceneName(stSeg.enScene));
            appendFmt(strOut, "%s\"begin_t\":%lld", stField.next(),
                      static_cast<long long>(toWallMs(stSeg.llBeginMs, llWallMs, llMonoMs)));
            appendFmt(strOut, "%s\"end_t\":%lld", stField.next(),
                      static_cast<long long>(toWallMs(stSeg.llEndMs, llWallMs, llMonoMs)));
            /* 时长自己算一份给出（秒）：平台拿两个毫秒时间戳相减也会得到同一个数，
               但设备上算的那一份才是判据认的（含最短段合并后的边界），两边对不上时这列能指认是谁 */
            appendFmt(strOut, "%s\"dur_s\":%lld", stField.next(),
                      static_cast<long long>((stSeg.llEndMs - stSeg.llBeginMs) / 1000));
            appendFmt(strOut, "%s\"board\":%d", stField.next(), stSeg.nBoardNum);
            appendFmt(strOut, "%s\"person\":%d", stField.next(), stSeg.nPersonNum);
            strOut += "}";
        }
        strOut += "]";
    }
    else
    {
        appendFmt(strOut, "%s\"segment\":null", stSceneSep.next());
    }
    strOut += "}";

    appendFmt(strOut, "%s\"reset\":%d}", stTop.next(), stBundle.bSessionReset ? 1 : 0);

    return strOut;
}

int64_t reportWallMs()
{
    struct timespec stTs;

    /* 刻意不回退到 time()：clock_gettime 在 CLOCK_REALTIME 上失败只会发生在时钟源本身
       不健康的时候，此时返回 0 会让 clock_sync 打成 0（报文自己承认没校过时），
       比拿一个来历不明的时刻塞进时间戳好 */
    if (0 != clock_gettime(CLOCK_REALTIME, &stTs))
    {
        return 0;
    }
    return static_cast<int64_t>(stTs.tv_sec) * 1000 + stTs.tv_nsec / 1000000;
}
}
