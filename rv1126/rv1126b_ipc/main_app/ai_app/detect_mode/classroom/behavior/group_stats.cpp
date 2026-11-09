/**
 * @FilePath     : group_stats.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 09:24:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 09:24:00
 * @Description  : 群体指标合成实现
 */

#include "group_stats.h"

#include <cstdio>

namespace Classroom
{
namespace
{
constexpr int32_t STU_COL_NUM = CDurationGate::STUDENT_COLUMN_NUM;

/* 比率：分母为 0 给 -1 而不是 0。这一行是整个指标流最容易被"顺手简化"掉的地方，
   简化成 0.0f 之后，"没人抬头"与"这一拍没证据"在平台上是同一个数 */
inline float rateOf(int32_t nNum, int32_t nDenom)
{
    if (nDenom <= 0)
    {
        return -1.0f;
    }
    return static_cast<float>(nNum) / static_cast<float>(nDenom);
}

/* 掩码位宽与列数的关系 */
static_assert(CDurationGate::BEHAVIOR_COLUMN_NUM <= 32,
              "行为列数超过 32：时段门/群体统计的位掩码装不下，"
              "要么把掩码换成 uint64，要么给新列一个独立的证据通道");

inline bool maskHas(uint32_t u32Mask, int nBit)
{
    return nBit >= 0 && nBit < 32 && (0 != (u32Mask & (1u << nBit)));
}

/* 取某行为的生效人数。列号越界（枚举里加了行为但没扩表）时返回 0 而不是读越界：
   表长由 BEHAVIOR_COLUMN_NUM 从枚举算出，正常不会越界，越界只可能是枚举与表被分开改过 */
inline int32_t activeAt(const int32_t *pnActive, Behavior_E enBehavior)
{
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0 || nCol >= CDurationGate::BEHAVIOR_COLUMN_NUM)
    {
        return 0;
    }
    return pnActive[nCol];
}

/* 把三条区域事实折成一个读数。优先级 讲台 > 黑板 > 学生席：讲台与黑板在现场常常画得重叠
   （黑板就在讲台后面），重叠时"他在讲台上"是对面板（讲台时长）更有用的那一条，
   而学生席排最后是因为它的兜底口径最宽（没画该角色时"讲台/黑板之外"都算，见 CSubjectFact_S） */
inline int32_t areaOf(const CRowStatInput_S &stIn)
{
    if (!stIn.bAreaKnown)
    {
        /* 没有槽位 = 仲裁器这一拍没给出区域事实 */
        return static_cast<int32_t>(TeacherArea_E::TEA_AREA_UNKNOWN);
    }
    if (stIn.bInPodium)
    {
        return static_cast<int32_t>(TeacherArea_E::TEA_AREA_PODIUM);
    }
    if (stIn.bInBoard)
    {
        return static_cast<int32_t>(TeacherArea_E::TEA_AREA_BOARD);
    }
    if (stIn.bInStudent)
    {
        return static_cast<int32_t>(TeacherArea_E::TEA_AREA_STUDENT);
    }
    return static_cast<int32_t>(TeacherArea_E::TEA_AREA_OTHER);
}

/* 把一行位图里所有置位的行为计入生效人数。列序换算只准走 behaviorColumn/behaviorAtColumn，
   这里不重抄 101~112 / 201~213 的区间（重抄一处就多一处会与枚举漂移的地方） */
void countMask(uint32_t u32Mask, int nBitBase, int32_t *pnActive)
{
    for (int nBit = 0; nBit < 32; ++nBit)
    {
        if (0 == (u32Mask & (1u << nBit)))
        {
            continue;
        }
        const int nCol = nBitBase + nBit;
        if (nCol >= 0 && nCol < CDurationGate::BEHAVIOR_COLUMN_NUM)
        {
            ++pnActive[nCol];
        }
    }
}
}

/* 时段判定。三条出口一条都不能省：
   ① 没有分母 → UNKNOWN。姿态时间片轮转每 20s 就把 G_POSE 摘出去一次，那几拍里
      "生效人数 0"是**没测**不是**没有**——报 FALSE 等于每个空窗给平台一次"全班不听讲"；
   ② 阈值 <=0（不设闸）→ 有证据即 TRUE；
   ③ 其余按生效人数比阈值。注意分子是**时长门里生效**的人数（已含 min_duration 的确认时长），
      不是本拍判出来的人数，所以这道闸天然带着确认时长的迟——时段读数要的就是这个迟，
      一个人转头看旁边两秒不该把"讨论时段"点开 */
