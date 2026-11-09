/**
 * @FilePath     : perceive_pipeline.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 09:20:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 09:20:00
 * @Description  : 感知管线
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "classroom_config.h"
#include "classroom_define.h"
#include "crop_unit.h"
#include "detect_decode.h"
#include "expression_estimator.h"
#include "face_align.h"
#include "face_feature.h"
#include "feature_frame.h"
#include "frame_bus.h"
#include "infer_scheduler.h"
#include "model_pool.h"
#include "person_tracker.h"
#include "phone_recheck.h"
#include "pose_admit.h"
#include "pose_decode.h"
#include "rga_pool.h"

namespace Classroom
{
/* 任务过期线 = 等待上限 × 该倍数。为什么必须比等待长：调用方不等了之后，
   那个任务仍要跑完才能释放等待槽；若它的 deadline 先到，worker 会立刻丢掉它并回调，
   于是"慢一拍"会变成"这一路每拍都白提交一次"，永远追不回来 */
#define CLASSROOM_INFER_DEADLINE_TIMES (2)

/* 在途看门狗倍数：超过 等待上限 × 该值 仍没有回调，判调度器丢了回调，强制放行并打 ERROR */
#define CLASSROOM_INFER_WEDGE_TIMES (10)

/**
 * @brief   : 感知管线需要的宿主能力（只有两项，由 CClassroomEngine 实现）
 * @note    : 为什么用抽象接口而不是直接持有 CClassroomEngine&：感知层不该知道节拍、
 *            守护、时间片轮转那些事，而主机单测需要一个不启线程、不碰设备的假宿主。
 *            生命周期由外部保证：宿主先于管线存在、后于管线销毁（引擎 deinit 已停节拍线程）
 */
class IPerceiveHost
{
public:
    IPerceiveHost() = default;
    virtual ~IPerceiveHost() = default;

    IPerceiveHost(const IPerceiveHost &) = delete;
    IPerceiveHost &operator=(const IPerceiveHost &) = delete;

    /* 该模型的输入生成档（随组驻留有、随组卸载无），取不到就是本拍跑不了 */
    virtual CropUnitPtr cropUnitOf(const std::string &strKey) const = 0;
    virtual CInferScheduler *scheduler() = 0;
    /* 只为「不加载、不计租约地下发门限」而给（见 CModelPool::peek 的注释） */
    virtual CModelPool &pool() = 0;
    /**
     * @brief   : 人脸底库，取特征之后那一步「512 维 → 身份」用
     * @return   {IFaceGallery*} 非拥有指针； nullptr 表示宿主没装配底库
     * @note    : 所有权留在引擎而不是管线，因为「底库从哪来」是配置层的事实
     *            （gallery.source 换 file/sqlite 时要重建实现方，而重建与配置换版同批发生，
     *            那正是 applySnapshot 的活儿；管线只在节拍里用它）。
     *            返回空与 available()==false 都是**正常态**：模型没交付、平台还没下发名单、
     *            考勤整段关着，这三种都该继续跑特征并把读数记在 u64FeatNoGallery 上，
     *            而不是让整条链停摆——名单到位的那一刻它自己就通了
     */
    virtual IFaceGallery *faceGallery() = 0;

    /**
     * @brief   : 这个人头是否落在某组区域里（「人脸检测区域」）
     * @param    {const CRectf_S&} stHeadBox：分析帧坐标的人头框（与规则层同一套坐标）
     * @param    {const std::vector<int32_t>&} vecRegionIds：区域编号；**空 = 不限制**
     * @return   {bool} true=在区域内（或没配区域）；配置坏了（引用的几何不存在）也给 true
     * @note    : 为什么由宿主回答而不是感知层自己算：区域几何的换算（webToSrc → srcToDst，
     *            含镜像/旋转/缩放）只有规则层那一处 CRegionTable 会，感知层再算一遍就是
     *            第二把尺——两处一旦分叉，表现是"网页画的框明明盖住人，程序说在区域外"，
     *            而这类 bug 的排查成本是整个坐标系再读一遍（§4.8）。
     *            配置坏了给 true 而不是 false：这是"该做而判不了"，不是"不在区域内"，
     *            按 false 处理会让一张配错的区域静默吃掉整条人脸链
     */
    virtual bool headInRegion(const CRectf_S &stHeadBox,
                              const std::vector<int32_t> &vecRegionIds) = 0;

    /**
     * @brief   : 玩手机复核的候选名单：上一拍判定「低头成立」的那些人的跟踪 ID
     * @return   {const std::vector<int32_t>&} 只读，引用须在本拍内有效
     * @note    : **为什么名单来自规则层而不是感知层自己判低头**：「低头」的定义只有一处
     *            （student_behavior 里那条颈/鼻比值），感知层重算一遍就是第二套口径，
     *            两边一旦漂移会出现「感知层裁了图而判定层不认这个人低头」——白烧 NPU 且不报错。
     *            §4.6 那条「引擎不认识判据」的纪律在这里同样成立，方向反过来也一样。
     *            **为什么必然晚一拍**：tickOnce 里感知排在规则之前，本拍的名单只能是上一拍的
     *            判定结果。低头是持续秒级的姿态（106 的时长门要求连续 3s），一拍 200ms 的
     *            相位差不影响任何一条判定；反过来为了「同拍」而在感知层自己判，代价是判据两份。
     *            空表是**正常态**而不是错误：没人低头、106/104 关着、规则层还没跑过第一拍，
     *            这三种都该整段跳过复核（一次推理都不提交）
     */
    virtual const std::vector<int32_t> &phoneRecheckIds() const = 0;

    /**
     * @brief   : NPU 等待期的紧急度预计算（流水线优化6 的挂点，optimization.urgency_precompute）
     * @note    : 默认空实现 = 零开销（主机假宿主什么都不做；开关关着时管线根本不调）。
     *            引擎侧的落点见 CClassroomEngine::precomputeUrgency 的注释：spec 设想的
     *            mergePoseUrgency(spFrame, llNowMs) 在现树不存在且早算的值必然被 ruleStep
     *            的清表重建覆盖，外提合并器 tick 又会打碎 P0-2 的吞 BEGIN 语义——
     *            引擎侧因此落为**空体挂点**，正式计算留在 ruleStep。调用保持不变：
     *            给将来真有可外提计算的那天留这个插槽，开关打开的实效 = 多一次虚调用
     */
    virtual void precomputeUrgency(int64_t llNowMs) { (void)llNowMs; }
};

/**
 * @brief 感知管线观测计数（心跳与 statusJson 出口，只在节拍线程累加，读侧拷贝即可）
 * @note  这一组数的用法是「排除法」：feat_pub 少 → 看 head_rounds 有没有涨；
 *        head_rounds 不涨 → 看 inflight/timeout（NPU 慢）还是 unready（模型没驻留）；
 *        head_rounds 涨但行数 0 → 看解码计数（detect_decode 的三道闸哪一道在吃框）
 */
