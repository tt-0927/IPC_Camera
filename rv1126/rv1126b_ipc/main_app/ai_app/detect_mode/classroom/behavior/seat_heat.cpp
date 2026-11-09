/**
 * @FilePath     : seat_heat.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-02 14:04:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 14:04:00
 * @Description  : 座位就坐热力实现
 */

#include "seat_heat.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "dlog.h"

namespace Classroom
{
namespace
{
/* 归一化坐标下的退化判据。取 1e-6 而不是 0：源帧 4096 宽时 1e-6 ≈ 0.004 px，
   任何"真的有一条边"的矩形都远大于它，而 dstToSrc 退化出来的框常常正好是 0 */
const float SEAT_EPS = 1.0e-6f;

float clamp01(float fV)
{
    if (!(fV > 0.0f))
    {
        return 0.0f;    /* 同时挡 NaN：NaN 与任何数的比较都是 false，走这条就是 0 */
    }
    return (fV > 1.0f) ? 1.0f : fV;
}

/* 交叠面积占**格子**的比例（不是占区域的比例）：问的是"这一格有多大一块是座位"，
   与 CRegionTable::coverageOf 的分母口径同构——画一个巨大的席区不该让走道格也算成座位 */
float coverOfCell(const CRectf_S &stRegion, const CRectf_S &stCell)
{
    const float fCellArea = stCell.area();
    if (!(fCellArea > 0.0f))
    {
        return 0.0f;
    }
    const float fW = std::min(stRegion.fX2, stCell.fX2) - std::max(stRegion.fX1, stCell.fX1);
    const float fH = std::min(stRegion.fY2, stCell.fY2) - std::max(stRegion.fY1, stCell.fY1);
    if (!(fW > 0.0f) || !(fH > 0.0f))
    {
        return 0.0f;
    }
    return (fW * fH) / fCellArea;
}
}

/* 与网格同段的比较：格子编号的含义由这几项共同决定，任一项变了旧秒数就挂错了地方。
   放在类内是因为 CGrid_S 是私有类型，让自由函数去认它等于把网格布局暴露成半个对外接口 */
bool CSeatHeat::sameGrid(const CGrid_S &stA, const CGrid_S &stB)
{
    return stA.bReady == stB.bReady && stA.nCols == stB.nCols && stA.nRows == stB.nRows
           && stA.nInPlayNum == stB.nInPlayNum
           && std::abs(stA.fX0 - stB.fX0) < SEAT_EPS && std::abs(stA.fY0 - stB.fY0) < SEAT_EPS
           && std::abs(stA.fX1 - stB.fX1) < SEAT_EPS && std::abs(stA.fY1 - stB.fY1) < SEAT_EPS
           && 0 == std::memcmp(stA.patInPlay, stB.patInPlay, sizeof(stA.patInPlay));
}


/**
 * @brief   : "这一轮画不出图"的日志闸门
 * @note    : 引擎在网格未就绪时按拍重试 rebuildGrid，裸 dlog_warn 会变成每采样周期一条，
 *            把心跳与事件日志冲干净——而那正是出问题时最需要看日志的时候。
 *            闸门在成功建格后由 rebuildGrid 复位，所以"改好了又改坏"仍能各喊一声
 */
void CSeatHeat::warnBuild(const char *pszFmt, ...)
{
    if (m_bBuildWarned)
    {
        return;
    }
    m_bBuildWarned = true;

    char szBuf[192] = {0};
    va_list ap;
    va_start(ap, pszFmt);
    const int nLen = vsnprintf(szBuf, sizeof(szBuf), pszFmt, ap);
    va_end(ap);
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        dlog_warn("classroom seat_heat: 无法构建热力图（原因文本过长）");
        return;
    }
    dlog_warn("classroom seat_heat: %s", szBuf);
}

