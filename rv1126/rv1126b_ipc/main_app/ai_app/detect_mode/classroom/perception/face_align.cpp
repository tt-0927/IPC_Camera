/**
 * @FilePath     : face_align.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-21 11:33:24
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 11:33:24
 * @Description  : 人脸五点对齐实现（纯函数，见 face_align.h 的文件头纪律）
 */

#include "face_align.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace Classroom
{
namespace
{
/* 判「两点是否重合」的绝对量（像素）。不取 0：关键点解出来常带 1e-6 量级的抖动，
   用 0 比等于这道闸门永不生效 */
const float ALIGN_COINCIDE_EPS = 1e-3f;

/* 判「五点铺开没有」的下限（源像素的均方根半径）。1 px 的意义：
   真实人脸最小也在十几像素以上，铺开不足 1 px 只可能是后端输出塌陷成一点 */
const float ALIGN_SPREAD_MIN_PX = 1.0f;

/* 噪声地板门的比例上限。地板随瞳距变小而无界放大（floor/span），不限住的话
   瞳距 8 px 的脸地板比例到 0.6+——那已经是点序错配的残差量级（10~20 px 模板），
   门就失去「挡镜像/点序错」的职能了 */
const float ALIGN_FLOOR_RATIO_CAP = 0.45f;

bool finitePoint(const CPointf_S &stP)
{
    return (std::isfinite(stP.fX) != 0) && (std::isfinite(stP.fY) != 0);
}

float distPx(const CPointf_S &stA, const CPointf_S &stB)
{
    const float fDx = stA.fX - stB.fX;
    const float fDy = stA.fY - stB.fY;
    return std::sqrt(fDx * fDx + fDy * fDy);
}

/* 三角形有向面积的两倍。取绝对值后与边长相比即「三点铺开程度」的无量纲度量 */
float triArea2(const CPointf_S &stA, const CPointf_S &stB, const CPointf_S &stC)
{
    return std::fabs((stB.fX - stA.fX) * (stC.fY - stA.fY) -
                     (stC.fX - stA.fX) * (stB.fY - stA.fY));
}
}

float eyeSpanPx(const CPointf_S *pastSrc)
{
    if (nullptr == pastSrc)
    {
        return 0.0f;
    }
    return distPx(pastSrc[static_cast<int>(FaceLandmark_E::FL_LEYE)],
                  pastSrc[static_cast<int>(FaceLandmark_E::FL_REYE)]);
}

CPointf_S applySimilarity(const float afM[6], const CPointf_S &stP)
{
    CPointf_S stOut;
    stOut.fX = afM[0] * stP.fX + afM[1] * stP.fY + afM[2];
    stOut.fY = afM[3] * stP.fX + afM[4] * stP.fY + afM[5];
    return stOut;
}

bool invertSimilarity(const float afM[6], float afMInv[6])
{
    if (nullptr == afM || nullptr == afMInv)
    {
        return false;
    }

    const float fDet = afM[0] * afM[4] - afM[1] * afM[3];
    if (!std::isfinite(fDet) || std::fabs(fDet) <= ALIGN_COINCIDE_EPS)
    {
        return false;
    }

    const float fInv = 1.0f / fDet;
    afMInv[0] =  afM[4] * fInv;
    afMInv[1] = -afM[1] * fInv;
    afMInv[3] = -afM[3] * fInv;
    afMInv[4] =  afM[0] * fInv;
    /* 平移项要过逆后的 2×2 再取负：漏掉这一步的后果是「形状对、位置偏」，
       比整体错更难归因 */
    afMInv[2] = -(afMInv[0] * afM[2] + afMInv[1] * afM[5]);
    afMInv[5] = -(afMInv[3] * afM[2] + afMInv[4] * afM[5]);
    return true;
}

bool faceAlignBufOk(int32_t nW, int32_t nH)
{
    if (nW <= 0 || nH <= 0)
    {
        return false;
    }
    /* 先比后乘：两个 int32 相乘在 32 位机上本身就是这道校验要挡的东西 */
    if (nW > CLASSROOM_FACE_ALIGN_MAX_BYTES || nH > CLASSROOM_FACE_ALIGN_MAX_BYTES)
    {
        return false;
    }
    return (static_cast<int64_t>(nW) * static_cast<int64_t>(nH) * 3) <=
           static_cast<int64_t>(CLASSROOM_FACE_ALIGN_MAX_BYTES);
}

const char *alignFailName(AlignFail_E enFail)
{
    switch (enFail)
    {
    case AlignFail_E::AF_OK:                return "ok";
    case AlignFail_E::AF_NULL_INPUT:        return "null_input";
    case AlignFail_E::AF_POINT_NOT_FINITE:  return "not_finite";
    case AlignFail_E::AF_SCORE_TOO_LOW:     return "low_score";
    case AlignFail_E::AF_SPREAD_TOO_SMALL:  return "spread_small";
    case AlignFail_E::AF_EYE_SPAN_TOO_SMALL:return "eye_span_small";
    case AlignFail_E::AF_EYE_LINE_BAD:      return "eye_line_bad";
    case AlignFail_E::AF_SCALE_OUT_OF_RANGE:return "scale_out_of_range";
    case AlignFail_E::AF_FIT_RESIDUAL:      return "fit_residual";
    default:                                return "unknown";
    }
}

bool solveFaceAlign(const CPointf_S *pastSrc, const float *pastScore,
                    const CPointf_S *pastTpl, const CFaceAlignParam_S &rstParam,
                    CFaceAlign_S &rstOut, CFaceAlignStat_S *pstStat, AlignFail_E &renFail,
                    float fInputPxPerSrcPx)
{
    renFail = AlignFail_E::AF_OK;
    rstOut.bValid = false;
    rstOut.fScale = 0.0f;
    rstOut.fAngleRad = 0.0f;
    rstOut.fResidualPx = 0.0f;

    const int32_t nNum = CLASSROOM_FACE_LANDMARK_NUM;
    if (nullptr == pastSrc || nullptr == pastTpl)
    {
        renFail = AlignFail_E::AF_NULL_INPUT;
        return false;
    }

    if (nullptr != pstStat)
    {
        ++pstStat->u64Tried;
    }

    /* 逐点闸门一次过完，后面所有判据都只看这张有效表——
       「哪里该算 NaN」写两遍迟早会漏一遍 */
    bool abValid[CLASSROOM_FACE_LANDMARK_NUM] = { false };
    int32_t nValid = 0;
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (!finitePoint(pastSrc[i]))
        {
            renFail = AlignFail_E::AF_POINT_NOT_FINITE;
            if (nullptr != pstStat)
            {
                ++pstStat->u64FailNotFinite;
            }
            return false;
        }
        /* 分数缺失（pastScore 为空）按「都信」处理：调用方是显式放弃这一维，
           不是忘了传——真忘了传的后果是有效点不足，会立刻以 low_score 现形 */
        const float fScore = (nullptr != pastScore) ? pastScore[i] : 1.0f;
        if (fScore >= rstParam.fPointScoreMin)
        {
            abValid[i] = true;
            ++nValid;
        }
    }
    if (nValid < 3)
    {
        renFail = AlignFail_E::AF_SCORE_TOO_LOW;
        if (nullptr != pstStat)
        {
            ++pstStat->u64FailLowScore;
        }
        return false;
    }

    /* 有效点的重心。只按有效点求，坏点不参与任何一项统计 */
    float fCx = 0.0f;
    float fCy = 0.0f;
    float fTx = 0.0f;
    float fTy = 0.0f;
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (!abValid[i])
        {
            continue;
        }
        fCx += pastSrc[i].fX;
        fCy += pastSrc[i].fY;
        fTx += pastTpl[i].fX;
        fTy += pastTpl[i].fY;
    }
    fCx /= static_cast<float>(nValid);
    fCy /= static_cast<float>(nValid);
    fTx /= static_cast<float>(nValid);
    fTy /= static_cast<float>(nValid);

    /* 复数域闭式最小二乘：min Σ|v - c·u|² → c = Σ(conj(u)·v) / Σ|u|²。
       与 2×2 SVD 等价但没有迭代与分支，跨编译器逐位可复现——这一点值得计较，
       因为同一份对齐结果要能在主机单测与设备上比对，SVD 的迭代容差做不到 */
    float fS = 0.0f;    /* Σ|u|² */
    float fA = 0.0f;    /* Σ(u.x·v.x + u.y·v.y) → c 的实部分子 */
    float fB = 0.0f;    /* Σ(u.x·v.y - u.y·v.x) → c 的虚部分子 */
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (!abValid[i])
        {
            continue;
        }
        const float fUx = pastSrc[i].fX - fCx;
        const float fUy = pastSrc[i].fY - fCy;
        const float fVx = pastTpl[i].fX - fTx;
        const float fVy = pastTpl[i].fY - fTy;
        fS += fUx * fUx + fUy * fUy;
        fA += fUx * fVx + fUy * fVy;
        fB += fUx * fVy - fUy * fVx;
    }

    if (!std::isfinite(fS) || !std::isfinite(fA) || !std::isfinite(fB) ||
        fS <= ALIGN_SPREAD_MIN_PX * ALIGN_SPREAD_MIN_PX)
    {
        /* 五点近于重合：自由度退化，解出的角度是纯噪声 */
        renFail = AlignFail_E::AF_SPREAD_TOO_SMALL;
        if (nullptr != pstStat)
        {
            ++pstStat->u64FailSpread;
        }
        return false;
    }

    /* 侧脸时「鼻偏离眼线」依然存在（鼻尖在画面里离开了两眼连线），只有点序错乱
       或后端输出塌陷才会三点共线。眼/鼻任一点被分数门剔掉时这一道跳过：
       测不了就说测不了，不拿别的三点凑——换一对点判出来的东西不是同一件事 */
    const int32_t nLeye = static_cast<int32_t>(FaceLandmark_E::FL_LEYE);
    const int32_t nReye = static_cast<int32_t>(FaceLandmark_E::FL_REYE);
    const int32_t nNose = static_cast<int32_t>(FaceLandmark_E::FL_NOSE);
    if (abValid[nLeye] && abValid[nReye] && abValid[nNose])
    {
        const float fArea2 = triArea2(pastSrc[nLeye], pastSrc[nReye], pastSrc[nNose]);
        const float fEyeLen = distPx(pastSrc[nLeye], pastSrc[nReye]);
        /* 高/底 < 0.05 即三点近于共线。用比值而不是绝对面积，是为了远焦小脸不被误杀 */
        if (fEyeLen <= ALIGN_COINCIDE_EPS)
        {
            renFail = AlignFail_E::AF_EYE_LINE_BAD;
            if (nullptr != pstStat)
            {
                ++pstStat->u64FailEyeLine;
            }
            return false;
        }
        if ((fArea2 / fEyeLen) < 0.05f)
        {
            renFail = AlignFail_E::AF_EYE_LINE_BAD;
            if (nullptr != pstStat)
            {
                ++pstStat->u64FailEyeLine;
            }
            return false;
        }
    }

    /* 脸的尺寸闸。双眼都在时按瞳距，缺一只眼时按有效点的最大跨度：
       大脸缺一只眼仍然该做（侧脸正是考勤要认的那些人），
       但「太小就别费劲」这条判据在两种情况下都成立，故只是换了个可测的量 */
    float fSpanPx = 0.0f;
    if (abValid[nLeye] && abValid[nReye])
    {
        fSpanPx = distPx(pastSrc[nLeye], pastSrc[nReye]);
    }
    else
    {
        for (int32_t i = 0; i < nNum; ++i)
        {
            for (int32_t j = i + 1; j < nNum; ++j)
            {
                if (abValid[i] && abValid[j])
                {
                    const float fD = distPx(pastSrc[i], pastSrc[j]);
                    if (fD > fSpanPx)
                    {
                        fSpanPx = fD;
                    }
                }
            }
        }
    }
    /* 用 !(a >= b) 而不是 (a < b)：NaN 时前者为真、走失败分支。
       这里虽然已在前面查过有限性，但闸门自身的判序不该依赖上游——
       依赖关系一变，比较方向就悄悄从「挡得住」变成「挡不住」 */
    if (!(fSpanPx >= rstParam.fMinEyeSpanPx))
    {
        renFail = AlignFail_E::AF_EYE_SPAN_TOO_SMALL;
        if (nullptr != pstStat)
        {
            ++pstStat->u64FailEyeSpan;
        }
        return false;
    }

    /* c = (A + iB)/S ⇒ 源→模板：x' = a·x - b·y，y' = b·x + a·y */
    const float fA0 = fA / fS;
    const float fB0 = fB / fS;
    const float fScale = std::sqrt(fA0 * fA0 + fB0 * fB0);

    if (!std::isfinite(fScale) || fScale < rstParam.fScaleMin || fScale > rstParam.fScaleMax)
    {
        renFail = AlignFail_E::AF_SCALE_OUT_OF_RANGE;
        if (nullptr != pstStat)
        {
            ++pstStat->u64FailScale;
        }
        return false;
    }

    CFaceAlign_S stTry;
    stTry.afSrcToTpl[0] = fA0;
    stTry.afSrcToTpl[1] = -fB0;
    stTry.afSrcToTpl[3] = fB0;
    stTry.afSrcToTpl[4] = fA0;
    stTry.afSrcToTpl[2] = fTx - (stTry.afSrcToTpl[0] * fCx + stTry.afSrcToTpl[1] * fCy);
    stTry.afSrcToTpl[5] = fTy - (stTry.afSrcToTpl[3] * fCx + stTry.afSrcToTpl[4] * fCy);
    stTry.fScale = fScale;
    stTry.fAngleRad = std::atan2(fB0, fA0);

    if (!invertSimilarity(stTry.afSrcToTpl, stTry.afTplToSrc))
    {
        renFail = AlignFail_E::AF_SCALE_OUT_OF_RANGE;
        if (nullptr != pstStat)
        {
            ++pstStat->u64FailScale;
        }
        return false;
    }

    /* 残差按模板像素计，门限取「模板瞳距 × 比例」。这是唯一能区分
       「解算成功但点序错了」与「解算成功且点对了」的量——镜像机位的左右眼互换、
       后端把鼻子当成左眼，都能给出一个数学上合法、几何上错一整个脸的变换 */
    float fSumSq = 0.0f;
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (!abValid[i])
        {
            continue;
        }
        const CPointf_S stPred = applySimilarity(stTry.afSrcToTpl, pastSrc[i]);
        const float fDx = stPred.fX - pastTpl[i].fX;
        const float fDy = stPred.fY - pastTpl[i].fY;
        if (!std::isfinite(fDx) || !std::isfinite(fDy))
        {
            renFail = AlignFail_E::AF_POINT_NOT_FINITE;
            if (nullptr != pstStat)
            {
                ++pstStat->u64FailNotFinite;
            }
            return false;
        }
        fSumSq += fDx * fDx + fDy * fDy;
    }
    const float fRms = std::sqrt(fSumSq / static_cast<float>(nValid));
    stTry.fResidualPx = fRms;

    /* 门 = max(相对门, 噪声地板门)，两者都按「模板瞳距的比例」表达 */
    const float fTplEye = distPx(pastTpl[nLeye], pastTpl[nReye]);
    float fGateRatio = rstParam.fMaxResidualRatio;
    if ((fInputPxPerSrcPx > 0.0f) && (fSpanPx > 0.0f) && (rstParam.fNoiseFloorSrcPx > 0.0f))
    {
        float fFloorRatio = rstParam.fNoiseFloorSrcPx * fInputPxPerSrcPx / fSpanPx;
        if (fFloorRatio > ALIGN_FLOOR_RATIO_CAP)
        {
            fFloorRatio = ALIGN_FLOOR_RATIO_CAP;
        }
        if (fFloorRatio > fGateRatio)
        {
            fGateRatio = fFloorRatio;
        }
    }
    if (!(fRms <= fGateRatio * fTplEye))
    {
        renFail = AlignFail_E::AF_FIT_RESIDUAL;
        if (nullptr != pstStat)
        {
            ++pstStat->u64FailResidual;
        }
        return false;
    }

    stTry.bValid = true;
    rstOut = stTry;
    if (nullptr != pstStat)
    {
        ++pstStat->u64Ok;
    }
    return true;
}

