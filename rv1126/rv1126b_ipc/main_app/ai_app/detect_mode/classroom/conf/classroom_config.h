/**
 * @FilePath     : classroom_config.h
 * @Author       : leiyy
 * @Date         : 2026-11-03 11:28:37
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 11:28:37
 * @Description  : 课堂配置：JSON → 不可变快照 → 原子换入
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "classroom_define.h"
#include "attendance.h"
#include "board_watcher.h"
#include "classroom_metric_log.h"
#include "duration_gate.h"
#include "expression_estimator.h"
#include "face_align.h"
#include "face_gallery.h"
#include "geometry.h"
#include "model_pool.h"
#include "ocr_decode.h"
#include "person_tracker.h"
#include "region.h"
#include "report_spool.h"
#include "seat_heat.h"
#include "seat_prior.h"
#include "scene_infer.h"

namespace Classroom
{
/* 课堂配置文件默认落点：统一走 USER_DATA_PATH，不引入第二套根路径 */
#define CLASSROOM_CONFIG_FILE_NAME "classroom_config.json"

/**
 * @brief 采样与性能相关参数
 */
typedef struct _SampleConf_
{
    int32_t nHeadIntervalMs = 200;   /* 人头检测周期，0 表示每帧 */
    int32_t nPoseIntervalMs = 1000;  /* 单个人姿态周期：5fps×每帧全人必爆 NPU */
    /* 下面这一项目前只解析、夹取，没有任何消费者：人脸流水线是 P5 的入口，键位先占住是为了
       Web 侧下发的字段口径一次谈定，不必到实现那轮再改下发协议（与已删掉的 snapshot_enable
       的区别就在于此——那个没有排期）。board_interval_ms 曾在同一列，P6d 落地时删掉了它：
       板书那条路真正的旋钮是"事件触发 + 重入下限"，与这里的"周期"不是同一个语义，
       留一个没人读的周期键比少一个键更危险（现场会去调它并期待有现象） */
    int32_t nFaceIntervalMs = 2000;  /* 人脸流水线周期（考勤/表情共用） */
    int32_t nMaxPosePersons = 12;    /* 一个周期内最多做几个人姿态，按面积优先 */
    /* 群体指标的采样周期。与 egress.report_interval_sec 是两件事：
       本项决定**账本的时间分辨率**（采样间隔就是时长积分的步长，定粗了"站立 3 分钟"
       会被算成 2.5 或 3.5），上一项决定**多久发一次报表**。两者各调各的，互不换算 */
    int32_t nGroupIntervalMs = 5000;
    /* 一拍里给姿态的毫秒预算：本拍人数 = 该值 / 实测单次推理耗时（EWMA）。
       与上一项的分工是「一拍」与「一个周期」两级闸门——只配周期配额挡不住一拍里连做四个人
       把整拍吃穿（人头检测因此延后一拍，人数指标跟着抖） */
    int32_t nPoseBeatBudgetMs = CLASSROOM_POSE_BEAT_BUDGET_MS;
    int32_t nNpuBusyHighPercent = 75;/* NPU 占用超此值则自动拉长周期（§4.4） */
    int32_t nNpuBusyLowPercent = 45; /* 低于此值才允许把周期收回来（迟滞，防抖） */
    /* 一次推理最多等多久（毫秒）。节拍线程是同步等 NPU 的，没有上限就等于
       把「一次节拍要多久」交给驱动调度决定——降级、掉帧、心跳全都不再可控 */
    int32_t nInferWaitMs = CLASSROOM_INFER_WAIT_MS;
} CSampleConf_S;

/* 聚合上报周期的**上限**（秒） */
#define CLASSROOM_REPORT_INTERVAL_MAX_SEC (60)

/**
 * @brief 输出侧参数
 */
typedef struct _EgressConf_
{
    bool    bOsdEnable = true;          /* 画框总开关 */
    /* 这里刻意没有 snapshot_enable / jpeg_quality：本版本的需求表里没有一条要求抓拍图片，
       而"配了没有任何反应"的旋钮正是现场最难查的一类问题——花时间调完，结论是自己配错了。
       抓拍真做时，键随实现一起加 */
    int32_t nReportIntervalSec = 60;    /* 聚合上报周期；上限见上（吞吐不变量，60s） */
    /* 会话最长时长：超过就强制收尾出一次会话报表并清零账本。
       没有这个上限时，「课堂结束」只来自 enable 置 false 那一条边沿 —— 现场没人按开关、
       或者那条下发丢了，会话账本就会一直开着：平台侧永远等不到会话报表，而设备侧
       的 present_s / person_s 会涨到连日志都读不出意义。6 小时是常规最长课时的一档余量 */
    int32_t nSessionMaxSec = CLASSROOM_SESSION_MAX_SEC;
    /* 会话静默收尾（#95）：连续这么久没有人「在场」（样本人数 > 0）就自动收尾出会话报表。
       兜的是「下课了没人关 enable」之外的第二种沉默——人走光了帧照来，max_sec 要再等几小时。
       0 = 关。收尾还要求样本仍在流动（流死了不归它管，见 classroom_define.h 的口径注释） */
    int32_t nSilenceSec = CLASSROOM_SESSION_SILENCE_SEC;
    /* ---- 断网滞留----
       取代原先的 event_keep_sec / retransmit_max_num：那两个键描述的是"内存里留多久、留几条"，
       而内存滞留环在断电瞬间就什么都不是了，参数也就没人读。落盘之后滞留能力按**空间**计，
       不按时间计——"留 5 天"这种口径在 32MB 上等于"看运气"，不如直接给容量 */
    bool        bSpoolEnable = true;
    std::string strSpoolDir = CLASSROOM_SPOOL_DIR;
    int32_t     nSpoolMaxMb = CLASSROOM_SPOOL_MAX_MB;
    int32_t     nSpoolSegMaxKb = CLASSROOM_SPOOL_SEG_MAX_KB;
    int32_t     nSpoolDrainMaxKb = CLASSROOM_SPOOL_DRAIN_KB;
    /* ---- 心跳落点 ----
       可回溯时长 = 环大小 ÷ 总写入速率，这两个数一个是分子一个是分母，所以旋钮只有这两个：
       metric_max_kb×metric_max_files 定分子，head_period_ms 决定往业务环里回抄多密（分母的一部分）。
       刻意不给"心跳详略"这类开关——折叠字段省的是份额，不解决与业务日志抢同一只环的问题 */
    bool        bMetricOwnFile = true;
    std::string strMetricFile = CLASSROOM_METRIC_FILE;
    int32_t     nMetricMaxKb = CLASSROOM_METRIC_MAX_KB;
    int32_t     nMetricMaxFiles = CLASSROOM_METRIC_MAX_FILES;
    /* 紧凑头回抄进业务日志的周期。0 = 完全不回抄：业务日志里从此看不到课堂读数，
       这是合法选择（现场只按 stream.log 排障的型号），不是"配置漏填"，
       所以夹取只在负数时兜底，不把 0 抬成默认值 */
    int32_t     nMetricHeadPeriodMs = CLASSROOM_METRIC_HEAD_MS;
} CEgressConf_S;

/**
 * @brief 平台统计推送
 */
