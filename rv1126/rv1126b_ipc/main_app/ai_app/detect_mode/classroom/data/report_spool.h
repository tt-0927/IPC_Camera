/**
 * @FilePath     : report_spool.h
 * @Author       : leiyy
 * @Date         : 2026-10-21 13:48:32
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 13:48:32
 * @Description  : 报表滞留落盘环
 */

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* 滞留环默认落点。放 /opt/course */
#define CLASSROOM_SPOOL_DIR          "/opt/course/spool/classroom/"
#define CLASSROOM_SPOOL_MAX_MB       (32)     /* 容量上限 */
#define CLASSROOM_SPOOL_SEG_MAX_KB   (1024)   /* 单段上限：32 段 × 1MB = 上限 */
#define CLASSROOM_SPOOL_DRAIN_KB     (64)     /* 一拍最多补传多少 KB，防止恢复瞬间把节拍拖穿 */

/* 记录类型标签。补传时要按原样把类型交回通道，所以类型跟着数据落盘，不靠"读的时候还记得" */
enum SpoolType_E
{
    SPOOL_TYPE_REPORT   = 1,   /* 周期报表（指标流） */
    SPOOL_TYPE_SESSION  = 2,   /* 课堂收尾报表（会话报告） */
    SPOOL_TYPE_EVENT    = 3    /* 行为事件：通道定了再接 */
};

/**
 * @brief 滞留环参数
 */
typedef struct _SpoolParam_
{
    bool        bEnable = true;
    std::string strDir = CLASSROOM_SPOOL_DIR;
    int32_t     nMaxMb = CLASSROOM_SPOOL_MAX_MB;
    int32_t     nSegMaxKb = CLASSROOM_SPOOL_SEG_MAX_KB;
    int32_t     nDrainMaxKb = CLASSROOM_SPOOL_DRAIN_KB;
} CSpoolParam_S;

/**
 * @brief 一条待补传记录
 */
typedef struct _SpoolRecord_
{
    uint32_t    u32Type = SPOOL_TYPE_REPORT;
    uint64_t    u64Seq = 0;      /* 记录位置：高 32 位段号、低 32 位段内序号。
                                    跨重启不重复，靠的是 ctl 里那份"段号水位"（32..39）——
                                    只从目录推段号会在环排干（所有段被 unlink）后退回 1 重新发号，
                                    那是本字段唯一一处会复用的形状，见 writeCtlLocked 的注释 */
    int64_t     llTsMs = 0;      /* 落盘时刻（单调时钟），补传时原样带上，平台按它排时间轴 */
    std::string strPayload;
} CSpoolRecord_S;

/**
 * @brief 滞留环读数
 */
typedef struct _SpoolStat_
{
    int32_t  nSegNum = 0;          /* 当前段数 */
    int64_t  llUsedBytes = 0;      /* 当前占用 */
    int64_t  llMaxBytes = 0;       /* 上限（init 时定，读日志时不用换算） */
    uint64_t u64Append = 0;        /* 成功落盘条数 */
    uint64_t u64AppendFail = 0;    /* 写失败（ENOSPC/IO）条数 */
    uint64_t u64Sent = 0;          /* 补传成功条数 */
    uint64_t u64DropSeg = 0;       /* 超限整段丢弃数 */
    uint64_t u64DropBytes = 0;     /* 超限丢弃字节数（含整段与单条） */
    uint64_t u64DropRec = 0;       /* 超限单条丢弃数（连一段都放不下时的最后手段） */
    uint64_t u64Corrupt = 0;       /* 校验失败跳段数 */
    uint64_t u64TornTail = 0;      /* 启动修复截掉的尾部字节数 */
    uint64_t u64IoFail = 0;        /* open/stat/opendir 失败次数 */
    uint64_t u64CtlWrite = 0;      /* 游标落盘次数 */
    uint64_t u64CtlBad = 0;        /* ctl 文件在、但没有任何有效槽（掉电写坏）的次数 */
} CSpoolStat_S;

