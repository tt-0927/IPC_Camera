/**
 * @FilePath     : student_behavior.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 10:07:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 10:07:00
 * @Description  : 学生姿态行为采样器实现
 */

#include "student_behavior.h"

#include "classroom_config.h"
#include "geometry.h"

namespace Classroom
{
namespace
{
/* 列顺序 -> 行为枚举的映射表
 * 这张表是采样器与外部的契约：rvOut 的第 i 个元素就代表 astBehavior[i]。
 * 下面的一串 static_assert 保证"表内容"与"列枚举"不会各自漂移——改动任一侧都会编译失败。 */
constexpr Behavior_E astBehavior[SC_NUM] = {
    Behavior_E::STU_STAND_UP,
    Behavior_E::STU_RAISE_HAND,
    Behavior_E::STU_HEAD_UP,
    Behavior_E::STU_HEAD_DOWN,
    Behavior_E::STU_TURN_BACK,
    Behavior_E::STU_PHONE,
    Behavior_E::STU_LISTEN,
    Behavior_E::STU_READING,
    Behavior_E::STU_DISCUSS,
    Behavior_E::STU_ON_PODIUM,
    Behavior_E::STU_PRESENT,
    Behavior_E::STU_EXPRESSION,
};

static_assert(Behavior_E::STU_STAND_UP == astBehavior[SC_STAND], "student sampler table order != SC_STAND");
static_assert(Behavior_E::STU_RAISE_HAND == astBehavior[SC_RAISE_HAND], "student sampler table order != SC_RAISE_HAND");
static_assert(Behavior_E::STU_HEAD_UP == astBehavior[SC_HEAD_UP], "student sampler table order != SC_HEAD_UP");
static_assert(Behavior_E::STU_HEAD_DOWN == astBehavior[SC_HEAD_DOWN], "student sampler table order != SC_HEAD_DOWN");
static_assert(Behavior_E::STU_TURN_BACK == astBehavior[SC_TURN_BACK], "student sampler table order != SC_TURN_BACK");
static_assert(Behavior_E::STU_PHONE == astBehavior[SC_PHONE], "student sampler table order != SC_PHONE");
static_assert(Behavior_E::STU_LISTEN == astBehavior[SC_LISTEN], "student sampler table order != SC_LISTEN");
static_assert(Behavior_E::STU_READING == astBehavior[SC_READING], "student sampler table order != SC_READING");
static_assert(Behavior_E::STU_DISCUSS == astBehavior[SC_DISCUSS], "student sampler table order != SC_DISCUSS");
static_assert(Behavior_E::STU_ON_PODIUM == astBehavior[SC_PODIUM], "student sampler table order != SC_PODIUM");
static_assert(Behavior_E::STU_PRESENT == astBehavior[SC_PRESENT], "student sampler table order != SC_PRESENT");
static_assert(Behavior_E::STU_EXPRESSION == astBehavior[SC_EXPRESSION], "student sampler table order != SC_EXPRESSION");

/* 把整批结果一次性置为同一状态/原因，用于"提前退出"的统一收尾
 * 典型场景：无姿态、遮挡、模糊、教师主体——此时所有行为都无结论，但保留各自已算好的特例列 */
void setAll(std::vector<CBehaviorSample_S> &rvOut, TriState_E enState, SampleReason_E enReason)
{
    for (CBehaviorSample_S &stSample : rvOut)
    {
        stSample.enState = enState;
        stSample.enReason = enReason;
    }
}

/* 由三态直接构造采样结果；UNKNOWN 时原因自动记为 KEY_LOW（关键信息不足） */
CBehaviorSample_S sampleOf(TriState_E enState)
{
    CBehaviorSample_S stSample;
    stSample.enState = enState;
    stSample.enReason = (TriState_E::TS_UNKNOWN == enState) ? SampleReason_E::SR_KEY_LOW
                                                            : SampleReason_E::SR_OK;
    return stSample;
}

/* 构造"本帧无法判定"的结果：UNKNOWN + 指定原因
 * 与 FALSE 区分开很重要——UNKNOWN 不会打断已成立的事件，FALSE 会 */
CBehaviorSample_S blockedOf(SampleReason_E enReason)
{
    CBehaviorSample_S stSample;
    stSample.enState = TriState_E::TS_UNKNOWN;
    stSample.enReason = enReason;
    return stSample;
}

/* 被更优先的行为否决（互斥抑制）：结果是 FALSE，原因 INHIBIT
 * 例如趴桌成立时，抬头/低头都不该再独立成立 */
CBehaviorSample_S inhibitedOf()
{
    CBehaviorSample_S stSample;
    stSample.enState = TriState_E::TS_FALSE;
    stSample.enReason = SampleReason_E::SR_INHIBIT;
    return stSample;
}

/* 小模型给出否定结论：FALSE + MODEL_NEG
 * 与 inhibitedOf 区分，是为了在上报侧能看出"是模型否掉的"还是"规则互斥否掉的" */
CBehaviorSample_S negatedOf()
{
    CBehaviorSample_S stSample;
    stSample.enState = TriState_E::TS_FALSE;
    stSample.enReason = SampleReason_E::SR_MODEL_NEG;
    return stSample;
}

/* 构造 FALSE + MODEL_POS，用于"模型已确认是 A，因此排除 B"的互斥结论
 * 注意：它产出的是 FALSE 状态，MODEL_POS 只表达"依据来自模型的肯定判断" */
CBehaviorSample_S affirmedOf()
{
    CBehaviorSample_S stSample;
    stSample.enState = TriState_E::TS_FALSE;
    stSample.enReason = SampleReason_E::SR_MODEL_POS;
    return stSample;
}

/* 106/108 共用的「低头 × 手在身前」形状。区别只在低头那一半的规则实例：
   106 用 stSnap.stHeadDown，108 用 stSnap.stHeadDownReading；手那一半也各用自己的实例
   （stPhone / stPhoneReading） */
CBehaviorSample_S phoneShapeOf(const CBehaviorSample_S &stHeadDown,
                               const CGeometryInput &stGeom,
                               const PhoneRule_S &stPhoneRule,
                               bool bOnBoundary)
{
    if (bOnBoundary)
    {
        return blockedOf(SampleReason_E::SR_EDGE);   /* 腕与肘的证据会被画面边切走一半 */
    }
    if (TriState_E::TS_TRUE != stHeadDown.enState)
    {
        CBehaviorSample_S stShape;
        stShape.enState  = stHeadDown.enState;
        stShape.enReason = stHeadDown.enReason;   /* 成因照抄低头那一行：趴桌压的记成 INHIBIT，点不够的记成 KEY_LOW */
        return stShape;
    }
    return sampleOf(judgePhoneUse(stGeom, stPhoneRule));
}

/* 106/108 的复核三态出口。两条形状各自算，但复核结论只有一份：共用这一函数是为了
   保证「106 与 108 不可能同时为真」这条不变式在两条形状上一致成立，而不是各抄一遍
   然后某天改了一边 */
void splitPhoneReading(const CBehaviorSample_S &stShape, bool bRecheckKnown, bool bPhoneObj,
                       CBehaviorSample_S &rstPhone, CBehaviorSample_S &rstReading)
{
    rstPhone   = stShape;
    rstReading = stShape;
    if (TriState_E::TS_TRUE != stShape.enState)
    {
        return;
    }
    if (!bRecheckKnown)
    {
        rstReading = blockedOf(SampleReason_E::SR_NO_MODEL);
        rstPhone.enReason = SampleReason_E::SR_FAIL_OPEN;
    }
    else if (bPhoneObj)
    {
        rstReading = affirmedOf();
    }
    else
    {
        rstPhone = negatedOf();
    }
}

/* 复合判据的合取。为什么要有这个函数而不是把 if 一层层套进去：
   107~111 每一条都是两三原子项的与，手写嵌套会把"哪一项把它挡住的"这条信息丢掉
   （而那条信息正是心跳 why 段唯一的读数），也会让 FALSE 与 UNKNOWN 的先后在各处不一致 */
CBehaviorSample_S andOf(const CBehaviorSample_S &stA, const CBehaviorSample_S &stB)
{
    if (TriState_E::TS_FALSE == stA.enState)
    {
        return stA;
    }
    if (TriState_E::TS_FALSE == stB.enState)
    {
        return stB;
    }
    if (TriState_E::TS_UNKNOWN == stA.enState)
    {
        return stA;
    }
    if (TriState_E::TS_UNKNOWN == stB.enState)
    {
        return stB;
    }
    return stB;
}

/* 三态逻辑非：TRUE→FALSE(INHIBIT)，FALSE→TRUE，UNKNOWN 保持 UNKNOWN
 * 用 INHIBIT 而非 MODEL_NEG 表达取反后的 FALSE，避免污染"模型结论"统计 */
CBehaviorSample_S notOf(const CBehaviorSample_S &stIn)
{
    if (TriState_E::TS_UNKNOWN == stIn.enState)
    {
        return stIn;
    }
    if (TriState_E::TS_TRUE == stIn.enState)
    {
        return inhibitedOf();
    }
    return sampleOf(TriState_E::TS_TRUE);
}

/* 取某行为生效的区域 ID 列表
 * 优先用该行为显式配置的区域；没配则回落到"该角色的默认区域"（讲台/座位），
 * 保证只配了角色区域时行为依然可用 */
const std::vector<int32_t> &behaviorRegionIds(const CConfigSnapshot_S &stSnap,
                                              const CRegionTable *pstRegion,
                                              Behavior_E enBehavior, RegionRole_E enRole)
{
    const CRegionRef_S &stRef = stSnap.regionRefOf(enBehavior);
    if (!stRef.vecRegionIds.empty())
    {
        return stRef.vecRegionIds;
    }
    return pstRegion->idsOfRole(enRole);
}

/* 计算多个区域并集的水平中心（取所有区域 X 范围的最小/最大再取中点）
 * rbKnown 输出"是否至少有一个区域存在"，false 时返回值无意义，调用方据此跳过方位判定 */
float regionUnionCenterX(const CRegionTable *pstRegion, const std::vector<int32_t> &vecIds,
                         bool &rbKnown)
{
    rbKnown = false;
    float fMinX = 0.0f;
    float fMaxX = 0.0f;
    for (const int32_t nId : vecIds)
    {
        const CRegionArea_S *pstArea = pstRegion->regionOf(nId);
        if (nullptr == pstArea)
        {
            continue;
        }
        if (!rbKnown)
        {
            fMinX = pstArea->stBox.fX1;
            fMaxX = pstArea->stBox.fX2;
            rbKnown = true;
        }
        else
        {
            fMinX = (pstArea->stBox.fX1 < fMinX) ? pstArea->stBox.fX1 : fMinX;
            fMaxX = (pstArea->stBox.fX2 > fMaxX) ? pstArea->stBox.fX2 : fMaxX;
        }
    }
    return rbKnown ? (fMinX + fMaxX) * 0.5f : 0.0f;
}

/* 判断参照物在人的左侧还是右侧，输出 -1/0/+1
 * 死区 = fDeadbandHead * 头宽：|差值| 在死区内返回 0（方位不可判），
 * 这样能避免人正对参照物时朝向判定在左右之间抖动 */
float refSignOf(float fRefCenterX, float fHeadCenterX, float fHeadWidth, float fDeadbandHead)
{
    const float fBand = fDeadbandHead * ((fHeadWidth > 0.0f) ? fHeadWidth : 1.0f);
    const float fDelta = fRefCenterX - fHeadCenterX;
    if (fDelta > fBand)
    {
        return 1.0f;
    }
    if (fDelta < -fBand)
    {
        return -1.0f;
    }
    return 0.0f;
}

/* 结合"参照物方位符号"做朝向判定；符号为 0（无参照）时直接给 UNKNOWN/REGION_MISS，
 * 不把"没有参照"误判成"没有面向" */
CBehaviorSample_S dirSampleOf(const CGeometryInput &stGeom, const FaceDirRule_S &stRule, float fRefSign)
{
    if (0.0f == fRefSign)
    {
        return blockedOf(SampleReason_E::SR_REGION_MISS);
    }
    return sampleOf(judgeFacingRef(stGeom, stRule, fRefSign));
}

/* 用"除自己以外的所有学生"的头框中心均值估算班级中心 X
 * 用途：座位区域没配置时，用班级中心作为"面向班级"的参照，属于兜底路径。
 * 过滤规则：跳过自己、跳过无效 ID、跳过教师、跳过无有效头框的行 */
bool classCenterX(const CBehaviorSampleIn_S &stIn, int32_t nSelfIdx, float &rfX)
{
    if (nullptr == stIn.pstFrame || nullptr == stIn.penRowSubject)
    {
        return false;
    }
    const int32_t nCap = static_cast<int32_t>(stIn.pstFrame->vecPersons.size());
    int32_t nScan = (stIn.pstFrame->nPersonNum < stIn.nRowSubjectNum) ? stIn.pstFrame->nPersonNum
                                                                       : stIn.nRowSubjectNum;
    if (nScan > nCap)
    {
        nScan = nCap;
    }

    float fSum = 0.0f;
    int32_t nNum = 0;
    for (int32_t i = 0; i < nScan; ++i)
    {
        if (i == nSelfIdx)
        {
            continue;
        }
        const CPersonFeature_S &stOther = stIn.pstFrame->vecPersons[static_cast<size_t>(i)];
        if (stOther.nPersonId < 0 || Subject_E::SUBJECT_TEACHER == stIn.rowSubjectAt(i))
        {
            continue;
        }
        if (stOther.stHeadBox.area() <= 0.0f)
        {
            continue;
        }
        fSum += stOther.stHeadBox.center().fX;
        ++nNum;
    }
    if (nNum <= 0)
    {
        return false;
    }
    rfX = fSum / static_cast<float>(nNum);
    return true;
}

/* 讲台在哪一侧 */
bool teacherHeadX(const CBehaviorSampleIn_S &stIn, float &rfX)
{
    if (nullptr == stIn.pstFrame || nullptr == stIn.penRowSubject)
    {
        return false;
    }
    const int32_t nCap = static_cast<int32_t>(stIn.pstFrame->vecPersons.size());
    int32_t nScan = (stIn.pstFrame->nPersonNum < stIn.nRowSubjectNum) ? stIn.pstFrame->nPersonNum
                                                                       : stIn.nRowSubjectNum;
    if (nScan > nCap)
    {
        nScan = nCap;
    }

    for (int32_t i = 0; i < nScan; ++i)
    {
        if (Subject_E::SUBJECT_TEACHER != stIn.rowSubjectAt(i))
        {
            continue;
        }
        const CPersonFeature_S &stOther = stIn.pstFrame->vecPersons[static_cast<size_t>(i)];
        if (stOther.nPersonId < 0 || stOther.stHeadBox.area() <= 0.0f)
        {
            continue;   /* 与 classCenterX 同一条：空框的中心会拽到画面左上角 */
        }
        rfX = stOther.stHeadBox.center().fX;
        return true;
    }
    return false;
}
}   // namespace

CStudentBehaviorSampler::CStudentBehaviorSampler()
{
    m_vecBehavior.assign(astBehavior, astBehavior + SC_NUM);
}

void CStudentBehaviorSampler::samplePerson(const CBehaviorSampleIn_S &stIn, std::vector<CBehaviorSample_S> &rvOut)
{
    /* 先整表铺 UNKNOWN：引擎为了运行期零分配而复用同一张样本表，表里留着的是**上一个人**的结果。
       漏写一项的后果不是不准，是「张三的站立的被当成李四的站立」——行为串人 */
    setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_NO_FRAME);