typedef struct _PerceiveStat_
{
    uint64_t u64HeadRounds = 0;      /* 真正提交过人头检测的拍数 */
    uint64_t u64HeadOk = 0;          /* 人头结果可用（含检出 0 人）的拍数 */
    uint64_t u64CropFail = 0;        /* RGA 借不到缓冲 / 整帧档未就绪 */
    uint64_t u64SubmitFail = 0;      /* 调度器拒收（未运行、参数非法、队列满且优先级不够） */
    uint64_t u64InferTimeout = 0;    /* 等到上限仍无结果 */
    uint64_t u64InferInflight = 0;   /* 上一拍还在途而跳过本拍：NPU 供不上的直接证据 */
    uint64_t u64InferUnready = 0;    /* 回调报 IR_MODEL_UNREADY（未加载 / 被时间片换下） */
    uint64_t u64InferStale = 0;      /* 回调报 IR_STALE_DROPPED：deadline 定得太紧 */
    uint64_t u64InferBackendFail = 0;/* 后端 run() 返回 false（张量尺寸不符最常见） */
    uint64_t u64WedgeCleared = 0;    /* 看门狗强制放行次数，>0 就必须查调度器 */
    uint64_t u64Rows = 0;            /* 写进特征帧的行数累计（除以 ok 拍数即平均人数） */
    uint64_t u64OsdPush = 0;
    uint64_t u64MapFail = 0;         /* CDetMap 建不起来：几何与模型尺寸对不上 */

    int32_t  nLastHeadDet = 0;       /* 本拍人头检出数（解码后） */
    int32_t  nLastTrackNum = 0;      /* 本拍输出轨迹数 */
    int64_t  llLastRoundMs = 0;      /* 上一拍总耗时，看护「一拍超过节拍」 */

    /* ---- 姿态路（P2）。单独一段计数是因为它的失效形态与检测路完全不同：
           检测路坏在「不出框」，姿态路坏在「出框了但没人有骨骼」，
           而后者在头/体两张表里一切正常，只有这几列能说明原因 ---- */
    uint64_t u64PoseTried = 0;       /* 真的提交过姿态推理的人次（含失败，配额就是按它扣的） */
    uint64_t u64PoseUsable = 0;      /* 解出可用骨骼的人次 */
    uint64_t u64PoseBusySkip = 0;    /* 因 NPU 忙而整段让路的拍数 */
    uint64_t u64PoseCacheFull = 0;   /* 缓存表装不下：只可能是 ID 洪泛，查跟踪器 */
    int32_t  nLastPoseRows = 0;      /* 本拍特征行里带可用骨骼的行数，规则层「有证据」的上限 */
    int32_t  nLastPoseWant = 0;      /* 本拍按预算算出的允许人数 */
    int32_t  nLastPoseLeft = 0;      /* 本周期配额还剩几个（0 且人数>0 就是配额在吃紧） */
    /* 姿态产能撑不起这一屋子人的拍数（#69-A）：一整圈乘余量被窗口上限钳住时才涨。
       这条计数是"后排零事件"唯一的机上出口——没有它，漏检与"后排就是没人举手"分不清 */
    uint64_t u64PoseCapacityShort = 0;

    /* ---- 人脸路（P5）。这一段的存在理由与姿态段一样：它的失效形态与检测段不同 ----
           检测段坏在「不出框」，人脸段坏在「框出了但配不到人身上」，
           后者在人头那张表里一切正常，只有这几列能说明为什么考勤一直是 0 */
    uint64_t u64FaceRounds = 0;      /* 真的提交过人脸检测的拍数 */
    uint64_t u64FaceOk = 0;          /* 拿到可用人脸输出的拍数（含检出 0 张） */
    int32_t  nLastFaceDet = 0;       /* 本拍解码后的人脸数 */
    int32_t  nLastFaceRow = 0;       /* 本拍写上人脸列的行数。恒小于上一行就是配不上人 */
    uint64_t u64FaceUnmatched = 0;   /* 一张脸都没配到的累计（远焦/俯仰角大的机位第一现场） */

    /* ---- 人脸特征段（P5d）。这一段单独一组计数，是因为「考勤名单一直空」的成因不止一条，
           而它们在上一段里长得一模一样：裁不出正方形（框太小）、点序表越界、五点解不出对齐、
           warp 借不到块或重采样失败、NPU 那一路排不上、底库没有名单。
           少了这几档，现场唯一的做法就是加日志重编一次 ---- */
    uint64_t u64FeatTried = 0;       /* 真走过「裁剪→对齐→推理」的人次（含中途失败） */
    uint64_t u64FeatOk = 0;          /* 特征写进表的次数。与上一行之差就是链路吃掉的比例 */
    /* 备料失败按**失败点**分五档，不按原因合并（原先一档 noalign 盖住五条路，
       于是「63 次没喂进推理」这句话指不出该找谁：框太小归机位、点序表越界归运维、
       五点解不出归算法组、借不到块与重采样失败归我们）。五条各自只在自己那个出口加一，
       所以 tried − ok − 这五条 − busy = 走到推理那一步的账，平不平整是可核对的 */
    uint64_t u64FeatNoCrop = 0;      /* 外扩后裁不出合法矩形（框太小或贴边） */
    uint64_t u64FeatNoPerm = 0;      /* 配置里的点序表用不了（越界）：改配置即可复现 */
    uint64_t u64FeatNoAlign = 0;     /* 五点解不出变换。原因在这条的 warn 里（resid/低分/间距…） */
    uint64_t u64FeatNoPool = 0;      /* warp 缓冲借不到块：池太小或被在途的推理占满 */
    uint64_t u64FeatNoWarp = 0;      /* 重采样本身失败（RGA/CPU 缩放返回失败） */
    uint64_t u64FeatBusySkip = 0;    /* 因 FaceFeature 那一路在途而让路的次数（整段或本张） */
    uint64_t u64FeatNoGallery = 0;   /* 有特征但底库不可用：名单为空/未交付/未装配 */
    uint64_t u64FeatHit = 0;         /* 比对出身份的结论落表次数（bHit=true，门限在账本侧） */
    uint64_t u64FeatRegionSkip = 0;  /* 人头落在「人脸检测区域」外而被跳过的人次。
                                        与 nocrop/noalign 分开：那几档是"这张脸做不了"，
                                        这一档是"按配置不做"，现场调大区域后它自己会掉 */
    int32_t  nLastFeatNum = 0;       /* 本拍做了几个人的特征。恒 0 且 nLastFaceRow>0 就是链路断了 */
    int64_t  llLastFeatCostMs = 0;   /* 本拍特征段耗时（含同步等推理），看护「一段吃掉一节拍」 */
    /* 预热全速（#151）跑了多少拍：只在 feat_per_beat_enable=true 时才可能增长。
       查"考勤出名字慢"时先看它——非零且持续增长 = 还在铺开（正常）；恒 0 且 pend 高 = 节流档下
       全班已各两轮，名字该出了，该查的是比对侧而不是取特征 */
    uint64_t u64FeatWarmBeats = 0;

    /* ---- 表情段（P5e）。这一段单独一组计数，理由与前几段同构而更极端：
           它连自己的节拍都没有——吃的是取特征段为这个人**已经算好的那张对齐图**，
           于是「情绪曲线一直是空的」至少有四条成因（上游考勤关着、这颗模型没驻留、
           输入尺寸与 warp 档对不上、平滑还没攒够帧），而它们在人脸那几列里全部正常。
           判定级的分解（概率不足/差值不足/长度不符…）在 estimator 自己那行里，
           这里只记管线这一层的四条 ---- */
    uint64_t u64ExprTried = 0;       /* 真提交过表情推理的人次（含失败，与 feat 的 tried 同口径） */
    uint64_t u64ExprFed = 0;         /* 拿到输出并喂进平滑器的人次。与上一行之差=推理那一路吃掉的 */
    uint64_t u64ExprBusySkip = 0;    /* 那一路在途而跳过的人次。**只跳本张不断整段**：
                                        表情失败不该连带拖累考勤取特征，二者是同一张图上前后两问 */
    uint64_t u64ExprSizeBad = 0;     /* 开着却做不了（这颗没登记 / 尺寸与 warp 档不符）而被跳过的人次。
                                        与 u64ExprTried 同量纲，两数一减就是"本该做而一次没做"；
                                        这一档不数出来，现象就是「驻留正常、考勤正常而 expr 恒 0」 */
    uint64_t u64ExprAngleSkip = 0;   /* 人脸偏转角超 max_angle 而被跳过的人次（Web 第 11 页那一格）。
                                        与 busy/szbad **不是同一类**：那是"这一路坏了"，这是
                                        "这一路正常，按现场配置把人滤掉了"。混在一起读会让
                                        "调小角度以后事件变少"看起来像故障；单独一列才能
                                        把"侧脸多"和"模型没跑"分开 */
    int32_t  nLastExprAngleBad = 0;  /* 本拍因角度量不出来（五点退化/NaN）而**放行**的人次。
                                        放行而不是拒绝：缺观测不否决，与全工程的"没证据不算否定"一致；
                                        这一列持续 >0 说明这批脸的瞳距太小，角度门处于失效状态 */
    int32_t  nLastExprRow = 0;       /* 本拍写进特征行的情绪列数（曲线给得出值的那些人） */

    /* ---- 玩手机复核段（P6b）。这一段单独一组计数的理由与前两段同构：它的失效形态是
           「复核一条都没做」，而人头/姿态/人脸三段的读数一切正常——少了这几列，
           现场看到的只是「106 一直不报」，分不清是没人低头、模型没驻留、还是名单被饿死 ---- */
    uint64_t u64PhoneTried = 0;      /* 真提交过复核推理的人次（含失败，配额按它扣） */
    uint64_t u64PhoneFound = 0;      /* 看到了手机框的人次（确认） */
    uint64_t u64PhoneNo = 0;         /* 看了、没看到的人次（否决）。与上一行必须成对看：
                                        只有 Found 没有 No，说明否决那半条链没通——而 106
                                        的误报恰恰全靠这半条压 */
    uint64_t u64PhoneCacheFull = 0;  /* 复核表装不下：只可能是 ID 洪泛，查跟踪器 */
    int32_t  nLastPhoneList = 0;     /* 规则层给的名单长度（本拍取到的原值，未经任何闸）。
                                        这三列要一起读才分得开那三种「一次都没复核」：
                                        list=0 是没人低头；list 恒 0 而 106 开着是复核被现场关掉
                                        （关在规则层那一步，名单压根不填）；list>0 而 want 恒 0
                                        再靠下面 skip= 三项分因：三项都不涨 = 模型没驻留或
                                        进入挑选前就退出，occl/box 涨 = 资格门在挡人，
                                        iv 涨 = 都刚复核过、只是间隔还没到 */
    int32_t  nLastPhoneWant = 0;     /* 本拍按名单与配额决定要复核几个人。名单恒长而本列恒 0 = 间隔或配额在吃紧 */
    int32_t  nLastPhoneRows = 0;     /* 本拍特征行里带复核结论的行数（含「没有」那些） */
    /* 名单上的人被哪道资格门挡掉（selectPhoneTargets 的分项归因）。这三列是
       「list>0 而 want 恒 0」剩下那两种成因的唯一读数：遮挡/贴边与体框过小的人
       永远进不了候选、也不会推进自己的 llLastMs，从 try/yes/no 上看与「没人低头」
       一模一样——而前者可以调 min_body_px 或换机位，后者什么都不用动 */
    uint64_t u64PhoneSkipOccl = 0;  /* 遮挡或贴边挡掉的名单人次 */
    uint64_t u64PhoneSkipBox = 0;   /* 体框缺失或短边过小挡掉的名单人次 */
    uint64_t u64PhoneSkipIv = 0;    /* 距上次复核不足间隔挡掉的名单人次 */
    /* #87 yes 分数直方图（8 桶，分桶与桶沿见 phone_recheck.h 的 phoneYesScoreBucket）：
       治理 106 误报先看分布再选方案——0.30~0.45 这一段有多少、0.45 以上有多少，
       决定「提门槛/双确认/接受」三选一，不许再拿首框单点分数当分布用 */
    uint64_t au64PhoneYesBucket[8] = {0};
} CPerceiveStat_S;