/**
 * @class CReportSpool
 * @brief 有界滞留环：追加写 + 按序读 + 整段回收
 */
class CReportSpool
{
public:
    CReportSpool() = default;
    ~CReportSpool();

    CReportSpool(const CReportSpool &) = delete;
    CReportSpool &operator=(const CReportSpool &) = delete;

    /**
     * @brief   : 建目录、扫描既有段、修复最新段的撕裂尾
     * @param   : stParam 参数；rstrErr 失败原因（只描述结论，不猜路径）
     * @return  : true=可用。false 时本对象仍可安全调用其余接口（全部退化为空操作）
     */
    bool init(const CSpoolParam_S &stParam, std::string &rstrErr);
    void deinit();
    bool isReady() const;

    /**
     * @brief   : 追加一条待补传记录
     * @return  : true=已落盘。false=没落（关闭/写失败/单条超上限），失败原因进 rstrErr
     */
    bool append(uint32_t u32Type, const void *pPayload, size_t uLen, std::string &rstrErr);

    /**
     * @brief   : 取最早一条待补传记录，**不**推进游标
     * @return  : true=有一条。false=积压已清空或环不可用
     * @note    : peek+commit 而不是"取一条就少一条"，是因为补传的成败只有通道知道：
     *            取走即删的话，"发送失败"与"发送成功但进程被杀"会变成同一件事
     */
    bool peek(CSpoolRecord_S &rstRec);
    bool commit();

    /* peek 放弃（连转 PEEK_LOOP_MAX 次仍读不出一条）的累计次数（单调） */
    uint64_t peekGiveupNum() const { return m_u64PeekGiveup; }

    /**
     * @brief   : 清空全部积压
     */
    void clear();

    int64_t pendingBytes() const;
    CSpoolStat_S stat() const;
    std::string statLine() const;    /* 心跳 spool= 段，与 data= 段同一风格 */
    std::string statJson() const;    /* statusJson 的 spool 段 */

    static const char *typeName(uint32_t u32Type);

private:
    typedef struct _SegInfo_
    {
        int64_t llNum = 0;
        int64_t llSize = 0;
    } CSegInfo_S;

    bool scanLocked(std::string &rstrErr);
    bool ensureWriteSegLocked(std::string &rstrErr);
    void closeWriteSegLocked();
    bool makeRoomLocked(int64_t llNeed, std::string &rstrErr);
    bool openReadSegLocked();
    /* 1=取到一条 / 0=本段到此为止（短尾，不是坏数据） / -1=坏记录（魔数或校验不过）。
       两者必须分开：短尾是掉电的正常形态，跳段即可；坏记录说明中段被写坏，
       混计会让 u64Corrupt 这个"该不该担心这块 flash"的读数失去意义 */
    int32_t readRecordLocked(CSpoolRecord_S &rstRec, int64_t &rllRecBytes);
    void advanceToNextSegLocked(bool bConsumeCurrent);
    /* 只有一段可读、且段尾读不出来时用：把文件截回读游标，写边界跟着退。
       不能跳段（没有下一段），也不能回收（回收等于把还在写的文件删了） */
    void reclaimTailLocked();
    /* 只剩一段且段内每条都已 commit：整段回收。文件此时已没有任何未交付的字节，
       不变式 3 不再禁止 unlink。留着的坏处比省一次 create 大——下一批记录会复用同一批序号 */
    void reclaimDrainedLocked();
    void dropOldestSegLocked();
    std::string segPathLocked(int64_t llNum) const;
    std::string ctlPathLocked() const;
    /* 游标持久化（不变式 4）：写=一次 64B 定长槽 pwrite，轮转槽位，永不改写文件内容 */
    void writeCtlLocked();
    void readCtlLocked();

private:
    mutable std::mutex m_mtx;

    bool m_bReady = false;
    CSpoolParam_S m_stParam;
    int64_t m_llMaxBytes = 0;
    int64_t m_llSegMaxBytes = 0;

