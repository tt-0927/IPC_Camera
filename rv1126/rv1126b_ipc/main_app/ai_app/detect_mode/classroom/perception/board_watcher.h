/**
 * @FilePath     : board_watcher.h
 * @Author       : leiyy
 * @Date         : 2026-11-03 13:58:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 13:58:00
 * @Description  : 板书记录与 OCR 后端接缝
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* 一次板书记录里文字串的字节上限 */
#define CLASSROOM_BOARD_TEXT_MAX_BYTES (512)

/* 引擎侧的板书条数硬上限 */
#define CLASSROOM_BOARD_ITEM_MAX (64)

/* 一次 OCR 能回报的最多行数（超出丢弃并计数）：防止后端异常时把环挤爆 */
#define CLASSROOM_BOARD_LINE_MAX (24)

/**
 * @brief OCR 后端的一行识别结果
 */
typedef struct _BoardText_
{
    std::string strText;        /* UTF-8 文本，后端已解码 */
    float       fScore = 0.0f;  /* 0~1 行级置信度 */
    CRectf_S    stBox;          /* OCR 输入图内的行框（像素），调用方负责换算回分析帧 */
} CBoardText_S;

/**
 * @brief   : OCR 后端抽象（det→cls→rec 串联对调用方不可见）
 * @note    : 与 IModelHandle 不是一层东西，故不复用：IModelHandle 是「一次张量进、一组浮点出」，
 *            而 OCR 是三段模型加一套字典解码。把两者捏成一个接口的后果是
 *            要么 detect_decode 那套打平约定被硬塞给 OCR，要么 OCR 在 handle 里私藏一整条流水线
 */
class IOcrBackend
{
public:
    IOcrBackend() = default;
    virtual ~IOcrBackend();

    IOcrBackend(const IOcrBackend &) = delete;
    IOcrBackend &operator=(const IOcrBackend &) = delete;

    /**
     * @brief   : 一张图 → 若干行文字
     * @param    {const uint8_t*} pucRgb：连续 RGB888（RGA 输出，通道序与检测路一致）
     * @param    {int32_t} nW / nH：输入图宽高
     * @param    {std::vector<CBoardText_S>&} rvecOut：函数内先 clear 再填（调用方保留容量）
     * @return   {bool} false 表示这张图没有可信结果。
     *             **空板（一块干净的黑板）必须返回 true 且 rvecOut 为空**，
     *             与"识别失败"区分开：前者是"没有板书内容"这一真实现象，
     *             后者被报成前者就会让整节课看起来一块空板
     */
    virtual bool recognize(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                           std::vector<CBoardText_S> &rvecOut) = 0;

    /* 后端名，进日志与 statusJson：多后端并存时要能一眼看出此刻在用哪个 */
    virtual const char *name() const = 0;
};

/**
 * @brief   : 模型未交付时的占位后端
 * @note    : 唯一被允许在本目录里写 TODO(model-pending) 的地方（§9.1 的统一占位约定）。
 *            它**不**假装识别出点什么：recognize() 恒 false，由 CBoardWatcher 记 unavailable，
 *            平台侧因此能区分"这节课没写字"和"这台机器没有 OCR 能力"
 */
class CNullOcrBackend : public IOcrBackend
{
public:
    bool recognize(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                   std::vector<CBoardText_S> &rvecOut) override;
    const char *name() const override { return "none"; }
};

/**
 * @brief 一条对外发布的板书记录：**只有计数，没有一个字的正文**
 * @note  这条结构会进上报报文，报文再进滞留环（落 flash、出设备）。板书内容按 PII 口径
 *        与花名册同级——一块板上写着谁的值日、谁的分数，与在学生姓名，都不该离开设备（铁律 7）。
 *        文本只在 onOcrResult 的调用栈里活一瞬间：够算行哈希与字节数，然后即弃。
 *        ★ 判据不是"写报文的地方恰好没打这个字段"，而是**这条结构装不下文本**：
 *        它刻意不含 std::string，report 用例对它有一条
 *        `static_assert(std::is_trivially_copyable<CBoardItem_S>::value)`——
 *        谁把文本加回来，闸门在编译期就拒绝，而不是等某个现场导出报文之后才发现
 */