TriState_E groupPeriodOf(int32_t nActive, int32_t nEvidenceDenom, int32_t nMin)
{
    if (nEvidenceDenom <= 0)
    {
        return TriState_E::TS_UNKNOWN;
    }
    if (nMin <= 0)
    {
        return TriState_E::TS_TRUE;
    }
    return (nActive >= nMin) ? TriState_E::TS_TRUE : TriState_E::TS_FALSE;
}

int studentMaskBit(Behavior_E enBehavior)
{
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    return (nCol >= 0 && nCol < STU_COL_NUM) ? nCol : -1;
}

int teacherMaskBit(Behavior_E enBehavior)
{
    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < STU_COL_NUM || nCol >= CDurationGate::BEHAVIOR_COLUMN_NUM)
    {
        return -1;
    }
    return nCol - STU_COL_NUM;
}

const char *teacherAreaName(int32_t nArea)
{
    switch (static_cast<TeacherArea_E>(nArea))
    {
        case TeacherArea_E::TEA_AREA_PODIUM:  return "podium";
        case TeacherArea_E::TEA_AREA_BOARD:   return "board";
        case TeacherArea_E::TEA_AREA_STUDENT: return "student";
        case TeacherArea_E::TEA_AREA_OTHER:   return "other";
        case TeacherArea_E::TEA_AREA_UNKNOWN: return "unknown";
        default:                              return "bad";
    }
}

void fillRowSubject(CRowStatInput_S &rstIn, const CSubjectFact_S *pstFact,
                    const CRectf_S &stHeadBox, const CFrameGeometry &stGeom,
                    float fGroundShiftHeads)
{
    /* 位置换算对**每一行**都做，不只是教师 */
    CPointf_S stSample = stHeadBox.center();
    stSample.fY += stHeadBox.height() * fGroundShiftHeads;
    CPointf_S stNorm;
    if (stHeadBox.area() > 0.0f && stGeom.dstToNormSrc(stSample, stNorm))
    {
        rstIn.fNormX = stNorm.fX;
        rstIn.fNormY = stNorm.fY;
    }
    /* 换算失败（几何没就绪、框退化）时留 -1 = 这一拍不知道他在哪，
       而不是留 (0,0) = 他在画面左上角。教师轨迹与座位热力读的是同一个约定 */

    /* 主体分流的**唯一证据来源是仲裁器的事实**，不是帧行里的 stRow.enSubject */
    const bool bTeacher = (nullptr != pstFact
                           && Subject_E::SUBJECT_TEACHER == pstFact->enSubject);
    rstIn.bStudent = !bTeacher;
    if (!bTeacher)
    {
        return;
    }

    /* 有槽位才谈得上区域：没槽位是"这一拍没人看着他"，折成 OTHER 等于报"他在别处站着" */
    rstIn.bAreaKnown = true;
    rstIn.bInPodium = pstFact->bInPodium;
    rstIn.bInBoard = pstFact->bInBoard;
    rstIn.bInStudent = pstFact->bInStudentArea;
}

