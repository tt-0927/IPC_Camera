/**
 * @FilePath     : rule_engine.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 14:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 14:57:00
 * @Description  : 规则引擎
 *                 职责边界：
 *                   1) 调度若干 IBehaviorSampler，把每帧每人采样成"行为 x 三态"矩阵；
 *                   2) 用 CDurationGate 做时间累积，把三态流变成有起止的 begin/end 事件；
 *                   3) 维护在场人员表、区域表、主体仲裁、事件环形队列与统计。
 *                 线程模型：run() 由感知线程独占调用，内部不加锁；
 *                           跨线程读统计/活跃数走 m_mtxPub 保护的双缓冲发布快照。
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "classroom_config.h"
#include "duration_gate.h"
#include "feature_frame.h"
#include "region.h"
#include "subject_arbiter.h"

namespace Classroom
{
/* 事件缓冲容量：下游（P3 上报）取走前在这里堆。满了丢最旧并计数，绝不无界增长
   （现网事件算法正是在这里没设上限，长跑后队列吃掉几十 MB）。
   容量不是一个凭手感的常数，而是**一拍的产出上界**：一拍里每个 (人 × 行为列) 至多产生一条
   事件，人数被 CLASSROOM_MAX_PERSONS 封顶、列数被 BEHAVIOR_COLUMN_NUM 封顶，所以这个乘积
   可证——取在它之下就会在正常场景里丢数据，而不是在异常场景里做背压。
   旧值 64 正是这样：总开关落下那一拍要给全班每个生效中的行为补一条 END，60 人 × 2 列就有
   120 条，而这一整批是在同一次调用里灌进来的、中间没有取数机会，于是被截断的是这一批里
   最早的那几十条，平台侧看到的正是"只有开始、没有结束"的悬挂时间轴（主机用例
   testEngineCloseBatchSurvivesRing 实测 120 进 64 出）。反向的 ON 沿同一条路：开关合上后
   所有人的门限同时起算，第一拍就会同时激活上百条。
   真实要防的"消费停了"仍然防着：跨拍堆积超过这个上界照样丢最旧并计 u64EventDrop。 */
#define CLASSROOM_RULE_EVENT_RING (CLASSROOM_MAX_PERSONS * CDurationGate::BEHAVIOR_COLUMN_NUM)

/* 采样器路数上限：P2 学生姿态、P4 教师、P5 人脸、P6 玩手机/板书/展示 —— 装配在 init 完成，
   运行期不再增删（与 §4.9 线程数同一纪律：可数的东西必须启动时就算清） */
#define CLASSROOM_MAX_SAMPLERS (8)

/* 班级级（帧级）事件用的哨兵 personId。
   207 指导 / 209 互动 / 210 巡视 是**整帧**事实，不挂在任何跟踪行上：
   210 问「讲台里有没有人头框」，207/209 问「这一帧有没有人站着/举手」。
   把它们硬挂在某一行（教师行或"证据最强"的学生行）会让事件主体随跟踪 ID 漂移，
   换一张脸就换一条事件。哨兵行由引擎自己喂门（feedClassLevel），跟踪 ID 从 1 起分配
   （person_tracker 的 m_nNextId），所以 0 永远撞不到真人；下游数据中心/合并器只丢
   nPersonId<0，0 能一路走到上报 —— 平台按 behavior_id + person=0 识别「班级级事件」 */
#define CLASSROOM_CLASS_PERSON_ID (0)

/* 感知断档超过「特征帧 TTL × 该倍数」时，先把所有生效中的行为收尾再判本拍。
   取 2 倍而不是 1 倍：一拍最多拖到 TTL 边缘是正常的（等推理），翻倍才算「这段时间真的没证据」 */
#define CLASSROOM_RULE_STALL_TIMES (2)

/* #92 「这一拍没有骨骼证据」（SR_NO_POSE）喂门的冻结上限（毫秒）。
   冻结 = 这拍不喂门，门里的容忍额度不被调度空窗消耗（守护降频/轮转空窗不再切事件）；
   真实离场由 retireGone 补 END、感知停摆由断档分支收，都不经过这里。
   但姿态组若真死（驻留再不回来），事件必须还能结束：冻结超过本值就按老口径
   喂一次 UNKNOWN 交还容忍额度。取 30000 = 盖住实测过的 19s wedge（§4.38）还有余量，
   又把僵尸事件的尾巴限在半分钟内；等机上量出降频期的真实拍距再标定 */
#define CLASSROOM_RULE_POSE_FREEZE_MS (30000)

/* 跟踪 ID 消失后的收尾宽限（毫秒）。
   背景：retireGone 原先「本拍没这一行」就当场补 END。
   一帧丢检（遮挡、跟踪抖动）与真正离场在数据上长得一样，于是同一段行为被切成几十段：
   best_t 教师机位 660s 内 211 出 26 段、212 出 37 段，段间空档 0.3~2.7s（12 个 0.3s），
   而把 211/212 的 gap_tolerance 抬到 3000 后段数几乎不变（22/39）——因为收尾走的是退役，
   不是门里的容忍额度。
   语义：宽限期内这个人「没被看见」而不是「走了」——门状态原样冻结（不喂 UNKNOWN，
   与 #92 姿态冻结同一条纪律：没有证据 ≠ 证据说没有）；超过本值才补 END 并回收槽位。
   取 2000：覆盖实测的 0.3~1.8s 丢检带，真实离场的 END 也只晚 2s（远小于 60s 上报周期）。
   宽限名单满（不应发生）时退回老口径当场收尾——宁可切一段，也不让门状态泄漏。 */
#define CLASSROOM_RULE_RETIRE_GRACE_MS (2000)

/**
 * @brief   : 一拍样本的成因（为什么是这个判定）
 * @note    : 这一组计数是 P2 上机后**唯一**能回答「行为为什么不出」的读数：
 *            UNKNOWN 一大半且 SR_NO_POSE 占大头 → 姿态组没驻留或时间片空窗；
 *            SR_REGION_MISS 非 0 → 区域没换算成功或引用了不存在的 id（配置问题）；
 *            SR_BLUR 吃满 → 门限配高了，镜头本来就该判这么严吗？看数说话
 */