bool warpFaceRgb(const uint8_t *pucSrcRgb, int32_t nSrcW, int32_t nSrcH,
                 const CFaceAlign_S &rstAlign, int32_t nTplW, int32_t nTplH,
                 std::vector<uint8_t> &rvecDst, uint8_t ucPadValue,
                 bool bClampEdge, CFaceAlignStat_S *pstStat)
{
    if (nullptr == pucSrcRgb || !rstAlign.bValid || nSrcW <= 0 || nSrcH <= 0 ||
        !faceAlignBufOk(nTplW, nTplH))
    {
        return false;
    }

    /* 上限是给 32 位机上 (y*nSrcW + x)*3 这个乘法划的界。4096 是宽裕的保守值：
       本子系统里没有任何一路的源图会超过 RGA 单次变换的合理尺寸 */
    if (nSrcW > 4096 || nSrcH > 4096)
    {
        return false;
    }

    const size_t nOutBytes = static_cast<size_t>(nTplW) * static_cast<size_t>(nTplH) * 3U;
    rvecDst.assign(nOutBytes, ucPadValue);

    for (int32_t ny = 0; ny < nTplH; ++ny)
    {
        for (int32_t nx = 0; nx < nTplW; ++nx)
        {
            /* 坐标约定：关键点与像素索引同基（整数即像素质心），故回映后直接采样，
               不加也不减 0.5。这一条必须与给出五点的后端一致——两侧不一致时
               整个对齐会系统性偏移半像素，而半像素不足以让识别失败，
               只足以让"在主机上验证过的那版"与"设备上的那一版"分数差一截 */
            const CPointf_S stSrc = applySimilarity(rstAlign.afTplToSrc, CPointf_S{
                static_cast<float>(nx), static_cast<float>(ny) });

            uint8_t *pucDst = &rvecDst[(static_cast<size_t>(ny) * static_cast<size_t>(nTplW) +
                                        static_cast<size_t>(nx)) * 3U];

            if (!std::isfinite(stSrc.fX) || !std::isfinite(stSrc.fY))
            {
                continue;
            }

            float fX = stSrc.fX;
            float fY = stSrc.fY;
            if (bClampEdge)
            {
                fX = (fX < 0.0f) ? 0.0f : ((fX > static_cast<float>(nSrcW - 1)) ?
                                           static_cast<float>(nSrcW - 1) : fX);
                fY = (fY < 0.0f) ? 0.0f : ((fY > static_cast<float>(nSrcH - 1)) ?
                                           static_cast<float>(nSrcH - 1) : fY);
            }

            const int32_t nX0 = static_cast<int32_t>(std::floor(fX));
            const int32_t nY0 = static_cast<int32_t>(std::floor(fY));
            const float fFx = fX - static_cast<float>(nX0);
            const float fFy = fY - static_cast<float>(nY0);

            /* 四邻域里任何一格出界就整格填背景：半取半钳会让边界上出现
               「一半真图一半边缘复制」的接缝，而接缝正好落在脸贴到裁剪框边的时候，
               那种图喂进识别的分数比整张填背景更容易骗过人（看着像有脸） */
            if (nX0 < 0 || nY0 < 0 || (nX0 + 1) >= nSrcW || (nY0 + 1) >= nSrcH)
            {
                pucDst[0] = ucPadValue;
                pucDst[1] = ucPadValue;
                pucDst[2] = ucPadValue;
                continue;
            }

            const float fW00 = (1.0f - fFx) * (1.0f - fFy);
            const float fW10 = fFx * (1.0f - fFy);
            const float fW01 = (1.0f - fFx) * fFy;
            const float fW11 = fFx * fFy;

            const int32_t nOff00 = (nY0 * nSrcW + nX0) * 3;
            const int32_t nOff10 = nOff00 + 3;
            const int32_t nOff01 = nOff00 + nSrcW * 3;
            const int32_t nOff11 = nOff01 + 3;

            for (int32_t c = 0; c < 3; ++c)
            {
                const float fVal =
                    static_cast<float>(pucSrcRgb[nOff00 + c]) * fW00 +
                    static_cast<float>(pucSrcRgb[nOff10 + c]) * fW10 +
                    static_cast<float>(pucSrcRgb[nOff01 + c]) * fW01 +
                    static_cast<float>(pucSrcRgb[nOff11 + c]) * fW11;
                /* +0.5 而非就近取整的 trunc：float→int 是截断，不补半格会整体偏暗一档 */
                int32_t nV = static_cast<int32_t>(fVal + 0.5f);
                nV = (nV < 0) ? 0 : ((nV > 255) ? 255 : nV);
                pucDst[c] = static_cast<uint8_t>(nV);
            }

            if (nullptr != pstStat)
            {
                ++pstStat->u64WarpPixel;
            }
        }
    }
    return true;
}