bool buildGroupMetric(const CFeatureFrame_S &stFrame, const CRowStatInput_S *patRows,
                      int32_t nRowNum, int64_t llSampleMs, const CGroupPeriodGate_S &stGate,
                      CGroupMetric_S &rstOut)
{
    rstOut = CGroupMetric_S();

    /* 三条拒绝各挡一种错：
       ① nRowNum>0 却没给位图数组 —— 但**空帧允许 patRows 为空**（"屏上 0 人"是一条有效样本，
          平台要看到 0，而不是看到"这一拍没采"，而空 vector 的 data() 本来就可能返回 nullptr）；
       ② nRowNum 超过 nPersonNum —— 位图数组是「每个特征行一条」，上界必须是本拍真行数；
          刻意不用 vecPersons.size()（那是编译期容量 60），拿它当上界等于不设闸，
          调用方传 2 行而实际只有 1 行时会直接读越界（ASan 抓到过）；
       ③ 帧自相矛盾（nPersonNum > 容量）—— 这种帧是别处的缺陷产物，不在这里替它兜 */
    if ((nRowNum > 0 && nullptr == patRows) || nRowNum < 0 || nRowNum > stFrame.nPersonNum ||
        stFrame.nPersonNum > static_cast<int32_t>(stFrame.vecPersons.size()))
    {
        return false;
    }
    if (llSampleMs <= 0)
    {
        return false;    /* 时基没起来：与规则层同一判据，宁可不采也不采一条时刻为 0 的样本 */
    }

    rstOut.llSampleMs = llSampleMs;
    rstOut.llFrameMs = stFrame.llMonotonicMs;
    rstOut.u32FeatureSeq = stFrame.u32Seq;
    rstOut.nPersonNum = stFrame.nPersonNum;
    rstOut.bTruncated = stFrame.bPersonTruncated;

    /* 逐人累加。行为之间允许同时成立，所以这里只加"生效人数"，
       任何复合指标都在下面按**同一行的位组合**判，不用生效人数做减法 */
    int32_t nStuUpNoDownNoBack = 0;
    int32_t anEviActive[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    int32_t nListenKnown = 0;
    int32_t nReadingKnown = 0;
    int32_t nDiscussKnown = 0;

    /* 被跟踪教师的行下标与"它那一行 TEA_TRACK 是否生效"。选法见 CGroupMetric_S 的注释：
       先落第一个教师行，之后任何跟踪生效的行把它换掉。两个教师同时被跟踪时取帧序里靠前的那一行
       ——本层不知道帧序按什么排（那是感知层写行的顺序），能依赖的只是它在一拍之内是确定的 */
    int32_t nChosenTea = -1;
    bool    bChosenTrack = false;

    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CRowStatInput_S &stIn = patRows[nRow];

        if (stIn.bStudent)
        {
            ++rstOut.nStudentNum;
            /* 逐类情绪人数：取这一行**最近一次判得出的**情绪平滑值 */
            const int32_t nExpr = static_cast<int32_t>(stFrame.vecPersons[nRow].enExpression);
            if (nExpr >= 0 && nExpr < CLASSROOM_EXPRESSION_CLASS_NUM)
            {
                ++rstOut.nExprNum[nExpr];
            }
            countMask(stIn.u32StuMask, 0, rstOut.nActive);
            if (stIn.bPoseEvidence)
            {
                ++rstOut.nStuEvidence;
                countMask(stIn.u32StuMask, 0, anEviActive);
                if (maskHas(stIn.u32StuKnownMask, studentMaskBit(Behavior_E::STU_LISTEN)))
                {
                    ++nListenKnown;
                }
                if (maskHas(stIn.u32StuKnownMask, studentMaskBit(Behavior_E::STU_READING)))
                {
                    ++nReadingKnown;
                }
                if (maskHas(stIn.u32StuKnownMask, studentMaskBit(Behavior_E::STU_DISCUSS)))
                {
                    ++nDiscussKnown;
                }
            }
            else
            {
                ++rstOut.nStuNoEvidence;
            }

            const int nUp = studentMaskBit(Behavior_E::STU_HEAD_UP);
            const int nDown = studentMaskBit(Behavior_E::STU_HEAD_DOWN);
            const int nBack = studentMaskBit(Behavior_E::STU_TURN_BACK);
            if (stIn.bPoseEvidence && maskHas(stIn.u32StuMask, nUp) &&
                !maskHas(stIn.u32StuMask, nDown) && !maskHas(stIn.u32StuMask, nBack))
            {
                ++nStuUpNoDownNoBack;
            }
        }
        else
        {
            ++rstOut.nTeacherNum;
            countMask(stIn.u32TeaMask, STU_COL_NUM, rstOut.nActive);
            if (stIn.bPoseEvidence)
            {
                ++rstOut.nTeaEvidence;
                countMask(stIn.u32TeaMask, STU_COL_NUM, anEviActive);
            }

            const bool bTrackOn = maskHas(stIn.u32TeaMask, teacherMaskBit(Behavior_E::TEA_TRACK));
            if (nChosenTea < 0 || (bTrackOn && !bChosenTrack))
            {
                nChosenTea = nRow;
                bChosenTrack = bTrackOn;
            }
        }
    }

    const int32_t nStuDenom = rstOut.nStuEvidence;
    const int32_t nTeaDenom = rstOut.nTeaEvidence;

    /* 被跟踪教师的读数。没人被选中时四条一起留默认值（id=-1、坐标=-1、区域=UNKNOWN）——
       刻意不把区域留成 OTHER：空场样本的 nPersonNum 已经是 0，此时"教师在哪里"
       这个问题不成立，答"在别处"是无中生有 */
    if (nChosenTea >= 0)
    {
        const CRowStatInput_S &stTea = patRows[nChosenTea];
        rstOut.nTeaPersonId = stTea.nPersonId;
        rstOut.fTeaX = stTea.fNormX;
        rstOut.fTeaY = stTea.fNormY;
        rstOut.nTeaArea = areaOf(stTea);
    }

    rstOut.fStuHeadUpRate = rateOf(activeAt(anEviActive, Behavior_E::STU_HEAD_UP), nStuDenom);
    rstOut.fStuHeadDownRate = rateOf(activeAt(anEviActive, Behavior_E::STU_HEAD_DOWN), nStuDenom);
    rstOut.fStuStandRate = rateOf(activeAt(anEviActive, Behavior_E::STU_STAND_UP), nStuDenom);
    rstOut.fStuRaiseRate = rateOf(activeAt(anEviActive, Behavior_E::STU_RAISE_HAND), nStuDenom);
    rstOut.fStuTurnBackRate = rateOf(activeAt(anEviActive, Behavior_E::STU_TURN_BACK), nStuDenom);
    rstOut.fStuAttention = rateOf(nStuUpNoDownNoBack, nStuDenom);
    rstOut.fStuSitRate = rateOf(nStuDenom - activeAt(anEviActive, Behavior_E::STU_STAND_UP),
                                nStuDenom);

    rstOut.fTeaHeadUpRate = rateOf(activeAt(anEviActive, Behavior_E::TEA_HEAD_UP), nTeaDenom);
    rstOut.fTeaStandRate = rateOf(activeAt(anEviActive, Behavior_E::TEA_STAND_UP), nTeaDenom);

    rstOut.enListenPeriod = groupPeriodOf(activeAt(anEviActive, Behavior_E::STU_LISTEN),
                                          nListenKnown, stGate.nListenMin);
    rstOut.enReadingPeriod = groupPeriodOf(activeAt(anEviActive, Behavior_E::STU_READING),
                                           nReadingKnown, stGate.nReadingMin);
    rstOut.enDiscussPeriod = groupPeriodOf(activeAt(anEviActive, Behavior_E::STU_DISCUSS),
                                           nDiscussKnown, stGate.nDiscussMin);

    return true;
}

