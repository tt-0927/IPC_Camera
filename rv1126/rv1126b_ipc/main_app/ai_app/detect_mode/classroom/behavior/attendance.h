/**
 * @FilePath     : attendance.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 14:13:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 14:26:00
 * @Description  : 课堂考勤账本
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"
#include "classroom_face_port.h"

namespace Classroom
{
/* 姓名字节上限（UTF-8） */
#define CLASSROOM_ATTEND_NAME_MAX (48)

/* 人脸特征维度（FaceFeature 模型的输出长度） */
#define CLASSROOM_FACE_FEATURE_DIM (512)

/**
 * @brief 缺件时注入的空底库
 */
class CNullFaceGallery : public IFaceGallery
{
public:
    bool match(const float *pafFeat, int32_t nDim, int64_t llNowMs, CFaceMatch_S &rstOut) override;
    bool available() const override { return false; }
    int32_t enrollNum() const override { return -1; }
    const char *name() const override { return "none"; }
};

/**
 * @brief 考勤参数
 */
typedef struct _AttendParam_
{
    bool    bEnable = true;
    /* 累计在场多久算到课（毫秒） */
    int64_t llPresentMinMs = 5000;
    /* 单次观测最多补计多少毫秒的在场时长 */
    int64_t llGapMaxMs = 2000;
    /* 人脸框短边下限（像素）*/
    int32_t nMinFacePx = 120;
    /* 远带下沿（像素，短边）*/
    int32_t nFaceFarMinPx = 48;
    /* 远带比对分下限 */
    float   fFarMatchScore = 0.85f;
    /* 远带命名票数下限（连续同 personId，与近带同一支投票表） */
    int32_t nFarMinHits = 3;
    /* 命名需要的连续命中次数 */
    int32_t nMinMatchHits = 2;
    /* 比对相似度下限。数值随模型与归一化习惯漂移，所以必须可配；交付时以模型侧标定为准 */
    float   fMinMatchScore = 0.75f;
    /* 未命名行空出多久可被回收（毫秒） */
    int64_t llRowIdleMs = 15000;
    /* 应到人数。<0 = 平台没下发 */
    int32_t nExpectedNum = -1;
    /* 考勤检测区域 */
    std::vector<int32_t> vecRegionIds;
} CAttendParam_S;

/**
 * @brief 一拍里某个人的考勤观测（由引擎从特征行 + 比对结果拼出来）
 */
/* ---- 同位置重入合并的判据常数 ----
   新跟踪 ID 与一条"刚丢不久"的未命名行在位置/尺度上对得上时，认作同一个人：
   位置半径取 1.2 倍头框高度——同一人低头/遮挡后抬头，中心位移通常小于半个头；
   相邻两座的中心距约 2 个头宽（≈1.5 个头高），1.2 是两者之间的分界。
   时间窗复用 llRowIdleMs（15s）：超过它，原行本来就允许被回收。
   最小间隔 1ms：**同一帧**里两个人都可见时，两行的末次时刻相同（llGap=0），不该被并；
   而下一帧（感知节拍 ~300ms）的新 ID 完全可能就是刚丢的那个人，不能被 400ms 挡掉——
   那等于把"隔一拍重连"这条最常见的路径关死。 */
#define CLASSROOM_ATTEND_REENTRY_RADIUS_HEADS (1.2f)
#define CLASSROOM_ATTEND_REENTRY_MIN_GAP_MS (1)
#define CLASSROOM_ATTEND_REENTRY_SCALE_MIN (0.60f)
#define CLASSROOM_ATTEND_REENTRY_SCALE_MAX (1.67f)

/* 到课门限的下界 */
#define CLASSROOM_ATTEND_PRESENT_MIN_BEAT_MS (200)

typedef struct _AttendObs_
{
    int32_t nTrackId = -1;            /* <0 非法，本条观测被拒 */
    int32_t nSeatId = -1;             /* 座位先验，未分配 -1（照抄特征行，本类不自己推） */
    int64_t llFrameMs = 0;            /* 单调时基，与特征帧同源 */
    int32_t nFaceW = 0;               /* 人脸框宽（分析帧像素），没检到脸填 0 */
    int32_t nFaceH = 0;
    /**
     * 本拍是否**真的做过**一次比对。三个"没做过"合在这一位上：G_FACE 空窗、
     * 这拍没检出脸、引擎本轮没排到他（人脸与姿态一样是按人轮询的）。
     * 刻意不叫 bHasFace——账本要区分的是"有没有比过"，而不是"有没有脸"
     */
    bool    bMatchTried = false;
    CFaceMatch_S stMatch;             /* bMatchTried=false 时整段不可信，本类不读 */

    /* ---- 位置证据 ---- */
    float   fNormX = -1.0f;
    float   fNormY = -1.0f;
    float   fHeadNorm = -1.0f;

    /* 本拍画面里有几个人头 */
    int32_t nFramePersons = 0;

} CAttendObs_S;