/* ============================ 点序口径 ============================ */

namespace
{
/* 置换表：语义为「模板槽 i 取模型输出的第 p[i] 个点」。
   每行都自检过是 0~4 的一个排列（单测 classroom_face_test 再钉一遍，写重号是静默错） */
const int32_t AST_PERM[CLASSROOM_ALIGN_PERM_NUM][CLASSROOM_FACE_LANDMARK_NUM] = {
    { 0, 1, 2, 3, 4 },   /* identity：模型出点序与模板同 */
    { 1, 0, 2, 4, 3 },   /* lr_swap：眼与嘴角整体换边（模型按被摄者解剖学出点时就是这张） */
    { 1, 0, 2, 3, 4 },   /* eye_swap：只换眼 */
    { 0, 1, 2, 4, 3 },   /* mouth_swap：只换嘴角 */
    { 4, 3, 2, 1, 0 },   /* reverse：整表倒序（有些导出脚本按"从右到左"写标签） */
    { 1, 2, 3, 4, 0 }    /* nose_first：模型把鼻尖放在第 0 位，其余顺移 */
};

const char *AST_PERM_NAME[CLASSROOM_ALIGN_PERM_NUM] = {
    "identity", "lr_swap", "eye_swap", "mouth_swap", "reverse", "nose_first"
};
}

