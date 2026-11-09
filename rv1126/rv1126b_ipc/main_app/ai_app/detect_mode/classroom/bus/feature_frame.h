/**
 * @FilePath     : feature_frame.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 17:02:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 17:02:00
 * @Description  : 特征帧与特征帧总线
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "classroom_define.h"
#include "frame_geometry.h"

namespace Classroom
{
/* 特征帧槽位数：4 槽 = 1 在读(latest) + 1 在写 + 2 给慢消费者，内存 = 4×人数×单行字节 */
#define CLASSROOM_FEATURE_SLOTS (4)

/* 特征帧超过该时长仍未更新，消费者必须当作「证据过期」处理 */
#define CLASSROOM_FEATURE_TTL_MS (2000)

/**
 * @brief 单个人的特征行
 */
typedef struct _PersonFeature_
{
    int32_t     nPersonId = -1;         /* 跟踪 ID，<0 表示该行无效 */
    Subject_E   enSubject = Subject_E::SUBJECT_UNKNOWN;
    int32_t     nSeatId = -1;           /* 座位先验编号（考勤/就坐率用），未分配为 -1 */
    int32_t     nTrackAgeFrames = 0;    /* 该轨迹已存活的感知帧数，新轨迹用它防抖 */

    CRectf_S    stHeadBox;              /* 人头框，分析帧坐标 */
    CRectf_S    stBodyBox;              /* 体框：由人头框外扩推得（见 fBodyExpand） */
    float       fHeadScore = 0.0f;

    /* ---- 姿态：模型输入是「人体框裁剪放大图」，点已换算回分析帧 ---- */
    CPoseSkeleton_S stSkeleton;         /* 26 点，bValid=false 时整帧不可用 */
    int64_t     llPoseMs = 0;           /* 该骨骼结果产出时刻（单调毫秒），用于跨帧缓存新鲜度 */

    /* ---- 人脸/表情：G_FACE 时间片内才有，缺证时以下字段一律不可信 ---- */
    CRectf_S    stFaceBox;              /* 人脸框，分析帧坐标 */
    float       fFaceScore = 0.0f;
    Expression_E enExpression = Expression_E::EXP_UNKNOWN;
    float       fExpressionScore = 0.0f;
    /* 情绪列的产出时刻（单调毫秒），0 = 从没量过 */
    int64_t     llExprMs = 0;
    int64_t     llFaceMs = 0;

    /* ---- 玩手机复核：G_OTHER 按需驻留且这一拍有人低头时才有，缺证时以下字段一律不可信 ---- */
    bool        bPhoneObj = false;      /* 人体框内检出「玩手机」目标框 */
    float       fPhoneScore = 0.0f;     /* 复核框最高置信度 */
    int64_t     llPhoneMs = 0;          /* 复核产出时刻（单调毫秒），0 = 从没复核过 */

    /* ---- 逐人的可判定性辅助（由感知层填写，规则层不再回头猜）---- */
    bool        bOccluded = false;      /* 与邻框 IoU 超门限 / 关键点大面积缺失 */
    bool        bOnBoundary = false;    /* 头中心贴边/出画：头已半出画，朝向类判定不可用 */
    float       fBlurScore = 0.0f;      /* 清晰度代理（人头框内梯度能量归一化） */

    /**
     * @brief   : 本行是否有可用的姿态证据
     * @param    {int64_t} llNowMs：单调时基
     * @param    {int64_t} llTtlMs：新鲜度窗口，取配置 sampling.pose_interval_ms 的同量级值
     */
    bool poseUsable(int64_t llNowMs, int64_t llTtlMs) const
    {
        if (!stSkeleton.bValid || llPoseMs <= 0)
        {
            return false;
        }
        return (llNowMs - llPoseMs) <= llTtlMs;
    }

    bool faceUsable(int64_t llNowMs, int64_t llTtlMs) const
    {
        if (llFaceMs <= 0 || stFaceBox.width() <= 0.0f)
        {
            return false;
        }
        return (llNowMs - llFaceMs) <= llTtlMs;
    }

    /**
     * @brief   : 本行是否有可用的玩手机复核结论
     */
    bool phoneUsable(int64_t llNowMs, int64_t llTtlMs) const
    {
        if (llPhoneMs <= 0)
        {
            return false;
        }
        return (llNowMs - llPhoneMs) <= llTtlMs;
    }
} CPersonFeature_S;