typedef struct _StatsConf_
{
    bool    bEnable = true;             /* 总开关；关掉后整条 0x62xx 推送不产生 */
    /* 累计计数（出勤/动作/表情/预警计数）的推送周期。10s 是"平台面板上的数看得见在动"
       与"总线不被统计报文占满"的折中：一节课 40 分钟约 240 条，远低于事件通道的量 */
    int32_t nPushIntervalMs = 10000;
    /* 行为段（教学行为/学生行为，带起止时间）的批量推送周期。段本身是边沿驱动的，
       攒 2s 再发是为了让一次"起立→坐下"这种短段不必各发一包；上限条数见 nSegBatchMax */
    int32_t nSegBatchMs = 2000;
    int32_t nSegBatchMax = 24;          /* 单条报文最多几段，受协议固定数组长度约束 */
    int32_t nAlertBatchMax = 8;         /* 单条报文最多几条预警（协议数组长度） */

    /* ---- 下面这组是"没有课表时怎么判教师/学生考勤"的会话内口径 ----
       本项目拿不到排课表，所有时间都相对「本会话第一份有学生的样本」：
       迟到宽限、拖堂宽限、早退判定都按这个基准推。现场若接入课表，
       改的是这几个数而不是判据本身 */
    int32_t nLessonMin = 45;            /* 标准课时长（分钟），拖堂从这里起算 */
    int32_t nArriveGraceSec = 300;      /* 开课后多久还没教师 = 未到达（一节课内可等） */
    int32_t nOverrunGraceMin = 5;       /* 超过 nLessonMin 再等这么久 = 拖堂 */
    int32_t nLeaveAbsentSec = 300;      /* 教师离开持续这么久且未到下课 = 早退 */
    int32_t nPatrolIntervalMin = 15;    /* 多久没有巡视行为 = 未巡视（0 = 不判） */

    /* 学生参与度：群体注意力比例低于 pct 且持续 hold 秒 → 一条预警 */
    int32_t nParticipateMinPct = 50;
    int32_t nParticipateHoldSec = 300;
    /* 课堂纪律：低头+背身+玩手机占在场人数的比例超过 pct 且持续 hold 秒 → 一条预警 */
    int32_t nDisciplineBadPct = 30;
    int32_t nDisciplineHoldSec = 180;
    /* 学生聚集：至少 num 人**站立**且彼此头距在半径内持续 hold 秒 */
    int32_t nGatherNum = 4;
    float   fGatherRadiusHeads = 2.5f;  /* 聚集半径按头宽计（不按像素：远近焦头大小差几倍） */
    int32_t nGatherHoldSec = 60;
    /* 学生趴桌：单人连续低头超过这么久（低头门 3s 只判"此刻低头"，
       趴桌是"一直没抬头"，所以单独给一个远大于门的时长） */
    int32_t nDeskHoldSec = 300;
} CStatsConf_S;

/**
 * @brief 主体归属仲裁参数（P4：先回答「谁是老师」，才谈得上教师那 13 列行为）
 * @note  这一组数描述的是**归属**而不是行为：教师行为本身的阈值在 vecGates 与各 rule 结构里。
 *        分开成一块是因为两者的调参主体不同——归属错了整段行为都不存在（现场表现为
 *        "教师面板全 0"），而行为阈值错了只是某一条不准；混在 thresholds 里会让运维
 *        在第一类故障上去调第二类的旋钮
 */
typedef struct _TeacherConf_
{
    bool    bEnable = true;
    /* 证据要连续攒够多久才认作教师。取 5s 是"老师走上讲台并站定"的量级：
       再短，路过讲台的学生会被认成教师（而教师一被安到学生头上，那个人的学生行为同时停止记账）；
       再长，一节课 40 分钟里老师上下讲台各几次，头几次归属全落在行为窗口之外 */
    int32_t nConfirmMs = 5000;
    /* 证据断了多久才收回身份。必须显著大于 nConfirmMs：
       两边相等时一次遮挡就是一次"下课+上课"，同一个人被劈成两段教师，
       而平台侧看到的是"这节课有 4 位老师"。取 20s ≈ 老师转身写板书、背对镜头那几拍 */
    int32_t nReleaseMs = 20000;
    /* 同时承认的教师数上限。默认 1 是这间教室的真实形态（一师多生），
       也是本类"身份是互斥资源"这条纪律的落点：给多大都行，但挑战者要自己攒够确认时长 */
    int32_t nMaxNum = 1;
    /* 头框落在讲台/黑板区域内的最小比例。与逐行为区域闸同一个默认值（同一把尺），
       单独开一个键是因为归属这道闸比单条行为更宽——老师半个身子探出讲台仍然在讲课 */
    float   fCoverage = CLASSROOM_REGION_MIN_COVERAGE;
    /* 「巡视」这条弱判据要求的窗口内累计位移（分析帧像素）。
       与 threshold.edge_band_px 同为"像素口径"而不是"框宽比例"：位移量的是走了多远，
       而人头大小随距离变化最剧烈，用比例会在远焦端把走动量缩没。
       120px 在 1920 宽画面上约等于一个人宽：站着不动的抖动累不出这个数 */
    int32_t nPatrolMovePx = 120;
    /* 位移统计窗口：超过这个窗口就归零重开。取 5s 与 nConfirmMs 同量级——
       位移是用来否决"同步起立"的，窗口比确认时长还长时，一次课间走动就够终身受用 */
    int32_t nPatrolWindowMs = 5000;
    /**
     * @brief 讲台/黑板证据的姿态证人窗口（毫秒），0=关（默认，T38）
     * @note  头检在深色板面会输出 ~300px 的幻影大头（其上 FastPose 拿不出人），
     *        无证人时它会以"最早候选"赢下真教师 track churn 后的空槽，把 204/208
     *        整段封死（ttea7 实测持有教师槽 2.5min）。开着时：区域证据只认"本轨
     *        见过一次可用骨骼"的行（ever-sticky），真人只晚 ~1s 晋升。
     *        取 0 为默认是失败安全：新判据不上电，出厂 conf 显式打开才生效 */
    float   fPoseWitnessTtlMs = 0.0f;
    /**
     * @brief 证人"自家头对账"余量（头框边长的百分比），0=关（默认，T38 v3）
     * @note  expand 裁剪会把邻近真人圈进同一张裁剪图：ttea9 的板面小幻影
     *        （120-213px，巨框闸拦不到）借到邻座学生的骨骼过了证人，照样夺槽
     *        126s/76s（抽帧目验框内是板面无人）。开闸后证人只认"骨骼头点（鼻/头顶）
     *        落在本行头框外扩本余量内"的骨骼，借来的骨骼永远等不到 latch。
     *        余量给头框的百分之多少要靠证人评估读数行定（ttea10 先 0 关闸只读数）；
     *        取 0 为默认是失败安全：关位=旧形态（首次可用骨骼即 latch） */
    int32_t nWitOwnHeadPct = 0;
    /**
     * @brief 证人 latch 的骨骼得分下限，0=关（默认，T38 v4）
     * @note  ttea10 测量跑（pct=0）证伪了"借来的骨骼错位明显"：板面幻影借到
     *        的骨骼近对齐（鼻/头顶偏移 ≤0.7 框，头顶常落在框内），余量闸切不开
     *        它与真人的分布。276 条证人评估读数给出的可分面是**骨骼得分**：
     *        中心板幻影（133.7s 夺槽）的全部 latch 评估 0.35-0.44，真人最差 0.53
     *        （讲授/板书全程 0.53-0.88），0.44 与 0.53 之间是可行窗，0.50 居中。
     *        现场值要靠证人评估读数行（骨得分列）按素材重定；取 0 为默认是
     *        失败安全：关位=旧形态（只要头点对上就 latch） */
    float   fPoseWitMinScore = 0.0f;
    /**
     * @brief 「静止指导」弱证据的停留时长（毫秒），0=关（默认，#93-4）
     * @note  补的是巡视判据漏的那一类：老师站在学生区**不走动**地指导（俯身看作业、
     *        站在某个小组旁边讲解）。位移判据只认"走过"，静止恰好反着来。
     *        直接放开会误伤两类场景，所以这条证据带三重闸（判据本体在 subject_arbiter）：
     *        讲台区必须空（讲台有人时站着的是真老师候选，不需要这条）、
     *        学生区站立者必须恰好 1 人（全班起立问好/齐读时人人站立，谁都不算）、
     *        连续停留满本数（走过场的瞬时站立攒不出来）。
     *        取 0 为默认是失败安全：新判据不上电，现场显式打开才生效 */
    int32_t nStayEvidenceMs = 0;
    /**
     * @brief 槽缺席容忍（毫秒），0=关（默认，T43）
     * @note  追踪器出参刻意不输出丢失拍（person_tracker.cpp「丢失中的不输出」防人数虚高），
     *        而回收循环原本缺一拍就静默收槽：头检在贴边写板/暗板面丢一拍，
     *        教师身份即碎，回来还要付满 confirm_ms 重新晋升。容忍期内槽不动，
     *        事件连续性仍由各时长门的 gap 容忍管——本值只保身份不保事件。
     *        取 0 为默认是失败安全：0=旧行为（缺一拍当拍收），现场显式打开才生效 */
    int32_t nAbsentHoldMs = 0;
    /**
     * @brief 教师重入快轨的记忆窗口（毫秒），0=关（默认，T44）
     * @note  hold 只保旧槽保不了身份：站立教师被检测漏检后跟踪器换 ID，
     *        继任从零攒 confirm_ms，这几秒里他按学生分类站在讲台上出 110（tt19 残余
     *        11 条 stu_on_podium 假阳性的全部成因）。本值 >0 时，教师身份丢失留一条
     *        记忆（谁/何时/最后位置）；本拍刚开始攒证据的候选若讲台/黑板强证据在身、
     *        头框落回记忆位置 reentry_px 半径内、且在窗口内——立即继承，不重攒确认。
     *        弱判据攒的候选不快轨（全班起立问好时位移最强的学生不能借道上位）；
     *        max_num 上限下调收回的身份不写记忆（运维收权不许被快轨顶回去）。
     *        取 0 为默认是失败安全：0=旧行为（回来重走确认），现场显式打开才生效 */
    int32_t nReentryMs = 0;
    /**
     * @brief 教师重入快轨的位置连续半径（分析帧像素），默认 600
     * @note  tt19 实测的换手位移：1.8~4s 的短断档 Δx ≤126px，9~15s 的长断档
     *        Δx 26~94px——站立教师沿讲台巡走的速度量级。600px ≈ 1/3 画宽，
     *        覆盖一个确认时长内的巡走位移，同时把"另一个人走到黑板前"（Δ≈760px）
     *        挡在外面。配 0 = 只认原地重入（几乎不触发），现场按机位重定 */
    int32_t nReentryPx = 600;
} CTeacherConf_S;