bool seatHeatParamOk(const CSeatHeatConf_S &rstConf, std::string &rstrErr)
{
    /* 上下界都是**报文体积**的界，不是功能性的界：16×8 是"一格约一到两个座位"的分辨率上限，
       再细就只是在给跟踪抖动让路（约束 1 的反面）。这里不钳制而是硬拒 */
    if (rstConf.nCols < 1 || rstConf.nCols > CLASSROOM_SEAT_MAX_COLS)
    {
        rstrErr = "列数超出 [1," + std::to_string(CLASSROOM_SEAT_MAX_COLS) + "]";
        return false;
    }
    if (rstConf.nRows < 1 || rstConf.nRows > CLASSROOM_SEAT_MAX_ROWS)
    {
        rstrErr = "行数超出 [1," + std::to_string(CLASSROOM_SEAT_MAX_ROWS) + "]";
        return false;
    }
    /* 1 是合法值，含义是"逐拍直读、放弃防抖"：调试时想知道原始落格长什么样，
       这个用法存在，就不该被拒。0 与负数没有含义（永远改不了判） */
    if (rstConf.nHoldBeats < 1 || rstConf.nHoldBeats > 1000)
    {
        rstrErr = "防抖拍数超出 [1,1000]";
        return false;
    }
    /* 0 = 整块外接框都算座位格，走道那一档就此失效（约束 3 被显式放弃）。
       这是一个能自圆其说的选择（"我就要一张铺满的图"），所以放行。
       上界是 100 而不是"随便多大"：覆盖率这个量到不了 100 以上，配 120 的后果是
       **永久**取不到任何成格的图（rebuildGrid 会撤掉图并报"没有图"），
       而下发的人以为只是"收紧了一点" */
    if (rstConf.nMinCellCoverPercent < 0 || rstConf.nMinCellCoverPercent > 100)
    {
        rstrErr = "最小覆盖率超出 [0,100]";
        return false;
    }
    if (rstConf.nIdleBeats < 1 || rstConf.nIdleBeats > 10000)
    {
        rstrErr = "空闲拍数超出 [1,10000]";
        return false;
    }
    return true;
}

CSeatHeat::CSeatHeat()
{
    /* 构造期定容，之后只有 nPersonId=-1 的空槽状态在流转，容量永不复变 */
    m_vecRow.assign(static_cast<size_t>(CLASSROOM_MAX_PERSONS), CSeatRow_S());
}

void CSeatHeat::setParam(const CSeatHeatConf_S &rstConf)
{
    const bool bGeomChanged = (rstConf.nCols != m_stConf.nCols || rstConf.nRows != m_stConf.nRows
                               || rstConf.nMinCellCoverPercent != m_stConf.nMinCellCoverPercent);
    m_stConf = rstConf;

    if (bGeomChanged || !m_stConf.bEnable)
    {
        /* 图与行一起清，两个理由各自成立：
           格子**编号**变了——旧秒数挂在新编号上就是新格记旧账（报表上看不出来），
           而行里的 nCell 也是旧编号，它不会自己失效，只会让接下来几拍被算到一个陌生的格子上；
           关掉——留着上一轮的图，报表侧只看 bEnable 的话哪次改动漏判一次，
           就会把"三节课前的座位分布"当现在的发出去。清完之后即使某个消费者忘了判 enable，
           它拿到的也是一张空图 + bAreaKnown=false，两处都在说"这里没有数据" */
        clearAccum();
        m_stGrid = CGrid_S();
        wipeRows();
    }
}