/**
 * @brief 一条考勤记录（名单快照里的一行）
 */
typedef struct _AttendRec_
{
    std::string strKey;               /* "P<personId>" 或 "T<trackId>"，集合去重用的就是它 */
    std::string strPersonId;          /* 未命名为空 */
    std::string strName;              /* 底库没给姓名时可为空，此时只报 ID */
    int32_t     nTrackId = -1;        /* 最后一次观测到这个人时用的跟踪 ID（合并后取最新） */
    int32_t     nSeatId = -1;
    int64_t     llFirstMs = 0;        /* 首次进画面 */
    int64_t     llLastMs = 0;         /* 最后一次进画面 */
    int64_t     llSeenMs = 0;         /* 累计在场时长（增量累加，非首尾差） */
    uint32_t    nSampleNum = 0;       /* 被观测到多少拍 */
    uint32_t    nMatchNum = 0;        /* 比对命中多少拍（含未跨门的） */
    float       fBestScore = 0.0f;    /* 历史最高相似度：误识投诉要能回查到当初的分 */
    bool        bCounted = false;     /* 已过 llPresentMinMs，计入到课人数 */
    bool        bNamed = false;       /* 已绑定身份（一旦为 true 不会再变 false） */
    /* 本行最后一次被计入“本拍在场人数”的拍号 */
    int64_t     llCountBeatMs = 0;

    /* ---- 末次位置证据 */
    float       fLastX = -1.0f;
    float       fLastY = -1.0f;
    float       fLastHeadNorm = -1.0f;
} CAttendRec_S;

/**
 * @brief 考勤账本计数（观测出口：回答"这个人为什么不在名单里"）
 */
typedef struct _AttendStat_
{
    uint64_t u64Obs = 0;              /* 收到多少条观测 */
    uint64_t u64RejectObs = 0;        /* 观测本身非法（trackId<0 / 时刻倒退） */
    uint64_t u64FaceSmall = 0;        /* 人脸尺寸不过门（短边 < face_far_min_px） */
    /**
     * 远带 effectiveness 的一对读数：try=远带的脸真的走到比对门
     * （旧形态里它们死在尺寸门，这一步根本不会发生），hit=过了远带分门投了票。
     * hit÷try 就是「face_far_match_score 定几」在机上唯一的定值依据——
     * 持续为 0 说明 0.85 对这颗模型的小脸太严，盲区换不来名字，门值就该往下调；
     * try 本身是 0 则说明盲区根本没进比对（先查人脸检测/特征段）
     */
    uint64_t u64FaceFarTry = 0;
    uint64_t u64FaceFarHit = 0;
    uint64_t u64NoFace = 0;           /* 这拍这行**没有可信人脸**（G_FACE 空窗、或压根没检出脸） */
    /**
     * 有脸但这轮没去比。与上一行必须分开：人脸列接上之后，"名单没名字"的两种原因
     * 长得完全不一样——u64NoFace 高说明人脸检测那段没出东西（去看 face try/det 与组轮转），
     * u64NoMatch 高说明脸有、是比对没做（底库没交付、或本轮没排到这个人）。
     * 合成一个数就等于让现场两种都查一遍
     */
    uint64_t u64NoMatch = 0;
    uint64_t u64MatchHit = 0;
    uint64_t u64MatchMiss = 0;        /* 做过比对而没上：底库没这个人 / 分数不够 */
    uint64_t u64ScoreLow = 0;         /* 命中了但分数低于 fMinMatchScore */
    uint64_t u64Named = 0;            /* 命名发生次数（含换绑） */
    uint64_t u64Merge = 0;            /* 同身份两条记录合并次数：人数虚高的直接读数 */
    /* 由**座位证据**促成的合并（mergeSamePerson 真正吃掉一次配对的次数）*/
    uint64_t u64SeatMerge = 0;
    /* 同位置重入合并的次数 */
    uint64_t u64ReentryMerge = 0;
    /* 座位复用改绑次数：同一座位号上来了新跟踪 ID，账本把旧行改绑给新 ID 的次数*/
    uint64_t u64SeatRebind = 0;
    /* 座位漂移观测数：「行上已有号、这条观测另带一个不同的号」的次数 */
    uint64_t u64SeatDrift = 0;
    /* ---- 本拍去重三道，同座位，同位置, 人头上限 */
    uint64_t u64DupSeat = 0;
    uint64_t u64DupPos = 0;
    uint64_t u64DupHead = 0;
    uint64_t u64RowFull = 0;          /* 表满丢人：CLASSROOM_MAX_PERSONS 之外的读数 */
    uint64_t u64Recycle = 0;          /* 未命名空行被回收的次数 */
    uint64_t u64GalleryAbsent = 0;    /* 底库不可用时收到的比对请求数 */
    uint64_t u64NameTruncated = 0;    /* 姓名超 CLASSROOM_ATTEND_NAME_MAX 被截 */
} CAttendStat_S;