typedef struct _BoardItem_
{
    uint32_t    u32Seq = 0;         /* 会话内单调递增，平台侧按它去重与排序 */
    int64_t     llTsMs = 0;         /* classroomNowMs 口径（发布时由引擎换算成墙上时间） */
    int32_t     nLineNum = 0;       /* 过了分数与归一化那道筛、且留在前 24 行之内的行数 */
    int32_t     nTextBytes = 0;     /* 这些行的 UTF-8 字节数（含 1 字节换行分隔）：只报长度，不报内容 */
    float       fConfidence = 0.0f; /* 行置信度的算术均值（旧注释写"面积加权"，代码从来是均值） */
} CBoardItem_S;

/**
 * @brief 板书记录参数（需求 11：全部可配）
 */
typedef struct _BoardParam_
{
    /**
     * @brief 重入下限（毫秒）：**不是采样周期**
     * @note  触发源是"教师面向黑板 → 转回面向学生"这条边沿（见 bRequireEpisode），
     *        这个数只决定两次边沿之间最少隔多久。取 60s 的量级依据：一次板书 20~40s，
     *        而黑板内容在一节课里真会变的间隔是分钟尺度。**0 的含义与直觉相反**：不是"不限"
     *        更不是"每次转回都采一张"，而是**本功能不参与节拍**（`wantCapture` 里
     *        `<= 0` 直接不采，见 board_watcher.cpp:231 与 warm_policy 的同一条）；配置层
     *        钳 [1000, 3600000]（`board.reentry_ms`），所以现场配不到 0，该分支是防御。
     *        真要"每次转回都采"是把此键调到 1s 量级，两者不是一回事
     */
    int64_t llIntervalMs = 60000;
    /**
     * @brief 同一板书 episode 内的重识别间隔（毫秒，0=关，默认 5000）
     * @note  Web 第 24 页「分析间隔时间（1-10s）」落在这个键上，与上面那条**不是同一个量**：
     *        上面 llIntervalMs 管"两次板书结束边沿之间最少隔多久"（防止一转身采两次），
     *        本项管"老师还在板前写的时候，每隔多久重新看一眼"。没有它时，一场 3 分钟的
     *        板书只在收笔那一刻采一张，中间擦改的板书内容全都看不到（需求要的是周期分析）。
     *        只在"教师站在黑板区域里"（notifyTeacherStay 的 bInBoard）时生效：
     *        没人站板前就没必要花一次 det+N 次 rec。
     *        取 5s 是需求区间的中值；调到 1s 会把 OCR 变成准实时（NPU 与上报配额都要重估），
     *        调到 10s 则一节 40 分钟的板书约 240 次，仍远超一次板书真正变化的次数 */
    int64_t llScanIntervalMs = 5000;
    /**
     * @brief 是否只在"板书结束"边沿上采集（P6d 定的触发口径）
     * @note  关掉它退回旧的纯周期采法（留这个阀是因为 OCR 到货后要能对比两种取图时机的读数）。
     *        默认 true 的理由是周期采法两头都亏：短板书（写两个字就转回来讲解）落在两个周期之间
     *        直接漏掉，而黑板没变的那些周期每次都要花一次 RGA 裁剪 + 一次几十到几百毫秒的推理
     */
    bool    bRequireEpisode = true;
    /* 在途超时：提交给引擎的采集请求一直没回结果，超过此刻强制放行。
       没有这一条时"某次回调被丢"会让板书永久停摆（见 m_bInFlight 的注释） */
    int64_t llInflightTimeoutMs = 10000;
    /**
     * @brief 区域停留结束边沿的最短停留（毫秒），0=关（#93-4 加的第二个触发源）
     * @note  上面那条边沿（bRequireEpisode）靠"教师背身→转回"这一对行为事件，
     *        漏的是**侧身板书**：半侧身写字时面部证据断续可见，行为层开不出
     *        "面向黑板"这一段，END 边沿就不存在。这条边沿改问另一个事实——
     *        教师在黑板区域里**连续站了多久**，站满本数然后离开，也算一次板书结束。
     *        取 3s：转身写两个字（写课题那种）可能不到 2s，故意放它漏；
     *        真正侧身解题/抄题是分钟级的。0 是合法配置（现场不想要这条边沿），
     *        所以夹取只在负数时兜底，不把 0 抬成默认值 */
    int64_t llStayExitEdgeMs = 3000;
    /* 遮挡比上限：黑板角色区域内被人体框覆盖的比例超过它就不采（本轮直接放弃，不算变更） */
    float fMaxOccludeRatio = 0.25f;
    /* 行置信度下限：低于它的行不进哈希（OCR 对侧身挡住的字常给 0.2 上下的垃圾文本，
       这些字每帧都不一样，进哈希就等于每帧都"板书变了"） */
    float fMinLineScore = 0.55f;
    /* 单条记录的文字字节上限，超出判整条非法 */
    int32_t nTextMaxBytes = CLASSROOM_BOARD_TEXT_MAX_BYTES;
    /* 最多缓存几条（含对外待取），超出丢最旧并计数 */
    int32_t nItemMax = CLASSROOM_BOARD_ITEM_MAX;
    /**
     * @brief 一次采集最多留多少行（超出按**阅读序**截，并计一次 u64LineOverflow）
     * @note  上界就是 CLASSROOM_BOARD_LINE_MAX：它是本层对"一次采集最多往环里塞多少行"的
     *        承诺（后端异常狂吐时不挤爆那条 64 格定长环），而上面那层（ocr_decode）能交回来的
     *        是 CLASSROOM_OCR_BOX_MAX=32 行 —— 32→24 这一格压缩正是发生在这里。配得比宏更大
     *        不是"存不下"而是"这一层的承诺没了"：那一格读数是 line_max 说话、宏不说话，
     *        现场按 30 配、实际按 30 留，环的容量假设（第 8 条：丢最旧）就被这格配置改写了。
     *        配小它是**有用**的：满板噪声多的时候少报几行比一次交 24 行 rec 便宜（每行一次识别），
     *        而"截的是哪几行"由排序保证稳定——这一格真正能配的只是"要不要少留"，
     *        不是"换一种截法"（按分数截会让同一块板两帧留下不同的行集合，见 lineLess 那条注释）
     */
    int32_t nLineMax = CLASSROOM_BOARD_LINE_MAX;
    /* 功能总开关：关掉时 wantCapture 恒 false，且**不清空已缓存待发的记录**——
       关一扇功能不该把已经采到的内容丢掉，那是用户看不见的静默删除 */
    bool bEnable = false;
    /* 调试开关：打开后把 OCR 解码出的原始文字行打进**本地业务日志**，
       供现场调试识别质量用。默认关=出厂行为；PII 铁律 7 的口径不变——心跳与上报仍只有
       计数与哈希，本开关只多一路本地日志，不影响任何对外报文 */
    bool bDebugText = false;
} CBoardParam_S;

