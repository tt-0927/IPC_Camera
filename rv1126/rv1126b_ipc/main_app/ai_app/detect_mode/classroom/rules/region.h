/**
 * @FilePath     : region.h
 * @Author       : leiyy
 * @Date         : 2026-10-26 10:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-26 10:10:00
 * @Description  : 分析区域
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "frame_geometry.h"

namespace Classroom
{
/* 区域个数上限：表容量必须编译期可算（需求 10 的无界容器红线）。
   网页是"每个功能页各画一块"（需求 1 的第 3 步），可画路径共
   23 条（19 个行为页 + 考勤 + 人脸 + 讲台角色 + 黑板角色），另加非网页页的学生席角色
   仍是 24 个引用位；旧值 16 按"讲台+黑板+若干学生席分区"估算，逐页配置到教师行为页时
   必然越界（现场 2935/2937 报 `too many regions: 17`）。32 = 24 个引用位 + 8 格余量，
   仍是固定容量；超上限的下发条目被校验拒绝并说明原因 */
#define CLASSROOM_MAX_REGIONS (32)

/* 单个多边形的顶点上限，同上：它决定 coverageOf 的最坏代价，不能由下发端随意给 */
#define CLASSROOM_REGION_MAX_POINTS (16)

/* 多边形覆盖率采样网格边长：5×5=25 点，粒度 4%（区域判定的门限通常配 0.5，精度过剩） */
#define CLASSROOM_REGION_SAMPLE_GRID (5)

/* 0~8191 归一坐标的取值上限（与 CoordSpace_E::COORD_SPACE_NORM_8192 同口径）。
   校验器要在**换算之前**判下发值本身有没有越界，那时还没有 CFrameGeometry 可用，
   故这个数必须在公共处单点定义，不能靠 frame_geometry.cpp 里那个私有常量 */
#define CLASSROOM_REGION_NORM_MAX (8191.0f)

/**
 * @brief   : 区域形状
 * @note    : 只有这两种是现网划线器真能画出来的（矩形 + 多边形），不做圆/扇形：
 *            多边形已经能逼近任意形状，加一类就多一套覆盖算法与一处会写错的分支
 */
typedef enum class RegionShape : int
{
    REGION_RECT = 0, /* 矩形 */
    REGION_POLY = 1, /* 多边形（可凹） */
} RegionShape_E;

/**
 * @brief   : 区域用途角色
 * @note    : 存在的理由是「行为要问的是用途，不是几何」：巡视的判据是「讲台区域没人」，
 *            如果代码里写的是「id=3 的区域」，现场重画一次讲台就把逻辑改死了。
 *            只收 CSV 里被点名的三种用途（讲台、黑板、学生席），其余区域一律由行为按 id 引用。
 *            STUDENT 追加在末尾且不动前两个的整数值：Web 侧与日志里已有的 0/1 口径不受影响
 */
typedef enum class RegionRole : int
{
    REGION_ROLE_PODIUM = 0,  /* 讲台 */
    REGION_ROLE_BOARD = 1,   /* 黑板 */
    REGION_ROLE_STUDENT = 2, /* 学生席（P4 巡视/指导判据用；没画则「讲台之外」即学生席） */
    REGION_ROLE_NUM = 3,
} RegionRole_E;

/**
 * @brief 下发侧的区域描述（坐标保持 Web 口径：像素或 0~8191 归一，未换算）
 * @note  刻意与换算后的 CRegionArea_S 分成两个类型：合成一个的话，「这一份坐标到底在哪个
 *        空间」就只剩靠注释约束，而配置层填的是 Web 值、判定层要的是分析帧值——
 *        填错一侧不报错，只是所有区域判定静默反向
 */
typedef struct _RegionDesc_
{
    int32_t      nId = 0;                          /* 区域内唯一编号，行为按它引用 */
    std::string  strName;                          /* 展示名，仅用于日志与 Web 回显 */
    RegionShape_E enShape = RegionShape_E::REGION_RECT;
    CRectf_S     stRectWeb;                        /* RECT：Web 口径矩形 */
    std::vector<CPointf_S> vecPointsWeb;           /* POLY：Web 口径顶点，不要求闭合 */
} CRegionDesc_S;

/**
 * @brief 角色 → 区域 id 的下发条目
 */
typedef struct _RegionRoleRef_
{
    RegionRole_E enRole = RegionRole_E::REGION_ROLE_PODIUM;
    std::vector<int32_t> vecRegionIds;
} CRegionRoleRef_S;

/* 区域覆盖率的默认门限：0.5 = 框有一半以上在区域内才算「在这个区域里」。
   取一半而不是取中心点：人头框在区域边界上时中心点判定会每帧翻一次，
   而按比例判定还有一小段缓冲带给时长门的中断容忍去消化 */
#define CLASSROOM_REGION_MIN_COVERAGE (0.5f)

/**
 * @brief 行为侧对区域表的下发引用（一条行为可引用多个区域，任一命中即通过）
 * @note  「任一」而非「全部」：现场真实用法是把教室分成几块学生席区域，
 *        一条行为关心的是「在任意一块学生席里」，要求全部命中等于要求一个人同时在两块区域
 */
typedef struct _RegionRef_
{
    std::vector<int32_t> vecRegionIds;   /* 空 = 不限区域（画区域前的默认形态） */
    float fMinCoverage = CLASSROOM_REGION_MIN_COVERAGE;
} CRegionRef_S;

/**
 * @brief 换算到分析帧坐标后的区域（判定用的最终形态）
 */