const int32_t *alignPermTable(int32_t nPerm)
{
    if (nPerm < 0 || nPerm >= CLASSROOM_ALIGN_PERM_NUM)
    {
        return AST_PERM[0];
    }
    return AST_PERM[nPerm];
}

const char *alignPermName(int32_t nPerm)
{
    if (nPerm < 0 || nPerm >= CLASSROOM_ALIGN_PERM_NUM)
    {
        return "unknown";
    }
    return AST_PERM_NAME[nPerm];
}

int32_t alignPermOf(const char *pName)
{
    if (nullptr == pName)
    {
        return -1;
    }
    for (int32_t i = 0; i < CLASSROOM_ALIGN_PERM_NUM; ++i)
    {
        if (0 == std::strcmp(pName, AST_PERM_NAME[i]))
        {
            return i;
        }
    }
    return -1;
}

bool applyAlignPerm(const CPointf_S *pastIn, const float *pastScoreIn,
                    CPointf_S *pastOut, float *pastScoreOut, int32_t nPerm)
{
    if (nullptr == pastIn || nullptr == pastOut ||
        nPerm < 0 || nPerm >= CLASSROOM_ALIGN_PERM_NUM)
    {
        return false;
    }

    const int32_t *pnMap = AST_PERM[nPerm];
    for (int32_t i = 0; i < CLASSROOM_FACE_LANDMARK_NUM; ++i)
    {
        pastOut[i] = pastIn[pnMap[i]];
    }
    /* 分数必须跟着点走。分开重排（或干脆不重排）会让"哪个点被分数门剔掉"这件事
       在置换前后不一致，残差与剔除原因就对不上号 */
    if (nullptr != pastScoreIn && nullptr != pastScoreOut)
    {
        for (int32_t i = 0; i < CLASSROOM_FACE_LANDMARK_NUM; ++i)
        {
            pastScoreOut[i] = pastScoreIn[pnMap[i]];
        }
    }
    return true;
}