/**
 * @brief   : P1 感知管线
 * @note    : 非线程安全（设计为节拍线程独占）。构造后每拍调 run()，配置换版时内部自同步
 */
class CPerceivePipeline
{
public:
    /**
     * @param {IPerceiveHost*} pHost：非拥有指针，须比本对象活得久
     */
    explicit CPerceivePipeline(IPerceiveHost *pHost);

    CPerceivePipeline(const CPerceivePipeline &) = delete;
    CPerceivePipeline &operator=(const CPerceivePipeline &) = delete;

    /**
     * @brief   : 跑一拍感知
     * @param    {const FramePtr&} spPixel：本拍像素帧（引擎已保证是新帧）
     * @param    {const ConfigPtr&} spConf：与本次分析同批的配置快照
     * @param    {const FeatureFramePtr&} spOut：引擎借出的可写特征帧（头字段已由引擎填好）
     * @return   {bool} false=本拍无产出，引擎不发布
     * @note    : **只有人头检测真的跑出来了才返回 true**。没有人头结果时发一张空帧，
     *            会被下游读成「这屏 0 人」并计入所有指标的分母 —— 那是需求 10 明令的误判，
     *            比这一拍没有数据危险得多（引擎据此计 perceive_fail，日志与 statusJson 都有真相）
     */
    bool run(const FramePtr &spPixel, const ConfigPtr &spConf, const FeatureFramePtr &spOut);

    CPerceiveStat_S stat() const { return m_stStat; }
    CDetDecodeStat_S decodeStat() const { return m_stDecodeStat; }
    CDetDecodeStat_S decodePhoneStat() const { return m_stPhoneDetStat; }
    CPoseDecodeStat_S poseDecodeStat() const { return m_stPoseStat; }
    CFaceDecodeStat_S faceStat() const { return m_stFaceStat; }
    /* 五点能不能解出对齐：人脸路真正的「这套点可不可用」读数（按失败原因分项） */
    CFaceAlignStat_S faceAlignStat() const { return m_stFaceAlignStat; }
    CBodyAssembleStat_S assembleStat() const { return m_stAssembleStat; }
    CTrackerStat_S trackStat() const { return m_stTrk.stat(); }

    /**
     * @brief   : 取走某个跟踪 ID 手上那份待用的人脸比对结论
     * @param    {int32_t} nTrackId：跟踪 ID，<0 直接返回 false
     * @param    {CFaceMatch_S&} rstOut：OUT=仅 true 时有意义（含 bHit=false 的"比过了，没上"）
     * @return   {bool} false=这个 ID 手上没有待取的结论
     * @note    : **特征表为什么在管线而取的人在场**：这张表按跟踪 ID 存，而 ID 是感知层的产物；
     *            放进特征帧就要给每行加两个 std::string（4 槽总线上多两处堆分配 churn，
     *            且 OSD/规则/数据中心任何一个消费者都读得到学生姓名——个人信息不该那样扩散）。
     *            一次性的 take 同时解决了"同一份证据被记两票"：账本命名要两次命中，
     *            而这两票必须来自两份不同的特征（见 face_feature.h 文件头第 2 条）。
     *            调用点是引擎的 attendStep，与本表的写侧同一条节拍线程，故不加锁
     */
    bool takeFaceMatch(int32_t nTrackId, CFaceMatch_S &rstOut);

#ifdef CLASSROOM_PROBE_FACE_DUMP
    /**
     * @brief   : 【仅探针】把某个跟踪 ID 当前那份人脸特征拷出来（产品构建不定义此宏，二进制零变化）
     * @return   {int32_t} 维度；<0 = 该 ID 没有可用特征
     * @note    : 只读快照，不消费、不置标记。用途是让回放验证件能导出特征来建测试底库，
     *            从而在设备上验 P5（≥90% 认对 / ≤1% 误识）。
     */
    int32_t probeFaceFeature(int32_t nTrackId, float *pafOut, int32_t nCap) const
    {
        if (nullptr == pafOut || nCap <= 0 || nTrackId < 0)
        {
            return -1;
        }
        const int32_t nIdx = faceFeatIndexOf(m_vecFeat, nTrackId);
        if (nIdx < 0)
        {
            return -1;
        }
        const CFaceFeatEntry_S &stEntry = m_vecFeat[static_cast<size_t>(nIdx)];
        if (stEntry.nDim <= 0 || stEntry.nDim > nCap)
        {
            return -1;
        }
        for (int32_t i = 0; i < stEntry.nDim; ++i)
        {
            pafOut[i] = stEntry.afFeat[i];
        }
        return stEntry.nDim;
    }

