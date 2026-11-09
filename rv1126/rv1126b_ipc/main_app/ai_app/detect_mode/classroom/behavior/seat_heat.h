/**
 * @FilePath     : seat_heat.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 14:04:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 14:04:00
 * @Description  : 座位就坐热力
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"
#include "region.h"

namespace Classroom
{
/* 格子数上限：16×8=128 格 */
#define CLASSROOM_SEAT_MAX_COLS (16)
#define CLASSROOM_SEAT_MAX_ROWS (8)
#define CLASSROOM_SEAT_MAX_CELLS (CLASSROOM_SEAT_MAX_COLS * CLASSROOM_SEAT_MAX_ROWS)

/* 改判一格需要的连续拍数。取 3：两拍可能是"抖出去又回来"，三拍在 5s 采样下是 15s，
   已经长过跟踪器一次遮挡重连的空窗，短于一次真实的换座 */
#define CLASSROOM_SEAT_HOLD_BEATS (3)

/**
 * @brief 座位热力参数
 */
typedef struct _SeatHeatConf_
{
    bool    bEnable = true;
    /* 网格边数。默认 16×8 的量级：一间教室后排 8~10 人、纵向 4~6 排，
       一格约等于 1~2 个座位，再细就是给跟踪抖动让路 */
    int32_t nCols = CLASSROOM_SEAT_MAX_COLS;
    int32_t nRows = CLASSROOM_SEAT_MAX_ROWS;
    /* 连续多少拍落在同一格才认这个格（见文件头约束 1）。1 = 逐拍直读，等于放弃防抖 */
    int32_t nHoldBeats = CLASSROOM_SEAT_HOLD_BEATS;
    /* 一格有多大比例落在学生席里才算"座位格"（0~100）。
       取 30 而不是 50：席区边界切过一格是常态（人画的是整块区域，格子是切出来的），
       要求过半会让靠走道那一列整列报 null，而那一列恰恰是"有人坐过道"最需要看见的地方 */
    int32_t nMinCellCoverPercent = 30;
    /* 一个人多久没被看到就把他的格子让出来（拍数）。取 6 拍 = 30s：
       再短，一次组轮转空窗（G_POSE/G_FACE 各 8~20s）就会把人从自己那格里踢出去；
       再长，一节课中途换座会出现两个人抢一格 */
    int32_t nIdleBeats = 6;
} CSeatHeatConf_S;

/**
 * @brief 一格的可判定性：席区没画 / 这格是走道 / 这格有人坐过，三态在快照里分开表达
 */
enum SeatCellState_E : int32_t
{
    SEAT_CELL_CORRIDOR = -2,   /* 格在网格内但不在席区里：报 null（"这里没有座位"） */
    SEAT_CELL_NOGRID   = -1,   /* 整张图无效（学生席没画 / 几何没就绪）：整段不上报 */
};

/**
 * @brief 一拍的逐人位置观测（折样的输入，与特征行同序）
 */
typedef struct _SeatObs_
{
    int32_t nPersonId = -1;
    /* 源帧归一化 0~1；-1 = 这一拍没有位置证据（几何未就绪或人头框退化） */
    float   fNormX = -1.0f;
    float   fNormY = -1.0f;
    /* 教师不入热力图 */
    bool    bStudent = true;
} CSeatObs_S;

/**
 * @brief 一次报表要带的座位快照
 */
typedef struct _SeatSnap_
{
    bool    bEnable = false;
    /**
     * 席区已知 = 学生席角色画了且几何就绪。false 时**整段不上报**，
     * 而不是报一张全 0 的图：全 0 会被平台画成"一间空教室"
     */
    bool    bAreaKnown = false;
    int32_t nCols = 0;
    int32_t nRows = 0;
    /* 网格外接框（源帧归一化 0~1）。平台按自己那份画面尺寸乘回去即可，
       它不需要知道分析帧尺寸、镜像模式或旋转角度 */
    float   fX0 = 0.0f, fY0 = 0.0f, fX1 = 0.0f, fY1 = 0.0f;
    /* 行优先（index = row*cols + col）的人·秒。
       -2 = 走道（不上报为 0），-1 = 整张图无效，>=0 = 这一格本周期累计的在场秒数。
       整表的 -1 由 snapshot() 抹（成员初始化写不出"128 个 -1"，`= { -1 }` 只覆盖首项），
       所以这张表**只能从 snapshot() 拿**，直接默认构造出来的一份不算数 */
    int32_t anSec[CLASSROOM_SEAT_MAX_CELLS] = { SEAT_CELL_NOGRID };
    /* 诊断读数：这份快照折进了多少个人·拍，以及被位置证据挡掉的人数。
       两者一起才答得出"热力图是空的"是为什么 */
    int32_t nPersonNum = 0;
    int32_t nNoPosNum = 0;
} CSeatSnap_S;