/**
 * @brief 一次报表要带的考勤快照（会话级，由引擎在组包时填）
 */
typedef struct _AttendSnap_
{
    bool    bEnable = false;
    bool    bClosed = false;          /* true=本节已收尾（收尾后这份名单就是终值） */
    int32_t nPresentNum = -1;         /* 到课人数 */
    int32_t nNamedNum = -1;           /* 其中已识别身份的 */
    int32_t nExpectedNum = -1;        /* 应到人数（，来源见 expectedNum() 的三级优先 */
    /**
     * 差异：>0 缺勤，<0 超员（旁听、走错教室、以及人数被 ID 抖动撑高）。
     * 必须配 bAbsentKnown 一起读：-1 在这里是"超员一人"这个**真值**，
     * 而账本的 absentNum() 也拿 -1 当"应到未知"的哨兵——同一个数当两件事用，
     * 报文里就分不清"超员 1 人"与"不知道应到几个"。所以这份快照另带一个已知标志，
     * 而不是把 absent 也塞进"负数即未知"那一档（那等于把超员这条线索删掉）
     */
    int32_t nAbsentNum = -1;
    bool    bAbsentKnown = false;
    std::vector<CAttendRec_S> vecRoster;   /* 已命名名单，按首次进画面顺序（容量上界 = 人数上限） */
} CAttendSnap_S;

/**
 * @brief   : 参数自检
 * @return   {bool} false 表示这套值不能用；rstrErr 是给运维看的一句话（**不回显配置内容**）
 */
bool attendanceParamOk(const CAttendParam_S &rstParam, std::string &rstrErr);

/**
 * @brief 考勤账本
 */
class CAttendanceLedger
{
public:
    CAttendanceLedger();

    /**
     * @brief   : 换参数。名单**不因为改门限而清空**（与表情 EMA 同一口径）
     */
    void setParam(const CAttendParam_S &rstParam);
    const CAttendParam_S &param() const { return m_stParam; }

    /* 底库只用于 available()/enrollNum() 两个常数时间的查询；match() 由引擎调 */
    void setGallery(IFaceGallery *pGallery);
    bool galleryAvailable() const;

    /**
     * @brief   : 投喂一个人的一拍观测
     * @return   {bool} true = 这条观测落到了某一行上；false = 被拒（已收尾 / 未启用 /
     *                  观测本身非法 / 表满且无可回收行）。**不要拿它当"报表要不要重发"的信号**：
     *                  时刻倒退那一类观测被接受而什么都没改，账本也不留改动痕迹
     */
    bool observe(const CAttendObs_S &rstObs);

    /**
     * @brief   : 上游出示"这两个跟踪 ID 是同一个人"的证据，把两行并成一行
     * @param    {int32_t} nTrackOld 证据里先被看着的那一头（座位上挂着的旧 ID）
     * @param    {int32_t} nTrackNew 证据里新出现的那一头（遮挡重连后的新 ID）
     * @return   {bool} true = 本拍真的并掉一行；false = 什么都没改（原因见下）
     */
    bool mergeSamePerson(int32_t nTrackOld, int32_t nTrackNew);

    /* 到课人数（已过时长门的全部记录，含未命名的） */
    int32_t presentNum() const;
    /* 其中已命名的条数：与 presentNum 的差就是"来了但没认出"的人数 */
    int32_t namedNum() const;
    /**
     * @brief   : 应到人数
     * @return   {int32_t} 未知返回 -1。三级来源按权威高低取：课表（beginSession 下发）
     *            > 配置 nExpectedNum > 底库人数。三档都可能没有，返回 -1 让上层上报
     *            "无应到值"而不是猜一个——应到数错了整节课的出勤率都是错的
     */
    int32_t expectedNum() const;
    /**
     * @brief   : 应到与实到的差异（CSV 26 要的就是这个差异值，不是逐人状态）
     * @return   {int32_t} 应到未知返回 -1（**不返回 0**：0 会被平台读成"一个没缺"，
     *            而实情是设备不知道应到几个）；否则返回 应到 − 实到，
     *            >0 是缺勤，<0 是超员
     */
    int32_t absentNum() const;