    /**
     * @brief   : 【仅探针】列出现有特征行的跟踪 ID 与维度
     * @return   {int32_t} 写出的行数
     */
    int32_t probeFaceTracks(int32_t *panId, int32_t *panDim, int32_t nCap) const
    {
        if (nullptr == panId || nullptr == panDim || nCap <= 0)
        {
            return 0;
        }
        int32_t nNum = 0;
        for (size_t i = 0; i < m_vecFeat.size() && nNum < nCap; ++i)
        {
            const CFaceFeatEntry_S &stEntry = m_vecFeat[i];
            if (stEntry.nTrackId >= 0 && stEntry.nDim > 0)
            {
                panId[nNum] = stEntry.nTrackId;
                panDim[nNum] = stEntry.nDim;
                ++nNum;
            }
        }
        return nNum;
    }
#endif

    /**
     * @brief   : 观测读数的一行文本（挂进引擎心跳与 statusJson，P1 的验收读数出口）
     * @return   {std::string} key=value 串，可直接 grep
     * @note    : **只允许在节拍线程调用**。本类的计数器无锁（写侧就是节拍线程），
     *            在 32 位机上跨线程读 uint64_t 会撕裂成半新半旧的值——诊断数字读出个天文数字，
     *            代价是让人顺着这条假线索去查一个不存在的 bug。引擎的 metrics 正好跑在节拍线程上，
     *            由它取串并缓存进自己的状态锁内对外给（见 CClassroomEngine::setPerceiveStatFn）
     */
    std::string statText() const;

private:
    /* 一次提交到一次取回的等待槽。按模型键各一张：同时至多一个在途，故不需要环形队列 */
    typedef struct _WaitSlot_
    {
        std::mutex mtx;
        std::condition_variable cv;
        bool bPending = false;        /* 已提交、回调未回 */
        bool bDone = false;           /* 回调已把结果放进 vecRaw */
        uint32_t u32PendingSeq = 0;   /* 在途那次提交的序号，0 表示「没有在途」 */
        uint32_t u32DoneSeq = 0;      /* vecRaw 属于哪次提交，等待方按它判「是不是我这拍的结果」 */
        int64_t llPendingSinceMs = 0;
        bool bOk = false;
        InferResultReason_E enReason = InferResultReason::IR_BACKEND_FAIL;
        int64_t llCostMs = 0;
        std::vector<float> vecRaw;   /* 打平框，容量回调后保留 */
    } SWaitSlot_S;

    typedef std::shared_ptr<SWaitSlot_S> WaitSlotPtr;

    /* 配置换版时同步门限与跟踪参数（比对 version，不做逐字段 diff） */
    void syncConfig(const ConfigPtr &spConf);
    WaitSlotPtr slotOf(const std::string &strKey);

    /**
     * @brief   : 整帧 → 一次检测推理 → 解码成分析帧框表（同步等，带上限）
     * @param    {const std::string&} strKey：模型键
     * @param    {const CFrame_S&} stFrame：像素帧
     * @param    {int32_t} nPriority：调度优先级（0 最高）
     * @param    {DetectSource_E} enSource：这批框的来源标记
     * @param    {const CDetectParam_S&} rstParam：门限与容量
     * @param    {CDetDecodeStat_S&} rstDecodeStat：本路的解码计数（人头/复核各一张，见下）
     * @param    {std::vector<CDetection_S>&} rvecOut：OUT=框表（调用方须已定容）
     * @return   {bool} false 表示本路本拍没有可用结果，调用方按「证据不足」处理
     */
    bool inferWhole(const std::string &strKey, const CFrame_S &stFrame, int32_t nPriority,
                    DetectSource_E enSource, const CDetectParam_S &rstParam,
                    CDetDecodeStat_S &rstDecodeStat, std::vector<CDetection_S> &rvecOut);

    /**
     * @brief   : 一次推理的共用内核：借输入缓冲 → 提交 → 同步等 → 建坐标映射
     * @param    {const CRectf_S*} pstRectSrc：nullptr=整帧档（人头/人脸检测）；
     *            非空=区域档，按这个源帧矩形裁剪（姿态按人体框裁剪放大，§4.5）
     * @param    {CDetMap&} rstMap：OUT=「本次真正喂进模型的那块区域 → 分析帧」的映射
     * @param    {const std::vector<float>**} ppvecRaw：OUT=该路等待槽里的打平输出，
     *            只在本函数返回 true 到调用方结束之间有效（期间不会再有第二次提交）
     * @param    {CRectf_S&} rstUsedSrc：OUT=钳制取偶后真正用到的源帧矩形（整帧档即全帧）
     * @return   {bool} false=这一路本拍没有可用结果
     * @note     : 本函数只做装配：residency 检查 → beginRound → RGA 备输入 → submitAndWait → 建映射。
     *            提交与等待那半在 submitAndWait 里，人脸特征段用的就是那半（它的输入不是 RGA 给的）。
     *            拆分后本体 = inferSubmitOne + inferWaitOne 的串调（协议与计数仍只有一份实现）
     */
    bool inferOne(const std::string &strKey, const CFrame_S &stFrame, int32_t nPriority,
                  const CRectf_S *pstRectSrc, CDetMap &rstMap,
                  const std::vector<float> **ppvecRaw, CRectf_S &rstUsedSrc);

    /**
     * @brief   : 交错推理的「一路在飞」凭据。只在同一拍、同一节拍线程里经手：
     *            inferSubmitOne 成功时填好，inferWaitOne 消费，拍尾即弃（无跨拍语义）
     */
    typedef struct _PendingTok_
    {
        WaitSlotPtr spSlot;     /* 占住的等待槽（成对纪律与 submitAsync/waitAsyncResult 相同） */
        uint32_t u32Gen = 0;    /* 本次提交的序号 */
        CRectf_S stUsedSrc;     /* 钳制取偶后真正喂进去的源帧矩形（整帧档即全帧） */
        CropUnitPtr spCrop;     /* **裁剪那一拍的档**。等待侧建映射必须用它，不能重取——
                                   提交与等待之间档可能被换出重建，重取会让映射按新档建、
                                   而输出还是旧档喂进去的，错位无声 */
    } SPendingTok_S;

    /**
     * @brief   : inferOne 的备料+提交段：residency 检查 → beginRound → RGA 备输入 → submitAsync
     * @param    {SPendingTok_S&} rstTok：OUT=成功时携带槽/序号/usedRect/裁剪档；
     *            失败时内容无意义且**槽已放回**（各失败分档的计数与日志在本函数内完成，
     *            与 inferOne 拆分前完全同位）
     * @return   {bool} false=本拍这一路交不出输入（与 inferOne 的 false 同义），
     *            此时**不要再调 inferWaitOne**
     * @note     : 与 inferWaitOne 成对——警告见 submitAndWait 的 @warning。
     *            优化2/3/5 的交错靠这一对：把「备 B 的料」压进「等 A」的窗口。
     *            提交失败不清 rstTok 里已无意义的槽——本函数已在各失败支放回并置空
     */
    bool inferSubmitOne(const std::string &strKey, const CFrame_S &stFrame, int32_t nPriority,
                        const CRectf_S *pstRectSrc, SPendingTok_S &rstTok);

    /**
     * @brief   : inferOne 的等待+映射段：waitAsyncResult → 用凭据里的裁剪档建坐标映射
     * @param    {stFrame}：只是给映射取源帧几何（帧活到拍尾，凭据不延长它的寿命）
     * @return   {bool} false=本拍这一路没有可用结果（等待失败按 IR_* 分档、映射失败计
     *            u64MapFail，计数与日志与 inferOne 拆分前完全同位）
     * @note     : 成功时 *ppvecRaw 有效期到本次取用结束为止（期间不得对该键再次提交）
     */
    bool inferWaitOne(const std::string &strKey, const CFrame_S &stFrame,
                      const SPendingTok_S &rstTok, const std::vector<float> **ppvecRaw,
                      CDetMap &rstMap);

