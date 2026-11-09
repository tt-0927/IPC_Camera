/**
 * @FilePath     : subject_arbiter.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 14:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 14:10:00
 * @Description  : 主体归属仲裁
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "classroom_config.h"
#include "classroom_define.h"
#include "feature_frame.h"
#include "region.h"

namespace Classroom
{
/* 归属翻转的待取条数上限：一拍里最多翻转 CLASSROOM_MAX_PERSONS 个，但真实形态是一次换人
   （一进一出两条）。取 8 是留够"重启后整班同时换"这种极端场景，取不到的那条不计数即隐形成谜
   ——故 pushFlip 在环满时 ++nFlipDrop 并挂进 statJson。丢的只是**通知**：身份已经改了，
   只是上层收不到那一条 END（这条只能确定性构造，一拍退 9 人，见 classroom_subject_test） */
#define CLASSROOM_SUBJECT_FLIP_RING (8)

/**
 * @brief 一个人本拍的归属事实（仲裁结果 + 它依据的两条区域事实与位移）
 * @note  单独给一个结构而不是让调用方回头问仲裁器五个方法：教师采样器要一次拿全，
 *        而"问五个方法"里任何一个漏问，都会在两个判据之间留下不一致的口径
 *
 *        ★ 本类只发布两种值：TEACHER（证据攒够）与 UNKNOWN（其余全部），**从不发布 STUDENT**。
 *        "不是老师"是排除法得到的结论，而排除法的输出很容易被下游当成"确认是学生"去用
 *        （P5 的考勤就要区分：确认的学生进名单，不知道的人不进）。需要学生身份的地方
 *        由调用方按"TEACHER 之外即学生"自行解读，这与 P2/P3 已有口径一致
 *        （student_behavior 只挡 TEACHER，group_stats 的 bStudent 同理）
 */
typedef struct _SubjectFact_
{
    Subject_E enSubject = Subject_E::SUBJECT_UNKNOWN;
    bool bInPodium = false;          /* 头框落在讲台角色区域内 */
    bool bInBoard = false;           /* 头框落在黑板角色区域内 */
    bool bInTeacherArea = false;     /* 上二者任一成立，归属用的就是这个 OR */
    bool bInStudentArea = false;     /* 学生席角色区域；没画该角色时按「讲台/黑板之外」处理 */
    bool bStanding = false;          /* 仲裁用的站立。bStandKnown=false 时不可信 */
    bool bStandKnown = false;
    bool bMoved = false;             /* 位移窗口内累计位移达 teacher.patrol_move_px */
    bool bMovedHold = false;         /* 走动保持：最近一次达门后 CLASSROOM_PATROL_HOLD_TIMES 个窗口内
                                        都算"在巡视"（只给 210 用；207 指导仍看瞬时 bMoved）。
                                        真实巡视是"走几步、在桌边停几秒、再走"，瞬时 5s 窗会把
                                        每次停驻都判成"没走"，5s 时长门永远攒不满（student3 实测
                                        T 段最长 4s、F 段 4~6s，confirm 降到 2s 也一样） */
    double dfMovePx = 0.0;           /* 窗口内累计位移（分析帧像素），给日志与调参看 */
} CSubjectFact_S;

/**
 * @brief 本拍的归属上下文（帧级，与逐人事实配套）
 * @note  为什么单独给一块而不是让采样器自己去查区域表：「讲台空不空」「这两个角色画没画」
 *        与仲裁用的是同一次遍历，采样器再查一边就是第二处换算与第二套覆盖门限——
 *        两处会在边界上分叉，表现是「仲裁认他是巡视的老师，巡视行为却不成立」。
 *
 *        ★ 三条 configured 与两条 empty 必须分开看，它们回答的是两个不同的问题：
 *        empty=false 可能是「确实有人站在讲台」，也可能是「讲台根本没画」。
 *        前者是证据（巡视判据应当为 FALSE），后者是缺配置（只能判 UNKNOWN）。
 *        合并成一个布尔就会把"没配"写成"没发生"——与 CRegionTable 空表语义同一类坑
 */
typedef struct _SubjectView_
{
    bool bPodiumConfigured = false;    /* 讲台角色引用了至少一块已建成的区域 */
    bool bBoardConfigured = false;     /* 黑板角色同上 */
    bool bStudentConfigured = false;   /* 学生席角色同上（没画时 bInStudentArea 是兜底口径） */
    bool bTeacherAreaKnown = false;    /* 讲台与黑板至少画了一个：empty 那两条才有意义 */
    bool bPodiumEmpty = false;         /* 讲台已画且本拍无人 */
    bool bTeacherAreaEmpty = false;    /* 已画的角色里本拍都没人（指导判据用这条） */
} CSubjectView_S;

