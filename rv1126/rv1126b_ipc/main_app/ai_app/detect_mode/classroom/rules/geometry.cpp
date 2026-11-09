/**
 * @FilePath     : geometry.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 10:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 10:10:00
 * @Description  : 几何判定纯函数实现
 */

#include "geometry.h"

#include <cmath>
#include <cstdio>

#define CLASSROOM_ANGLE_EPSILON (1e-6f)
#define CLASSROOM_DIST_EPSILON  (1e-6f)
#define CLASSROOM_RAD2DEG       (57.29577951308232f)

namespace Classroom
{
/* ---- T37 背身面部分数直方图（埋点例外：本文件其余函数保持无状态） ----
   judgeTurnBack 的面部证据在 50px 级头框上「正面讲课」与「真背身」的得分带互相骑门限
   （tt ea4 face 0.5：204 出 1 真 + 3 假；tt ea5 face 0.3：204 全灭、206 好转）——
   单一门限值在哪个位置都拆不开这两群。埋 8 桶 × 头框两带（<34px / ≥34px，34 取
   poseKeyScoreForHeadPx 满门带宽）× 两判（背身/可见）的 max 面点分分布，
   让下一次动门限（或换成证人方案）有实测分布可依，而不是第三次盲调 */
#define CLASSROOM_TB_FACE_BAND_PX (34.0f)
#define CLASSROOM_TB_FACE_BUCKETS (8)
struct TurnBackFaceStat_S
{
    /* [band*CLASSROOM_TB_FACE_BUCKETS + bucket]；bucket 按 max 面点分
       [0,.1)/[.1,.2)/[.2,.3)/[.3,.4)/[.4,.5)/[.5,.6)/[.6,.7)/[.7,1] */
    uint64_t au64Back[16] = {0};
    uint64_t au64Vis[16] = {0};
};
static TurnBackFaceStat_S &tbFaceStat()
{
    static TurnBackFaceStat_S s;
    return s;
}

void turnbackFaceStatText(char *pszBuf, int32_t nSize)
{
    if (nullptr == pszBuf || nSize <= 0)
    {
        return;
    }
    const TurnBackFaceStat_S &stStat = tbFaceStat();
    int32_t nOff = 0;
    pszBuf[0] = '\0';
    for (int32_t nGrp = 0; nGrp < 4; ++nGrp)
    {
        const uint64_t *pau = (0 == nGrp) ? stStat.au64Back
                              : (1 == nGrp) ? (stStat.au64Back + CLASSROOM_TB_FACE_BUCKETS)
                              : (2 == nGrp) ? stStat.au64Vis
                                            : (stStat.au64Vis + CLASSROOM_TB_FACE_BUCKETS);
        nOff += snprintf(pszBuf + nOff, static_cast<size_t>(nSize - nOff), "%s%llu",
                         (0 == nOff) ? "tbf=" : "/", static_cast<unsigned long long>(pau[0]));
        for (int32_t b = 1; b < CLASSROOM_TB_FACE_BUCKETS; ++b)
        {
            nOff += snprintf(pszBuf + nOff, static_cast<size_t>(nSize - nOff), ",%llu",
                             static_cast<unsigned long long>(pau[b]));
        }
        if (nOff >= nSize - 1)
        {
            break;
        }
    }
}

void turnbackFaceStatReset()
{
    tbFaceStat() = TurnBackFaceStat_S();
}

float distance2d(const CPointf_S &stA, const CPointf_S &stB)
{
    float fDx = stA.fX - stB.fX;
    float fDy = stA.fY - stB.fY;
    return std::sqrt(fDx * fDx + fDy * fDy);
}

CPointf_S perpendicularFoot(const CPointf_S &stA, const CPointf_S &stB, const CPointf_S &stP)
{
    float fABx = stB.fX - stA.fX;
    float fABy = stB.fY - stA.fY;
    float fLenSq = fABx * fABx + fABy * fABy;

    /* 双眼重合（侧面远焦时常见）时退化为 A 点本身，不返回 NaN */
    if (fLenSq < CLASSROOM_DIST_EPSILON)
    {
        return stA;
    }

    float fT = ((stP.fX - stA.fX) * fABx + (stP.fY - stA.fY) * fABy) / fLenSq;
    /* 眉心必须落在两眼之间，钳制到线段而非无限延长 */
    fT = clampValue(fT, 0.0f, 1.0f);

    return CPointf_S{ stA.fX + fT * fABx, stA.fY + fT * fABy };
}

float angleWithVertical(const CPointf_S &stA, const CPointf_S &stB)
{
    float fDx = std::fabs(stB.fX - stA.fX);
    float fDy = stB.fY - stA.fY;

    /* 两点重合：无方向可言，按"与竖直方向夹角 0"处理，交由调用方的 score 门限挡掉 */
    if (fDx < CLASSROOM_DIST_EPSILON && std::fabs(fDy) < CLASSROOM_DIST_EPSILON)
    {
        return 0.0f;
    }

    /* atan2 天然处理 fDy==0（水平线 → 90°），不需要原实现那种"除零靠 inf 侥幸"的写法 */
    float fAngle = std::atan2(fDx, std::fabs(fDy)) * CLASSROOM_RAD2DEG;
    return clampValue(fAngle, 0.0f, 90.0f);
}

float angleAtVertex(const CPointf_S &stPivot, const CPointf_S &stA, const CPointf_S &stB)
{
    float fAx = stA.fX - stPivot.fX;
    float fAy = stA.fY - stPivot.fY;
    float fBx = stB.fX - stPivot.fX;
    float fBy = stB.fY - stPivot.fY;

    float fLenA = std::sqrt(fAx * fAx + fAy * fAy);
    float fLenB = std::sqrt(fBx * fBx + fBy * fBy);
    if (fLenA < CLASSROOM_DIST_EPSILON || fLenB < CLASSROOM_DIST_EPSILON)
    {
        return 0.0f;
    }

    float fCos = (fAx * fBx + fAy * fBy) / (fLenA * fLenB);
    /* 必须钳制：acos 定义域外会返回 NaN，NaN 参与比较恒为 false，表现为"行为忽有好忽无"的抖动 */
    fCos = clampValue(fCos, -1.0f, 1.0f);

    return std::acos(fCos) * CLASSROOM_RAD2DEG;
}

bool posePoint(const CPoseSkeleton_S *pstSkeleton, PosePoint_E enPoint, CPointf_S &stOut, float &rfScore)
{
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return false;
    }