bool CSeatHeat::rebuildGrid(const CRegionTable &stRegion, const CFrameGeometry &stGeom)
{
    if (!m_stConf.bEnable)
    {
        return false;
    }
    if (!stGeom.isReady())
    {
        /* 瞬时不可用（还没建槽位）：保留旧网格与旧秒数，等下一拍。这里清空的话，
           一次总几何重建就会把一节课攒的图打回 UNKNOWN —— 而 UNKNOWN 是"没数据"，
           不是"数据还没到齐"，平台侧无法区分 */
        return false;
    }

    const std::vector<int32_t> &vecIds = stRegion.idsOfRole(RegionRole_E::REGION_ROLE_STUDENT);
    if (vecIds.empty())
    {
        if (m_bAreaKnown)
        {
            dlog_warn("classroom seat_heat: 学生区域已被移除，热力图停止上报");
        }
        m_bAreaKnown = false;
        m_stGrid = CGrid_S();
        return false;
    }
    m_bAreaKnown = true;

    const float fSrcW = static_cast<float>(stGeom.srcWidth());
    const float fSrcH = static_cast<float>(stGeom.srcHeight());
    if (!(fSrcW > 0.0f) || !(fSrcH > 0.0f))
    {
        return false;    /* isReady 已保证，这条是给"以后有人放宽 isReady"留的哨 */
    }

    /* 席区可以是好几块（前区 + 后区，中间隔着走道）。全部换算到源帧归一化后取并集当网格
       外接框，逐格再问"落在哪块席区里"——所以并集内部的空洞由 patInPlay 表达，
       不需要网格本身是连通集 */
    CRectf_S astArea[CLASSROOM_MAX_REGIONS];
    /* POLY 席区的逐顶点：外接框口径会把 L 形席区的走道格
       盖成全覆盖——旧实现正是这么漏的，逐格覆盖得问多边形本身。
       anPtNum[k]<3 表示第 k 块按外接框口径参与（退化多边形与"区域不存在"同口径） */
    CPointf_S astPts[CLASSROOM_MAX_REGIONS * CLASSROOM_REGION_MAX_POINTS];
    int32_t anPtNum[CLASSROOM_MAX_REGIONS] = {0};
    int32_t nAreaNum = 0;
    for (size_t i = 0; i < vecIds.size(); ++i)
    {
        if (nAreaNum >= CLASSROOM_MAX_REGIONS)
        {
            warnBuild("学生角色引用了 %zu 个区域，只有前 %d 个生效",
                      vecIds.size(), CLASSROOM_MAX_REGIONS);
            break;
        }
        const CRegionArea_S *pstArea = stRegion.regionOf(vecIds[i]);
        if (nullptr == pstArea)
        {
            /* 角色引用了一个没建成的区域：配置层已按"引用不存在的 id"拒过一道，
               走到这里说明是运行期区域表被换坏了，说清是哪一条比沉默有用 */
            warnBuild("学生区域 id %d 在区域表里找不到", vecIds[i]);
            continue;
        }
        /* 区域表给的是**分析帧**坐标（那是判定用的口径），本类一律换算到源帧归一化再用 */
        const CRectf_S stSrc = stGeom.dstToSrc(pstArea->stBox);
        CRectf_S stNorm;
        stNorm.fX1 = clamp01(stSrc.fX1 / fSrcW);
        stNorm.fY1 = clamp01(stSrc.fY1 / fSrcH);
        stNorm.fX2 = clamp01(stSrc.fX2 / fSrcW);
        stNorm.fY2 = clamp01(stSrc.fY2 / fSrcH);
        if (!(stNorm.area() > SEAT_EPS))
        {
            warnBuild("学生区域 id %d 几何变换后退化为空", vecIds[i]);
            continue;
        }
        if (RegionShape_E::REGION_POLY == pstArea->enShape)
        {
            if (static_cast<int32_t>(pstArea->vecPoints.size()) > CLASSROOM_REGION_MAX_POINTS)
            {
                warnBuild("学生区域 id %d 有 %zu 个顶点，超过上限 %d；改用外接框",
                          vecIds[i], pstArea->vecPoints.size(), CLASSROOM_REGION_MAX_POINTS);
            }
            else
            {
                const size_t nBase = static_cast<size_t>(nAreaNum) * CLASSROOM_REGION_MAX_POINTS;
                int32_t nKept = 0;
                for (size_t v = 0; v < pstArea->vecPoints.size(); ++v)
                {
                    CPointf_S stNormPt;
                    if (!stGeom.dstToNormSrc(pstArea->vecPoints[v], stNormPt))
                    {
                        nKept = 0;
                        break;
                    }
                    astPts[nBase + static_cast<size_t>(nKept++)] = stNormPt;
                }
                anPtNum[nAreaNum] = (nKept >= 3) ? nKept : 0;
            }
        }
        astArea[nAreaNum++] = stNorm;
    }
    if (0 == nAreaNum)
    {
        m_stGrid = CGrid_S();
        return false;
    }

    /* 这里不钳制：nCols×nRows 的上界同时是 patInPlay 与 m_anAccumMs 的数组长度，
       越界的后果不是"图变粗一点"而是写越界。配置层的 seatHeatParamOk 已经拒过一道，
       这一道挡的是绕过配置层直接 setParam 的调用方（单测、以及以后可能的内部通道） */
    if (m_stConf.nCols < 1 || m_stConf.nCols > CLASSROOM_SEAT_MAX_COLS ||
        m_stConf.nRows < 1 || m_stConf.nRows > CLASSROOM_SEAT_MAX_ROWS)
    {
        warnBuild("网格 %dx%d 超出 %dx%d 的格子容量上限", m_stConf.nCols, m_stConf.nRows,
                  CLASSROOM_SEAT_MAX_COLS, CLASSROOM_SEAT_MAX_ROWS);
        return false;
    }

    CGrid_S stNew;
    stNew.nCols = m_stConf.nCols;
    stNew.nRows = m_stConf.nRows;
    stNew.fX0 = astArea[0].fX1;
    stNew.fY0 = astArea[0].fY1;
    stNew.fX1 = astArea[0].fX2;
    stNew.fY1 = astArea[0].fY2;
    for (int32_t i = 1; i < nAreaNum; ++i)
    {
        stNew.fX0 = std::min(stNew.fX0, astArea[i].fX1);
        stNew.fY0 = std::min(stNew.fY0, astArea[i].fY1);
        stNew.fX1 = std::max(stNew.fX1, astArea[i].fX2);
        stNew.fY1 = std::max(stNew.fY1, astArea[i].fY2);
    }
    stNew.fCellW = (stNew.fX1 - stNew.fX0) / static_cast<float>(stNew.nCols);
    stNew.fCellH = (stNew.fY1 - stNew.fY0) / static_cast<float>(stNew.nRows);
    if (!(stNew.fCellW > 0.0f) || !(stNew.fCellH > 0.0f))
    {
        warnBuild("学生区域退化成一条线");
        return false;    /* 席区被画成了一条线：没有可分的格子，也不要有图 */
    }

    /* 覆盖率门限。内层循环的早退条件 `fBest < fNeed` 与门外的判据 `fBest >= fNeed` 同侧，
       所以 percent=0 不需要特判：一次都不进循环、fBest 停在 0、0>=0 成立 → 每格都成格
       （正是"别拿覆盖率的架子筛格子"这个用法的本意）。最后一格用外接框的右/下边收尾，
       不让浮点误差在图边上留一条缝 */
    const float fNeed = static_cast<float>(m_stConf.nMinCellCoverPercent);
    for (int32_t nRow = 0; nRow < stNew.nRows; ++nRow)
    {
        for (int32_t nCol = 0; nCol < stNew.nCols; ++nCol)
        {
            CRectf_S stCell;
            stCell.fX1 = stNew.fX0 + stNew.fCellW * static_cast<float>(nCol);
            stCell.fY1 = stNew.fY0 + stNew.fCellH * static_cast<float>(nRow);
            stCell.fX2 = (nCol + 1 == stNew.nCols) ? stNew.fX1 : (stCell.fX1 + stNew.fCellW);
            stCell.fY2 = (nRow + 1 == stNew.nRows) ? stNew.fY1 : (stCell.fY1 + stNew.fCellH);
            float fBest = 0.0f;
            const float fCellArea = stCell.area();
            for (int32_t k = 0; k < nAreaNum && fBest < fNeed; ++k)
            {
                /* 有顶点问多边形、没顶点问外接框：分母仍是**格子**面积，
                   与 coverOfCell 同一口径——画一个巨大的席区不该让走道格也算成座位 */
                float fCover = 0.0f;
                if (anPtNum[k] >= 3 && fCellArea > 0.0f)
                {
                    fCover = polyRectOverlapArea(
                                 astPts + static_cast<size_t>(k) * CLASSROOM_REGION_MAX_POINTS,
                                 anPtNum[k], stCell) / fCellArea;
                }
                else
                {
                    fCover = coverOfCell(astArea[k], stCell);
                }
                fBest = std::max(fBest, fCover * 100.0f);
            }
            if (fBest >= fNeed)
            {
                stNew.patInPlay[static_cast<size_t>(nRow * stNew.nCols + nCol)] = 1;
                ++stNew.nInPlayNum;
            }
        }
    }

    if (0 == stNew.nInPlayNum)
    {
        /* 覆盖率门限配得比任何一格的真实占比都高（例如席区边界正好切过每一格）。
           留着上一张图继续用是错的：那张图的格子号与现在这块席区已经对不上了。
           这里既不换新图也不保留旧图，而是把图撤掉——读数变成"没有图"，
           比"有一张看着正常但指错了地方的图"诚实 */
        warnBuild("没有任何格子达到覆盖率 %d%%", m_stConf.nMinCellCoverPercent);
        m_stGrid = CGrid_S();
        clearAccum();
        return false;
    }

    stNew.bReady = true;
    const bool bChanged = !sameGrid(m_stGrid, stNew);
    m_stGrid = stNew;
    m_bBuildWarned = false;    /* 这次画出来了，下回真画不出来时还要能喊一声 */
    if (bChanged)
    {
        /* 只在格子真的挪过时清账。rebuildGrid 每换一次配置快照都会被调（区域表也一起重建），
           无条件清的话"改一个无关键"就把这节课的热力图清空了 */
        clearAccum();
        wipeRows();    /* 旧格号在新图上没有含义，改判从头攒 */
        dlog_info("classroom seat_heat: 网格 %dx%d 有效格=%d 范围=[%.3f,%.3f,%.3f,%.3f]",
                  stNew.nCols, stNew.nRows, stNew.nInPlayNum,
                  stNew.fX0, stNew.fY0, stNew.fX1, stNew.fY1);
    }
    return true;
}