/**
 * @brief 板书路观测计数
 */
typedef struct _BoardStat_
{
    uint64_t u64Want = 0;             /* 到点次数（含被门挡掉的） */
    uint64_t u64Capture = 0;          /* 真正发起采集次数 */
    uint64_t u64SkipOccluded = 0;     /* 因遮挡放弃 */
    uint64_t u64SkipNoGeom = 0;       /* 几何未就绪（镜像/旋转参数没落地） */
    uint64_t u64NoRegion = 0;         /* 到点但黑板区域没画：与上一项分开，
                                        否则「现场忘了划线」会读成「几何有问题」，查错方向 */
    uint64_t u64BackendFail = 0;      /* 后端返回 false：含"能力不可用" */
    uint64_t u64NoChange = 0;         /* 结果与上一次相同，不产生记录 */
    uint64_t u64Emitted = 0;          /* 产生的记录条数 */
    uint64_t u64EmptyBoard = 0;       /* 识别成功但无内容（真·空板）次数 */
    uint64_t u64InflightTimeout = 0;  /* 在途超时放行次数，>0 就要查引擎的回调链 */
    /* 回来的结果与在途那一次配不上号（迟到的旧作业）：没有这一格，"旧图顶着新采集的名义
       进了报表"这件事在读数上与"正常多了一条板书"完全一样（#64 第 4 刀） */
    uint64_t u64ResultStale = 0;
    uint64_t u64DropOldest = 0;       /* 环满丢最旧 */
    uint64_t u64LineOverflow = 0;     /* 一次采集里"行数超上限"的事件数（一次到点最多计 1，
                                        不是被丢的行数——溢出时只留阅读序的前 24 行，
                                        报"溢了几行"会让同一块满板两帧报出不同的差值） */
    uint64_t u64TextTooLong = 0;      /* 整条因超长被判非法 */
    uint64_t u64BackendUnavailable = 0;/* 能力不可用的采集次数（unavailable 标记的来源） */
    /* 到点却因为 OCR 还在预热路上而**没消费这次机会**的次数（#64 第 8 刀）。
       与 u64BackendUnavailable 分开的理由是方向相反：那一列说"永远没有这个能力"，这一列说
       "再等一拍就有"。没有它，"开了板书而一条记录不进"在心跳里就只有 want 在涨这一个事实，
       查的人会去数模型文件（该查的是内存预算与暖组节拍） */
    uint64_t u64SkipNotWarm = 0;
    uint64_t u64Episode = 0;          /* 收到多少次「教师转回面向学生」的板书结束边沿 */
    uint64_t u64EpisodeMerged = 0;    /* 其中被合并的次数（上一次还压在下限里 / 请求已挂着）。
                                        与 u64Want 并排读才分得开"这节课没人板书"和"板书了但被合并" */
    /* 区域停留结束边沿触发并放行的次数（#93-4）。与 u64Episode 分开：两条边沿合账后
       分不清"侧身板书那条边沿有没有在干活"——它整节课为 0 而板书记录正常时，
       读数上与"这台机器没开这条边沿"一样 */
    uint64_t u64StayEdge = 0;
    uint64_t u64ScanDue = 0;         /* 因"老师还在板前"的周期重识别而到点的次数
                                        （与 u64Want 分开：那一项含边沿触发，混在一起看不出
                                        周期键到底有没有生效） */
    /* ---- 以下四列是 #64 第 7 刀加的：心跳里那一格只放计数，而"识别得好不好"必须能
       在**看不到正文**的前提下判断（正文是 PII，不出设备）。四列合起来给的是 OCR 质量的
       分解：交回来多少框、留下多少行、写了多少字节、平均分多少 ---- */
    uint64_t u64BoxRaw = 0;           /* 后端交回来的行框数（**过滤前**，一次采集一批） */
    uint64_t u64LineKept = 0;         /* 过了分数与归一化那道筛、且留在前 24 行之内的行数。
                                        raw-kept 就是被 fMinLineScore、空行与截断吃掉的那部分 */
    uint64_t u64TextBytes = 0;        /* 已发出记录的正文合计字节（超长整条丢弃的不计）：
                                        它是"这块板写得多满"的度量，替代原来的文本长度 */
    uint64_t u64ConfPermilleSum = 0;  /* 有内容的记录的平均置信度之和（千分位整数）*/
    uint64_t u64ConfNum = 0;          /* 上面那个和的分母（**只数有内容的记录**）：
                                        拿 emit 当分母会被真·空板那些 0 分记录稀释，
                                        于是"这节课擦得很干净"读起来像"OCR 在掉分"——
                                        两个都是假读数。心跳里那一列是两者之商，
                                        和与分母都不单独出列（80B 一行心跳装不下两列没人看的数） */
    int32_t  nLastLineNum = 0;
    /* ---- 以下两列是 #64 第 10 刀加的：后端那两个读数（单帧耗时 / 被阈值截掉的行累计）
       一直在写进 CPPOcrBackend 自己的 stat()，但全板无读者——换件窗口到点量不到。
       本类的纪律是 watcher 不跨线程碰后端，
       所以这两个数由**引擎取结果那一拍顺手带进 onOcrResult**，抄在这里给 statText 读。
       -1 = 这一拍引擎没带新读数（当场回填的失败、还没投出去，后端计数器本来也没动），
       保持上一次的值——一次采集失败不该把"上一次真实跑多久"抹成 0（同 §"没跑过写 - 不写 0.000"） ---- */
    int64_t  llLastJobMs = -1;        /* 后端最近一次成功作业的**总**耗时（det + 逐行 rec） */
    uint64_t u64TruncTotal = 0;       /* 后端 DB 解码累计被 nBoxMax 截掉的行数（单调） */
} CBoardStat_S;