typedef enum class SampleReason : int
{
    SR_OK = 0,        /* 证据齐，判定可信 */
    SR_NO_FRAME,      /* 特征帧/人数行缺失 */
    SR_NO_POSE,       /* 姿态组未驻留，或本行的骨骼结果已过期 */
    SR_KEY_LOW,       /* 需要的关键点置信度不足 */
    SR_OCCLUDED,      /* 与邻框重叠超门限，身体不可见 */
    SR_EDGE,          /* 框贴画面边，姿态被切一半 */
    SR_BLUR,          /* 清晰度低于判定下限（需求 11：远焦糊掉就不该硬判） */
    SR_SUBJECT,       /* 这一行不归本采样器管（学生采样器遇到教师行） */
    SR_INHIBIT,       /* 证据齐全，但被同族更高优先级的判定压掉（趴桌成立时不再报低头/抬头） */
    SR_REGION_MISS,   /* 引用的区域查不到：不能判「不在区域内」，只能判不可判 */
    SR_MODEL_NEG,     /* 规则证据成立，但按需复核看了那一张图、没检出目标：这是真「没有」，不是「不知道」 */
    SR_MODEL_POS,     /* 复核在那张图里**检出了**目标：靠「没有目标」成立的那一列（108 阅读）因此不成立。
                         与 MODEL_NEG 同为「模型定的」而方向相反，分成两个原因才答得出「阅读为什么没有」 */
    SR_NO_MODEL,      /* 本列的判定要看复核结论，而这一拍没有复核结论（没排到、没驻留、复核被关）：
                         108 与 106 共用同一个形状，缺了复核就是「不知道在看什么」，不得当成「没在看」 */
    SR_FAIL_OPEN,     /* 106 复核缺证时的**放行**（状态仍 TRUE）：与 SR_NO_MODEL 分开是因为处置相反——
                         NO_MODEL 挂在 108（判不出），FAIL_OPEN 挂在 106（宁可放行不可静默吃掉）。
                         fail-open 恒放行而读数不可见，就是「复核跟不上」在现场唯一的现形处（#92） */
    SR_STAY_SHORT,    /* 207 指导：这一行确实在区域里站着，但连续停留还没到 stay_s。
                        给 UNKNOWN 而不是 FALSE——「刚站起来的人」不是「没人站着」，
                         写成 FALSE 会把一条正在成立的 207 提前收尾（Web 14 页 stay_s） */
    SR_NUM
} SampleReason_E;

/**
 * @brief 一次行为样本
 */
typedef struct _BehaviorSample_
{
    TriState_E    enState = TriState_E::TS_UNKNOWN;
    SampleReason_E enReason = SampleReason_E::SR_OK;
    /* 列专属的附加读数，语义按行为列定义，没写的列恒 -1（#88）：
       目前只有 112 写它（表情类别 Expression_E，喂门的那一拍的类别）。
       放在样本而不是让引擎回头查感知行：类别是「开门那次判定」的一部分，
       门里存的就是最后一次 TRUE 采样自带的类别 */
    int8_t        nAux = -1;
} CBehaviorSample_S;

/**
 * @brief   : 本拍班级级事实的原始汇总（逐行样本的计数，不含判据）
 * @note    : 207/209 的证据来自学生两列（101/102）的**原判**，在逐行派发里就地累加。
 *            与旧版「站立/举手各一组」相比，这两组是**按页过滤之后**的事实：
 *            207 只数「页 14 画的框内、且连续站够 stay_s」的行；
 *            209 只数「页 15 画的框内、且骨骼均分过交互置信门」的站立/举手行。
 *            两页的框与门不同，所以不能共用一份计数（同一行在 207 里成立、在 209 里可能不成立）。
 *            三态聚合与开关闸仍在 CRuleEngine::feedClassLevel：
 *            「存在性」判据的口径（TRUE 任一、FALSE 须全否）属于引擎的记账纪律
 */
typedef struct _ClassFacts_
{
    /* 207 指导：页 14 区域 + stay_s 之后仍成立的站立行 */
    uint32_t u32GuideNum = 0;
    uint32_t u32GuideTrue = 0;
    uint32_t u32GuideFalse = 0;
    SampleReason_E enGuideUnknown = SampleReason_E::SR_NO_POSE;
    /* 209 互动：页 15 区域 + 交互置信门之后的站立∪举手行 */
    uint32_t u32InteractNum = 0;
    uint32_t u32InteractTrue = 0;
    uint32_t u32InteractFalse = 0;
    SampleReason_E enInteractUnknown = SampleReason_E::SR_NO_POSE;
} CClassFacts_S;

/**
 * @brief   : 教师机讲台状态（210 的判据事实）的发布件
 * @note    : 平台收到学生机 207/209 的事件后会**主动来问教师机**「讲台有没有人」，
 *            课程报告（AC_CLASSROOM_GET_COURSE_REPORT）里的 `podium` 段就是这份答案。
 *            写工作份只在节拍线程（feedClassLevel），发布件在 publishStat 的同一把锁里抄，
 *            读侧（reportJson，命令线程）拿到的永远是某一整拍的结果。
 *            `llWallMs` 是判定那一拍的**墙钟**而不是帧时：感知停摆时帧时不前进，
 *            用帧时算 age 会把一段十分钟的陈旧读数显示成 0ms——平台据此判断"这个答案现在还算不算数"。
 */
typedef struct _ClassPodiumState_
{
    bool           bValid = false;                              /* 还没跑过一拍：默认值不是读数 */
    TriState_E     enState = TriState_E::TS_UNKNOWN;
    SampleReason_E enReason = SampleReason_E::SR_NO_FRAME;
    int64_t        llFrameMs = 0;                               /* 判定所用特征帧的时刻（诊断用） */
    int64_t        llWallMs = 0;                                /* 同一拍的墙钟（age_ms 的来源） */
} CClassPodiumState_S;

/**
 * @brief   : 成因名（日志与报文共用的唯一一份表）
 * @note    : 从 CRuleEngine::reasonName 挪出来是因为报文层（egress）也要写这个名字。
 *            做成 inline 而不是再开一个 .cpp：egress 那套夹具不链规则引擎（它只需要这个名字表），
 *            真正的定义放在 .cpp 里就会变成一条 "undefined reference" 的构建坑。
 *            枚举加一项这里自动多一项，switch 少了 case 由 -Wswitch 当场报
 */