int32_t CSeatHeat::cellOf(float fNormX, float fNormY, bool &rbOutOfBox) const
{
    rbOutOfBox = false;
    if (!m_stGrid.bReady)
    {
        return SEAT_CELL_NOGRID;
    }
    /* 与 attendance/align 同一套 fail-safe 写法：NaN 过不了任何区间比较，
       而 -1（没有位置证据）与 NaN 在这里该有同一个下场——都不落格，都不是"坐在了第 0 格" */
    if (!(fNormX >= 0.0f) || !(fNormY >= 0.0f) || !(fNormX <= 1.0f) || !(fNormY <= 1.0f))
    {
        return SEAT_CELL_NOGRID;
    }
    /* 边界**取得到**：dstToNormSrc 会把坐标钳到 [0,1]，恰好坐在最右/最下那条线上的人
       如果在这里被判成"图外"，那张图的最后一列就永远不热——而下标算出来是 nCols，
       由下面的钳位收回最后一格，两边一配合正好把这条线还给人 */
    if (fNormX < m_stGrid.fX0 || fNormX > m_stGrid.fX1
        || fNormY < m_stGrid.fY0 || fNormY > m_stGrid.fY1)
    {
        /* 落在并集外接框之外：与走道是两件事。前者多半是"后排把椅子挪到图外了"
           或者席区画小了，后者是"这人站在过道里"——只有分开计数，
           out 与 corridor 的大小关系才能告诉现场该去重画区域还是该改门限 */
        rbOutOfBox = true;
        return SEAT_CELL_CORRIDOR;
    }
    int32_t nCol = static_cast<int32_t>((fNormX - m_stGrid.fX0) / m_stGrid.fCellW);
    int32_t nRow = static_cast<int32_t>((fNormY - m_stGrid.fY0) / m_stGrid.fCellH);
    /* 钳而不是拒：fX1 那一侧已经用 >= 挡过，但浮点除法在格子边界上可以给到 nCols，
       一个恰好坐在最右边那条线上的人不该因此从图上消失 */
    nCol = std::min(std::max(nCol, 0), m_stGrid.nCols - 1);
    nRow = std::min(std::max(nRow, 0), m_stGrid.nRows - 1);
    const int32_t nIdx = nRow * m_stGrid.nCols + nCol;
    if (0 == m_stGrid.patInPlay[static_cast<size_t>(nIdx)])
    {
        return SEAT_CELL_CORRIDOR;
    }
    return nIdx;
}