/**
 * @brief 板书采集请求：由本类产出、由引擎执行
 */
typedef struct _BoardCapture_
{
    bool     bWant = false;
    CRectf_S stRectSrc;        /* 源帧坐标的黑板区域（已合并多块黑板的外接框） */
    int64_t  llDueMs = 0;      /* 本次请求对应的到期时刻，回填时按它记时间戳 */
} CBoardCapture_S;

/**
 * @brief 板书记录器
 * @note  非线程安全：节拍线程独占。缓存容量在构造时按参数定死，运行期不再扩容
 */
class CBoardWatcher
{
public:
    CBoardWatcher();

    CBoardWatcher(const CBoardWatcher &) = delete;
    CBoardWatcher &operator=(const CBoardWatcher &) = delete;

    /**
     * @brief   : 换参数（配置换版）。nItemMax 变化时重建缓存，其余只换值
     * @note    : 换容量时**保留**已有记录：配置热更是现场改个数看效果的动作，
     *            把待发的板书记录清空等于让用户看到"一改配置内容就没了"
     */
    void setParam(const CBoardParam_S &stParam);
    const CBoardParam_S &param() const { return m_stParam; }

    /**
     * @brief   : 注入 OCR 后端
     * @param    {IOcrBackend*} pBackend：非拥有指针，须比本对象活得久；nullptr 表示能力不可用
     * @note    : 后端为空时本类**照常按周期到点并留下 unavailable 读数**，但不发起裁剪
     *            （那是白花的 RGA 与缓冲）。若空后端就连表都不走，
     *            「这功能没跑」与「跑着但没能力」在设备上就分不出来了。
     *            **本类不调 recognize()**：OCR 一次是几十到几百毫秒，压在节拍线程上
     *            违反 §4.2 规则 5。引擎拿 wantCapture 的框去取图并识别，结果回 onOcrResult；
     *            这里持有指针只作为「能力在不在」的判据
     */
    void setBackend(IOcrBackend *pBackend);
    bool backendReady() const { return nullptr != m_pBackend; }