    /**
     * @brief   : 背压协议的前半：占住这一路的等待槽（含在途看门狗放行）
     * @param    {WaitSlotPtr&} rspSlot：OUT=占住的槽（调用方要把它交给 submitAndWait 或 abortRound）
     * @param    {uint32_t&} ru32Gen：OUT=本次提交的序号
     * @return   {bool} false=本拍这一路放弃，且**没有**占住槽（调度器没起来 / 还在途且未过看门狗线）
     * @note     : 三段拆开的唯一理由是「备输入要花多少钱」：整帧/区域档的输入是一次 RGA 搬运，
     *            白做一次可接受；人脸特征段的输入是一次 CPU 双线性重采样加一次 NPU 等待，
     *            NPU 排不上时合在一个函数里就只能先做再被拒。协议本身仍然只有一份实现
     */
    bool beginRound(const std::string &strKey, WaitSlotPtr &rspSlot, uint32_t &ru32Gen);

    /* 占住槽之后、提交之前失败了：把在途标志放回原样。不调这里会把这一路锁死一整拍 */
    void abortRound(const WaitSlotPtr &rspSlot);

    /**
     * @brief   : 背压协议的后半（同步等一整次）。拆分后保留为兼容壳 =
     *            submitAsync + waitAsyncResult，行为与拆分前一致；
     *            optimization.submit_async_enable 打开后新路径一律走下面那对
     * @param    {u32Gen}：beginRound 给出的序号
     * @param    {nPriority}：调度优先级（0 最高）。留在这里而不是记在槽上：
     *            同一个模型键可以被不同优先级的调用方用（现在没有，但按键记就等于将来要猜）
     * @param    {rspBuf}：输入缓冲，**由调用方备好且尺寸与模型输入一致**。类型就是调度器
     *            请求里那个 RgaBufPtr，所以 RGA 直出的和 CPU 造的都走这一个入口
     * @return   {bool} false=本拍这一路没有结果，原因已在 m_stStat 分档
     * @warning  拆出来的两个函数**必须成对调用**，顺序只能 submit→wait：
     *            submit 了不 wait 会永久占住 WaitSlot（bPending 永远 true，
     *            下一拍 beginRound 会被挡住）；wait 了没 submit 会拿到上一拍的残留结果
     */
    bool submitAndWait(const std::string &strKey, const WaitSlotPtr &rspSlot, uint32_t u32Gen,
                       int32_t nPriority, const RgaBufPtr &rspBuf,
                       const std::vector<float> **ppvecRaw);

    /**
     * @brief   : 背压协议后半之前半：下发门限 → 构请求 → 提交（不等结果）
     * @param    {rspSlot/u32Gen}：beginRound 占住的槽与序号，原样转交
     * @return   {bool} false=提交失败（空缓冲/调度器没起/拒收），槽已被 abortRound 放回，
     *            此时**不要再调 waitAsyncResult**
     * @note     : 与 waitAsyncResult 成对（警告见 submitAndWait）。单拆出来是为了在一次
     *            NPU 等待窗口里塞进别的 CPU 活（优化2/5 的交错提交）：
     *            提交 A → 备 B 的料 → 等 A，A 的等待就不再是纯空转。
     *            提交失败的分档计数（crop/submit）与本体的日志闸门都在这半边，
     *            与拆分前完全同位
     */
    bool submitAsync(const std::string &strKey, const WaitSlotPtr &rspSlot, uint32_t u32Gen,
                     int32_t nPriority, const RgaBufPtr &rspBuf);

    /**
     * @brief   : 背压协议后半之后半：等回调 → 超时/失败分档计数 → 交出打平输出
     * @param    {nTimeoutMs}：等待上限。兼容壳传 m_nInferWaitMs；交错路径传同一个值
     *            （任务的 deadline 在提交那半按 nInferWaitMs×CLASSROOM_INFER_DEADLINE_TIMES
     *            定死，等待由谁数不影响它）
     * @param    {penReason}：可空 OUT。交错路径要它：A 路超时/失败不能读成 B 路的原因
     * @return   {bool} true 时 *ppvecRaw 有效，有效期到本次取用结束为止
     *            （期间不得对该键再次提交，否则指针指向的 vecRaw 会被回调覆盖）
     * @note     : 等待进入前会按开关调一次宿主 precomputeUrgency（优化6 挂点）——
     *            这里是全部推理等待的唯一汇合点，一处插入即覆盖六路
     */
    bool waitAsyncResult(const std::string &strKey, const WaitSlotPtr &rspSlot, uint32_t u32Gen,
                         int32_t nTimeoutMs, const std::vector<float> **ppvecRaw,
                         InferResultReason_E *penReason = nullptr);

    /**
     * @brief   : 这一路此刻是不是「问了也不会有结果」（等待槽还在途且未过看门狗线）
     * @return   {bool} true=在途。只读，**不建槽**：给没启用的模型白留一份槽缓冲没有意义
     */
    bool inferBusy(const std::string &strKey);

