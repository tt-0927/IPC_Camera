/**
 * @FilePath     : person_tracker.h
 * @Author       : leiyy
 * @Date         : 2026-10-12 10:40:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-12 10:40:00
 * @Description  : 人头跟踪器
 */

#pragma once

#include <cstdint>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* 跟踪器容量：与特征帧人数上限同值，一次分配、运行期零 new（§1.2 红线） */
#define CLASSROOM_TRACK_MAX (CLASSROOM_MAX_PERSONS)

/* 关联主门限（静态部分）：中心距离 / 两框对角线均值 超过该值即视为不同人。
   0.6 是"允许一个人每帧位移到自身框外 6 成对角线"的松值——教室里的真实位移远小于此，
   宁可略松（短暂跟错、下一帧纠正）也不要紧到把同一个人拆成两个 ID */
#define CLASSROOM_TRACK_GATE_NORM_DIST (0.60f)

/* 关联门限的运动部分（像素/毫秒）：允许的额外位移 = 该值 × 距上次命中的毫秒数。
   有了这一项，"走动的教师"才不会每帧都变成新 ID——速度预测负责系统性的位移，
   本项只兜预测误差（首帧速度为 0、以及模型框本身的抖动） */
#define CLASSROOM_TRACK_MOVE_SLACK_PX_PER_MS (0.25f)

/* 门限上限（按对角线倍数）：遮挡 1 秒后轨迹可能已经跑出很远，若不封顶，
   一条久未更新的轨迹会以"很大的容差"吞掉附近任何一个新出现的人头 */
#define CLASSROOM_TRACK_MAX_GATE_DIAG (2.0f)

/* 速度上限（像素/秒）：速度 EMA 的硬化钳。单帧跳变（漏检一拍后框跳回真位置、
   邻座短暂遮挡后错框一下、检测模型本身在遮挡沿抽风）会让"这次位移 / 这一拍的间隔"
   算出一个远超人体的速度，而速度直接进匹配外推 `predPos + v×Δt`——
   一条被污染的速度会把后续几拍的预测点甩到画面外，于是本来该匹配上的检测
   因"离预测点太远"全部失配，一个人被拆成好几个新 ID。
   1080p 下课堂里人能有的真实速度：快走 ~300px/s、学生起身 ~500px/s，
   600 给了一倍余量（与 subject_arbiter.cpp 的 CLASSROOM_MOVE_MAX_SPEED_PX_S 同值，
   两处必须一致——一个是"位移算不算走动"，一个是"这个速度能不能用来外推"） */
#define CLASSROOM_TRACK_MAX_SPEED_PX_S (600.0f)

/* 尺寸比惩罚权重（#93-2，相对量）：关联成本乘上 (1 + w×(大对角/小对角 − 1))。
   同一个人相邻两拍的框尺寸接近（比值≈1 → 乘数恰为 1，成本与旧式逐位相同，
   现网普通目标不受影响）；只有"尺寸明显不符"的候选（邻座误配、远近景串档、
   大小模型档切换）被抬高，贪心自动把这次检测让给尺寸相符的那条轨迹。
   这是**排序权重**不是硬门——硬门是 0.60 那一道，两者语义分开才各自可调；
   0 = 关闭，回到纯距离+IoU。weights 上限在配置层钳（tracker.size_penalty） */
#define CLASSROOM_TRACK_SIZE_PENALTY (1.0f)

/* 新建轨迹被认定为"确认"所需的最少命中次数（防单帧误检立刻产生 ID） */
#define CLASSROOM_TRACK_MIN_HITS (2)

/* 轨迹连续消失多久即回收（毫秒）。不用帧数：帧率会随守护降级变化，
   用时间才保证"最短持续 1.5s"这类参数在不同档位下含义一致 */
#define CLASSROOM_TRACK_KEEP_MS (1200)

/**
 * @brief 跟踪器参数
 */