int32_t CSeatHeat::indexOf(int32_t nPersonId) const
{
    if (nPersonId < 0)
    {
        return -1;
    }
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].nPersonId == nPersonId)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

int32_t CSeatHeat::takeRow(int32_t nPersonId)
{
    const int32_t nExist = indexOf(nPersonId);
    if (nExist >= 0)
    {
        return nExist;
    }
    int32_t nFree = -1;
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].nPersonId < 0)
        {
            nFree = static_cast<int32_t>(i);
            break;
        }
    }
    if (nFree < 0)
    {
        /* 池满：抢最久没看到的那一行。这里可以抢而考勤账本不能抢，是因为本表的一行
           只是"某个格子的归属"——被抢的人下一拍回来会重新攒众数，最多少记几拍；
           而账本的一行是"到课人数"这个数本身的载体（见 attendance.cpp 的 allocRow） */
        uint64_t u64Oldest = UINT64_MAX;
        for (size_t i = 0; i < m_vecRow.size(); ++i)
        {
            if (m_vecRow[i].u64LastBeat < u64Oldest)
            {
                u64Oldest = m_vecRow[i].u64LastBeat;
                nFree = static_cast<int32_t>(i);
            }
        }
        ++u64RowFull;
    }
    m_vecRow[static_cast<size_t>(nFree)] = CSeatRow_S();
    m_vecRow[static_cast<size_t>(nFree)].nPersonId = nPersonId;
    return nFree;
}

