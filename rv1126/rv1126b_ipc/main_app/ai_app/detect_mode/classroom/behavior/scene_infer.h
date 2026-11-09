/**
 * @FilePath     : scene_infer.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 09:34:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 09:34:00
 * @Description  : 课堂场景时段推断
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* 段环容量 */
#define CLASSROOM_SCENE_RING (64)

/* 一份报表最多带几段 */
#define CLASSROOM_SCENE_REPORT_MAX (16)

/**
 * @brief   : 课堂场景
 */
typedef enum class Scene : int
{
    SCENE_UNKNOWN = 0,    /* 判不出：课间、无人、或这一拍没有任何有效证据（这是一段真读数，不是失败） */
    SCENE_TEACHING = 1,   /* 知识教授 */
    SCENE_QA = 2,         /* 教学问答 */
    SCENE_DISCUSS = 3,    /* 小组讨论 */
    SCENE_PRESENT = 4,    /* 上台展示 */
    SCENE_PRACTICE = 5,   /* 课堂练习 */
    SCENE_NUM = 6,
} Scene_E;

/**
 * @brief 时段推断的参数
 */
typedef struct _SceneParam_
{
    bool bEnable = true;
    /* 一段要连续观察到多久才承认它开始。取 30s ≈ 六个群体采样周期：
       再短，一次"老师转身擦黑板 + 有人举手"就能把课堂切成三段；
       再长，一节 40 min 的课里真正的短问答（1~2 min）会被整段抹平 */
    int32_t nEnterMs = 30000;
    /* 已承认的段允许被多长的一拍反证打断。刻意**小于** nEnterMs：
       进入要稳、退出不必同样难，否则一段真讨论里两次 5s 的证据空窗会把讨论切没了 */
    int32_t nExitMs = 15000;
    /* 一段最短多久。短于此的段不单独成段（并入相邻段），依据是平台按段算占比时
       三秒的段会把"讨论占 12 分钟"变成"18 段平均 40 秒"，前者是有用的数，后者不是 */
    int32_t nMinSegmentMs = 30000;
    /* 问答那一档的举手密度：举手生效人数 ÷ 有证据学生数。
       默认 0.15 的量级依据：40 人班里 6 个人举手就是明显的问答现场。
       这是本文件两把**比例**旋钮之一（另一把是下面的 fHeadDownRatioPractice），它量的
       不是"什么行为成立"（那是采样器的事），而是"多少人做这个动作才算一堂问答课"——
       这句话只有时段这一层需要回答 */
    float fRaiseRatioQa = 0.15f;
    /* 练习那一档的"各自低头"密度，同上。**只看低头（104）一项**，不另加阅读（108）：
       108 与 106 共用同一个形状（低头 × 手在身前，见 student_behavior 第 8 步），
       阅读本就是低头的子集——两项相加等于把同一批人数两遍。低头密度到线即练习；
       手里拿的是书还是手机，是 108/106 那两列的账，不是本层的 */
    float fHeadDownRatioPractice = 0.40f;
    /* 教师讲授这一档是否还要求"听讲时段成立"。默认 true：
       关掉它就退化成"老师在讲台上讲话就是知识教授"，那正是学生上台讲题那三分钟被误判的原因 */
    bool  bTeachingNeedsListen = true;
    /*「讨论」这一档的最少参与人数 */
    int32_t nDiscussMinPersons = 2;
    /* 教师轨迹/座位的采样点**俯角修正**：采样点是头框中心，
       而俯机位下"头框中心在图像上的位置"与"这个人脚下的地面点"的对应关系随俯角漂移。
       这个键把采样点沿画面下方平移 stHeadBox.height()×键值 个像素后再做归一化，
       供现场对着教师轨迹/座位热力标定。0 = 不动（与旧口径逐位一致）；
       上界 2 个头框高是"再大就该查几何安装而不是靠这个键硬拉"的量级 */
    float fGroundShiftHeads = 0.0f;
} CSceneParam_S;

/**
 * @brief 一拍的场景原料。**全部是别处已经算好的事实**，本层不再判任何东西
 */
typedef struct _SceneFact_
{
    int64_t  llNowMs = 0;         /* 采样时刻（单调毫秒，与群体样本同一时基） */
    int32_t  nEvidenceNum = -1;   /* 有证据的学生数（比率的分母），<0 = 这一拍没测 */
    bool     bTeacherPresent = false;  /* 本拍有可归属的教师 */
    bool     bTeachingOn = false;      /* 206 讲授生效 */
    bool     bInteractOn = false;      /* 209 互动生效 */
    bool     bBoardEdge = false;       /* 上次采样以来出现过一次板书采集边沿 */
    int32_t  nRaiseNum = 0;            /* 102 生效人数 */
    int32_t  nHeadDownNum = 0;         /* 104 生效人数 */
    int32_t  nDiscussNum = 0;          /* 109 生效人数 */
    int32_t  nPresentNum = 0;          /* 110/111 生效人数（在讲台区/演示） */
    int32_t  nListenPeriod = -1;       /* 群体层三态：1 成立 / 0 未达 / -1 没测 */
    int32_t  nDiscussPeriod = -1;      /* 同上（讨论＝最少参与人数） */
} CSceneFact_S;

/**
 * @brief 一段已闭合（或仍在开）的场景
 */
