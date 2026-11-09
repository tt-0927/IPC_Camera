/**
 * @FilePath     : ocr_decode.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-03 14:37:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 14:37:00
 * @Description  : PP-OCR 前后处理纯函数实现
 */

#include "ocr_decode.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "dlog.h"

namespace Classroom
{
namespace
{
/* 字典文件尺寸上限：交付件 6624 行 = 32,883 字节，给 1MB
   是把"拿错文件"和"内存失控"分开 */
const long OCR_TOKEN_FILE_MAX_BYTES = 1024 * 1024;

inline int32_t ocrMin(int32_t a, int32_t b) { return a < b ? a : b; }
inline int32_t ocrMax(int32_t a, int32_t b) { return a > b ? a : b; }

/* 概率图上的一个连通域：只留外接框与像素数。
   不存像素列表是因为后面用到的只有"外接框"与"框内平均概率"两个读数，
   而一行板的域可以有几万像素——留着它等于在栈上放一帧图的零头 */
typedef struct _Comp_
{
    int32_t nMinX = 0;
    int32_t nMinY = 0;
    int32_t nMaxX = 0;
    int32_t nMaxY = 0;
    int32_t nCount = 0;
} COcrComp_S;

/**
 * @brief   : 一个连通域能不能成为一个文本行框（含外扩）
 * @param    {const float*} pfProb：概率图首地址
 * @param    {int32_t} nStride：概率图行跨距（= 图宽）
 * @return   {bool} false 时按 stComp 的几何形状计入对应的丢弃档
 * @note    : 顺序照参照实现的非多边形分支来：先尺寸、再框内平均概率、再外扩、最后再判尺寸。
 *            平均概率取的是**外接框内全部像素**的均值（boxScoreFast 同一个口径），
 *            不是"域内像素"的均值——后者永远接近 1（域里的点都是过了 mask 门的），
 *            那道门就白设了：挡不住"一片糊开的反光"这类外接框很大而填充稀疏的东西
 */
bool compOk(const COcrComp_S &stComp, const float *pfProb, int32_t nStride,
            const COcrDbParam_S &stParam, COcrBox_S &rstBox, COcrDbStat_S *pstStat)
{
    const int32_t nW = stComp.nMaxX - stComp.nMinX + 1;
    const int32_t nH = stComp.nMaxY - stComp.nMinY + 1;
    /* 3 像素是参照实现的 nMinSize（sside < 3 直接 continue）：比这小的域连一个笔画都装不下。
       nCount<9 那条是同一道门的面积表述，放在这里只为躲掉下面那次外接框求和——
       一张噪声图上有几千个碎点，每个都走一遍框内求和是白付的 */
    if (nW < 3 || nH < 3 || stComp.nCount < 9)
    {
        if (pstStat) { pstStat->u64DropSmall++; }
        return false;
    }

    double dSum = 0.0;
    for (int32_t y = stComp.nMinY; y <= stComp.nMaxY; ++y)
    {
        const float *pfRow = pfProb + static_cast<size_t>(y) * nStride;
        for (int32_t x = stComp.nMinX; x <= stComp.nMaxX; ++x)
        {
            dSum += pfRow[x];
        }
    }
    const float fScore = static_cast<float>(dSum / static_cast<double>(nW * nH));
    if (fScore < stParam.fBoxThreshold)
    {
        if (pstStat) { pstStat->u64DropScore++; }
        return false;
    }

    /* 外扩（unclip）按矩形算：Clipper 对矩形做距离 d 的等距外扩，结果仍是这个矩形外扩 d，
       所以轴向版本与参照实现在"水平文字"这一族上给出同样的框。
       d = 面积×ratio/周长 = w×h×ratio / (2(w+h))——长行给出的 d 接近行高的一半，
       这正是"把上下各留一点空白"的意思 */
    const float fPerim = 2.0f * static_cast<float>(nW + nH);
    const float fDist = (fPerim > 0.0f)
                        ? (static_cast<float>(nW) * static_cast<float>(nH) * stParam.fUnclipRatio) / fPerim
                        : 0.0f;
    const int32_t nGrow = static_cast<int32_t>(std::floor(fDist));

    rstBox.nX1 = stComp.nMinX - nGrow;
    rstBox.nY1 = stComp.nMinY - nGrow;
    rstBox.nX2 = stComp.nMaxX + 1 + nGrow;   /* 半开区间：末坐标是"下一个像素" */
    rstBox.nY2 = stComp.nMaxY + 1 + nGrow;
    rstBox.fScore = fScore;

    if (rstBox.width() <= 4 || rstBox.height() <= 4)
    {
        /* 参照实现末尾那道 4 像素门：比这小的框连一次 rec 缩放都撑不满 */
        if (pstStat) { pstStat->u64DropGrow++; }
        return false;
    }
    return true;
}

/* 两框是否该并成一行：竖直重叠足够 + 水平间隙足够小（判据与上游 sorted_boxes 同） */
bool canMergeLine(const COcrBox_S &stA, const COcrBox_S &stB, const COcrDbParam_S &stParam)
{
    const int32_t nMinH = ocrMin(stA.height(), stB.height());
    if (nMinH <= 0)
    {
        return false;
    }
    const int32_t nOverlap = ocrMin(stA.nY2, stB.nY2) - ocrMax(stA.nY1, stB.nY1);
    if (static_cast<float>(nOverlap) < stParam.fMergeVOverlap * static_cast<float>(nMinH))
    {
        return false;
    }
    /* 相交时这个值是负的，负值一律放行（两块本来就叠着） */
    const int32_t nGapX = ocrMax(stA.nX1, stB.nX1) - ocrMin(stA.nX2, stB.nX2);
    return static_cast<float>(nGapX) <= stParam.fMergeHGap * static_cast<float>(nMinH);
}

void mergeInto(COcrBox_S &rstA, const COcrBox_S &stB)
{
    const int32_t nAreaA = rstA.width() * rstA.height();
    const int32_t nAreaB = stB.width() * stB.height();
    /* 分数按面积加权：一行里"三个字密实的一坨"与"一个字松散的一坨"不该等权 */
    const int32_t nSum = nAreaA + nAreaB;
    if (nSum > 0)
    {
        rstA.fScore = (rstA.fScore * static_cast<float>(nAreaA)
                       + stB.fScore * static_cast<float>(nAreaB)) / static_cast<float>(nSum);
    }
    rstA.nX1 = ocrMin(rstA.nX1, stB.nX1);
    rstA.nY1 = ocrMin(rstA.nY1, stB.nY1);
    rstA.nX2 = ocrMax(rstA.nX2, stB.nX2);
    rstA.nY2 = ocrMax(rstA.nY2, stB.nY2);
}
}

/* ---------------- 字典 ---------------- */

void COcrTokenTable::clear()
{
    m_strBlob.clear();
    m_vecOff.clear();
    m_bReady = false;
}

bool COcrTokenTable::loadFromText(const std::string &strText, std::string &rstrErr)
{
    clear();

    size_t nBegin = 0;
    /* 只吃文件开头那一处 BOM：中间出现的 EF BB BF 是数据（某种占位字符），不能碰 */
    if (strText.size() >= 3 && 0 == strText.compare(0, 3, "\xEF\xBB\xBF", 3))
    {
        nBegin = 3;
    }
    if (nBegin >= strText.size())
    {
        rstrErr = "字典为空";
        return false;
    }

    /* 逐行切：先按 \n 断，再丢掉行尾的 \r。
       行内空格**必须保留**——交付字典里空格是一个真实字符，把它 trim 掉就少一个词条，
       而词条数是 ctcClassMap 判一致性的依据，少一条的表现为「整份识别结果错一位」 */
    size_t nLine = nBegin;
    while (nLine < strText.size())
    {
        size_t nEnd = strText.find('\n', nLine);
        const bool bLast = (std::string::npos == nEnd);
        if (bLast)
        {
            nEnd = strText.size();
        }

        size_t nStop = nEnd;
        if (nStop > nLine && '\r' == strText[nStop - 1])
        {
            nStop--;
        }

        if (nStop == nLine && bLast)
        {
            /* 文件以换行结尾时的最后一段：那是文件写法，不是一个空词条 */
            nLine = nEnd + 1;
            continue;
        }

        if (m_vecOff.size() > CLASSROOM_OCR_TOKEN_MAX)
        {
            rstrErr = "字典条数超上限";
            clear();
            return false;
        }

        if (0 == m_vecOff.size())
        {
            m_vecOff.push_back(0);
        }
        m_strBlob.append(strText, nLine, nStop - nLine);
        m_vecOff.push_back(static_cast<uint32_t>(m_strBlob.size()));
        nLine = nEnd + 1;
    }

    if (m_vecOff.size() < 2 || m_strBlob.empty())
    {
        /* 两条门是两件事：条数为 0 是"没切出行"，条数大于 0 而字节数为 0 是"每一行都是空行"。
           后者更该挡：这样的表 ready() 为真、numTokens() 也为真，但 appendToken 对任何下标
           都返回 false —— 每一条板书都会解成空串、置信度 0，而报表上看起来是"这节课没写字"，
           与"字典文件装坏了"是两种要区别对待的现象（PII 红线之外，这更是"坏了要说"） */
        rstrErr = "字典没有一条有效词条";
        clear();
        return false;
    }
    m_bReady = true;
    return true;
}

bool COcrTokenTable::loadFromFile(const std::string &strPath, std::string &rstrErr)
{
    FILE *pf = fopen(strPath.c_str(), "rb");
    if (nullptr == pf)
    {
        rstrErr = "打不开: " + strPath;
        return false;
    }
    if (0 != fseek(pf, 0, SEEK_END))
    {
        fclose(pf);
        rstrErr = "seek 失败: " + strPath;
        return false;
    }
    const long lSize = ftell(pf);
    rewind(pf);
    if (lSize <= 0 || lSize > OCR_TOKEN_FILE_MAX_BYTES)
    {
        fclose(pf);
        rstrErr = "尺寸异常: " + std::to_string(lSize);
        return false;
    }

    std::string strText;
    try
    {
        strText.resize(static_cast<size_t>(lSize));
    }
    catch (const std::bad_alloc &)
    {
        fclose(pf);
        rstrErr = "无内存";
        return false;
    }
    const size_t nRead = fread(&strText[0], 1, static_cast<size_t>(lSize), pf);
    fclose(pf);
    if (nRead != static_cast<size_t>(lSize))
    {
        rstrErr = "读不满";
        return false;
    }

    if (!loadFromText(strText, rstrErr))
    {
        return false;
    }
    dlog_info("ocr_decode: 字典 %s 共 %u 条 %zu 字节", strPath.c_str(), numTokens(),
              m_strBlob.size());
    return true;
}

bool COcrTokenTable::appendToken(uint32_t nIndex, std::string &rstrOut, int32_t nLimitBytes) const
{
    if (!m_bReady || nIndex + 1 >= m_vecOff.size())
    {
        return false;
    }
    const uint32_t nFrom = m_vecOff[nIndex];
    const uint32_t nTo = m_vecOff[nIndex + 1];
    if (nTo <= nFrom)
    {
        return false;   /* 空词条：不追加，让调用方按「这个字没认出来」处理 */
    }
    const int32_t nLen = static_cast<int32_t>(nTo - nFrom);
    /* 0 与负数一律当「一个字节都装不下」，不是「不设限」：这道门存在的理由就是
       给上行文本一个硬上界，把 0 解释成无限等于让「配置漏了一项」变成无界追加 */
    if (static_cast<int32_t>(rstrOut.size()) + nLen > nLimitBytes)
    {
        return false;
    }
    rstrOut.append(m_strBlob, nFrom, nTo - nFrom);
    return true;
}

std::string ocrDictPathOf(const std::string &strRecModelPath)
{
    const size_t nPos = strRecModelPath.find_last_of('/');
    if (std::string::npos == nPos)
    {
        /* 空串、或只有文件名：没有目录可依附。不猜默认目录（理由见头文件） */
        return std::string();
    }
    return strRecModelPath.substr(0, nPos + 1) + CLASSROOM_OCR_DICT_NAME;
}

/* ---------------- DB 检测解码 ---------------- */

int32_t dbDecodeTextBoxes(const float *pfProbMap, int32_t nMapW, int32_t nMapH,
                          const COcrDbParam_S &stParam, std::vector<COcrBox_S> &rvecBox,
                          COcrDbStat_S *pstStat)
{
    rvecBox.clear();
    if (nullptr == pfProbMap || nMapW <= 0 || nMapH <= 0)
    {
        return -1;
    }

    const size_t nPx = static_cast<size_t>(nMapW) * static_cast<size_t>(nMapH);
    std::vector<uint8_t> vecMask;
    std::vector<int32_t> vecStack;
    try
    {
        vecMask.assign(nPx, 0u);
        vecStack.reserve(CLASSROOM_OCR_CCL_STACK_MAX);
    }
    catch (const std::bad_alloc &)
    {
        dlog_warn("ocr_decode: 概率图 %dx%d 的工作缓冲申请失败", nMapW, nMapH);
        return -1;
    }

    /* 步 1：二值化。同时校验「这确实是一张概率图」——取值全在 0~1 之外时
       说明模型尾部没有 sigmoid，或者喂进去的是一张错的张量。
       这里不当 0 处理是有意的：当成空板会把「整条 OCR 路配置错了」读成「这节课老师没写字」 */
    uint32_t u32OutRange = 0;
    for (size_t i = 0; i < nPx; ++i)
    {
        const float fP = pfProbMap[i];
        if (!(fP >= 0.0f && fP <= 1.0f))
        {
            ++u32OutRange;
        }
        if (fP >= stParam.fMaskThreshold)
        {
            vecMask[i] = 1u;
        }
    }
    if (u32OutRange == nPx)
    {
        dlog_warn("ocr_decode: 概率图 %dx%d 全部落在 0~1 之外，不是概率图", nMapW, nMapH);
        return -1;
    }

    /* 步 2+3+4：连通域 → 判据 → 外扩。BFS 的容器是堆上的 vector，
       上限（CCL_STACK_MAX）只作"可疑域"的指示灯，**不再当场作废整个域**；
       是否丢弃由 compOk 的平均概率门判。详见下面循环内的 #160 注释。 */
    std::vector<COcrBox_S> vecBox;
    COcrComp_S stComp;
    int32_t nW = nMapW;
    int32_t nH = nMapH;
    for (int32_t y = 0; y < nH; ++y)
    {
        for (int32_t x = 0; x < nW; ++x)
        {
            const size_t nSeed = static_cast<size_t>(y) * nW + x;
            if (0 == vecMask[nSeed])
            {
                continue;
            }
            vecMask[nSeed] = 0u;
            stComp.nMinX = stComp.nMaxX = x;
            stComp.nMinY = stComp.nMaxY = y;
            stComp.nCount = 1;
            vecStack.clear();
            vecStack.push_back(static_cast<int32_t>(nSeed));

            /* 第一处：BFS 走完，不因"域大"当场弃掉整个域。
               旧实现在 vecStack 触顶时把整块域作废——反光把一大片区域连成一个巨域时，
               那一片里的**所有字**跟着一起没了（"整行一个字都不出"），
               而日志里只有 u64StackFull 在涨，没有字段能指出丢了哪几行。
               现在：遍历照走完（堆内存上界 = 概率图像素数，不会爆），
               触顶只记一个可疑计数，最终是否丢由 compOk 的平均概率门判——
               "这个域是不是字"本来就只有那一道门有资格判。

               另外修掉一个伴随 Bug：旧代码把 `vecMask[nNi]=0` 写在触顶判定**之前**，
               于是被判"太大了"的那个邻居被标记成已访问却没有入栈 —— 它既不属于
               任何已走完的域，也永远不会再被扫到，那一个像素被静默漏检。 */
            bool bTooLarge = false;
            while (!vecStack.empty())
            {
                const int32_t nIdx = vecStack.back();
                vecStack.pop_back();
                const int32_t nCx = nIdx % nW;
                const int32_t nCy = nIdx / nW;
                for (int32_t nDy = -1; nDy <= 1; ++nDy)
                {
                    const int32_t nNy = nCy + nDy;
                    if (nNy < 0 || nNy >= nH)
                    {
                        continue;
                    }
                    for (int32_t nDx = -1; nDx <= 1; ++nDx)
                    {
                        if (0 == nDx && 0 == nDy)
                        {
                            continue;
                        }
                        const int32_t nNx = nCx + nDx;
                        if (nNx < 0 || nNx >= nW)
                        {
                            continue;
                        }
                        const size_t nNi = static_cast<size_t>(nNy) * nW + nNx;
                        if (0 == vecMask[nNi])
                        {
                            continue;
                        }
                        if (vecStack.size() >= static_cast<size_t>(CLASSROOM_OCR_CCL_STACK_MAX))
                        {
                            /* 只记"这个域可疑"，**不清 vecMask**——该像素留给后续正常 BFS 收走，
                               这样漏检像素会被下一个种子重新访问到，而不是凭空消失 */
                            bTooLarge = true;
                            continue;
                        }
                        vecMask[nNi] = 0u;
                        vecStack.push_back(static_cast<int32_t>(nNi));
                        if (nNx < stComp.nMinX) { stComp.nMinX = nNx; }
                        if (nNx > stComp.nMaxX) { stComp.nMaxX = nNx; }
                        if (nNy < stComp.nMinY) { stComp.nMinY = nNy; }
                        if (nNy > stComp.nMaxY) { stComp.nMaxY = nNy; }
                        stComp.nCount++;
                    }
                }
            }

            if (pstStat) { pstStat->u64CompNum++; }
            if (bTooLarge && pstStat) { pstStat->u64StackFull++; }

            COcrBox_S stBox;
            if (!compOk(stComp, pfProbMap, nW, stParam, stBox, pstStat))
            {
                continue;
            }
            /* 钳进图内：外扩可以越界，rec 裁剪不接受越界框 */
            stBox.nX1 = ocrMax(0, stBox.nX1);
            stBox.nY1 = ocrMax(0, stBox.nY1);
            stBox.nX2 = ocrMin(nW, stBox.nX2);
            stBox.nY2 = ocrMin(nH, stBox.nY2);
            if (stBox.width() <= 4 || stBox.height() <= 4)
            {
                if (pstStat) { pstStat->u64DropGrow++; }
                continue;
            }
            vecBox.push_back(stBox);
        }
    }

    /* 步 5：同行碎片合并。反复扫到不再变化为止，最多 8 轮——
       只扫一轮的话「A 先和 B 并、之后本可以和 C 并」的链条会在中间断掉，
       而中文板书恰好是最容易断成三块的情形。8 轮对 32 个框是纯算术 */
    for (int32_t nPass = 0; nPass < 8 && vecBox.size() > 1; ++nPass)
    {
        bool bChanged = false;
        for (size_t i = 0; i < vecBox.size(); ++i)
        {
            size_t j = i + 1;
            while (j < vecBox.size())
            {
                if (canMergeLine(vecBox[i], vecBox[j], stParam))
                {
                    mergeInto(vecBox[i], vecBox[j]);
                    vecBox.erase(vecBox.begin() + j);
                    bChanged = true;
                    if (pstStat) { pstStat->u64Merged++; }
                }
                else
                {
                    ++j;
                }
            }
        }
        if (!bChanged)
        {
            break;
        }
    }

    /* 步 6：阅读序 + 截断。先排序再截，截掉的才是"最后几行"而不是"随机几行"。
       按竖直中心为主键：同一行内按 x，行与行之间按 y。
       这里不追求"人眼读的行序"（多栏板书会串行），只要求**确定性**——
       CBoardWatcher 的变更判据按行集合哈希比较，顺序抖动不会误报变更，
       但报表里拼出来的整段文字若每轮换个顺序，人读起来就是"板书在打乱" */
    std::sort(vecBox.begin(), vecBox.end(), [](const COcrBox_S &a, const COcrBox_S &b) {
        const int32_t nCa = a.nY1 + a.nY2;
        const int32_t nCb = b.nY1 + b.nY2;
        if (nCa != nCb)
        {
            return nCa < nCb;
        }
        return a.nX1 < b.nX1;
    });

    const int32_t nCap = (stParam.nBoxMax > 0) ? stParam.nBoxMax : CLASSROOM_OCR_BOX_MAX;
    if (static_cast<int32_t>(vecBox.size()) > nCap)
    {
        if (pstStat)
        {
            pstStat->u64Truncated += static_cast<uint64_t>(vecBox.size() - nCap);
        }
        vecBox.resize(static_cast<size_t>(nCap));
    }
    rvecBox.swap(vecBox);
    return static_cast<int32_t>(rvecBox.size());
}

void scaleBoxToImage(const COcrBox_S &stMapBox, int32_t nMapW, int32_t nMapH,
                     int32_t nImgW, int32_t nImgH, COcrBox_S &rstImgBox)
{
    rstImgBox = stMapBox;
    if (nMapW <= 0 || nMapH <= 0 || nImgW <= 0 || nImgH <= 0)
    {
        return;
    }
    const float fSx = static_cast<float>(nImgW) / static_cast<float>(nMapW);
    const float fSy = static_cast<float>(nImgH) / static_cast<float>(nMapH);

    int32_t nX1 = static_cast<int32_t>(std::floor(static_cast<float>(stMapBox.nX1) * fSx));
    int32_t nX2 = static_cast<int32_t>(std::ceil(static_cast<float>(stMapBox.nX2) * fSx));
    int32_t nY1 = static_cast<int32_t>(std::floor(static_cast<float>(stMapBox.nY1) * fSy));
    int32_t nY2 = static_cast<int32_t>(std::ceil(static_cast<float>(stMapBox.nY2) * fSy));

    rstImgBox.nX1 = ocrMax(0, nX1);
    rstImgBox.nY1 = ocrMax(0, nY1);
    rstImgBox.nX2 = ocrMin(nImgW, nX2);
    rstImgBox.nY2 = ocrMin(nImgH, nY2);
    if (rstImgBox.nX2 <= rstImgBox.nX1 + 4)
    {
        rstImgBox.nX2 = ocrMin(nImgW, rstImgBox.nX1 + 5);
    }
    if (rstImgBox.nY2 <= rstImgBox.nY1 + 4)
    {
        rstImgBox.nY2 = ocrMin(nImgH, rstImgBox.nY1 + 5);
    }
}

int32_t recCanvasWidth(int32_t nBoxW, int32_t nBoxH, int32_t nCanvasW, int32_t nCanvasH)
{
    const int32_t nMaxW = nCanvasW & ~3;   /* RGA 对 RGB888 目标的硬要求 */
    if (nBoxW <= 0 || nBoxH <= 0 || nMaxW < 4 || nCanvasH <= 0)
    {
        return (nMaxW >= 4) ? nMaxW : 0;
    }
    const float fRatio = static_cast<float>(nBoxW) / static_cast<float>(nBoxH);
    int32_t nW = static_cast<int32_t>(std::ceil(static_cast<float>(nCanvasH) * fRatio));
    if (nW > nMaxW)
    {
        nW = nMaxW;
    }
    nW &= ~3;
    if (nW < 4)
    {
        nW = 4;
    }
    return nW;
}

void padRecCanvasRight(uint8_t *pucCanvas, int32_t nCanvasW, int32_t nCanvasH,
                       int32_t nFilledW, uint8_t ucPad)
{
    if (nullptr == pucCanvas || nCanvasW <= 0 || nCanvasH <= 0)
    {
        return;
    }
    if (nFilledW <= 0)
    {
        memset(pucCanvas, ucPad, static_cast<size_t>(nCanvasW) * nCanvasH * 3);
        return;
    }
    if (nFilledW >= nCanvasW)
    {
        return;   /* 满幅，本来就没有右侧空隙 */
    }

    const size_t nSrcRow = static_cast<size_t>(nFilledW) * 3;
    const size_t nDstRow = static_cast<size_t>(nCanvasW) * 3;
    const size_t nPadRow = nDstRow - nSrcRow;
    /* 自下而上：往低行摊时目标区间一定还在已读过的源区间之上，不会踩到还没读的源行 */
    for (int32_t r = nCanvasH - 1; r >= 0; --r)
    {
        uint8_t *pucDst = pucCanvas + static_cast<size_t>(r) * nDstRow;
        const uint8_t *pucSrc = pucCanvas + static_cast<size_t>(r) * nSrcRow;
        if (pucDst != pucSrc)
        {
            memmove(pucDst, pucSrc, nSrcRow);
        }
        memset(pucDst + nSrcRow, ucPad, nPadRow);
    }
}

/* ---------------- rec 输出解码 ---------------- */

bool ctcClassMap(uint32_t u32ClassNum, const COcrTokenTable &stToken, uint32_t &ru32Shift)
{
    const uint32_t u32Num = stToken.numTokens();
    if (0 == u32Num)
    {
        return false;
    }
    if (u32ClassNum == u32Num)
    {
        ru32Shift = 0;      /* 交付件把 blank 当第一条写进字典：类别号即词条号 */
        return true;
    }
    if (u32ClassNum == u32Num + 1)
    {
        ru32Shift = 1;      /* 字典不含 blank，而模型类别 0 恒为 blank */
        return true;
    }
    return false;
}

float ctcGreedyDecode(const float *pfOut, int32_t nTimeStep, uint32_t u32ClassNum,
                      uint32_t u32Shift, const COcrTokenTable &stToken,
                      int32_t nMaxBytes, std::string &rstrText)
{
    rstrText.clear();
    if (nullptr == pfOut || nTimeStep <= 0 || u32ClassNum < 2 || !stToken.ready())
    {
        return 0.0f;
    }

    float fScoreSum = 0.0f;
    int32_t nAccepted = 0;
    int32_t nPrev = -1;
    bool bTruncated = false;

    for (int32_t t = 0; t < nTimeStep; ++t)
    {
        const float *pfRow = pfOut + static_cast<size_t>(t) * u32ClassNum;
        uint32_t nBest = 0;
        float fMax = pfRow[0];
        for (uint32_t c = 1; c < u32ClassNum; ++c)
        {
            const float fV = pfRow[c];
            if (fV > fMax)
            {
                fMax = fV;
                nBest = c;
            }
        }

        const int32_t nBestIdx = static_cast<int32_t>(nBest);
        if (nBestIdx == static_cast<int32_t>(nPrev))
        {
            /* 折叠重复：与上游一致，blank 也会打断重复——「A blank A」是两个字，
               而「A A」是一个字，所以 nPrev 在 blank 分支同样要更新 */
            continue;
        }
        nPrev = nBestIdx;
        if (0 == nBest)
        {
            continue;   /* blank：不产字 */
        }

        /* softmax 分母只在「这一步真要产字」时才算。反例代价：交付字典 18,709 条、
           每行 80 步，blank/折叠步占一半以上，它们的分母算完就被 continue 丢掉——
           每行白跑几十万次 expf。
           循环方向与减最大值的做法一字未动 ⇒ 这是**同一个数**，不是近似 */
        float fExpSum = 0.0f;
        for (uint32_t c = 0; c < u32ClassNum; ++c)
        {
            fExpSum += std::exp(pfRow[c] - fMax);
        }
        const float fProb = (fExpSum > 0.0f) ? (1.0f / fExpSum) : 0.0f;

        const uint32_t nToken = (nBestIdx >= static_cast<int32_t>(u32Shift))
                                ? (nBest - u32Shift) : 0xFFFFFFFFu;
        if (!stToken.appendToken(nToken, rstrText, nMaxBytes))
        {
            /* 越界意味着类别数与字典对不上（配置错位），拼出来的串是半条，整行判废；
               超出字节上限则是行比 512B 还长，同样整行弃——见 board_watcher.h 那条同样的取舍 */
            bTruncated = true;
            break;
        }
        fScoreSum += fProb;
        nAccepted++;
    }

    if (bTruncated)
    {
        rstrText.clear();
        return 0.0f;
    }
    return (nAccepted > 0) ? (fScoreSum / static_cast<float>(nAccepted)) : 0.0f;
}
}