void CSeatHeat::fold(const CSeatObs_S *patObs, int32_t nNum, int32_t nStepMs)
{
    /* nNum=0 是合法调用（空教室）：不折人，但那"多久没看到就算离场"的账必须照走，
       否则一节空课堂里行池永不老化，人回来时新 ID 会继承走掉那个人的格子。
       指针为空或步长非正才是真的没法折 */
    if (!m_stConf.bEnable || nullptr == patObs || nNum < 0 || nStepMs <= 0)
    {
        return;
    }
    if (!m_stGrid.bReady)
    {
        ++u64SkipNoGrid;
        return;
    }
    ++m_u64Beat;

    int32_t nPersonNum = 0;
    int32_t nNoPosNum = 0;
    const int32_t nHold = m_stConf.nHoldBeats;

    for (int32_t i = 0; i < nNum; ++i)
    {
        const CSeatObs_S &stObs = patObs[i];
        if (!stObs.bStudent)
        {
            continue;
        }
        ++nPersonNum;
        if (stObs.nPersonId < 0)
        {
            continue;    /* 没身份的行不配占一格：它的"位置"下一拍可能属于另一个人 */
        }

        bool bOutOfBox = false;
        const int32_t nCell = cellOf(stObs.fNormX, stObs.fNormY, bOutOfBox);
        if (SEAT_CELL_NOGRID == nCell)
        {
            /* 网格在上面已判就绪，所以这里只会是"坐标无效"= 这一拍没有位置证据。
               不累加也不清行：他还在不在，下一拍的拍号说了算，不在这里靠一次换算失败下结论 */
            ++nNoPosNum;
            continue;
        }

        const int32_t nRow = takeRow(stObs.nPersonId);
        if (nRow < 0 || nRow >= static_cast<int32_t>(m_vecRow.size()))
        {
            continue;    /* 数组下标不依赖被调函数的返回值语义 */
        }
        CSeatRow_S &stRow = m_vecRow[static_cast<size_t>(nRow)];
        stRow.u64LastBeat = m_u64Beat;
        stRow.nIdleBeat = 0;

        if (SEAT_CELL_CORRIDOR == nCell)
        {
            if (bOutOfBox)
            {
                ++u64OutOfGrid;
            }
            else
            {
                ++u64Corridor;
            }
            continue;    /* 看见人了，但他不在任何座位格上：不记秒，也不动改判状态 */
        }

        if (stRow.nCell == nCell)
        {
            stRow.nCand = -1;
            stRow.nCandNum = 0;
            m_anAccumMs[static_cast<size_t>(nCell)] += nStepMs;
            continue;
        }
        if (stRow.nCell < 0)
        {
            /* 首次归属不等众数门：约束 1 要挡的是"在两个格之间抖"，而一个还没有格的人
               不存在可抖的对象。等他攒够 3 拍才开始记，等于每个人凭空少记 3 个采样周期，
               那是一份系统性偏小、且随 n_hold_beats 变化的人·秒 */
            stRow.nCell = nCell;
            m_anAccumMs[static_cast<size_t>(nCell)] += nStepMs;
            continue;
        }

        if (stRow.nCand != nCell)
        {
            stRow.nCand = nCell;
            stRow.nCandNum = 1;
        }
        else
        {
            ++stRow.nCandNum;
        }

        if (stRow.nCandNum >= nHold)
        {
            stRow.nCell = nCell;
            stRow.nCand = -1;
            stRow.nCandNum = 0;
            ++u64CellMove;
            m_anAccumMs[static_cast<size_t>(nCell)] += nStepMs;
        }
        else
        {
            /* 争议期内这几拍记在**旧格**：众数的定义就是"这段时间他属于哪一格"，
               记到新格等于让一次没通过的抖动也留下一格的热度，而热区正是交付物 */
            m_anAccumMs[static_cast<size_t>(stRow.nCell)] += nStepMs;
        }
    }

    /* 没被看到的人：攒够 n_idle_beats 就让格。刻意放在逐人循环之外——
       放进循环里就只有"这一拍出现过的行"能被刷新，而要被踢的恰恰是没出现的那些 */
    for (auto &stRow : m_vecRow)
    {
        if (stRow.nPersonId < 0 || stRow.u64LastBeat == m_u64Beat)
        {
            continue;
        }
        if (++stRow.nIdleBeat >= m_stConf.nIdleBeats)
        {
            stRow = CSeatRow_S();
        }
    }

    m_nFoldPersonNum = nPersonNum;
    m_nFoldNoPosNum = nNoPosNum;
    ++u64Fold;
}