    int nIdx = static_cast<int>(enPoint);
    if (nIdx < 0 || nIdx >= CLASSROOM_POSE_KEYPOINT_NUM)
    {
        return false;
    }

    const CPosePoint_S &stPoint = pstSkeleton->astPoints[nIdx];
    stOut.fX = stPoint.fX;
    stOut.fY = stPoint.fY;
    rfScore = stPoint.fScore;
    return true;
}

bool posePointUsable(const CPoseSkeleton_S *pstSkeleton, PosePoint_E enPoint, float fScoreThreshold)
{
    CPointf_S stPoint;
    float fScore = 0.0f;
    if (!posePoint(pstSkeleton, enPoint, stPoint, fScore))
    {
        return false;
    }
    return fScore >= fScoreThreshold;
}

float shoulderWidthOf(const CPoseSkeleton_S &stSkeleton, float fScoreThreshold)
{
    CPointf_S stLeft;
    CPointf_S stRight;
    float fLeftScore = 0.0f;
    float fRightScore = 0.0f;

    if (!posePoint(&stSkeleton, PosePoint_E::POSE_L_SHOULDER, stLeft, fLeftScore) ||
        !posePoint(&stSkeleton, PosePoint_E::POSE_R_SHOULDER, stRight, fRightScore))
    {
        return 0.0f;
    }
    if (fLeftScore < fScoreThreshold || fRightScore < fScoreThreshold)
    {
        return 0.0f;
    }
    return distance2d(stLeft, stRight);
}

