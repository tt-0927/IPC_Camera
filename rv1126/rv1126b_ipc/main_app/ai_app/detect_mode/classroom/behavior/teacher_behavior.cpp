/**
 * @FilePath     : teacher_behavior.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 15:02:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-24 09:05:00
 * @Description  : 教师行为采样器实现（纯算术，无分配、无锁、无日志）
 */

#include "teacher_behavior.h"

#include "classroom_config.h"
#include "geometry.h"

namespace Classroom
{
namespace
{
/* 声明表：下标即 TeacherColumn_E，同学生路——编译期把「表序与列号是否一致」钉死。
   缺的两列（205 玩手机、213 教师身份）刻意不在这里：它们分属 P6/P5，
   本路声明了却给不出判定，就是「配了、开了、永远零事件」那条陷阱的复现 */
constexpr Behavior_E astBehavior[TC_NUM] = {
    Behavior_E::TEA_STAND_UP,
    Behavior_E::TEA_HEAD_UP,
    Behavior_E::TEA_HEAD_DOWN,
    Behavior_E::TEA_TURN_BACK,
    Behavior_E::TEA_TEACHING,
    Behavior_E::TEA_BOARD_WRITE,
    Behavior_E::TEA_ENTER_PODIUM,
    Behavior_E::TEA_TRACK,
    Behavior_E::TEA_PHONE,
};

static_assert(Behavior_E::TEA_STAND_UP == astBehavior[TC_STAND], "teacher sampler table order != TC_STAND");
static_assert(Behavior_E::TEA_HEAD_UP == astBehavior[TC_HEAD_UP], "teacher sampler table order != TC_HEAD_UP");
static_assert(Behavior_E::TEA_HEAD_DOWN == astBehavior[TC_HEAD_DOWN], "teacher sampler table order != TC_HEAD_DOWN");
static_assert(Behavior_E::TEA_TURN_BACK == astBehavior[TC_TURN_BACK], "teacher sampler table order != TC_TURN_BACK");
static_assert(Behavior_E::TEA_TEACHING == astBehavior[TC_TEACHING], "teacher sampler table order != TC_TEACHING");
static_assert(Behavior_E::TEA_BOARD_WRITE == astBehavior[TC_BOARD_WRITE], "teacher sampler table order != TC_BOARD_WRITE");
static_assert(Behavior_E::TEA_ENTER_PODIUM == astBehavior[TC_ENTER_PODIUM], "teacher sampler table order != TC_ENTER_PODIUM");
static_assert(Behavior_E::TEA_TRACK == astBehavior[TC_TRACK], "teacher sampler table order != TC_TRACK");
static_assert(Behavior_E::TEA_PHONE == astBehavior[TC_PHONE], "teacher sampler table order != TC_PHONE");

void setAll(std::vector<CBehaviorSample_S> &rvOut, TriState_E enState, SampleReason_E enReason)
{
    for (CBehaviorSample_S &stSample : rvOut)
    {
        stSample.enState = enState;
        stSample.enReason = enReason;
    }
}

/* 教师三条"无概率输出"事件（206 讲授 / 208 板书 / 209 互动）的置信度门 */
bool teachScoreLow(const CGeometryInput &stInput, float fMinScore)
{
    if (!(fMinScore > 0.0f))
    {
        return false;
    }
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if ((nullptr == pstSkeleton) || !pstSkeleton->bValid)
    {
        return false;
    }
    return pstSkeleton->fScore < fMinScore;
}

/* 判据给什么就记什么：UNKNOWN 的成因由调用方给（复合判据知道自己是被哪一句挡住的） */
CBehaviorSample_S sampleOf(TriState_E enState, SampleReason_E enReasonIfUnknown = SampleReason_E::SR_KEY_LOW)
{
    CBehaviorSample_S stSample;
    stSample.enState = enState;
    stSample.enReason = (TriState_E::TS_UNKNOWN == enState) ? enReasonIfUnknown : SampleReason_E::SR_OK;
    return stSample;
}

CBehaviorSample_S blockedOf(SampleReason_E enReason)
{
    CBehaviorSample_S stSample;
    stSample.enState = TriState_E::TS_UNKNOWN;
    stSample.enReason = enReason;
    return stSample;
}

/* 被同族判据压掉：这一项是真的不成立，不是判不出来（与学生路同一个口径） */
CBehaviorSample_S inhibitedOf()
{
    CBehaviorSample_S stSample;
    stSample.enState = TriState_E::TS_FALSE;
    stSample.enReason = SampleReason_E::SR_INHIBIT;
    return stSample;
}

/* 三态取反：朝向判据答的是「背不背身」，而讲授要的是「正不正对学生」。
   UNKNOWN 不能顺手当成 TRUE——那是把「看不出他朝哪」写成「他正对着学生」 */
TriState_E triNot(TriState_E enState)
{
    if (TriState_E::TS_TRUE == enState)
    {
        return TriState_E::TS_FALSE;
    }
    if (TriState_E::TS_FALSE == enState)
    {
        return TriState_E::TS_TRUE;
    }
    return TriState_E::TS_UNKNOWN;
}

/**
 * @brief   : 复合判据的三态合取器
 */
class CTriAnd
{
public:
    void add(TriState_E enOne, SampleReason_E enReasonIfUnknown)
    {
        if (TriState_E::TS_FALSE == enOne)
        {
            m_enState = TriState_E::TS_FALSE;   /* 硬否：证据齐但不成立，成因是 ok */
            return;
        }
        if (TriState_E::TS_FALSE == m_enState)
        {
            return;   /* 已经否了，后面的 UNKNOWN 不再改变结论 */
        }
        if (TriState_E::TS_UNKNOWN == enOne && TriState_E::TS_TRUE == m_enState)
        {
            m_enState = TriState_E::TS_UNKNOWN;
            m_enBlock = enReasonIfUnknown;
        }
    }
    void add(bool bOne)
    {
        add(bOne ? TriState_E::TS_TRUE : TriState_E::TS_FALSE, SampleReason_E::SR_OK);
    }
    /**
     * @brief   : 前置条件不满足时整条判不出（UNKNOWN），而不是判成「没发生」
     */
    void require(bool bKnown, SampleReason_E enReasonIfMissing)
    {
        if (!bKnown && !m_bMissing)
        {
            m_bMissing = true;
            m_enMissing = enReasonIfMissing;
        }
    }
    CBehaviorSample_S sample() const
    {
        if (m_bMissing)
        {
            return blockedOf(m_enMissing);
        }
        return sampleOf(m_enState, m_enBlock);
    }

private:
    TriState_E     m_enState = TriState_E::TS_TRUE;
    SampleReason_E m_enBlock = SampleReason_E::SR_OK;
    bool           m_bMissing = false;
    SampleReason_E m_enMissing = SampleReason_E::SR_OK;
};
}   // namespace

CTeacherBehaviorSampler::CTeacherBehaviorSampler()
{
    m_vecBehavior.assign(astBehavior, astBehavior + TC_NUM);
}

void CTeacherBehaviorSampler::samplePerson(const CBehaviorSampleIn_S &stIn,
                                           std::vector<CBehaviorSample_S> &rvOut)
{
    /* 先整表铺 UNKNOWN：引擎复用同一张样本表，表里留着的是上一个人的判定（同学生路的理由） */
    setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_NO_FRAME);