int32_t CSeatHeat::seatOf(int32_t nPersonId) const
{
    if (!m_stGrid.bReady)
    {
        /* 格子号只对"当前这张图"有意义：图撤了还报一个编号，考勤那边会把它当成
           一个可比的座位先验（两个不同时期、不同网格下的同一串号会被判成同一座）。
           几何临时不可用不走这条路——那种情况 rebuildGrid 保留旧图，先验继续有效 */
        return -1;
    }
    const int32_t nRow = indexOf(nPersonId);
    if (nRow < 0)
    {
        return -1;
    }
    return m_vecRow[static_cast<size_t>(nRow)].nCell;
}

void CSeatHeat::snapshot(CSeatSnap_S &rsnapOut)
{
    rsnapOut = CSeatSnap_S();
    /* 整表先抹成"这张图无效"再谈别的：数组的成员初始化只能给首项（`= { SEAT_CELL_NOGRID }`
       把 [0] 置成 -1，其余是 0），而 0 在报表里是一句肯定的话——"这个座位这个周期没人坐"。
       下面两处提前返回都把这张表原样发出去，不抹一遍就是凭空造出 127 个空座位 */
    for (int32_t i = 0; i < CLASSROOM_SEAT_MAX_CELLS; ++i)
    {
        rsnapOut.anSec[static_cast<size_t>(i)] = SEAT_CELL_NOGRID;
    }
    rsnapOut.bEnable = m_stConf.bEnable;
    if (!m_stConf.bEnable)
    {
        return;
    }
    rsnapOut.bAreaKnown = m_bAreaKnown && m_stGrid.bReady;
    if (!rsnapOut.bAreaKnown)
    {
        /* 约束 3：席区没画 / 网格没就绪 → 整段不上报（报文里 seat 是 null），
           而不是一张全 0 的图。全 0 会被平台画成"一间空教室"，
           而实情是"这台设备不知道座位在哪"——两件事在需求 10 里是两个处置方式 */
        return;
    }

    rsnapOut.nCols = m_stGrid.nCols;
    rsnapOut.nRows = m_stGrid.nRows;
    rsnapOut.fX0 = m_stGrid.fX0;
    rsnapOut.fY0 = m_stGrid.fY0;
    rsnapOut.fX1 = m_stGrid.fX1;
    rsnapOut.fY1 = m_stGrid.fY1;
    rsnapOut.nPersonNum = m_nFoldPersonNum;
    rsnapOut.nNoPosNum = m_nFoldNoPosNum;

    const int32_t nCellNum = m_stGrid.nCols * m_stGrid.nRows;
    for (int32_t i = 0; i < CLASSROOM_SEAT_MAX_CELLS; ++i)
    {
        if (i >= nCellNum)
        {
            /* 网格比 16×8 小，尾巴上这些不是"0 秒的座位"而是"这张图里没有这一格"。
               留 -1 而不是 0：格式化侧按 nCols×nRows 取数，取不到这些，
               但单测与 statusJson 会整表看，给 0 就等于凭空造出一批空座位 */
            rsnapOut.anSec[static_cast<size_t>(i)] = SEAT_CELL_NOGRID;
            continue;
        }
        if (0 == m_stGrid.patInPlay[static_cast<size_t>(i)])
        {
            rsnapOut.anSec[static_cast<size_t>(i)] = SEAT_CELL_CORRIDOR;
            continue;
        }
        int64_t &llMs = m_anAccumMs[static_cast<size_t>(i)];
        const int64_t llSec = llMs / 1000;
        /* 扣掉报出去的整秒、留下不足一秒的余数。
           每拍直接截断的话，5s 步长在 128 格上每周期最多丢 128 个亚秒，
           一节课累计成一个看得见的小偏差——小，但它只朝一个方向偏 */
        llMs -= llSec * 1000;
        rsnapOut.anSec[static_cast<size_t>(i)] = static_cast<int32_t>(llSec);
    }
}

