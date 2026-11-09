/**
 * @FilePath     : phone_recheck.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 10:00:16
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 10:00:16
 * @Description  : 玩手机复核的跨帧缓存与轮转挑选

 */

#pragma once

#include <cstdint>
#include <vector>

#include "classroom_define.h"
#include "feature_frame.h"

namespace Classroom
{
/**
 * @brief 一次复核结论的跨帧缓存行（按跟踪 ID 存）
 * @note  三件事分开记，是因为它们各有各的读者：
 *        - llPhoneMs = 「什么时候看过」→ 挑选据此把人推后一轮（盖不盖决定谁会饿死）；
 *        - bValid    = 「那一次真的看到图了吗」→ 只有 true 才允许回填进特征行；
 *        - bFound    = 「看到了什么」→ bValid 为 false 时无意义。
 *        少 bValid 这一栏会把「推理失败」和「看了没有」压成同一个值，而这两件事在判定上
 *        完全相反：前者是「不知道」，后者是否决一条真行为的唯一依据。姿态表用
 *        stSkl.bValid 表达同一件事，这里是它的 24 字节版本
 */
typedef struct _PhoneCacheEntry_
{
    int32_t nTrackId = -1;      /* <0 = 空槽 */
    int64_t llPhoneMs = 0;      /* 该次复核产出时刻（单调毫秒） */
    bool    bValid = false;     /* 那次复核真拿到了可判的图（推理成功且映射建得起来） */
    bool    bFound = false;     /* bValid=true 时才有意义：人体框内看到了手机目标 */
    float   fScore = 0.0f;      /* 复核框最高置信度，bFound=false 时为 0 */
} CPhoneCacheEntry_S;

/**
 * @brief   : 按跟踪 ID 找缓存行
 * @return   {const CPhoneCacheEntry_S*} 没有返回 nullptr
 */
const CPhoneCacheEntry_S *phoneCacheFind(const std::vector<CPhoneCacheEntry_S> &vecCache, int32_t nTrackId);

/**
 * @brief   : 写入/覆盖一个 ID 的缓存行
 * @param    {bool} bValid：那一次复核真拿到了可判的图。false 也要入表——llPhoneMs 是
 *            「什么时候试过」，挑选要靠它把人推后；但 bFound/fScore 随之归零，
 *            因为「没试成」在 applyPhoneCache 里必须与「查不到」同处理
 * @param    {int32_t} nCapInUse：当前已用槽数，OUT=写入后的值（新 ID 才 +1）
 * @return   {bool} false 表示表满（人数上限之外不该有第六十一个 ID，满了只可能是 ID 洪泛）
 * @note     : 与 poseCacheUpsert 同一策略——空槽优先复用最靠前的，不做 LRU：
 *            表容量与人数上限同值，装不下本身就是异常，要靠读数去看而不是悄悄替换
 */
bool phoneCacheUpsert(std::vector<CPhoneCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                      int32_t nTrackId, bool bValid, bool bFound, float fScore, int64_t llPhoneMs);

/**
 * @brief   : 丢掉「已经旧到规则层不会再用」的缓存行
 * @param    {int32_t} nTtlMs：新鲜度窗口，与规则层同一个 phoneUsable 口径
 * @return   {int32_t} 被清掉的槽数
 * @note     : 与姿态表同一条清表依据：按产出时刻而不按「这个 ID 还在不在」。跟踪器给同一个
 *            人换新 ID 时旧行自然过期；留着刚掉线的人等 ID 复用是危险的——那会把上一位
 *            的"没有手机"扣到新坐下的人头上
 */
int32_t phoneCachePrune(std::vector<CPhoneCacheEntry_S> &vecCache, int32_t &rnCapInUse,
                        int64_t llNowMs, int32_t nTtlMs);

/**
 * @brief   : 把缓存里仍然新鲜的复核结论抄进特征行
 * @param    {int64_t} llTtlMs：与规则层同一个窗口，两处不一致会出现「感知层说结论还在、
 *            规则层说没结论」，而规则层那句会让这一列整拍不判——现象是复核在跑但事件断
 * @return   {int32_t} 拿到复核结论的行数
 * @note     : 两种行都不抄：查不到（从没为这人复核过）、以及查到但那一拍 bValid=false
 *            （复核跑了没拿到图）。把它们当「模型说没有」是一次语义升级，代价是全班
 *            在 106 上被否决干净而读数一切正常。没抄的行保持 acquire() 清零后的默认
 *            （llPhoneMs=0），规则层据此走「只有规则证据」那条分支，而不是判 FALSE
 */
int32_t applyPhoneCache(CPersonFeature_S *patRows, int32_t nRowNum,
                        const std::vector<CPhoneCacheEntry_S> &vecCache,
                        int64_t llNowMs, int64_t llTtlMs);

/* #87 复核 yes 分数直方图的分桶（桶沿 0.3/0.4/…/0.9，共 8 桶）：
   b0 = <0.3（含 0.25~0.30 这一段——它正是 #85 那把没生效的枪的射程，必须能看见），
   b7 = >=0.9。NaN/负数也进 b0：坏读数要可见，不能静默丢。
   #85 的教训：只看「首框 score=0.40」就下旋钮结论，而分布里根本没有那一格的证据 */
constexpr int32_t PHONE_YES_BUCKET_NUM = 8;
int32_t phoneYesScoreBucket(float fScore);

/**
 * @brief 复核挑选参数（取自配置快照，需求 11）
 */
typedef struct _PhoneSelectParam_
{
    int64_t llNowMs = 0;
    /* 同一个人两次复核的最小间隔。<=0 在 selectPhoneTargets 里按 1ms 兜底（等价于不限
       间隔，只保证同一毫秒内不重复复核）；配置层钳到 >=100ms（一次推理的量级），
       现场配不到 0，所以这条兜底只影响直接调纯函数的夹具 */
    int32_t nIntervalMs = 1000;
    /* 本拍最多复核几个人（一轮的成本上限：一次 RGA 裁剪 + 一次 NPU 等待） */
    int32_t nWantNum = 2;
    /**
     * @brief 人体框短边下限（分析帧像素），小于它的人**整行不参与复核**
     *
     *        这不是性能旋钮，是否决权的资格门。后排同学的体框只有十几像素，裁出来再放大到
     *        模型输入那么大，模型看到的是"一坨糊"而不是"手里没有东西"——它返回 0 个框，
     *        而这个 0 会被记成 bFound=false，也就是一条否决。于是整排后排同学的 106
     *        被一次根本没法判的复核悄悄杀掉，而读数上复核一切正常（try/no 都在涨）。
     *        与 bOccluded / bOnBoundary 那两条是同一条理由：图不足以支撑否证时，
     *        宁可不看（规则层走"只有规则证据"分支）也不要有"看了说没有"的结论
     *
     *        入参取检测层那把同名尺寸门（threshold.min_body_px）而不是新配一个键：
     *        "这么小的框我们不信它是个人"这件事全工程只有一个口径，两处配就是两处会漂移
     */
    int32_t nMinBodyEdge = CLASSROOM_MIN_BODY_PIXEL;
} CPhoneSelectParam_S;

/**
 * @brief 挑选过程的一次候选（纯函数内部的临时量，容量由调用方预留）
 * @note  与 CPoseCand_S 同形：只多一个「在不在名单上」的判断，排序键完全一样
 */
typedef struct _PhoneCand_
{
    int32_t nRow = -1;        /* 特征行下标 */
    int64_t llLastMs = 0;     /* 上次复核时刻，0 = 从没复核过 */
    float   fArea = 0.0f;     /* 人体框面积，用于打破同一次数里的先后 */
} CPhoneCand_S;

/**
 * @brief   : 一次挑选里「名单上却没进候选」的分项归因（OUT，可选）
 * @note    : 回答的是「list>0 而 want 恒 0」里除「模型没驻留」之外的两种成因——
 *            那一档在调用方连本函数都进不来（裁剪档不 ready 就整段退出），剩下来的
 *            间隔闸与体框图不合格在这里分开计。配额截断（进了候选没被选中）刻意不算
 *            skip：它是 want 与 list 的直接差，不需要第二个读数
 */
typedef struct _PhoneSelectStat_
{
    int32_t nSkipOccluded = 0;   /* 遮挡或贴边：裁出来的图不足以支撑一次否证 */
    int32_t nSkipBox = 0;        /* 没有体框，或短边小于 nMinBodyEdge：裁出来是糊的 */
    int32_t nSkipInterval = 0;   /* 距上次复核不足 nIntervalMs，本拍让给别人 */
} CPhoneSelectStat_S;

/**
 * @brief   : 从「规则层点名要复核的人」里挑出本拍真正做的几个
 * @param    {patRows / nRowNum}：本拍已写好的特征行（要 stBodyBox 与 nPersonId——
 *            该字段是**跟踪 ID**（classroom_define.h），不是跨帧身份；轨道号回收复用后
 *            同一个号可能已是别人，所以缓存生命周期必须由 ttl 兜住）
 * @param    {vecWantIds}：规则层给的候选跟踪 ID 表（本拍判定「低头成立」的那些人）
 * @param    {vecCache}：当前缓存表，用来取每人上次复核时刻
 * @param    {rstParam}：时间闸、本拍配额与小框下限（后者决定是否决权，见字段注释）
 * @param    {rstScratch}：临时表，调用方预留到人数上限（本函数只用不扩）
 * @param    {rvecOutRow}：OUT=行下标，长度 ≤ nWantNum，函数内先 clear
 * @param    {pstStat}：OUT=被各道资格门挡掉的名单人次；传 nullptr 表示调用方不看。
 *            每次调用先清零（不是累加），跨拍累计由调用方自己加
 * @return   {size_t} 挑中的行数
 * @note     : 排序键与 selectPoseTargets 同构（最久没复核优先，其次人体框面积大的优先）。
 *            为什么不在这里自己判「谁低头了」：那是 §4.6 的口径——**引擎不认识判据**，
 *            低头的定义只在 student_behavior 那一处。感知层要做的只是"给名单上的人裁一张图"，
 *            名单为空就一段都不跑（这是它比姿态段更便宜的原因，也是它不会在全班低头前
 *            白占 NPU 的原因）
 */
size_t selectPhoneTargets(const CPersonFeature_S *patRows, int32_t nRowNum,
                          const std::vector<int32_t> &vecWantIds,
                          const std::vector<CPhoneCacheEntry_S> &vecCache,
                          const CPhoneSelectParam_S &rstParam,
                          std::vector<CPhoneCand_S> &rstScratch,
                          std::vector<int32_t> &rvecOutRow,
                          CPhoneSelectStat_S *pstStat = nullptr);
}