TriState_E judgeStanding(const CGeometryInput &stInput, const StandRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* #91 远带折扣：头框小的行（远排）分数系统性偏低，门按头高下放；
       框没画出来（高 0）时不折——量不到尺度就维持原门，比凭空放门保守 */
    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());

    CPointf_S stLHip, stRHip, stLKnee, stRKnee;
    float fLHip = 0.0f, fRHip = 0.0f, fLKnee = 0.0f, fRKnee = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_L_HIP, stLHip, fLHip);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_HIP, stRHip, fRHip);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_KNEE, stLKnee, fLKnee);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_KNEE, stRKnee, fRKnee);

    /* 双腿关键点全部不可用时才是"未知"；只要有一条腿可用就能判，判不出即为否 */
    bool bLeftLegUsable = (fLHip >= fGate && fLKnee >= fGate);
    bool bRightLegUsable = (fRHip >= fGate && fRKnee >= fGate);
    if (!bLeftLegUsable && !bRightLegUsable)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 髋高于膝（图像坐标 y 向下）且小腿/大腿近似竖直，才可能是站立 */
    bool bLegVertical = false;
    if (bLeftLegUsable && stLHip.fY < stLKnee.fY &&
        angleWithVertical(stLKnee, stLHip) < stRule.fLegVerticalAngle)
    {
        bLegVertical = true;
    }
    if (bRightLegUsable && stRHip.fY < stRKnee.fY &&
        angleWithVertical(stRKnee, stRHip) < stRule.fLegVerticalAngle)
    {
        bLegVertical = true;
    }
    if (!bLegVertical)
    {
        return TriState_E::TS_FALSE;
    }

    /* 在网约定的第二条：必须有一只手自然下垂且手臂伸直，用以排除"坐着但腿被桌子挡住"的假站立 */
    CPointf_S stLShoulder, stLElbow, stLWrist, stRShoulder, stRElbow, stRWrist;
    float fLSh = 0.0f, fLEl = 0.0f, fLWr = 0.0f, fRSh = 0.0f, fREl = 0.0f, fRWr = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_L_SHOULDER, stLShoulder, fLSh);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_ELBOW, stLElbow, fLEl);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_WRIST, stLWrist, fLWr);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_SHOULDER, stRShoulder, fRSh);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_ELBOW, stRElbow, fREl);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_WRIST, stRWrist, fRWr);

    bool bLeftArmStraight = (fLSh >= fGate && fLEl >= fGate && fLWr >= fGate &&
                             stLWrist.fY > stLElbow.fY &&
                             angleAtVertex(stLElbow, stLShoulder, stLWrist) > stRule.fArmStraightAngle);
    bool bRightArmStraight = (fRSh >= fGate && fREl >= fGate && fRWr >= fGate &&
                              stRWrist.fY > stRElbow.fY &&
                              angleAtVertex(stRElbow, stRShoulder, stRWrist) > stRule.fArmStraightAngle);

    if (bLeftArmStraight || bRightArmStraight)
    {
        /* 髋必须真的在桌面之上。
           量不到肩宽/颈/髋就不给"是"——与下面"手臂看不到不给是也不给否"同一条口径。 */
        if (stRule.fMinTorsoShoulder > 0.0f)
        {
            const float fShoulderW = shoulderWidthOf(*pstSkeleton, fGate);
            float fHipSumX = 0.0f;
            float fHipSumY = 0.0f;
            int nHipUsable = 0;
            if (fLHip >= fGate)
            {
                fHipSumX += stLHip.fX;
                fHipSumY += stLHip.fY;
                ++nHipUsable;
            }
            if (fRHip >= fGate)
            {
                fHipSumX += stRHip.fX;
                fHipSumY += stRHip.fY;
                ++nHipUsable;
            }
            CPointf_S stNeck;
            float fNeckScore = 0.0f;
            /* 量不到肩宽/颈/髋就不否决——这是修复前的口径。
               判成 UNKNOWN 会把 s1 的真站立砍掉约一半（stand_person_s 82/145 → 62/31，
               dlog 的 101 起数 6~8 → 3~4），而 s3 的误报全部量得到（比值 0.82~1.20）。
               宁可放过量不到的少数，也不误杀真站立。 */
            if (fShoulderW > 1.0f && nHipUsable > 0 &&
                posePoint(pstSkeleton, PosePoint_E::POSE_NECK, stNeck, fNeckScore) &&
                fNeckScore >= fGate)
            {
                CPointf_S stHipMid;
                stHipMid.fX = fHipSumX / static_cast<float>(nHipUsable);
                stHipMid.fY = fHipSumY / static_cast<float>(nHipUsable);
                if (distance2d(stNeck, stHipMid) < fShoulderW * stRule.fMinTorsoShoulder)
                {
                    return TriState_E::TS_FALSE;
                }
            }
        }
        return TriState_E::TS_TRUE;
    }
    /* 腿站直但手臂看不到（被前排遮挡）时不给"是"，也不给"否"——交时间平滑决定 */
    if (fLSh < fGate && fRSh < fGate)
    {
        return TriState_E::TS_UNKNOWN;
    }
    return TriState_E::TS_FALSE;
}

