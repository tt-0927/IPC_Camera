/**
 * @FilePath     : ocr_pipeline.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-04 11:07:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-04 11:07:00
 * @Description  : 板书 OCR 编排层的实现
 */

#include "ocr_pipeline.h"

#include <algorithm>
#include <cmath>

namespace Classroom
{
IOcrStagePort::~IOcrStagePort() = default;

namespace
{
/* 一帧最多补 2 次推理。空串那一档最常见的原因是行框把字的
   上下沿切掉了（DB 的 unclip 在下沿比上沿更保守），补一次"上下各扩 1/4 高"的重裁
   往往就出字；但整块板都是噪声时逐行都补等于把 rec 开销翻倍，而"这一帧本来就认不出"
   不会因为多跑几次而变——所以预算按帧给，不按行走 */
constexpr int32_t CLASSROOM_OCR_RETRY_FRAME_BUDGET = 2;

/* 只在竖直方向扩：横向的 unclip 是 DB 给的，再往外扩会把相邻两行切进来 */
bool expandRowBox(COcrBox_S &rstBox, int32_t nImgH)
{
    const int32_t nGrow = std::max(2, rstBox.height() / 4);
    rstBox.nY1 = std::max(0, rstBox.nY1 - nGrow);
    rstBox.nY2 = std::min(nImgH, rstBox.nY2 + nGrow);
    return rstBox.width() >= 2 && rstBox.height() >= 2;
}

/* 一个候选读法。det 装 (W,H)，rec 装 (T,C)——两者的判据不同但"收几个候选"这件事相同，
   所以共用一个类型而不是各写一遍（各写一遍的后果是将来只有一边补上"去重"这条规则） */
typedef struct _ShapePick_
{
    int32_t  nFirst = 0;        /* det 的宽 / rec 的步数 */
    uint32_t u32Second = 0;     /* det 的高 / rec 的类别数 */
    uint32_t u32Shift = 0;      /* 只有 rec 用得到（字典偏移） */
} CShapePick_S;

/**
 * @brief "唯一自洽读法"的容器：只存一个，第二个不同值本身就是结论
 * @note  这里刻意不留数组。规则是"两种排法都自洽 ⇒ 这一路判不可用"，所以一旦收下第二个
 *        **数值不同**的候选就没有任何后续动作需要它——存下来再数，就必然出现"数完还是挑一个"
 *        的那条路。本文件前一版写成容量 2 的数组，第二个不同值被安静收下、at(0) 取了第一个，
 *        而 [1,T,C] 与 [1,C,T] 两种读法同时自洽恰恰是 rec 的常态（T×C 的积对称），
 *        等于规则明令禁止的"取默认"。改成一格之后歧义在 add() 里就退出了。
 */
class CShapePick
{
public:
    /**
     * @brief   : 收下一个候选
     * @return   {bool} false=这是**第二个数值不同**的自洽读法（歧义，调用方立刻判不可用）；
     *                  true=收下，或与已有候选完全相同（视作同一个读法，丢掉）
     * @note    : "数值完全相同 ⇒ 同一个读法"是本文件唯一一处去重，理由要看它挡的是什么：
     *            [1,1,240,240] 的方形概率图，(a=2,b=3) 与 (a=3,b=2) 两个有序组合给出
     *            **同一个** (240,240)。不去重就把"模型自己报了个正方形"读成
     *            "两种排法都自洽"而判不可用——那是把最常见的一种形状误杀。
     *            比较只看 (nFirst,u32Second)：shift 是由 (T,C) 经 ctcClassMap 算出的，
     *            同一对 (T,C) 不可能给出两个 shift，比它一次就是第三说法
     */
    bool add(int32_t nFirst, uint32_t u32Second, uint32_t u32Shift)
    {
        if (m_bTaken)
        {
            return (m_stPick.nFirst == nFirst && m_stPick.u32Second == u32Second);
        }
        m_stPick.nFirst = nFirst;
        m_stPick.u32Second = u32Second;
        m_stPick.u32Shift = u32Shift;
        m_bTaken = true;
        return true;
    }