/**
 * @brief 一次归属翻转（供规则引擎给被弃用的那一族行为列补发 END）
 */
typedef struct _SubjectFlip_
{
    int32_t   nPersonId = -1;
    Subject_E enFrom = Subject_E::SUBJECT_UNKNOWN;
    Subject_E enTo = Subject_E::SUBJECT_UNKNOWN;
    int64_t   llMs = 0;            /* 翻转发生的帧时刻 */
} CSubjectFlip_S;

/**
 * @brief 仲裁观测
 * @note  bPodiumConfigured / bStudentRoleConfigured 是本类最重要的两条读数：
 *        教师行为整段不出时，先看的不是阈值而是这两条——区域角色没下发时，
 *        下面所有计数都会**安静地**停在 0，看起来像"没人当老师"
 */
typedef struct _SubjectArbiterStat_
{
    uint64_t u64Updates = 0;       /* 真正跑过仲裁的拍数 */
    uint64_t u64Promote = 0;       /* 慢轨：攒满 confirm_ms 成为教师的次数。
                                      promote 与 reentry 互斥（快轨上位只记 reentry），
                                      两者相加才是一次身份上位的总数；demote 覆盖两条轨 */
    uint64_t u64Demote = 0;        /* 放弃教师身份的次数（含快轨幽灵交接的退位）。
                                      promote+reentry 长期大于 demote 说明教师在变多，
                                      两者长期不等说明在反复换手 */
    uint64_t u64Reentry = 0;       /* T44：重入快轨继承身份的次数。恒 0 而现场确有
                                      「教师槽收槽后新 ID 5s 转正」时，先查 reentry_ms
                                      是否被钳掉、继任是否带讲台/黑板强证据、头框离
                                      记忆位置是否超 reentry_px——三关任一没过都不继承 */
    uint64_t u64SlotSteal = 0;     /* 槽池满到要挤掉别人才坐下：本拍未出现的槽就地回收，
                                      所以正常恒为 0；非 0 说明行里出现了超过容量的人。
                                      被挤掉的那个人若正挂着教师身份，这条要连着一条 flip +
                                      demote 计数——他人就在本拍行里，引擎"人不见了"那条
                                      收尾循环不会为他触发（#84 的 D2） */
    uint64_t u64MoveMemEvict = 0;  /* 位移记忆表全在期仍要写入、只能淘汰一条活记忆的次数
                                      （#158 第 24 轮）。0 = 表够用或淘汰的都是过期格；
                                      非 0 说明满座高 churn 下位移连续性被打断过，
                                      与 210 巡视段被切在一起看（每淘汰一次 ≈ 一个人丢
                                      最多 MOVE_MEM_TTL_MS 的累计位移） */
    uint64_t u64StandBlocked = 0;  /* 因没有骨骼证据而放弃用站立做判据的人数·次。
                                      四个 stand_blk_* 子计数是它的分解（T25）：同一人同拍只记
                                      先命中的一挡，四项之和 == u64StandBlocked。no_pose 包含
                                      姿态组没驻留与证据过期两种；blur 只在前面三挡都没接住时记 */
    uint64_t u64StandBlkNoPose = 0;  /* 姿态组没驻留 / 该行骨骼证据过期 */
    uint64_t u64StandBlkOccl = 0;    /* 行被 IoU 判遮挡 */
    uint64_t u64StandBlkEdge = 0;    /* 行贴画面边（骨骼会被切一半） */
    uint64_t u64StandBlkBlur = 0;    /* 头框模糊分低于按头高缩放后的闸 */
    int32_t  nTeacherNum = 0;      /* 本拍教师数 */
    int32_t  nCandidateNum = 0;    /* 本拍有教师证据但还没到确认时长的人数 */
    int32_t  nFlipDrop = 0;        /* 翻转环满被丢的条数：非 0 说明一拍里翻了太多人，判据在抖 */
    int32_t  nRoleRefError = 0;    /* 角色引用了没建成的区域（判定不完整）的人数·次，见 statJson。
                                      ★ 经 CRegionTable::build 的源头过滤（引用未建成的 id 会被剔除），
                                      走公开 API 到不了这条路，实测恒 0；留着是因为 inRole 仍要能表达
                                      "这一拍查不到区域"，随机流已把它钉成恒 0 的断言（#84 subjfuzz） */
    uint64_t u64StayEvidence = 0;  /* 静止指导弱证据成立的拍数（#93-4）。恒 0 而配置已开时，
                                      先查这三件事：stay_evidence_ms 是否被钳掉、讲台是否一直有人、
                                      学生区站立者是否从来不止一个——三关任何一关没过都是它 */
    uint64_t u64PoseWitBlocks = 0; /* T38：讲台/黑板区域命中被姿态证人拦下的人数·次。
                                      开着证人时恒 0 说明拦错了对象（幻影框上姿态模型反而有输出）；
                                      大量非 0 且教师晋升变慢，说明把真人也拦了——两边都要查 */
    uint64_t u64WitHeadMis = 0;    /* T38 v3：证人评估里"骨骼头点不在本行头框"的人数·次（借来的
                                      骨骼/板面幻觉）。own_head_pct=0 时只计数不拦——这个数就是
                                      余量调参的读数；开闸后非 0 且真教师晋升变慢 = 余量太紧 */
    uint64_t u64WitScoreRej = 0;   /* T38 v4：证人评估里"骨骼得分低于 min_score 下限"的人数·次。
                                      与 wit_head_mis 分开计数：现场调参要分得清一次拒绝
                                      是余量拒的还是得分拒的，混在一起两头都调不准 */
    bool     bPodiumEmpty = false;
    bool     bPodiumConfigured = false;
    bool     bBoardConfigured = false;
    bool     bTeacherAreaEmpty = false;
    bool     bStudentRoleConfigured = false;
} CSubjectArbiterStat_S;

