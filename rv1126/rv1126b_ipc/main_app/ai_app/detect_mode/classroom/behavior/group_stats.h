/**
 * @FilePath     : group_stats.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 09:24:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 09:24:00
 * @Description  : 群体指标合成
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"
#include "duration_gate.h"
#include "feature_frame.h"
#include "subject_arbiter.h"

namespace Classroom
{
/* 一行的输入：本拍该人当前生效了哪些行为 */
typedef struct _RowStatInput_
{
    int32_t  nPersonId = -1;
    uint32_t u32StuMask = 0;          /* 学生 12 列的生效位（列序同 behaviorColumn） */
    uint32_t u32StuKnownMask = 0;     /* 学生 12 列里"最近一次判定有结论"的位（TRUE 或 FALSE；UNKNOWN 不置位）*/
    uint32_t u32TeaMask = 0;          /* 教师 13 列的生效位，位号 = 该列在教师段内的序号 */
    bool     bPoseEvidence = false;   /* 本行此刻有可用骨骼（姿态类比率的分母） */
    bool     bStudent = true;         /* 主体分流。取归属仲裁的结果，不是感知层逐帧写的默认值
                                         （那一路恒为 STUDENT，见 CSubjectArbiter） */
    bool     bFaceEvidence = false;   /* 本行此刻有可用人脸/表情证据 */

    /* ---- 位置（每行都填）与区域（只有 bStudent=false 的行会被填）---- */
    float    fNormX = -1.0f;          /* -1 = 这一拍没有位置证据（几何未就绪或无框） */
    float    fNormY = -1.0f;
    bool     bAreaKnown = false;      /* 区域事实可用（归属仲裁器给了这一行的槽位） */
    bool     bInPodium = false;
    bool     bInBoard = false;
    bool     bInStudent = false;      /* 学生席；未画该角色时是「讲台/黑板之外」的兜底口径 */
} CRowStatInput_S;

/**
 * @brief 时段人数阈值（CSV 16~18 那三条【抬头人数阈值】/【低头人数阈值】/【转身人数阈值】，
 *        以及讨论的「最少参与人数」）
 */
typedef struct _GroupPeriodGate_
{
    int32_t nListenMin  = 1;   /* 听讲 107：生效人数达此数才算这一拍在听讲时段 */
    int32_t nReadingMin = 1;   /* 阅读 108 */
    int32_t nDiscussMin = 2;   /* 讨论 109：最少参与人数，1 个人不叫讨论 */
} CGroupPeriodGate_S;

/**
 * @brief   : 时段判定：生效人数够不够、有没有分母
 */
TriState_E groupPeriodOf(int32_t nActive, int32_t nEvidenceDenom, int32_t nMin);

/**
 * @brief 一条群体指标样本
 */
typedef struct _GroupMetric_
{
    int64_t  llSampleMs = 0;          /* 采样时刻（单调毫秒，与特征帧同一时基） */
    int64_t  llFrameMs = 0;           /* 所依据特征帧的产出时刻，0 表示这一拍没有新帧 */
    uint32_t u32FeatureSeq = 0;

    int32_t  nPersonNum = 0;          /* 屏上确认人数（含无证据行） */
    int32_t  nStudentNum = 0;
    int32_t  nTeacherNum = 0;
    bool     bTruncated = false;      /* 人数被容量截断：带上它，"人数偏少"才有解释 */

    int32_t  nStuEvidence = 0;        /* 有骨骼证据的学生数 = 学生姿态类比率的分母 */
    int32_t  nTeaEvidence = 0;
    int32_t  nStuNoEvidence = 0;      /* 与上一行互补：这条决定"缺证率"，是降级判据 */

    /* 各行为生效人数。下标 = CDurationGate::behaviorColumn()，
       与门表/配置同一套列序；未开启的行为恒 0，与"开了但没人做"同为 0 ——
       区分它们看配置快照，样本不重复存开关状态 */
    int32_t  nActive[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };

    /* 本拍各情绪类别的人数 */
    int32_t  nExprNum[CLASSROOM_EXPRESSION_CLASS_NUM] = { 0 };

    /* 复合指标：分母为 0 时一律 -1.0f（UNKNOWN），不是 0.0f */
    float    fStuHeadUpRate = -1.0f;
    float    fStuHeadDownRate = -1.0f;
    float    fStuStandRate = -1.0f;
    float    fStuRaiseRate = -1.0f;
    float    fStuTurnBackRate = -1.0f;
    /* 专注度：抬头 且 不低头 且 不背身 的人数 / 有证据人数。逐人判而非比率相减，
       理由见文件头口径 2 */
    float    fStuAttention = -1.0f;
    /* 就坐率是**代理指标**：(人数 - 站立人数) / 人数 */
    float    fStuSitRate = -1.0f;

    float    fTeaHeadUpRate = -1.0f;
    float    fTeaStandRate = -1.0f;


    /* 时段判定（CSV 16~18 的【…人数阈值】） */
    TriState_E enListenPeriod  = TriState_E::TS_UNKNOWN;
    TriState_E enReadingPeriod = TriState_E::TS_UNKNOWN;
    TriState_E enDiscussPeriod = TriState_E::TS_UNKNOWN;   /* 讨论＝最少参与人数（默认 2） */

    /* ---- 被跟踪教师----
       max_num 可配 >1，而"教师当前在哪"在面板上只能有一个人，所以这里必须写清选了谁：
       取本拍 TEA_TRACK 生效的第一行，没有生效列时退到第一个有位置的教师行 */
    int32_t  nTeaPersonId = -1;       /* -1 = 本拍没有可归属的教师 */
    float    fTeaX = -1.0f;           /* 源帧归一化 0~1；-1 = 没有位置证据（≠ 在左上角） */
    float    fTeaY = -1.0f;
    int32_t  nTeaArea = static_cast<int32_t>(TeacherArea_E::TEA_AREA_UNKNOWN);
} CGroupMetric_S;