float faceYawDeg(const CPointf_S *pastSlot, int32_t nNum)
{
    if (nullptr == pastSlot || nNum < CLASSROOM_FACE_LANDMARK_NUM)
    {
        return std::nanf("");
    }
    const CPointf_S stLEye = pastSlot[static_cast<int32_t>(FaceLandmark_E::FL_LEYE)];
    const CPointf_S stREye = pastSlot[static_cast<int32_t>(FaceLandmark_E::FL_REYE)];
    const CPointf_S stNose = pastSlot[static_cast<int32_t>(FaceLandmark_E::FL_NOSE)];
    if (!finitePoint(stLEye) || !finitePoint(stREye) || !finitePoint(stNose))
    {
        return std::nanf("");
    }

    const float fEyeDx = stREye.fX - stLEye.fX;
    const float fEyeDy = stREye.fY - stLEye.fY;
    const float fEyeSpan = std::sqrt(fEyeDx * fEyeDx + fEyeDy * fEyeDy);
    /* 1 px 的下限而不是 0：小脸（瞳距 4~6 px）上关键点各差 1 px 就能让这个比翻倍，
       而此时算出来的角度没有意义。返回 NaN 让调用方按「量不出来」处理而不是按 0 处理 */
    if (!(fEyeSpan >= 1.0f))
    {
        return std::nanf("");
    }

    const float fDx = stNose.fX - 0.5f * (stLEye.fX + stREye.fX);
    /* 横向位移用「半瞳距」归一：正脸时鼻尖落在双眼中点（模板里 56.03 vs 55.91），
       横摆 90° 时鼻尖投影到一侧眼角外，位移约等于半瞳距，asin 正好收到 90°。
       系数 2.0 就是这一步（dx / (eyeSpan/2) = 2·dx/eyeSpan），没有别的可调量 */
    float fRatio = 2.0f * fDx / fEyeSpan;
    if (fRatio > 1.0f)
    {
        fRatio = 1.0f;
    }
    else if (fRatio < -1.0f)
    {
        fRatio = -1.0f;
    }
    const float fDeg = std::asin(fRatio) * (180.0f / 3.14159265358979323846f);
    return std::isfinite(fDeg) ? fDeg : std::nanf("");
}