/* 底库文本文件的默认落点：与报表滞留同一块可写分区。
   刻意不写 /userdata：机上实测那一段没有挂载，
   把默认值放在一个必然不存在的目录里，等于让考勤这条链出厂即死且没有原因 */
#define CLASSROOM_GALLERY_FILE_NAME "/opt/course/data/classroom/gallery.txt"

/**
 * @brief 人脸底库（点名册）来源
 * @note  一份底库、两种来源，且**同时只激活一种**：底库同时从文件与库表读会出现
 *        "同一学生在两边各有一行、分数不同"，比谁赢取决于遍历顺序，这种不确定性
 *        在验收线（误识 ≤1%）上是不可接受的。切换在交付侧只需改 source 一个键
 */
typedef struct _GalleryConf_
{
    bool    bEnable = true;
    /* "file"=文本底库；"sqlite"=平台那套 FaceSqlite.db；
       "none"=明确不做比对（到课人数照出，只是名单没名字）。
       默认 file 而不是 none：sqlite 那条在今天的机上
       连写入方都没有——拿一个读不出东西的来源当默认，等于让验收靠人记住"还要改一个键" */
    std::string strSource = "file";
    std::string strPath = CLASSROOM_GALLERY_FILE_NAME;
    /* sqlite 模式下的名单组名。空 = 全部名单组（平台侧"跨组考勤"的形态）。
       这一项会拼进 SQL，故配置装载时按白名单校验字符集（见 galleryTableOk） */
    std::string strTable;
    /* 底库文件的检查周期（秒）。取 30s：名单是开课前一次性下发的东西，
       再短只是每拍 stat 一次；再长，现场"改了文件没反应"会等一分钟才确认自己是否白改 */
    int32_t nReloadSec = 30;
    /* 载入行数上限（内存闸门，非功能闸门）。超限只计数不载入，见 CFaceGalleryStore::add */
    int32_t nMaxEnroll = CLASSROOM_GALLERY_MAX_ENROLL;
    /** 人脸检测区域。
     *  落在区域外的人脸不做对齐/取特征/表情——这是"这里根本不是学生区"的显式表达
     *  （例如镜头带进走廊或讲台），省的是 NPU 与误识，不是判定口径。
     *  与考勤区域（attendance.region_ids）分开：那一项管"算不算到课"，这一项管"做不做" */
    std::vector<int32_t> vecRegionIds;
} CGalleryConf_S;

/**
 * @brief 人脸特征段（对齐 → 重采样 → FaceFeature → 比对）的节拍与容量
 * @note  与 CAttendParam_S（判定门限）、CSampleConf_S（检测周期）三块各管一段：
 *        这一块的数全部是"花多少钱"，改它不改变判定结果，只改变覆盖率与节拍占用
 */
typedef struct _FaceFeatConf_
{
    /* 关掉就是"有脸、有底库、但不出名字"。留这个阀是因为取特征是整条考勤链里
       唯一同时吃 NPU（一次推理）与 CPU（一次重采样 + 一次全库遍历）的一段，
       小内存机型上内存吃紧时需要一个只关它、不关人脸检测的档位（人数指标还要用） */
    bool    bEnable = true;
    /* 一轮最多做几张脸。8 的算法：一次推理约 30ms、重采样约 0.2ms，
       8 张就是 250ms 量级——已接近一个节拍，再大就要把人脸段拆到跨拍。
       与 sampling.max_pose_persons 同构，但独立配：脸比骨骼便宜，配额不必一样 */
    int32_t nMaxPersons = 8;
    /* 每拍最多做几张脸（stOpt.bFeatPerBeatEnable=true 时生效）：把一轮 8 张的 NPU 峰值
       摊到多拍上，人脸拍的节拍占用从 532ms 量级降到 ~165ms。取特征/表情/质量三段
       共用一张 warp 缓冲，这个数只改变"一拍做几张"，不改变三段的先后与缓存口径；
       轮转一圈的时长由 faceFeatIntervalMs 按 (脸数 ÷ 本值) 自动重推（60 人 ÷ 3 张/拍
       = 20 圈 × 2s = 40s，比 8 张/拍的 16s 慢——考勤出名字变慢是这条档位的已知代价）。
       钳制上界取 nMaxPersons（它仍是容量口径），下界 1 */
    int32_t nPerBeatPersons = 3;
    /* 同一人两次取特征的最小间隔。0 = 按「人脸检测周期 × 轮转圈数」推导
       （见 face_feature.h 的 faceFeatIntervalMs）。给 0 当默认是因为圈数随人数变，
       写死一个数会在 20 人班和 60 人班里一个太松一个太紧 */
    int32_t nMinIntervalMs = 0;
    /* 一份特征在表里活多久。必须大于轮转一圈的时间，否则前排的特征会在
       轮到自己第二次之前先被清掉（读数上是 pend 恒高、名单迟迟不出）。
       30s 在 60 人 × 8 张/轮 × 2s 一轮 = 16s 一圈的量级之上留一倍余量 */
    int32_t nTtlMs = 30000;
} CFaceFeatConf_S;