    std::vector<CSegInfo_S> m_vecSeg;   /* 升序；front=最旧，back=最新 */
    int64_t m_llUsedBytes = 0;
    int64_t m_llNextSegNum = 1;
    /* 下一条记录在**当前写段**里的序号。记录序号 = (段号<<32 | 本序号)，因此跨重启、跨清空
       都天然单调且不回卷，不需要另存一个全局计数器（那个计数器一丢写，新记录就会复用旧序号）*/
    uint32_t m_u32WriteRecIdx = 0;

    /* 写侧：只在 back() 上追加，m_llWriteBytes 恒等于"已确认写完的字节数" */
    int32_t m_fdWrite = -1;
    int64_t m_llWriteBytes = 0;
    /* back() 已封口（轮转时 fsync+close 过）：下一次写必须另起一段。少了这个标记，
       "没写满"的旧段会被重新打开续写，而它可能已经被读侧当成完整段跳过了后半截 */
    bool m_bWriteSegSealed = false;
    std::vector<uint8_t> m_vecScratch;   /* 头+体一次成块写下去的暂存，容量只增不减 */

    /* 读侧：只在 front() 上顺序读 */
    int32_t m_fdRead = -1;
    int64_t m_llReadSegNum = -1;   /* 游标所属段号；-1 表示"没归属"，下次开段从头读 */
    int64_t m_llReadOff = 0;
    int64_t m_llReadSize = 0;

    /* 游标文件：fd 常驻。打开/写失败只影响"重启后会不会重发"，不影响本进程内的正确性 */
    int32_t m_fdCtl = -1;
    uint32_t m_u32CtlSlot = 0;
    uint64_t m_u64CtlSlide = 0;

    /* peek 出来但还没 commit 的那条占多少字节（commit 时推进游标要用） */
    int64_t m_llPendingRecBytes = 0;

    uint64_t m_u64PeekGiveup = 0;

    CSpoolStat_S m_stStat;
};

/**
 * @brief 补传通道回调（与引擎的 CUploadFn 同签名；通道返回 false = 通道说"这条没送达"）
 */
typedef std::function<bool(uint32_t u32Type, const std::string &strPayload)> SpoolUploadFn;

/**
 * @brief 一拍补传的账
 */
typedef struct _SpoolDrainStat_
{
    int32_t     nSent = 0;              /* 本拍 commit 成功条数 */
    int64_t     llSentKb = 0;           /* 本拍读掉的 flash 字节（含 36B 头，KB 向上取整） */
    int32_t     nUploadError = 0;       /* 通道返回 false 的次数（一停就 break，实际只会 0/1） */
    int32_t     nUploadThrow = 0;       /* 通道抛异常的次数（同上） */
    std::string strThrowWhat;           /* 异常 what()，只进日志 */
    bool        bCommitStuck = false;   /* commit 了游标却没推进（存储层异常态） */
    uint64_t    u64StuckSeq = 0;        /* 卡住那条的序号 */
    /* peek 连续坏段后放弃的次数 */
    int32_t     nPeekGiveup = 0;
    /* 同上的累计口径（跨拍单调，给心跳用） */
    uint64_t    u64PeekGiveupTotal = 0;
} CSpoolDrainStat_S;

/**
 * @brief   补传一拍：按序 peek→上传→commit，直到预算用满或积压清空
 * @param   fnUpload   通道回调，允许为空（空 = 本函数直接返回 0）
 * @param   llBudgetKb 本拍最多读多少 KB（含 36B 头）。预算在取**下一条**之前判，
 *                     所以预算只管"超过就不再拿"——每拍至少尝试一条，
 *                     配 0 则一拍 0 条（节奏在调用方，本函数不替现场改主意）
 * @param   rstStat    本拍账目，进函数前被清零
 * @return  commit 成功条数（≥0）
 */
int32_t spoolDrainBeat(CReportSpool &rstSpool, const SpoolUploadFn &fnUpload,
                       int64_t llBudgetKb, CSpoolDrainStat_S &rstStat);

}   /* namespace Classroom */