/* 姿态新鲜度窗口 = **一整圈** × 该倍数。取 2 是留一圈的余量：人数与每圈产能的比值会漂
   （有人在中间坐下/离开），只按一圈算会让窗口在"刚好够用"与"不够用"之间来回跳 */
#define CLASSROOM_POSE_TTL_ROUNDS (2)
#define CLASSROOM_POSE_TTL_MIN_MS (1500)
/* 上限必须存在：人数被截断到容量、节拍又被拉到很慢时，「过期」这件事会彻底消失 */
#define CLASSROOM_POSE_TTL_MAX_MS (30000)

/**
 * @brief   : 一整圈要多久（毫秒）——「这一屏每个人都过一次姿态」要多少时间
 * @param    {int32_t} nRoundMs：感知层的**实测**一轮周期（一拍多久）。0 = 还没量出来
 * @param    {int32_t} nPerRound：感知层的**实测**一轮可做人数。0 = 还没量出来
 * @param    {int32_t} nPersonNum：本帧人数
 * @return   {int64_t} ceil(人数 / 每轮人数) × 一轮周期；入参没量出来时返回 0
 */
int64_t poseCycleMs(int32_t nRoundMs, int32_t nPerRound, int32_t nPersonNum);

/**
 * @brief   : 骨骼证据的新鲜度窗口（毫秒）——本子系统里「这副骨骼还作不作数」的唯一口径
 * @return   {int32_t} 钳在 [CLASSROOM_POSE_TTL_MIN_MS, CLASSROOM_POSE_TTL_MAX_MS] 内；
 *            节拍还没量出来时直接给下限（给 0 等于"骨骼一产出就过期"，那是最窄不是最保守）
 */
int32_t poseEvidenceTtlMs(int32_t nRoundMs, int32_t nPerRound, int32_t nPersonNum);

/**
 * @brief   : 一圈乘完余量之后**装不进窗口上限** = 姿态产能撑不起这一屋子人
 * @return   {bool} true 时下游拿到的 TTL 已经小于实际轮转周期，圈尾的人会被判过期
 */
bool poseCapacityShort(int32_t nRoundMs, int32_t nPerRound, int32_t nPersonNum);

/**
 * @brief 姿态产能估计器：把「每隔多久做了几个人」的样本平滑成 poseEvidenceTtlMs 的两个入参
 */
typedef struct _PoseCadence_
{
    int64_t llLastWorkMs = 0;        /* 上一个「真的做了推理」的拍的时刻，0=还没有 */
    int64_t llRoundEwmaMs = 0;       /* 相邻两个实做拍的墙钟间隔 EWMA，0=样本不足一个 */
    int32_t nPerRoundEwma100 = 0;    /* 每个实做拍的人次 EWMA ×100：定标是为了让 1.4 与 1 有别
                                        （取整到 1 再喂回来就再也涨不动了）。读出去时向下取整，
                                        见 poseCadenceOf */
} CPoseCadence_S;

/**
 * @brief   : 喂一个实做拍样本
 * @param    {int32_t} nTried：本拍真的提交了几次姿态推理。<=0 直接忽略
 */
void poseCadenceFeed(CPoseCadence_S &stCadence, int32_t nTried, int64_t llNowMs);

/**
 * @brief   : 取出当前估计值，直接作 poseEvidenceTtlMs / poseCapacityShort 的前两个入参
 * @return   没量出来时两列都给 0（语义=未知，由 poseEvidenceTtlMs 退回下限）
 */
void poseCadenceOf(const CPoseCadence_S &stCadence, int32_t &rnRoundMs, int32_t &rnPerRound);

/**
 * @brief 节拍闸：**判定与记账分离**
 */
struct SBeatGate_S
{
    int64_t llLastMs = 0;

    bool due(int64_t llNowMs, int32_t nIntervalMs) const
    {
        const int64_t llInterval = (nIntervalMs > 0) ? nIntervalMs : 1;
        return !(llLastMs > 0 && llNowMs - llLastMs < llInterval);
    }