/**
 * @brief 表情/情绪（对齐人脸 → FaceExpression → 逐人平滑）的开关与存活期
 * @note  判定门限直接用 `CExpressionParam_S`，不在配置层再抄一份同名字段（与 stAlign、
 *        stAttend 同一条理由：两处各存一份就要有人负责同步，漏同步的表现是"改了配置表情不认"）。
 *        本块里剩下的两项都是**口径**而不是阈值，故不进那个结构
 *
 *        ★ 为什么这里没有"每几秒做一次、一次做几个人"这一档：表情吃的是**特征段刚造出来的那张
 *        对齐图**（框架图组合4「复用组合2 的对齐人脸」那条虚线边的实现）。它没有自己的裁剪与
 *        重采样，因此也就没有独立节拍——做一次特征就顺带判一次情绪，某个人的情绪刷新周期
 *        就是 `face_feature.ttl_ms`。要更密的曲线就调那一档，代价是全班的取特征一起变密
 *
 *        ★ 与 `face_feature.enable` 的上下游关系（现场最容易配反的一处）：关掉取特征，
 *        这张对齐图就不存在，本块开着也不会出数。`face_feature.enable=false` 时这里应同时关，
 *        否则读数上会出现"表情恒 try=0"而配置里明明是 true
 */
typedef struct _ExpressionConf_
{
    bool bEnable = true;
    CExpressionParam_S stParam;
    /* 一个人的平滑向量在没人喂多久之后整行清零。取 60s = 两个特征 TTL：
       比那个值短的线会让"轮转还没轮到他"被误当成"这个人消失了"，
       曲线因此每圈断一次，而断档在平台上是"没情绪"（比画错一条线更难解释） */
    int32_t nTtlMs = 60000;
    /* 112 事件证据的新鲜度。**与 nTtlMs 分家**：
       nTtlMs 管的是"平滑行活多久"（大班轮转一圈可能 40s，短了曲线会缺人），
       这一项管的是"最后一次判得出的结论还能不能当事件证据"。
       事件若借 nTtlMs，一个人被挡/离开后旧表情最多还能撑 60s，段长被拖长。取 12s≈两个取特征周期：
       一次漏采样不断事件，连续两次没结论就收尾。0/非法由配置层钳到 [1000, nTtlMs] */
    int32_t nEventTtlMs = 12000;
} CExpressionConf_S;

/**
 * @brief 玩手机复核（PlayPhoneDetect）的按需档位：本块全是"什么时候花这一次 NPU、花几次"
 * @note  与 rules.phone（判定门限：带、肘角、腕距）刻意分成两块，理由与 stAttend /
 *        stGallery 那一对相同——改了那块会改变"低头×手在身前算不算玩手机"，
 *        改了这块只改变"要不要为这个人再裁一张图"。调复核成本时不该碰到判据
 *
 *        为什么这里没有 score_threshold：复核框的置信度门与头/体/脸三路共用同一个
 *        threshold.det_score（同一颗镜头、同一套曝光，多一个键就多一处配错的地方）。
 *        它与本条链的关系还有一层不对称：漏检一个手机框=否决一次真事件（错杀），
 *        误检一个=只是"没否决"（与不复核同果）。所以把 det_score 往低调是安全的，
 *        往高调会静默吃掉 106——这条口径写在 _notes 里让运维看得见
 */
typedef struct _PhoneRecheckConf_
{
    /* 关掉 = 只按规则判（低头×手在身前），模型连被要都不被要：G_OTHER 的需求直接断掉，
       这一项是"内存吃紧时把复核摘掉、但 106 还出事件"那一档（与 face_feature.enable 同构） */
    bool    bEnable = true;
    /* 一拍最多复核几个人。取 2 的量级：一次裁剪+一次 640×384 推理约 25~30ms，
       两拍之间还要塞下人头检测与姿态轮询。上限由人数钳，配到 60 只是把整拍吃穿 */
    int32_t nMaxPersons = 2;
    /* 同一个人两次复核的最小间隔。取 1000ms 与结论窗口(3s)的比是 1:3——
       一次复核撑得住三拍判定，又不至于让人低头十秒只被看过一次 */
    int32_t nMinIntervalMs = 1000;
    /* 一次复核结论在表里活多久。默认与规则层读证据用的 CLASSROOM_PHONE_TTL_MS 同值：
       两侧不同值会出现「感知层还认为结论有效、判定层已经当没结论」，现象是事件一卡一卡 */
    int32_t nTtlMs = CLASSROOM_PHONE_TTL_MS;
    /**
     * @brief 复核框的短边下限（分析帧像素），0 = 不设这道闸
     *
     *        默认关着而不是给一个"看起来合理"的 8：这道闸吃掉的每一个框，语义上都会被记成
     *        "看了，没有"——也就是一条否决。拦掉噪声小框本身没错，错在"8"这个数没标过：
     *        拿一个没标过的数去否掉一条真行为，方向是反的（漏检=错杀，误检=只是没否决）。
     *        真框尺寸分布要在实机上量出来才配得动它，所以它保留为现场旋钮而默认不生效
     *
     *        "那小框怎么办"不靠这里回答：后排那种十几像素的人**整行不参与复核**，
     *        那道门取的是 threshold.min_body_px（同一个"这么小的框我们不信它是个人"的口径，
     *        见 phone_recheck.h 的 nMinBodyEdge），不在这块里再配第二个尺寸键
     */
    int32_t nMinBoxPx = 0;
} CPhoneRecheckConf_S;

/**
 * @brief 群体与展示类行为（107 听讲 / 108 阅读 / 109 讨论 / 110 上讲台 / 111 演示）的判据参数
 * @note  三档朝向读数（听讲看讲台、讨论看邻座、演示看学生席）都是同一个 `judgeFacingRef`
 *        的三个实例，共用 `headYawNorm` 那一个数——分开各写一份判据，同一颗头上就会出现
 *        三个互相矛盾的"朝哪儿"（与 `rules.phone` 只判"手"那一半是同一条切分）
 *
 *        **为什么这几对数是比例不是角度**：26 点里没有可靠的下颌/耳轴，能拿到的只有
 *        「鼻相对双眼中点的横偏移 ÷ 肩宽」。它单调、镜像无关（见 judgeFacingRef 的论证），
 *        但不是度数，所以 CSV 第 16/18 条写的「头部朝向阈值 60°」在这里对应的是一个
 *        未标定的比例门限；把 60 抄成 60.0f 会得到一个恒真的判据
 *
 *        **108 阅读为什么这块里没有参数**：它的形状与 106 完全相同（低头 ∧ 手在身前带内），
 *        分家依据只有那一次复核的结论（见 `phoneUsable()` 与采样器第 8 步）。再加一组
 *        带与肘角，等于承认"看书时手的形状和玩手机时不一样"这个我们没有证据的假设
 *
 *        **三道人数阈值默认 1/1/2**：听讲与阅读按"单人也记"（默认表那条注释），
 *        讨论按 CSV 第 18 条的「最少参与人数」至少两人。它们是**时段**这一层的闸
 *        （"这一段课里有没有发生"），不吃逐人事件的判据，所以调它不会改变任何一个人的
 *        行为判定，只改变报表上那三个时段成立与否——与 stPhoneRecheck 的成本档同构
 */
