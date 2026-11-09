/**
 * @FilePath     : pose_decode.h
 * @Author       : leiyy
 * @Date         : 2026-10-15 9:46:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-15 10:30:00
 * @Description  : 姿态输出的解码与轮转挑选
 */

#pragma once

#include <cstdint>
#include <vector>

#include "classroom_define.h"
#include "detect_decode.h"
#include "feature_frame.h"

namespace Classroom
{
/**
 * @brief 解码参数（取自配置快照，需求 11：全部可配）
 */
typedef struct _PoseDecodeParam_
{
    /* 单点置信度门：低于此值的点视为不可用（fScore 原样保留，规则层按各自判据再设门） */
    float   fKeyScore = CLASSROOM_POSE_SCORE_THRESHOLD;
    /* 躯干可用点数下限：不足则整副骨骼判无效 */
    int32_t nMinTrunkPoints = CLASSROOM_POSE_MIN_TRUNK_POINTS;
    /* 解码后是否把点钳制在分析画布内由 CDetMap 负责，本层不重复钳 */
    int32_t nKeypointNum = CLASSROOM_POSE_KEYPOINT_NUM;
} CPoseDecodeParam_S;

/**
 * @brief 解码参数（挑选本拍给谁做姿态，§4.4 的按人轮询落地点）
 */
typedef struct _PoseSelectParam_
{
    int64_t llNowMs = 0;
    /* 单人两次姿态的最小间隔（sampling.pose_interval_ms）。距今不足此值的人不参与挑选。
       这里刻意不给默认值：配置项才是唯一真值，写第二个默认值就等于留一处
       「改了配置没生效」的现成事故点。<=0 表示不设单人周期，只受下面的配额限制 */
    int32_t nIntervalMs = 0;
    /* 本拍还允许做几个人：由 NPU 预算与周期配额在外层算好后传进来，本函数只管排序与截断 */
    int32_t nWantNum = 1;
    /**
     * 遮挡行整行跳过（bOccluded）。规则层对遮挡行的所有行为一律给 SR_OCCLUDED，
     * 也就是说这一行做出来的骨骼**必然**被扔掉——花一次 NPU 换一份必定作废的证据是纯亏，
     * 而这次亏掉的名额本可以给隔壁那个还没骨骼的人
     */
    bool bSkipOccluded = true;
    /**
     * 清晰度低于此值的行跳过（<=0 表示不按清晰度筛）。理由与上一项相同（SR_BLUR 整行不判）。
     * 刻意不自己定一个值：那会与 threshold.pose_blur_min 成为两个真值，
     * 出现「感知层不采样、规则层却还在等这行证据」的错位
     */
    float fBlurMin = 0.0f;
    /* 规则层上一拍的紧急度表（借用指针，生命周期覆盖本函数调用即可）。空/0 条 = 没人等。
       语义与标尺见 feature_frame.h 的 CLASSROOM_POSE_URG_*：排序只决定「谁先」，
       不决定「做不做」——遮挡/糊行照旧整行滤掉 */
    const CPoseUrgencyHint_S *pastUrgency = nullptr;
    int32_t nUrgencyNum = 0;
    /* 姿态证据窗口毫秒（帧上 poseTtlMs() 的实测值）：>0 时启用「临近过期提前续期」一档。
       取实测值不取配置的理由同 runPoseStage 顶部那条——拿希望值算窗口就是后排漏检的成因 */
    int32_t nPoseTtlMs = 0;
} CPoseSelectParam_S;

/**
 * @brief 一副骨骼的跨帧缓存行（按跟踪 ID 存，§4.4「骨骼结果缓存」的载体）
 * @note  存在的理由不是省一次推理，而是**让没被轮到的那一拍也有证据**：
 *        姿态按 max_pose_persons 限流，60 人的教室轮到某人可能要 5 拍，
 *        期间他的特征行如果空着，规则层每拍都会记一次 SR_NO_POSE，
 *        时长门就把一段连续的行为切成好几条短事件（需求 10 的抖动红线）
 */
typedef struct _PoseCacheEntry_
{
    int32_t         nTrackId = -1;        /* <0 = 空槽 */
    int64_t         llPoseMs = 0;         /* 该骨骼产出时刻（单调毫秒） */
    CPoseSkeleton_S stSkl;                /* 已是分析帧坐标 */
} CPoseCacheEntry_S;

/**
 * @brief 姿态解码计数（观测出口：回答「为什么这拍没有骨骼」）
 */
typedef struct _PoseDecodeStat_
{
    uint64_t u64Decoded = 0;          /* 解出一份骨骼的次数（含判无效的，见下两行） */
    uint64_t u64Usable = 0;           /* bValid=true 的次数 */
    uint64_t u64TooShort = 0;         /* 浮点数不够 26×3：后端输出与拓扑约定对不上 */
    uint64_t u64NotFinite = 0;        /* NaN / inf 出现：喂进去的图或模型本身有问题 */
    uint64_t u64MapNotReady = 0;      /* 映射没建起来，这份结果整副丢弃 */
    uint64_t u64WeakTrunk = 0;        /* 躯干点不足：判无效，但点仍写回供调试绘制 */
    /**
     * 耳证据普查（#65 第一刀）：鼻 + 双耳三点都过 fKeyScore 的副数。
     * 与 u64Usable 不是一回事——本数问的是**耳**，躯干够不够点不影响它。
     * 为什么要单列一个数：文档给"抬头"的必然判断是「鼻高于两耳」，而本仓 26 点表里的双耳
     * （索引 3/4）在**全仓从未被任何判据读过**（grep 只命中枚举定义本身）。要么这颗模型的耳点
     * 根本不可信（那就永远不该接进来），要么可信（那就该接，且能补上夹角门 120° 漏掉的那批强仰头）。
     * 这两种情况的修法相反，而区分它们只要一个比值 ⇒ 先留读数，不动判据。
     */
    uint64_t u64EarUsable = 0;        /* 鼻与双耳三点分数都够的一副 */
    uint64_t u64EarNoseAbove = 0;     /* 上一项成立、且鼻的 y 高于双耳（文档"抬头必然判断"的候选） */
    /**
     * 文档口径普查（#65 第三刀，与上面两笔同一纪律：**只计数，不接任何判据出口**）。
     * 为什么还要这一刀： above 只覆盖文档抬头那条**必然判断**（鼻高于双耳）的极端形状；
     * 两条**阈值判断**的主条款——d1/d2>0.2，d1=眼中点到耳线的距离、d2=颈点到耳线的距离
     * 接不接、按哪边接都没有依据。这里把文档几何原样量出来：五点分数可用数作分母，
     * 异侧/同侧两笔作分子，下一个真实课堂就能直接裁决 geometry.h:37-39 那段"不读耳"的理由。
     */
    uint64_t u64EarLineUsable = 0;    /* 双眼双耳颈五点分数都够的一副（文档几何的分母；鼻不参与） */
    uint64_t u64DocUpShape = 0;       /* 上项成立、眼中点与颈在耳线异侧且 d1/d2>0.2（文档抬头阈值形状） */
    uint64_t u64DocDownShape = 0;     /* 上项成立、眼中点与颈在耳线同侧且 d1/d2>0.2（文档低头阈值形状） */
} CPoseDecodeStat_S;

/**
 * @brief   : 打平浮点输出 → 分析帧坐标系的一副骨骼
 * @param    {const std::vector<float>&} vecRaw：nKeypointNum × CLASSROOM_POSE_POINT_FLOAT_NUM，
 *             坐标是**模型输入像素**（FastPose 为 192×256 那张放大图），不是归一化值（P0-1 实测）
 * @param    {const CDetMap&} rstMap：本次裁剪实际用的源帧矩形 → 模型输入的映射，
 *             必须与真正喂进去的那张图同源，否则点会整体偏移（见 cropRoi 的 pstUsedSrc）
 * @param    {const CPoseDecodeParam_S&} rstParam：分数门与容量
 * @param    {CPoseSkeleton_S&} rstOut：OUT=分析帧坐标的 26 点；无效时 bValid=false 但点仍填好
 * @param    {CPoseDecodeStat_S&} rstStat：累加
 * @return   {bool} false 表示这份输出根本解不出来（长度不足 / 映射未就绪 / 有非有限值），
 *                  调用方必须按「本拍无证据」处理，不得把这一行当成「没这个人」
 * @note     : bValid 的判据只看躯干点（索引 5~19），**面部点不计入**：
 *            「鼻与双眼集体低分而躯干清楚」正是背身那一副骨骼的样子，
 *            把它判无效会让背身行为永远出不来（judgeTurnBack 要的就是这个特征）
 */
bool decodePose(const std::vector<float> &vecRaw, const CDetMap &rstMap,
                const CPoseDecodeParam_S &rstParam, CPoseSkeleton_S &rstOut,
                CPoseDecodeStat_S &rstStat);

/* ============================ 缓存表操作（纯函数，表本身由调用方持有） ============================ */

/**
 * @brief   : 按跟踪 ID 找缓存行
 * @return   {const CPoseCacheEntry_S*} 没有返回 nullptr
 */
const CPoseCacheEntry_S *poseCacheFind(const std::vector<CPoseCacheEntry_S> &vecCache, int32_t nTrackId);

/**
 * @brief   : 写入/覆盖一个 ID 的缓存行
 * @param    {int32_t} nCapInUse：当前已用槽数，OUT=写入后的值（新 ID 才 +1）
 * @return   {bool} false 表示表满（人数上限之外不该有第六十一个 ID，满了只可能是 ID 洪泛）
 * @note     : 空槽优先复用最靠前的那个，不做 LRU 淘汰：表容量与人数上限同值，
 *            装不下的情况本身就是异常，异常要靠 u64PoseCacheFull 这个读数去看，不靠悄悄替换
 */
bool poseCacheUpsert(std::vector<CPoseCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                     int32_t nTrackId, const CPoseSkeleton_S &rstSkl, int64_t llPoseMs);

/**
 * @brief   : 丢掉「已经旧到规则层不会再用」的缓存行
 * @param    {int32_t} nTtlMs：新鲜度窗口，与规则层同一个 poseEvidenceTtlMs
 * @return   {int32_t} 被清掉的槽数
 * @note     : 清表按 llPoseMs 而不按「这个 ID 还在不在」：跟踪器给同一个人生成新 ID 时，
 *            旧行自然会因为不再被刷新而过期；反过来留着刚掉线的人等 ID 复用是危险的，
 *            那会让新来的人继承上一位的骨骼（现象是「一坐下就举手」）
 */
int32_t poseCachePrune(std::vector<CPoseCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                       int64_t llNowMs, int32_t nTtlMs);

/**
 * @brief   : 把缓存里仍然新鲜的骨骼抄进特征行
 * @param    {int64_t} llTtlMs：与规则层 poseEvidenceTtlMs 同值，两处不一致会让「有证据」两种口径
 * @return   {int32_t} 拿到骨骼的行数
 * @note     : 行号与跟踪 ID 的对应关系在写行时就已确定（stRow.nPersonId），这里只查表；
 *            没查到的行保持 acquire() 清零后的默认（bValid=false、llPoseMs=0），
 *            规则层据此记 SR_NO_POSE —— 那是「这拍没证据」而不是「这人没站着」
 */
int32_t applyPoseCache(CPersonFeature_S *patRows, int32_t nRowNum,
                       const std::vector<CPoseCacheEntry_S> &vecCache,
                       int64_t llNowMs, int64_t llTtlMs);

/**
 * @brief 挑选过程的一次候选（纯函数内部的临时量，容量由调用方预留）
 */
typedef struct _PoseCand_
{
    int32_t nRow = -1;        /* 特征行下标 */
    int64_t llLastMs = 0;     /* 上次姿态时刻，0 = 从没做过 */
    float   fArea = 0.0f;     /* 人体框面积，用于打破同一次数里的先后 */
    int32_t nUrgency = 0;     /* 紧急度 [0,1000]：规则提示 ∨ 临近过期，主排序键 */
} CPoseCand_S;

/**
 * @brief   : 挑出本拍该做姿态的行
 * @param    {patRows / nRowNum}：本拍已写好的特征行（stBodyBox 决定优先级）
 * @param    {vecCache}：当前缓存表，用来取每人上次姿态时刻
 * @param    {rstParam}：时间闸与本拍配额
 * @param    {std::vector<CPoseCand_S>&} rstScratch：临时表，调用方预留到人数上限（本函数只用不扩）
 * @param    {std::vector<int32_t>&} rvecOutRow：OUT=行下标，长度 ≤ nWantNum，函数内先 clear
 * @return   {size_t} 挑中的行数
 * @note     : 排序键 =（紧急度降序，同档最久没做优先，再按人体框面积大优先）：
 *            「紧急度」是主键（P0-1 接近激活阈值优先）：规则层在等证据的人（生效中的行为靠
 *            容忍额度撑着 / 攒时长攒到阈值附近）与证据临近过期的人先做——60 人转一圈要
 *            几十秒，纯公平轮转等得起、瞬时行为等不起；
 *            「同档最久没做」保留原公平红线：紧急度是粗分档（0/500/600~999/1000），
 *            最大的一档往往站着几十个人，若在档内改用面积优先，后排会立刻回到
 *            「整节课没有骨骼」的老坑——档内公平、档间让急，两头才都守得住；
 *            「面积大优先」只用于打破同一次数里的先后：面积大意味着离镜头近、关键点更准，
 *            也意味着他更可能是被判定关注的人（§4.4 的面积优先口径）。
 *            从没做过姿态的人（缓存里没有）记 llLast=0，在无人加急时自然排最前，
 *            这是冷启动能铺开的原因。
 *            面积取人体框而不是头框：同一颗镜头下头框几乎恒定，区分不出谁更近
 */
size_t selectPoseTargets(const CPersonFeature_S *patRows, int32_t nRowNum,
                         const std::vector<CPoseCacheEntry_S> &vecCache,
                         const CPoseSelectParam_S &rstParam,
                         std::vector<CPoseCand_S> &rstScratch,
                         std::vector<int32_t> &rvecOutRow);
}