    void book(int64_t llNowMs) { llLastMs = llNowMs; }
};

/* ============================ 姿态紧急度提示（规则层 → 姿态调度） ============================ */

/* 紧急度标尺（千分制）。两层数值必须同源：规则层往上写、挑选层往下读，
   中间没有任何换算——谁私自定义一套数，排序就悄悄反过来 */
#define CLASSROOM_POSE_URG_EXPIRE   (500)   /* 证据临近过期：挑选层按帧上 TTL 就地算，提前续期 */
#define CLASSROOM_POSE_URG_TEACHER  (550)   /* 教师候选的姿态加急：候选（含弱证据坐上去的）
                                               身份靠姿态不断重认，丢了就是重认 5s + 行为列假分裂。
                                               排在普通过期之上、攒分之下——不压过真正在攒的 */
#define CLASSROOM_POSE_URG_ACC_BASE (600)   /* 未生效但在攒时长：刚起步的尝试 */
#define CLASSROOM_POSE_URG_ACC_SPAN (399)   /* 攒到阈值前一刻 = 600+399=999，仍低于 ACTIVE：
                                               事件断了可以重启，生效中的断了是假拆一次 */
#define CLASSROOM_POSE_URG_ACTIVE   (1000)  /* 生效中的行为缺证据：事件正靠容忍额度撑 */

/* 姿态年龄过 TTL 的这个千分比就算「快过期」。挑选层的 EXPIRE 抬档与规则层的教师加急
   必须读同一个数：各写一个 7/10，迟早有一天一边改了另一边不知道 */
#define CLASSROOM_POSE_URG_STALE_PERMILLE (700)

/**
 * @brief 一次姿态紧急度提示（规则层在喂门处就地算出，引擎抄进特征帧）
 */
typedef struct _PoseUrgencyHint_
{
    int32_t nPersonId = -1;
    int32_t nUrgency = 0;   /* [0,1000]，越大越该把下一个姿态名额给这个人 */
} CPoseUrgencyHint_S;

/**
 * @brief 一帧感知结果（只读快照）
 */