typedef struct _GroupBehaviorConf_
{
    /* 107 听讲：头朝讲台。收得宽（略偏向反侧也算朝）是刻意的——这一列的 FALSE 才是信息
       （转过头去），而中间带太窄会让远焦端整排人在 UNKNOWN/TRUE 之间抖 */
    FaceDirRule_S stListenDir  = { 0.5f, -0.12f, -0.30f };
    /* 「无书写动作」这一半：手落在身前带内时不判听讲。默认改为关**：
       同批交替 A/B（student3×600，其余键不动）里这道否决单独就砍掉约 80% 的听讲段
       （listen 中位 6→31），而被它挡掉的大多数是"手搁在桌上、头抬着朝讲台"的正常听课姿势。
       玩手机有独立事件 106（含复核链），不该靠听讲这一列代扣；写字的人本来就低头，
       另由 103/104 与姿态判据覆盖。需要旧口径的现场把它配回 true，行为与 #98 平反链不变：
       配 true 时，若 106 的复核链留有未过期的「看过裁剪图、没有手机」结论
       （phoneUsable 且 !bPhoneObj），捧书形状撤回否决；没复核过/过期/复核说有手机仍否决 */
    bool          bListenForbidHand = false;
    /* 109 讨论：头朝邻座。反侧阈值与正侧对称：明确背对某个邻座是否决它的证据 */
    FaceDirRule_S stDiscussDir = { 0.5f, 0.12f, -0.12f };
    /* 111 演示：站在讲台上头朝学生席。与听讲同宽（面向镜头的学生就是在面向全班） */
    FaceDirRule_S stPresentDir = { 0.5f, -0.12f, -0.30f };
    /* 邻座候选的头框中心距离上限（头宽倍数，两框中心距 ÷ 本行头宽）。
       取 4：一间标准教室同桌中心距约 2~3 个头宽，4 已经盖到隔一个空位；
       再远就不是"转头说话"而是"朝全班喊"，那条判据属于讨论之外 */
    float         fNeighborMaxHead = 4.0f;
    /* 参考方向死区（头宽倍数）：参考点与本行头框的横向距离小于此值时按"没有横向参考"处理
       （fRefSign=0 → 判 UNKNOWN）。不给死区，参考点几乎正对人时符号只剩噪声，
       而这里的噪声直接决定"听讲"这一列的 TRUE/FALSE */
    float         fRefDeadbandHead = 0.3f;
    int32_t       nListenMinPersons  = 1;   /* 抬头人数阈值（CSV 16） */
    int32_t       nReadingMinPersons = 1;   /* 低头人数阈值（CSV 17） */
    /* 109 讨论的**生效计数门**：网页有两个字段（最少参与人数 min_persons ≥2、
       转身人数阈值 turn_persons ≥1）都描述它——本实现只有一个计数：朝向邻座
       成立的生效人数。两个字段各占一个配置键（Get 回读各自的值，网页整表
       回存不会互相覆盖），生效门取两者之大（两条下界同时满足）。
       合并点在这份快照生成时（parseIntoJson / validate），消费者只读这一个数 */
    int32_t       nDiscussMinPersons = 2;
    int32_t       nDiscussTurnPersons = 2;  /* 网页 turn_persons 的钳制后原文值 */
} CGroupBehaviorConf_S;

/**
 * @brief CPU-NPU 流水线优化开关组
 * @note  默认**全 false** = 行为与旧串行管线逐字节一致：这组开关是给现场灰度用的
 *        （先开 1 验证心跳计数，再逐项开 5/3/4/6，异常关对应键即回退），
 *        不是给"出厂即全开"用的——出厂 json 里也按 false 落，开着哪些由配置决定而不是代码。
 *        bSubmitAsyncEnable 是**主闸**：bFaceEarlySubmit / bPosePhoneInterleave 两条
 *        交错路径都以「主闸 && 各自开关」为生效条件，主闸关掉时整条提交-等待协议退回
 *        submitAndWait 单函数形态（行为等价、结构最旧）
 */
typedef struct _OptimizeConf_
{
    bool bSubmitAsyncEnable   = false;  /* 主闸：提交/等待拆分为 submitAsync+waitAsyncResult */
    bool bFaceEarlySubmit     = false;  /* 人脸检测提前到跟踪后、姿态前 */
    bool bFeatPerBeatEnable   = false;  /* 取特征分散多拍：每拍 per_beat_persons 人 */
    bool bPosePhoneInterleave = false;  /* 姿态/手机复核交替提交 */
    bool bUrgencyPrecompute   = false;  /* NPU 等待期预热规则层收尾 */
} COptimizeConf_S;

/**
 * @brief 不可变配置快照
 * @note  构造完成后只读；运行期通过 CClassroomConfig::current() 取 shared_ptr<const>
 */