/**
 * @brief   : 参数自检（配置装载与热更时调，与 attendanceParamOk 同一口径）
 * @return   {bool} false 表示这套值不能用；rstrErr 是给运维看的一句话（**不回显配置内容**）
 */
bool seatHeatParamOk(const CSeatHeatConf_S &rstConf, std::string &rstrErr);

/**
 * @brief 座位热力累加器
 */
class CSeatHeat
{
public:
    CSeatHeat();

    /**
     * @brief   : 换参数。网格定义变了要清图，只改节拍参数（改判拍数、闲置拍数）不清
     */
    void setParam(const CSeatHeatConf_S &rstConf);
    const CSeatHeatConf_S &param() const { return m_stConf; }

    /**
     * @brief   : 由学生席区域（分析帧）+ 本帧几何重建源帧归一化网格
     * @param    {const CRegionTable&} stRegion：区域表（未画 STUDENT 角色时返回 false）
     * @param    {const CFrameGeometry&} stGeom：本帧几何（未就绪时返回 false）
     * @return   {bool} false = 这张图现在画不出来，调用方**保留上一份网格**并让快照整段不上报
     */
    bool rebuildGrid(const CRegionTable &stRegion, const CFrameGeometry &stGeom);
    bool gridReady() const { return m_stGrid.bReady; }
    /* 学生席角色画过没有。与 gridReady() 分开问才答得出"为什么这张图是空的"（见 m_bAreaKnown） */
    bool areaKnown() const { return m_bAreaKnown; }

    /**
     * @brief   : 折一次样（与群体采样同一拍、同一个步长）
     * @param    {const CSeatObs_S*} patObs：本拍每行的位置观测，与特征行同序
     * @param    {int32_t} nNum：行数。0 是有效折样（空教室）——不记任何秒数，但拍号照进、
     *             闲置的人照老化；把"没人"当"不用调"会让行池在一节空课堂里永不老化
     * @param    {int32_t} nStepMs：这一拍代表多长时长（= 群体采样周期，不是实测间隔）
     */
    void fold(const CSeatObs_S *patObs, int32_t nNum, int32_t nStepMs);

    /**
     * @brief   : 某人当前认定的格子号（行优先），没有返回 -1
     */
    int32_t seatOf(int32_t nPersonId) const;

    /**
     * @brief   : 格子图的**代次**——行池每被抹一次（= 格子编号换过含义）就 +1，见 wipeRows
     */
    uint64_t gridEpoch() const { return m_u64GridEpoch; }

    /**
     * @brief   : 一格的归一化宽度（源帧 0~1），0 = 没有图
     */
    float cellWidth() const { return m_stGrid.bReady ? m_stGrid.fCellW : 0.0f; }

    /**
     * @brief   : 取一份报表快照：**拷出秒数并就地扣掉已报部分**
     * @param    {CSeatSnap_S&} rsnapOut：OUT=整份覆盖
     */
    void snapshot(CSeatSnap_S &rsnapOut);

    /* 一节课收尾：秒数清零、行池清空 */
    void reset();

    /* 心跳一行（`seat=` 段）：只给"为什么这张图是空的"那几项计数 */
    std::string statText() const;

private:
    /* 网格：源帧归一化空间下的外接框 + 每格是否在席区内 */
    typedef struct _Grid_
    {
        bool    bReady = false;
        int32_t nCols = 0;
        int32_t nRows = 0;
        float   fX0 = 0.0f, fY0 = 0.0f, fX1 = 0.0f, fY1 = 0.0f;
        float   fCellW = 0.0f, fCellH = 0.0f;
        uint8_t patInPlay[CLASSROOM_SEAT_MAX_CELLS] = { 0 };
        int32_t nInPlayNum = 0;
    } CGrid_S;

    /* 每人的格子归属（约束 1 的持有者）。按 personId 线性找：人数上限 60，
       一次折样最多 60 次查找，不值得为它上一张哈希表 */
    typedef struct _SeatRow_
    {
        int32_t nPersonId = -1;       /* -1 = 空槽 */
        int32_t nCell = -1;           /* 已认定的格 */
        int32_t nCand = -1;           /* 候选格（还没过众数门） */
        int32_t nCandNum = 0;
        uint64_t u64LastBeat = 0;     /* 第几拍看到他（拍号，不用时刻，见 fold 的注释） */
        int32_t nIdleBeat = 0;        /* 连续多少拍没看到他 */
    } CSeatRow_S;