    if (rvOut.size() != static_cast<size_t>(TC_NUM))
    {
        return;   /* 契约已被破坏，引擎那边会喊；这里绝不越界写 */
    }
    if (nullptr == stIn.pstFrame || nullptr == stIn.pstPerson || nullptr == stIn.pstSnap)
    {
        return;
    }

    const CFeatureFrame_S   &stFrame = *stIn.pstFrame;
    const CPersonFeature_S  &stRow   = *stIn.pstPerson;
    const CConfigSnapshot_S &stSnap  = *stIn.pstSnap;
    const CSubjectView_S    &stView  = stIn.stSubjectView;

    /* ---- 1) 归属闸：本路只管教师 ----
       与学生路相反，这里要求**明确成立**：UNKNOWN 主体不出教师行为（confirm_ms 之内不出是对的），
       帧内那一列只作兜底读法（见 CBehaviorSampleIn_S::pstSubject 的注释） */
    const Subject_E enSubject = (nullptr != stIn.pstSubject) ? stIn.pstSubject->enSubject : stRow.enSubject;
    if (Subject_E::SUBJECT_TEACHER != enSubject)
    {
        setAll(rvOut, TriState_E::TS_UNKNOWN, SampleReason_E::SR_SUBJECT);
        return;
    }

    /* 归属事实缺位时只挡住"要吃区域与位移"的那几列：位置类判据没有事实可退，
       而姿态与跟踪两列的证据并不来自归属。用一份全 false 的空事实顶位、逐列 require，
       比给这六列各写一遍 UNKNOWN 分支少五处会各自烂掉的地方 */
    const CSubjectFact_S stNoFact;
    const CSubjectFact_S &stFact = (nullptr != stIn.pstSubject) ? *stIn.pstSubject : stNoFact;
    const bool bFactKnown = (nullptr != stIn.pstSubject);