/**
 * @brief   : 把「归属事实 + 人头框」折成一行的主体、区域与位置三组输入
 * @param    {CRowStatInput_S&} rstIn：IN/OUT，调用方已复位并填好位图与证据位
 * @param    {const CSubjectFact_S*} pstFact：仲裁器给这一行的事实，nullptr = 本拍没有他的槽位
 * @param    {const CRectf_S&} stHeadBox：本行人头框（分析帧坐标）
 * @param    {const CFrameGeometry&} stGeom：本帧几何（未就绪时位置留 -1）
 * @param    {float} fGroundShiftHeads：采样点俯角修正（scene.ground_shift，0=不动）。
 *            只影响位置换算，不碰主体与区域三事实——它标定的是"他在图像上的哪儿"，
 *            不是"他是谁"
 */
void fillRowSubject(CRowStatInput_S &rstIn, const CSubjectFact_S *pstFact,
                    const CRectf_S &stHeadBox, const CFrameGeometry &stGeom,
                    float fGroundShiftHeads);

/**
 * @brief   : 合成一条群体样本
 * @param    {const CFeatureFrame_S&} stFrame：本拍特征帧（人数、截断标志、时刻都取这里）
 * @param    {patRows / nRowNum}：与 stFrame.vecPersons 同序的「每人生效位图」，
 *            由调用方从 CRuleEngine::isActive() 逐人逐列取（节拍线程内，门表无锁）
 * @param    {int64_t} llSampleMs：本条样本的采样时刻
 * @param    {const CGroupPeriodGate_S&} stGate：三道人数阈值（取自配置快照的 rules.group，
 *            由调用方搬运）。这里不收 CGroupBehaviorConf_S：群体层不 include 配置层，
 *            否则每个用群体层的单测都要拖一整套 JSON 依赖（与 fillRowSubject 只要三个裸事实同理）
 * @param    {CGroupMetric_S&} rstOut：OUT=完整一条样本，函数内先复位
 * @return   {bool} false 表示入参不可用（行数与容量不符等），rstOut 已复位，调用方不得计入
 */
bool buildGroupMetric(const CFeatureFrame_S &stFrame, const CRowStatInput_S *patRows,
                      int32_t nRowNum, int64_t llSampleMs, const CGroupPeriodGate_S &stGate,
                      CGroupMetric_S &rstOut);

/**
 * @brief   : 学生某列的位号（教师列另有一套，见 CRowStatInput_S::u32TeaMask）
 * @return   {int} 非学生列返回 -1
 */
int studentMaskBit(Behavior_E enBehavior);

/**
 * @brief   : 教师某列在其段内的位号
 * @return   {int} 非教师列返回 -1
 */
int teacherMaskBit(Behavior_E enBehavior);

/**
 * @brief   : 教师区域读数的文本（报表与心跳共用，唯一口径表）
 */
const char *teacherAreaName(int32_t nArea);

/* 读数一行文本（`classroom report:` 那条日志的头部，由引擎的 deliverReport 打） */
std::string groupMetricText(const CGroupMetric_S &stM);
}