TriState_E judgeRaiseHand(const CGeometryInput &stInput, const RaiseHandRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    CPointf_S stHeadTop;
    float fHeadScore = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_HEAD_TOP, stHeadTop, fHeadScore);

    CPointf_S stElbow[2];
    CPointf_S stWrist[2];
    float fElbowScore[2] = { 0.0f, 0.0f };
    float fWristScore[2] = { 0.0f, 0.0f };

    posePoint(pstSkeleton, PosePoint_E::POSE_L_ELBOW, stElbow[0], fElbowScore[0]);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_WRIST, stWrist[0], fWristScore[0]);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_ELBOW, stElbow[1], fElbowScore[1]);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_WRIST, stWrist[1], fWristScore[1]);

    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());

    /* 「举手有效」门：先数有几只手**完整过闸**，再按模式判。
       旧实现是"任一只手过就返回 TRUE"，于是网页的「单手/双手/单双手」三选一没有落点。
       计数与判据分开是因为三种模式问的是同一个量的不同阈值：
       过闸手数 ≥1 ⇒ 单/双手；≥2 ⇒ 双手；==1 ⇒ 单手；
       0 ⇒ 没举手（但看见了手就报 FALSE，没看见手报 UNKNOWN）。 */
    int32_t nRaised = 0;
    bool bAnyArmVisible = false;
    for (int i = 0; i < 2; ++i)
    {
        if (fElbowScore[i] < fGate || fWristScore[i] < fGate)
        {
            continue;
        }
        bAnyArmVisible = true;

        /* 腕高于肘 */
        if (stElbow[i].fY <= stWrist[i].fY)
        {
            continue;
        }
        if (angleWithVertical(stElbow[i], stWrist[i]) >= stRule.fVerticalAngle)
        {
            continue;
        }
        /* 手腕需高出头顶一定肩宽比例，抑制"摸头/挠头"误判 */
        if (stRule.fMinWristAboveHead > 0.0f && stInput.fShoulderWidth > CLASSROOM_ANGLE_EPSILON)
        {
            float fRise = (stHeadTop.fY - stWrist[i].fY) / stInput.fShoulderWidth;
            if (fRise < stRule.fMinWristAboveHead)
            {
                continue;
            }
        }
        ++nRaised;
    }

    if (nRaised >= 2)
    {
        /* 双手举起。nHandMode==0（仅单手）时这一档**不算**举手：
           现场要的是"只举了一只手"的答题姿态，双手齐举通常是擦黑板/举手加挥舞 */
        return (0 == stRule.nHandMode) ? TriState_E::TS_FALSE : TriState_E::TS_TRUE;
    }
    if (1 == nRaised)
    {
        /* 恰好一只手。nHandMode==1（仅双手）时不算 */
        return (1 == stRule.nHandMode) ? TriState_E::TS_FALSE : TriState_E::TS_TRUE;
    }
    return bAnyArmVisible ? TriState_E::TS_FALSE : TriState_E::TS_UNKNOWN;
}

TriState_E judgeHeadDown(const CGeometryInput &stInput, const HeadDownRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    CPointf_S stNose, stREye, stLEye, stNeck;
    float fNose = 0.0f, fREye = 0.0f, fLEye = 0.0f, fNeck = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_NOSE, stNose, fNose);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_EYE, stREye, fREye);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_EYE, stLEye, fLEye);
    posePoint(pstSkeleton, PosePoint_E::POSE_NECK, stNeck, fNeck);

    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());
    if (fNose < fGate || fNeck < fGate ||
        fREye < fGate || fLEye < fGate)
    {
        return TriState_E::TS_UNKNOWN;
    }
    /* #91 尺度可信门：双眼挤在 6px 内，眉心垂足与比值都在读数误差里打转——
       这不是"没低头"，是"角度读不出来"，必须 UNKNOWN 让时长门当没证据处理 */
    if (distance2d(stREye, stLEye) < CLASSROOM_POSE_MIN_EYE_GAP_PX)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 眉心 = 鼻子到双眼连线的垂足；低头时鼻尖向颈部靠拢，故该比值增大 */
    CPointf_S stBrow = perpendicularFoot(stREye, stLEye, stNose);
    float fDenominator = distance2d(stBrow, stNeck);
    if (fDenominator < CLASSROOM_DIST_EPSILON)
    {
        return TriState_E::TS_UNKNOWN;
    }

    float fRatio = distance2d(stBrow, stNose) / fDenominator;
    return (fRatio > stRule.fRatio) ? TriState_E::TS_TRUE : TriState_E::TS_FALSE;
}

