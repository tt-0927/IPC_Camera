/**
 * @FilePath     : face_feature.h
 * @Author       : leiyy
 * @Date         : 2026-10-21 9:33:06
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 9:33:06
 * @Description  : 人脸特征表的读写与轮转挑选
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_face_port.h"
#include "classroom_define.h"
#include "detect_decode.h"

namespace Classroom
{
/**
 * 单行特征的存储容量上界（float 个数）。
 * **这是内存预算，不是"期望维度"**：真正的维度口径是底库第一行锁定的那个数
 * （见 face_gallery.h 的 CFaceGalleryStore），比对时两者不等就整行作废并计数。
 * 后端把输出打平成 512+1（多一个 bias）时整表写越界——那是能把堆写坏的错法。
 */
#define CLASSROOM_FACE_FEAT_MAX_DIM (516)

/**
 * 预热全速的「铺开」判据：同一个人攒到这么多次尝试之后，即使每拍节流开着也不再算「还没轮到」
 */
#define CLASSROOM_FACE_WARM_TRIES (2)

/**
 * @brief 一个人的特征行（按跟踪 ID 存，与姿态缓存表同一套键）
 */
typedef struct _FaceFeat_
{
    int32_t nTrackId = -1;                        /* <0 = 空槽 */
    int64_t llFeatMs = 0;                         /* 特征产出时刻（单调毫秒），TTL 按它判 */
    /**
     * 上次「试过给他取特征」的时刻（成功与中途失败都算）。挑选排序用这一个而不是 llFeatMs：
     * 一个总在裁剪/对齐那两步就失败的人如果拿不到时刻，就会永久排在队首占住配额，
     * 把其余所有人挤出去（远焦机位上"脸太小裁不出正方形"是可以同时命中八个人的常态）。
     * 两个时刻刻意不合成一个：llFeatMs 管"这份证据还能不能用"，llTryMs 管"该轮到谁"，
     * 混用后要么失败尝试白扣新鲜度，要么成功产出被当成没做过
     */
    int64_t llTryMs = 0;
    /* 本行攒到的尝试次数（成功与中途失败都算）。只服务预热全速的**退场判据**：
       llTryMs 说的是"上次什么时候试的"，回答不了"这个人已经被铺开过几轮"，
       而节流档要点齐全班名字需要两轮特征。写满后饱和（INT32_MAX），不回绕 */
    int32_t nTryNum = 0;
    int32_t nDim = 0;                             /* 本行实际维度，0 = 没有特征 */
    float   afFeat[CLASSROOM_FACE_FEAT_MAX_DIM] = { 0 };

    /* ---- 比对侧：由管线在拿到特征的那一拍写入，由引擎取走一次 ---- */
    bool    bHasMatch = false;                    /* 有比对结论待取 */
    bool    bMatchTaken = false;                  /* 该结论已被账本取走（见文件头的一次性口径） */
    int64_t llMatchMs = 0;                        /* 比对完成时刻，只做观测，不参与判定 */
    CFaceMatch_S stMatch;
} CFaceFeatEntry_S;

/**
 * @brief 特征表计数（回答"这一拍为什么没有新的比对结果"，与人脸检测计数分开看）
 */
typedef struct _FaceFeatStat_
{
    uint64_t u64Wrote = 0;        /* 写进表的次数（含覆盖同一 ID 的旧特征） */
    uint64_t u64Stamped = 0;      /* 只记「试过」、没写出特征的次数 */
    uint64_t u64RejectNull = 0;   /* 空指针 / 维度非法：调用方与后端契约出了问题 */
    uint64_t u64RejectWide = 0;   /* 维度超过 CLASSROOM_FACE_FEAT_MAX_DIM：换模型没换容量 */
    uint64_t u64RejectFull = 0;   /* 表满：人数上限之外的 ID，只可能是 ID 洪泛 */
    uint64_t u64Pruned = 0;       /* 被 TTL 清掉的行数 */
    uint64_t u64PrunedPending = 0;/* 清行时还挂着**未取走**比对结论的条数：u64MatchSet−u64Taken
                                     的差里有一块是这里丢的，另一块才是"还在等"；不分列就
                                     只能看到差，回答不了"结论是丢了还是没轮到" */
    uint64_t u64MatchSet = 0;     /* 比对结论落到行上的次数 */
    uint64_t u64MatchMissRow = 0; /* 结论找不到行（特征被 TTL 清掉 / ID 已换） */
    uint64_t u64Taken = 0;        /* 被账本取走的次数：与 u64MatchSet 的差 = 取前被清掉 + 还在等 */
} CFaceFeatStat_S;