    if (rvOut.size() != static_cast<size_t>(SC_NUM))
    {
        /* 长度对不上声明表＝契约已被破坏，引擎那边会喊这条错（并补齐长度）。
           这里只做一件事：不往下写，越界写会把错误变成内存问题 */
        return;
    }

    if (nullptr == stIn.pstFrame || nullptr == stIn.pstPerson || nullptr == stIn.pstSnap)
    {
        return;   /* 保持整表 UNKNOWN/SR_NO_FRAME */
    }

    const CFeatureFrame_S   &stFrame = *stIn.pstFrame;
    const CPersonFeature_S  &stRow   = *stIn.pstPerson;
    const CConfigSnapshot_S &stSnap  = *stIn.pstSnap;

    // 主体身份：优先用仲裁器结论，没有则退回特征自带的身份。教师不参与学生行为判定
    const Subject_E enSubject = (nullptr != stIn.pstSubject) ? stIn.pstSubject->enSubject
                                                             : stRow.enSubject;
    if (Subject_E::SUBJECT_TEACHER == enSubject)
    {
        setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_SUBJECT);
        return;
    }

    // 讲台判定：只依赖头框与区域表，与姿态无关，故放在所有姿态检查之前，且提前返回时也需要保留
    CBehaviorSample_S stPodium = blockedOf(SampleReason_E::SR_REGION_MISS);
    if (nullptr != stIn.pstRegion && !stIn.pstRegion->isEmpty())
    {
        const std::vector<int32_t> &vecPodiumIds =
            behaviorRegionIds(stSnap, stIn.pstRegion, Behavior_E::STU_ON_PODIUM, RegionRole_E::REGION_ROLE_PODIUM);
        if (!vecPodiumIds.empty())
        {
            bool bConfigError = false;
            const bool bHit = stIn.pstRegion->inAnyRegion(vecPodiumIds,
                                                          stSnap.regionRefOf(Behavior_E::STU_ON_PODIUM).fMinCoverage,
                                                          stRow.stHeadBox, bConfigError);
            stPodium = bConfigError ? blockedOf(SampleReason_E::SR_REGION_MISS)
                                     : sampleOf(bHit ? TriState_E::TS_TRUE : TriState_E::TS_FALSE);
        }
    }
    rvOut[SC_PODIUM] = stPodium;

    // 姿态模型未驻留 / 本帧姿态已过期：其余行为全部无结论
    if (!stFrame.bPoseResident)
    {
        setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_NO_POSE);
        rvOut[SC_PODIUM] = stPodium;
        return;
    }
    const int32_t nPoseTtlMs = stFrame.poseTtlMs();
    if (!stRow.poseUsable(stIn.llNowMs, nPoseTtlMs))
    {
        setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_NO_POSE);
        rvOut[SC_PODIUM] = stPodium;
        return;
    }
    // 画面过糊：关键点位置不可信，宁可给 UNKNOWN 让门控保持，也不要给出错误的 FALSE
    if (stRow.fBlurScore > 0.0f &&
        stRow.fBlurScore < poseBlurMinForHeadPx(stSnap.fPoseBlurMin, stRow.stHeadBox.height()))
    {
        setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_BLUR);
        rvOut[SC_PODIUM] = stPodium;
        return;
    }

    if (stRow.bOccluded)
    {
        setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_OCCLUDED);
        rvOut[SC_PODIUM] = stPodium;
        return;
    }

    // 统一几何输入：把骨架、头框、身体框、肩宽打包，后面所有 judgeXxx 都只吃这一份
    CGeometryInput stGeom;
    stGeom.pstSkeleton    = &stRow.stSkeleton;
    stGeom.stHeadBox      = stRow.stHeadBox;
    stGeom.stBodyBox      = stRow.stBodyBox;
    stGeom.fShoulderWidth = shoulderWidthOf(stRow.stSkeleton, stSnap.fPoseScoreThreshold);
    const float fHeadWidth = (stRow.stHeadBox.width() > 0.0f)
                                 ? stRow.stHeadBox.width() : stGeom.fShoulderWidth;
    const float fHeadCenterX = stRow.stHeadBox.center().fX;

    const CBehaviorSample_S stStand = stRow.bOnBoundary ? blockedOf(SampleReason_E::SR_EDGE)
                                                        : sampleOf(judgeStanding(stGeom, stSnap.stStand));
    const CBehaviorSample_S stRaise = stRow.bOnBoundary ? blockedOf(SampleReason_E::SR_EDGE)
                                                        : sampleOf(judgeRaiseHand(stGeom, stSnap.stRaiseHand));
    const CBehaviorSample_S stTurn  = stRow.bOnBoundary ? blockedOf(SampleReason_E::SR_EDGE)
                                                        : sampleOf(judgeTurnBack(stGeom, stSnap.stTurnBack));

    CBehaviorSample_S stHeadUp   = sampleOf(judgeHeadUp(stGeom, stSnap.stHeadUp));
    CBehaviorSample_S stHeadDown = sampleOf(judgeHeadDown(stGeom, stSnap.stHeadDown));
    /* 解耦：104/106/108 的低头门各判各的 */
    CBehaviorSample_S stHeadDownPhone   = sampleOf(judgeHeadDown(stGeom, stSnap.stHeadDownPhone));
    CBehaviorSample_S stHeadDownReading = sampleOf(judgeHeadDown(stGeom, stSnap.stHeadDownReading));

    /* 头部一族消歧（趴桌 > 低头 > 抬头） */
    const TriState_E enLie = judgeLieOnDesk(stGeom, stSnap.stLieOnDesk);
    if (TriState_E::TS_TRUE == enLie)
    {
        stHeadUp            = inhibitedOf();
        stHeadDown          = inhibitedOf();
        stHeadDownPhone     = inhibitedOf();
        stHeadDownReading   = inhibitedOf();
    }
    else if (TriState_E::TS_TRUE == stHeadDown.enState && TriState_E::TS_TRUE == stHeadUp.enState)
    {
        /* 两条同时为真在几何上不共存（低头比值与抬头夹角会同时越门，说明关键点在抖），
           按在网口径留低头、压抬头：抬头率是本子系统的头条指标，宁可少计不可虚高 */
        stHeadUp = inhibitedOf();
    }

    /*  玩手机 106 与阅读 108：**同一个形状的两个出口**  */
    const CBehaviorSample_S stPhoneShape =
        phoneShapeOf(stHeadDownPhone, stGeom, stSnap.stPhone, stRow.bOnBoundary);
    const CBehaviorSample_S stReadingShape =
        phoneShapeOf(stHeadDownReading, stGeom, stSnap.stPhoneReading, stRow.bOnBoundary);

    /* 复核结论是否可用只看时间戳：phoneUsable 刻意不要求 bPhoneObj，「看了，没有」是这里唯一
       能否决 106、也唯一能确立 108 的真结论 */
    const bool bRecheckKnown = stRow.phoneUsable(stIn.llNowMs, stSnap.stPhoneRecheck.nTtlMs);

    CBehaviorSample_S stPhone;
    CBehaviorSample_S stReading;
    CBehaviorSample_S stDiscard;
    splitPhoneReading(stPhoneShape, bRecheckKnown, stRow.bPhoneObj, stPhone, stDiscard);
    splitPhoneReading(stReadingShape, bRecheckKnown, stRow.bPhoneObj, stDiscard, stReading);

    /* 参考侧：讲台在哪一侧（区域几何给的，不给配置里填"左/右"——那种旋钮在镜像机位上整体判反）*/
    float fPodiumSign = 0.0f;
    float fClassSign  = 0.0f;
    if (nullptr != stIn.pstRegion && !stIn.pstRegion->isEmpty())
    {
        bool bKnown = false;
        const std::vector<int32_t> &vecPodiumIds =
            behaviorRegionIds(stSnap, stIn.pstRegion, Behavior_E::STU_ON_PODIUM, RegionRole_E::REGION_ROLE_PODIUM);
        const float fPodiumX = regionUnionCenterX(stIn.pstRegion, vecPodiumIds, bKnown);
        if (bKnown)
        {
            fPodiumSign = refSignOf(fPodiumX, fHeadCenterX, fHeadWidth, stSnap.stGroup.fRefDeadbandHead);
        }

        const std::vector<int32_t> &vecSeatIds =
            behaviorRegionIds(stSnap, stIn.pstRegion, Behavior_E::STU_PRESENT, RegionRole_E::REGION_ROLE_STUDENT);
        const float fSeatX = regionUnionCenterX(stIn.pstRegion, vecSeatIds, bKnown);
        if (bKnown)
        {
            fClassSign = refSignOf(fSeatX, fHeadCenterX, fHeadWidth, stSnap.stGroup.fRefDeadbandHead);
        }
    }
    if (0.0f == fPodiumSign)
    {
        /* 讲台区域没画（或中心落进死区）时的兜底 */
        float fTeacherX = 0.0f;
        if (teacherHeadX(stIn, fTeacherX))
        {
            fPodiumSign = refSignOf(fTeacherX, fHeadCenterX, fHeadWidth, stSnap.stGroup.fRefDeadbandHead);
        }
    }
    if (0.0f == fClassSign)
    {
        /* 学生席没画（或画成正对着一排人的死区）时，"全班在哪一侧"还有第二个来源：
           这一帧里其他学生的平均位置。它比"判不出"更贴近事实——演示的人是站在同学对面说话的，
           而同学此刻就在画面里 */
        float fClassX = 0.0f;
        if (classCenterX(stIn, stIn.nSelfIdx, fClassX))
        {
            fClassSign = refSignOf(fClassX, fHeadCenterX, fHeadWidth, stSnap.stGroup.fRefDeadbandHead);
        }
    }

    // 听讲 = 抬头 且 未回头 且 面向讲台；若同时检出明显举手/手机动作则否决（配置可关）
    CBehaviorSample_S stListen = andOf(andOf(stHeadUp, notOf(stTurn)),
                                       stRow.bOnBoundary ? blockedOf(SampleReason_E::SR_EDGE)
                                                          : dirSampleOf(stGeom, stSnap.stGroup.stListenDir, fPodiumSign));
    if (stSnap.stGroup.bListenForbidHand && TriState_E::TS_TRUE == stListen.enState)
    {
        /* 排在这里而不是并进合取：前三项都不成立时连手都不必看（廉价在前） */
        const CBehaviorSample_S stHand = sampleOf(judgePhoneUse(stGeom, stSnap.stPhoneReading));
        if (TriState_E::TS_TRUE == stHand.enState)
        {
            /* 平反：低头×手在身前是「捧书」与「捧手机」共有的二维形状，几何分不开两者 */
            const bool bExonerated =
                stRow.phoneUsable(stIn.llNowMs, stSnap.stPhoneRecheck.nTtlMs) && !stRow.bPhoneObj;
            if (!bExonerated)
            {
                stListen = inhibitedOf();
            }
        }
    }

    // 讨论 = 在邻座距离内，且面向其中至少一人
    CBehaviorSample_S stDiscuss = blockedOf(SampleReason_E::SR_REGION_MISS);
    if (stRow.bOnBoundary)
    {
        stDiscuss = blockedOf(SampleReason_E::SR_EDGE);
    }
    else if (nullptr == stIn.penRowSubject)
    {
        /* 没有逐行归属这一列就分不清邻座与老师：学生转身问老师会被判成讨论，
           而那条账在互动 209 与举手 102 上。这里宁可整列判不出 */
        stDiscuss = blockedOf(SampleReason_E::SR_SUBJECT);
    }
    else
    {
        const float fMaxGap = stSnap.stGroup.fNeighborMaxHead * fHeadWidth;
        bool bAnyCandidate = false;
        bool bAnyUnknown = false;
        CBehaviorSample_S stLastUnknown = blockedOf(SampleReason_E::SR_KEY_LOW);
        stDiscuss = sampleOf(TriState_E::TS_FALSE);   /* 有候选、都判得出、没人被朝向 → 不成立 */
        /* 三重上界 */
        int32_t nScan = (stFrame.nPersonNum < stIn.nRowSubjectNum) ? stFrame.nPersonNum
                                                                    : stIn.nRowSubjectNum;
        const int32_t nCap = static_cast<int32_t>(stFrame.vecPersons.size());
        if (nScan > nCap)
        {
            nScan = nCap;
        }
        for (int32_t i = 0; i < nScan; ++i)
        {
            if (i == stIn.nSelfIdx)
            {
                continue;
            }
            const CPersonFeature_S &stOther = stFrame.vecPersons[static_cast<size_t>(i)];
            if (stOther.nPersonId < 0 || Subject_E::SUBJECT_TEACHER == stIn.rowSubjectAt(i))
            {
                continue;
            }
            if (stOther.stHeadBox.area() <= 0.0f)
            {
                continue;
            }
            const CPointf_S stSelfC = stRow.stHeadBox.center();
            const CPointf_S stOtherC = stOther.stHeadBox.center();
            if (distance2d(stSelfC, stOtherC) > fMaxGap)
            {
                continue;   /* 不在"转头够得着"的范围里，不是这一条判据的邻座 */
            }
            bAnyCandidate = true;
            const float fSign = refSignOf(stOtherC.fX, stSelfC.fX, fHeadWidth,
                                          stSnap.stGroup.fRefDeadbandHead);
            const CBehaviorSample_S stOne = dirSampleOf(stGeom, stSnap.stGroup.stDiscussDir, fSign);
            if (TriState_E::TS_TRUE == stOne.enState)
            {
                stDiscuss = stOne;
                break;
            }
            if (TriState_E::TS_UNKNOWN == stOne.enState)
            {
                bAnyUnknown = true;
                stLastUnknown = stOne;
            }
        }
        if (!bAnyCandidate)
        {
            stDiscuss = blockedOf(SampleReason_E::SR_REGION_MISS);   /* 这帧里没有邻座可参照 */
        }
        else if (bAnyUnknown && TriState_E::TS_TRUE != stDiscuss.enState)
        {
            stDiscuss = stLastUnknown;
        }
    }

    // 上台演示 = 站在讲台区域 且 站立 且 面向班级
    const CBehaviorSample_S stPresentDir = stRow.bOnBoundary
                                               ? blockedOf(SampleReason_E::SR_EDGE)
                                               : dirSampleOf(stGeom, stSnap.stGroup.stPresentDir, fClassSign);
    CBehaviorSample_S stPresent = andOf(stPodium, stPresentDir);
    if (TriState_E::TS_FALSE == stStand.enState)
    {
        stPresent.enState = TriState_E::TS_FALSE;
        stPresent.enReason = SampleReason_E::SR_NO_POSE;
    }

    /* ----  表情识别 112：读感知层已经算稳的那一列，本列不设第二道门 ---- */
    CBehaviorSample_S stExpr;
    /* 新鲜度用**事件 TTL**而不是平滑行 */
    const bool bExprFresh = (0 != stRow.llExprMs) &&
                            (stIn.llNowMs - stRow.llExprMs <= stSnap.stExpr.nEventTtlMs);
    if (!bExprFresh || Expression_E::EXP_UNKNOWN == stRow.enExpression)
    {
        stExpr = blockedOf(SampleReason_E::SR_NO_MODEL);
    }
    else if (Expression_E::EXP_NEUTRAL == stRow.enExpression)
    {
        stExpr = sampleOf(TriState_E::TS_FALSE);
    }
    else
    {
        stExpr = sampleOf(TriState_E::TS_TRUE);
        /* 类别随 TRUE 采样进门：begin 报开门类别，end 报最后维持的类别，
           平台与逐事件核验从此分得清「出现了哪种表情」 */
        stExpr.nAux = static_cast<int8_t>(stRow.enExpression);
    }

    rvOut[SC_STAND]      = stStand;
    rvOut[SC_RAISE_HAND] = stRaise;
    rvOut[SC_HEAD_UP]    = stHeadUp;
    rvOut[SC_HEAD_DOWN]  = stHeadDown;
    rvOut[SC_TURN_BACK]  = stTurn;
    rvOut[SC_PHONE]      = stPhone;
    rvOut[SC_LISTEN]     = stListen;
    rvOut[SC_READING]    = stReading;
    rvOut[SC_DISCUSS]    = stDiscuss;
    rvOut[SC_PODIUM]     = stPodium;
    rvOut[SC_PRESENT]    = stPresent;
    rvOut[SC_EXPRESSION] = stExpr;
}
}