    /**
     * @brief   : 设黑板区域（源帧归一化坐标，见 §4.7.1 同一口径）
     * @param    {const CRectf_S&} stRectNorm：多块黑板时取外接框；空框表示未配置
     * @note    : 用归一化而不是像素：镜像与旋转之后像素框会翻，而"哪块是黑板"这个事实不会。
     *            配置侧已经是归一化坐标，本类不再引入第二套坐标约定
     */
    void setBoardRectNorm(const CRectf_S &stRectNorm, int32_t nSrcW, int32_t nSrcH);

    /**
     * @brief   : 每拍问一次「到点采不采」
     * @param    {int64_t} llNowMs：单调时基
     * @param    {bool} bGeomReady：几何（镜像/旋转/画布）是否已就绪
     * @param    {float} fOccludeRatio：黑板区域被人体框覆盖的比例 0~1，无人体证据传 0
     * @param    {bool} bWaitWarm：OCR 那颗模型此刻没驻留、但**再等一拍就会到**（引擎判，见下面那条注）
     * @param    {CBoardCapture_S&} rstOut：OUT=bWant 时带源帧像素框与到期时刻
     * @return   {bool} 等价于 rstOut.bWant
     * @note     : 副作用是推进在途标记与计时，因此**每拍只准调一次**（引擎节拍里）
     *             bWaitWarm=true 时**不消费这次机会**（不推到点表、不清挂着的边沿），只计一列 warm=：
     *             引擎的暖组排在 boardStep 之前，而板书边沿是本拍 ruleStep 才发出来的，
     *             所以每个边沿的第一拍必然碰上"模型还在来的路上"。让它照样到点，
     *             等于把每条板书记录固定丢一次（表现为 ep 与 want 一起涨而 cap 不动）。
     *             本参数不由本类自己判"驻留没驻留"，因为「没驻留」有两种：还在 load（等）
     *             与永远不会有（缺件/预算被拒，照旧往下走，把 unav= 与 st= 报出去）。
     *             这一格分错方向的代价是"板书整节课一条不进而日志里没有一行解释"
     */
    bool wantCapture(int64_t llNowMs, bool bGeomReady, float fOccludeRatio, bool bWaitWarm,
                     CBoardCapture_S &rstOut);