typedef struct _ConfigSnapshot_
{
    int64_t     llVersion = 0;         /* 每次成功换入 +1，用于观测与「配置是否变了」判定 */
    int64_t     llLoadMonoMs = 0;      /* 换入时刻（单调时钟） */
    std::string strSourcePath;

    /* 出厂即开：本仓只出课堂 SKU（子系统无条件参与编译），
       配置文件缺失时「什么都不跑且没有原因」比「按默认跑、模型没到位就回报 MODEL_MISSING」
       更难在现场定位——后者会把缺件信息直接落到日志与模型状态里，可观测本身就是提示 */
    bool      bEnable = true;          /* 课堂分析总开关 */
    /* 本地联动开关：开启后课堂即时事件（207 指导 / 209 互动）在推送 TVSDK 的同时，
       也走事件联动通道（声光/邮件/录像）。默认关闭：未配置联动动作的现场不产生空联动 */
    bool      bLinkageEnable = false;
    /* 设备角色（#126）：顶层 "role" 键，"teacher"/"student"/"both"（默认 both=不分档）。
       强制关发生在 validate()，这里存的是**生效后**的值——心跳 role= 列读的就是它 */
    DeviceRole_E enRole = DeviceRole_E::ROLE_BOTH;

    /* ---- 画面几何（需求 11：画面可能镜像、旋转安装）---- */
    MirrorMode_E enMirror = MirrorMode_E::MIRROR_NONE;
    int32_t   nRotate = 0;
    int32_t   nSrcWidth = 1920;        /* AI 通道分析帧尺寸，与 VPSS 配置一致 */
    int32_t   nSrcHeight = 1080;
    int32_t   nCanvasWidth = 1920;     /* 课堂内部分析画布（缩放后）尺寸 */
    int32_t   nCanvasHeight = 1080;
    int32_t   nRegionRefWidth = 1920;  /* Web 下发区域坐标的参考分辨率 */
    int32_t   nRegionRefHeight = 1080;
    CoordSpace_E enRegionSpace = CoordSpace_E::COORD_SPACE_NORM_8192;

    /* ---- 分析区域（CSV 里几乎每条行为都带一项「XX 分析区域」）----
       存的必须是**下发口径**（Web 坐标，未换算）：换算要 CFrameGeometry，而那份几何由帧总线
       在建槽位时定死，配置层既拿不到也不该拿。判定用的是 CRegionTable（分析帧坐标），
       由规则引擎在换入快照时按本表重建 —— 中间只有那一处换算，链路上再无第二份坐标 */
    std::vector<CRegionDesc_S> vecRegions;
    std::vector<CRegionRoleRef_S> vecRegionRoles;

    /* 送模型像素的通道顺序，决定 RGA 输出 RGB_888 还是 BGR_888：
       算法组确认之前的唯一逃生阀（错配只表现为满屏 UNKNOWN，不会报错，见 ChannelOrder_E 注释） */
    ChannelOrder_E enModelChannel = ChannelOrder_E::CH_RGB;

    /* 整帧检测输入等比缩放（ letterbox，preprocess.letterbox，默认关）：
       开=head/person/face 三路整帧保宽高比、底部补 114 灰；ROI 档不受此键影响。
       默认关=现网拉伸形态逐字节不变；开之前必须先过主机 A/B（mut 与用例钉坐标侧） */
    bool bLetterboxEnable = false;

    /* letterbox 的作用路名单（preprocess.letterbox_models，空 = head+face 两路，
       与 #93-1 原行为一致）。单列这个键是因为两路对纵向拉伸的偏好相反：
       teacher 机位实测 head 在 letterbox 下每拍少 1.3 个头（13.1→11.8），
       而 face 的对齐残差失败从 270 降到 9（s3×2700）——只能一路一路地开。
       只认 head/face 两个键：其余路（姿态/复核/对齐/板书）是 ROI 档，本就不受影响 */
    std::vector<std::string> vecLetterboxModels;

    /* ---- 判定阈值 ---- */
    float     fPoseScoreThreshold = CLASSROOM_POSE_SCORE_THRESHOLD;
    int32_t   nMinFacePixel = CLASSROOM_MIN_FACE_PIXEL;
    /* 人脸**检测**框的短边下限，与上一行不是一道闸：120 判「这张脸配不配被起名字」
       （闸在账本的命名链上），本行判「这一坨值不值得往下解」。0 = 不设这道闸 */
    int32_t   nMinFaceDetectPixel = CLASSROOM_MIN_FACE_DETECT_PIXEL;
    int32_t   nMaxPersons = CLASSROOM_MAX_PERSONS;
    /* ---- 检测门限（P1：人头检测器每拍下发给后端，并在解析后复检）---- */
    float     fDetectScoreThreshold = CLASSROOM_DETECT_SCORE_THRESHOLD;
    float     fDetectNmsThreshold = CLASSROOM_DETECT_NMS_THRESHOLD;
    int32_t   nMinHeadPixel = CLASSROOM_MIN_HEAD_PIXEL;
    /* 人头**检测**框长边上限（threshold.max_head_px），0=关（默认）。
       与上一行的 min 是同一把尺的两端，该框内含前排真人 → 裁剪上 FastPose 出真骨骼，
       姿态证人（teacher.pose_witness_ttl_ms）拦不住，教师槽照样易主 2.5min——
       根因在检测器多检，闸就设在检测出口。取 0 为默认是失败安全：新判据不上电，
       出厂 conf 显式打开才生效；校验拒绝 max<=min（那等于全拒） */
    int32_t   nMaxHeadPixel = 0;
    int32_t   nMinBodyPixel = CLASSROOM_MIN_BODY_PIXEL;
    /* 远带人头（#90）：画布短边 < head_far_edge_px 的人头框用 det_score×head_far_gate_ratio
       的折扣门放行（后排的头分数天然更低，用基门拦会把后排整个拦没）。
       edge 配 0 = 分带关闭 = 旧形态；两个值的标定读数 = u64LowScore/u64TooSmall 前后对比 */
    int32_t   nHeadFarEdgePx = CLASSROOM_HEAD_FAR_EDGE_PX;
    float     fHeadFarGateRatio = CLASSROOM_HEAD_FAR_GATE_RATIO;

    /* ---- 逐人「这一行有没有证据可判」四道闸----
       前三项由感知层用来写特征行的辅助字段（bOccluded/bOnBoundary/fBlurScore），
       第四项由采样器直接读：它们都不决定「是什么行为」，只决定「这一行配不配被拿来判」 */
    float     fOccludeIou = CLASSROOM_OCCLUDE_IOU;    /* 邻框 IoU 超此值 → bOccluded */
    int32_t   nEdgeBandPx = CLASSROOM_EDGE_BAND_PX;  /* 头中心距画面边小于此值 → bOnBoundary（T42） */
    float     fBlurFullGrad = CLASSROOM_BLUR_FULL_GRAD; /* 梯度均值到此起记为「完全清晰」 */
    /* 低于此值的行整路不判姿态（采样器给 SR_BLUR）。与上一项是同一把尺的两端：
       fBlurFullGrad 定「怎么量」，本项定「量到多少就不信」，改前者会让后者的读数一起变。
       #91 起生效值 = 本项 × min(1, 头框高/34)（poseBlurMinForHeadPx，采样/规则两侧同一函数） */
    float     fPoseBlurMin = CLASSROOM_POSE_BLUR_MIN;

    /* ---- 人头跟踪（每帧都跑，CPU 近 0 成本）----
       这四个值直接决定 ID 会不会洪泛：门限太紧 → 同一个人每帧换 ID → 所有"最短持续"判定永不满足 */
    CTrackerParam_S stTracker;

    HeadDownRule_S    stHeadDown;
    HeadUpRule_S      stHeadUp;
    /* ---- 教师三条"无独立判据"事件的置信度门----
       206 讲授 / 208 板书 / 209 师生互动 的判据是几何事实的组合（区域 + 非背身/背身 +
       学生侧有人举手），本身**没有概率输出**。它们的「置信度」只能落在一个真实存在的
       观测量上，最贴近的是姿态骨骼的整体可信度 CPoseSkeleton_S::fScore（各点均值）：
       门限越高，越要求这一拍的骨骼点可靠——低于门限给 UNKNOWN（"没量准"）而不是 FALSE，
       与全工程「缺证据不否决」同一口径。
       默认 0.1（网页显示 1）：骨骼均分低于 0.1 基本是解码失败，此时三条都不该出结论；
       调到 0.3~0.5 会让远带小目标整段变成 UNKNOWN，现场按读数取舍。
       ★ 这三个键与 rules.group.listen.key_score / board.min_line_score **无关**：
       改「讲授置信度」会顺手改掉学生听讲的
       朝向门、"板书置信度"会改掉 OCR 行分门——一页改另一页的行为，是最难查的串台 */
    float     fTeachKeyScore = 0.1f;
    float     fBoardWriteKeyScore = 0.1f;
    float     fInteractKeyScore = 0.1f;
    /* 207 页「区域停留时间」（rules.guiding.stay_ms，网页 1..10s）：
       判定口径是「同一行在页 14 框内连续站立达到此时长」——它约束的是单个学生自己，
       与 behaviors.207.min_duration_ms（班级事实持续多久）串联：首个事件最早在两者之和出现。
       结构默认 0 = 不设停留门（只影响没读到该键的临时快照）；出厂 JSON 落 5000，
       与网页默认「5 秒」一致，网页下发 1..10s 即覆盖。
       为什么不做成「几个人先后站过累计」：那种算法让路过、起立问好也能凑够时长，
       而连续段一断就清零，"一直在画面里站着"这条事实才经得起复查 */
    int32_t   nGuidingStayMs = 0;
    StandRule_S       stStand;
    RaiseHandRule_S   stRaiseHand;
    TurnBackRule_S    stTurnBack;
    LieOnDeskRule_S   stLieOnDesk;
    PhoneRule_S       stPhone;

    /* ---- 人脸五点对齐----
       判定阈值直接用 face_align.h 那个参数结构，不在配置层再抄一份同名字段：
       两处各存一份就要有人负责同步，而漏同步的表现是"改了配置对齐不认"（与 stAttend 同理）。
       下面四项是这条链上的**口径**而不是阈值，故不进那个结构（solveFaceAlign 不认识它们） */
    CFaceAlignParam_S stAlign;
    /* 五点置换表下标（0..CLASSROOM_ALIGN_PERM_NUM-1）。交付的 FaceDetect 没有点序文档，
       而点序接反不会报错、只会让特征整体走低，所以这一项必须能配、且能靠探针读数挑出来。
    */
    int32_t   nAlignPointOrder = CLASSROOM_ALIGN_PERM_IDENTITY;
    /* 点序探针：每拍对每张脸把六张表都解一遍，取平均残差进心跳。
       常开（默认 true）：它是一次上机就能定死的答案，而关掉它的代价是"点序到底对不对"
       重新变成只能靠猜的问题。开销是每张脸 5 次 2×2 最小二乘（微秒级） */
    bool      bAlignOrderProbe = true;
    /* 送去重采样的那张裁剪图边长（像素，正方形）。比模型输入的 112 大一档的理由：
       对齐是「先按框裁一次、再按五点旋转缩放一次」，两趟重采样串起来会把细节磨掉一层，
       前一趟留余量就等于后一趟拿到近似原始像素。上限由裁剪档与内存闸门钳 */
    int32_t   nAlignCropPx = CLASSROOM_ALIGN_CROP_DEFAULT_PX;

    /* ---- 主体归属（教师是谁）----
       判据要用的三条区域事实来自 vecRegionRoles 里的 PODIUM/BOARD/STUDENT 三个角色，
       角色没下发时本块的全部参数都不会产生归属（见 CSubjectArbiter 文件头） */
    CTeacherConf_S    stTeacher;

    /* ---- 考勤账本参数（CSV 25~30 的那四条：到课人数 / 应到差异 / 人脸比对 / 身份展示）----
       直接用 behavior/attendance.h 里那个参数结构，而不是在配置层再抄一份同名字段：
       两处各存一份就要有人负责同步，而漏同步的表现是"改了配置账本不认"。
       默认开着——底库没交付时它照样出到课人数（那是 CSV 25 要的数），
       只是名单上没有名字（CSV 28 要等底库） */
    CAttendParam_S    stAttend;
    /* ---- 底库与取特征：考勤链上的「从哪拿名单」与「花多少钱」两段 ----
       刻意与 stAttend 分开：那一份是判定门限（改了会改变谁被记成到课），
       这两份改了只改变"有没有名单可比"与"多久比一次"，现场调精度时不该动它们 */
    CGalleryConf_S    stGallery;
    CFaceFeatConf_S   stFaceFeat;
    /* 表情/情绪。紧跟取特征是因为两者共用同一张对齐图：stFaceFeat 是上游总闸，
       本块只决定"造出来的这张图要不要再送一次分类推理"（口径见 CExpressionConf_S 注释） */
    CExpressionConf_S stExpr;
    /* 玩手机复核的成本档。与 stPhone（rules.phone：低头×手在身前那三条门限）分开的理由
       与上面那一对同构：这一块改了只改变"要不要为这个人裁一张图"，不改变任何判定口径 */
    CPhoneRecheckConf_S stPhoneRecheck;

    /* CPU-NPU 流水线优化开关（COptimizeConf_S 文件头那条灰度纪律）。放在成本档之后：
       它与判定门限无关，全部是"怎么排 NPU 的队"的口径 */
    COptimizeConf_S   stOpt;

    /* ---- 群体与展示类行为（107/109/111 的朝向判据、108 的书写代理闸、三道人数阈值）----
       前三项由学生采样器消费（`behavior/student_behavior.cpp`），后三项由群体层消费
       （`behavior/group_stats.cpp` 的时段判定）：同一块里的数分属两层，是因为它们描述的是
       同一件需求（CSV 16~18），拆成两块会让 Web 侧在两个地方找同一个旋钮 */
    CGroupBehaviorConf_S stGroup;

    /* ---- 座位就坐热力----
       单独成块而不是塞进 sample：这一块管的是"格子怎么切、多久才认一次改判"，
       既不改任何行为判定也不改任何采样周期，调它只改变那张图的形状。
       依赖 vecRegionRoles 里的 STUDENT 角色——没画席区时整段不上报（不是报一张全 0 的图），
       所以这里的默认值全开而配置层不因"没画区域"报错：区域与参数是两件事 */
    CSeatHeatConf_S   stSeat;

    /* ---- 座位先验：格子 → 座位那一层的两张表 ----
       单独成块而不是并进 seat_heat：那一块改的是"格子怎么切"，本块改的是"隔多久才允许把
       一个号发给下一个人"，后者的余量要与 attendance.row_idle_ms 做算术（见 seatPriorIdleOk），
       合在一块里会让现场以为"这只是同一张图的粗细"。
       enable 出厂为 **false**：这一层发出去的号会直接进考勤账本（`"seat":N` 那一列），
       而"两个活人并成一行"这种错在报表上看不出来——先按关着的姿态把 fold 跑起来（折样照跑、
       消费才关，见 CClassroomEngine::attendStep），拿主机侧 25 套件与设备心跳的 inherit/steal
       比例证明它不制造重号，再由现场决定开 */
    CSeatPriorConf_S  stPrior;

    /* ---- 板书记录----
       直接用感知层那个参数结构（`CBoardParam_S`），不在配置层再抄一份同名字段：两处各存一份
       就要有人负责同步，漏同步的表现是"改了配置板书记录不认"（与 stAttend / stAlign 同一条理由）。
       一块**关系钳**：inflight_timeout_ms 必须 ≤ reentry_ms——在途标记决定何时重试时，
       一次被丢的回调会把这条路静默停摆到超时，而那比下限还长就等于下限白配（钳到相等 + 记 soft）。
       item_max / text_max_bytes 只钳到编译期容量（环与单条的定长上限），**不钳二者乘积**：
       报文里那一段的字节预算是在写报文的地方按"从新到旧装到装不下为止 + 计数"兜的
       （§1.18 那条"预算是加和的"的落点在那里，不在这里乘一次得到一个没人执行的数）*/
    CBoardParam_S     stBoard;

    /* ---- 板书 OCR 的 DB 检测后处理----
       同样直接复用感知层那个结构（`COcrDbParam_S`），不在配置层再抄一份：这三道门在
       `ocr_decode.cpp` 里被读，多存一份就要有人负责同步。
       ★ 交付件那颗 .rknn 自带 mask 0.6 / box 0.25（`ocr_decode.h` 那句"配置可覆盖"从本刀起才是真的），
       而 `unclip_ratio` 是参照实现里写死的成员默认值——它是**我们这边**定的行框松紧，
       改它要连着看合并那一步，所以它的区间钳得最窄。
       合并那两条（fMergeVOverlap / fMergeHGap）**不做成可配**：它们是上游 sorted_boxes 的取值，
       单独调其中一条会得到"一行碎成三行"或"两行并成一行"，而这两种在心跳读数上都只是
       lines= 变了——现场调不出更好的答案，只会调出一份没人能解释的报表（与 scene 的优先级同一条理由）*/
    COcrDbParam_S   stOcr;

    /* ---- 课堂场景时段----
       这块里只有**时间**（进入滞回、反证间隔、最短段）与两个"多少人算一堂问答/练习"的密度。
       五种场景的优先级刻意不做成可配：那是"什么算问答课"的定义而不是阈值，换一种顺序就是
       换一种场景，现场调不出更好的答案，只会调出一个没人能解释的报表（理由见 scene_infer.h 头）。
       同样刻意**不**在这里重复任何行为阈值（举手要多久算举手、讨论要几个人算讨论，
       分别归时长门与 rules.group）——这一层看到的输入全是别处算好的事实 */
    CSceneParam_S     stScene;

    /* ---- 人头框 → 人体框外扩比例---- */
    float     fBodyExpandW = CLASSROOM_BODY_EXPAND_W_HEAD;      /* 各向左右扩几个头宽 */
    float     fBodyExpandDown = CLASSROOM_BODY_EXPAND_DOWN_HEAD;/* 向下扩几个头高 */
    float     fBodyExpandUp = CLASSROOM_BODY_EXPAND_UP_HEAD;    /* 向上扩几个头高 */

    /* ---- 按行为列号索引的开关与时长门 ---- */
    std::vector<CGateParam_S> vecGates;    /* 长度 = CDurationGate::BEHAVIOR_COLUMN_NUM */

    /* ---- 与 vecGates 同下标、同长度的区域引用 ----
       另开一张表而不是往 CGateParam_S 里塞字段：时长门是纯时间逻辑（可单测、可被群体统计复用），
       让它认识矩形就等于把坐标系拽进门的单元测试里，之后每加一种区域形状都要回去改它 */
    std::vector<CRegionRef_S> vecBehaviorRegions;

    /* ---- 模型表与内存预算 ---- */
    std::vector<CModelDesc_S> vecModels;   /* 启动时逐条 registerModel */
    CModelPoolParam_S stPool;

    CSampleConf_S  stSample;
    CEgressConf_S  stEgress;
    CStatsConf_S   stStats;

    /**
     * @brief   : 取某行为的时长门参数；越界返回禁用门（不抛、不越界读）
     */
    const CGateParam_S &gateOf(Behavior_E enBehavior) const;
    /**
     * @brief   : 取某行为的区域引用；越界返回「不限区域」的空引用（不抛、不越界读）
     */
    const CRegionRef_S &regionRefOf(Behavior_E enBehavior) const;
    /* 某行为是否启用（门本身带 bEnable，总开关另算） */
    bool behaviorOn(Behavior_E enBehavior) const;
    /* 任一行为启用即为「需要该组模型」，供调度器决定是否加载 */
    bool anyBehaviorOn() const;
} CConfigSnapshot_S;

