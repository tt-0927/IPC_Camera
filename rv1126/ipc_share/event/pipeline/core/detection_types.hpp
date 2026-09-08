/**
 * @FilePath     : detection_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : AI 检测结果标准化契约类型定义
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace AiPipeline_NS
{
/* 语义目标类型，与厂商模型类别解耦 */
enum class ObjectType_E
{
    UNKNOWN = 0, /* 未知类型 */
    PERSON,      /* 行人 */
    HEAD,        /* 人头（人员密度统计目标） */
    FACE,        /* 人脸 */
    VEHICLE,     /* 车辆 */
    PET,         /* 宠物 */
    BAG,         /* 包裹 */
    GARBAGE,     /* 垃圾 */
    WALLET,      /* 钱包 */
    PHONE,       /* 手机 */
};

/* 跟踪生命周期状态 */
enum class TrackState_E
{
    UNAVAILABLE = 0, /* 无稳定跟踪，禁止依赖 Track 的处理器使用 */
    STARTED,         /* 跟踪开始，首次出现 */
    TRACKED,         /* 跟踪持续更新 */
    ENDED,           /* 跟踪结束，保留 Track ID 用于状态清理 */
};

/* 检测能力位，用于分发器筛选处理器 */
enum class DetectionCapability_E : uint32_t
{
    NONE = 0U,                  /* 无能力 */
    OBJECT_DETECTION = 1U << 0, /* 普通目标检测 */
    TRACK_ID = 1U << 1,         /* 目标带稳定 Track ID */
    TRACK_LIFECYCLE = 1U << 2,  /* 目标带 NEW/UPDATE/DIE 生命周期 */
};

/**
 * @brief   : 能力位类型安全辅助，禁止调用方散落整数位运算
 */
namespace capability
{
/**
 * @brief   : 判断能力位集合是否包含指定能力
 * @param    {uint32_t} unCaps：能力位集合
 * @param    {DetectionCapability_E} enCap：待判断能力
 * @return   {bool} true：包含 false：不包含
 */
inline bool has(uint32_t unCaps, DetectionCapability_E enCap)
{
    return (unCaps & static_cast<uint32_t>(enCap)) != 0U;
}

/**
 * @brief   : 设置能力位
 * @param    {uint32_t &} unCaps：能力位集合
 * @param    {DetectionCapability_E} enCap：待设置能力
 * @return   {void}
 */
inline void set(uint32_t &unCaps, DetectionCapability_E enCap)
{
    unCaps |= static_cast<uint32_t>(enCap);
}

/**
 * @brief   : 判断能力位集合是否覆盖全部必需能力
 * @param    {uint32_t} unCaps：能力位集合
 * @param    {uint32_t} unRequired：必需能力位集合
 * @return   {bool} true：全部覆盖 false：存在缺失
 */
inline bool covers(uint32_t unCaps, uint32_t unRequired)
{
    return (unCaps & unRequired) == unRequired;
}
} // namespace capability

/* 帧分辨率元数据 */
struct FrameSize_S
{
    uint32_t nWidth = 0;  /* 宽度 */
    uint32_t nHeight = 0; /* 高度 */
};

/* 归一化矩形，相对原生结果坐标分辨率归一化到 [0,1] */
struct NormalizedRect_S
{
    double dLeft = 0.0;   /* 左边界 */
    double dTop = 0.0;    /* 上边界 */
    double dRight = 0.0;  /* 右边界 */
    double dBottom = 0.0; /* 下边界 */
};

/* 单个检测目标 */
struct DetectionObject_S
{
    ObjectType_E enType = ObjectType_E::UNKNOWN;           /* 语义目标类型 */
    float fConfidence = 0.0F;                              /* 置信度 [0,1] */
    bool bHasGeometry = false;                             /* 是否携带有效几何，ENDED 目标允许无几何 */
    NormalizedRect_S stRect;                               /* 归一化矩形 */
    std::optional<uint64_t> optTrackId;                    /* 可选 Track ID，0 仍是合法值 */
    TrackState_E enTrackState = TrackState_E::UNAVAILABLE; /* 跟踪状态 */
};

/* 帧元数据：三种尺寸显式分离，禁止互相隐式替代 */
struct FrameMetadata_S
{
    int nChannelId = 0;                 /* 媒体通道号 */
    uint64_t ullFrameId = 0;            /* 单调递增帧标识，同一帧全链路共用 */
    int64_t llWallTimestampMs = 0;      /* wall clock 毫秒时间戳，用于周期上报与定时清零 */
    int64_t llMonotonicTimestampMs = 0; /* monotonic 毫秒时间戳，用于 Track TTL 与报警生命周期 */
    FrameSize_S stSourceFrameSize;      /* 实际源帧分辨率 */
    FrameSize_S stModelInputSize;       /* 模型输入分辨率 */
    FrameSize_S stNativeResultSize;     /* 原生检测结果坐标分辨率，几何归一化的基准 */
    uint32_t unCapabilities = 0;        /* 检测能力位集合 */
};

/* 标准检测批次：构造完成后只读分发 */
struct DetectionBatch_S
{
    FrameMetadata_S stMetadata;                /* 帧元数据 */
    std::vector<DetectionObject_S> vecObjects; /* 检测目标集合 */
};
} // namespace AiPipeline_NS