inline const char *sampleReasonName(SampleReason_E enReason)
{
    switch (enReason)
    {
    case SampleReason_E::SR_OK:           return "ok";
    case SampleReason_E::SR_NO_FRAME:     return "no_frame";
    case SampleReason_E::SR_NO_POSE:      return "no_pose";
    case SampleReason_E::SR_KEY_LOW:      return "key_low";
    case SampleReason_E::SR_OCCLUDED:     return "occl";
    case SampleReason_E::SR_EDGE:         return "edge";
    case SampleReason_E::SR_BLUR:         return "blur";
    case SampleReason_E::SR_SUBJECT:      return "subject";
    case SampleReason_E::SR_INHIBIT:      return "inhibit";
    case SampleReason_E::SR_REGION_MISS:  return "region";
    case SampleReason_E::SR_MODEL_NEG:    return "model_neg";
    case SampleReason_E::SR_MODEL_POS:    return "model_pos";
    case SampleReason_E::SR_NO_MODEL:     return "no_model";
    case SampleReason_E::SR_FAIL_OPEN:    return "fail_open";
    case SampleReason_E::SR_STAY_SHORT:   return "stay_short";
    default:                              return "bad";
    }
}

/**
 * @brief 采样器的一拍输入（全部为借用指针，函数返回即失效）
 */
typedef struct _BehaviorSampleIn_
{
    const CFeatureFrame_S   *pstFrame = nullptr;   /* 整帧：需要邻框/驻留标志时用 */
    const CPersonFeature_S  *pstPerson = nullptr;  /* 本行 */
    const CConfigSnapshot_S *pstSnap = nullptr;    /* 与本次判定同一份配置快照 */
    const CRegionTable      *pstRegion = nullptr;  /* 区域表（分析帧坐标） */
    /**
     * @brief 本行的归属事实，null 表示这一拍没有仲裁结论
     * @note  采样器**优先读它、退读 pstPerson->enSubject**：感知层那一列写的是逐帧默认值
     *        （目前恒为 STUDENT），仲裁层写的才是带确认/放弃迟滞的判断。反过来以帧内那列为准，
     *        教师归属就在采样这一步被抹掉，而这正是 P4 全部教师行为的总闸
     */
    const CSubjectFact_S    *pstSubject = nullptr;
    CSubjectView_S           stSubjectView;        /* 帧级归属上下文（角色画没画、空不空） */
    /* 上一拍各列的生效人数，null 表示还没有（第一拍）。为什么是上一拍：
       「学生举手 + 老师讲台」这类判据要的是**整帧**的群体状态，而逐行判定时学生那几行
       可能排在教师行之后、这一拍还没判完。用上一拍等于最多晚一拍开事件（采样间隔本来就是
       100~1000ms），比在同一个数组上边写边读安全——后者会让同一条互动判据的结果
       取决于"老师那一行恰好排在学生前面还是后面"，而这种依赖在换机型、换人数时会翻 */
    const int32_t           *panActivePrev = nullptr;
    /* 本帧每一行的归属，与 pstFrame->vecPersons **同下标**（不是同 personId）。
       入参本已有 pstSubject，但那是**本行**的事实：讨论 109 要问「邻座那一位是不是老师」、
       演示 111 要问「全班在哪个方向」，这两问没有这一列就答不了——学生会因为转身问老师
       被判成讨论（那是互动，账在 209/102 上），而全班方向会被老师那一站带偏。
       nullptr = 本拍没填（单测手搓输入、老调用点）：读它的判据必须退成 UNKNOWN，
       **不得**退成「当他是学生」——那等于把没证据读成有证据 */
    const int8_t            *penRowSubject = nullptr;
    int32_t                  nRowSubjectNum = 0;
    /* 本行在 vecPersons 里的下标（-1 = 没填）。要看别人的那些判据（邻座、全班方向）
       第一件事是**跳过自己**：拿 personId 比也行，但那样每个判据都要自己再认一遍"哪个是空行"，
       而下标是引擎本来就有的事实，给出来就是省掉每一路各自发明一次排除法 */
    int32_t                  nSelfIdx = -1;
    int64_t                  llNowMs = 0;          /* 时基：取特征帧产出时刻，回放可复现 */

    /* 上一拍某行为的生效人数（未注册/越界/第一拍返回 0，即"没有人"而不是"不知道"：
       这是群体条件的必要项，缺证据时判据本就该不成立） */
    int32_t prevActiveNum(Behavior_E enBehavior) const;

    /**
     * @brief   : 取第 nIdx 行（vecPersons 下标）的归属
     * @return   {Subject_E} 没填、越界、该行没有槽位一律 SUBJECT_UNKNOWN——调用方按「不知道」处理
     */
    Subject_E rowSubjectAt(int32_t nIdx) const
    {
        if (nullptr == penRowSubject || nIdx < 0 || nIdx >= nRowSubjectNum)
        {
            return Subject_E::SUBJECT_UNKNOWN;
        }
        return static_cast<Subject_E>(penRowSubject[nIdx]);
    }
} CBehaviorSampleIn_S;

/**
 * @brief   : 行为采样器（每路一组行为，P4/P5/P6 各自新增实现，不改引擎）
 * @note    : 实现方必须保证 rvOut 与 behaviors() 同序、长度一致，且**不改变外部状态**
 *            （同一拍可能因为降级被重跑，副作用会让计数翻倍）
 */
class IBehaviorSampler
{
public:
    virtual ~IBehaviorSampler() = default;

    IBehaviorSampler(const IBehaviorSampler &) = delete;
    IBehaviorSampler &operator=(const IBehaviorSampler &) = delete;

    /* 日志与观测里用的短名，必须小写无空格（会直接拼进 statusJson） */
    virtual const char *name() const = 0;

    /* 本路负责的行为，rvOut 的下标与此表一一对应 */
    virtual const std::vector<Behavior_E> &behaviors() const = 0;

    /**
     * @brief   : 对一个人出一拍样本
     * @param    {const CBehaviorSampleIn_S&} stIn：本拍输入
     * @param    {std::vector<CBehaviorSample_S>&} rvOut：OUT=与 behaviors() 同序的判定，
     *            引擎已按长度 resize，实现方只需逐项写
     * @note    : 没有证据时必须逐项写 TS_UNKNOWN 并给原因，**不得**用 FALSE 表示「不知道」
     */
    virtual void samplePerson(const CBehaviorSampleIn_S &stIn,
                              std::vector<CBehaviorSample_S> &rvOut) = 0;

    /**
     * @brief   : 本路是否还有开着的行为（全关就整路不跑，省一次骨骼遍历）
     * @note    : 非虚：判据只是「声明表 ∩ 配置开关」，没有需要多态的部分
     */
    bool anyEnabled(const CConfigSnapshot_S &stSnap) const;

protected:
    /* 声明了 delete 的拷贝构造会连带抑制隐式默认构造，派生类于是连基类子对象都造不出来。
       显式给一条、且给在 protected：接口本身不该被实例化，构造权只留给实现方 */
    IBehaviorSampler() = default;
};