/**
 * @brief   : 主体归属仲裁器
 * @note    : 无外部依赖（区域表、配置、特征帧全部逐拍传入）
 */
class CSubjectArbiter
{
public:
    CSubjectArbiter() = default;
    ~CSubjectArbiter() = default;

    CSubjectArbiter(const CSubjectArbiter &) = delete;
    CSubjectArbiter &operator=(const CSubjectArbiter &) = delete;

    /**
     * @brief   : 跑一拍仲裁
     * @param    {const CFeatureFrame_S&} stFrame：本拍特征帧（行序即处理序）
     * @param    {const CRegionTable&} stRegion：区域表（分析帧坐标，已换算）
     * @param    {const CConfigSnapshot_S&} stSnap：与本次判定同一份配置快照
     * @param    {int64_t} llFrameMs：特征帧时刻（单调毫秒），**不用 wall clock**——
     *             回放要能复现，且 NTP 校时的回拨会把确认/放弃计时整个吞掉
     * @note    : 关掉仲裁（teacher.enable=false）时**仍要照调本方法**：那条路径负责把在任
     *             教师的身份就地收回并发出翻转。调用方若因为开关关掉就跳过，教师行为列
     *             等不到收尾，现象是"把教师分析关掉之后他的站立还挂着没结束"
     */
    void update(const CFeatureFrame_S &stFrame, const CRegionTable &stRegion,
                const CConfigSnapshot_S &stSnap, int64_t llFrameMs);

    /**
     * @brief   : 取某人本拍的归属事实
     * @return   {bool} false 表示没有该人的槽位（调用方按 SUBJECT_UNKNOWN 处理，
     *            即"不知道"，**不是**"确定不是教师"）
     */
    bool factOf(int32_t nPersonId, CSubjectFact_S &rstOut) const;

    /* 便捷读法：没有槽位时返回 UNKNOWN */
    Subject_E subjectOf(int32_t nPersonId) const;

    /* 取一条待处理的翻转（false=环空）。规则引擎每拍取空为止 */
    bool pollFlip(CSubjectFlip_S &rstOut);

    /* 本拍教师数 / 讲台是否有人 / 复位（引擎 reset、总开关关闭时） */
    int32_t teacherNum() const { return m_nTeacherNum; }
    bool isPodiumEmpty() const { return m_bPodiumEmpty; }
    /**
     * @brief   : 当前在任教师的跟踪 ID（-1 = 本拍无教师；候选不算）
     * @note    : 多教师（max_num>1）时取**表序第一个**——板书停留边沿一次只关心一位，
     *            在任教师默认就是 1 位（文件头"身份是互斥资源"），多教师的教室里
     *            边沿跟谁都可能错，取确定的一个比取"恰好站在板前那个"更可复现
     */
    int32_t teacherId() const;
    /**
     * @brief   : 这个人是不是「教师位上的候选」——在任教师，或证据还没攒满确认时长的候选
     * @note    : 候选也要算（#93-5）：弱证据（游走/停留/站立）坐上候选时身份靠姿态重认，
     *            只认在任教师会让加急晚整整一个确认时长到场。找不到人 = false
     */
    bool isTeacherCandidate(int32_t nPersonId) const;
    /**
     * @brief   : 取本拍的帧级归属上下文（教师采样器要的那四条：两个角色画没画、空不空）
     * @note    : 节拍线程专用，与 isPodiumEmpty() 同一线程域；跨线程要读的是 stat() 那份发布件
     */
    CSubjectView_S view() const;
    void reset();