    /**
     * @brief   : 名单快照：已命名**且已计入到课**的记录
     */
    int32_t roster(std::vector<CAttendRec_S> &rvecOut) const;
    /* 全部在册记录（含未命名、含没过时长门的）：人数对不上时现场要看的正是"那几条没名字的长什么样" */
    int32_t records(std::vector<CAttendRec_S> &rvecOut) const;

    /* 一节课开始：清账并记下应到人数（平台随课表下发；<0 = 不下发，见 expectedNum 的三级来源） */
    void beginSession(int32_t nExpectedNum);
    /**
     * @brief   : 一节课收尾：**冻结**账本，之后的观测一律丢掉并计入 u64RejectObs
     */
    void closeSession(int64_t llNowMs);
    void reset();

    CAttendStat_S stat() const { return m_stStat; }
    int32_t rowInUse() const;
    /* 单行读数，供心跳/状态串使用。刻意不带姓名（心跳里塞中文姓名既浪费又可能带出学生信息） */
    std::string statText() const;

    /**
     * @brief   : 取一份组报用的考勤快照（四个数 + 名单）
     * @param    {CAttendSnap_S&} rsnapOut：OUT=整份覆盖，函数内先清名单再填
     */
    void snapshot(CAttendSnap_S &rsnapOut) const;

private:
    /* 找行：先按跟踪 ID（本拍的观测都是跟踪 ID 口径），再看这个跟踪是否已被并入身份行 */
    int32_t findRowByTrack(int32_t nTrackId) const;
    int32_t findRowByKey(const std::string &strKey) const;
    /* 为新跟踪 ID 找一格：座位复用 → 空槽 → 回收最久的「既未命名也未计到课」的行 → 满 */
    int32_t allocRow(int32_t nTrackId, int32_t nSeatId, int64_t llNowMs,
                     float fNormX = -1.0f, float fNormY = -1.0f, float fHeadNorm = -1.0f);
    /* 命名落定：把 strKey 从 T<track> 换成 P<personId>，同身份已有行则合并 */
    void bindIdentity(int32_t nRow, const CFaceMatch_S &stMatch);
    /* 把 nFrom 并进 nTo：时长取和、首次取早、末次取晚，被并的行清空 */
    void mergeRow(int32_t nTo, int32_t nFrom);
    void clearRow(int32_t nRow);
    /* 到课去重：这一行是否已被**本拍**另一条在场且到课的行代表 */
    bool isDuplicateCounted(const CAttendRec_S &stRow, const CAttendObs_S &stObs);
    /* 一拍收尾：把本拍的人数（受人头数上限约束）并进峰值。换拍时由 rollBeat 调 */
    void finishBeat();
    void rollBeat(int64_t llFrameMs, int32_t nFramePersons);

private:
    /* 行表：构造期定容 CLASSROOM_MAX_PERSONS，此后不再改容量。
       姓名/ID 用 std::string 是这一条上唯一的松动：每节课每人最多各写一次，
       不在每拍路径上，60 行的上限也让它不可能长成堆碎片 */
    std::vector<CAttendRec_S> m_vecRow;
    /* 每行的"投票"（够 nMinMatchHits 次才落定）。与记录分开放是因为它会在两次命中之间被推翻 */
    std::vector<std::string> m_vecVoteId;
    std::vector<int32_t> m_vecVoteNum;

    CAttendParam_S m_stParam;
    CAttendStat_S m_stStat;
    IFaceGallery *m_pGallery;
    /* 课表下发的应到数。刻意不写进 m_stParam：配置热更会把这一行的值冲掉，
       而"这节课应到 42 人"与配置文件里那个默认值是两件事，混在一格就会出现
       改一次配置、全班的出勤率跟着跳 */
    int32_t m_nSessionExpected;
    bool    m_bClosed;                /* 已收尾：迟到观测一律丢掉并计数，见 closeSession */
    int64_t m_llCloseMs;

    /* ---- 到课人数 = “同时在场人数”的峰值---- */
    int64_t m_llBeatMs = 0;           /* 当前拍号（观测的帧时）；0 = 还没有过观测 */
    int32_t m_nBeatCounted = 0;       /* 本拍已计入的“在场且到课”人数 */
    int32_t m_nBeatPersons = 0;       /* 本拍人头数上限（观测里带的最大值；0 = 不知道） */
    int32_t m_nPresentPeak = 0;       /* 已完成拍里的最大同时在场人数 */
};
}