/**
 * @brief   : 行为枚举 → 稳定短名（心跳文本、statusJson 的 key、日志标签的唯一来源）
 * @return   {const char*} 未知枚举返回 "unknown"，不返回 nullptr——拼日志的地方不判空，
 *            返回空指针会让整条日志在 printf 的 %s 上断掉，把一个可观测点变成崩溃点
 * @note    : 名字与编号同为对外契约，权威只在 classroom_config.cpp 的默认表里那一份，
 *            新增行为必须同处加行（漏行会被该文件的启动自检喊出来）
 */
const char *behaviorName(Behavior_E enBehavior);

/**
 * @brief   : 行为枚举 → 中文名（**只给人看**：日志、现场排障；报表/平台报文/statusJson 仍走
 *            behaviorName 的英文表，那是解析契约，两边都必须继续成立）
 * @return   {const char*} 未知枚举返回 "未知行为"，不返回 nullptr——与 behaviorName 同一条纪律
 */
const char *behaviorCnName(Behavior_E enBehavior);

/**
 * @brief   : 设备角色枚举 → 心跳/日志短名（"teacher"/"student"/"both"）
 * @return   {const char*} 未知枚举返回 "unknown"，与 behaviorName 同一条纪律：拼日志处不判空
 */
const char *deviceRoleName(DeviceRole_E enRole);