    CSubjectArbiterStat_S stat() const;
    void resetStat();
    std::string statLine() const;    /* 心跳 subj= 段 */
    std::string statJson() const;    /* statusJson 的 subject 段 */

private:
    /* 一个跟踪 ID 的跨拍记忆 */
    struct CSlot_S
    {
        int32_t  nOwnerId = -1;            /* -1 = 空槽 */
        Subject_E enSubject = Subject_E::SUBJECT_UNKNOWN;
        int64_t  llCandidateMs = 0;        /* 本轮教师证据起点，0 = 当前无证据 */
        int64_t  llLastEvidenceMs = 0;     /* 最近一次证据成立时刻，放弃计时基准 */
        int64_t  llMoveWindowMs = 0;       /* 位移窗口起点 */
        int64_t  llLastSeenMs = 0;         /* 最近一次在本拍出现的帧时刻，0 = 从未 */
        /* 学生区连续停留的锚点（#93-4 静止指导弱证据），0 = 本拍不在学生区。
           锚的清理不需要显式做：不在学生区置 0、人不在本拍整格回收（retireSlot 的
           全格清零），两条既有路径顺手把这件事做掉了 */
        int64_t  llStudentStaySinceMs = 0;
        double   dfMovePx = 0.0;           /* 最近一个窗口的累计位移 */
        int64_t  llLastMoveMs = 0;         /* 最近一次"达门"的时刻（走动保持的锚） */
        CPointf_S stLastPos;               /* 上次位置，0 表示还没有 */
        /* 位移滑窗：窗口一满就归零会把「持续慢走」测成 5s 真 / 5s 假
           的锯齿，时长门（min_duration + gap_tolerance）被反复打断，210 永远开不出事件。
           改成 MOVE_BUCKET_NUM 个等长桶的环形窗：dfMovePx = 最近一个窗口的路径和，
           桶长 = patrol_window_ms / N（量化误差 ≤ 桶长，5s 窗口下 0.5s）。
           额外内存 10×8B + 8B + 4B ≈ 92B/槽，60 槽 < 6KB */
        static constexpr int32_t MOVE_BUCKET_NUM = 10;
        double   afMoveBucket[MOVE_BUCKET_NUM] = {};
        int32_t  nMoveBucketIdx = 0;
        int64_t  llMoveBucketMs = 0;
        /* 本槽的环形桶是按哪个 patrol_window_ms 切出来的（#158 第 23 轮）：桶长 = 窗宽/N，
           窗宽一变格点就不是同一把尺。存下来只为跨缺席恢复时判断"旧桶还能不能接" */
        int32_t  nWindowLenMs = 0;
        /* T38 姿态证人（ever-sticky）：本轨只要见过一次可用骨骼就置位，整格回收时随槽清零。
           头检在深色板面会输出 ~300px 的幻影大头（其上 FastPose 拿不出人），以"见过姿态"
           把这类框挡在教师证据之外；真人在场的头一秒内就有骨骼，代价只是晋升晚 ~1s */
        bool     bPoseWitness = false;
        /* T38 v3：证人读数行只打每轨第一次可用骨骼那拍（行数有界，pct 调参的原始数据） */
        bool     bWitEvalLogged = false;
        CSubjectFact_S stFact;
    };