/**
 * @brief 本拍给哪些脸取特征（轮转挑选参数，取自配置快照）
 */
typedef struct _FaceFeatSelectParam_
{
    int64_t llNowMs = 0;
    /* 本拍最多做几张脸。由 NPU 预算与人脸段的节拍配额在外层算好，本函数只排序与截断 */
    int32_t nWantNum = 1;
    /**
     * 同一个人两次取特征的最小间隔（毫秒，face_feature.min_interval_ms）。
     * <=0 = 不设单人周期，只受上面的配额限制。存在的理由不是省算力而是**防抖**：
     * 同一张脸在同一拍里被反复取特征，两次结果会因为 RGA 取偶与关键点抖动而略有差别，
     * 底库给出的相似度就会在一次课里来回跨门限，名单上的人名随之跳动
     */
    int32_t nMinIntervalMs = 0;
} CFaceFeatSelectParam_S;

/* ============================ 表操作（纯函数，表由调用方持有） ============================ */

/**
 * @brief   : 按跟踪 ID 找行
 * @return   {int32_t} 行下标；没有返回 -1（**返回下标而不是指针**：调用方要在拿到特征后
 *            就地改这一行，返回 const 指针会逼出第二次查找）
 */
int32_t faceFeatIndexOf(const std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId);

/**
 * @brief   : 写入/覆盖一个 ID 的特征
 * @param    {pafFeat / nDim}：源特征。nDim 须 >0 且 <= CLASSROOM_FACE_FEAT_MAX_DIM
 * @param    {llFeatMs}：产出时刻，同时作为 TTL 与单人间隔的基准
 * @return   {bool} false=入参非法或表满（两种都已在 rstStat 计数，调用方不再重复记）
 */
bool faceFeatUpsert(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                    const float *pafFeat, int32_t nDim, int64_t llFeatMs,
                    CFaceFeatStat_S &rstStat);

/**
 * @brief   : 只记「这拍给他试过」，不写特征也不动已有的特征
 * @return   {bool} false=表满且没有空槽可用（计数已在 rstStat，调用方不必重复记）
 */
bool faceFeatStampAttempt(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                          int64_t llNowMs, CFaceFeatStat_S &rstStat);

/**
 * @brief   : 丢掉旧到不该再被用的行
 * @param    {nTtlMs}：新鲜度窗口（face_feature.ttl_ms）
 * @return   {int32_t} 被清掉的行数
 */
int32_t faceFeatPrune(std::vector<CFaceFeatEntry_S> &vecTab, int64_t llNowMs, int32_t nTtlMs,
                      CFaceFeatStat_S &rstStat);

/**
 * @brief   : 这份特征要不要拿去比对（有特征、且这份特征还没有结论）
 */
bool faceFeatNeedsMatch(const CFaceFeatEntry_S &stEntry);

/**
 * @brief   : 把一次比对结论记到行上
 * @return   {bool} false=找不到行（特征已被清掉），已在 rstStat 计数
 */
bool faceFeatSetMatch(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                      const CFaceMatch_S &rstMatch, int64_t llNowMs, CFaceFeatStat_S &rstStat);

/**
 * @brief   : 取走待用的比对结论（读一次即消费）
 * @param    {CFaceMatch_S&} rstOut：OUT=仅 true 时有意义
 * @return   {bool} false=没有待取的结论（从没比过，或已被取走）
 */
bool faceFeatTake(std::vector<CFaceFeatEntry_S> &vecTab, int32_t nTrackId,
                  CFaceMatch_S &rstOut, CFaceFeatStat_S &rstStat);