typedef struct _FeatureFrame_
{
    uint32_t u32Seq = 0;                /* 特征帧序号，单调递增 */
    int64_t  llMonotonicMs = 0;         /* 产出时刻（classroomNowMs 口径），时长门与时延看护用它 */
    uint64_t u64PtsUs = 0;              /* 对应像素帧的推流 PTS，仅用于对齐视频时间轴 */
    uint32_t u32PixelSeq = 0;           /* 对应像素帧序号（CFrameBus 的 u32Seq），可反查是哪一帧 */

    int32_t  nPersonNum = 0;            /* 有效行数，[0, CLASSROOM_MAX_PERSONS] */
    bool     bPersonTruncated = false;  /* 检测数超出容量被截断 —— 人数指标必须带此标记上报 */

    /* 本帧产出时两类重资源模型是否驻留。为 false 时相关行为必须判 TS_UNKNOWN，
       不得按「没发生」计数（时间片轮转的空窗不是证据）。
       G_OTHER（玩手机复核）不在这里：缺复核的出口是"没有否决权"而不是"判不出"，
       逐行的 llPhoneMs 才是它的证据有无，帧级再挂一个布尔只会多一处没人读的旗子 */
    bool     bPoseResident = false;
    bool     bFaceResident = false;

    /* 感知链路的**节拍事实**（#69-A）：这一帧是在"一拍多久、一拍做了几个人"的现场产出的。
       下游算骨骼新鲜度只能读这两个数——读配置里的 max_pose_persons 等于替感知层猜产能，
       猜宽的后果见 poseCycleMs 的说明。0 = 感知层还没量出来，此时 poseTtlMs 给下限 */
    int32_t  nPoseRoundMs = 0;
    int32_t  nPosePerRound = 0;

    /* 感知链路健康度快照 */
    int32_t  nHeadDetectNum = 0;        /* 人头检测原始框数（含被跟踪器丢弃的） */
    int64_t  llPerceptCostMs = 0;       /* 本帧感知总耗时 */

    /* 规则层上一拍的姿态紧急度表（引擎在 perceiveStep 抄入，随后整个感知调用内只读） */
    CPoseUrgencyHint_S astPoseUrgency[CLASSROOM_MAX_PERSONS];
    int32_t  nPoseUrgencyNum = 0;       /* 有效条数，[0, CLASSROOM_MAX_PERSONS] */

    CFrameGeometry stGeom;              /* 分析帧↔源帧变换，下游换算坐标的唯一来源 */

    /* 人数行，容量在 init() 定死后不再变化 */
    std::vector<CPersonFeature_S> vecPersons;

    /**
     * @brief   : 本帧的姿态证据窗口（毫秒）——五个消费点共用的唯一出口
     */
    int32_t poseTtlMs() const
    {
        return poseEvidenceTtlMs(nPoseRoundMs, nPosePerRound, nPersonNum);
    }

    /**
     * @brief   : 某人的姿态紧急度
     * @return   {int32_t} 没在表里 = 0（没人等他的证据），不是「不知道」
     */
    int32_t poseUrgencyOf(int32_t nPersonId) const
    {
        if (nPersonId < 0)
        {
            return 0;
        }
        for (int32_t i = 0; i < nPoseUrgencyNum && i < CLASSROOM_MAX_PERSONS; ++i)
        {
            if (astPoseUrgency[i].nPersonId == nPersonId)
            {
                return astPoseUrgency[i].nUrgency;
            }
        }
        return 0;
    }

    /**
     * @brief   : 整表抄入紧急度提示（引擎 perceiveStep 专用，超出容量截断）
     */
    void setPoseUrgency(const CPoseUrgencyHint_S *pstSrc, int32_t nNum)
    {
        nPoseUrgencyNum = 0;
        if (nullptr == pstSrc || nNum <= 0)
        {
            return;
        }
        if (nNum > CLASSROOM_MAX_PERSONS)
        {
            nNum = CLASSROOM_MAX_PERSONS;
        }
        for (int32_t i = 0; i < nNum; ++i)
        {
            astPoseUrgency[i] = pstSrc[i];
        }
        nPoseUrgencyNum = nNum;
    }

    void clear()
    {
        u32Seq = 0;
        llMonotonicMs = 0;
        u64PtsUs = 0;
        u32PixelSeq = 0;
        bPersonTruncated = false;
        bPoseResident = false;
        bFaceResident = false;
        nPoseRoundMs = 0;
        nPosePerRound = 0;
        nHeadDetectNum = 0;
        llPerceptCostMs = 0;
        nPoseUrgencyNum = 0;   /* 表内旧条目不清：读到的地方全部以条数为界，界外是死空间 */
        for (int32_t i = 0; i < nPersonNum && i < static_cast<int32_t>(vecPersons.size()); ++i)
        {
            vecPersons[i] = CPersonFeature_S();
        }
        nPersonNum = 0;
    }

    /**
     * @brief   : 帧是否仍然新鲜
     * @return   {bool} false 表示消费者必须按「证据不足」处理
     */
    bool isFresh(int64_t llNowMs, int64_t llTtlMs) const
    {
        if (llMonotonicMs <= 0 || nPersonNum < 0)
        {
            return false;
        }
        return (llNowMs - llMonotonicMs) <= llTtlMs;
    }

    /* 按跟踪 ID 找行；没有返回 nullptr。线性扫，人数上限 60，不做索引 */
    const CPersonFeature_S *findPerson(int32_t nPersonId) const
    {
        if (nPersonId < 0)
        {
            return nullptr;
        }
        for (int32_t i = 0; i < nPersonNum && i < static_cast<int32_t>(vecPersons.size()); ++i)
        {
            if (vecPersons[i].nPersonId == nPersonId)
            {
                return &vecPersons[i];
            }
        }
        return nullptr;
    }

    /**
     * @brief   : 预留 rnWant 行供感知线程填写（预留即占位）
     * @param    {int32_t&} rnWant：IN=想要的行数，OUT=实际可写的行数（已钳到容量）
     * @return   {CPersonFeature_S*} 首行指针，可直接下标写 [0, rnWant)
     */
    CPersonFeature_S *reservePersons(int32_t &rnWant)
    {
        const int32_t nCap = static_cast<int32_t>(vecPersons.size());
        const int32_t nOrig = rnWant;
        int32_t nWant = (rnWant < 0) ? 0 : rnWant;

        if (nWant > nCap)
        {
            /* 只想 0 行不算截断：那是调用方传参缺陷，伪装成容量问题会把调参引到错方向 */
            nWant = nCap;
            if (nOrig > nCap)
            {
                bPersonTruncated = true;
            }
        }
        rnWant = nWant;
        nPersonNum = nWant;
        return nCap > 0 ? vecPersons.data() : nullptr;
    }
} CFeatureFrame_S;