    /* 位移记忆：槽位在"缺一拍"时整格回收（retireSlot 无差别全清），
       位移环形窗随之清零；而跟踪器刻意不输出丢失拍（防人数虚高）。这里给刚回收的槽留一份位移记忆：同 ID 在 TTL 内重新出现就接上
       （跨过的桶照样清掉——缺席那段没看见人，不能把停之前的位移算进来）。
       只在回收/重认领这两点使用，不参与归属与计数。
       ★ 表满时的取舍（#158 第 24 轮）：现行语义下"接回即失效"（restoreMoveMem 命中后
       把记忆格置空），所以同一 ID 不会同时占两格、新写入总是走到环位——旧实现的环位覆盖
       因此**恰好**等于"按最旧覆盖"（这一点本轮核对过：把修复整个退掉，行为读数不变，
       只有新增计数变红）。本实现不依赖这条隐含等式：先复用空/过期格，全表在期时淘汰
       llSaveMs 最旧的一条，并把"在期淘汰"计入 move_mem_evict。值得显式化的理由不是
       旧码错，而是 60 人满座时一次检测闪断就在同一拍倒下 60 条记忆、两次闪断表就满，
       210 的巡视连续性正靠这条记忆跨过闪断——淘汰策略不该由"环位恰好等于年龄序"
       这种跨函数耦合兜着走。64×~112B ≈ 7KB */
    static constexpr int32_t MOVE_MEM_NUM = 64;
    static constexpr int64_t MOVE_MEM_TTL_MS = 3000;
    typedef struct _MoveMem_
    {
        int32_t  nOwnerId = -1;
        int64_t  llSaveMs = 0;
        int64_t  llBucketMs = 0;
        int64_t  llWindowMs = 0;   /* 上次看见他的那一拍（窗口锚），接回时用它算真实缺席时长 */
        int32_t  nBucketIdx = 0;
        int64_t  llLastMoveMs = 0;
        CPointf_S stLastPos;
        int32_t  nWindowLenMs = 0;  /* 存入时槽位桶格的窗宽；接回时与当前窗宽不符 ⇒ 累计不继承 */
        double   afBucket[CSlot_S::MOVE_BUCKET_NUM] = {};
    } CMoveMem_S;
    CMoveMem_S m_aMoveMem[MOVE_MEM_NUM];

    /* llNowMs 只用于淘汰决策：判断哪几格已过期、以及全在期时挑出最旧的一条 */
    void saveMoveMem(const CSlot_S &rstSlot, int64_t llNowMs);
    void restoreMoveMem(CSlot_S &rstSlot, int64_t llFrameMs, int32_t nWindowMs);

    /* 按 ID 取槽位（精确匹配，没有则 nullptr）；与取模索引的区别见文件头 */
    CSlot_S *findSlot(int32_t nPersonId);
    const CSlot_S *findSlot(int32_t nPersonId) const;
    /**
     * @brief   : 认领槽位：已占用则复用，否则取空槽；无空槽时挤掉最久未见的那个
     * @note     : 槽数与在场人数上限同为 CLASSROOM_MAX_PERSONS，且本拍没出现的行会被回收，
     *             所以"挤占"正常不可能发生；它一旦发生就说明某处生命周期不对齐，
     *             宁可计数并挂在 u64SlotSteal 上，也不要悄悄丢掉某个人的归属。
     *             分配顺序是**确定的**（精确匹配 → 第一个空槽 → 最久未见，并列取表序靠前），
     *             台账可以逐格重放，故随机不变量长跑按等式查它（#84 的 S4/S5）
     */
    CSlot_S &acquireSlot(int32_t nPersonId, int64_t llFrameMs);
    /* 整格回收（本拍没有这个人的行）：与 CDurationGate::retirePerson 同一时机、同一理由。
       llFrameMs 只传给位移记忆，用来判断哪几格可用（见 saveMoveMem） */
    void retireSlot(CSlot_S &stSlot, int64_t llFrameMs);

    void pushFlip(int32_t nPersonId, Subject_E enFrom, Subject_E enTo, int64_t llMs);
    void publishStat();
    /**
     * @brief   : 框是否落在给定角色区域的任一个里
     * @param    {const std::vector<int32_t>&} vecIds：调用方每拍取一次的该角色 id 列表
     * @param    {bool&} rbRefError：OUT=true 表示该角色引用了一个没建成的区域（判定已不完整）
     * @return   {bool} 空列表一律 false —— 不借用 CRegionTable::inAnyRegion 的空表语义
     *            （那边空表表示"不限区域"，是给逐行为区域闸用的；这里借用了就等于
     *            "没画讲台区域"变成"全员都在讲台"，归属整条反向）
     */
    bool inRole(const CRegionTable &stRegion, const std::vector<int32_t> &vecIds, float fCoverage,
                const CRectf_S &stHeadBox, bool &rbRefError) const;

private:
    CSlot_S m_astSlot[CLASSROOM_MAX_PERSONS];

    CSubjectFlip_S m_astFlip[CLASSROOM_SUBJECT_FLIP_RING];
    uint32_t m_u32FlipHead = 0;
    uint32_t m_u32FlipNum = 0;