float faceAlignFitRms(const CPointf_S *pastSrc, const CPointf_S *pastTpl, float *pfScale)
{
    if (nullptr != pfScale)
    {
        *pfScale = 0.0f;
    }
    if (nullptr == pastSrc || nullptr == pastTpl)
    {
        return std::nanf("");
    }

    const int32_t nNum = CLASSROOM_FACE_LANDMARK_NUM;
    float fCx = 0.0f;
    float fCy = 0.0f;
    float fTx = 0.0f;
    float fTy = 0.0f;
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (!finitePoint(pastSrc[i]) || !finitePoint(pastTpl[i]))
        {
            return std::nanf("");
        }
        fCx += pastSrc[i].fX;
        fCy += pastSrc[i].fY;
        fTx += pastTpl[i].fX;
        fTy += pastTpl[i].fY;
    }
    fCx /= static_cast<float>(nNum);
    fCy /= static_cast<float>(nNum);
    fTx /= static_cast<float>(nNum);
    fTy /= static_cast<float>(nNum);

    float fS = 0.0f;
    float fA = 0.0f;
    float fB = 0.0f;
    for (int32_t i = 0; i < nNum; ++i)
    {
        const float fUx = pastSrc[i].fX - fCx;
        const float fUy = pastSrc[i].fY - fCy;
        const float fVx = pastTpl[i].fX - fTx;
        const float fVy = pastTpl[i].fY - fTy;
        fS += fUx * fUx + fUy * fUy;
        fA += fUx * fVx + fUy * fVy;
        fB += fUx * fVy - fUy * fVx;
    }

    /* 退化时返回 NaN 而不是 0：探针里 0 意味着"完美拟合"，会把表排序整个带错，
       而 NaN 过任何比较都是 false，那张表自然排不到第一 */
    if (!std::isfinite(fS) || fS <= ALIGN_SPREAD_MIN_PX * ALIGN_SPREAD_MIN_PX)
    {
        return std::nanf("");
    }

    const float fA0 = fA / fS;
    const float fB0 = fB / fS;
    if (nullptr != pfScale)
    {
        *pfScale = std::sqrt(fA0 * fA0 + fB0 * fB0);
    }

    float fSumSq = 0.0f;
    /* 矩阵的构造与 solveFaceAlign 里逐行同构（含平移项的符号）：探针挑中的表
       必须在真解算里得到同一个残差，两处写法不同就等于探针与闸门互相打脸 */
    float afM[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    afM[0] = fA0;
    afM[1] = -fB0;
    afM[3] = fB0;
    afM[4] = fA0;
    afM[2] = fTx - (afM[0] * fCx + afM[1] * fCy);
    afM[5] = fTy - (afM[3] * fCx + afM[4] * fCy);

    for (int32_t i = 0; i < nNum; ++i)
    {
        const CPointf_S stP = applySimilarity(afM, pastSrc[i]);
        const float fDx = stP.fX - pastTpl[i].fX;
        const float fDy = stP.fY - pastTpl[i].fY;
        fSumSq += fDx * fDx + fDy * fDy;
    }
    return std::sqrt(fSumSq / static_cast<float>(nNum));
}