    /**
     * @brief   : 位置 → 格子号
     * @param    {bool&} rbOutOfBox：OUT=true 表示这一点落在网格外接框之外（区别于走道格）
     * @return   {int32_t} SEAT_CELL_NOGRID=网格没就绪或坐标无效（-1/NaN，即"这一拍没位置证据"）；
     *            SEAT_CELL_CORRIDOR=在框内但这一格不是座位格；>=0=格子号
     */
    int32_t cellOf(float fNormX, float fNormY, bool &rbOutOfBox) const;
    /* 下标查找：indexOf 只找已有行，takeRow 找不到就占槽（槽满时先抢最久没见的那个） */
    int32_t indexOf(int32_t nPersonId) const;
    int32_t takeRow(int32_t nPersonId);
    void clearAccum();
    /* 整张行池抹回空槽。格子编号变了 / 关掉了 / 一节课收尾，三处的理由不同但动作相同 */
    void wipeRows();
    /* 两份网格是否等价（边数、外接框、成格掩码）。决定重建后要不要清账：
       等价就不清，否则"改一个无关键"会把这节课攒的图抹掉 */
    static bool sameGrid(const CGrid_S &stA, const CGrid_S &stB);
    /* 画不出图的原因，一轮只喊一声（引擎会按拍重试，见 seat_heat.cpp 的 warnBuild） */
    void warnBuild(const char *pszFmt, ...);

private:
    CSeatHeatConf_S m_stConf;
    CGrid_S         m_stGrid;
    bool            m_bBuildWarned = false;   /* 见 warnBuild：一轮失败只喊一条日志 */
    /**
     * 学生席角色**画过没有**。与 m_stGrid.bReady 是两件事，分开存才答得出两种"没有"：
     * bReady=false 而本项=true 是"席区画了但这一会儿几何不可用"（临时，保留旧网格等下一拍），
     * 本项=false 是"这台设备压根没画学生区"（永久，整段不该上报）。
     * 合成一个布尔的话，现场为了试镜像把总几何改坏一次，热力图就从"没配"变成"配了但空"
     */
    bool    m_bAreaKnown = false;
    /* 行池构造期定容 CLASSROOM_MAX_PERSONS，此后不再改容量 */
    std::vector<CSeatRow_S> m_vecRow;

    /* 上一次折样的两个读数，随快照一起出去：报表上"这张图全空"时，
       nPersonNum=0 是没人被看着，nPersonNum=40 而 nNoPosNum=40 是几何/换算链断了 ——
       两种"空"的排查方向相反，只给图不给这两个数等于让现场从头猜 */
    int32_t m_nFoldPersonNum = 0;
    int32_t m_nFoldNoPosNum = 0;

    int64_t m_anAccumMs[CLASSROOM_SEAT_MAX_CELLS] = { 0 };
    /* 用 64 位不是习惯性冗余：一格的上界是「人数 × 会话时长」，按配置允许的极限
       （60 人全坐进同一格、session_max_sec=86400）就是 5.18e9 毫秒，int32 在半天课上会翻负，
       而报表侧看到的是一格消失了半节课的负秒数。1 KB 常驻买断这件事，值 */

    /* 折样拍号：每次成功折样 +1，行池用它判"这一拍见到没有"（比时刻可靠：不依赖时基单调，
       也不会因为两拍落在同一个毫秒上而误判成"见过"） */
    uint64_t m_u64Beat = 0;

    /* 格子图代次：wipeRows 每调一次 +1（图重画 / 边数改 / 关掉 / 一节课收尾）。
       与 m_u64Beat 分开存：折样几十拍而图一次没换是常态，两个数各自只有一个含义 */
    uint64_t m_u64GridEpoch = 0;

    /* 计数：每个"没有"都要有一个数 */
    uint64_t u64Fold = 0;             /* 成功折样的拍数 */
    uint64_t u64SkipNoGrid = 0;       /* 网格没画出来，整拍没折 */
    uint64_t u64OutOfGrid = 0;        /* 人在画面里但不在席区外接框内（后排坐到了图外） */
    uint64_t u64Corridor = 0;         /* 落进走道格：有位置但不计入任何座位格 */
    uint64_t u64CellMove = 0;         /* 改判发生次数：这个数高说明 n_hold_beats 配小了 */
    uint64_t u64RowFull = 0;          /* 行池满，丢了人 */
};
}