/**
 * @brief 一次行为起止事件（P3 上报与 OSD 标注的共同数据源）
 */
typedef struct _BehaviorEvent_
{
    int32_t      nPersonId = -1;
    int32_t      nSeatId = -1;            /* 座位先验，未分配 -1 */
    Behavior_E   enBehavior = Behavior_E::STU_STAND_UP;
    bool         bBegin = true;           /* true=进入该行为，false=退出 */
    int64_t      llMs = 0;                /* 进入=本轮起点（回推到第一个 TRUE），退出=结束时刻 */
    int64_t      llHeldMs = 0;            /* 持续时长，退出时有效 */
    CRectf_S     stHeadBox;               /* 触发时的人头框（分析帧坐标），换算上报时用几何逆变换 */
    uint32_t     u32FeatureSeq = 0;       /* 对应的特征帧序号，可与抓拍/时间轴对齐 */
    uint64_t     u64EventSeq = 0;         /* 事件唯一序号（聚合中心入库时分配，1 起）。
                                             即时事件通道（SPOOL_TYPE_EVENT）与周期报表会各带一份
                                             同一事件，平台按它去重——0 表示没进过环（没法去重） */
    int8_t       nExprClass = -1;         /* 表情类别（Expression_E，#88）。begin=开门那次判定的类别，
                                             end=最后维持事件的类别；非 112 列恒 -1。类别不进
                                             心跳/统计文本（PII 纪律同板书：只报计数不报内容） */
    bool         bSubjectCut = false;     /* 此 END 由主体/角色切换（仲裁翻转）产生，不是丢检换 ID。
                                             合并器据此**禁止**把它与随后的 BEGIN 并成一段：换身份是
                                             换了个人（教师交接），不是同一个人被跟踪器重发了 ID。
                                             人消失（retireGone）那条路径保持 false——那才是 C 形 */
} CBehaviorEvent_S;

/**
 * @brief 规则引擎观测（心跳 rules= 段与 statusJson 的数据源）
 */
typedef struct _RuleEngineStat_
{
    uint64_t u64Runs = 0;            /* 真正跑了判定的拍数（0 表示规则没被喂过帧） */
    uint64_t u64Samples = 0;         /* 采纳进时长门的 (人 × 行为) 次数 */
    uint64_t u64Throttled = 0;       /* 未到采样间隔被节流 */
    uint64_t u64ColumnOff = 0;       /* 这一列被配置关掉而整列没喂的 (人 × 行为) 次数（2026-10-23）。
                                        与 u64Throttled 分开的理由：默认配置下学生 10 列有 7 列关、
                                        教师还有手机列关，逐列照喂的话每拍每行都会给一个
                                        "未到采样间隔"的记账灌进去，于是 u64Throttled 的真实含义
                                        （节拍太快）被淹掉，读它的人会得出"该调采样间隔"的错误结论 */
    uint64_t u64True = 0;
    uint64_t u64False = 0;
    uint64_t u64Unknown = 0;
    uint64_t u64RegionReject = 0;    /* 因不在区域内被否掉的样本 */
    uint64_t u64RegionMiss = 0;      /* 引用的区域查不到 */
    uint64_t u64EventBegin = 0;
    uint64_t u64EventEnd = 0;
    uint64_t u64EventDrop = 0;       /* 缓冲满被丢的最旧事件。缓冲容量已是"一拍的产出上界"，
                                        所以非 0 只剩一个意思：下游连续几拍没取（取数在节拍线程
                                        的 pump 里），不再是"正常的大批量被误伤" */
    uint64_t u64Retired = 0;         /* 轨迹退役人数（时长门槽位回收） */
    uint64_t u64GateDeny = 0;        /* 时长门行池满到拒收喂入的次数：非 0 说明同时在门里的人数
                                        超过了行池容量（ROW_NUM）。正常跑一节课恒为 0，
                                        它和 u64Throttled 一起涨才是"人太多"而不是"节拍太快" */
    uint64_t u64StallClose = 0;      /* 因感知断档被强制收尾的批次数 */
    uint64_t u64PoseFrozen = 0;      /* 因「没有骨骼证据」被冻结（未喂门）的拍次数（#92）。
                                        持续上涨而 no_pose 不涨 = 调度空窗，事件不该被切；
                                        frz 涨过冻结上限后事件仍结束 = 姿态组真死，去查驻留 */
    uint64_t u64SubjectClose = 0;    /* 因归属翻转被提前收尾的事件数：换老师/认错人都会在这里现形，
                                        一节课里这条数远大于 promote+demote 之和就是判据在抖 */
    uint64_t au64Reason[static_cast<int>(SampleReason_E::SR_NUM)] = { 0 };
    /* 这是**单列**峰值（某一行为同时生效的人数），不是全班 25 列的同时生效总量。
       命名与语义差一个字，读者会把 3 读成"全班只有 3 个事件在跑"，而实际上可能是
       "每列各 3 个、合计几十个"。statJson 的键名 `active_max` 是平台契约（板上
       `metrics_cn.sh` 也读它），**不改键**——只把这里说清楚，并在心跳行同款标注。 */
    int32_t  nActiveMax = 0;
    int32_t  nTrackMax = 0;          /* 单拍见过的最大人数行数 */
    int64_t  llLastCostUs = 0;       /* 最近一拍规则判定耗时（微秒；按毫秒存会恒为 0，白占字段） */
    int32_t  nRegionNum = 0;         /* 区域表已建成数 */
    int32_t  nRegionConfigured = 0;  /* 配置里下发的区域条数：0/N 才是「换算全失败」，0/0 是没画区域 */
    int32_t  nUnjudged = 0;          /* 配置里开着、却没有任何采样器会喂的行为行数：
                                        非 0 就是「这条行为永远不会出事件」。它与 nRegionConfigured
                                        同一条读法——单独一个数没有意义，要对着配置那一行看 */
} CRuleEngineStat_S;

/**
 * @brief   : 规则引擎
 * @note    : 生命周期与引擎一致；不持有任何感知层对象，输入只有特征帧与配置快照
 */
class CRuleEngine
{
public:
    /**
     * @brief 在场人员的一小条记忆（跨帧留存，补发 END 事件时用）
     * @note  人消失、行为被关掉、感知断档这三条路径都要发 END，而发的时候那一帧早已过去：
     *        按帧取人头框会取不到，事件在平台侧就成了一幅没有位置的矩形。留最后已知位置
     *        不是精度问题，是完整性问题
     */
    struct CSeen_S
    {
        int32_t  nPersonId = -1;
        int32_t  nSeatId = -1;
        CRectf_S stHeadBox;
        /* 第一次发现「这一拍没有他」的时刻（P13 退役宽限）。0 = 本拍在场。
           只在 m_astGone 里非 0；m_astSeen / m_astSeenNext 里恒 0，免得两处口径打架 */
        int64_t  llMissingSinceMs = 0;
    };