int32_t alignProbeFeed(const CPointf_S *pastSrc, const CPointf_S *pastTpl,
                       const CFaceAlignParam_S &rstParam, CFaceAlignProbe_S &rstProbe)
{
    if (nullptr == pastSrc || nullptr == pastTpl)
    {
        return -1;
    }

    const float fGate = rstParam.fMaxResidualRatio *
                        distPx(pastTpl[static_cast<int>(FaceLandmark_E::FL_LEYE)],
                               pastTpl[static_cast<int>(FaceLandmark_E::FL_REYE)]);
    int32_t nBest = -1;
    float fBest = 0.0f;
    ++rstProbe.u64Face;

    for (int32_t p = 0; p < CLASSROOM_ALIGN_PERM_NUM; ++p)
    {
        CPointf_S astTry[CLASSROOM_FACE_LANDMARK_NUM];
        if (!applyAlignPerm(pastSrc, nullptr, astTry, nullptr, p))
        {
            continue;
        }
        const float fRms = faceAlignFitRms(astTry, pastTpl, nullptr);
        if (!std::isfinite(fRms))
        {
            continue;
        }
        /* 首样本按本表的计数判，不拿"累加和为 0"当哨兵：合成样本（单测里那些）
           的残差可以正好是 0.0，那时哨兵永不复位，min 会一直停在 0 上 */
        if (0 == rstProbe.u64Num[p] || fRms < rstProbe.afMinRms[p])
        {
            rstProbe.afMinRms[p] = fRms;
        }
        ++rstProbe.u64Num[p];
        rstProbe.adSumRms[p] += static_cast<double>(fRms);
        if (fRms <= fGate)
        {
            ++rstProbe.u64Ok[p];
        }
        if (nBest < 0 || fRms < fBest)
        {
            nBest = p;
            fBest = fRms;
        }
    }
    if (nBest < 0)
    {
        ++rstProbe.nBad;
    }
    return nBest;
}