    /**
     * @brief   : 姿态一段（§4.4 按人轮询 + §4.5 人体框裁剪 + 跨帧缓存回填）
     * @param    {CPersonFeature_S*} patRows / nRowNum：本拍已写好的特征行，就地补骨骼列
     * @param    {CFeatureFrame_S&} stOut：本拍特征帧，回填实测节拍两列（见下）
     * @note     : 必须在写完特征行之后调：挑选要用到体框面积与跟踪 ID，
     *            而这两样在关联与写行阶段才定下来
     *            新鲜度窗口的口径由这里产出、由消费侧读取：本段是全场唯一知道
     *            「实际一拍做了几个、两拍隔多久」的人，配置里的 nMaxPosePersons 不是
     */
    void runPoseStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                      CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs,
                      CFeatureFrame_S &stOut);

    /**
     * @brief   : 人脸一段（P5 的第一段：整帧人脸检测 → 把脸配到人头上 → 写人脸列）
     * @param    {CPersonFeature_S*} patRows / nRowNum：本拍已写好的特征行，就地补人脸列
     * @param    {const std::vector<CTrackItem_S>&} vecTrack：本拍轨迹表（要用 nDetIdx 这条 ID 链）
     * @param    {const std::vector<int32_t>&} vecRowOfHead：头框下标 → 行下标，-1=没配到行
     * @note    : 与姿态段的两点不同，都是有意为之：
     *            1) **整帧检测**，不逐人裁剪——这颗模型收的就是整幅画面（640×640 里放全教室），
     *               成本与人数无关，所以它不需要姿态那套按人轮询的配额；
     *            2) 只在 sampling.face_interval_ms 到点时跑，失败**不推进**上次时刻，
     *               下一拍就重试——固定 2 秒一次的「稳定失败」读起来像「屏上就是没人脸」。
     *            这一段失败同样不影响本拍产出：没有人脸列只是这个人认不出，
     *            而人头已经跑出来了，特征帧照发。
     *
     *            配行的主判据是 **跟踪器本帧命中的头框下标**（vecTrack[n].nDetIdx → 行号），
     *            不是几何包含关系：同一帧的人头框与人脸框是两颗模型各自独立画的，中心会偏，
     *            而 ID 链是确定的。两条都用的顺序不能反——先按坐标挑会把邻座的脸配过来，
     *            而"名字挂错人"是 P5 验收线上最贵的一类错（误识 ≤1%）。
     *            几何包含只在 ID 链断掉时兜底（遮挡行没进特征行、或这拍该轨迹没命中头框）
     */
    void runFaceStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                      CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs,
                      const std::vector<CTrackItem_S> &vecTrack,
                      const std::vector<int32_t> &vecRowOfHead);

    /**
     * @brief   : 人脸检测**只提交不等**（优化3 的前半，bFaceEarlySubmit 开时由 run() §4.9 调）
     * @return   {bool} true=已提交（凭据在 m_stFaceTok，本拍 §5b 必须等它）；false=本拍没提交
     * @note     : 人脸检测只依赖原始帧，不依赖姿态——提前投进 NPU 队列，
     *            NPU 跑人脸的几十毫秒里 CPU 正去做姿态的挑选与裁剪。
     *            到期判定沿用 m_stFaceBeat（due 只判不写）；++u64FaceRounds 语义与原段一致
     *            （到期即计一次，无论提交成不成）；**提交失败不 book**——
     *            推进了就会变成固定 2 秒一次的稳定失败，读数与「屏上就是没人脸」一样。
     *            等待/解码/book/配行全在 runFaceFeatAndAssign，两半必须同拍成对
     */
    bool runFaceDetectOnly(const CFrame_S &stFrame, int32_t nRowNum, int64_t llNowMs);

    /**
     * @brief   : 人脸**等待+配行+取特征**（优化3 的后半，bFaceEarlySubmit 开时由 run() §5b 调）
     * @note     : 本拍没提交（没到期/提交失败/段被关）= 与原段提前返回完全同形：
     *            不解码不配行不取特征，人脸列与 nLastFaceDet/Row 都留在上一拍。
     *            等待失败同样不 book（下一拍重试）；成功路径（book/++u64FaceOk/decodeFaces/
     *            assignFaceRows/nLastFaceRow/runFaceFeatStage）与 runFaceStage 逐行同源。
     *            与优化5 共存时本函数落在统一驱动之后：配行只吃 trackId，与人脸在拍内的位次无关
     */
    void runFaceFeatAndAssign(const CFrame_S &stFrame, const ConfigPtr &spConf,
                              CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs,
                              const std::vector<CTrackItem_S> &vecTrack,
                              const std::vector<int32_t> &vecRowOfHead);

    /**
     * @brief   : warp 输出档就绪检查（缺就按模型输入尺寸建）
     * @return   {bool} true=m_stWarpPool 可借，且尺寸与模型输入一致
     * @note     : 建池这件事**不挂在 syncConfig 上**：配置版本不变而模型后到位（首版快照
     *            apply 时 FaceFeature 还没登记）就会让这一段永久缺池，而现象是"考勤一直没名字"。
     *            放在调用点前判一次，descOf 只是一次 map 查找，代价是每 2 秒一次
     */
    bool ensureWarpPool();

    /**
     * @brief   : 人脸特征一段（P5d）：逐人「裁剪 → 五点对齐 → 重采样 → 特征推理 → 底库比对」
     * @param    {int32_t} nRowNum：本拍特征行数，只用于读数（本段的人脸→ID 由 m_vecFeatTrack 给）
     * @note     : 本段不写特征行：特征与身份进 m_vecFeat 那张按跟踪 ID 存的跨帧表，由账本取走；
     *            顺路算出的情绪值也只进平滑器，落行统一交给每拍的 fillExpressionRows
     *            链路每一步的坐标口径都必须钉死，否则错法是"能跑、认不出人"：
     *            1) 框与五点用的是 **stMap 解出来的分析帧坐标**（镜像/旋转已施加），
     *               而像素从**未镜像的源帧**上裁，所以先 dstToSrc 再裁；
     *            2) 裁出来的是 nAlignCropPx 见方的图（160），不是模型输入的 112：
     *               对齐那一趟重采样承担缩放，让"裁 + 旋 + 缩"合成一次插值。
     *               直接按模型尺寸裁会把脸切成正方形以外的形状，再 warp 就是二次模糊；
     *            3) 换算五点用 cropRoi 回传的**真正用到的**那块源矩形（钳制取偶后的），
     *               用请求矩形会整体偏移最多 1 px，小脸上这 1 px 就是残差门的一半；
     *            4) 送解算前过 applyAlignPerm：镜像机位与模型点序这两件事全在那张表里，
     *               不翻转像素（翻转是恒等变换，见 face_align.h 文件末的推导）。
     *
     *            **为什么跑在人脸检测那一拍里而不是自己一个节拍**：这一步吃的框与点是
     *            「本拍刚解出来的」，跨拍沿用旧点去裁新帧，要么对齐偏掉（残差把整段吃光），
     *            要么被瞳距门静默拒掉——两种都表现为"识别率莫名低"，而读数上一切正常。
     *            同帧耦合是这条链的不变量，不是优化。
     *
     *            配额：每次最多 m_stFeatConf.nMaxPersons 张（默认 8），一张约一次 RGA +
     *            一次 warp + 一次 NPU 等待。60 人教室跑满一轮 = 8 张/轮 × 每 2s 一轮 ≈ 16s，
     *            两次命中才落名 ⇒ 点齐全班约 30~60s。这对"一节课的到课名单"是够的
     *            （考勤是会话级指标，不是逐秒判据），对"实时点名"不够——那种需求要的是
     *            提高 max_persons 或缩短 face_interval_ms，两个旋钮都在配置里（需求 11）
     */
    void runFaceFeatStage(const CFrame_S &stFrame, int32_t nRowNum, int64_t llNowMs);

    /**
     * @brief   : 在一张**刚为本段 warp 好的对齐图**上跑一次表情推理并喂进平滑器
     * @param    {rspBuf}：即取特征那份 warp 缓冲。刻意不另开裁剪档（框架图 组合4=复用组合2）
     * @param    {nTrackId}：平滑表的键。不落特征行——行由每拍的 fillExpressionRows 统一回填
     * @note     : 三件事在这里一并判掉，是因为它们都只在"图已经备好了"这一刻才判得准：
     *            1) 输入尺寸闸门——表情那颗与 FaceFeature 共用这块缓冲，尺寸不一致时
     *               raw 后端按字节数比期望会直接拒，而拒回来的读数与"模型失效"长得一样，
     *               所以先比描述表，不符就整段不做并记 u64ExprSizeBad；
     *            2) 那一路在途就跳过本张而**不让整段路**：考勤比情绪优先级高，
     *               表情排不上只是这个人这节课少一个采样点；
     *            3) 失败一律不回滚特征——这一步发生在 faceFeatUpsert 之后，
     *               情绪算不出来时名单该照常出（两问同一张图，但不该同生共死）
     */
    void runExpressionOfWarp(const RgaBufPtr &rspBuf, int32_t nTrackId, int64_t llNowMs,
                             float fYawDeg);

    /**
     * @brief   : 每拍把平滑器里的情绪值回填进特征行
     * @note     : 单独一拍回填而不是在取特征那一段里就地写：取特征约 16s 才轮到一个人，
     *            就地写会让这条曲线在报文里 1/8 的时间是值、7/8 是 UNKNOWN——
     *            那不是"稀疏"，那是下游读到的锯齿。与姿态/复核两列的跨帧缓存回填完全同构。
     *            取不到曲线值的那些行**显式清成 UNKNOWN/0/0**而不是留着：总线槽位是复用的，
     *            留着会把上一个人在这行的情绪接给新来的 ID（表按 ID 存，而 ID 会被复用）
     */
    void fillExpressionRows(CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs);

    /**
     * @brief   : 玩手机复核一段（P6b 的「事件触发路由」）：按规则层名单逐人裁一张全身图
     *            → PlayPhoneDetect → 把「看到了 / 没看到」写进跨帧表 → 回填特征行
     * @param    {CPersonFeature_S*} patRows / nRowNum：本拍已写好的特征行，就地补复核列
     * @note    : 三点与姿态段相同、一点不同，都是有意为之：
     *            1) **裁剪档复用姿态那副人体框**（§4.5 ①/④ 同一条链）：两次裁剪的输入区域一致，
     *               所以判「手里有没有东西」与判「手在不在身前」看到的是同一张图，
     *               不会出现规则层说手在下巴下面、复核层看到的却是另一块地方的错位；
     *            2) 表按跟踪 ID 存、按 TTL 清、回填放在段尾——与 m_vecPoseCache 完全同构，
     *               理由见 phone_recheck.h 文件头（特征行每拍重建而复核是限流的）；
     *            3) 上一拍在途 / 等到超时时**不盖时间戳**并放弃本拍剩下的人（bGiveUpRound 那条），
     *               裁剪太小、后端拒绝、映射建不起来是真的试过，必须盖：不盖的话名单顶名
     *               那个人每拍排第一，别人永远轮不到（与姿态段同一个饿死形状）；
     *            4) 与姿态段唯一不同：**候选来自宿主而不是自己挑人**。感知层不认识「低头」
     *               这个判据（见 IPerceiveHost::phoneRecheckIds），名单为空时一次推理都不提交。
     *
     *            模型没驻留（裁剪档缺失）时整段直接返回，且**不写任何否决**：跑不了不等于没目标。
     *            这一段失败同样不影响本拍产出——复核缺位只是这行没有否决权，
     *            而规则证据本来就在帧里
     */
    void runPhoneStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                       CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs);

    /**
     * @brief   : 姿态+手机复核统一驱动（优化5）：两路目标一次挑好，提交与等待交错进行
     * @param    {CPersonFeature_S*} patRows / nRowNum：本拍已写好的特征行，就地补骨骼列与复核列
     * @param    {CFeatureFrame_S&} stOut：本拍特征帧，姿态实测节拍两列由这里发布（与姿态段同一列）
     * @note     : 仅在 bSubmitAsyncEnable && bPosePhoneInterleave 时由 run() 调；
     *            开关关闭走原 runPoseStage + runPhoneStage（逐行为一致）。交替形状：
     *            提交姿态 → 等手机 → 提交手机 → 等姿态——NPU 队列里始终压着两路任务，
     *            CPU 在等待间隙做解码与结账，不再「姿态全做完 → 手机才开始」地串行空等。
     *            与原两段的纪律逐条相同：
     *            1) 产能窗（poseWantNum 有副作用：推进配额窗口与迟滞闩）每拍仍只调一次；
     *            2) 让路（beginRound 在途拒绝 / 等待超时）不盖时间戳不扣配额，且**本侧收工**，
     *               另一侧继续——两路的在途槽是 m_mapSlot 里两个独立条目，互不占位；
     *            3) 其余失败（裁剪太小/后端拒绝/映射失败）都是真实尝试，立刻按原语义结账
     *               （tried++ + 无效缓存 + 配额），不让名单顶名的人饿死一整排；
     *            4) 统计口径不变：nLastPoseWant/Left/Rows、nLastPhoneWant/Rows/List 与
     *               两段的循环体逐格同源，只是记账时点从「等完」提前到「提交即知败」的那几格
     */
    void runPoseAndPhoneStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                              CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs,
                              CFeatureFrame_S &stOut);

    /* 交错驱动的单人结账半段：等待返回（成功/映射失败）与「提交即真败」共用，
       计数、首例日志、缓存回填与原两段循环体逐行相同。bDecoded=false 时 stSkl
       必须是默认构造（无效骨骼），applyPoseCache 据此不当证据 */
    void accountPoseAttempt(CPersonFeature_S &stRow, bool bDecoded, const CPoseSkeleton_S &stSkl,
                            const CRectf_S &stUsed, int64_t llNowMs, int32_t nRowNum);
    void accountPhoneAttempt(CPersonFeature_S &stRow, bool bLooked, bool bFound, float fScore,
                             const CRectf_S &stUsed, int64_t llNowMs,
                             const std::vector<int32_t> &vecWantIds, int32_t nRowNum);

    /**
     * @brief   : 本拍允许做几个人姿态 = min(本周期配额剩余, NPU 预算允许的人数)
     * @param    {int32_t&} rnLeftInWindow：OUT=扣掉本拍之后本周期还剩几个人（诊断用）
     * @return   {int32_t} 0 表示这一拍整段让路（配额用完 / NPU 忙 / 预算配成 0）
     * @note     : 副作用是推进配额窗口与那条迟滞闩，因此**每拍只准调一次**（在 runPoseStage 里）
     */
    int32_t poseWantNum(CInferScheduler *pSched, const ConfigPtr &spConf, int64_t llNowMs,
                        int32_t nRowNum, int32_t &rnLeftInWindow);

    /* 人头框内亮度梯度的均值 → 0~1 清晰度代理（需求 11 的远焦模糊） */
    float blurScoreOf(const CFrame_S &stFrame, const CRectf_S &stBoxCanvas) const;

    /* 把本帧人员框换算回源帧坐标后送整机 OSD（画什么由配置 osd_enable 决定） */
    void pushOsd(const CFeatureFrame_S &stFrame);