    /**
     * @brief   : 登记"这一次采集已经交给后端了"，带上后端给的作业号
     * @param    {uint32_t} u32Seq：0 以外的值。0 是"还没投出去"的哨兵，不是作业号
     * @return   {bool} false=登记不上（这一拍没有在途采集，或传进来的是 0）。登记不上时，
     *             那张图的结果回来必然配不上号而被判迟到，所以引擎不必接这个返回值
     * @note     : 为什么必须登记（#64 第 4 刀）：一次板面识别是"1 次 det + N 次 rec"，
     *            几十到几百毫秒，最坏比在途超时（默认 10s）还久。那一刻本类会强制放行
     *            下一次采集，而后端手里那张旧图**还在跑**——不配对的话，旧作业的结果会
     *            顶着新一次采集的名义写进报表，两个读数都合法而内容属于另一块板子。
     *            在途配对只有这一格（信箱本来就只有一格），所以它不在引擎里再记一份
     */
    bool markCaptureSubmitted(uint32_t u32Seq);
    /* 在途那一次采集的作业号（0=还没投出去）。只给判据与心跳用，不改账 */
    uint32_t flightSeq() const { return m_u32FlightSeq; }

    /**
     * @brief   : 通知一次「板书结束」边沿（教师面向黑板 → 转回面向学生）
     * @param    {int64_t} llEndMs：边沿时刻（单调毫秒，取规则层那条 END 事件的时间）
     * @note    : 本类**不认识行为枚举**，边沿由引擎喂进来——朝向判据住在规则层（背身 = 面部证据
     *            不可见，镜像无关），板书记录的节奏住在感知层。两边各自 owned 自己的事实，
     *            在这里只留一根针脚。请求挂着时再次通知按一次算（合并计数），
     *            因为"转过去又转回来两次而黑板只采得了一次"
     */
    void notifyEpisodeEnd(int64_t llEndMs);
    bool episodePending() const { return m_bEpisode; }

    /**
     * @brief   : 喂一拍「教师是否站在黑板区域里」的事实（#93-4：侧身板书的第二条边沿）
     * @param    {bool} bInBoard：当前教师（归属仲裁承认的那位）此刻头框在黑板区域内
     * @param    {int64_t} llNowMs：单调时基
     * @note    : 站住的事实由引擎喂（它认得"谁是教师"），起止由本类记——
     *            与 notifyEpisodeEnd 同一条纪律：本类不认识行为枚举，也不认识区域表，
     *            只认"进/出"这根针脚。bInBoard 连续站满 llStayExitEdgeMs 后**离开**那一拍
     *            发一次与 notifyEpisodeEnd 同规格的结束边沿（进→出的下降沿才发，
     *            站着不动期间不发：站着时黑板本来就被挡着，采了也是"少了半行的同一块板"）。
     *            教师换人/消失时按"离开"记账——停留锚点随事实一起断
     */
    void notifyTeacherStay(bool bInBoard, int64_t llNowMs);
    /**
     * @brief   : 板书这一路此刻"有事"吗：有一次挂着的采集请求，或有一次采集在后端手里
     * @note    : 只给引擎的暖组保留期用（warmGroups 每拍读一次）。为什么本类不自己管驻留：
     *            模型驻留是内存账，归池子与预热策略；本类只管"什么时候该采一张"。
     *            为什么 in-flight 也算：一次板面识别是 1 次 det + ≤N 次 rec，后端每次推理
     *            各取一次租约、没有作业级的持有，中途把租约放掉就是自己掐掉这次识别
     */
    bool captureBusy() const { return m_bEpisode || m_bInFlight; }

    /**
     * @brief   : 回填一次采集结果
     * @param    {int64_t} llDueMs：wantCapture 给出的那次到期时刻（时间戳按它记，不按回填时刻：
     *             回填可能晚 1~2s，而板书的先后顺序对报表有意义）
     * @param    {uint32_t} u32Seq：这次结果所属的作业号；**当场回填的失败传 0**（那次采集
     *             根本没走到后端，没有号可配）。非 0 而与在途那一次不符 ⇒ 判迟到，丢弃并计
     *             u64ResultStale，且**不清在途标记**（那格账是别的那次采集的）
     * @param    {bool} bOk：后端是否给出了可信结果（false 含"能力不可用"）
     * @param    {BackendState_E} enState：bOk=false 时说明原因，供 unavailable 标记
     * @param    {const std::vector<CBoardText_S>&} vecText：识别行（bOk=true 且空=真·空板）
     * @param    {int64_t} llLastJobMs：#64 第 10 刀——引擎顺手带进来的后端"最近一次成功作业耗时"。
     *             **-1=这一拍没有新读数**（当场回填的失败、还没投出去 ⇒ 后端计数器本来也没动），
     *             保持上一次的值；由引擎在 takeResult 那一拍传真值（本类不跨线程读后端）。
     * @param    {uint64_t} u64TruncTotal：后端 DB 解码累计截掉的行数；**UINT64_MAX=没有新读数**，
     *             同上理由。传真值时它是单调累计，直接覆盖即"截至这一拍后端一共截了几行"。
     * @return   {bool} true 表示产生了一条新记录
     */
    bool onOcrResult(int64_t llDueMs, uint32_t u32Seq, bool bOk, BackendState_E enState,
                     const std::vector<CBoardText_S> &vecText,
                     int64_t llLastJobMs = -1, uint64_t u64TruncTotal = UINT64_MAX);