const char *alignProbeText(const CFaceAlignProbe_S &rstProbe, char *pacBuf, int32_t nBufSize)
{
    if (nullptr == pacBuf || nBufSize <= 0)
    {
        return "";
    }
    pacBuf[0] = '\0';

    /* 按剩余空间推进指针：拼接观测串最常见的错法是先 strlen 再 strncat，
       长度算差一时截断的是"最后一张表"，而那张恰好是残差最小的表 */
    char *pucAt = pacBuf;
    int32_t nLeft = nBufSize;
    for (int32_t p = 0; p < CLASSROOM_ALIGN_PERM_NUM; ++p)
    {
        if (0 == rstProbe.u64Num[p])
        {
            continue;
        }
        const double dAvg = rstProbe.adSumRms[p] / static_cast<double>(rstProbe.u64Num[p]);
        const int32_t nWrite = std::snprintf(pucAt, static_cast<size_t>(nLeft),
                                             "%s%s=%.1f n%llu ok%llu",
                                             (pucAt == pacBuf) ? "" : " ",
                                             alignPermName(p), dAvg,
                                             static_cast<unsigned long long>(rstProbe.u64Num[p]),
                                             static_cast<unsigned long long>(rstProbe.u64Ok[p]));
        if (nWrite < 0 || nWrite >= nLeft)
        {
            break;    /* 缓冲区满了：丢掉尾部几张表，比越界写强 */
        }
        pucAt += nWrite;
        nLeft -= nWrite;
    }
    return pacBuf;
}

/* ============================ 对齐用的裁剪几何 ============================ */

bool faceAlignCropRect(const CRectf_S &stBox, float fExpand, const CRectf_S &stLimit,
                       CRectf_S &rstOut)
{
    rstOut = CRectf_S();
    const CPointf_S stC = stBox.center();
    /* 外扩基准取长边：脸框在俯拍里普遍"高大于宽"，按宽外扩会把下巴切掉，
       而下巴正是嘴角点周围的信息所在 */
    const float fSideLong = stBox.width() > stBox.height() ? stBox.width() : stBox.height();
    if (!(fSideLong > 0.0f) || !(fExpand >= 1.0f) ||
        !std::isfinite(fSideLong) || !std::isfinite(fExpand))
    {
        return false;
    }
    if (!(stLimit.width() > 0.0f) || !(stLimit.height() > 0.0f))
    {
        return false;
    }

    const float fHalf = fSideLong * fExpand * 0.5f;
    float fX1 = stC.fX - fHalf;
    float fY1 = stC.fY - fHalf;
    const float fW = fHalf * 2.0f;

    /* 平移回画面内而不是缩边：缩边会让正方形变长方形且把脸推出模板中心，
       而这里的坐标要一直用到 warp，中心一偏就等于对齐白做 */
    if (fX1 < stLimit.fX1)
    {
        fX1 = stLimit.fX1;
    }
    if (fX1 + fW > stLimit.fX2)
    {
        fX1 = stLimit.fX2 - fW;
    }
    if (fY1 < stLimit.fY1)
    {
        fY1 = stLimit.fY1;
    }
    if (fY1 + fW > stLimit.fY2)
    {
        fY1 = stLimit.fY2 - fW;
    }

    /* 画面本身比外扩框还小时，上面两条平移会互相抵消，只剩零宽或负宽——此时确实没法裁，
       明确返回 false 让调用方计数，而不是交给 RGA 去判一个模糊失败 */
    if (!(fW > 0.0f) || fX1 < stLimit.fX1 || fY1 < stLimit.fY1 ||
        fX1 + fW > stLimit.fX2 + 0.001f || fY1 + fW > stLimit.fY2 + 0.001f)
    {
        return false;
    }

    rstOut = CRectf_S{ fX1, fY1, fX1 + fW, fY1 + fW };
    return true;
}

void mapPointsToRect(const CPointf_S *pastSrc, int32_t nNum, const CRectf_S &stUsed,
                     int32_t nDstW, int32_t nDstH, CPointf_S *pastOut)
{
    if (nullptr == pastSrc || nullptr == pastOut || nNum <= 0 || nDstW <= 0 || nDstH <= 0)
    {
        return;
    }
    const float fW = stUsed.width();
    const float fH = stUsed.height();
    if (!(fW > 0.0f) || !(fH > 0.0f))
    {
        return;
    }
    const float fSx = static_cast<float>(nDstW) / fW;
    const float fSy = static_cast<float>(nDstH) / fH;
    for (int32_t i = 0; i < nNum; ++i)
    {
        pastOut[i].fX = (pastSrc[i].fX - stUsed.fX1) * fSx;
        pastOut[i].fY = (pastSrc[i].fY - stUsed.fY1) * fSy;
    }
}
}