typedef struct _RegionArea_
{
    int32_t      nId = 0;
    std::string  strName;
    RegionShape_E enShape = RegionShape_E::REGION_RECT;
    CRectf_S     stBox;                        /* 分析帧外接框（RECT 即区域本身） */
    std::vector<CPointf_S> vecPoints;          /* POLY 顶点（分析帧），RECT 时为空 */
} CRegionArea_S;

/**
 * @brief   : 射线法判断点是否落在多边形内（奇偶规则，允许凹多边形与顺时针/逆时针任意绕序）
 * @return   {bool} 顶点不足 3 个返回 false（退化多边形按「区域不存在」处理）
 */
bool pointInPolygon(const CPointf_S &stPoint, const std::vector<CPointf_S> &vecPoints);

/**
 * @brief   : 多边形与矩形的精确相交面积（Sutherland–Hodgman 裁剪 + 鞋带面积）
 * @param    {const CPointf_S*} pstPoints / {int32_t} nPtNum：多边形顶点（不足 3 个按面积 0 处理）
 * @return   {float} 相交面积；矩形退化时返回 0
 * @note    : 凹多边形成立——polygonCoverage 的格心采样对凹形会整格误判（L 形席区的走道格
 *            被外接框口径盖成全覆盖），座位逐格覆盖用的是本函数而非采样口径。
 *            收指针而非 vector：座位侧顶点存定长数组，逐格调用不该每次拼一个临时 vector
 */
float polyRectOverlapArea(const CPointf_S *pstPoints, int32_t nPtNum, const CRectf_S &stRect);

/**
 * @brief   : 区域表（只读快照，随配置换入重建）
 * @note    : 线程安全：build() 之后全表只读，可被任意线程并发查询（与 CFrameGeometry 同一套路）。
 *            重建由 CRuleEngine 在节拍线程触发，读者也在节拍线程，因此本类内部不加锁
 */
class CRegionTable
{
public:
    CRegionTable() = default;
    ~CRegionTable() = default;

    CRegionTable(const CRegionTable &) = delete;
    CRegionTable &operator=(const CRegionTable &) = delete;

    /**
     * @brief   : 把 Web 口径的下发区域换算到分析帧坐标并整表重建
     * @param    {const std::vector<CRegionDesc_S>&} vecDesc：下发区域（Web 坐标）
     * @param    {const std::vector<CRegionRoleRef_S>&} vecRoles：角色 → 区域 id 引用
     * @param    {CoordSpace_E} enSpace / {int} nRefW / {int} nRefH：Web 坐标口径与参考分辨率
     * @param    {const CFrameGeometry&} stGeom：源帧→分析帧变换（未就绪时整表清空，等就绪后重建）
     * @return   {int32_t} 被采纳的区域数（<= vecDesc.size()），退化条目被跳过的数量由日志说明
     * @note    : 换算失败（几何未就绪）时清空而不是「按 Web 坐标凑合用」——凑合的结果是
     *            区域判定在镜像机型上整体反向，且不崩不报错，属于最难查的一类失效
     */
    int32_t build(const std::vector<CRegionDesc_S> &vecDesc,
                  const std::vector<CRegionRoleRef_S> &vecRoles,
                  CoordSpace_E enSpace, int nRefW, int nRefH,
                  const CFrameGeometry &stGeom);
    void clear();

    bool isEmpty() const { return m_vecRegions.empty(); }
    int32_t regionNum() const { return static_cast<int32_t>(m_vecRegions.size()); }

    /* 按 id 取区域；没有返回 nullptr */
    const CRegionArea_S *regionOf(int32_t nId) const;

    /**
     * @brief   : 给定框（分析帧坐标）落在区域内的比例
     * @param    {int32_t} nId：区域编号
     * @param    {const CRectf_S&} stRect：要判定的框（人头框或人体框）
     * @param    {bool&} rbFound：OUT=false 表示该 id 不在表里 —— 这是**配置错误**，
     *            调用方必须输出 TS_UNKNOWN 而不是 TS_FALSE（区域没建起来不等于人不在区域里）
     * @return   {float} [0,1]，落在区域内的面积 / 框自身面积；框面积为 0 时返回 0
     * @note    : 分母是「框自身面积」而不是「区域面积」——问的是「这个人有多大部分在区域里」，
     *            画一个巨大的区域不应该让人更容易命中（那样区域就失去了筛选意义）
     */
    float coverageOf(int32_t nId, const CRectf_S &stRect, bool &rbFound) const;

    /**
     * @brief   : 框是否至少落入给定 id 列表中的一个区域
     * @param    {const std::vector<int32_t>&} vecIds：为空表示「不限区域」，直接返回 true
     * @param    {float} fMinCoverage：单个区域的最低覆盖比例
     * @param    {bool&} rbConfigError：OUT=true 表示列表里有 id 查不到（被引用的区域没建成）。
     *            **整张表都走完才置位**：命中不提前返回，否则这个标志的值会随 id 排列顺序变化，
     *            而调用方按它决定「报 UNKNOWN 还是信 bHit」——顺序不同结论不同就没法排查
     * @return   {bool} true 表示通过区域这道闸
     */
    bool inAnyRegion(const std::vector<int32_t> &vecIds, float fMinCoverage,
                     const CRectf_S &stRect, bool &rbConfigError) const;

    /* 取某角色引用的区域 id 列表；未配置返回空表（长度 0，引用方按「该用途没画区域」处理） */
    const std::vector<int32_t> &idsOfRole(RegionRole_E enRole) const;

private:
    std::vector<CRegionArea_S> m_vecRegions;
    std::vector<int32_t> m_avecRoleIds[static_cast<size_t>(RegionRole_E::REGION_ROLE_NUM)];
};
}