typedef std::shared_ptr<CFeatureFrame_S> FeatureFramePtr;
typedef std::shared_ptr<const CFeatureFrame_S> ConstFeatureFramePtr;

/**
 * @brief 特征总线观测计数
 */
typedef struct _FeatureBusStat_
{
    uint64_t u64Published = 0;
    uint64_t u64DroppedNoSlot = 0;      /* 无空闲槽：消费者太慢或槽数太少，与帧总线同一含义 */
    uint64_t u64RejectedArg = 0;
    uint64_t u64TruncatedFrames = 0;    /* 出现过人数截断的帧数，非 0 就要考虑调 CLASSROOM_MAX_PERSONS */
    uint32_t u32BusyMax = 0;            /* 同时被外部持有的槽位峰值 */
    uint32_t u32PersonPeak = 0;         /* 单帧最大人数，容量是否够用的实测依据 */
    uint32_t u32Slots = 0;
    uint32_t u32SlotBytes = 0;          /* 单槽字节数，init() 时算死并写日志 */
} CFeatureBusStat_S;

/**
 * @brief   : 特征帧生产者/消费者总线
 */
class CFeatureFrameBus
{
public:
    CFeatureFrameBus() = default;
    ~CFeatureFrameBus();

    CFeatureFrameBus(const CFeatureFrameBus &) = delete;
    CFeatureFrameBus &operator=(const CFeatureFrameBus &) = delete;

    /**
     * @brief   : 一次性分配全部槽位与人数行
     * @param    {int} nSlotNum：槽位数，<=0 取 CLASSROOM_FEATURE_SLOTS
     * @param    {int} nMaxPersons：单帧容量，<=0 取 CLASSROOM_MAX_PERSONS
     * @return   {bool} 参数非法或内存不足返回 false（512MB 机器上必须能观测到失败）
     */
    bool init(int nSlotNum, int nMaxPersons);
    void deinit();
    bool isReady() const;

    /**
     * @brief   : 借一个可写槽（头字段已清零，人数行内容为空）
     * @return   {FeatureFramePtr} 无空闲返回 nullptr，调用方本帧直接放弃（不得排队等）
     */
    FeatureFramePtr acquire();

    /**
     * @brief   : 发布一帧并把调用方句柄置空
     * @param    {FeatureFramePtr&} rspFrame：IN=acquire() 借出的句柄，OUT=空
     * @return   {bool} false 表示句柄不属于本总线（张冠李戴或已发布过），此时不动 latest
     */
    bool publish(FeatureFramePtr &rspFrame);

    /**
     * @brief   : 取当前最新特征帧
     * @return   {ConstFeatureFramePtr} 从未发布过返回 nullptr（上层按证据不足处理）
     */
    ConstFeatureFramePtr latest() const;

    int32_t capacity() const;
    CFeatureBusStat_S stat() const;
    void resetStat();

private:
    /* 槽位：内容 + 用于反查归属的裸地址（publish 用它判「这帧是不是我借出去的」） */
    struct Slot
    {
        CFeatureFrame_S stFrame;
    };

    int findFreeSlotLocked() const;
    void updateBusyLocked();

private:
    mutable std::mutex m_mtx;
    std::vector<std::shared_ptr<Slot>> m_apSlot;
    int m_nLatest = -1;
    int m_nCapacity = 0;
    uint32_t m_u32SeqGen = 0;
    bool m_bReady = false;
    CFeatureBusStat_S m_stStat;
};
}