    CRuleEngine()
    {
        /* 停留计时表构造期定容：运行期只 erase/push 不扩容（§1.2 红线）。
           这里 reserve 而不是等第一次 push_back，是为了「开机第一条站立」不落在分配器上 */
        m_vecStayTrack.reserve(static_cast<size_t>(CLASSROOM_MAX_PERSONS));
    }
    ~CRuleEngine() = default;

    CRuleEngine(const CRuleEngine &) = delete;
    CRuleEngine &operator=(const CRuleEngine &) = delete;

    /**
     * @brief   : 注册一路采样器（非持有，调用方保证其生命周期）
     * @return   {bool} 空指针、重复注册、超出 CLASSROOM_MAX_SAMPLERS 返回 false
     */
    bool addSampler(IBehaviorSampler *pSampler);

    /**
     * @brief   : 采样器全部注册完后收口一次（此后配置换版时自动复查「开着没人喂」）
     * @note    : 必须收口而不是每注册一路就查：两路采样器陆续注册的过程中，先到的那一路
     *            会把后到那一路的行为看成「没人喂」并喊出来——开机日志刷一堆假警报，
     *            而真出事时这条线正是靠它被看见。未收口时查也只算「还不知道」
     */
    void verifySamplerAssembly();

    /**
     * @brief   : 换入配置快照（区域表在下一拍按新快照重建，此处不碰几何）
     * @param    {int64_t} llNowMs：节拍线程单调时刻，只用于给「被关掉的行为」补发 END 事件
     * @note    : 不在此处重建区域是刻意的：换算需要 CFrameGeometry，而它只在特征帧里
     *            （总线未建或刚重建时拿不到），配置线程更没有。放在下一拍既拿到真几何，
     *            又保证「配置与几何同拍」——两条换入路径迟早分叉
     */
    void applyConfig(const ConfigPtr &spSnap, int64_t llNowMs);

    /**
     * @brief   : 跑一拍判定（每个节拍都调，无新帧时也要调——断档收尾靠的就是这一条）
     * @param    {const ConstFeatureFramePtr&} spFrame：最新特征帧，nullptr 表示还没有证据
     * @param    {int64_t} llNowMs：节拍线程的单调时刻，**只用于判「多久没来新证据」**
     * @note    : 两个时间基各管各的是刻意的：喂进时长门的时基取 spFrame->llMonotonicMs，
     *            这样同一批特征帧回放能复现出同一串事件（调参靠回放，不能靠运气）；
     *            断档判定却必须用真实时间，因为「感知停了」这件事在帧时间轴上根本不存在——
     *            最后一帧之后就没有帧了，任何按帧推进的逻辑都看不见这段空白。
     *            同一 u32Seq 的帧被重复传入时只喂一次（节拍 100ms、感知 200ms，多数拍没有新帧）
     */
    void run(const ConstFeatureFramePtr &spFrame, int64_t llNowMs);

    /**
     * @brief   : 取一条事件（最旧优先）
     * @return   {bool} false 表示缓冲空
     * @note    : 只在节拍线程被取走（P3 的聚合在同一拍里把它们转进自己的有界队列再上报）。
     *            事件环无锁，换线程取就等于把「满了丢最旧」这条唯一的保护改成竞态现场
     */
    bool pollEvent(CBehaviorEvent_S &stOut);

    /* 某人当前是否处于某行为（群体快照与 P4 的教师状态机都用它）：门表无锁，节拍线程专用 */
    bool isActive(int32_t nPersonId, Behavior_E enBehavior) const;

    /* 某人某行为最近一次判定是否有结论（TRUE/FALSE 都算）。群体时段门的分母用它，
       与 isActive 的区别见 CDurationGate::isKnown */
    bool isKnown(int32_t nPersonId, Behavior_E enBehavior) const;

    /* 某行为当前生效人数，供「行为人数/抬头率」类指标直接取，不必自己扫门表（同上，节拍线程） */
    int32_t activeNum(Behavior_E enBehavior) const;

    /* 区域表在 ensureRegions() 里被整表重建，因此也只有节拍线程能拿（P3/P4 的取数都在节拍线程） */
    const CRegionTable &regions() const { return m_stRegion; }

    /**
     * @brief   : 上一拍「低头」判定成立的人，供感知层下一拍做玩手机复核（同上，节拍线程专用）
     * @note    : 取的是**样本**而不是 isActive(104)：复核的依据是几何结论本身，与「低头那一行
     *            有没有配开关、够不够 3 秒」无关——关掉 104 就把 106 的复核也饿死，是两条不相干
     *            的配置互相绑架。判据留在规则层，感知层只消费这份名单，这样「谁知道低头」这件事
     *            仍然只有一处答案（§4.6 引擎不认识判据的那条红线）
     */
    const std::vector<int32_t> &phoneRecheckIds() const { return m_vecPhoneRecheck; }

    /* ---- 玩手机复核的两条判据----
       从 run() 里提出来成纯函数：它们是"为什么这一拍要为谁花一次 NPU"的全部依据，
       而 run() 要整台设备才跑得起来，提不出来就只能靠读代码——上一版把教师行漏掉
       正是这么发生的（注释写"205 不在本路"，代码里 205 偏偏在本路）。
       `phoneRecheckColumn`：哪些行为列成立时该给这个人排一次复核。
       学生 104 与教师 203 是同一个形状（低头）的两种主体，只认 104 就等于
       教师行永远拿不到复核结论、205 只能 fail-open。
       `phoneRecheckWanted`：这一拍到底要不要花这次 NPU（总开关 + 三个消费者任一开着）。 */
    static bool phoneRecheckColumn(Behavior_E enBehavior)
    {
        return Behavior_E::STU_HEAD_DOWN == enBehavior || Behavior_E::TEA_HEAD_DOWN == enBehavior;
    }

    static bool phoneRecheckWanted(bool bRecheckEnable, bool bStuPhone, bool bStuReading, bool bTeaPhone)
    {
        return bRecheckEnable && (bStuPhone || bStuReading || bTeaPhone);
    }

