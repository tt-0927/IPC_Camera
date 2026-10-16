/**
 * @FilePath     : bestshot_types.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 抓拍取优公共类型（评分后端 + 选优器 + 门面共用）。
 */

#pragma once

#include <cstdint>

namespace Bestshot_NS
{
/* 帧内矩形（像素坐标，调用方保证在帧内） */
struct BestShotRect_S
{
    int32_t nX = 0;
    int32_t nY = 0;
    uint32_t unWidth = 0;
    uint32_t unHeight = 0;
};

/* 帧视图：CPU 后端读虚拟地址，硬件后端（IVE 等）用物理地址，二者由后端各取所需 */
struct BestShotFrameView_S
{
    const uint8_t *pYVirAddr = nullptr;
    uint64_t unYPhysAddr = 0;
    uint32_t unStride = 0;
};

/* 取优喂入项（清晰度由模块内部评分后端计算，调用方不填） */
struct BestShotCandidateInput_S
{
    BestShotRect_S stRoi; /* 与出图一致的裁剪框；无效 ROI 时全零（仅置信度维度参与） */
    float fDetectScore = 0.0f;
    uint64_t unArea = 0;       /* 目标面积（像素数，参与面积维度评分） */
    long long llTimestamp = 0; /* 毫秒时间戳 */
    float fBrightness = -1.0f; /* ROI 平均亮度 [0,255]；-1 表示未知（查询失败或平台未接入），
                                  亮度门限与评分整体跳过，回退三维评分 */
};

/* feed 结果：调试日志由调用方按需打印，模块自身零日志依赖 */
struct BestShotFeedResult_S
{
    bool bNewBest = false;      /* 本帧是否成为新 best（调用方此时编码缓存小图） */
    bool bGated = false;        /* 本帧是否被亮度门限否决（过暗/过曝，未参与评选） */
    bool bGateBypassed = false; /* 首帧保底放行：该目标此前无候选帧，虽过暗/过曝
                                   仍保底入桶（保证全程逆光/过曝的目标也有图出） */
    float fSharpness = -1.0f;   /* 本帧清晰度 [0,1]，-1 表示未评分/评分失败 */
    float fTotalScore = -1.0f;  /* 本帧综合分；被门限否决时保持 -1 */
    float fBestScore = -1.0f;   /* 当前 best 综合分，无 best 时 -1 */
};

/* 取优配置 */
struct BestShotConfig_S
{
    float fHysteresis = 0.1f;        /* 滞回比例：新分须超过 best*(1+该值) 才切换，防抖动 */
    long long llHoldWindowMs = 5000; /* 无候选保持窗口：超时后 best 失效（目标已离开） */
    uint32_t unMaxTargets = 32;      /* 选优桶数量上限，防调用方漏清理导致 map 无界增长 */
    float fBrightnessMin = 40.0f;    /* ROI 平均亮度下限：低于判过暗，一票否决不参与评选 */
    float fBrightnessMax = 220.0f;   /* ROI 平均亮度上限：高于判过曝，一票否决不参与评选 */
    float fDimLightTh = 60.0f;       /* 暗光阈值：亮度低于该值时清晰度按比例降权（夜间高增益
                                        噪声让梯度占比虚高，不能全信）；<=0 关闭降权 */
};
} // namespace Bestshot_NS