typedef struct _TrackerParam_
{
    float   fGateNormDist = CLASSROOM_TRACK_GATE_NORM_DIST;
    float   fMoveSlackPxPerMs = CLASSROOM_TRACK_MOVE_SLACK_PX_PER_MS;
    /* 尺寸比惩罚权重（#93-2）：cost ×(1 + w×(对角大/小 −1))，比值=1 不动；0=关 */
    float   fSizePenalty = CLASSROOM_TRACK_SIZE_PENALTY;
    int32_t nMinHits = CLASSROOM_TRACK_MIN_HITS;
    int32_t nKeepMs = CLASSROOM_TRACK_KEEP_MS;
    /* 框小于该边长（像素）直接丢弃：远处噪点不值得占一条轨迹，
       且小框中心极不稳，留着会把邻座的 ID 抢走 */
    int32_t nMinBoxEdge = 6;
} CTrackerParam_S;

/**
 * @brief 一条轨迹的对外快照（只读，指针有效期到下一次 update()）
 */
typedef struct _TrackItem_
{
    int32_t     nTrackId = -1;        /* 全生命周期唯一，回收后不复用 */
    CRectf_S    stBox;                /* 平滑后的框，分析帧坐标 */
    CRectf_S    stRawBox;             /* 本帧检测原框（未平滑），抓拍/OSD 要原框 */
    int32_t     nHits = 0;            /* 累计命中帧数 */
    int32_t     nConsecutiveMiss = 0; /* 连续丢帧数，0=本帧刚到 */
    bool        bConfirmed = false;   /* 命中数达门限，行为判定只认已确认轨迹 */
    bool        bNewThisFrame = false;
    /* 本帧命中的检测下标（指向 update() 入参 vecDet），未命中为 -1。
       感知层要用它把跟踪 ID 关联到同帧的其它检测（人体框、人脸框）；
       坐标反查在平滑框上会偶发错配，故直接给下标 */
    int32_t     nDetIdx = -1;
    float       fVxPxPerMs = 0.0f;    /* 平滑速度，巡视轨迹/静止判定的依据 */
    float       fVyPxPerMs = 0.0f;
    DetectSource_E enSource = DetectSource_E::DET_SRC_HEAD; /* 本帧框来自哪个检测器 */
    float       fScore = 0.0f;
} CTrackItem_S;

/**
 * @brief 跟踪器观测计数（心跳出口）
 */
typedef struct _TrackerStat_
{
    uint64_t u64Updates = 0;
    uint64_t u64Detections = 0;
    uint64_t u64Matched = 0;
    uint64_t u64TracksCreated = 0;
    uint64_t u64TracksDropped = 0;    /* 因容量满被丢弃的检测数（不是回收数） */
    /* 计数落点是「建轨时表已满」：这条路径**不回收旧 ID**（跟踪 ID 单调分配、永不复用），
       所以旧名 u64IdReuseRejected 说的是一个不存在的动作。>0 ⇒ 本拍有检测没能起轨，
       该扩的是容量而不是查复用。心跳线上键仍叫 `idrej=`（改名会动现场已存档的解析脚本），
       读心跳的人按本注释理解，别按键名 */
    uint64_t u64TrackPoolFull = 0;
    int32_t  nActiveMax = 0;
    int32_t  nConfirmedMax = 0;
    /* ID 水位：现网排查"人数对不上"时，第一个要看的就是有没有 ID 洪泛 */
    int32_t  nNextId = 0;
} CTrackerStat_S;

/**
 * @brief   : 单目标跟踪器（人头/人体共用同一实现，各自持有实例）
 * @note    : 非线程安全，设计为单线程持有（引擎节拍线程）
 */
class CPersonTracker
{
public:
    /* 所有工作缓冲按编译期上限一次定容：update() 运行期零分配（§1.2 红线"每帧 new 即设计违规"） */
    CPersonTracker()
    {
        m_vecPick.reserve(CLASSROOM_TRACK_MAX);
        m_vecPickIdx.reserve(CLASSROOM_TRACK_MAX);
        m_vecDetOfTrack.reserve(CLASSROOM_TRACK_MAX);
        m_vecTrackOfDet.reserve(CLASSROOM_TRACK_MAX);
        m_vecOut.reserve(CLASSROOM_TRACK_MAX);
        m_vecTracks.reserve(CLASSROOM_TRACK_MAX);
        m_vecCost.reserve(CLASSROOM_TRACK_MAX * CLASSROOM_TRACK_MAX);
        m_vecDetOrder.reserve(CLASSROOM_TRACK_MAX * CLASSROOM_TRACK_MAX);
    }