    /**
     * @brief   : 上一拍的姿态紧急度表（规则层 → 姿态调度的反馈通道，与 phoneRecheck 同哲学）
     * @return   {const CPoseUrgencyHint_S*} 表首指针，rnNum=OUT 有效条数；0 条表示没人等证据
     * @note    : 谁在等姿态，判定现场（样本成因 + 门状态 + 阈值参数）只有喂门那一行同时知道，
     *            所以在 feedOne 里就地记，而不是收尾再扫一遍门表——后者要么在门格上多存一份成因，
     *            要么拿「生效中」冒充「缺证据」，两种都把 25 列的账本变成 25 列的猜谜。
     *            引擎在 perceiveStep 把它抄进下一帧（astPoseUrgency），姿态挑选据此排序。
     *            节拍线程专用：perceiveStep（读）与 run（写）在同一次 tickOnce 的先后两步里，
     *            读到的永远是上一拍的完整表，没有并发
     */
    const CPoseUrgencyHint_S *poseUrgencyTable(int32_t &rnNum) const
    {
        rnNum = m_nPoseUrgNum;
        return m_astPoseUrg;
    }

    /**
     * @brief   : 归属仲裁的观测面（"谁是老师"这一层的读数）
     * @note    : 只给 const 引用：update()/reset() 都必须在节拍线程跟着 run() 走，
     *            放到外面调就等于让第二个线程改同一张槽表。而 statLine()/statJson()
     *            读的是仲裁器自己每拍末尾发布的快照，因此命令线程拿这个引用调它们是安全的
     */
    const CSubjectArbiter &subjects() const { return m_stSubject; }
    /**
     * @brief   : 本拍发生主体翻转的人（含没有事件可收的翻转）
     * @param    {int32_t&} rnNum：出参，条数
     * @return   {const int32_t*} 内部数组，调用方不得保存到下一拍
     * @note    : 合并器要用它把**已经挂在窗口里**的旧 END 也封掉。翻转与 END 可能差半拍
     *            （证据先断、身份后收），只靠 closeAndRetirePerson 打标会漏掉那半拍，
     *            合并器随后又把继任者的 BEGIN 并到旧人名下（P14 的第二条路径）
     */
    const int32_t *flippedIds(int32_t &rnNum) const
    {
        rnNum = m_nFlipIdNum;
        return m_anFlipIds;
    }


    /**
     * @brief   : 取观测读数（任意线程可调，读到的是上一拍末尾发布的快照，最多滞后一拍）
     */
    CRuleEngineStat_S stat() const;
    void resetStat();

    /**
     * @brief   : 讲台状态发布件（任意线程可调，语义同 stat()：最多滞后一拍）
     * @note    : bValid=false 表示还没有任何一拍跑过——平台不得把默认的 UNKNOWN 当读数
     */
    CClassPodiumState_S podiumState() const;

    /**
     * @brief   : 全表复位并给所有生效中的行为补一条结束事件
     * @note    : 内存降级到应急档、或总开关被关时调用。不补就留下「永不结束的事件」，
     *            平台侧时间轴会出现一条拉到底的站立/低头 —— 用户看到的正是这条，而不是内存曲线
     */
    void shutdownActive(int64_t llNowMs);

    /**
     * @brief   : 全量复位（引擎重新 init 时用），先收尾再抹环，最后把观测计数归零
     * @note    : 比 shutdownActive 多两件事，缺一件都会在下一次启动时冒出来：
     *            1) 抹掉事件环——收尾产出的那批 END 这一次没有读者，
     *               留着它们，新一轮的第一次取数会吐出一批上一轮的旧事件；
     *            2) 清掉区域重建的判据字段——重启后配置版本号会重新从 1 开始，
     *               与上一轮的版本号恰好相等时区域就不换了。
     *            只能在节拍线程未运行时调（与所有写侧方法同一线程域）
     */
    void reset(int64_t llNowMs);

    /* 观测一行文本（心跳用） */
    std::string statLine() const;
    /* statusJson 的 rules 段（一个 JSON 对象，含 per-behavior 生效人数） */
    std::string statJson() const;

private:
    /* 区域表是否需要按当前快照/几何重建 */
    void ensureRegions(const CFrameGeometry &stGeom);
    /**
     * @brief   : 喂一条班级级样本（哨兵行 CLASSROOM_CLASS_PERSON_ID）
     * @param    {bool} bSkipRegionGate：true=不再按 behaviors.<id>.region_ids 做逐行区域闸
     * @note    : 班级级列要跳过区域闸：哨兵行的人头框面积为 0，任何区域都会把它判成
     *            "框坏了"（SR_NO_FRAME）。它们的区域事实在聚合里已经用掉了——
     *            210 读的就是讲台角色的占用，207/209 问的是整帧
     */
    void feedOne(const CPersonFeature_S &stRow, Behavior_E enBehavior, const CBehaviorSample_S &stSample,
                 int64_t llNowMs, uint32_t u32Seq, bool bSkipRegionGate = false);
    /**
     * @brief   : 判「这一帧」的 207/209/210 并喂门
     * @note    : 三列的主体是整帧而不是某一行，所以走哨兵行；时长（指导/互动要「持续一定时间」）
     *            仍由 behaviors.207/209 的门参数承担，这里只出一拍三态
     */
    void feedClassLevel(const CClassFacts_S &stFacts, int64_t llFrameMs, uint32_t u32Seq);