/**
 * @brief   : 本拍有无人需要比对（纯查询，不改表）
 * @return   {int32_t} 需要比对的行数；0 表示本拍这段可以直接跳过（省一次底库遍历）
 */
int32_t faceFeatNeedMatchNum(const std::vector<CFaceFeatEntry_S> &vecTab);

/* ============================ 轮转挑选 ============================ */

/**
 * @brief 挑选过程的一次候选（临时量，容量由调用方预留）
 */
typedef struct _FaceFeatCand_
{
    int32_t nFaceIdx = -1;    /* 本拍人脸表下标 */
    /* 该跟踪的上次**尝试**时刻（表里的 llTryMs，非 llFeatMs），0 = 从没做过。
       换成证据时刻会让"每次都失败在推理之前"的人永久排队首，见 llTryMs 的说明 */
    int64_t llLastMs = 0;
    float   fArea = 0.0f;     /* 人脸框面积，用于打破同一次数里的先后 */
} CFaceFeatCand_S;

/**
 * @brief   : 挑出本拍该取特征的几张脸
 * @param    {vecFace / vecTrackId}：本拍已解码并已配好行的人脸表（长度 nFaceNum；
 *             vecTrackId 与该人无关的行填 -1，-1 一律不参与）
 * @param    {vecTab}：特征表，用来取每人上次时刻
 * @param    {rstScratch}：临时表，调用方预留到人脸数上限（本函数只用不扩）
 * @param    {rvecOutIdx}：OUT=人脸表下标，长度 ≤ nWantNum，函数内先 clear
 * @return   {size_t} 挑中的张数
 */
size_t selectFaceFeatTargets(const std::vector<CFaceDet_S> &vecFace,
                             const std::vector<int32_t> &vecTrackId,
                             const std::vector<CFaceFeatEntry_S> &vecTab,
                             const CFaceFeatSelectParam_S &rstParam,
                             std::vector<CFaceFeatCand_S> &rstScratch,
                             std::vector<int32_t> &rvecOutIdx);

/**
 * @brief   : 本拍取特征的配额：稳态节流值，或预热全速的容量口径
 * @param    {bPerBeatThrottle}：optimization.feat_per_beat_enable
 * @param    {nPerBeatPersons}：face_feature.per_beat_persons（节流开着时的稳态值）
 * @param    {nMaxPersons}：face_feature.max_persons（容量口径 / 预热全速值）
 * @param    {vecFace / vecTrackId / vecTab}：本拍人脸表、配行表与特征表（与挑选同一份输入）
 * @return   {int32_t} 本拍最多做几张脸（已钳 [1, CLASSROOM_MAX_PERSONS]）
 */
int32_t faceFeatWantNum(bool bPerBeatThrottle, int32_t nPerBeatPersons, int32_t nMaxPersons,
                        const std::vector<CFaceDet_S> &vecFace,
                        const std::vector<int32_t> &vecTrackId,
                        const std::vector<CFaceFeatEntry_S> &vecTab);

/* ============================ 观测出口 ============================ */

/**
 * @brief   : 特征表的一行读数（进心跳）
 * @return   {const char*} pchBuf，便于拼进更长的格式串
 */
const char *faceFeatStatText(const std::vector<CFaceFeatEntry_S> &vecTab,
                             const CFaceFeatStat_S &rstStat, char *pchBuf, int32_t nBufSize);

/**
 * @brief   : 单人两次取特征的最小间隔：配置值与「跑满一轮需要多久」之间的口径统一处
 * @param    {nConfigMs}：face_feature.min_interval_ms，<=0 = 未配（用推导值）
 * @param    {nIntervalMs}：sampling.face_interval_ms（人脸检测段的周期）
 * @param    {nWantNum}：face_feature.max_persons（一轮能做几张）
 * @param    {nFaceNum}：本拍待处理的人脸数
 * @return   {int32_t} 毫秒
 */
int32_t faceFeatIntervalMs(int32_t nConfigMs, int32_t nIntervalMs, int32_t nWantNum, int32_t nFaceNum);
}