TriState_E judgeHeadUp(const CGeometryInput &stInput, const HeadUpRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    CPointf_S stNose, stREye, stLEye;
    float fNose = 0.0f, fREye = 0.0f, fLEye = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_NOSE, stNose, fNose);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_EYE, stREye, fREye);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_EYE, stLEye, fLEye);

    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());
    if (fNose < fGate || fREye < fGate || fLEye < fGate)
    {
        return TriState_E::TS_UNKNOWN;
    }
    /* #91 尺度可信门：双眼距小于数个像素时夹角被读数误差淹没，出 UNKNOWN 而不是噪声答案
       （与低头判据同一道门；远排小脸的 103/104 由此归"没证据"，不再输出抖动结果） */
    if (distance2d(stREye, stLEye) < CLASSROOM_POSE_MIN_EYE_GAP_PX)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 以鼻尖为顶点看双眼：面部越朝上，双眼连线相对鼻尖越"平"，夹角越大 */
    float fAngle = angleAtVertex(stNose, stREye, stLEye);
    return (fAngle > stRule.fAngle) ? TriState_E::TS_TRUE : TriState_E::TS_FALSE;
}

/* T45 镜像签名：姿态双眼的「交叉布局」（眼Lx 落框右半、眼Rx 落框左半）是真实人脸摆不出的
   布局——tt16/tt18/tt19 三跑 5454 探针行上真背身 36/36、真正面 315/315 互斥（正面全部
   >0.55，0.45~0.55 空带）。fSplit<=0（关位）、眼点缺失（分 0）、头框退化时一律给 false：
   指纹必须证出来，不许默认疑罪。双眼同侧（真侧脸）永不为 true——那是人脸证人的本职场景 */
bool turnbackFaceMirrored(const CGeometryInput &stInput, float fSplit)
{
    if (fSplit <= 0.0f || nullptr == stInput.pstSkeleton || !stInput.pstSkeleton->bValid)
    {
        return false;
    }
    CPointf_S stLEye, stREye;
    float fLEye = 0.0f, fREye = 0.0f;
    if (!posePoint(stInput.pstSkeleton, PosePoint_E::POSE_L_EYE, stLEye, fLEye) ||
        !posePoint(stInput.pstSkeleton, PosePoint_E::POSE_R_EYE, stREye, fREye) ||
        fLEye <= 0.0f || fREye <= 0.0f)
    {
        return false;
    }
    const float fBoxW = stInput.stHeadBox.width();
    if (fBoxW <= 1.0f)
    {
        return false;
    }
    const float fLx = (stLEye.fX - stInput.stHeadBox.fX1) / fBoxW;
    const float fRx = (stREye.fX - stInput.stHeadBox.fX1) / fBoxW;
    return (fLx > fSplit && fRx < fSplit);
}