    /**
     * @brief   : 把一行的站立/举手原判折算进 207/209 的班级事实
     * @param    {CClassFacts_S&} rstFacts：本拍累加器
     * @param    {CPersonFeature_S&} stRow：这一行（取头框做区域闸、取骨骼均分做置信门）
     * @param    {CBehaviorSample_S&} stStand/stRaise：101/102 的原判（采样器给的）
     * @param    {bool} bHaveStand/bHaveRaise：这一行是否真的拿到了该列样本
     * @param    {int64_t} llFrameMs：本拍帧时（stay_s 的连续计时）
     * @note     ：207 与 209 的区域、门限各不相同，必须各算一份（同一行可以在 207 成立、
     *            在 209 不成立）。三态聚合留给 feedClassLevel，这里只按行过滤与计数
     */
    void accumulateClassFacts(CClassFacts_S &rstFacts, const CPersonFeature_S &stRow,
                              const CBehaviorSample_S &stStand, bool bHaveStand,
                              const CBehaviorSample_S &stRaise, bool bHaveRaise,
                              int64_t llFrameMs);
    /* 把某人的姿态紧急度并进本拍表（同人多列取最大）：列间取最大而不是相加，
       因为列是同一个名额的竞争者不是证据叠加 */
    void mergePoseUrgency(int32_t nPersonId, int32_t nUrgency);
    void mergePoseUrgencyOf(const CPersonFeature_S &stRow, const CBehaviorSample_S &stFinal,
                            const CGateState_S &stBefore, int64_t llNowMs, Behavior_E enBehavior);
    void pushEvent(const CBehaviorEvent_S &stEvent);
    /* 给所有生效中的行为补结束事件，返回补了几条（0 条时不算一次断档收尾，不计数） */
    int32_t closeActiveAll(int64_t llNowMs);
    /**
     * @brief   : 给某人全部 25 列生效中的行为补 END 并回收其时长门槽位
     * @param    {const CSeen_S&} stAt：该人最后已知的座位与人头框（事件要带位置）
     * @return   {int32_t} 补发的条数
     * @note    : 人消失与身份翻转两条路径共用这一份实现：两者的收尾要求一模一样
     *            （先 END 再清表，反序会把退出事件吞掉），拆开就是两处会各自烂掉的重复。
     *            bSubjectCut 只影响事件上的一个标记：翻转收尾的 END 不许被合并器当成
     *            「同一段行为的换 ID 接手」（P14：教师交接时 212 挂在旧教师名下 25s）
     */
    int32_t closeAndRetirePerson(const CSeen_S &stAt, int64_t llNowMs, uint32_t u32Seq,
                                 bool bSubjectCut = false);
    /**
     * @brief   : 取空仲裁翻转环，给换了身份的那个人换一本行为账
     * @note    : 必须在逐行判定**之前**做完：翻转之后那一拍采样器就会按新身份出判定，
     *            先收尾才不会出现在同一拍里"学生的站立还挂着、教师的站立已经开了"的两本账并存
     */
    void applySubjectFlips(const CFeatureFrame_S *pstFrame, int64_t llNowMs, uint32_t u32Seq);
    /* 某人最后已知的座位与人头框：先查本拍行（位置更新），查不到退到上一拍的在场表 */
    void locatePerson(const CFeatureFrame_S *pstFrame, int32_t nPersonId, CSeen_S &rstOut) const;
    /**
     * @brief   : 某行为被关掉时给它名下所有生效中的事件收尾
     * @note    : 走「喂一次 bEnable=false 的样本」而不是直接清门表：门本来就实现了关闭语义
     *            （生效中则补一次退出），清表却会把这条退出吞掉，平台侧留下一条拉到底的事件
     */
    void disableColumn(int nCol, int64_t llNowMs);
    /**
     * @brief   : 喊出「配置里开着、但没有任何采样器会喂它」的行为行
     * @note    : 这种行不崩、不报错、日志里一切正常，唯一的现象是平台侧那条行为永远零事件，
     *            而现场第一反应是"画面里没人"。§6 的出厂开关口径要的是"让它从一开始就看到这一行是关的"，
     *            这里守的就是那条口径漏下来之后的口子（新增行为时忘了声明列）。
     *            只在集合变化时打：配置是 mtime 轮询热生效的，每次都喊会把这条真正该看见的线淹掉
     */
    void checkUnjudgedEnabled();
    void markSeen(int32_t nPersonId, int32_t nSeatId, const CRectf_S &stHeadBox);
    /* 上一拍在场、本拍不见了的人：回收其门表槽位并给生效中的行为补 END（不发就是永不结束的事件） */
    void retireGone(int64_t llFrameMs, uint32_t u32Seq);
    /* 把无锁工作计数一次性抄进发布件（每拍一次，是本类唯一的加锁点） */
    void publishStat();
    static const char *reasonName(SampleReason_E enReason);

private:
    ConfigPtr    m_spSnap;
    CRegionTable m_stRegion;
    CDurationGate m_stGate;
    /* 归属仲裁器在这里被持有而不是由 CClassroomEngine 持有：它的输入（区域表 + 特征帧 + 配置快照）
       三样都在本类的拍内、且与时长门必须同一个时基同一份快照。拆到上一层就会有两处节拍、
       两份快照，"这一拍的行为用的是哪一拍的归属"立刻变成没有答案的问题 */
    CSubjectArbiter m_stSubject;

    std::vector<IBehaviorSampler *> m_avecSampler;
    /* 采样器装配是否已收口（verifySamplerAssembly）：没收口前「没人喂」只说明还没问完 */
    bool m_bAssembled = false;
    /* 已注册采样器声明过的行为列（bit = 列号）。只服务一个判据：某行开着却没人喂它
       （见 checkUnjudgedEnabled）。装配期写、运行期只读，与 m_avecSampler 同一条纪律 */
    uint32_t m_u32Declared = 0;
    /* 最近一次算出的「开着但没人喂」列集合：checkUnjudgedEnabled 是唯一写它的人，
       publishStat 只取数量。算完就存是为了不把「读快照」这件事扩散到节拍线程之外 */
    uint32_t m_u32Unjudged = 0;
    /* 上一次已经喊过的集合：热更新每拍都可能换版，重复喊会把这条线淹掉 */
    uint32_t m_u32UnjudgedWarned = 0;
    static_assert(CDurationGate::BEHAVIOR_COLUMN_NUM <= 32,
                  "behavior columns no longer fit the uint32_t declared-bitmap");
    /* 与采样器声明表同长的样本缓冲：容量在 addSampler 时按各路最大长度一次给足，
       之后每次调用前只缩放到本路长度（不跨容量，运行期零分配） */
    std::vector<CBehaviorSample_S> m_vecSample;

    /* 上一拍低头成立的人（感知层玩手机复核的候选名单）：run() 里清空重填，容量稳定后不再分配 */
    std::vector<int32_t> m_vecPhoneRecheck;

    /* T39/T40 背身探针节流：TRUE 逐条、其余按 (判,成因) 变化沿 + 2s 心跳。
       变化沿是 ttea13 的教训——探针不打印的成因（SUBJECT/EDGE/OCCLUDED）造成整段静默，
       静默段正是归属 churn 的现场，成因不落日志就没法归因 */
    int64_t m_llTbProbeMs = 0;
    TriState_E m_enTbProbeLastState = TriState_E::TS_UNKNOWN;
    SampleReason_E m_enTbProbeLastReason = SampleReason_E::SR_NO_FRAME;

