/**
 * @FilePath     : board_watcher.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-03 13:58:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 13:58:00
 * @Description  : 板书记录实现
 */

#include "board_watcher.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "classroom_fmt.h"   /* truncateUtf8 */
#include "dlog.h"

namespace Classroom
{
namespace
{
/* FNV-1a 64 位参数：定值，不做配置项——哈希一致性的意义就在于两端同值 */
const uint64_t FNV_OFFSET_BASIS = 1469598103934665603ULL;
const uint64_t FNV_PRIME = 1099511628211ULL;

uint64_t fnv1a(const std::string &str)
{
    uint64_t h = FNV_OFFSET_BASIS;
    for (const char c : str)
    {
        h ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        h *= FNV_PRIME;
    }
    return h;
}

/* 归一化一行文本供哈希用：去掉半角空格/制表与全角空格（U+3000 = E3 80 80）。
   全角空格在中文板书 OCR 里极常见，而它是分段噪声不是内容：留着的话
   同一块板两次采样会因为"这次多识别出一个空格"被判成板书变了 */
std::string normalizeForHash(const std::string &strIn)
{
    std::string strOut;
    strOut.reserve(strIn.size());
    for (size_t i = 0; i < strIn.size(); ++i)
    {
        if (' ' == strIn[i] || '\t' == strIn[i])
        {
            continue;
        }
        if (i + 2 < strIn.size() &&
            static_cast<unsigned char>(strIn[i]) == 0xE3 &&
            static_cast<unsigned char>(strIn[i + 1]) == 0x80 &&
            static_cast<unsigned char>(strIn[i + 2]) == 0x80)
        {
            i += 2;
            continue;
        }
        strOut += strIn[i];
    }
    return strOut;
}

/* 一行与其归一化文本成对存放：两份平行容器一旦按行框排序就会错位，
   而错位后的集合哈希变成与行序相关——正是本文件头要避免的那件事 */
typedef struct _BoardLine_
{
    CBoardText_S stText;
    std::string  strNorm;
} CBoardLine_S;

/* 行框的阅读序：先按中心 y、再按中心 x。det 的输出序按置信度排，两行的先后会随帧抖动。
   ★ 这条排序在**报文里没有读者**了（第 7 刀之后出去的是计数，没有正文），它现在的理由
   是"超出 CLASSROOM_BOARD_LINE_MAX 时丢掉哪几行"必须确定：不排的话截的是置信度序，
   同一块板两帧会留下不同的 24 行，于是 kept= 与 byte= 都会随帧抖，
   而那两列恰恰是"板书掉没掉"的读数——它们一抖就没有任何一列还能用 */
bool lineLess(const CBoardLine_S &stA, const CBoardLine_S &stB)
{
    const float fAy = (stA.stText.stBox.fY1 + stA.stText.stBox.fY2) * 0.5f;
    const float fBy = (stB.stText.stBox.fY1 + stB.stText.stBox.fY2) * 0.5f;
    /* 1/4 行高算同一行。取行高而不是固定像素：同一块板远近两次的行差是成比例的，
       固定阈值在远焦那一次会把两行判成一行、在近焦那一次又把一行判成两行 */
    /* 容差必须取**两侧行高的较大者**，不能只取 stA 的。
       只取 stA 时，容差随"谁是左操作数"而变：一行很高（tol=100）配一行很矮（tol=1）、
       两行中心相距 52 时，
         lineLess(高, 矮)  走"同一行"分支  → 比 x
         lineLess(矮, 高)  走"不同行"分支 → 比 y，且 52 > 1 必返 true
       于是 **lineLess(A,B) 与 lineLess(B,A) 同时为 true** —— std::sort 的比较器
       违反严格弱序（antisymmetry），是未定义行为：轻则这一帧的阅读序算错，
       重则 libstdc++ 的排序在运行时越界写（ASan 下表现为 heap-buffer-overflow）。
       取 max 之后两侧容差恒等，"同一行"这件事变成一个与方向无关的等价类。 */
    const float fHa = (stA.stText.stBox.height() > 0.0f) ? stA.stText.stBox.height() : 0.0f;
    const float fHb = (stB.stText.stBox.height() > 0.0f) ? stB.stText.stBox.height() : 0.0f;
    const float fRowH = (fHa > fHb) ? fHa : fHb;
    const float fRowTol = 0.25f * ((fRowH > 0.0f) ? fRowH : 1.0f);
    if (std::fabs(fAy - fBy) > fRowTol)
    {
        return fAy < fBy;
    }
    return (stA.stText.stBox.fX1 + stA.stText.stBox.fX2) <
           (stB.stText.stBox.fX1 + stB.stText.stBox.fX2);
}
}

IOcrBackend::~IOcrBackend() = default;

bool CNullOcrBackend::recognize(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                                std::vector<CBoardText_S> &rvecOut)
{
    /* (model-pending): 需要板书文字检测 + 行识别模型（det→cls→rec 串联）与其字典文件。
       到位后新增 CPPOcrBackend : IOcrBackend 并在模型表注册 Desc，
       本类保留给「本型号不出 OCR」的 SKU 用 */
    (void)pucRgb;
    (void)nW;
    (void)nH;
    rvecOut.clear();
    return false;
}

CBoardWatcher::CBoardWatcher()
{
    /* 构造即定容：环与临时表都在此一次分配，运行期只挪下标 */
    m_nCap = m_stParam.nItemMax;
    m_vecItem.resize(static_cast<size_t>(m_nCap));
    m_vecLineHashStr.reserve(CLASSROOM_BOARD_LINE_MAX);
}

void CBoardWatcher::setParam(const CBoardParam_S &stParam)
{
    if (stParam.nItemMax != m_nCap)
    {
        /* 换容量要保住待发记录：配置热更是"改个数看效果"的动作，
           直接 resize 会把没来得及上报的板书清掉，用户看到的是"一改配置内容就没了" */
        std::vector<CBoardItem_S> vecKeep;
        vecKeep.reserve(static_cast<size_t>(m_nCap));
        CBoardItem_S stItem;
        while (pop(stItem))
        {
            vecKeep.push_back(stItem);
        }
        m_nCap = (stParam.nItemMax > 0) ? stParam.nItemMax : 1;
        m_vecItem.assign(static_cast<size_t>(m_nCap), CBoardItem_S());
        m_nHead = 0;
        m_nNum = 0;
        /* 超出的从**最旧**一侧丢：新的那些才是用户刚采到的 */
        const int32_t nDrop = static_cast<int32_t>(vecKeep.size()) - m_nCap;
        const size_t nBegin = (nDrop > 0) ? static_cast<size_t>(nDrop) : 0U;
        for (size_t i = nBegin; i < vecKeep.size(); ++i)
        {
            m_vecItem[static_cast<size_t>(m_nNum)] = vecKeep[i];
            ++m_nNum;    /* m_nNum 是「个数」不是下标，取模会把满表读成空表 */
        }
    }
    m_stParam = stParam;
}

void CBoardWatcher::setBackend(IOcrBackend *pBackend)
{
    m_pBackend = pBackend;
    if (nullptr == m_pBackend)
    {
        /* 一次性说明为什么这一路没有内容：不在此处刷日志，也不在每拍刷。
           运行期的持续真相由 lastState() 与 u64BackendUnavailable 给（平台侧标 unavailable） */
        m_enLastState = BackendState::MODEL_MISSING;
    }
}

void CBoardWatcher::setBoardRectNorm(const CRectf_S &stRectNorm, int32_t nSrcW, int32_t nSrcH)
{
    m_bRectReady = false;
    m_stRectSrc = CRectf_S();

    if (nSrcW <= 0 || nSrcH <= 0)
    {
        return;
    }
    /* 归一化框允许是 (x1>x2) 的（Web 侧划线方向不定），这里顺手规范化而不是拒收：
       拒收的后果是"画了框但系统说没黑板区域"，而那个框看起来明明是好的 */
    float fX1 = (stRectNorm.fX1 < stRectNorm.fX2) ? stRectNorm.fX1 : stRectNorm.fX2;
    float fX2 = (stRectNorm.fX1 < stRectNorm.fX2) ? stRectNorm.fX2 : stRectNorm.fX1;
    float fY1 = (stRectNorm.fY1 < stRectNorm.fY2) ? stRectNorm.fY1 : stRectNorm.fY2;
    float fY2 = (stRectNorm.fY1 < stRectNorm.fY2) ? stRectNorm.fY2 : stRectNorm.fY1;

    fX1 = (fX1 < 0.0f) ? 0.0f : ((fX1 > 1.0f) ? 1.0f : fX1);
    fX2 = (fX2 < 0.0f) ? 0.0f : ((fX2 > 1.0f) ? 1.0f : fX2);
    fY1 = (fY1 < 0.0f) ? 0.0f : ((fY1 > 1.0f) ? 1.0f : fY1);
    fY2 = (fY2 < 0.0f) ? 0.0f : ((fY2 > 1.0f) ? 1.0f : fY2);

    CRectf_S stRect;
    stRect.fX1 = fX1 * static_cast<float>(nSrcW);
    stRect.fY1 = fY1 * static_cast<float>(nSrcH);
    stRect.fX2 = fX2 * static_cast<float>(nSrcW);
    stRect.fY2 = fY2 * static_cast<float>(nSrcH);
    /* 退化框（线宽为零）不能拿去裁剪：RGA 会返回失败，
       而失败点离原因三步远，现象是"黑板采集一直不出图" */
    if (stRect.width() < 2.0f || stRect.height() < 2.0f)
    {
        return;
    }
    m_stRectSrc = stRect;
    m_bRectReady = true;
}

bool CBoardWatcher::wantCapture(int64_t llNowMs, bool bGeomReady, float fOccludeRatio,
                                bool bWaitWarm, CBoardCapture_S &rstOut)
{
    rstOut.bWant = false;

    if (!m_stParam.bEnable)
    {
        return false;
    }

    /* 在途超时放行放在最前：某次采集请求被丢（引擎降级、帧没取到、回调链断）时，
       没有这一条的话 m_bInFlight 永久为真，板书功能静默停摆到下次重启 */
    if (m_bInFlight && (llNowMs - m_llFlightSinceMs) > m_stParam.llInflightTimeoutMs)
    {
        dlog_warn("classroom board: 采集在途超过 %lldms，强制放行",
                  static_cast<long long>(m_stParam.llInflightTimeoutMs));
        m_bInFlight = false;
        m_u32FlightSeq = 0;   /* 强制放行后旧作业号必须一起作废：留着它，下一次采集配对的是
                                 上一张图的号，等于把"迟到"读成"正常"（#64 第 4 刀） */
        ++m_stStat.u64InflightTimeout;
    }

    if (m_bInFlight)
    {
        return false;
    }
    if (m_stParam.llIntervalMs <= 0)
    {
        /* 下限配成 0 的语义是「本功能不参与节拍」，而不是「每拍都采」。
           后者会把一次分钟尺度的事件推到 10fps，把 NPU 与 32MB 上报配额一起吃光 */
        return false;
    }
    /* 触发闸（P6d）：事件驱动时先要有一次挂着的板书结束边沿。
       排在推表**之前**是必须的——没有边沿的那些拍只是"还没到机会"，把它们记成一次到点，
       u64Want 就成了"这节课黑板被看了多少次"这种没人要的读数，而它原本要说的是"采了几次" */
    /* 两条触发源（Web 24 页把第二条做成"分析间隔"）：
       ① 边沿：老师转回来/离开板前的那一下（bRequireEpisode）；
       ② 周期：老师还在板前写的时候，每 scan_interval_ms 重看一次——没有它，
          一场 3 分钟的板书只在收笔时采一张，中间擦改的内容全看不到 */
    const bool bEdgeDue = m_bEpisode;
    const bool bScanDue = (m_stParam.llScanIntervalMs > 0) && m_bTeacherInBoard &&
                          (m_llLastDueMs <= 0 ||
                           (llNowMs - m_llLastDueMs) >= m_stParam.llScanIntervalMs);
    if (m_stParam.bRequireEpisode && !bEdgeDue && !bScanDue)
    {
        return false;
    }
    if (bScanDue && !bEdgeDue)
    {
        ++m_stStat.u64ScanDue;
    }
    /* 边沿事件仍受 llIntervalMs 限制（防止一转身采两次）；周期触发自己的间隔已在
       bScanDue 里判过，不再叠一道，否则两个键会互相打脸 */
    if ((bEdgeDue || !m_stParam.bRequireEpisode) &&
        m_llLastDueMs > 0 && (llNowMs - m_llLastDueMs) < m_stParam.llIntervalMs)
    {
        /* 重入下限还压着：事件驱动模式下**不丢这次请求**（m_bEpisode 保持为真），
           下限一到就采。丢掉它等于"老师转回来那一刻刚好撞上下限，这块板就永远没记录" */
        return false;
    }

    /* 到点：先推表再判闸门。被挡掉的那轮同样消耗掉这次机会，
       否则老师站在板前 40s、走开的那一刻会连着采两轮（哈希必然不同 → 一条假变更）。
       ★ 上面那句"被挡掉也消耗"对本档**不成立**：遮挡/没几何/没区域是这台机器此刻拍不出
       可用的图，重来一次要等下一次边沿；而"OCR 还在预热路上"是一拍之后就会自己好的状态，
       让它照样到点等于把每条板书记录固定丢一次（边沿是本拍 ruleStep 发的，而暖组排在
       boardStep 之前 ⇒ 每个边沿的第一拍必然撞上这一格）。不清 m_bEpisode、不推 m_llLastDueMs，
       下一拍预热到位就从这里照常走一遍（判据与两种"没驻留"的分辨在引擎，见头文件该参数那条注） */
    if (bWaitWarm)
    {
        ++m_stStat.u64SkipNotWarm;
        return false;
    }

    m_llLastDueMs = llNowMs;
    ++m_stStat.u64Want;
    /* 这次机会已经被消费：事件驱动下连同挂着的边沿一起清掉，后面那几道闸（没区域、
       没几何、被遮挡、没后端）挡住的这一轮不再重复计数——与上面"被挡掉的那轮也消耗机会"同一条 */
    const int64_t llStampMs = (m_bEpisode && m_llEpisodeMs > 0) ? m_llEpisodeMs : llNowMs;
    m_bEpisode = false;

    if (!m_bRectReady)
    {
        ++m_stStat.u64NoRegion;
        return false;
    }
    if (!bGeomReady)
    {
        ++m_stStat.u64SkipNoGeom;
        return false;
    }
    if (!(fOccludeRatio <= m_stParam.fMaxOccludeRatio))
    {
        ++m_stStat.u64SkipOccluded;
        return false;
    }
    if (nullptr == m_pBackend)
    {
        /* 没有后端就不去裁剪取图（那是白花的 RGA 与内存），但**每周期都留下读数**：
           否则"这功能没跑"与"跑着但没能力"在设备日志里长得一模一样 */
        m_enLastState = BackendState::MODEL_MISSING;
        ++m_stStat.u64BackendUnavailable;
        return false;
    }

    m_bInFlight = true;
    m_llFlightSinceMs = llNowMs;
    m_llFlightDueMs = llStampMs;
    m_u32FlightSeq = 0;     /* 这一次还没交给后端；引擎投成功时经 markCaptureSubmitted 填上 */

    rstOut.bWant = true;
    rstOut.stRectSrc = m_stRectSrc;
    rstOut.llDueMs = llStampMs;
    ++m_stStat.u64Capture;
    return true;
}

bool CBoardWatcher::markCaptureSubmitted(uint32_t u32Seq)
{
    if (!m_bInFlight || 0 == u32Seq)
    {
        /* 两种情况都是调用方用错：没有在途就没号可登记（那格账已经交过了，
           登记上去会让下一次回填配上一个它不认识的号）；0 是"还没投出去"的哨兵 */
        return false;
    }
    m_u32FlightSeq = u32Seq;
    return true;
}

void CBoardWatcher::notifyEpisodeEnd(int64_t llEndMs)
{
    if (!m_stParam.bEnable)
    {
        return;   /* 关着的不清账也不计数：这一路压根没在跑，计数涨了就说明"关而不静" */
    }
    ++m_stStat.u64Episode;
    if (m_bEpisode)
    {
        /* 已经挂着一次：这一次并入那一次（黑板上的内容本来就是后一次覆盖前一次），
           但**更新时刻**——挂着的那次要报的是"最近一次板书结束"，不是第一次的 */
        ++m_stStat.u64EpisodeMerged;
    }
    m_bEpisode = true;
    m_llEpisodeMs = (llEndMs > 0) ? llEndMs : 0;
}

void CBoardWatcher::notifyTeacherStay(bool bInBoard, int64_t llNowMs)
{
    /* 周期重识别用的"人在板前"事实：与下面那条停留边沿无关，先记下来再走边沿逻辑 */
    m_bTeacherInBoard = bInBoard && m_stParam.bEnable;
    /* 功能关着（总开关或这条边沿配 0）时清锚点：留着它，重开之后第一次"离开"
       会拿一笔跨越开关期的时长去触发边沿——那是开关期里根本没发生的假停留 */
    if (!m_stParam.bEnable || m_stParam.llStayExitEdgeMs <= 0)
    {
        m_llStaySinceMs = 0;
        return;
    }
    if (bInBoard)
    {
        if (m_llStaySinceMs <= 0)
        {
            m_llStaySinceMs = llNowMs;    /* 进板前那一拍起锚：站着期间不动它 */
        }
        return;
    }
    if (m_llStaySinceMs <= 0)
    {
        return;    /* 本来就没站着：板前没人不构成"离开"边沿 */
    }
    const int64_t llStayMs = llNowMs - m_llStaySinceMs;
    m_llStaySinceMs = 0;
    if (llStayMs < m_stParam.llStayExitEdgeMs)
    {
        return;    /* 短停留（路过、擦黑板）：故意漏，见参数注释 */
    }
    ++m_stStat.u64StayEdge;
    /* 与行为边沿走同一条出口：合并/下限/在途去重全在那一头，这里只判"站够了没有" */
    notifyEpisodeEnd(llNowMs);
}

uint64_t CBoardWatcher::hashOfSet(const std::vector<std::string> &vecLine, int32_t nNum)
{
    if (nNum <= 0)
    {
        /* 空板也要有确定值：0 会被误当成「还没比过」（m_bHasLastHash 的另一层含义）。
           用一个与任何非空集合都不同的常数，且它自己稳定 */
        return FNV_OFFSET_BASIS;
    }
    /* 逐行哈希后排序再合并 —— 行序无关 */
    std::vector<uint64_t> vecHash;
    vecHash.reserve(static_cast<size_t>(nNum));
    for (int32_t i = 0; i < nNum; ++i)
    {
        vecHash.push_back(fnv1a(vecLine[static_cast<size_t>(i)]));
    }
    std::sort(vecHash.begin(), vecHash.end());

    uint64_t h = FNV_OFFSET_BASIS;
    for (const auto &u64 : vecHash)
    {
        h ^= u64;
        h *= FNV_PRIME;
    }
    return h;
}

bool CBoardWatcher::onOcrResult(int64_t llDueMs, uint32_t u32Seq, bool bOk, BackendState_E enState,
                                const std::vector<CBoardText_S> &vecText,
                                int64_t llLastJobMs, uint64_t u64TruncTotal)
{
    /* 配对：带号的结果必须是当前在途那一次的，否则它是上一次采集的迟到结果——
       那种图的内容已经不属于这次的时间戳，收下就等于凭空多一条"板书变了"。
       **不清在途标记**：那格账是别的那次采集的，这里清了它就一直悬着。
       当场回填的失败（还没投出去）传 0，按上面那条规则天然通过 */
    if (0 != u32Seq && u32Seq != m_u32FlightSeq)
    {
        ++m_stStat.u64ResultStale;
        return false;
    }

    /* 回填晚于超时放行是允许的：内容仍然真，只是时间戳旧。
       刻意不按 dueMs 丢弃这次结果——板书内容的先后顺序对报表有意义，而丢一条比晚一条糟 */
    m_bInFlight = false;
    m_u32FlightSeq = 0;

    /* 把引擎带进来的两个后端读数抄进本类观测。排在配对门**之后**、
       bOk 分支**之前**——迟到结果（上面已 return）不该覆盖，但一次失败的采集（bOk=false）
       仍然带来了后端最新的耗时/截断累计，那两个数是后端的账、与这一次成没成无关，照收。
       哨兵值表示"这一拍引擎没带新读数"（当场回填的失败、还没投出去）：保持上一次的值，
       别让一次采集失败把"上一次真实跑了多久"抹成 0 或 -1（同头文件那一格的口径） */
    if (llLastJobMs >= 0)
    {
        m_stStat.llLastJobMs = llLastJobMs;
    }
    if (UINT64_MAX != u64TruncTotal)
    {
        m_stStat.u64TruncTotal = u64TruncTotal;
    }

    if (!bOk)
    {
        m_enLastState = enState;
        ++m_stStat.u64BackendFail;
        if (BackendState::READY != enState)
        {
            ++m_stStat.u64BackendUnavailable;
        }
        return false;
    }
    m_enLastState = BackendState::READY;

    /* 调试开关（默认关，board.debug_text）：把后端解码出的**原始**行（筛分前）打进本地
       业务日志——现场要看的正是"分数筛吃掉了什么"，所以打印在筛之前、带各自的分数。
       单行截到 96 字节防刷屏；只进本地日志，心跳与上报的计数/哈希口径不变 */
    if (m_stParam.bDebugText)
    {
        dlog_info("classroom board: 调试-原始行 raw=%zu dueMs=%lld",
                  vecText.size(), static_cast<long long>(llDueMs));
        for (size_t i = 0; i < vecText.size(); ++i)
        {
            /* 按 UTF-8 边界截，不用 `%.*s` 按字节硬切。
               板书正文一个字都不进报文（第 7 刀 PII 纪律），但**这条调试日志会打原文**，
               现场排"分数筛吃掉了什么"要看的就是原文。按字节硬切在中文上会切出半个字，
               日志里留下乱码或半个引号，读日志的人反而看不出那行写的是什么。
               truncateUtf8 住在 common 纯函数层：
               perception 不许 include behavior，而两处都需要同一个函数。 */
            bool bCut = false;
            const std::string strShow = truncateUtf8(vecText[i].strText, 96, bCut);
            dlog_info("classroom board:   [%zu] 分数=%.3f 文字='%s'%s",
                      i, static_cast<double>(vecText[i].fScore), strShow.c_str(),
                      bCut ? "…(截)" : "");
        }
    }

    /* 三步的顺序就是这一段的判据：先按分数筛（只看分数）→ 再按阅读序排（要看行框）→
       最后才截到行上限（`m_stParam.nLineMax`，宏是它的上界）。截断排在排序**之前**的话，截的是 det 交回来的
       置信度序，而那个序每帧都抖：同一块板两帧会留下不同的 24 行，
       kept=/byte= 随帧抖，而那 24 行的集合哈希也跟着变——凭空多出一条"板书变了"，
       正是本文件头第二条要避免的那件事在溢出分支上复活 */
    std::vector<CBoardLine_S> vecLine;
    vecLine.reserve(vecText.size());
    /* 后端交回来的框数在这里计，**在筛之前**：这一列与 kept= 的差就是"被分数吃掉多少行"，
       放到筛之后再计，raw 与 kept 恒等，那一列就成了装饰 */
    m_stStat.u64BoxRaw += static_cast<uint64_t>(vecText.size());
    for (const auto &stText : vecText)
    {
        if (!(stText.fScore >= m_stParam.fMinLineScore))
        {
            /* 分数不足的行不进哈希也不进正文：OCR 对被挡住的半行常给 0.2 上下的乱码，
               这些字每帧都不同，进哈希就等于每帧都"板书变了" */
            continue;
        }
        CBoardLine_S stLine;
        stLine.strNorm = normalizeForHash(stText.strText);
        if (stLine.strNorm.empty())
        {
            continue;
        }
        stLine.stText = stText;
        vecLine.push_back(stLine);
    }

    if (vecLine.empty())
    {
        ++m_stStat.u64EmptyBoard;
    }
    else
    {
        std::sort(vecLine.begin(), vecLine.end(), lineLess);
        /* 上限取自参数而不是宏：那一格现在是可配的（`board.line_max`，#64 第 9 刀），
           宏只是它的**上界**（定长表按宏开，配大了存不下）。参数被绕过配置层直接设成
           0 或负数时按宏走——"截到 0 行"不是一种保守，是把这条功能整条关掉而读数上看不出来 */
        const int32_t nLineMax = (m_stParam.nLineMax > 0 &&
                                  m_stParam.nLineMax <= CLASSROOM_BOARD_LINE_MAX)
                                     ? m_stParam.nLineMax
                                     : CLASSROOM_BOARD_LINE_MAX;
        if (static_cast<int32_t>(vecLine.size()) > nLineMax)
        {
            ++m_stStat.u64LineOverflow;
            vecLine.resize(static_cast<size_t>(nLineMax));
        }
        m_stStat.u64LineKept += static_cast<uint64_t>(vecLine.size());
    }

    m_vecLineHashStr.clear();
    for (const auto &stLine : vecLine)
    {
        m_vecLineHashStr.push_back(stLine.strNorm);
    }

    const uint64_t u64Hash = hashOfSet(m_vecLineHashStr, static_cast<int32_t>(m_vecLineHashStr.size()));
    if (m_bHasLastHash && u64Hash == m_u64LastHash)
    {
        ++m_stStat.u64NoChange;
        return false;
    }
    m_bHasLastHash = true;
    m_u64LastHash = u64Hash;

    /* 长度与分数在这里加总，**不拼文本**：拼接出来的那一条整段正文在本函数之外没有用途
       （第 7 刀之前它是上报字段，现在是 PII），一份从不存在的东西不可能被误发出去——
       所以这里的口径是"行数 + 字节数 + 均分"，与记录里那三列一一对应。
       换行分隔符按 1 字节计入总长（与旧版的 strText.size() 逐字节同值，别让报表换算法） */
    int32_t nBytes = 0;
    float fScoreSum = 0.0f;
    for (size_t i = 0; i < vecLine.size(); ++i)
    {
        if (0 != i)
        {
            ++nBytes;
        }
        nBytes += static_cast<int32_t>(vecLine[i].stText.strText.size());
        fScoreSum += vecLine[i].stText.fScore;
    }

    if (nBytes > m_stParam.nTextMaxBytes)
    {
        /* 整条判非法而不是截断（见头文件里那条理由）。哈希上面已经更新：
           这一块的板书写得再满也只报一次"太长"，不会每周期重复占用上报配额 */
        dlog_warn("classroom board: 文本 %d 字节超过上限 %d，条目被丢弃",
                  nBytes, m_stParam.nTextMaxBytes);
        ++m_stStat.u64TextTooLong;
        return false;
    }

    CBoardItem_S stItem;
    stItem.u32Seq = m_u32Seq;
    ++m_u32Seq;
    stItem.llTsMs = llDueMs;
    stItem.nTextBytes = nBytes;
    stItem.fConfidence = vecLine.empty() ? 0.0f : (fScoreSum / static_cast<float>(vecLine.size()));
    stItem.nLineNum = static_cast<int32_t>(vecLine.size());
    /* 千分位整数累加，**只累有内容的记录**（分母 u64ConfNum 同一条件，见头文件那一格）：
       不直接存均值，是因为两条记录的平均再平均不等于总体平均 */
    if (!vecLine.empty())
    {
        m_stStat.u64ConfPermilleSum +=
            static_cast<uint64_t>(static_cast<double>(stItem.fConfidence) * 1000.0 + 0.5);
        ++m_stStat.u64ConfNum;
    }
    m_stStat.u64TextBytes += static_cast<uint64_t>(nBytes);

    if (m_nNum >= m_nCap)
    {
        /* 环满丢最旧并计数：上报节奏由引擎决定，本类不反压回去（反压会把采集停掉，
           而停采集是用户看不见的静默失效，丢最旧是看得见的信息缺失） */
        m_nHead = (m_nHead + 1) % m_nCap;
        --m_nNum;
        ++m_stStat.u64DropOldest;
    }
    const int32_t nTail = (m_nHead + m_nNum) % m_nCap;
    m_vecItem[static_cast<size_t>(nTail)] = stItem;
    ++m_nNum;
    /* 上面「满则先 head 前移一格、num 减一」保证了这里 ++ 之后 num 不会超过 cap，
       所以尾下标永远落在刚腾出的那一格上 */

    ++m_stStat.u64Emitted;
    m_stStat.nLastLineNum = stItem.nLineNum;
    return true;
}

bool CBoardWatcher::pop(CBoardItem_S &rstItem)
{
    if (m_nNum <= 0)
    {
        return false;
    }
    rstItem = m_vecItem[static_cast<size_t>(m_nHead)];
    m_vecItem[static_cast<size_t>(m_nHead)] = CBoardItem_S();
    m_nHead = (m_nHead + 1) % m_nCap;
    --m_nNum;
    return true;
}

int32_t CBoardWatcher::pendingNum() const
{
    return m_nNum;
}

void CBoardWatcher::closeSession(int64_t llNowMs)
{
    (void)llNowMs;
    /* 只清在途与变更基线：新会话的第一次采集必然与上一节课不同，
       但"不同"要由内容本身说出来，不能靠留着上一节课的哈希把它吃掉 */
    m_bInFlight = false;
    m_llFlightSinceMs = 0;
    m_llFlightDueMs = 0;
    m_u32FlightSeq = 0;
    m_llLastDueMs = 0;
    /* 挂着的那次边沿也不带给下一节课：它是上一节课最后一块板的请求，
       在新会话里采到的是擦干净之后的板，报成一条"板书变更"是凭空造内容 */
    m_bEpisode = false;
    m_llEpisodeMs = 0;
    /* 停留锚点同理：上一节课站到铃响的那段，不该在新会话第一次"离开"时兑现 */
    m_llStaySinceMs = 0;
    m_bHasLastHash = false;
    m_u64LastHash = 0;
}

void CBoardWatcher::reset()
{
    closeSession(0);
    m_nHead = 0;
    m_nNum = 0;
    m_u32Seq = 0;
    for (auto &stItem : m_vecItem)
    {
        stItem = CBoardItem_S();
    }
    m_stStat = CBoardStat_S();
    m_enLastState = BackendState::MODEL_MISSING;
}

std::string CBoardWatcher::statText() const
{
    /* 448B：留到 448 而不是"够用就行" */
    char szBuf[448] = {0};
    /* 平均置信度：整条心跳只有计数没有正文，这一列就是"OCR 在今天掉没掉"的唯一读数。
       分母只数有内容的记录（u64ConfNum），空板不进分母 */
    const uint64_t u64ConfNum = m_stStat.u64ConfNum;
    const uint64_t u64ConfAvgPermille =
        (u64ConfNum > 0) ? (m_stStat.u64ConfPermilleSum / u64ConfNum) : 0;
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "board want=%llu ep=%llu mg=%llu cap=%llu occ=%llu nogeom=%llu noreg=%llu be=%llu unav=%llu same=%llu emit=%llu empty=%llu tmo=%llu sq=%llu drop=%llu long=%llu raw=%llu kept=%llu byte=%llu conf=%llu pend=%d rect=%d seq=%u fseq=%u st=%d warm=%llu lastjob=%lldms trunc=%llu",
                              static_cast<unsigned long long>(m_stStat.u64Want),
                              static_cast<unsigned long long>(m_stStat.u64Episode),
                              static_cast<unsigned long long>(m_stStat.u64EpisodeMerged),
                              static_cast<unsigned long long>(m_stStat.u64Capture),
                              static_cast<unsigned long long>(m_stStat.u64SkipOccluded),
                              static_cast<unsigned long long>(m_stStat.u64SkipNoGeom),
                              static_cast<unsigned long long>(m_stStat.u64NoRegion),
                              static_cast<unsigned long long>(m_stStat.u64BackendFail),
                              static_cast<unsigned long long>(m_stStat.u64BackendUnavailable),
                              static_cast<unsigned long long>(m_stStat.u64NoChange),
                              static_cast<unsigned long long>(m_stStat.u64Emitted),
                              static_cast<unsigned long long>(m_stStat.u64EmptyBoard),
                              static_cast<unsigned long long>(m_stStat.u64InflightTimeout),
                              static_cast<unsigned long long>(m_stStat.u64ResultStale),
                              static_cast<unsigned long long>(m_stStat.u64DropOldest),
                              static_cast<unsigned long long>(m_stStat.u64TextTooLong),
                              /* 这四列合起来才是"识别得好不好"：交回来多少框（raw）、
                                 留下多少行（kept）、写了多少字节（byte）、平均分（conf 千分位）。
                                 第 7 刀之前这里能放正文，现在放不了也不该放（PII） */
                              static_cast<unsigned long long>(m_stStat.u64BoxRaw),
                              static_cast<unsigned long long>(m_stStat.u64LineKept),
                              static_cast<unsigned long long>(m_stStat.u64TextBytes),
                              static_cast<unsigned long long>(u64ConfAvgPermille),
                              m_nNum,
                              m_bRectReady ? 1 : 0,
                              m_u32Seq,
                              /* sq= 与 fseq= 并排才读得出两种病："号配不上"（sq 在涨）与
                                 "号挂着不回来"（fseq 隔很久还是同一个数）*/
                              m_u32FlightSeq,
                              static_cast<int>(m_enLastState),
                              /* warm= 之后是第 10 刀那两列（放最后=新加的，前面的次序是历史读数）：
                                 lastjob= 一次板面识别整帧耗时（后端 det+rec），trunc= 后端累计截行数。
                                 -1 会打成 lastjob=-1ms，读的人据此知道"这一拍没带新读数"而不是"0 毫秒" */
                              static_cast<unsigned long long>(m_stStat.u64SkipNotWarm),
                              static_cast<long long>(m_stStat.llLastJobMs),
                              static_cast<unsigned long long>(m_stStat.u64TruncTotal));
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        return "board stat=truncated";
    }
    return std::string(szBuf);
}
}