TriState_E judgeTurnBack(const CGeometryInput &stInput, const TurnBackRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 身体证据：双肩。头顶不参与——背身时后脑勺/长发让头顶分在 0.38~0.83 之间晃
       （写板教师鼻 0.03~0.19、头顶 0.38~0.83、整骨分 0.71），
       把「头在这」押在头顶分上，写板段就整段死在体门（KEY_LOW 心跳 44 条 vs TRUE 3 条）。
       「这里有个头」本来就是本行的前提：行存在即 HeadDetect 在跟一个头框，骨骼是关联上来的；
       体门的职责只是拒绝「骨骼没长在人身上」的行，双肩是背身视角下最稳的躯干证据。
       两道门同吃 #91 折扣：远排面部点塌陷与低分是常态，不折会把远带整排判成背身 */
    CPointf_S stLShoulder, stRShoulder;
    float fLSh = 0.0f, fRSh = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_L_SHOULDER, stLShoulder, fLSh);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_SHOULDER, stRShoulder, fRSh);

    const float fBodyGate = poseKeyScoreForHeadPx(stRule.fBodyScore, stInput.stHeadBox.height());
    if (fLSh < fBodyGate || fRSh < fBodyGate)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 面部证据：鼻 + 双眼。背对镜头时模型看到的是后脑勺，这些点得分会集体塌陷 */
    CPointf_S stDummy;
    float afFaceScore[3] = { 0.0f, 0.0f, 0.0f };
    const PosePoint_E astFacePoints[3] = {
        PosePoint_E::POSE_NOSE, PosePoint_E::POSE_L_EYE, PosePoint_E::POSE_R_EYE
    };
    bool bAnyFaceVisible = false;
    const float fFaceGate = poseKeyScoreForHeadPx(stRule.fFaceScore, stInput.stHeadBox.height());
    for (int i = 0; i < 3; ++i)
    {
        if (!posePoint(pstSkeleton, astFacePoints[i], stDummy, afFaceScore[i]))
        {
            return TriState_E::TS_UNKNOWN;
        }
        if (afFaceScore[i] > fFaceGate)
        {
            bAnyFaceVisible = true;
        }
    }

    /* T37 埋点：只记数，不影响判据。带按头框高（34px=满门带宽），桶按 max 面点分 */
    {
        float fMaxFace = afFaceScore[0];
        if (afFaceScore[1] > fMaxFace)
        {
            fMaxFace = afFaceScore[1];
        }
        if (afFaceScore[2] > fMaxFace)
        {
            fMaxFace = afFaceScore[2];
        }
        int32_t nBucket = (fMaxFace >= 0.7f) ? 7 : static_cast<int32_t>(fMaxFace * 10.0f);
        if (nBucket < 0)
        {
            nBucket = 0;
        }
        const int32_t nIdx = ((stInput.stHeadBox.height() >= CLASSROOM_TB_FACE_BAND_PX) ? 1 : 0) *
                                 CLASSROOM_TB_FACE_BUCKETS +
                             nBucket;
        if (bAnyFaceVisible)
        {
            ++tbFaceStat().au64Vis[nIdx];
        }
        else
        {
            ++tbFaceStat().au64Back[nIdx];
        }
    }

    return bAnyFaceVisible ? TriState_E::TS_FALSE : TriState_E::TS_TRUE;
}

TriState_E judgeLieOnDesk(const CGeometryInput &stInput, const LieOnDeskRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    CPointf_S stHeadTop, stLShoulder, stRShoulder;
    float fHead = 0.0f, fLSh = 0.0f, fRSh = 0.0f;
    posePoint(pstSkeleton, PosePoint_E::POSE_HEAD_TOP, stHeadTop, fHead);
    posePoint(pstSkeleton, PosePoint_E::POSE_L_SHOULDER, stLShoulder, fLSh);
    posePoint(pstSkeleton, PosePoint_E::POSE_R_SHOULDER, stRShoulder, fRSh);

    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());
    if (fHead < fGate || fLSh < fGate || fRSh < fGate)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 头顶不高于较高一侧肩膀（y 更小为更高）→ 趴桌 */
    float fMinShoulderY = (stLShoulder.fY < stRShoulder.fY) ? stLShoulder.fY : stRShoulder.fY;
    return (stHeadTop.fY >= fMinShoulderY) ? TriState_E::TS_TRUE : TriState_E::TS_FALSE;
}