    /* 班级级三列（207/209/210）探针节流（2026-10-28）：这三列的行是哨兵 0，
       逐行探针（背身/低头复核）覆盖不到。下标按 {207,209,210} 固定，
       按 (判,成因) 变化沿 + 10s 心跳落一条——读不出「为什么不出」的列等于没有观测 */
    int64_t m_llClsProbeMs = 0;
    TriState_E m_aenClsProbeState[3] = { TriState_E::TS_UNKNOWN, TriState_E::TS_UNKNOWN,
                                         TriState_E::TS_UNKNOWN };
    SampleReason_E m_aenClsProbeReason[3] = { SampleReason_E::SR_NO_FRAME, SampleReason_E::SR_NO_FRAME,
                                              SampleReason_E::SR_NO_FRAME };

    /* 上一拍在等姿态证据的人（P0-1 接近激活阈值优先）：feedOne 就地合并最大值，
       run() 开头清零。定长数组而非 vector：条数被人数上限封顶，一拍最多 60 条，
       换来的是反馈路径上零分配、零加锁 */
    CPoseUrgencyHint_S m_astPoseUrg[CLASSROOM_MAX_PERSONS];
    int32_t m_nPoseUrgNum = 0;

    /* 上一拍在场的人：用于发现「这个 ID 从画面里消失了」并回收其时长门槽位，
       也是所有补发 END 事件路径的取数来源（人头框、座位号） */
    CSeen_S m_astSeen[CLASSROOM_MAX_PERSONS];
    int32_t m_nSeenNum = 0;
    /* 本拍在场的人，先写这里再整体换过去（边扫边改会让自己判自己丢） */
    CSeen_S m_astSeenNext[CLASSROOM_MAX_PERSONS];
    int32_t m_nSeenNextNum = 0;
    /* 宽限期内「这一拍没看见」的人（P13）：llMissingSinceMs 记第一次消失的时刻。
       与 m_astSeen 分开存是必须的——m_astSeen 每拍整体换成 m_astSeenNext，
       消失的人下一拍就不在里面了，宽限计时会跟着一起丢。 */
    CSeen_S m_astGone[CLASSROOM_MAX_PERSONS];
    int32_t m_anFlipIds[CLASSROOM_MAX_PERSONS] = { 0 };   /* 本拍翻转的人（P14，给合并器封口） */
    int32_t m_nFlipIdNum = 0;
    int32_t m_nGoneNum = 0;

    /* 本拍每一行的归属（下标 = vecPersons 下标，非 personId），逐拍整列覆写、容量编译期定死。
       存在的唯一理由：采样器入参带的是**本行**的归属事实，而讨论/演示这两条要看别人的那一行
       （见 CBehaviorSampleIn_S::penRowSubject）。只在本拍的人数行内有效，行外的值是上一拍的残留 */
    int8_t m_anRowSubject[CLASSROOM_MAX_PERSONS] = { 0 };

    /* 207 指导的「区域停留」连续计时（Web 14 页 stay_s）：
       按跟踪 ID 记「这一行连续站在 207 区域里」的起点。明确 FALSE（不站了/离开区域）当场删；
       UNKNOWN（没量到）不推进也不删；行本拍没出现的条目在行循环之后按 CLASSROOM_STAY_GAP_MS
       宽限清掉，不靠 TTL——帧时停摆时 TTL 永远不会过期，一个人走了十分钟再回来会被当成
       「一直站着」。容量按人数上限，满时换掉最久未出现的条目，运行期零分配 */
    typedef struct _StayTrack_
    {
        int32_t nPersonId = -1;
        int64_t llSinceMs = 0;
        int64_t llLastMs = 0;
    } CStayTrack_S;
    std::vector<CStayTrack_S> m_vecStayTrack;

    /* 每列行为的当前生效人数（由 bActivated/bDeactivated 增减，供 activeNum O(1) 取） */
    int32_t m_anActiveNum[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    /* 上一拍的同一份：交给采样器做群体条件（互动看"有没有学生在举手"）。
       不在 m_anActiveNum 上就地读的原因见 CBehaviorSampleIn_S::panActivePrev 的注释——
       逐行判定时这一列还在被本拍改写，读它等于把判定结果挂在行的先后上 */
    int32_t m_anActivePrev[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };

    CBehaviorEvent_S m_astEventRing[CLASSROOM_RULE_EVENT_RING];
    uint32_t m_u32EventHead = 0;
    uint32_t m_u32EventNum = 0;

    /* ---- 以下只在节拍线程读写，不加锁 ---- */
    uint32_t m_u32LastSeq = 0;       /* 上次真正喂过样的特征帧序号，0 表示还没喂过 */
    bool     m_bFedOnce = false;
    int64_t  m_llLastRunMs = 0;      /* 上次喂样的帧时间，断档判定的参照点 */
    int64_t  m_llLastRunWallMs = 0;  /* 上次喂样的墙钟时间，断档判定用（帧时与墙钟不同基，不能混减） */
    int64_t  m_llLastSubjectMs = -1; /* #174：上一份「画面里至少一个人」的帧时，-1 表示本轮还没见过 */
    int64_t  m_llRegionVersion = -1; /* 已采纳的区域对应的配置版本 */
    bool     m_bRegionGeomReady = false;
    int      m_nRegionSrcW = 0;
    int      m_nRegionSrcH = 0;
    int      m_nRegionDstW = 0;
    int      m_nRegionDstH = 0;
    int      m_nRegionRotate = -1;
    int      m_nRegionMirror = -1;

    CRuleEngineStat_S m_stStat;      /* 工作计数，写侧就是节拍线程 */
    /* 环形队列满那条 warn 的上次落日志时刻（帧时基）。
       旧写法是 `count % 1000 == 0`——它限的是"每 1000 次一条"，与**时间**无关：
       59 人重载时 1000 次丢弃几秒就走完（刷屏），课间空场时几千次要几小时（等于不记）。
       改成时间窗之后，速率与事件密度解耦，"异常持续时每秒最多一条"这句话才真的成立。
       节拍线程独占，不与发布件的锁发生关系。 */
    int64_t m_llDropLogMs = 0;

    /* 发布件：节拍线程每拍末尾抄一份，命令线程只读这里。
       单独一份而不是「读的时候加锁抄」，是因为读侧若在判定中途插进来，
       抄到的会是「事件已计入但样本还没计」这类自相矛盾的中间态 */
    mutable std::mutex m_mtxPub;
    CRuleEngineStat_S  m_stStatPub;
    int32_t m_anActiveNumPub[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    /* 210 讲台状态的发布件：工作份在 feedClassLevel 写（节拍线程），这里抄给命令线程 */
    CClassPodiumState_S m_stPodium;
    CClassPodiumState_S m_stPodiumPub;
};
}
