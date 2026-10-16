/**
 * @FilePath     : detection_tracker.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : 统一检测层通用目标跟踪模块（基于 ai_share cIouTracker 算法）。
 *
 * 定位：模型检测输出与事件/取优/统计消费之间的编排组件，供 detection 层各模型复用
 * （人脸取优、展馆 person/head 双跟踪、后续 HVF 周界事件等）。
 *
 * 相对裸用 cIouTracker 的改进：
 *   1. 框语义统一为"左上角 + 宽高"（cIouTracker 内部为中心点语义，转换在模块内封装）；
 *   2. 多类别单入口：检测混喂（带 nClassId），内部按类别分流各自跟踪，单类别模型无感；
 *   3. 状态机输出：TENTATIVE / STABLE / PREDICTED，消费方按状态过滤即可，无需自行防抖；
 *   4. 轨迹结束回收（take_ended）：内置前后帧 diff，告别调用方手工对比。
 *
 * 参数须按模型检测节流调整：跟踪以"检测帧"为时间步，
 *   - 连续高频检测模型（如展馆 3fps）：nMinHits=3、nMaxAge=2（默认）；
 *   - 强节流模型（如人脸默认 3s 一帧）：nMinHits=1、nMaxAge=1。
 */

#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "IouTracker.hpp"

namespace DetectionTracker_NS
{

/*
 * 跟踪目标状态（对齐海思 ot_aidetect_track_status 约定）：
 *   NEW       ~= OT_AIDETECT_TRACK_STATUS_NEW    （首现轨迹）
 *   TRACKING  ~= OT_AIDETECT_TRACK_STATUS_UPDATE （持续命中）
 *   PREDICTED ~= miss 容忍内的预测帧（本模块扩展）
 *   轨迹结束  ~= OT_AIDETECT_TRACK_STATUS_DIE    （经 take_ended() 输出，不经状态枚举）
 */
enum class TrackState_E
{
    NEW = 0,   /* 命中不足 nMinHits：新目标抖动期，仅供 OSD 预览 */
    TRACKING,  /* 稳定跟踪：事件判定 / 取优 / 统计的消费口径 */
    PREDICTED, /* 丢失预测帧（nMaxAge 内）：事件不消费，取优可参考 */
};

/* 跟踪输入：检测框（左上角 + 宽高，与检测输出同坐标系，模块无分辨率依赖） */
struct DetectionInput_S
{
    /* C++11：NSDMI 成员使结构体失去聚合性，补构造函数支持 push_back({...}) 花括号初始化 */
    DetectionInput_S() = default;
    DetectionInput_S(int nLeft_, int nTop_, int nWidth_, int nHeight_, int nClassId_, float fConfidence_, int nUserData_)
        : nLeft(nLeft_), nTop(nTop_), nWidth(nWidth_), nHeight(nHeight_), nClassId(nClassId_), fConfidence(fConfidence_),
          nUserData(nUserData_)
    {
    }

    int nLeft = 0;
    int nTop = 0;
    int nWidth = 0;
    int nHeight = 0;
    int nClassId = 0;
    float fConfidence = 1.0f;
    int nUserData = 0; /* 透传到输出的用户数据（如原始检测数组下标） */
};

/* 跟踪输出：稳定 trackId + 状态 */
struct TrackedTarget_S
{
    int nTrackId = -1;
    int nClassId = 0;
    int nLeft = 0;
    int nTop = 0;
    int nWidth = 0;
    int nHeight = 0;
    float fConfidence = 1.0f;
    TrackState_E enState = TrackState_E::NEW;
    int nHits = 0; /* 连续命中帧数 */
    int nLost = 0; /* 当前连续丢失帧数（STABLE 输出时为 0） */
    int nUserData = 0;
};

/* 本帧结束的轨迹（连续丢失超 nMaxAge 被回收） */
struct EndedTrack_S
{
    /* C++11：同 DetectionInput_S，补构造函数支持花括号初始化 */
    EndedTrack_S() = default;
    EndedTrack_S(int nTrackId_, int nClassId_) : nTrackId(nTrackId_), nClassId(nClassId_)
    {
    }

    int nTrackId = -1;
    int nClassId = 0;
};

/* 通用多类别目标跟踪器 */
class CDetectionTracker
{
public:
    struct Config
    {
        /* C++11：NSDMI 成员使结构体失去聚合性，补构造函数支持 Config{...} 花括号初始化 */
        Config() = default;
        Config(int nMinHits_, int nMaxAge_, float fIouThreshold_, int nMaxTargets_)
            : nMinHits(nMinHits_), nMaxAge(nMaxAge_), fIouThreshold(fIouThreshold_), nMaxTargets(nMaxTargets_)
        {
        }

        int nMinHits = 3;           /* 稳定上报阈值（防单帧误检），按检测节流调整 */
        int nMaxAge = 2;            /* 丢失容忍帧数（期间输出 PREDICTED） */
        float fIouThreshold = 0.3f; /* 帧间关联 IOU 阈值 */
        int nMaxTargets = 0;        /* 单类别轨迹容量上限，0 = 不限 */
    };

    CDetectionTracker() : CDetectionTracker(Config())
    {
    }
    explicit CDetectionTracker(const Config &stCfg);
    ~CDetectionTracker() = default;

    /**
     * @brief   喂入一帧检测（多类别混喂，内部按 nClassId 分流）
     * @return  当前帧全部活跃目标（含三类状态；消费方按 enState 过滤）
     */
    std::vector<TrackedTarget_S> update(const std::vector<DetectionInput_S> &vecDetections);

    /**
     * @brief   取走自上次调用以来结束的轨迹（供事件层做轨迹状态清理）
     */
    std::vector<EndedTrack_S> take_ended();

    /**
     * @brief   清空全部轨迹（事件结束 / 配置切换 / 模型重启）
     */
    void reset();

private:
    /* 按需创建类别跟踪器（内部 cIouTracker 的 minHits 固定 1，状态判定由本层负责） */
    Inference_NS::cIouTracker &ensure_class_tracker(int nClassId);

    Config m_stCfg;
    std::map<int, std::unique_ptr<Inference_NS::cIouTracker>> m_mapTrackers;
    /* 上帧各类别活跃 trackId（ended diff 用） */
    std::map<int, std::set<int>> m_mapLastSeenIds;
    /* 待取走的结束轨迹 */
    std::vector<EndedTrack_S> m_vecEnded;
};

} // namespace DetectionTracker_NS