    /* ---- 2) 姿态证据档：只挡需要骨骼的那几列 ----
       闸的内容与学生路逐条相同（驻留、新鲜度、遮挡、清晰度），差别只在**不整路返回**：
       位置类三列不吃骨骼，被姿态模型的调度状态连坐会把面板指标打成锯齿 */
    const int32_t nPoseTtlMs = stFrame.poseTtlMs();   /* 帧上自带的实测节拍，理由见学生路 */
    SampleReason_E enPoseBlock = SampleReason_E::SR_OK;
    if (!stFrame.bPoseResident || !stRow.poseUsable(stIn.llNowMs, nPoseTtlMs))
    {
        enPoseBlock = SampleReason_E::SR_NO_POSE;
    }
    else if (stRow.bOccluded)
    {
        enPoseBlock = SampleReason_E::SR_OCCLUDED;
    }
    else if (stRow.fBlurScore > 0.0f &&
             stRow.fBlurScore < poseBlurMinForHeadPx(stSnap.fPoseBlurMin, stRow.stHeadBox.height()))
    {
        /* fBlurScore==0 是「没量出来」而不是「糊到零梯度」，拿 0 当糊等于用一个测不到的量
           去否决测得到的量（学生路文件头同一条）；门按头高缩放（#91），与学生路同一把尺 */
        enPoseBlock = SampleReason_E::SR_BLUR;
    }

    /* ---- 3) 四列直接判据 ----
       贴边只挡「证据会被切出画面」的站立与背身，抬头/低头不挡：那两条判据自己会因
       面部点不足返回 UNKNOWN，再挡一道只是白丢「额头还露着」那部分证据（与学生路一致） */
    CGeometryInput stInput;
    stInput.pstSkeleton    = &stRow.stSkeleton;
    stInput.stHeadBox      = stRow.stHeadBox;
    stInput.stBodyBox      = stRow.stBodyBox;
    stInput.fShoulderWidth = shoulderWidthOf(stRow.stSkeleton, stSnap.fPoseScoreThreshold);
    /* 3b 的玩手机 205 与学生 106 同形：带和间距按肩宽归一（judgePhoneUse 见 0 直接判不出）。
       抬头/低头不看这一列，填上它对那两路零影响 */

    CBehaviorSample_S stStand = blockedOf(enPoseBlock);
    CBehaviorSample_S stTurn  = blockedOf(enPoseBlock);
    CBehaviorSample_S stUp    = blockedOf(enPoseBlock);
    CBehaviorSample_S stDown  = blockedOf(enPoseBlock);

    if (SampleReason_E::SR_OK == enPoseBlock)
    {
        stStand = stRow.bOnBoundary ? blockedOf(SampleReason_E::SR_EDGE)
                                    : sampleOf(judgeStanding(stInput, stSnap.stStand));
        stTurn  = stRow.bOnBoundary ? blockedOf(SampleReason_E::SR_EDGE)
                                    : sampleOf(judgeTurnBack(stInput, stSnap.stTurnBack));
        /* 人脸证人：面点塌陷这一路分不出「背身」与「3/4 侧脸」 */
        if (TriState_E::TS_TRUE == stTurn.enState && stSnap.stTurnBack.fFaceWitnessTtlMs > 0.0f &&
            !turnbackFaceMirrored(stInput, stSnap.stTurnBack.fFaceWitnessMirrorRx) &&
            stRow.faceUsable(stIn.llNowMs, static_cast<int64_t>(stSnap.stTurnBack.fFaceWitnessTtlMs)))
        {
            stTurn.enState  = TriState_E::TS_FALSE;
            stTurn.enReason = SampleReason_E::SR_MODEL_NEG;
        }
        stUp    = sampleOf(judgeHeadUp(stInput, stSnap.stHeadUp));
        stDown  = sampleOf(judgeHeadDown(stInput, stSnap.stHeadDown));

        if (TriState_E::TS_TRUE == stDown.enState && TriState_E::TS_TRUE == stUp.enState)
        {
            /* 两条同时为真在几何上不共存（关键点在抖），留低头压抬头：与学生路同一取舍 */
            stUp = inhibitedOf();
        }
    }