TriState_E judgePhoneUse(const CGeometryInput &stInput, const PhoneRule_S &stRule)
{
    const CPoseSkeleton_S *pstSkeleton = stInput.pstSkeleton;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 带与间距都以肩宽归一，故双肩必须同时可用：少了任一侧就没有这把尺子，
       而 fShoulderWidth 为 0 时拿它去除会得到 inf，判据会在边界上恒真 */
    CPointf_S stShoulder[2];
    float fShoulderScore[2] = { 0.0f, 0.0f };
    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());
    if (!posePoint(pstSkeleton, PosePoint_E::POSE_L_SHOULDER, stShoulder[0], fShoulderScore[0]) ||
        !posePoint(pstSkeleton, PosePoint_E::POSE_R_SHOULDER, stShoulder[1], fShoulderScore[1]) ||
        fShoulderScore[0] < fGate || fShoulderScore[1] < fGate ||
        stInput.fShoulderWidth <= CLASSROOM_DIST_EPSILON)
    {
        return TriState_E::TS_UNKNOWN;
    }

    const float fAnchorY = (stShoulder[0].fY + stShoulder[1].fY) * 0.5f;

    CPointf_S stQualifyWrist[2];
    int nQualify = 0;
    bool bAnyArmVisible = false;

    const PosePoint_E aenElbow[2] = { PosePoint_E::POSE_L_ELBOW, PosePoint_E::POSE_R_ELBOW };
    const PosePoint_E aenWrist[2] = { PosePoint_E::POSE_L_WRIST, PosePoint_E::POSE_R_WRIST };

    for (int i = 0; i < 2; ++i)
    {
        CPointf_S stElbow, stWrist;
        float fElbowScore = 0.0f, fWristScore = 0.0f;
        if (!posePoint(pstSkeleton, aenElbow[i], stElbow, fElbowScore) ||
            !posePoint(pstSkeleton, aenWrist[i], stWrist, fWristScore) ||
            fElbowScore < fGate || fWristScore < fGate)
        {
            continue;
        }
        bAnyArmVisible = true;

        /* 腕落在肩线上下的带内（y 轴向下为正，故"以上"是负偏移） */
        const float fOffset = (stWrist.fY - fAnchorY) / stInput.fShoulderWidth;
        if (fOffset < -stRule.fBandAboveShoulder || fOffset > stRule.fBandBelowShoulder)
        {
            continue;
        }
        /* 屈肘：肩-肘-腕 夹角足够小，手才收在身前而不是垂着或伸着 */
        if (angleAtVertex(stElbow, stShoulder[i], stWrist) > stRule.fMaxElbowAngle)
        {
            continue;
        }

        stQualifyWrist[nQualify++] = stWrist;
    }

    if (0 == nQualify)
    {
        /* 看得见手而手不在带内 → 否；两侧手都被挡住 → 判不出，不能当成"没在玩" */
        return bAnyArmVisible ? TriState_E::TS_FALSE : TriState_E::TS_UNKNOWN;
    }

    /* 只有双手都落在带内才谈得上靠拢：单侧成立时这一道没有两端可比 */
    if (2 == nQualify)
    {
        const float fGap = distance2d(stQualifyWrist[0], stQualifyWrist[1]);
        if (fGap > stRule.fMaxWristGapNorm * stInput.fShoulderWidth)
        {
            return TriState_E::TS_FALSE;
        }
    }
    return TriState_E::TS_TRUE;
}

float headYawNorm(const CPoseSkeleton_S *pstSkeleton, float fKeyScore, float fShoulderWidth, bool &rbUsable)
{
    rbUsable = false;
    if (nullptr == pstSkeleton || !pstSkeleton->bValid || fShoulderWidth <= CLASSROOM_DIST_EPSILON)
    {
        return 0.0f;
    }

    CPointf_S stNose, stLeftEye, stRightEye;
    float fNose = 0.0f, fLeft = 0.0f, fRight = 0.0f;
    if (!posePoint(pstSkeleton, PosePoint_E::POSE_NOSE, stNose, fNose) ||
        !posePoint(pstSkeleton, PosePoint_E::POSE_L_EYE, stLeftEye, fLeft) ||
        !posePoint(pstSkeleton, PosePoint_E::POSE_R_EYE, stRightEye, fRight))
    {
        return 0.0f;
    }
    /* 三个面点任一塌陷就不读：接近侧脸时鼻与近侧眼会重合，此时"偏移"量的是模型在两点间
       的插值误差，而这条判据要的恰恰是符号——把误差当符号用，读数会在一拍里翻两次 */
    if (fNose < fKeyScore || fLeft < fKeyScore || fRight < fKeyScore)
    {
        return 0.0f;
    }

    float fYaw = (stNose.fX - (stLeftEye.fX + stRightEye.fX) * 0.5f) / fShoulderWidth;
    if (fYaw > 1.0f)
    {
        fYaw = 1.0f;
    }
    else if (fYaw < -1.0f)
    {
        fYaw = -1.0f;
    }
    rbUsable = true;
    return fYaw;
}

