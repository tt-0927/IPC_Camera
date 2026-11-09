/**
 * @FilePath     : region.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-26 10:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-26 10:10:00
 * @Description  : 分析区域换算与覆盖判定
 */

#include "region.h"

#include <cmath>

#include "dlog.h"

namespace Classroom
{
namespace
{
/**
 * @brief   : Web 口径单点 → 源帧坐标
 * @note    : 故意借 rect 版的 webToSrc 而不是在这里重写归一化公式：§4.8 要求坐标换算只有一个
 *            出口，多写一份实现就多一处在镜像/旋转机型上分叉的地方。传一个退化成点的矩形，
 *            顺带把「点落在画面外」钳制到画面内（webToSrc 内部钳制），不必自己判边界
 */
CPointf_S webPointToSrc(const CFrameGeometry &stGeom, const CPointf_S &stPoint,
                        CoordSpace_E enSpace, int nRefW, int nRefH)
{
    const CRectf_S stDot = { stPoint.fX, stPoint.fY, stPoint.fX, stPoint.fY };
    const CRectf_S stSrc = stGeom.webToSrc(stDot, enSpace, nRefW, nRefH);

    return CPointf_S{ stSrc.fX1, stSrc.fY1 };
}

/* 分析帧坐标下的外接框（多边形用），点数已保证 >0 */
CRectf_S boundingBox(const std::vector<CPointf_S> &vecPoints)
{
    if (vecPoints.empty())
    {
        CRectf_S stZero = { 0.0f, 0.0f, 0.0f, 0.0f };
        return stZero;
    }
    /* NaN 不参与 min/max：任何一次与 NaN 的比较都是 false，而这里的比较链是
       「小于才更新」，于是 NaN 点被静默跳过——那正是想要的（只丢一个坏点，
       不让它把整块区域的外接框变成 NaN 后连面积都算不出来）。
       但首点若本身就是 NaN，下面那轮初值就带着 NaN：此时「小于」全 false、
       「大于」也全 false，最终外接框 = 一个 NaN 点，面积算不出来、区域整块失效。
       所以先找到第一个可用点做初值；一个都没有就退回原点（调用方随后会因
       点数/面积不足丢掉这块区域） */
    size_t nBase = vecPoints.size();
    for (size_t i = 0; i < vecPoints.size(); ++i)
    {
        if (std::isfinite(vecPoints[i].fX) && std::isfinite(vecPoints[i].fY))
        {
            nBase = i;
            break;
        }
    }
    if (nBase >= vecPoints.size())
    {
        CRectf_S stZero = { 0.0f, 0.0f, 0.0f, 0.0f };
        return stZero;
    }
    CRectf_S stBox = { vecPoints[nBase].fX, vecPoints[nBase].fY,
                       vecPoints[nBase].fX, vecPoints[nBase].fY };

    for (size_t i = 0; i < vecPoints.size(); ++i)
    {
        /* 非有限点整点跳过：不参与外接框，也不让它污染半径/面积（调用方拿面积做分母） */
        if (!std::isfinite(vecPoints[i].fX) || !std::isfinite(vecPoints[i].fY))
        {
            continue;
        }
        stBox.fX1 = (vecPoints[i].fX < stBox.fX1) ? vecPoints[i].fX : stBox.fX1;
        stBox.fY1 = (vecPoints[i].fY < stBox.fY1) ? vecPoints[i].fY : stBox.fY1;
        stBox.fX2 = (vecPoints[i].fX > stBox.fX2) ? vecPoints[i].fX : stBox.fX2;
        stBox.fY2 = (vecPoints[i].fY > stBox.fY2) ? vecPoints[i].fY : stBox.fY2;
    }
    return stBox;
}

/* 网格采样求「待判定矩形有多少比例落在多边形内」，见 region.h 头部对采样取舍的说明 */
float polygonCoverage(const CRectf_S &stRect, const std::vector<CPointf_S> &vecPoints)
{
    /* 退化矩形（宽或高 ≤0）与点数不足的「多边形」都不做采样：前者除出来是 0 宽格子、
       后者 pointInPolygon 自己会拒，但这里先挡掉可以让返回值稳定为 0 而不是 0.x */
    if (stRect.width() <= 0.0f || stRect.height() <= 0.0f || vecPoints.size() < 3)
    {
        return 0.0f;
    }
    constexpr int nGrid = CLASSROOM_REGION_SAMPLE_GRID;
    const float fCellW = stRect.width() / static_cast<float>(nGrid);
    const float fCellH = stRect.height() / static_cast<float>(nGrid);
    int32_t nInside = 0;

    for (int32_t r = 0; r < nGrid; ++r)
    {
        for (int32_t c = 0; c < nGrid; ++c)
        {
            /* 取每个子格的中心而不是格点：取格点会让边界那一列/那一行被重复计入，
               且当待判定框小于一个子格时永远只采到一列，覆盖率退化成 0/1 */
            const CPointf_S stSample = { stRect.fX1 + (static_cast<float>(c) + 0.5f) * fCellW,
                                         stRect.fY1 + (static_cast<float>(r) + 0.5f) * fCellH };
            if (pointInPolygon(stSample, vecPoints))
            {
                ++nInside;
            }
        }
    }
    return static_cast<float>(nInside) / static_cast<float>(nGrid * nGrid);
}

/* 裁剪边端点跨线时求与边界线的交点：t = sp/(sp−sc)，两端点线性插值 */
CPointf_S clipIntersect(const CPointf_S &stP, float fSp, const CPointf_S &stC, float fSc)
{
    CPointf_S stOut;
    /* 除零防护：两端点的有向距离相等（fSp-fSc==0）时本函数被调用，
       意味着「上一个端点在区域内、这一个端点也在区域内」——边没有跨线，不该走到插值。
       但线段与裁剪边**重合**（一条边正好压在裁剪边上）时两端点的有向距离都≈0，
       相减恰为 0，除法给出 inf/NaN，而 NaN 一旦进 vecTmp 就再也出不去：
       后续每一条边与它比大小都得 false，该行点被静默丢掉，最终 vecPoints.size()<3，
       区域整块失效却看不出是哪一块。端点重合时线段的「方向」仍由本端点决定，
       取本端点自身是最小惊讶的选择（与重合边上的常规 Sutherland–Hodgman 实现一致）。 */
    const float fDen = fSp - fSc;
    if (!(std::fabs(fDen) > 1e-6f) || !std::isfinite(fDen))
    {
        stOut = stC;
        return stOut;
    }
    const float fT = fSp / fDen;
    /* t 出界说明两条边的距离符号在数值上不自洽（Web 坐标裁剪后的退化边），
       直接取近端端点而不是任其跑出裁剪区域 */
    const float fTC = (fT < 0.0f) ? 0.0f : ((fT > 1.0f) ? 1.0f : fT);
    stOut.fX = stP.fX + fTC * (stC.fX - stP.fX);
    stOut.fY = stP.fY + fTC * (stC.fY - stP.fY);
    return stOut;
}
}

bool pointInPolygon(const CPointf_S &stPoint, const std::vector<CPointf_S> &vecPoints)
{
    const size_t nNum = vecPoints.size();
    if (nNum < 3)
    {
        return false;
    }

    /* 射线法（水平向右射线与边的交点数奇偶）：凹多边形与任意绕序都成立，
       而「点在顶点上」这条退化情形按奇偶规则本来就归到某一边，判定门限给的是
       覆盖比例而非精确布尔，1/25 的采样抖动不影响结论 */
    bool bInside = false;
    for (size_t i = 0, j = nNum - 1; i < nNum; j = i++)
    {
        const CPointf_S &stA = vecPoints[i];
        const CPointf_S &stB = vecPoints[j];

        const bool bStraddles = ((stA.fY > stPoint.fY) != (stB.fY > stPoint.fY));
        if (!bStraddles)
        {
            continue;
        }

        const float fDenom = stB.fY - stA.fY;
        if (std::fabs(fDenom) < 1e-6f)
        {
            continue;   /* 水平边：与射线平行，交给上一条边判奇偶 */
        }

        const float fCrossX = stA.fX + (stPoint.fY - stA.fY) * (stB.fX - stA.fX) / fDenom;
        if (stPoint.fX < fCrossX)
        {
            bInside = !bInside;
        }
    }
    return bInside;
}

float polyRectOverlapArea(const CPointf_S *pstPoints, int32_t nPtNum, const CRectf_S &stRect)
{
    if (nullptr == pstPoints || nPtNum < 3 || !(stRect.area() > 0.0f))
    {
        return 0.0f;
    }

    /* Sutherland–Hodgman：多边形依次被矩形的 4 条半平面裁剪。凹多边形会被裁成多块，
       输出用退化桥边连成一张图——鞋带面积里桥边一来一回正好抵消，总面积仍然对，
       这是不逐块求交、直接对裁剪结果取带符号面积的依据。
       界内用 std::vector 而不是定长缓冲：对 m 边凸域裁剪 n 边形的输出上界是 n+m，
       四趟叠加是 n+16——这个界依赖"调用方只喂矩形域"，通用函数不押这个注 */
    std::vector<CPointf_S> vecOut(pstPoints, pstPoints + nPtNum);
    std::vector<CPointf_S> vecTmp;

    for (int32_t nEdge = 0; nEdge < 4; ++nEdge)
    {
        if (vecOut.empty())
        {
            return 0.0f;
        }

        /* 前两条切 x、后两条切 y；锚点取矩形对应边，上界取反号，
           统一成「侧值 >= 0 即界内」 一种比较 */
        const bool bXAxis  = (nEdge < 2);
        const float fAnchor = (0 == nEdge) ? stRect.fX1 :
                              (1 == nEdge) ? stRect.fX2 :
                              (2 == nEdge) ? stRect.fY1 : stRect.fY2;
        const float fSign  = (1 == nEdge || 3 == nEdge) ? -1.0f : 1.0f;

        vecTmp.clear();
        for (size_t i = 0, j = vecOut.size() - 1; i < vecOut.size(); j = i++)
        {
            const CPointf_S &stPrev = vecOut[j];
            const CPointf_S &stCur  = vecOut[i];
            const float fSp = fSign * (bXAxis ? (stPrev.fX - fAnchor) : (stPrev.fY - fAnchor));
            const float fSc = fSign * (bXAxis ? (stCur.fX - fAnchor) : (stCur.fY - fAnchor));

            if (fSc >= 0.0f)
            {
                if (fSp < 0.0f)
                {
                    vecTmp.push_back(clipIntersect(stPrev, fSp, stCur, fSc));
                }
                vecTmp.push_back(stCur);
            }
            else if (fSp >= 0.0f)
            {
                vecTmp.push_back(clipIntersect(stPrev, fSp, stCur, fSc));
            }
        }
        vecOut.swap(vecTmp);
    }

    if (vecOut.size() < 3)
    {
        return 0.0f;
    }

    /* 鞋带公式：取绝对值以兼容任意绕序 */
    float fSigned = 0.0f;
    for (size_t i = 0, j = vecOut.size() - 1; i < vecOut.size(); j = i++)
    {
        fSigned += vecOut[j].fX * vecOut[i].fY - vecOut[i].fX * vecOut[j].fY;
    }
    return std::fabs(fSigned) * 0.5f;
}

int32_t CRegionTable::build(const std::vector<CRegionDesc_S> &vecDesc,
                           const std::vector<CRegionRoleRef_S> &vecRoles,
                           CoordSpace_E enSpace, int nRefW, int nRefH,
                           const CFrameGeometry &stGeom)
{
    clear();

    if (vecDesc.empty())
    {
        return 0;
    }

    if (!stGeom.isReady())
    {
        /* 不降级成「直接用 Web 坐标」：那是把两套坐标系混在一张表里，
           镜像机型上的表现是所有区域判定整体反向且不报错 */
        dlog_warn("classroom region: 帧几何未就绪，%d 条区域被丢弃，待重建",
                  static_cast<int32_t>(vecDesc.size()));
        return 0;
    }

    int32_t nSkipped = 0;
    for (size_t i = 0; i < vecDesc.size(); ++i)
    {
        const CRegionDesc_S &stDesc = vecDesc[i];

        if (static_cast<int32_t>(m_vecRegions.size()) >= CLASSROOM_MAX_REGIONS)
        {
            dlog_warn("classroom region: 已达区域容量上限 %d，余下 %d 条描述被忽略",
                      CLASSROOM_MAX_REGIONS,
                      static_cast<int32_t>(vecDesc.size()) - static_cast<int32_t>(i));
            break;
        }
        if (nullptr != regionOf(stDesc.nId))
        {
            /* 重复 id 必须拒掉而不是覆盖：留哪一条取决于数组顺序，而现场看 Web 界面时
               两条都在，「到底哪条生效」就成了不可答题 */
            dlog_warn("classroom region: 重复 id %d，整条拒绝", stDesc.nId);
            ++nSkipped;
            continue;
        }

        CRegionArea_S stArea;
        stArea.nId = stDesc.nId;
        stArea.strName = stDesc.strName;
        stArea.enShape = stDesc.enShape;

        if (RegionShape_E::REGION_RECT == stDesc.enShape)
        {
            const CRectf_S stSrc = stGeom.webToSrc(stDesc.stRectWeb, enSpace, nRefW, nRefH);
            stArea.stBox = stGeom.srcToDst(stSrc);

            if (stArea.stBox.area() <= 0.0f)
            {
                dlog_warn("classroom region: id %d 矩形经坐标变换后退化为空，整条拒绝", stDesc.nId);
                ++nSkipped;
                continue;
            }
        }
        else
        {
            const size_t nPts = stDesc.vecPointsWeb.size();
            if (nPts < 3 || nPts > static_cast<size_t>(CLASSROOM_REGION_MAX_POINTS))
            {
                /* 顶点数超上限同样是拒绝而非截断：截断会把一个凹区域变成另一个区域 */
                dlog_warn("classroom region: id %d 多边形顶点数 %u 不在 [3,%d] 内，整条拒绝",
                          stDesc.nId, static_cast<unsigned int>(nPts), CLASSROOM_REGION_MAX_POINTS);
                ++nSkipped;
                continue;
            }

            stArea.vecPoints.reserve(nPts);
            for (size_t p = 0; p < nPts; ++p)
            {
                const CPointf_S stSrc = webPointToSrc(stGeom, stDesc.vecPointsWeb[p], enSpace, nRefW, nRefH);
                stArea.vecPoints.push_back(stGeom.srcToDst(stSrc));
            }
            stArea.stBox = boundingBox(stArea.vecPoints);

            if (stArea.stBox.area() <= 0.0f)
            {
                dlog_warn("classroom region: id %d 多边形经坐标变换后退化为空，整条拒绝", stDesc.nId);
                ++nSkipped;
                continue;
            }
        }

        m_vecRegions.push_back(stArea);
    }

    for (size_t r = 0; r < vecRoles.size(); ++r)
    {
        const RegionRole_E enRole = vecRoles[r].enRole;
        const size_t nRole = static_cast<size_t>(enRole);

        if (nRole >= static_cast<size_t>(RegionRole_E::REGION_ROLE_NUM))
        {
            dlog_warn("classroom region: 未知角色 %d，忽略", static_cast<int>(enRole));
            continue;
        }

        const std::vector<int32_t> &vecIds = vecRoles[r].vecRegionIds;
        for (size_t k = 0; k < vecIds.size(); ++k)
        {
            /* 角色引用一个不存在的 id：只丢这一条引用，不连带整个角色。
               现场常见形态是「先画讲台、再勾黑板」，勾了却没画完时讲台那条仍要能用 */
            if (nullptr == regionOf(vecIds[k]))
            {
                dlog_warn("classroom region: 角色 %d 引用的区域 id %d 不存在，忽略",
                          static_cast<int>(enRole), vecIds[k]);
                continue;
            }
            m_avecRoleIds[nRole].push_back(vecIds[k]);
        }
    }

    dlog_info("classroom region: %d 个区域建于画布 %dx%d (space=%d ref=%dx%d, %d 条跳过)",
              static_cast<int32_t>(m_vecRegions.size()), stGeom.dstWidth(), stGeom.dstHeight(),
              static_cast<int>(enSpace), nRefW, nRefH, nSkipped);
    return static_cast<int32_t>(m_vecRegions.size());
}

void CRegionTable::clear()
{
    m_vecRegions.clear();
    for (size_t r = 0; r < static_cast<size_t>(RegionRole_E::REGION_ROLE_NUM); ++r)
    {
        m_avecRoleIds[r].clear();
    }
}

const CRegionArea_S *CRegionTable::regionOf(int32_t nId) const
{
    for (size_t i = 0; i < m_vecRegions.size(); ++i)
    {
        if (m_vecRegions[i].nId == nId)
        {
            return &m_vecRegions[i];
        }
    }
    return nullptr;
}

float CRegionTable::coverageOf(int32_t nId, const CRectf_S &stRect, bool &rbFound) const
{
    rbFound = false;

    const CRegionArea_S *pstRegion = regionOf(nId);
    if (nullptr == pstRegion)
    {
        return 0.0f;
    }
    rbFound = true;

    const float fRectArea = stRect.area();
    if (fRectArea <= 0.0f)
    {
        return 0.0f;
    }
    if (0.0f == rectIntersectionArea(stRect, pstRegion->stBox))
    {
        return 0.0f;
    }

    if (RegionShape_E::REGION_RECT == pstRegion->enShape)
    {
        return rectIntersectionArea(stRect, pstRegion->stBox) / fRectArea;
    }
    return polygonCoverage(stRect, pstRegion->vecPoints);
}

bool CRegionTable::inAnyRegion(const std::vector<int32_t> &vecIds, float fMinCoverage,
                              const CRectf_S &stRect, bool &rbConfigError) const
{
    rbConfigError = false;

    if (vecIds.empty())
    {
        return true;
    }

    /* 不提前返回：旧实现「命中即 return true」会把排在后头的坏 id 一起吃下去，
       于是 rbConfigError 是否置位**取决于 id 的排列顺序**——而调用方（student_behavior.cpp:384
       讲台判据、subject_arbiter.cpp:266 角色判据）的语义是「有配置错误就报 UNKNOWN/blocked，
       不拿一个可能不准的 bHit 去判」。同一条配置写成 {7,99} 与 {99,7} 会得到
       「TRUE」与「blocked」两个不同结论，现场按 bHit 排查永远查不出为什么。
       代价是命中后仍要把剩下的 id 走完：区域引用表按行为配置，长度典型 1~3，
       摊到每拍每个人上是几次浮点比较，远小于规则层其它判据一格。 */
    bool bIn = false;
    for (size_t i = 0; i < vecIds.size(); ++i)
    {
        bool bFound = false;
        const float fCoverage = coverageOf(vecIds[i], stRect, bFound);

        if (!bFound)
        {
            rbConfigError = true;
            continue;
        }
        if (!bIn && fCoverage >= fMinCoverage)
        {
            bIn = true;
        }
    }
    return bIn;
}

const std::vector<int32_t> &CRegionTable::idsOfRole(RegionRole_E enRole) const
{
    static const std::vector<int32_t> s_vecEmpty;

    const size_t nRole = static_cast<size_t>(enRole);
    if (nRole >= static_cast<size_t>(RegionRole_E::REGION_ROLE_NUM))
    {
        return s_vecEmpty;
    }
    return m_avecRoleIds[nRole];
}
}
