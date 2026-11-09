/**
 * @FilePath     : detect_decode.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-13 15:12:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-12 15:12:00
 * @Description  : 检测输出解码与头身关联实现
 */

#include "detect_decode.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "dlog.h"

namespace Classroom
{
/* 体框占用表用 uint64 位图 */
static_assert(CLASSROOM_MAX_PERSONS <= 64, "体框占用位图按 64 人上限设计");

/* 畸形输出的日志节流 */
static bool shouldLogMalformed(uint64_t u64Count)
{
    return (0 != u64Count) && (0 == (u64Count & (u64Count - 1)));
}

bool CDetMap::setup(const CFrameGeometry &stSrcToCanvas,
                    int nRectX, int nRectY, int nRectW, int nRectH,
                    int nModelW, int nModelH, std::string &rstrErr)
{
    /* 无 letterbox：与既有调用方逐字节同形（内容平面 == 模型平面） */
    return setup(stSrcToCanvas, nRectX, nRectY, nRectW, nRectH, nModelW, nModelH,
                 CLetterboxPlan_S(), rstrErr);
}

bool CDetMap::setup(const CFrameGeometry &stSrcToCanvas,
                    int nRectX, int nRectY, int nRectW, int nRectH,
                    int nModelW, int nModelH, const CLetterboxPlan_S &stLb, std::string &rstrErr)
{
    m_bReady = false;
    rstrErr.clear();

    if (!stSrcToCanvas.isReady())
    {
        rstrErr = "source->canvas geometry not ready";
        return false;
    }

    if (nRectW <= 0 || nRectH <= 0 || nModelW <= 0 || nModelH <= 0)
    {
        rstrErr = "rect/model size must be positive";
        return false;
    }

    /* 裁剪矩形必须整体落在源帧内。越界说明调用方的钳制逻辑算错了，
       这里替他钳回去只会让坐标整体偏移而看不出问题，宁可不映射 */
    const int nSrcW = stSrcToCanvas.srcWidth();
    const int nSrcH = stSrcToCanvas.srcHeight();
    if (nRectX < 0 || nRectY < 0 || (nRectX + nRectW) > nSrcW || (nRectY + nRectH) > nSrcH)
    {
        rstrErr = "crop rect out of source frame";
        return false;
    }

    /* 内容平面：letterbox 关 = 模型档本身；开 = 计划给出的左上锚定内容子矩形。
       pad 只落右/下，所以模型平面到内容平面没有偏移量，
       仅有"平面变小"这一件事——下游所有映射式不用加任何 if */
    int nPlaneW = nModelW;
    int nPlaneH = nModelH;
    if (stLb.bEnable)
    {
        if (stLb.nContentW <= 0 || stLb.nContentH <= 0 ||
            stLb.nContentW > nModelW || stLb.nContentH > nModelH)
        {
            rstrErr = "letterbox plan does not fit model plane";
            return false;
        }
        nPlaneW = stLb.nContentW;
        nPlaneH = stLb.nContentH;
    }

    /* 直接沿用帧自带几何：镜像/旋转/缩放参数的唯一权威是 CFrameBus 建的那一份，
       在这里重算一遍等于开了第二个真相来源 */
    m_stSrcToCanvas = stSrcToCanvas;

    /* 裁剪矩形 → 内容平面是纯缩放：RGA 不做镜像也不做旋转（见 crop_unit.h），
       所以这一段的 mirror/rotate 恒为「无」，与上一段的旋转不能合并成一个变换 */
    if (!m_stRectToModel.setup(nRectW, nRectH, nPlaneW, nPlaneH,
                               MirrorMode_E::MIRROR_NONE, 0))
    {
        rstrErr = "rect->model geometry setup failed";
        m_stSrcToCanvas = CFrameGeometry();
        return false;
    }

    if (!m_stSrcToCanvas.selfCheck() || !m_stRectToModel.selfCheck())
    {
        /* 往返不闭合意味着镜像/旋转的组合在本机位下不自洽（理论上不该发生，
           发生了就是 frame_geometry 被改坏），此时任何输出坐标都不可信 */
        rstrErr = "geometry self check failed";
        m_stSrcToCanvas = CFrameGeometry();
        m_stRectToModel = CFrameGeometry();
        return false;
    }

    m_nRectX = nRectX;
    m_nRectY = nRectY;
    m_nRectW = nRectW;
    m_nRectH = nRectH;
    m_nModelW = nModelW;
    m_nModelH = nModelH;
    /* 分母取内容平面：letterbox 开时它就是"一个模型像素对应多少源帧像素"的真值
       （两个方向同比值 = 保了宽高比的可观测形状） */
    m_fSrcPxPerModelX = static_cast<float>(nRectW) / static_cast<float>(nPlaneW);
    m_fSrcPxPerModelY = static_cast<float>(nRectH) / static_cast<float>(nPlaneH);

    m_bReady = true;
    return true;
}

CPointf_S CDetMap::modelToSrc(const CPointf_S &stPoint) const
{
    /* 先缩放回裁剪矩形局部坐标，再平移到源帧坐标。
       顺序不能反：平移量是源帧像素，必须在「已经回到源帧尺度」之后加 */
    const CPointf_S stLocal = m_stRectToModel.dstToSrc(stPoint);

    return CPointf_S{ stLocal.fX + static_cast<float>(m_nRectX),
                      stLocal.fY + static_cast<float>(m_nRectY) };
}

CPointf_S CDetMap::srcToModel(const CPointf_S &stPoint) const
{
    CPointf_S stLocal;
    stLocal.fX = stPoint.fX - static_cast<float>(m_nRectX);
    stLocal.fY = stPoint.fY - static_cast<float>(m_nRectY);

    return m_stRectToModel.srcToDst(stLocal);
}

CPointf_S CDetMap::modelToCanvas(const CPointf_S &stPoint) const
{
    if (!m_bReady)
    {
        return stPoint;
    }
    return m_stSrcToCanvas.srcToDst(modelToSrc(stPoint));
}

CRectf_S CDetMap::modelToCanvas(const CRectf_S &stRect) const
{
    if (!m_bReady)
    {
        return stRect;
    }

    /* 两条腿都是「镜像/旋转为 0 的轴对齐单调映射」，故矩形版可直接用，
       且每一级都自带钳制：先在模型图上裁掉溢出部分，再换算到分析帧 */
    CRectf_S stSrc = m_stRectToModel.dstToSrc(stRect);
    stSrc.fX1 += static_cast<float>(m_nRectX);
    stSrc.fY1 += static_cast<float>(m_nRectY);
    stSrc.fX2 += static_cast<float>(m_nRectX);
    stSrc.fY2 += static_cast<float>(m_nRectY);

    return m_stSrcToCanvas.srcToDst(stSrc);
}

CRectf_S CDetMap::canvasToModel(const CRectf_S &stRect) const
{
    if (!m_bReady)
    {
        return stRect;
    }
    /* 逆变换走 frame_geometry 的矩形版本：它在 src 侧已经钳过一次，
       这里要的就是「画在模型图上时不越界」，与正变换的钳制口径对称 */
    const CRectf_S stSrc = m_stSrcToCanvas.dstToSrc(stRect);
    return m_stRectToModel.srcToDst(stSrc);
}

float containRatioOf(const CRectf_S &stInner, const CRectf_S &stOuter)
{
    const float fInnerArea = stInner.area();
    if (fInnerArea <= 0.0f)
    {
        return 0.0f;
    }

    const float fIx1 = (stInner.fX1 > stOuter.fX1) ? stInner.fX1 : stOuter.fX1;
    const float fIy1 = (stInner.fY1 > stOuter.fY1) ? stInner.fY1 : stOuter.fY1;
    const float fIx2 = (stInner.fX2 < stOuter.fX2) ? stInner.fX2 : stOuter.fX2;
    const float fIy2 = (stInner.fY2 < stOuter.fY2) ? stInner.fY2 : stOuter.fY2;

    if (fIx2 <= fIx1 || fIy2 <= fIy1)
    {
        return 0.0f;
    }
    return ((fIx2 - fIx1) * (fIy2 - fIy1)) / fInnerArea;
}

float iouOf(const CRectf_S &stA, const CRectf_S &stB)
{
    const float fAreaA = stA.area();
    const float fAreaB = stB.area();
    if (fAreaA <= 0.0f || fAreaB <= 0.0f)
    {
        return 0.0f;
    }

    const float fIx1 = (stA.fX1 > stB.fX1) ? stA.fX1 : stB.fX1;
    const float fIy1 = (stA.fY1 > stB.fY1) ? stA.fY1 : stB.fY1;
    const float fIx2 = (stA.fX2 < stB.fX2) ? stA.fX2 : stB.fX2;
    const float fIy2 = (stA.fY2 < stB.fY2) ? stA.fY2 : stB.fY2;

    if (fIx2 <= fIx1 || fIy2 <= fIy1)
    {
        return 0.0f;
    }
    const float fInter = (fIx2 - fIx1) * (fIy2 - fIy1);
    return fInter / (fAreaA + fAreaB - fInter);
}

bool touchesEdgeOf(const CRectf_S &stBox, int nCanvasW, int nCanvasH, int nEdgePx)
{
    if (nCanvasW <= 0 || nCanvasH <= 0)
    {
        return true;    /* 画幅都不明，视作不可判定而非「没贴边」：贴边会放行朝向判定 */
    }
    const float fEdge = static_cast<float>((nEdgePx > 0) ? nEdgePx : 0);

    /* 头框是外接框，触边往往只削掉发际几像素；判据必须落在头中心——
       中心出带（头已半出画）才真不可判。证据不足的后手在各分数闸。 */
    const float fCx = (stBox.fX1 + stBox.fX2) * 0.5f;
    const float fCy = (stBox.fY1 + stBox.fY2) * 0.5f;

    return (fCx <= fEdge) || (fCy <= fEdge) ||
           (fCx >= (static_cast<float>(nCanvasW) - fEdge)) ||
           (fCy >= (static_cast<float>(nCanvasH) - fEdge));
}

size_t decodeBoxes(const std::vector<float> &vecRaw, const CDetMap &rstMap,
                   const CDetectParam_S &rstParam, DetectSource_E enSource,
                   std::vector<CDetection_S> &rvecOut, CDetDecodeStat_S &rstStat)
{
    rvecOut.clear();

    if (!rstMap.isReady())
    {
        /* 映射没建起来却收到了输出，说明这一帧的输入图与映射不同源——
           丢框但必须留痕，否则表现是「偶发人数为 0」，无从下手 */
        ++rstStat.u64MapNotReady;
        if (shouldLogMalformed(rstStat.u64MapNotReady))
        {
            dlog_error("classroom decode: 映射未就绪，丢弃 %zu 个浮点 (#%llu)",
                       vecRaw.size(), (unsigned long long)rstStat.u64MapNotReady);
        }
        return 0;
    }

    const size_t nFullRows = vecRaw.size() / CLASSROOM_BOX_FLOAT_NUM;
    const size_t nTail = vecRaw.size() % CLASSROOM_BOX_FLOAT_NUM;
    if (0 != nTail)
    {
        /* 长度不整是后端与解析侧对框长理解不一致的信号（比如某后端把 mask 系数也打平进来），
           宁可丢最后半行也不要错位解析：错位的框会落在真框旁边，看起来像「多了一个人」。
           这类错一旦存在就是每帧都有，故与畸形框共用同一节流，否则一天能冲四十万行日志 */
        ++rstStat.u64Malformed;
        if (shouldLogMalformed(rstStat.u64Malformed))
        {
            dlog_warn("classroom decode: 原始长度 %zu 不是 %d 的整数倍",
                      vecRaw.size(), CLASSROOM_BOX_FLOAT_NUM);
        }
    }

    int32_t nCap = rstParam.nMaxBoxNum;
    if (nCap > CLASSROOM_MAX_PERSONS)
    {
        nCap = CLASSROOM_MAX_PERSONS;
    }
    if (nCap <= 0)
    {
        nCap = CLASSROOM_MAX_PERSONS;
    }

    const float fMinEdge = static_cast<float>((rstParam.nMinBoxEdge > 0) ? rstParam.nMinBoxEdge : 0);

    for (size_t i = 0; i < nFullRows; ++i)
    {
        const float *pafRow = &vecRaw[i * CLASSROOM_BOX_FLOAT_NUM];
        rstStat.u64Rows++;

        const float fX1 = pafRow[0];
        const float fY1 = pafRow[1];
        const float fX2 = pafRow[2];
        const float fY2 = pafRow[3];
        const float fScore = pafRow[4];
        const float fLabel = pafRow[5];

        if (!std::isfinite(fX1) || !std::isfinite(fY1) || !std::isfinite(fX2) ||
            !std::isfinite(fY2) || !std::isfinite(fScore) || !std::isfinite(fLabel))
        {
            /* NaN 进跟踪器的代价函数不报错，只会让匹配结果随机漂移，必须在此拦截 */
            ++rstStat.u64Malformed;
            if (shouldLogMalformed(rstStat.u64Malformed))
            {
                dlog_warn("classroom decode: %llu 行框数据非有限值，模型输出已损坏",
                          (unsigned long long)rstStat.u64Malformed);
            }
            continue;
        }

        if (fX2 <= fX1 || fY2 <= fY1)
        {
            ++rstStat.u64Malformed;
            continue;
        }

        const CRectf_S stCanvas = rstMap.modelToCanvas(CRectf_S{ fX1, fY1, fX2, fY2 });
        if (stCanvas.width() <= 0.0f || stCanvas.height() <= 0.0f)
        {
            ++rstStat.u64OffCanvas;
            continue;
        }

        /* 置信度门在画布坐标之后：
           远带按**画布**短边分带，配置里的像素线写的是画布口径，拿模型输入坐标
           分带，缩放一变"远近"就跟着变。折扣只给人头表——复核小图里的手机框
           个个大件，给折扣只会放进更多误检，DET_SRC_PHONE 照旧用基门 */
        const float fGate = (DetectSource_E::DET_SRC_HEAD == enSource)
                                ? headGateScore(stCanvas, rstParam)
                                : rstParam.fScoreThreshold;
        if (fScore < fGate)
        {
            ++rstStat.u64LowScore;
            continue;
        }

        if ((stCanvas.width() < fMinEdge) || (stCanvas.height() < fMinEdge))
        {
            ++rstStat.u64TooSmall;
            continue;
        }

        /* 巨大"头"框与过小同罪——1080p 里装不下 240px 的真人头，
           这种框只可能是深色板面上的幻影（或贴脸失真），下游谁都消不了它：
           框内含真人时姿态反而有输出，留着就是教师槽易主。
           计数单独记，与 small 一样读 diff */
        if (rstParam.nMaxBoxEdge > 0 &&
            (stCanvas.width() > static_cast<float>(rstParam.nMaxBoxEdge) ||
             stCanvas.height() > static_cast<float>(rstParam.nMaxBoxEdge)))
        {
            ++rstStat.u64TooLarge;
            continue;
        }

        if (rvecOut.size() >= static_cast<size_t>(nCap))
        {
            rstStat.u64Truncated += (nFullRows - i);
            break;
        }

        CDetection_S stDet;
        stDet.stBox = stCanvas;
        stDet.fScore = fScore;
        stDet.nLabel = static_cast<int32_t>(fLabel);
        stDet.enSource = enSource;
        rvecOut.push_back(stDet);
        ++rstStat.u64Accepted;
    }

    return rvecOut.size();
}

float faceGateScore(const CRectf_S &stCanvas, const CFaceDecodeParam_S &rstParam)
{
    /* 配错折扣（<=0 / >=1）当没配：这条门存在的目的是把后排的脸放进来，
       任何形态都不得反过来把门抬得比基门更严 */
    if (rstParam.fFarGateRatio <= 0.0f || rstParam.fFarGateRatio >= 1.0f ||
        rstParam.nNearEdge <= 0)
    {
        return rstParam.fScoreThreshold;
    }
    const float fShort = std::min(stCanvas.width(), stCanvas.height());
    if (fShort >= static_cast<float>(rstParam.nNearEdge))
    {
        return rstParam.fScoreThreshold;
    }
    return rstParam.fScoreThreshold * rstParam.fFarGateRatio;
}

float headGateScore(const CRectf_S &stCanvas, const CDetectParam_S &rstParam)
{
    /* 与 faceGateScore 同一条钳制：这条门存在的目的是把后排的头放进来，
       任何形态都不得反过来把门抬得比基门更严 */
    if (rstParam.fFarGateRatio <= 0.0f || rstParam.fFarGateRatio >= 1.0f ||
        rstParam.nFarEdgePx <= 0)
    {
        return rstParam.fScoreThreshold;
    }
    const float fShort = std::min(stCanvas.width(), stCanvas.height());
    if (fShort >= static_cast<float>(rstParam.nFarEdgePx))
    {
        return rstParam.fScoreThreshold;
    }
    return rstParam.fScoreThreshold * rstParam.fFarGateRatio;
}

size_t decodeFaces(const std::vector<float> &vecRaw, const CDetMap &rstMap,
                   const CFaceDecodeParam_S &rstParam,
                   std::vector<CFaceDet_S> &rvecOut, CFaceDecodeStat_S &rstStat)
{
    rvecOut.clear();

    if (!rstMap.isReady())
    {
        ++rstStat.u64MapNotReady;
        if (shouldLogMalformed(rstStat.u64MapNotReady))
        {
            dlog_error("classroom face decode: 映射表未就绪，%zu 个浮点被丢弃 (#%llu)",
                       vecRaw.size(), (unsigned long long)rstStat.u64MapNotReady);
        }
        return 0;
    }

    const size_t nFullRows = vecRaw.size() / CLASSROOM_FACE_BOX_FLOAT_NUM;
    if (0 != vecRaw.size() % CLASSROOM_FACE_BOX_FLOAT_NUM)
    {
        /* 长度不整只可能是「生产者与消费者对于每行几个 float 理解不一致」。
           这里丢的是尾行而不是首行：错位解析会把下一行的点当成上一行的坐标，
           整批结果全废；丢尾行只少一张脸，且这一条日志会说明为什么少 */
        ++rstStat.u64Malformed;
        if (shouldLogMalformed(rstStat.u64Malformed))
        {
            dlog_warn("classroom face decode: 原始长度 %zu 不是 %d 的整数倍",
                      vecRaw.size(), CLASSROOM_FACE_BOX_FLOAT_NUM);
        }
    }

    int32_t nCap = rstParam.nMaxBoxNum;
    if (nCap > CLASSROOM_MAX_PERSONS || nCap <= 0)
    {
        nCap = CLASSROOM_MAX_PERSONS;
    }
    const float fMinEdge = static_cast<float>((rstParam.nMinBoxEdge > 0) ? rstParam.nMinBoxEdge : 0);

    for (size_t i = 0; i < nFullRows; ++i)
    {
        const float *pafRow = &vecRaw[i * CLASSROOM_FACE_BOX_FLOAT_NUM];
        rstStat.u64Rows++;

        bool bFinite = true;
        for (int nCol = 0; nCol < CLASSROOM_FACE_BOX_FLOAT_NUM; ++nCol)
        {
            if (!std::isfinite(pafRow[nCol]))
            {
                bFinite = false;
                break;
            }
        }
        if (!bFinite)
        {
            /* 点里的 NaN 比框里的更要紧：框坏了顶多少一个人，点坏了会把对齐解算
               的残差污染成一个看起来"刚好过门"的数（NaN 参与比较一律 false，
               于是"不合格"这条判据反而不成立） */
            ++rstStat.u64Malformed;
            if (shouldLogMalformed(rstStat.u64Malformed))
            {
                dlog_warn("classroom face decode: %llu 行人脸含非有限值，模型输出损坏",
                          (unsigned long long)rstStat.u64Malformed);
            }
            continue;
        }

        /* 倒挂框先于一切换算：modelToCanvas 按轴对齐映射，倒挂框进去未必出不来一个
           "看着合法"的框——废行拦在换算之前，账才落在 malformed 上 */
        if (pafRow[2] <= pafRow[0] || pafRow[3] <= pafRow[1])
        {
            ++rstStat.u64Malformed;
            continue;
        }

        const CRectf_S stCanvas = rstMap.modelToCanvas(
            CRectf_S{ pafRow[0], pafRow[1], pafRow[2], pafRow[3] });
        if (stCanvas.width() <= 0.0f || stCanvas.height() <= 0.0f)
        {
            ++rstStat.u64OffCanvas;
            continue;
        }

        /* 置信度门按远近带取值：还够不到命名线的那段距离里，
           真脸的分数本来就更低，用近带的基门去拦，后排的人会先于尺寸门死在这里。
           门必须发生在画布坐标之后：配置里那两条像素线写的是画布口径，
           拿模型输入坐标的短边分带，缩放一变"远近"就跟着变 */
        if (pafRow[4] < faceGateScore(stCanvas, rstParam))
        {
            ++rstStat.u64LowScore;
            continue;
        }
        if ((stCanvas.width() < fMinEdge) || (stCanvas.height() < fMinEdge))
        {
            ++rstStat.u64TooSmall;
            continue;
        }

        if (rvecOut.size() >= static_cast<size_t>(nCap))
        {
            rstStat.u64Truncated += (nFullRows - i);
            break;
        }

        CFaceDet_S stFace;
        stFace.stBox = stCanvas;
        stFace.fScore = pafRow[4];

        /* 点走**同一个** map：单独换算一次就是留第二套坐标口径，
           镜像/旋转配反时的现场形态是"框是对的、点是镜像的"——对齐照样解得出矩阵，
           解出来的是一张拧着的脸，而这条链路上没有任何一处会报错 */
        const float fMargin = 0.5f;    /* 半个像素：整数框与浮点框之间本就有取整差 */
        for (int nPt = 0; nPt < CLASSROOM_FACE_LANDMARK_NUM; ++nPt)
        {
            const float *pafPt = &pafRow[CLASSROOM_BOX_FLOAT_NUM + nPt * 3];
            stFace.astPoints[nPt] = rstMap.modelToCanvas(CPointf_S{ pafPt[0], pafPt[1] });
            stFace.afPointScore[nPt] = pafPt[2];

            const CPointf_S &stP = stFace.astPoints[nPt];
            if (stP.fX < stCanvas.fX1 - fMargin || stP.fX > stCanvas.fX2 + fMargin ||
                stP.fY < stCanvas.fY1 - fMargin || stP.fY > stCanvas.fY2 + fMargin)
            {
                /* 只计数不丢框：检测框本来就常把下巴裁掉，真按"点必须在框内"丢框，
                   留下的有效样本会系统性偏向正脸大框——于是识别率与验收口径一起偏，
                   而且是在设备上一切正常的那种偏 */
                ++rstStat.u64PointOutOfBox;
            }
        }

        rvecOut.push_back(stFace);
        ++rstStat.u64Accepted;
    }

    return rvecOut.size();
}

void assignFaceRows(const std::vector<CFaceDet_S> &vecFace, CPersonFeature_S *patRows,
                    int32_t nRowNum, const std::vector<CTrackItem_S> &vecTrack,
                    const std::vector<int32_t> &vecRowOfHead,
                    std::vector<int32_t> &rvecRowOfFace, int32_t &rnDropped)
{
    rvecRowOfFace.assign(vecFace.size(), -1);
    rnDropped = 0;
    if (vecFace.empty() || nullptr == patRows || nRowNum <= 0)
    {
        return;
    }

    for (size_t f = 0; f < vecFace.size(); ++f)
    {
        const CRectf_S &stFace = vecFace[f].stBox;
        int32_t nBest = -1;

        /* 1) ID 链：轨迹本帧命中的头框 → 行。多个人各自命中的头框重叠时取轨迹表里靠前的那个，
              结果确定可复现（同一个人连续两拍配到不同行，比偶尔配错一行糟得多——
              时长门会把它切成两段事件） */
        for (size_t t = 0; t < vecTrack.size() && nBest < 0; ++t)
        {
            const int32_t nHead = vecTrack[t].nDetIdx;
            if (nHead < 0 || nHead >= static_cast<int32_t>(vecRowOfHead.size()))
            {
                continue;
            }
            const int32_t nRow = vecRowOfHead[static_cast<size_t>(nHead)];
            if (nRow >= 0 && nRow < nRowNum && containRatioOf(stFace, patRows[nRow].stHeadBox) > 0.0f)
            {
                nBest = nRow;
            }
        }

        /* 2) 几何兜底（ID 链断了：这拍该轨迹没命中头框，或那个人根本没进特征行）。
              取「脸落在头框内的比例」最大者。刻意不再加一道比例门：真正的判据在下游
              那条命名链上（尺寸 120 + 质量 + 分数 + 两次投票），这里再加一道只会让
              "为什么没名字"多出第四个可能原因，而四个比两个难查 */
        if (nBest < 0)
        {
            float fBest = 0.0f;
            for (int32_t r = 0; r < nRowNum; ++r)
            {
                const float fHit = containRatioOf(stFace, patRows[r].stHeadBox);
                if (fHit > fBest)
                {
                    fBest = fHit;
                    nBest = r;
                }
            }
        }
        rvecRowOfFace[f] = nBest;
    }

    /* 一行只留一张脸：同拍多张时留分数最高的（另一张多半是邻座飘过来的侧脸）。
       两张都写会怎样：账本会先给这行绑一张脸、下一拍又绑另一张，
       投票永远攒不够 nMinMatchHits，最后是**两个人都认不出**——比留一张准的更糟 */
    for (int32_t r = 0; r < nRowNum; ++r)
    {
        int32_t nKeep = -1;
        for (size_t f = 0; f < vecFace.size(); ++f)
        {
            if (rvecRowOfFace[f] != r)
            {
                continue;
            }
            if (nKeep < 0 || vecFace[f].fScore > vecFace[static_cast<size_t>(nKeep)].fScore)
            {
                nKeep = static_cast<int32_t>(f);
            }
        }
        if (nKeep < 0)
        {
            continue;
        }
        for (size_t f = 0; f < vecFace.size(); ++f)
        {
            if (rvecRowOfFace[f] == r && static_cast<int32_t>(f) != nKeep)
            {
                rvecRowOfFace[f] = -1;
                ++rnDropped;
            }
        }
    }
}

CRectf_S expandHeadToBody(const CFrameGeometry &stSrcToCanvas, const CRectf_S &stHead,
                          float fWidthHead, float fDownHead, float fUpHead)
{
    const float fW = stHead.width();
    const float fH = stHead.height();

    CRectf_S stOut = stHead;
    stOut.fX1 = stHead.fX1 - fWidthHead * fW;
    stOut.fX2 = stHead.fX2 + fWidthHead * fW;
    stOut.fY1 = stHead.fY1 - fUpHead * fH;
    stOut.fY2 = stHead.fY2 + fDownHead * fH;

    /* 外扩量按头宽/头高给，画幅边界却只有 frame_geometry 知道，故钳制放在这里而不是调用方：
       谁都能忘，忘了的后果是「一整块背景喂进姿态模型」 */
    const float fMaxX = static_cast<float>(stSrcToCanvas.dstWidth());
    const float fMaxY = static_cast<float>(stSrcToCanvas.dstHeight());

    stOut.fX1 = clampValue(stOut.fX1, 0.0f, fMaxX);
    stOut.fY1 = clampValue(stOut.fY1, 0.0f, fMaxY);
    stOut.fX2 = clampValue(stOut.fX2, 0.0f, fMaxX);
    stOut.fY2 = clampValue(stOut.fY2, 0.0f, fMaxY);

    if (stOut.fX2 < stOut.fX1)
    {
        stOut.fX2 = stOut.fX1;
    }
    if (stOut.fY2 < stOut.fY1)
    {
        stOut.fY2 = stOut.fY1;
    }
    return stOut;
}

void assemblePersons(const std::vector<CDetection_S> &vecHead,
                     const std::vector<CDetection_S> &vecBody,
                     const CFrameGeometry &stSrcToCanvas,
                     const CBodyAssembleParam_S &rstParam,
                     std::vector<CPersonBox_S> &rvecPerson,
                     CBodyAssembleStat_S &rstStat)
{
    rvecPerson.clear();

    rstStat.u64HeadNum += vecHead.size();
    rstStat.u64BodyNum += vecBody.size();

    if (!stSrcToCanvas.isReady())
    {
        rstStat.u64Expanded += vecHead.size();
        return;
    }

    /* 位图容量上限：解码侧已按 CLASSROOM_MAX_PERSONS 截断，这里再兜一次，
       因为本函数也可能被单测/回放以任意长度调用 */
    const size_t nBodyNum = (vecBody.size() > static_cast<size_t>(CLASSROOM_MAX_PERSONS))
                          ? static_cast<size_t>(CLASSROOM_MAX_PERSONS) : vecBody.size();
    uint64_t u64BodyUsed = 0;

    for (size_t i = 0; i < vecHead.size(); ++i)
    {
        const CDetection_S &stHead = vecHead[i];

        CPersonBox_S stPerson;
        stPerson.stHead = stHead.stBox;
        stPerson.fHeadScore = stHead.fScore;
        stPerson.nHeadIdx = static_cast<int32_t>(i);

        int32_t nBest = -1;
        float fBestArea = 0.0f;

        for (size_t j = 0; j < nBodyNum; ++j)
        {
            if (0 != (u64BodyUsed & (static_cast<uint64_t>(1) << j)))
            {
                continue;   /* 一个体框只服务一个头，避免前后排两人共用一框 */
            }

            const CDetection_S &stBody = vecBody[j];
            if ((stBody.stBox.width() < static_cast<float>(rstParam.nMinBodyEdge)) ||
                (stBody.stBox.height() < static_cast<float>(rstParam.nMinBodyEdge)))
            {
                continue;   /* 远处退化的体框：宁回落外扩，也不让一个「半个人」当人体证据 */
            }

            if (containRatioOf(stHead.stBox, stBody.stBox) < rstParam.fContain)
            {
                continue;
            }

            /* 取面积最小的包住者：大框通常是前后排粘连出来的，选它会让两人的体框重叠 */
            if (nBest < 0 || stBody.stBox.area() < fBestArea)
            {
                nBest = static_cast<int32_t>(j);
                fBestArea = stBody.stBox.area();
            }
        }

        if (nBest >= 0)
        {
            u64BodyUsed |= (static_cast<uint64_t>(1) << nBest);
            stPerson.stBody = vecBody[nBest].stBox;
            stPerson.nBodyIdx = nBest;
            ++rstStat.u64Matched;
        }
        else
        {
            stPerson.stBody = expandHeadToBody(stSrcToCanvas, stHead.stBox,
                                               rstParam.fWidthHead, rstParam.fDownHead,
                                               rstParam.fUpHead);
            ++rstStat.u64Expanded;
        }

        rvecPerson.push_back(stPerson);
    }

    uint64_t u64UsedNum = 0;
    for (uint64_t u64Mask = u64BodyUsed; 0 != u64Mask; u64Mask &= (u64Mask - 1))
    {
        ++u64UsedNum;
    }
    if (nBodyNum > static_cast<size_t>(u64UsedNum))
    {
        rstStat.u64BodyUnmatched += (nBodyNum - static_cast<size_t>(u64UsedNum));
    }
}
}