typedef struct _SceneSegment_
{
    Scene_E  enScene = Scene_E::SCENE_UNKNOWN;
    int64_t  llBeginMs = 0;
    int64_t  llEndMs = 0;        /* 0 = 还没闭合 */
    int32_t  nPersonNum = 0;     /* 段内最后一次看到的人数（"32 人的小组讨论"里的 32） */
    int32_t  nBoardNum = 0;      /* 段内板书采集边沿次数（老师写了多少板），也是"这段是讲授"的旁证 */
} CSceneSegment_S;

/**
 * @brief 场景层的观测（心跳 scene= 段与 statusJson 的数据源）
 */
typedef struct _SceneStat_
{
    uint64_t u64Fed = 0;          /* 被喂了几拍 */
    uint64_t u64Skipped = 0;      /* 因为 enable=false 没判的拍数 */
    uint64_t u64Changed = 0;      /* 候选场景变化的次数（含还没承认的）：与 u64Closed 的差就是滞回挡掉的抖动 */
    uint64_t u64Closed = 0;       /* 闭合出的段数 */
    uint64_t u64MergedShort = 0;  /* 因短于 nMinSegmentMs 被并掉的段数 */
    uint64_t u64DropOldest = 0;   /* 环满丢最旧 */
    uint64_t u64NoFact = 0;       /* 喂进来的事实不可用（时间戳没起来等） */
    int32_t  nCurrent = static_cast<int32_t>(Scene_E::SCENE_UNKNOWN);
} CSceneStat_S;

/**
 * @brief   : 场景时段推断器
 */
class CSceneInfer
{
public:
    CSceneInfer() = default;

    CSceneInfer(const CSceneInfer &) = delete;
    CSceneInfer &operator=(const CSceneInfer &) = delete;

    void setParam(const CSceneParam_S &stParam);
    const CSceneParam_S &param() const { return m_stParam; }

    /**
     * @brief   : 喂一拍原料
     * @param    {const CSceneFact_S&} stFact：本拍事实（时间戳必须单调不回退，回退的那拍被拒并计数）
     */
    void feed(const CSceneFact_S &stFact);

    /* 当前（可能还在延长）的那一段。返回 false = 一段都还没有（刚起线或被 reset） */
    bool current(CSceneSegment_S &rstOut) const;
    /* 当前场景（无段时 UNKNOWN），给心跳与状态用 */
    Scene_E currentScene() const { return m_enCurrent; }

    /**
     * @brief   : 会话收尾：把还在开的段按 llEndMs 闭合，但**不清环**
     */
    void closeSession(int64_t llEndMs);
    /* 总开关关掉或重启本节：丢掉所有段与状态。与 closeSession 的区别就在这一条——
       closeSession 是"下课了，段还在等着被取走"，reset 是"这些数据不要了" */
    void reset();

    /**
     * @brief   : 取走最多 nMax 条已闭合的段（最旧的先走）
     * @param    {int32_t} nMax：本次最多取几条（<=0 视为 1，避免"取 0 条"被当成 bug）
     */
    std::vector<CSceneSegment_S> takeClosed(int32_t nMax);
    /* 还留在环里的闭合段数（诊断用） */
    int32_t closedNum() const;
    CSceneStat_S stat() const { return m_stStat; }
    std::string statText() const;

    /* 场景名的唯一出处（报表字符串与日志共用，别处不许再拼一份表） */
    static const char *sceneName(Scene_E enScene);

private:
    /* 这一拍按优先级算出来的候选（不含滞回） */
    Scene_E decide(const CSceneFact_S &stFact) const;
    void appendClosed(const CSceneSegment_S &stSeg);

    CSceneParam_S m_stParam;

    Scene_E  m_enCurrent = Scene_E::SCENE_UNKNOWN;   /* 已承认的当前场景 */
    /* 滞回的两个数：**累计**反证时长（到 nEnterMs 才切段）与上一拍反证的时刻
       （两拍之间隔超过 nExitMs 就算这一轮反证断了，重新累计）。
       刻意不记"候选是什么"：候选每拍现算，记下来就会有一处"算的是一份、比的是另一份"的窗口 */
    uint64_t m_u64AgainstMs = 0;
    int64_t  m_llAgainstSinceMs = 0;     /* 本轮反证的第一拍，切段时段的边界就是它 */
    int64_t  m_llLastAgainstMs = 0;      /* 最近一拍反证，用来判"隔了多久" */
    bool     bHaveSegment = false;
    CSceneSegment_S m_stOpen;

    /* 本轮反证**第一拍之前**的人数与板书计数。段的边界退到那一拍（见 feed 的 llCutMs），
       段内读数也必须停在那里——否则 enter_ms 窗口（默认 30s）里的板书边沿与人数读数
       会被记到已经结束的那一段上，报表上"讨论段写了 3 块板"其实后 30s 已经在上新课 */
    int32_t  m_nPersonAtAgainst = 0;
    int32_t  m_nBoardAtAgainst = 0;

    std::vector<CSceneSegment_S> m_vecClosed;        /* 定容环，构造时按 CLASSROOM_SCENE_RING 定 */
    int32_t m_nHead = 0;
    int32_t m_nNum = 0;
    int64_t m_llLastMs = 0;
    CSceneStat_S m_stStat;
};
}