typedef std::shared_ptr<const CConfigSnapshot_S> ConfigPtr;

/**
 * @brief   : 某一路整帧检测要不要走 letterbox（引擎建裁剪池与坐标映射都调这一个判断）
 * @return   {bool} true=等比缩放+底部补灰；false=整幅拉伸
 * @note    : 两处必须同源：像素侧 CCropUnit::setup 的 bLetterbox 与坐标侧 CDetMap 的
 *            letterboxPlan 各算一次就会"图加了 pad、映射还按整幅"——这里给出唯一答案。
 *            名单为空 = head+face 两路（#93-1 原行为）；名单非空时只认名单里的键，
 *            写错名字等于该路不开（失败安全：宁可保持旧形态，也不悄悄换几何）
 */
inline bool letterboxForModel(const CConfigSnapshot_S &rstSnap, const std::string &strKey)
{
    if (!rstSnap.bLetterboxEnable)
    {
        return false;
    }
    if (CLASSROOM_MODEL_KEY_HEAD != strKey && CLASSROOM_MODEL_KEY_FACE != strKey)
    {
        return false;
    }
    if (rstSnap.vecLetterboxModels.empty())
    {
        return true;
    }
    for (size_t i = 0; i < rstSnap.vecLetterboxModels.size(); ++i)
    {
        if (rstSnap.vecLetterboxModels[i] == strKey)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief   : 配置持有者
 * @note    : 线程安全。current() 可在任意热路径调用（一次互斥 + 一次引用计数）
 */
class CClassroomConfig
{
public:
    CClassroomConfig() = default;

    /**
     * @brief   : 用默认值建首个快照（配置文件不存在时也要能跑，模型未交付同样可部署）
     */
    void loadDefaults(const std::string &strSourcePath);

    /**
     * @brief   : 读文件并换入
     * @param    {const std::string&} strPath：JSON 绝对路径
     * @param    {std::string&} rstrErr：OUT=失败原因（写日志与 Web 回显用）
     * @return   {bool} true 表示新快照已生效；false 表示保持旧快照不变
     * @note    : 校验在换入之前完成，任何一条硬错误都整体拒绝，不做「部分接受」
     */
    bool reloadFromFile(const std::string &strPath, std::string &rstrErr);

    /**
     * @brief   : 从 JSON 文本换入（下发通道走的就是这条，与文件版共用同一套校验）
     */
    bool reloadFromText(const std::string &strJson, const std::string &strSourceTag, std::string &rstrErr);

    ConfigPtr current() const;

    /**
     * @brief   : 默认配置文件全路径（USER_DATA_PATH + 文件名）
     */
    static std::string defaultPath();

    /**
     * @brief   : 把一份 JSON 原文原子写回配置文件（Web 下发落盘用）
     * @param    {const std::string&} strPath：目标路径（一般传 defaultPath()）
     * @param    {const std::string&} strText：要写的原文（必须已经过 reloadFromText 校验）
     * @param    {std::string&} rstrErr：OUT=失败原因
     * @return   {bool} true 表示已落盘
     * @note    : ① **先写 .tmp 再 rename**：掉电时要么是旧文件、要么是新文件，
     *              不会留下半份 JSON（现场手改配置时最怕的就是半份）；
     *            ② 只写不读：本函数不解析、不校验，校验必须由调用方先做——
     *              写一份解析器不认的配置比不写更糟；
     *            ③ 失败时删掉 .tmp（`std::remove`，不是 shell 的 rm）
     */
    static bool saveTextToFile(const std::string &strPath, const std::string &strText,
                               std::string &rstrErr);

    /**
     * @brief   : 最近一次成功换入的 JSON 原文
     * @return   {const std::string&} 空串表示"从没有成功换入过"（只用默认值跑）
     * @note    : 留着原文而不是把快照序列化回 JSON：序列化器要和 parseIntoJson
     *            维护同一份键名，那是两份必然漂移的口径；原文则是"设备现在照着跑的
     *            那一份"的精确副本，Web 取什么键都不会和解析器不一致。
     *            线程安全：返回值在锁外读（下次换入时旧串才可能被替换），
     *            调用方若要与 current() 快照配对，先取 current() 再取本串
     */
    std::string lastConfigText() const;

    /**
     * @brief   : 最近一次「键类型不符」的完整键名（段.键，#95 逐键点名）。
     *            每次解析重置；没有坏键时返回空串。进程级静态——读它不需要持锁，
     *            写侧只有配置换入线程
     */
    static const char *lastBadKeyText();

    /**
     * @brief   : 纯校验（无副作用，可单测）
     * @return   {bool} false 时 rstrErr 说明第一条硬错误
     * @note    : 软错误（超出物理范围的阈值）在这里钳制到边界并记 warn，不拒绝整份配置
     */
    static bool validate(CConfigSnapshot_S &stSnap, std::string &rstrErr);

private:
    /* 解析 JSON 到快照；缺字段一律保留默认值（向后兼容：老机器上多出来的键忽略，缺的键用默认） */
    static void parseIntoJson(void *pJson, CConfigSnapshot_S &stSnap);

    mutable std::mutex m_mtx;
    ConfigPtr m_spCurrent;
    /* 最近一次成功换入的原文。与 m_spCurrent 在同一把锁下更新，
       所以"current() 与 lastConfigText() 配对"只需各自取一次 */
    std::string m_strLastText;
};
}