    bool taken() const { return m_bTaken; }
    const CShapePick_S &value() const { return m_stPick; }

private:
    CShapePick_S m_stPick;
    bool m_bTaken = false;
};

/* 有向宽高比是否自洽。任一侧为 0 都由调用方挡（这里的除法不做保护：
   传 0 进来是编程错误，而它会在 ASan/UBSan 闸门里当场炸，比在这里静默返回 false 好找） */
bool aspectOk(int32_t nW, int32_t nH, int32_t nImgW, int32_t nImgH)
{
    const double dMap = static_cast<double>(nW) / static_cast<double>(nH);
    const double dImg = static_cast<double>(nImgW) / static_cast<double>(nImgH);
    return std::fabs(dMap - dImg) <= static_cast<double>(CLASSROOM_OCR_ASPECT_TOLERANCE) * dImg;
}
}

bool resolveDetMapShape(const CRawTensorShape_S &rstShape, int32_t nImgW, int32_t nImgH,
                        COcrDetShape_S &rstOut)
{
    rstOut = COcrDetShape_S();
    /* 维数 <2 时没有任何"两个维"可选；u32Elems 不必再判 0——fillFromRaw 保证过了它
       就意味着每一维都非 0，而单维形状已由这一条挡掉 */
    if (rstShape.u32NumDims < 2 || nImgW <= 0 || nImgH <= 0)
    {
        return false;
    }

    CShapePick stPick;
    for (uint32_t u32A = 0; u32A < rstShape.u32NumDims; ++u32A)
    {
        for (uint32_t u32B = 0; u32B < rstShape.u32NumDims; ++u32B)
        {
            if (u32A == u32B)
            {
                continue;
            }
            const uint64_t ullW = rstShape.u32Dim[u32A];
            const uint64_t ullH = rstShape.u32Dim[u32B];
            if (ullW * ullH != static_cast<uint64_t>(rstShape.u32Elems))
            {
                continue;
            }
            /* 乘积等于元素数就已经保证两边都 ≤ uint32，但 int32 那一格还要自己判：
               越界不是"数错了"而是 UB，而 UB 在这里的表现是 nMapW 变成负数，
               接下来会被下游当成合法尺寸用（dbDecodeTextBoxes 只判 <=0 的那一档） */
            if (ullW > static_cast<uint64_t>(INT32_MAX) || ullH > static_cast<uint64_t>(INT32_MAX))
            {
                continue;
            }
            if (!aspectOk(static_cast<int32_t>(ullW), static_cast<int32_t>(ullH), nImgW, nImgH))
            {
                continue;
            }
            if (!stPick.add(static_cast<int32_t>(ullW), static_cast<uint32_t>(ullH), 0u))
            {
                return false;   // 两个不同候选都自洽：歧义，不挑
            }
        }
    }

    /* 歧义（第二个不同读法）在 add() 里就当场退出了，所以这一道只判"一个候选都没有"：
       所有读法都不自洽 ⇒ 这一路解释不了，交调用方降级 */
    if (!stPick.taken())
    {
        return false;
    }
    rstOut.nMapW = stPick.value().nFirst;
    rstOut.nMapH = static_cast<int32_t>(stPick.value().u32Second);
    return true;
}

bool resolveRecShape(const CRawTensorShape_S &rstShape, int32_t nCanvasW,
                     const COcrTokenTable &rstToken, COcrRecShape_S &rstOut)
{
    rstOut = COcrRecShape_S();
    if (rstShape.u32NumDims < 2 || nCanvasW <= 0 || !rstToken.ready())
    {
        return false;
    }

    CShapePick stPick;
    for (uint32_t u32A = 0; u32A < rstShape.u32NumDims; ++u32A)
    {
        for (uint32_t u32B = 0; u32B < rstShape.u32NumDims; ++u32B)
        {
            if (u32A == u32B)
            {
                continue;
            }
            const uint64_t ullT = rstShape.u32Dim[u32A];
            const uint64_t ullC = rstShape.u32Dim[u32B];
            if (ullT * ullC != static_cast<uint64_t>(rstShape.u32Elems))
            {
                continue;
            }
            if (ullT > static_cast<uint64_t>(nCanvasW))
            {
                continue;
            }
            uint32_t u32Shift = 0;
            if (!ctcClassMap(static_cast<uint32_t>(ullC), rstToken, u32Shift))
            {
                continue;
            }
            /* 这里不再判 ullT 的 int32 上界：上面那道"步数 ≤ 画布宽"已经把它压到 int32 之内，
               再加一道就是第三种说法（与 raw_model_handle.h 里删掉 0==u32Elems 同一口径） */
            if (!stPick.add(static_cast<int32_t>(ullT), static_cast<uint32_t>(ullC), u32Shift))
            {
                return false;
            }
        }
    }

    /* 同 det：歧义那一半（[T,C] 与 [C,T] 都过三道门）在 add() 里已经退出了，
       这一道判"没有任何读法过三道门" */
    if (!stPick.taken())
    {
        return false;
    }
    rstOut.nTimeStep = stPick.value().nFirst;
    rstOut.u32ClassNum = stPick.value().u32Second;
    rstOut.u32DictShift = stPick.value().u32Shift;
    return true;
}

OcrPipeStage_E ocrRunBoard(const COcrDetShape_S &rstDet, const COcrRecShape_S &rstRec,
                           int32_t nCanvasW, int32_t nCanvasH,
                           const COcrDbParam_S &stDbParam, const COcrTokenTable &rstToken,
                           IOcrStagePort &rstPort,
                           const uint8_t *pucRgb, int32_t nImgW, int32_t nImgH,
                           std::vector<CBoardText_S> &rvecOut,
                           COcrPipeStat_S *pstStat, COcrDbStat_S *pstDbStat)
{
    rvecOut.clear();

    /* 形状解不出来 / 字典没装载 / 图不给读：三种都在这里合并成一档 BAD_PARAM。
       不分开是因为它们的现场动作是同一句"这台机器的 OCR 这一路没配起来"，
       而区分它们靠的是加载期那几行日志，不是每次识别的返回值 */
    if (nullptr == pucRgb || nImgW <= 0 || nImgH <= 0 ||
        nCanvasW <= 0 || nCanvasH <= 0 ||
        rstDet.nMapW <= 0 || rstDet.nMapH <= 0 ||
        rstRec.nTimeStep <= 0 || 0 == rstRec.u32ClassNum || !rstToken.ready())
    {
        return OcrPipeStage_E::OCR_PIPE_BAD_PARAM;
    }

    std::vector<float> vecMap;
    if (!rstPort.runDet(pucRgb, nImgW, nImgH, vecMap))
    {
        return OcrPipeStage_E::OCR_PIPE_DET_FAIL;
    }
    if (vecMap.size() != static_cast<size_t>(rstDet.nMapW) * static_cast<size_t>(rstDet.nMapH))
    {
        return OcrPipeStage_E::OCR_PIPE_DET_SIZE;
    }

    std::vector<COcrBox_S> vecMapBox;
    const int32_t nBoxNum = dbDecodeTextBoxes(vecMap.data(), rstDet.nMapW, rstDet.nMapH,
                                             stDbParam, vecMapBox, pstDbStat);
    if (0 > nBoxNum)
    {
        return OcrPipeStage_E::OCR_PIPE_DB_REJECT;
    }
    if (0 == nBoxNum)
    {
        /* 空板是**成功**：一块干净的黑板与"这台机器认不出字"是两件事，
           混成一件事会让整节课读起来是一块空板 */
        if (nullptr != pstStat)
        {
            ++pstStat->u64FrameOk;
        }
        return OcrPipeStage_E::OCR_PIPE_OK;
    }

    std::vector<float> vecRec;
    rvecOut.reserve(static_cast<size_t>(nBoxNum));
    const size_t nCanvasBytes =
        static_cast<size_t>(nCanvasW) * static_cast<size_t>(nCanvasH) * 3u;
    const size_t nRecElems =
        static_cast<size_t>(rstRec.nTimeStep) * static_cast<size_t>(rstRec.u32ClassNum);

    /* 补推理的帧预算：跨行走完之前一直有效（见 CLASSROOM_OCR_RETRY_FRAME_BUDGET） */
    int32_t nRetryLeft = CLASSROOM_OCR_RETRY_FRAME_BUDGET;

    for (const COcrBox_S &stMapBox : vecMapBox)
    {
        if (nullptr != pstStat)
        {
            ++pstStat->u64BoxNum;
        }

        COcrBox_S stImgBox;
        scaleBoxToImage(stMapBox, rstDet.nMapW, rstDet.nMapH, nImgW, nImgH, stImgBox);
        /* 2 像素是裁剪档的下限（CCropUnit：钳制取偶后不足 2×2 直接拒），
           在到这里之前先挡掉，读数才分得清"框本来就太小"和"RGA 拒了" */
        if (stImgBox.width() < 2 || stImgBox.height() < 2)
        {
            if (nullptr != pstStat)
            {
                ++pstStat->u64BoxClip;
            }
            continue;
        }

        /* 补推理时裁的是这一份（只在空串后竖直扩），交出去的坐标仍用原框：
           框是 DB 说的那一行，扩出来的边只用来多给模型看一点上下文 */
        COcrBox_S stCropBox = stImgBox;
        bool bRowRetried = false;

        for (;;)
        {
            const int32_t nFilledW =
                recCanvasWidth(stCropBox.width(), stCropBox.height(), nCanvasW, nCanvasH);

            COcrRowCanvas_S stCanvas;
            if (!rstPort.cropRow(pucRgb, nImgW, nImgH, stCropBox, nCanvasW, nCanvasH, nFilledW, stCanvas) ||
                nullptr == stCanvas.pData || stCanvas.nBytes != nCanvasBytes)
            {
                /* RGA 拒绝是确定性的：同一个框重裁一次还是同一个结果，不拿预算去试 */
                if (nullptr != pstStat)
                {
                    ++pstStat->u64CropFail;
                }
                break;
            }

            if (!rstPort.runRec(stCanvas, vecRec))
            {
                /* rec 这一档多半是瞬时的（NPU 队列满 / 一次任务超时），补一次是值得的；
                   预算用完就按行失败记，读数与补之前一致 */
                if (nRetryLeft > 0)
                {
                    --nRetryLeft;
                    bRowRetried = true;
                    if (nullptr != pstStat)
                    {
                        ++pstStat->u64RowRetry;
                    }
                    continue;
                }
                if (nullptr != pstStat)
                {
                    ++pstStat->u64RecFail;
                }
                break;
            }
            if (vecRec.size() != nRecElems)
            {
                /* 整帧作废而不是跳过这一行：这一档说明 T 与 C 读错了，
                   于是**剩下的每一行都会解出乱码**，而乱码是带分数的、看起来完全正常。
                   少丢这一帧的代价（一次识别白跑）与读错一位的代价不成比例 */
                if (nullptr != pstStat)
                {
                    ++pstStat->u64RecSize;
                }
                rvecOut.clear();
                return OcrPipeStage_E::OCR_PIPE_REC_SIZE;
            }

            std::string strText;
            const float fScore = ctcGreedyDecode(vecRec.data(), rstRec.nTimeStep, rstRec.u32ClassNum,
                                                rstRec.u32DictShift, rstToken,
                                                CLASSROOM_OCR_LINE_TEXT_MAX, strText);
            if (strText.empty())
            {
                /* 分数为 0 与串为空是同一档：ctcGreedyDecode 在"整行都是 blank"与"字节上限装不下"
                   两种情况下都返回空串，两者都不该进报表。上限那一档由行宽决定，
                   板面行宽在 40 汉字以内（256 字节是它的两倍余量），真撞上就是阈值调坏了。
                   补一次上下扩框：最常见的成因是行框把字的上沿切掉，模型整行读成 blank */
                if (nRetryLeft > 0 && expandRowBox(stCropBox, nImgH))
                {
                    --nRetryLeft;
                    bRowRetried = true;
                    if (nullptr != pstStat)
                    {
                        ++pstStat->u64RowRetry;
                    }
                    continue;
                }
                if (nullptr != pstStat)
                {
                    ++pstStat->u64RowEmpty;
                }
                break;
            }

            CBoardText_S stText;
            stText.strText = strText;
            stText.fScore = fScore;
            stText.stBox.fX1 = static_cast<float>(stImgBox.nX1);
            stText.stBox.fY1 = static_cast<float>(stImgBox.nY1);
            stText.stBox.fX2 = static_cast<float>(stImgBox.nX2);
            stText.stBox.fY2 = static_cast<float>(stImgBox.nY2);
            rvecOut.push_back(stText);
            if (nullptr != pstStat)
            {
                ++pstStat->u64RowOut;
                if (bRowRetried)
                {
                    ++pstStat->u64RowRetryOk;
                }
            }
            break;
        }
    }

    if (rvecOut.empty())
    {
        /* 有框却一行都不出 ⇒ 失败。报成空板等于"后端坏了"被读成"老师没写字" */
        return OcrPipeStage_E::OCR_PIPE_REC_FAIL;
    }
    if (nullptr != pstStat)
    {
        ++pstStat->u64FrameOk;
    }
    return OcrPipeStage_E::OCR_PIPE_OK;
}
}