private:
    IPerceiveHost *m_pHost;

    CPersonTracker m_stTrk;

    /* ---- 运行期缓冲：构造期一次定容，此后不再改变容量（§1.2 零分配红线）---- */
    std::vector<CDetection_S> m_vecHead;         /* 送跟踪器的人头框 */
    std::vector<CPersonBox_S> m_vecPersonBox;
    std::vector<int32_t> m_vecBoxOfDet;          /* 人头框下标 → 人员行下标，-1=无 */
    std::vector<CRectf_S> m_vecOsdBox;           /* 送 OSD 的框缓冲（分析帧坐标，换算在 egress 侧） */
    /* 骨骼跨帧缓存：定容 CLASSROOM_MAX_PERSONS，单行 ≈328B，满表约 20KB。
       这 20KB 是买「轮不到的那几拍也有证据」——不缓存的话 60 人教室里后排同学的骨骼
       每 5 拍才出现一次，时长门会把一段连续行为切成好几条短事件（需求 10 的抖动红线） */
    std::vector<CPoseCacheEntry_S> m_vecPoseCache;
    std::vector<CPoseCand_S> m_vecPoseCand;      /* 挑选临时表，只在本拍内有效 */
    std::vector<int32_t> m_vecPoseSel;           /* 本拍选中的行下标 */
    /* 本拍解出的人脸表：构造期定容 CLASSROOM_MAX_PERSONS（§1.2）。
       上限取人数上限而不是「一屏能有几张脸」——同屏脸数多于人数时多出来的必然配不到行，
       留更大容量只是把配不上的脸留在内存里，观测上反而看不出是被谁挤掉的 */
    std::vector<CFaceDet_S> m_vecFace;
    std::vector<int32_t> m_vecFaceRow;           /* 脸下标 → 行下标，-1=没配上（只在本拍内有效） */

    /* ---- 人脸特征段（P5d）的四张表 + 一块 warp 缓冲 ----
       特征表是这段里唯一常驻的大块：每行 ≈2.2KB（512 维 float + 比对结论里那两个 string），
       定容 CLASSROOM_MAX_PERSONS 后满表 ≈132KB。这 132KB 买的是「检出过的人在一轮轮转里
       各有一份等取的证据」——不留表的话比对只能在检出那一拍做完，而人脸段每 2 秒才跑一次，
       后排同学的特征就永远排不到（face_feature.h 文件头第 1 条）。
       m_stWarpPool 是 CPU 重采样的输出：2 块 × 模型输入字节数（112²×3 ≈ 37KB），
       刻意用 CRgaPool 而不是一个裸 vector——它的 shared_ptr 语义正是调度器请求要的那种
       所有权移交（缓冲活到推理结束自动回收），而"连续两次推理不得用同一块"这条实测约束
       由它的环形游标满足（见 rga_pool.h 约束 2）。自己写一遍就是两处真值 */
    std::vector<CFaceFeatEntry_S> m_vecFeat;
    std::vector<int32_t> m_vecFeatTrack;         /* 脸下标 → 跟踪 ID，-1=不参与挑选（本拍内有效） */
    std::vector<CFaceFeatCand_S> m_vecFeatCand;  /* 挑选临时表，只在本拍内有效 */
    std::vector<int32_t> m_vecFeatSel;           /* 本拍选中的脸下标 */
    CRgaPool m_stWarpPool;
    int32_t m_nWarpW = 0;                        /* 池已按此尺寸建好；0=还没建（模型没登记） */
    int32_t m_nWarpH = 0;

    /* 复核的跨帧表：定容 CLASSROOM_MAX_PERSONS，每行 24B，满表 ≈1.5KB。
       这 1.5KB 买的是「没轮到复核的那几拍也还留着上一次的结论」——复核一拍只做两个人，
       而特征行每拍从零重建，不留表的话规则层看到的永远是「一拍有一拍没有」，
       106 的 3s 时长门一条也攒不出来（详见 phone_recheck.h 文件头）。
       临时表与选中表只 reserve：它们每拍 clear 后按名单实际长度增长 */
    std::vector<CPhoneCacheEntry_S> m_vecPhoneCache;
    std::vector<CPhoneCand_S> m_vecPhoneCand;
    std::vector<int32_t> m_vecPhoneSel;
    /* 复核框的解码输出：一次只装一个人裁剪图里解出的那几个框，拍内复用。
       按人数上限定容而不是按「一屏能有几部手机」——decodeBoxes 只 clear 不缩容，
       留小了就会在某个人身上悄悄触发一次 realloc（§1.2 零分配红线） */
    std::vector<CDetection_S> m_vecPhoneBox;

    std::map<std::string, WaitSlotPtr> m_mapSlot;

    /* ---- 只在节拍线程写的状态 ---- */
    int64_t m_llConfVersion = -1;
    /* 人脸节拍的闸（#88 P0-4）：判定与记账分离——due() 只判，book() 只在成功后调，
       失败的那一拍不许把重试推迟一个整周期（原两条节拍里的人体那条已随 #96 摘除） */
    SBeatGate_S m_stFaceBeat;                    /* 人脸段自己的节拍（sampling.face_interval_ms） */
    /* 人脸检测提前提交（优化3）的在途凭据：提交在 §4.9、等待在 §5b，同一拍内必达成对，
       不跨拍存活——bFacePending 在等待入口即清， token 失效路径只剩「提交失败」（本来就无凭据） */
    SPendingTok_S m_stFaceTok;
    bool m_bFacePending = false;
    uint32_t m_u32SubmitGen = 0;                 /* 提交序号发生器，0 留作「无在途」哨兵 */

    int32_t m_nPoseCacheInUse = 0;               /* 缓存表已用槽数 */
    /* ---- 姿态**实测**节拍（#69-A）：一拍多久 × 一拍几个人，两列合起来就是产能。
           窗口（TTL）由它算，不再由配置里的 nMaxPosePersons 算——那是「希望一拍做几个」，
           配额/NPU/时间片三层闸门任一收紧就做不到（板上实测 1 个/拍），拿它当产能会让
           窗口比一整圈还短，表现成「后排永远 UNKNOWN、举手固定晚一圈」这种最难查的漏检。
           平滑与语义口径都在总线层的 poseCadenceFeed 里（连它的用例也在那一侧）---- */
    CPoseCadence_S m_stPoseCadence;
    int32_t m_nPhoneCacheInUse = 0;              /* 复核表已用槽数 */
    /* 准入判据（配额窗口 / NPU 忙闲迟滞 / 预算自适应）的跨拍状态（P2-8）：
       判据本体在 pose_admit 纯函数层（主机可测），这里是那一份真身；
       人次账由提交侧在这里累加（含失败的尝试），判据层只读 */
    SPoseAdmitState_S m_stPoseAdmit;

    CDetectParam_S m_stHeadParam;
    CBodyAssembleParam_S m_stAssembleParam;
    CPoseDecodeParam_S m_stPoseParam;
    CFaceDecodeParam_S m_stFaceParam;
    /* 复核框的解码参数：置信度门与头/体/脸同源（threshold.det_score），尺寸门独立
       （手机在人体裁剪图里本来就比头小一档，拿 min_head_pixel 当闸会整批挡掉真框） */
    CDetectParam_S m_stPhoneParam;
    /* 五点 → 相似度变换的解算参数。配置键在 align.*（classroom_config.h 的 stAlign），
       由下面的 syncConfig 逐版同步；模板点与三道闸门各自的理由写在 CFaceAlignParam_S 的字段上 */
    CFaceAlignParam_S  m_stFaceAlignParam;
    /* 点序置换表下标（align.point_order）。镜像机位与模型出点顺序这两件事**只由这一个量表达**：
       翻转像素是恒等变换（face_align.h 文件末有推导），所以这里没有第二个旋钮 */
    int32_t m_nAlignPointOrder = CLASSROOM_ALIGN_PERM_IDENTITY;
    bool    m_bAlignOrderProbe = true;   /* align.order_probe：喂探针，用来标定上面那个数 */
    int32_t m_nAlignCropPx = CLASSROOM_ALIGN_CROP_DEFAULT_PX;  /* align.crop_px：见方裁剪档边长 */
    CFaceFeatConf_S m_stFeatConf;        /* face_feature.*：开关与本段配额 */
    /*「人脸检测区域」（gallery.region_ids，空 = 全画面）。
       存在管线里而不是每拍从快照取：特征段拿不到 ConfigPtr（它是 runFaceFeatStage 的入参
       之外的上下文），而这一项只随配置换版变化，抄一份与 m_stFeatConf 同一口径 */
    std::vector<int32_t> m_vecGalleryRegions;
    CPhoneRecheckConf_S m_stPhoneConf;   /* phone_recheck.*：复核开关、每拍几个人、同人间隔、结论活多久 */
    /* expression.*：只有开关与平滑行的 TTL 两项是本层的，判定门限在 m_stExpr 里
       （它才是消费那些门限的人）。**没有配额/间隔项**是刻意的：表情没有自己的节拍，
       它的频率就是取特征轮转到这个人的频率，另配一个旋钮会出现两个口径互相打架 */
    CExpressionConf_S m_stExprConf;
    CExpressionEstimator m_stExpr;
    /* optimization.* 六个灰度开关（流水线优化批）：构造期全 false，随配置版本整份抄。
       生效判定一律在各自的调用点做「主闸 && 分开关」，这里不再派生第二份有效位缓存——
       两份开关状态就是两处会漂移的真值 */
    COptimizeConf_S m_stOpt;
    /**
     * 表情那颗的输入尺寸与本段 warp 输出档是否一致。随 ensureWarpPool 一起算，
     * 不放 syncConfig 也不每拍现查：前者在模型登记之前就跑完了（配置版本不变而模型
     * 后到位时永远读到 false），后者是为一个只在换模型那天才会变的量付 map 查找。
     * 不一致时整段不做——把 112 的缓冲喂给 160 的模型，后端按字节数比期望会拒，
     * 而那条读数与"模型坏了"在心跳里是同一个数，分不开
     */
    bool m_bExprSizeOk = false;

    /* 表操作自己的计数与探针读数。前者已由 faceFeatStatText 进心跳，
       后者是「这台设备的五点到底是什么顺序」的唯一出口——一次上机就够定论，故常驻不清 */
    CFaceFeatStat_S m_stFeatStat;
    CFaceAlignProbe_S m_stAlignProbe;

    /* 每拍要用但不属于 CDetectParam_S 的几项，随配置版本一起同步（需求 11 全可配）。
       置信度门单独留一份是因为下发点在 inferOne 里，而那里只认模型键不认「哪一路的参数表」 */
    int32_t m_nInferWaitMs = CLASSROOM_INFER_WAIT_MS;
    int32_t m_nPoseBeatBudgetMs = CLASSROOM_POSE_BEAT_BUDGET_MS;
    /* 0 = 这一段关着（尚未同步到配置，或配置里就配了 0）。不在这里再写一个 2000：
       配置里那份才是真值，第二处默认值就是「改了配置没生效」的现成事故点 */
    int32_t m_nFaceIntervalMs = 0;
    float   m_fDetectScoreThreshold = CLASSROOM_DETECT_SCORE_THRESHOLD;
    float   m_fNmsThreshold = CLASSROOM_DETECT_NMS_THRESHOLD;
    float   m_fBlurFullGrad = CLASSROOM_BLUR_FULL_GRAD;

    /* 两张解码表而不是共享一张：现场第一个要回答的问题是「哪一路不出框了」，
       合计表答不了 —— 人头恒 0、复核正常，与两路各出一半，在合计里是同一个数字 */
    CDetDecodeStat_S m_stDecodeStat;       /* 人头路 */
    CDetDecodeStat_S m_stPhoneDetStat;     /* 复核路（PlayPhoneDetect）。第二张同构表：
                                              「复核一个框都不出」与「复核框全被尺寸门吃掉」
                                              在合计里是同一个数字，而处置方式完全不同 */
    CBodyAssembleStat_S m_stAssembleStat;
    CPoseDecodeStat_S m_stPoseStat;        /* 姿态路 */
    CFaceDecodeStat_S m_stFaceStat;        /* 人脸路（P5） */
    CFaceAlignStat_S  m_stFaceAlignStat;   /* 五点解算路（P5） */
    CPerceiveStat_S m_stStat;
};
}