    /**
     * @brief   : 取一条待发记录（引擎按上报节奏消费）
     * @return   {bool} false 表示没有
     */
    bool pop(CBoardItem_S &rstItem);

    /* 会话收尾时清在途标记与待发计数（不清已产生的记录：收尾前要先让引擎取完） */
    void closeSession(int64_t llNowMs);
    void reset();

    CBoardStat_S stat() const { return m_stStat; }
    int32_t pendingNum() const;
    /* 最近一次采集的可用状态，供 metrics 标 unavailable */
    BackendState_E lastState() const { return m_enLastState; }
    std::string statText() const;

private:
    /* 行集合哈希：逐行 FNV-1a 后排序再合并，行序无关（见文件头第二条） */
    static uint64_t hashOfSet(const std::vector<std::string> &vecLine, int32_t nNum);

    CBoardParam_S m_stParam;
    IOcrBackend  *m_pBackend = nullptr;

    /* 源帧像素框：由归一化框 × 源分辨率得到，setBoardRectNorm 里算一次 */
    CRectf_S m_stRectSrc;
    bool     m_bRectReady = false;

    bool     m_bInFlight = false;
    int64_t  m_llFlightSinceMs = 0;
    int64_t  m_llFlightDueMs = 0;
    /* 这一次采集在后端那一格的作业号，0=还没投出去（见 markCaptureSubmitted） */
    uint32_t m_u32FlightSeq = 0;
    /* 挂着的一次板书采集请求（由 notifyEpisodeEnd 置上，wantCapture 消费掉）。
       刻意做成"一次"而不是计数：连着转过去两次而没采成，第二次的内容覆盖的就是第一次的板，
       数两次只会让 ep 与 want 对不上而没人能解释 */
    bool     m_bEpisode = false;
    int64_t  m_llEpisodeMs = 0;        /* 这次边沿的时刻，采集请求拿它当时间戳（不是回填时刻） */
    /* 区域停留边沿的锚点（#93-4）：教师进黑板区域那一拍置上，离开/换人/关功能清掉。
       0 = 没有挂着的停留（时间基上 0 是过去，与"刚开机"歧义由事实侧保证——
       真课堂里开机第一拍教师就已在板前站满阈值的情况可忽略，漏一条边沿代价是少采一张） */
    int64_t  m_llStaySinceMs = 0;
    /* 教师此刻是否站在黑板区域里（notifyTeacherStay 每拍刷新）。
       与 m_llStaySinceMs 分开：那个锚点受 stay_exit_edge_ms 控制（配 0 就永远是 0），
       而周期重识别只需要"人在不在板前"这一个事实 */
    bool     m_bTeacherInBoard = false;
    int64_t  m_llLastDueMs = 0;         /* 上次到点时刻（不是采集时刻：被挡掉的那轮也要推表，
                                           否则遮挡 40s 后会出现一次连采两轮的抖动） */
    uint64_t m_u64LastHash = 0;
    bool     m_bHasLastHash = false;
    uint32_t m_u32Seq = 0;

    /* 待发记录环：定长、构造期一次分配，运行期只挪下标（§1.2）。
       第 7 刀之后 CBoardItem_S 是 POD，这一格连 std::string 都没有了；
       本类里唯一带字符串的是下面那张临时表（一次采集一行，节奏 30s，不是每拍热路径） */
    std::vector<CBoardItem_S> m_vecItem;
    int32_t m_nHead = 0;
    int32_t m_nNum = 0;
    int32_t m_nCap = 0;

    /* 行哈希的临时表：构造期 reserve 到 CLASSROOM_BOARD_LINE_MAX，运行期只 clear 不缩容 */
    std::vector<std::string> m_vecLineHashStr;

    BackendState_E m_enLastState = BackendState::MODEL_MISSING;
    CBoardStat_S   m_stStat;
};
}