std::string groupMetricText(const CGroupMetric_S &stM)
{
    /* 心跳里这一段的用途是"一眼看出指标是不是可信"：人数与分母并排给，
       只给百分比的话，"抬头率 0" 与 "没证据" 在日志里同样读不出来 */
    char szBuf[320] = {0};   /* 比常态长度宽一档的余量是留给"再加一个读数"的：
                                截断的读数比缺失的读数更危险（半行的 per= 会被读成一个值） */

    /* 时段三态用字符而不是 0/1/2：`per=1,0,2` 会被读成三个人数，而 y/n/? 不会被误认成计数，
       且 "?" 一眼就是"这一拍没测"。三个字符的**顺序**是 听讲/阅读/讨论 */
    auto chTri = [](TriState_E enState) -> char {
        return (TriState_E::TS_TRUE == enState) ? 'y'
                                                : ((TriState_E::TS_FALSE == enState) ? 'n' : '?');
    };

    /* 教师尾巴：现场判断"轨迹为什么没点"全靠这一串。tea=- 与 tea=2@unknown@-1.00,-1.00
       是三种完全不同的缺失（没教师 / 有教师但区域没画 / 有教师但几何没就绪），
       合并成一种就要靠翻 statusJson 才能分开，而看日志的人手边通常只有日志 */
    char szTea[64] = {0};
    if (stM.nTeaPersonId < 0)
    {
        (void)snprintf(szTea, sizeof(szTea), "tea=-");
    }
    else
    {
        const int nTea = snprintf(szTea, sizeof(szTea), "tea=%d@%s@%.2f,%.2f", stM.nTeaPersonId,
                                  teacherAreaName(stM.nTeaArea), stM.fTeaX, stM.fTeaY);

        if (nTea < 0 || static_cast<size_t>(nTea) >= sizeof(szTea))
        {
            (void)snprintf(szTea, sizeof(szTea), "tea=%d@trunc", stM.nTeaPersonId);
        }
    }

    snprintf(szBuf, sizeof(szBuf),
             "num=%d(stu %d/tea %d) evi=%d|%d miss=%d trunc=%d "
             "up=%.2f down=%.2f stand=%.2f raise=%.2f back=%.2f att=%.2f sit=%.2f "
             "per=%c%c%c %s",
             stM.nPersonNum, stM.nStudentNum, stM.nTeacherNum,
             stM.nStuEvidence, stM.nTeaEvidence, stM.nStuNoEvidence,
             stM.bTruncated ? 1 : 0,
             stM.fStuHeadUpRate, stM.fStuHeadDownRate, stM.fStuStandRate,
             stM.fStuRaiseRate, stM.fStuTurnBackRate, stM.fStuAttention, stM.fStuSitRate,
             chTri(stM.enListenPeriod), chTri(stM.enReadingPeriod), chTri(stM.enDiscussPeriod),
             szTea);
    return std::string(szBuf);
}
}