    CPersonTracker(const CPersonTracker &) = delete;
    CPersonTracker &operator=(const CPersonTracker &) = delete;

    void setParam(const CTrackerParam_S &stParam);
    CTrackerParam_S param() const { return m_stParam; }

    /**
     * @brief   : 用一帧检测更新轨迹集合
     * @param    {const std::vector<CDetection_S>&} vecDet：分析帧坐标，调用方保证非空指针有效
     * @param    {int64_t} llNowMs：单调时基（classroomNowMs 口径），0 表示按帧序推进
     * @return   {const std::vector<CTrackItem_S>&} 本帧有效轨迹（含未确认的），引用内部缓冲
     * @note    : 顺序不保证稳定，下游按 nTrackId 索引；调用方不得跨 update() 持有返回引用
     */
    const std::vector<CTrackItem_S> &update(const std::vector<CDetection_S> &vecDet, int64_t llNowMs);

    /**
     * @brief   : 清空全部轨迹（换配置、几何变化、功能重开时调用）
     * @note    : ID 计数器**不复位**——复位会让新旧轨迹撞号，规则层的时间门会把
     *            上一个人的持续时间算到下一个头上，这是最难复现的一类错
     */
    void reset(bool bResetId = false);

    CTrackerStat_S stat() const { return m_stStat; }
    void clearStat();

    int32_t activeNum() const { return static_cast<int32_t>(m_vecTracks.size()); }
    int32_t confirmedNum() const;

private:
    /* 内部轨迹槽，含未确认与在途丢失的 */
    typedef struct _Track_
    {
        int32_t nId = -1;
        CRectf_S stBox;
        CRectf_S stRawBox;
        int32_t nHits = 0;
        int32_t nMiss = 0;
        bool bConfirmed = false;
        bool bAliveThisFrame = false;
        int32_t nDetIdx = -1;         /* 本帧命中的入参下标，未命中 -1 */
        float fVx = 0.0f;
        float fVy = 0.0f;
        int64_t llLastMs = 0;
        DetectSource_E enSource = DetectSource_E::DET_SRC_HEAD;
        float fScore = 0.0f;
    } CTrackInternal_S;

    /* 关联代价：越小越像同一人；返回负值表示不可关联。
       llNowMs 用于把轨迹中心按速度外推到本帧时刻再比距离（见文件头"匹配度量"一段） */
    float matchCost(const CTrackInternal_S &stTrack, const CDetection_S &stDet, int64_t llNowMs) const;

    /* 按代价升序做贪心一对一匹配（检测数 ≤60，不必上匈牙利，见文件头说明）。
       m_vecCost/m_vecDetOrder 只装"可关联"的配对，故列数必须由调用方给出 */
    void associate(int32_t nDetNum, std::vector<int32_t> &rvecDetOfTrack, std::vector<int32_t> &rvecTrackOfDet);

private:
    CTrackerParam_S m_stParam;

    std::vector<CTrackInternal_S> m_vecTracks;   /* 容量 CLASSROOM_TRACK_MAX，init 期定死 */
    std::vector<CTrackItem_S> m_vecOut;
    /* 匹配用的代价表，同样预分配（update 内零分配是硬要求） */
    std::vector<float> m_vecCost;
    std::vector<int32_t> m_vecDetOrder;
    /* 本轮入选检测（指回入参，不复制框）与双向匹配结果 */
    std::vector<const CDetection_S *> m_vecPick;
    /* 与 m_vecPick 平行：入选者对应到入参 vecDet 的下标。过滤器会丢掉小框/坏分数，
       少了这张表就无法把轨迹映射回调用方自己的下标 */
    std::vector<int32_t> m_vecPickIdx;
    std::vector<int32_t> m_vecDetOfTrack;
    std::vector<int32_t> m_vecTrackOfDet;

    int32_t m_nNextId = 1;
    CTrackerStat_S m_stStat;
};

}