TriState_E judgeFacingRef(const CGeometryInput &stInput, const FaceDirRule_S &stRule, float fRefSign)
{
    /* 0 = 本拍没有横向参考（邻座都在距离外、讲台/学生席区域没画、参考点几乎与头框同侧）。
       这一档必须判不出而不是判"朝"：否则"没画区域"会读成"全班都在听讲"，
       而这两件事在报表上是同一个数 */
    if (0.0f == fRefSign)
    {
        return TriState_E::TS_UNKNOWN;
    }

    bool bUsable = false;
    /* #91：远排面点分数整体偏低，门按头高折扣后再交给读数函数——
       折扣发生在这一个读点，headYawNorm 本身保持"给什么门用什么门"的纯函数形状 */
    const float fGate = poseKeyScoreForHeadPx(stRule.fKeyScore, stInput.stHeadBox.height());
    const float fYaw = headYawNorm(stInput.pstSkeleton, fGate, stInput.fShoulderWidth, bUsable);
    if (!bUsable)
    {
        return TriState_E::TS_UNKNOWN;
    }

    /* 折到参考侧：转正 = 朝参考点那边偏。镜像安全性的论证见头文件（两次符号翻转互相抵消） */
    const float fToward = fYaw * ((fRefSign > 0.0f) ? 1.0f : -1.0f);
    if (fToward >= stRule.fTrueAbove)
    {
        return TriState_E::TS_TRUE;
    }
    if (fToward <= stRule.fFalseBelow)
    {
        return TriState_E::TS_FALSE;
    }
    return TriState_E::TS_UNKNOWN;
}

CRectf_S estimateBodyBoxFromSkeleton(const CPoseSkeleton_S *pstSkeleton, float fScoreThreshold)
{
    CRectf_S stBox;

    if (nullptr == pstSkeleton || !pstSkeleton->bValid)
    {
        return stBox;
    }

    static const PosePoint_E astUsedPoints[] = {
        PosePoint_E::POSE_HEAD_TOP,
        PosePoint_E::POSE_L_SHOULDER, PosePoint_E::POSE_R_SHOULDER,
        PosePoint_E::POSE_L_ELBOW,    PosePoint_E::POSE_R_ELBOW,
        PosePoint_E::POSE_L_WRIST,    PosePoint_E::POSE_R_WRIST,
        PosePoint_E::POSE_L_HIP,      PosePoint_E::POSE_R_HIP,
        PosePoint_E::POSE_L_KNEE,     PosePoint_E::POSE_R_KNEE,
        PosePoint_E::POSE_L_ANKLE,    PosePoint_E::POSE_R_ANKLE,
    };

    bool bFirst = true;
    float fMinX = 0.0f, fMinY = 0.0f, fMaxX = 0.0f, fMaxY = 0.0f;
    int nUsed = 0;

    CPointf_S stPoint;
    float fScore = 0.0f;
    for (size_t i = 0; i < sizeof(astUsedPoints) / sizeof(astUsedPoints[0]); ++i)
    {
        if (!posePoint(pstSkeleton, astUsedPoints[i], stPoint, fScore) || fScore < fScoreThreshold)
        {
            continue;
        }

        if (bFirst)
        {
            fMinX = fMaxX = stPoint.fX;
            fMinY = fMaxY = stPoint.fY;
            bFirst = false;
        }
        else
        {
            fMinX = (stPoint.fX < fMinX) ? stPoint.fX : fMinX;
            fMaxX = (stPoint.fX > fMaxX) ? stPoint.fX : fMaxX;
            fMinY = (stPoint.fY < fMinY) ? stPoint.fY : fMinY;
            fMaxY = (stPoint.fY > fMaxY) ? stPoint.fY : fMaxY;
        }
        ++nUsed;
    }

    /* 可用点不足 3 个时框没有统计意义，返回空框让调用方走人头框外扩兜底 */
    if (nUsed < 3)
    {
        return CRectf_S();
    }

    stBox.fX1 = fMinX;
    stBox.fY1 = fMinY;
    stBox.fX2 = fMaxX;
    stBox.fY2 = fMaxY;
    return stBox;
}
}