void CSeatHeat::reset()
{
    clearAccum();
    wipeRows();
    m_u64Beat = 0;
    m_nFoldPersonNum = 0;
    m_nFoldNoPosNum = 0;
    u64Fold = 0;
    u64SkipNoGrid = 0;
    u64OutOfGrid = 0;
    u64Corridor = 0;
    u64CellMove = 0;
    u64RowFull = 0;
    /* 网格与 m_bAreaKnown 刻意留着：本节结束没有改变"学生席画在哪"这件事 */
}

std::string CSeatHeat::statText() const
{
    char szBuf[256] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "seat en=%d area=%d grid=%d %dx%d play=%d "
                              "fold=%llu nogrid=%llu out=%llu corr=%llu move=%llu full=%llu "
                              "per=%d nopos=%d",
                              m_stConf.bEnable ? 1 : 0, m_bAreaKnown ? 1 : 0,
                              m_stGrid.bReady ? 1 : 0, m_stGrid.nCols, m_stGrid.nRows,
                              m_stGrid.nInPlayNum,
                              static_cast<unsigned long long>(u64Fold),
                              static_cast<unsigned long long>(u64SkipNoGrid),
                              static_cast<unsigned long long>(u64OutOfGrid),
                              static_cast<unsigned long long>(u64Corridor),
                              static_cast<unsigned long long>(u64CellMove),
                              static_cast<unsigned long long>(u64RowFull),
                              m_nFoldPersonNum, m_nFoldNoPosNum);
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        return "seat stat=truncated";
    }
    return std::string(szBuf);
}

void CSeatHeat::clearAccum()
{
    for (int32_t i = 0; i < CLASSROOM_SEAT_MAX_CELLS; ++i)
    {
        m_anAccumMs[static_cast<size_t>(i)] = 0;
    }
}

void CSeatHeat::wipeRows()
{
    /* 只把槽位抹回空槽，容量不动：m_vecRow 的定容发生在构造期，此后不复变 */
    for (auto &stRow : m_vecRow)
    {
        stRow = CSeatRow_S();
    }
    /* 代次跟着涨：本函数是"格子编号失去含义"这件事唯一的落点（图重画 / 边数改 / 关掉 / 收尾
       四个入口都从这里过），所以代次记在这里而不是记在 rebuildGrid 里——记在那儿会漏掉
       setParam 与 reset 那两条，而它们同样让旧编号变成另一块画面。
       递增放在抹表之后：读到的新代次一定对应一张已经抹干净的行池，
       不会出现"代次变了而行池还是旧的"那半拍 */
    ++m_u64GridEpoch;
}
}