    /* T44 教师重入记忆：最近丢过身份的教师留下的（谁、何时、最后位置）。
       写入点两条，都是"槽被回收、人可能还在"：① 回收循环收掉教师槽（含容忍期满）；
       ② 槽池满时挤掉在任教师（acquireSlot，此时人在本拍行里、只是行序靠后）。
       放弃时长收回、总开关收回、上限下调都不写——三者要么是明确的管理动作
       （收权不许被快轨顶回去，#84 的 D1），要么在 tt19 的病理里不出现
       （出现时按旧契约重攒确认）。**多条、每条用后即焚**：同拍多位教师退位时
       谁都不能覆盖谁（第 47 轮），一次丢失只补偿一次继承，第二个同位置的候选
       照常排队攒确认——不给"商队"留门。容量与槽表同宽（同一拍最多这么多个教师
       退位），满了淘汰最旧的一条（LRU，写入者只在"槽回收"这一侧） */
    struct CReentryMem_S
    {
        int32_t   nOwnerId = -1;   /* -1 = 无记忆 */
        int64_t   llRetireMs = 0;
        CPointf_S stPos;           /* 丢身份那拍的槽位最后位置（头框中心） */
    };
    static constexpr int32_t REENTRY_MEM_NUM = CLASSROOM_MAX_PERSONS;
    CReentryMem_S m_astReentry[REENTRY_MEM_NUM];
    uint64_t m_u64Reentry = 0;
    int64_t  m_llStealLogMs = 0;   /* 挤占告警上一次落盘的帧时刻（1s 窗口限频） */

    /* 写一条重入记忆：先占空格，全满时淘汰 llRetireMs 最旧的一条 */
    void saveReentryMem(int32_t nOwnerId, const CPointf_S &stPos, int64_t llFrameMs);
    /* 按位置找**最近**的一条在窗内记忆；没有返回 -1。
       用最近而不是表序第一条：同拍两位教师退位、继任各回原位时，距离才是身份的证据 */
    int32_t findReentryMem(const CPointf_S &stPos, int64_t llFrameMs, int32_t nWindowMs,
                           int32_t nPx) const;

    /* 讲台占位保持窗：证人开着时，换槽/丢检会造出 ≤2s 的
       "本拍没人"空拍。最近一次被证人计到在讲台起 2s 内仍算占着——210 的 TRUE
       段从「最后见证 + 保持窗」起算，而不是把空拍缝进上一段再回推起点。
       与 210 的 gap_tolerance 2000 同量级，但作用在源头。证人关位（ttl=0）时不生效：
       那种形态里 bInPodium 本拍即真，没有换槽空拍可补 */
    static constexpr int64_t PODIUM_OCCUPANCY_TTL_MS = 2000;
    int64_t m_llPodiumWitnessMs = -1;   /* 最近一次被证人计到在讲台的帧时刻，-1=从未 */

    /* ---- 以下只在节拍线程读写 ---- */
    int32_t m_nTeacherNum = 0;
    bool    m_bPodiumEmpty = true;
    bool    m_bTeacherAreaEmpty = true;   /* 讲台与黑板（画了的那些）本拍都无人，见 CSubjectView_S */
    bool    m_bBoardConfigured = false;
    uint64_t m_u64Updates = 0;
    uint64_t m_u64Promote = 0;
    uint64_t m_u64Demote = 0;
    uint64_t m_u64SlotSteal = 0;
    uint64_t m_u64MoveMemEvict = 0;
    uint64_t m_u64StandBlocked = 0;
    uint64_t m_u64StandBlkNoPose = 0;
    uint64_t m_u64PoseWitBlocks = 0;
    uint64_t m_u64WitHeadMis = 0;
    uint64_t m_u64WitScoreRej = 0;
    uint64_t m_u64StandBlkOccl = 0;
    uint64_t m_u64StandBlkEdge = 0;
    uint64_t m_u64StandBlkBlur = 0;
    uint64_t m_u64StayEvidence = 0;
    int32_t  m_nCandidateNum = 0;
    int32_t  m_nFlipDrop = 0;
    int32_t  m_nRoleRefError = 0;
    bool     m_bPodiumConfigured = false;
    bool     m_bStudentRoleConfigured = false;
    /* 上一拍发布出去的观测：stat() 会被命令线程读，逐字段抄一份的成本换掉撕裂读数 */
    mutable std::mutex m_mtxPub;
    CSubjectArbiterStat_S m_stStatPub;
};
}