    /* ---- 玩手机 205：与学生 106 同一个形状（低头 × 手在身前），复核三态同一套出口 ---- */
    CBehaviorSample_S stPhone = blockedOf(enPoseBlock);
    if (stRow.bOnBoundary)
    {
        stPhone = blockedOf(SampleReason_E::SR_EDGE);   /* 腕与肘的证据会被画面边切走一半 */
    }
    else if (TriState_E::TS_TRUE != stDown.enState)
    {
        stPhone.enState  = stDown.enState;
        stPhone.enReason = stDown.enReason;
    }
    else
    {
        stPhone = sampleOf(judgePhoneUse(stInput, stSnap.stPhone));
    }
    if (TriState_E::TS_TRUE == stPhone.enState)
    {
        /* 复核结论可用只看时间戳：phoneUsable 刻意不要求 bPhoneObj（feature_frame.h 那条） */
        const bool bRecheckKnown = stRow.phoneUsable(stIn.llNowMs, stSnap.stPhoneRecheck.nTtlMs);
        if (bRecheckKnown && !stRow.bPhoneObj)
        {
            stPhone.enState  = TriState_E::TS_FALSE;
            stPhone.enReason = SampleReason_E::SR_MODEL_NEG;   /* 「看了，没有」 */
        }
        else if (!bRecheckKnown)
        {
            stPhone.enReason = SampleReason_E::SR_FAIL_OPEN;
        }
    }

    /* ---- 4) 区域类复合列 ----
       归属事实缺位的成因记 SR_SUBJECT 而不是新加一类枚举：诊断出的下一步是同一件事——
       去看归属那一段（心跳 subj= / statusJson 的 subject）缺了什么 */

    /* 206 讲授 = 在讲台/黑板区域 + 非背身  */
    const bool bTeachScoreLow = teachScoreLow(stInput, stSnap.fTeachKeyScore);
    CTriAnd stTeaching;
    stTeaching.require(bFactKnown, SampleReason_E::SR_SUBJECT);
    stTeaching.require(stView.bTeacherAreaKnown, SampleReason_E::SR_REGION_MISS);
    stTeaching.add(stFact.bInTeacherArea);
    if (bTeachScoreLow)
    {
        stTeaching.add(TriState_E::TS_UNKNOWN, SampleReason_E::SR_KEY_LOW);
    }
    stTeaching.add(triNot(stTurn.enState), stTurn.enReason);

    /* 208 板书 = 在黑板区域 + 背身。黑板角色没画时是判不出，不是「他没板书」 */
    const bool bBoardScoreLow = teachScoreLow(stInput, stSnap.fBoardWriteKeyScore);
    CTriAnd stBoard;
    stBoard.require(bFactKnown, SampleReason_E::SR_SUBJECT);
    stBoard.require(stView.bBoardConfigured, SampleReason_E::SR_REGION_MISS);
    stBoard.add(stFact.bInBoard);
    if (bBoardScoreLow)
    {
        stBoard.add(TriState_E::TS_UNKNOWN, SampleReason_E::SR_KEY_LOW);
    }
    stBoard.add(stTurn.enState, stTurn.enReason);

    /* 211 进入讲台 = 人在讲台区域 */
    CTriAnd stEnter;
    stEnter.require(bFactKnown, SampleReason_E::SR_SUBJECT);
    stEnter.require(stView.bPodiumConfigured, SampleReason_E::SR_REGION_MISS);
    stEnter.add(stFact.bInPodium);

    rvOut[TC_STAND]        = stStand;
    rvOut[TC_HEAD_UP]      = stUp;
    rvOut[TC_HEAD_DOWN]    = stDown;
    rvOut[TC_TURN_BACK]    = stTurn;
    rvOut[TC_TEACHING]     = stTeaching.sample();
    rvOut[TC_BOARD_WRITE]  = stBoard.sample();
    rvOut[TC_ENTER_PODIUM] = stEnter.sample();
    rvOut[TC_PHONE]        = stPhone;

    /* 212 教师跟踪 = 归属成立期间持续在场 */
    rvOut[TC_TRACK]        = sampleOf(TriState_E::TS_TRUE);
}
}
