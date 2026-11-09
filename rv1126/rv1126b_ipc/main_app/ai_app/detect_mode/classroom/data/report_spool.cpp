/**
 * @FilePath     : report_spool.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-21 13:48:32
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 13:48:32
 * @Description  : 报表滞留落盘环实现（三段式：追加写 / 按序读 / 整段回收）
 */

#include "report_spool.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "classroom_clock.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 记录头 36 字节，逐字段小端落盘 */
constexpr uint32_t REC_MAGIC = 0x504C5343;   /* 'C' 'S' 'L' 'P' */
constexpr uint16_t REC_VER = 1;
constexpr int32_t REC_HDR_SIZE = 36;
constexpr int32_t REC_PAYLOAD_MAX = (64 * 1024);

constexpr int32_t SPOOL_MB_MIN = 1;
constexpr int32_t SPOOL_MB_MAX = 512;
constexpr int32_t SPOOL_SEG_KB_MIN = 4;      /* 一个页大小。下限存在的唯一理由是"一段连一条记录都放不下
                                                时轮转会变成常态"，与 flash 块大小无关；卡死在 128KB
                                                只会让"多条记录跨段"这类真实形态无法用小样本验证 */
constexpr int32_t SPOOL_SEG_KB_MAX = 8192;

constexpr char SEG_PREFIX[] = "clspl_";
constexpr char SEG_SUFFIX[] = ".dat";
constexpr int32_t SEG_NUM_DIGITS = 16;

/* 游标文件（不变式 4）。8 个 64B 槽轮转覆盖，整个文件正好一个 flash 块：
   - 定长槽 + 轮转 → 每次 commit 只改 64B，且改写位置在动，不会反复擦同一块
   - 槽里有 slide 单调号与独立校验 → 掉电打断某一次写，只会废掉那一个槽，其余槽仍指向一个
     合法（略旧）的游标。旧游标的后果是重发，不是漏发，这与不变式 4 选的失效方向一致
   - 不用"文件截断重写"的做法：那需要先 ftruncate 再写，中间掉电就是零长度文件，游标全丢 */
constexpr char CTL_FILE_NAME[] = "clspl.ctl";
constexpr int32_t CTL_SLOT_SIZE = 64;
constexpr int32_t CTL_SLOT_NUM = 8;
constexpr uint32_t CTL_MAGIC = 0x434C5343;   /* 'C' 'S' 'L' 'C' */
constexpr uint16_t CTL_VER = 1;
constexpr int32_t CTL_OFF_CRC = 60;          /* 槽尾 4 字节是前 60 字节的 FNV 截断 */

/* 一次 peek 最多跳几段：目录里全是坏段时不能把节拍线程拖在 IO 里 */
constexpr int32_t PEEK_LOOP_MAX = 64;

inline void putU16(uint8_t *p, uint16_t u16Val)
{
    p[0] = static_cast<uint8_t>(u16Val & 0xFF);
    p[1] = static_cast<uint8_t>((u16Val >> 8) & 0xFF);
}

inline void putU32(uint8_t *p, uint32_t u32Val)
{
    for (int32_t i = 0; i < 4; ++i)
    {
        p[i] = static_cast<uint8_t>((u32Val >> (8 * i)) & 0xFF);
    }
}

inline void putU64(uint8_t *p, uint64_t u64Val)
{
    for (int32_t i = 0; i < 8; ++i)
    {
        p[i] = static_cast<uint8_t>((u64Val >> (8 * i)) & 0xFF);
    }
}

inline uint16_t getU16(const uint8_t *p)
{
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

inline uint32_t getU32(const uint8_t *p)
{
    uint32_t u32Val = 0;
    for (int32_t i = 0; i < 4; ++i)
    {
        u32Val |= static_cast<uint32_t>(p[i]) << (8 * i);
    }
    return u32Val;
}

inline uint64_t getU64(const uint8_t *p)
{
    uint64_t u64Val = 0;
    for (int32_t i = 0; i < 8; ++i)
    {
        u64Val |= static_cast<uint64_t>(p[i]) << (8 * i);
    }
    return u64Val;
}

/* FNV-1a 64：只为检出"写一半掉电/文件系统截断"，不是防篡改，所以不引第三方 CRC 表 */
uint64_t fnv1a64(const void *pData, size_t uLen)
{
    const uint8_t *pBytes = static_cast<const uint8_t *>(pData);
    uint64_t u64Hash = 0xCBF29CE484222325ULL;
    for (size_t i = 0; i < uLen; ++i)
    {
        u64Hash ^= pBytes[i];
        u64Hash *= 0x100000001B3ULL;
    }
    return u64Hash;
}

/* 记录序号 = 段号在高 32 位 + 段内序号在低 32 位。段号只增不减（重启后也是从目录里取最大值+1），
   所以这个组合天然严格递增、跨重启稳定，且不需要任何额外的持久化计数器 */
inline uint64_t makeSeq(int64_t llSegNum, uint32_t u32Idx)
{
    return (static_cast<uint64_t>(static_cast<uint32_t>(llSegNum)) << 32) |
           static_cast<uint64_t>(u32Idx);
}

bool makeDirs(const std::string &strPath, int32_t &rnErrno)
{
    char szWalk[512];
    if (strPath.empty() || strPath.size() >= sizeof(szWalk))
    {
        rnErrno = ENAMETOOLONG;
        return false;
    }
    snprintf(szWalk, sizeof(szWalk), "%s", strPath.c_str());

    const size_t uLen = strlen(szWalk);
    for (size_t i = 1; i < uLen; ++i)
    {
        if ('/' != szWalk[i])
        {
            continue;
        }
        szWalk[i] = '\0';
        if (0 != ::mkdir(szWalk, 0775) && EEXIST != errno)
        {
            rnErrno = errno;
            return false;
        }
        szWalk[i] = '/';
    }
    if (0 != ::mkdir(szWalk, 0775) && EEXIST != errno)
    {
        rnErrno = errno;
        return false;
    }
    rnErrno = 0;
    return true;
}

/* 目录项名 → 段号。要求"前缀 + 恰好 16 位数字 + 后缀"，其它一律忽略：
   现场有人往这个目录里丢 README.txt 不该被当成一段待补传数据 */
bool parseSegName(const char *pszName, int64_t &rllNum)
{
    const size_t uNameLen = strlen(pszName);
    const size_t uWant = strlen(SEG_PREFIX) + SEG_NUM_DIGITS + strlen(SEG_SUFFIX);
    if (uNameLen != uWant)
    {
        return false;
    }
    if (0 != strncmp(pszName, SEG_PREFIX, strlen(SEG_PREFIX)) ||
        0 != strcmp(pszName + uNameLen - strlen(SEG_SUFFIX), SEG_SUFFIX))
    {
        return false;
    }
    char szNum[SEG_NUM_DIGITS + 1] = {0};
    memcpy(szNum, pszName + strlen(SEG_PREFIX), SEG_NUM_DIGITS);
    for (int32_t i = 0; i < SEG_NUM_DIGITS; ++i)
    {
        if (szNum[i] < '0' || szNum[i] > '9')
        {
            return false;
        }
    }

    char *pszEnd = nullptr;
    const long long llVal = strtoll(szNum, &pszEnd, 10);
    if (nullptr == pszEnd || '\0' != *pszEnd || llVal <= 0)
    {
        return false;
    }
    rllNum = llVal;
    return true;
}

int32_t fullRead(int32_t nFd, void *pBuf, int32_t nNeed, int64_t llOff)
{
    uint8_t *pWalk = static_cast<uint8_t *>(pBuf);
    int32_t nGot = 0;
    while (nGot < nNeed)
    {
        const ssize_t nRet = ::pread(nFd, pWalk + nGot, static_cast<size_t>(nNeed - nGot),
                                     llOff + nGot);
        if (nRet > 0)
        {
            nGot += static_cast<int32_t>(nRet);
            continue;
        }
        if (0 > nRet && EINTR == errno)
        {
            continue;
        }
        break;   /* 0=EOF，-1=出错：两种都按"读不出完整记录"处理 */
    }
    return nGot;
}
}   /* namespace */

CReportSpool::~CReportSpool()
{
    deinit();
}

bool CReportSpool::init(const CSpoolParam_S &stParam, std::string &rstrErr)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    /* 允许对同一个对象二次 init（换目录、改容量）：先把上一轮的 fd 全放下。
       不这么做的话，"换了个 strDir 但 fd 还是旧目录的"会写到一个没人扫描的地方 */
    closeWriteSegLocked();
    if (m_fdRead >= 0)
    {
        ::close(m_fdRead);
        m_fdRead = -1;
    }
    if (m_fdCtl >= 0)
    {
        ::close(m_fdCtl);
        m_fdCtl = -1;
    }

    if (!stParam.bEnable)
    {
        m_bReady = false;
        rstrErr = "disabled by config";
        return false;
    }
    if (stParam.strDir.empty())
    {
        m_bReady = false;
        rstrErr = "empty spool dir";
        return false;
    }

    /* 夹紧而不是拒绝：这是"滞留能力"的参数，配错了按边界跑 + 告警，
       比让整条分析链因为一个容量数字起不来更合现场预期 */
    m_stParam = stParam;
    int32_t nMaxMb = stParam.nMaxMb;
    if (nMaxMb < SPOOL_MB_MIN || nMaxMb > SPOOL_MB_MAX)
    {
        dlog_warn("classroom spool: max_mb %d 超 [%d,%d], 已夹取", nMaxMb,
                  SPOOL_MB_MIN, SPOOL_MB_MAX);
        nMaxMb = (nMaxMb < SPOOL_MB_MIN) ? SPOOL_MB_MIN : SPOOL_MB_MAX;
    }
    int32_t nSegKb = stParam.nSegMaxKb;
    if (nSegKb < SPOOL_SEG_KB_MIN || nSegKb > SPOOL_SEG_KB_MAX)
    {
        dlog_warn("classroom spool: seg_max_kb %d 超 [%d,%d], 已夹取", nSegKb,
                  SPOOL_SEG_KB_MIN, SPOOL_SEG_KB_MAX);
        nSegKb = (nSegKb < SPOOL_SEG_KB_MIN) ? SPOOL_SEG_KB_MIN : SPOOL_SEG_KB_MAX;
    }
    /* 段不能大于总量，否则"最多一段"就等于没有上限可言 */
    if (nSegKb > nMaxMb * 1024)
    {
        nSegKb = nMaxMb * 1024;
    }
    m_llMaxBytes = static_cast<int64_t>(nMaxMb) * 1024 * 1024;
    m_llSegMaxBytes = static_cast<int64_t>(nSegKb) * 1024;

    std::string strDir = stParam.strDir;
    if ('/' != strDir[strDir.size() - 1])
    {
        strDir += '/';
    }
    m_stParam.strDir = strDir;

    int32_t nErrno = 0;
    if (!makeDirs(strDir, nErrno))
    {
        m_bReady = false;
        char szErr[128] = {0};
        snprintf(szErr, sizeof(szErr), "mkdir %s failed: %s", strDir.c_str(), strerror(nErrno));
        rstrErr = szErr;
        ++m_stStat.u64IoFail;
        return false;
    }

    if (!scanLocked(rstrErr))
    {
        m_bReady = false;
        return false;
    }

    m_bReady = true;
    dlog_info("classroom spool: 就绪 dir=%s max=%lldKB seg=%lldKB segs=%d used=%lldKB",
              strDir.c_str(), static_cast<long long>(m_llMaxBytes / 1024),
              static_cast<long long>(m_llSegMaxBytes / 1024),
              static_cast<int32_t>(m_vecSeg.size()),
              static_cast<long long>(m_llUsedBytes / 1024));
    return true;
}

void CReportSpool::deinit()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    closeWriteSegLocked();
    if (m_fdRead >= 0)
    {
        ::close(m_fdRead);
        m_fdRead = -1;
    }
    if (m_fdCtl >= 0)
    {
        ::close(m_fdCtl);
        m_fdCtl = -1;
    }
    m_bReady = false;
    /* 文件留着：跨重启正是这个环存在的理由之一（断网 + 重启期间报表不能丢） */
}

bool CReportSpool::isReady() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bReady;
}

bool CReportSpool::scanLocked(std::string &rstrErr)
{
    m_vecSeg.clear();
    m_llUsedBytes = 0;
    m_llNextSegNum = 1;
    m_llWriteBytes = 0;
    m_u32WriteRecIdx = 0;
    m_bWriteSegSealed = false;
    m_llReadSegNum = -1;
    m_llReadOff = 0;
    m_llReadSize = 0;
    m_llPendingRecBytes = 0;

    DIR *pDir = ::opendir(m_stParam.strDir.c_str());
    if (nullptr == pDir)
    {
        char szErr[160] = {0};
        snprintf(szErr, sizeof(szErr), "opendir %s failed: %s", m_stParam.strDir.c_str(),
                 strerror(errno));
        rstrErr = szErr;
        ++m_stStat.u64IoFail;
        return false;
    }

    struct dirent *pEnt = nullptr;
    while (nullptr != (pEnt = ::readdir(pDir)))
    {
        int64_t llNum = 0;
        if (!parseSegName(pEnt->d_name, llNum))
        {
            continue;
        }
        std::string strPath = m_stParam.strDir + pEnt->d_name;
        struct stat stInfo;
        if (0 != ::stat(strPath.c_str(), &stInfo) || !S_ISREG(stInfo.st_mode))
        {
            ++m_stStat.u64IoFail;
            continue;
        }
        /* 0 字节的段是"建了没写"的残留（上一进程刚轮转就掉了）：留着当当前段用，
           不删——删了再建只是多一次目录写 */
        CSegInfo_S stSeg;
        stSeg.llNum = llNum;
        stSeg.llSize = static_cast<int64_t>(stInfo.st_size);
        m_vecSeg.push_back(stSeg);
        if (llNum >= m_llNextSegNum)
        {
            m_llNextSegNum = llNum + 1;
        }
    }
    ::closedir(pDir);

    std::sort(m_vecSeg.begin(), m_vecSeg.end(),
              [](const CSegInfo_S &a, const CSegInfo_S &b) { return a.llNum < b.llNum; });
    for (const auto &stSeg : m_vecSeg)
    {
        m_llUsedBytes += stSeg.llSize;
    }

    if (m_vecSeg.empty())
    {
        /* 目录空 ≠ 没有持久状态。段号水位住在 ctl 里，而"环被排干"（补传读完 ⇒ 唯一那段被
           unlink）与"配置 clear 作废积压"都会把目录清空——这里直接 return 就等于下一次开机
           从 1 重新发号，新记录拿到与上一批**逐字节相同**的 seq */
        readCtlLocked();
        return true;
    }

    /* 不变式 2：只有最新段可能有半条记录，所以只修它。
       扫描从 0 开始逐条走，走到第一个"读不出完整记录"的边界就是可信长度 */
    CSegInfo_S &stLast = m_vecSeg.back();
    const std::string strLastPath = segPathLocked(stLast.llNum);
    const int32_t nFd = ::open(strLastPath.c_str(), O_RDWR);
    if (nFd < 0)
    {
        char szErr[192] = {0};
        snprintf(szErr, sizeof(szErr), "open %s failed: %s", strLastPath.c_str(), strerror(errno));
        rstrErr = szErr;
        ++m_stStat.u64IoFail;
        return false;
    }

    int64_t llOff = 0;
    uint32_t u32RecIdx = 0;
    uint8_t astHdr[REC_HDR_SIZE] = {0};
    for (;;)
    {
        if (llOff + REC_HDR_SIZE > stLast.llSize)
        {
            break;
        }
        if (REC_HDR_SIZE != fullRead(nFd, astHdr, REC_HDR_SIZE, llOff))
        {
            break;
        }
        const uint32_t u32Magic = getU32(astHdr);
        const uint32_t u32Len = getU32(astHdr + 8);
        if (REC_MAGIC != u32Magic || u32Len > static_cast<uint32_t>(REC_PAYLOAD_MAX))
        {
            break;
        }
        if (llOff + REC_HDR_SIZE + static_cast<int64_t>(u32Len) > stLast.llSize)
        {
            break;
        }
        /* 校验体：只验头部字段的话，"头部写完、体写了一半"会被当成合法记录读出去 */
        std::vector<uint8_t> vecBody(u32Len);
        if (u32Len > 0 &&
            static_cast<int32_t>(u32Len) != fullRead(nFd, vecBody.data(), static_cast<int32_t>(u32Len),
                                                     llOff + REC_HDR_SIZE))
        {
            break;
        }
        if (getU64(astHdr + 28) != fnv1a64(vecBody.data(), u32Len))
        {
            break;
        }
        llOff += REC_HDR_SIZE + static_cast<int64_t>(u32Len);
        ++u32RecIdx;   /* 这条是好的，写段的"段内第几条"要从它之后接着数 */
    }
    ::close(nFd);
    m_u32WriteRecIdx = u32RecIdx;

    if (llOff != stLast.llSize)
    {
        const int64_t llTorn = stLast.llSize - llOff;
        if (0 != ::truncate(strLastPath.c_str(), llOff))
        {
            dlog_warn("classroom spool: 截断 %s 到 %lld 失败: %s", strLastPath.c_str(),
                      static_cast<long long>(llOff), strerror(errno));
        }
        m_stStat.u64TornTail += static_cast<uint64_t>(llTorn);
        m_llUsedBytes -= llTorn;
        stLast.llSize = llOff;
        dlog_warn("classroom spool: 已修复最新段 %s，丢弃 %lldB 残尾 (used=%lldKB)",
                  strLastPath.c_str(), static_cast<long long>(llTorn),
                  static_cast<long long>(m_llUsedBytes / 1024));
    }

    /* 游标必须在撕裂尾修完之后恢复：它要把偏移夹到"该段的实际可信长度"上，
       夹在一个还没截干净的文件长度上会把坏字节当成待读数据 */
    readCtlLocked();
    return true;
}

std::string CReportSpool::segPathLocked(int64_t llNum) const
{
    char szName[64] = {0};
    snprintf(szName, sizeof(szName), "%s%016lld%s", SEG_PREFIX, static_cast<long long>(llNum),
             SEG_SUFFIX);
    return m_stParam.strDir + szName;
}

std::string CReportSpool::ctlPathLocked() const
{
    return m_stParam.strDir + CTL_FILE_NAME;
}

void CReportSpool::writeCtlLocked()
{
    if (!m_bReady)
    {
        return;
    }
    if (m_fdCtl < 0)
    {
        /* 一次把整个文件撑到 512B：之后每次都是"覆盖已有块"，既不扩长度也不动 extent 树，
           这才让"每条 commit 写一次游标"的代价稳在 64B 数据写上 */
        const int32_t nFd = ::open(ctlPathLocked().c_str(), O_RDWR | O_CREAT, 0664);
        if (nFd < 0)
        {
            ++m_stStat.u64IoFail;
            return;
        }
        struct stat stInfo;
        if (0 == ::fstat(nFd, &stInfo) && static_cast<int32_t>(stInfo.st_size) < CTL_SLOT_SIZE * CTL_SLOT_NUM)
        {
            if (0 != ::ftruncate(nFd, CTL_SLOT_SIZE * CTL_SLOT_NUM))
            {
                dlog_warn("classroom spool: ctl 延长失败: %s", strerror(errno));
            }
        }
        m_fdCtl = nFd;
    }

    const uint32_t u32Slot = m_u32CtlSlot;
    uint8_t astSlot[CTL_SLOT_SIZE] = {0};
    putU32(astSlot, CTL_MAGIC);
    putU16(astSlot + 4, CTL_VER);
    putU16(astSlot + 6, static_cast<uint16_t>(u32Slot));
    putU64(astSlot + 8, m_u64CtlSlide + 1);
    putU64(astSlot + 16, static_cast<uint64_t>(m_llReadSegNum));
    putU64(astSlot + 24, static_cast<uint64_t>(m_llReadOff));
    /* 32..39：段号水位（= 下一条新段的号） */
    putU64(astSlot + 32, static_cast<uint64_t>(m_llNextSegNum));
    /* 校验只覆盖前 60 字节，末尾 4 字节留给校验本身，否则自指 */
    putU32(astSlot + CTL_OFF_CRC,
           static_cast<uint32_t>(fnv1a64(astSlot, static_cast<size_t>(CTL_OFF_CRC)) & 0xFFFFFFFFu));

    m_u64CtlSlide += 1;
    m_u32CtlSlot = (u32Slot + 1) % CTL_SLOT_NUM;

    const ssize_t nRet = ::pwrite(m_fdCtl, astSlot, CTL_SLOT_SIZE,
                                  static_cast<off_t>(u32Slot) * CTL_SLOT_SIZE);
    if (nRet != CTL_SLOT_SIZE)
    {
        /* 不告警第二次：断网补传时这条会以每条记录一次的频率刷，日志会被它自己刷爆。
           写失败的后果只是"重启后可能重发"，交给 ctl_bad 读数在心跳里说明 */
        ++m_stStat.u64IoFail;
    }
    else
    {
        ++m_stStat.u64CtlWrite;
    }
}

void CReportSpool::readCtlLocked()
{
    const std::string strPath = ctlPathLocked();
    struct stat stInfo;
    if (0 != ::stat(strPath.c_str(), &stInfo) || 0 == stInfo.st_size)
    {
        return;   /* 首次运行：没有游标就是"从头开始"，不是异常 */
    }

    const int32_t nFd = ::open(strPath.c_str(), O_RDONLY);
    if (nFd < 0)
    {
        ++m_stStat.u64IoFail;
        return;
    }
    uint8_t astAll[CTL_SLOT_SIZE * CTL_SLOT_NUM] = {0};
    const int32_t nGot = fullRead(nFd, astAll,
                                  static_cast<int32_t>(sizeof(astAll)), 0);
    ::close(nFd);

    int32_t nBest = -1;
    uint64_t u64BestSlide = 0;
    for (int32_t i = 0; i + CTL_SLOT_SIZE <= nGot; i += CTL_SLOT_SIZE)
    {
        const uint8_t *p = astAll + i;
        if (CTL_MAGIC != getU32(p) || CTL_VER != getU16(p + 4))
        {
            continue;   /* 全零槽（还没写过）与版本不符的旧格式：跳过而不是判异常 */
        }
        const uint32_t u32Crc = getU32(p + CTL_OFF_CRC);
        if (u32Crc != static_cast<uint32_t>(fnv1a64(p, static_cast<size_t>(CTL_OFF_CRC)) & 0xFFFFFFFFu))
        {
            continue;
        }
        const uint64_t u64Slide = getU64(p + 8);
        if (nBest < 0 || u64Slide > u64BestSlide)
        {
            nBest = i;
            u64BestSlide = u64Slide;
        }
    }
    if (nBest < 0)
    {
        ++m_stStat.u64CtlBad;
        dlog_warn("classroom spool: ctl 存在但无有效槽位，积压将重放");
        return;
    }

    const uint8_t *p = astAll + nBest;
    const int64_t llSegNum = static_cast<int64_t>(getU64(p + 16));
    const int64_t llReadOff = static_cast<int64_t>(getU64(p + 24));
    const int64_t llWater = static_cast<int64_t>(getU64(p + 32));
    m_u64CtlSlide = u64BestSlide;

    /* 段号水位只做"抬高"，不做"接管"：目录里还有段时以目录算出的那个为准（下面这行取的是 max，
       且只在 ctl 值更大时生效），目录空了才靠它接着往下发号。老 ctl（没写过这一格的）读到 0，
       max 之后等于什么都没发生——退回本次改动之前的行为，不会更糟 */
    if (llWater > m_llNextSegNum)
    {
        m_llNextSegNum = llWater;
    }

    /* 游标指向的段可能已经被回收（补传读完就 unlink，之后才掉电）。这时按"更新的段全部没发过"
       处理：宁可重发一段，也不能把未发的当成已发的 */
    bool bFound = false;
    if (llSegNum > 0)
    {
        for (const auto &stSeg : m_vecSeg)
        {
            if (stSeg.llNum == llSegNum)
            {
                m_llReadSegNum = llSegNum;
                m_llReadOff = (llReadOff < stSeg.llSize) ? llReadOff : stSeg.llSize;
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            for (const auto &stSeg : m_vecSeg)
            {
                if (stSeg.llNum > llSegNum)
                {
                    m_llReadSegNum = stSeg.llNum;
                    m_llReadOff = 0;
                    bFound = true;
                    break;
                }
            }
        }
    }
    if (!bFound)
    {
        m_llReadSegNum = -1;
        m_llReadOff = 0;
    }

    dlog_info("classroom spool: 游标已恢复 seg=%lld off=%lld (segs=%d)",
              static_cast<long long>(m_llReadSegNum), static_cast<long long>(m_llReadOff),
              static_cast<int32_t>(m_vecSeg.size()));
}

bool CReportSpool::ensureWriteSegLocked(std::string &rstrErr)
{
    if (m_fdWrite >= 0)
    {
        return true;
    }
    if (!m_bWriteSegSealed && !m_vecSeg.empty() && m_vecSeg.back().llSize < m_llSegMaxBytes)
    {
        /* 复用最后一段（重启后接着写，或刚清过 fd）：段号不变，写偏移取文件实际长度。
           刚轮转封口的那一段不算在内——它已经 fsync+close 了，再往里写等于把"这一段的长度
           从此不再变"这个承诺收回，而读侧正是按这个承诺决定何时 unlink 它的 */
        const std::string strPath = segPathLocked(m_vecSeg.back().llNum);
        const int32_t nFd = ::open(strPath.c_str(), O_RDWR);
        if (nFd >= 0)
        {
            m_fdWrite = nFd;
            m_llWriteBytes = m_vecSeg.back().llSize;
            return true;
        }
        ++m_stStat.u64IoFail;
        char szErr[192] = {0};
        snprintf(szErr, sizeof(szErr), "reopen %s failed: %s", strPath.c_str(), strerror(errno));
        rstrErr = szErr;
        return false;
    }

    const int64_t llNum = m_llNextSegNum++;
    const std::string strPath = segPathLocked(llNum);
    const int32_t nFd = ::open(strPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0664);
    if (nFd < 0)
    {
        ++m_stStat.u64IoFail;
        char szErr[192] = {0};
        snprintf(szErr, sizeof(szErr), "create %s failed: %s", strPath.c_str(), strerror(errno));
        rstrErr = szErr;
        return false;
    }
    /* 新文件的名字要在掉电后还在，否则写进去的数据没人找得到。目录 fsync 只在轮转时做一次，
       不是每条记录一次——这就是"每条 fsync"与"整段 fsync"的差别所在 */
    const int32_t nDirFd = ::open(m_stParam.strDir.c_str(), O_RDONLY | O_DIRECTORY);
    if (nDirFd >= 0)
    {
        ::fsync(nDirFd);
        ::close(nDirFd);
    }

    CSegInfo_S stSeg;
    stSeg.llNum = llNum;
    stSeg.llSize = 0;
    m_vecSeg.push_back(stSeg);
    m_fdWrite = nFd;
    m_llWriteBytes = 0;
    m_u32WriteRecIdx = 0;
    m_bWriteSegSealed = false;

    return true;
}

void CReportSpool::closeWriteSegLocked()
{
    if (m_fdWrite < 0)
    {
        return;
    }
    ::fsync(m_fdWrite);   /* 轮转点：这一整段以后不会再改，落定它 */
    ::close(m_fdWrite);
    m_fdWrite = -1;
}

void CReportSpool::dropOldestSegLocked()
{
    if (m_vecSeg.empty())
    {
        return;
    }
    const CSegInfo_S stFront = m_vecSeg.front();
    /* 正在写的那一段不能当"最旧"丢掉：它同时是唯一还没轮转的数据，也是写游标本身 */
    if (1 == m_vecSeg.size())
    {
        return;
    }

    if (m_fdRead >= 0)
    {
        ::close(m_fdRead);
        m_fdRead = -1;
    }
    m_llReadSegNum = -1;
    m_llReadOff = 0;
    m_llReadSize = 0;
    m_llPendingRecBytes = 0;

    const std::string strPath = segPathLocked(stFront.llNum);
    if (0 != ::unlink(strPath.c_str()) && ENOENT != errno)
    {
        dlog_warn("classroom spool: unlink %s 失败: %s", strPath.c_str(), strerror(errno));
        ++m_stStat.u64IoFail;
    }
    m_llUsedBytes -= stFront.llSize;
    m_stStat.u64DropSeg += 1;
    m_stStat.u64DropBytes += static_cast<uint64_t>(stFront.llSize);
    m_vecSeg.erase(m_vecSeg.begin());
}

bool CReportSpool::makeRoomLocked(int64_t llNeed, std::string &rstrErr)
{
    if (llNeed > m_llMaxBytes)
    {
        char szErr[128] = {0};
        snprintf(szErr, sizeof(szErr), "record %lldB exceeds spool max %lldB",
                 static_cast<long long>(llNeed), static_cast<long long>(m_llMaxBytes));
        rstrErr = szErr;
        return false;
    }

    while (m_llUsedBytes + llNeed > m_llMaxBytes)
    {
        if (m_vecSeg.empty())
        {
            break;   /* 已经没有可丢的了，剩下的空间一定够（上面判过 llNeed <= max） */
        }
        if (1 == m_vecSeg.size())
        {
            /* 只剩"读游标与写游标同处一段"这一种形态。此时超限丢最旧退化成一件事：
               整段作废。做法是把文件截回读游标（读游标必然 ≤ 写游标），两个游标一起归到该点 */
            const int64_t llKeep = (m_llReadOff < m_llWriteBytes) ? m_llReadOff : m_llWriteBytes;
            const int64_t llFreed = m_vecSeg.front().llSize - llKeep;
            const std::string strPath = segPathLocked(m_vecSeg.front().llNum);
            if (0 != ::truncate(strPath.c_str(), llKeep))
            {
                dlog_warn("classroom spool: 回收 %s 到 %lld 失败: %s", strPath.c_str(),
                          static_cast<long long>(llKeep), strerror(errno));
            }
            m_stStat.u64DropSeg += 1;
            m_stStat.u64DropBytes += static_cast<uint64_t>((llFreed > 0) ? llFreed : 0);
            m_llUsedBytes -= llFreed;
            m_llWriteBytes = llKeep;
            m_vecSeg.front().llSize = llKeep;
            m_llReadSize = llKeep;
            if (m_llReadOff > llKeep)
            {
                m_llReadOff = llKeep;
            }
            m_llPendingRecBytes = 0;
            if (m_llUsedBytes < 0)
            {
                m_llUsedBytes = 0;
            }
            dlog_warn("classroom spool: 积压只剩单个段仍溢出，已丢弃 %lldB",
                      static_cast<long long>(llFreed));
            break;
        }
        dropOldestSegLocked();
    }
    return true;
}

bool CReportSpool::append(uint32_t u32Type, const void *pPayload, size_t uLen, std::string &rstrErr)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        rstrErr = "spool not ready";
        return false;
    }
    if (nullptr == pPayload || 0 == uLen)
    {
        rstrErr = "empty payload";
        return false;
    }
    if (uLen > static_cast<size_t>(REC_PAYLOAD_MAX))
    {
        rstrErr = "payload exceeds record limit";
        ++m_stStat.u64DropRec;
        return false;
    }

    const int64_t llRecBytes = REC_HDR_SIZE + static_cast<int64_t>(uLen);

    if (m_llWriteBytes + llRecBytes > m_llSegMaxBytes)
    {
        closeWriteSegLocked();
        m_llWriteBytes = 0;
        m_bWriteSegSealed = true;   /* 封口：这一段从此长度不变，下一次写必须另起一段 */
    }
    if (!makeRoomLocked(llRecBytes, rstrErr))
    {
        ++m_stStat.u64AppendFail;
        return false;
    }
    if (!ensureWriteSegLocked(rstrErr))
    {
        ++m_stStat.u64AppendFail;
        return false;
    }

    m_vecScratch.assign(static_cast<size_t>(llRecBytes), 0u);
    uint8_t *pHdr = m_vecScratch.data();
    putU32(pHdr, REC_MAGIC);
    putU16(pHdr + 4, REC_VER);
    putU16(pHdr + 6, static_cast<uint16_t>(u32Type));
    putU32(pHdr + 8, static_cast<uint32_t>(uLen));
    putU64(pHdr + 12, makeSeq(m_vecSeg.back().llNum, m_u32WriteRecIdx));
    putU64(pHdr + 20, static_cast<uint64_t>(classroomNowMs()));
    memcpy(m_vecScratch.data() + REC_HDR_SIZE, pPayload, uLen);
    putU64(pHdr + 28, fnv1a64(pPayload, uLen));

    /* 一次 pwrite 把"头+体"作为一个块交下去：窗口越小，掉电时留下半条记录的概率越低。
       但这只是概率，不是保证——真正的保证在启动修复与读侧校验里 */
    const ssize_t nWritten = ::pwrite(m_fdWrite, m_vecScratch.data(), static_cast<size_t>(llRecBytes),
                                      m_llWriteBytes);
    if (nWritten != static_cast<ssize_t>(llRecBytes))
    {
        const int32_t nErr = (nWritten < 0) ? errno : EIO;
        /* 回到"已确认写完"的边界，文件里不留半条记录：否则这条坏记录会把整段的后半截
           都带进读侧的跳段逻辑（读侧遇到坏记录只能跳到下一段），白丢一段好数据 */
        if (0 != ::ftruncate(m_fdWrite, m_llWriteBytes))
        {
            dlog_warn("classroom spool: 回滚截断失败: %s", strerror(errno));
        }
        ++m_stStat.u64AppendFail;
        char szErr[128] = {0};
        snprintf(szErr, sizeof(szErr), "pwrite %lldB failed: %s", static_cast<long long>(llRecBytes),
                 strerror(nErr));
        rstrErr = szErr;
        return false;
    }

    m_u32WriteRecIdx += 1;
    m_llWriteBytes += llRecBytes;
    m_vecSeg.back().llSize += llRecBytes;
    m_llUsedBytes += llRecBytes;
    m_stStat.u64Append += 1;
    if (1 == m_vecSeg.size())
    {
        /* 只有一段时读写同处一个文件：读侧的"可见长度"必须跟着写边界走，
           否则刚落的记录这一拍读不到，下一拍才被看见（多一跳延迟且没人知道） */
        m_llReadSize = m_vecSeg.front().llSize;
    }
    return true;
}

bool CReportSpool::openReadSegLocked()
{
    if (m_fdRead >= 0)
    {
        return true;
    }
    if (m_vecSeg.empty())
    {
        return false;
    }
    const std::string strPath = segPathLocked(m_vecSeg.front().llNum);
    const int32_t nFd = ::open(strPath.c_str(), O_RDONLY);
    if (nFd < 0)
    {
        ++m_stStat.u64IoFail;
        dlog_warn("classroom spool: 打开读取段 %s 失败: %s", strPath.c_str(), strerror(errno));
        return false;
    }
    m_fdRead = nFd;
    if (m_llReadSegNum != m_vecSeg.front().llNum)
    {
        /* 只在"换了一段"时归零游标。同一段重开 fd（deinit 后重启、或单段场景）
           必须留在原地，否则已 commit 的记录会被再补传一遍——重复上报比漏报更难查 */
        m_llReadSegNum = m_vecSeg.front().llNum;
        m_llReadOff = 0;
    }
    m_llReadSize = m_vecSeg.front().llSize;
    return true;
}

int32_t CReportSpool::readRecordLocked(CSpoolRecord_S &rstRec, int64_t &rllRecBytes)
{
    rllRecBytes = 0;

    uint8_t astHdr[REC_HDR_SIZE] = {0};
    if (m_llReadOff + REC_HDR_SIZE > m_llReadSize)
    {
        return 0;   /* 本段读完（或不足一个头）：交给上层跳段 */
    }
    if (REC_HDR_SIZE != fullRead(m_fdRead, astHdr, REC_HDR_SIZE, m_llReadOff))
    {
        return 0;
    }
    const uint32_t u32Magic = getU32(astHdr);
    const uint32_t u32Len = getU32(astHdr + 8);
    if (REC_MAGIC != u32Magic)
    {
        return -1;
    }
    /* 版本必须同等校验 */
    if (REC_VER != getU16(astHdr + 4))
    {
        return -1;
    }
    if (u32Len > static_cast<uint32_t>(REC_PAYLOAD_MAX))
    {
        return -1;
    }
    if (m_llReadOff + REC_HDR_SIZE + static_cast<int64_t>(u32Len) > m_llReadSize)
    {
        /* 头是好的、体不够长：写到这里就没电了，属于短尾而不是坏数据 */
        return 0;
    }

    rstRec.strPayload.assign(u32Len, '\0');
    if (u32Len > 0 &&
        static_cast<int32_t>(u32Len) != fullRead(m_fdRead, &rstRec.strPayload[0],
                                                 static_cast<int32_t>(u32Len),
                                                 m_llReadOff + REC_HDR_SIZE))
    {
        rstRec.strPayload.clear();
        return 0;
    }
    if (getU64(astHdr + 28) != fnv1a64(rstRec.strPayload.data(), u32Len))
    {
        rstRec.strPayload.clear();
        return -1;
    }

    rstRec.u32Type = getU16(astHdr + 6);
    rstRec.u64Seq = getU64(astHdr + 12);
    rstRec.llTsMs = static_cast<int64_t>(getU64(astHdr + 20));
    rllRecBytes = REC_HDR_SIZE + static_cast<int64_t>(u32Len);
    return 1;
}

void CReportSpool::advanceToNextSegLocked(bool bConsumeCurrent)
{
    if (m_fdRead >= 0)
    {
        ::close(m_fdRead);
        m_fdRead = -1;
    }
    m_llPendingRecBytes = 0;

    if (m_vecSeg.empty())
    {
        m_llReadSegNum = -1;
        m_llReadOff = 0;
        m_llReadSize = 0;
        return;
    }
    if (1 == m_vecSeg.size())
    {
        /* 只有一段：它既是读段也是写段，回收它会连未写的数据一起没掉（不变式 3），
           而**归零读游标**会造成另一个更糟的后果——已 commit 的记录被重新补传一遍。
           所以这里只把"可见长度"跟到当前写边界，游标原地不动 */
        m_llReadSegNum = m_vecSeg.front().llNum;
        m_llReadSize = m_vecSeg.front().llSize;
        if (m_llReadOff > m_llReadSize)
        {
            m_llReadOff = m_llReadSize;
        }
        return;
    }

    m_llReadSegNum = -1;
    m_llReadOff = 0;
    m_llReadSize = 0;
    if (!bConsumeCurrent)
    {
        return;
    }
    dropOldestSegLocked();
}

void CReportSpool::reclaimTailLocked()
{
    if (m_vecSeg.empty())
    {
        return;
    }
    /* 就地截回读游标：不这么做的话，下一次 append 会接在坏字节后面，
       而读侧遇到坏记录只会跳段——段又只有一个，于是整条积压永远读不出来 */
    const std::string strPath = segPathLocked(m_vecSeg.front().llNum);
    if (0 != ::truncate(strPath.c_str(), m_llReadOff))
    {
        dlog_warn("classroom spool: 回收 %s 尾段到 %lld 失败: %s", strPath.c_str(),
                  static_cast<long long>(m_llReadOff), strerror(errno));
    }
    const int64_t llFreed = m_vecSeg.front().llSize - m_llReadOff;
    if (llFreed > 0)
    {
        m_llUsedBytes -= llFreed;
        m_stStat.u64DropBytes += static_cast<uint64_t>(llFreed);
    }
    m_vecSeg.front().llSize = m_llReadOff;
    m_llWriteBytes = m_llReadOff;
    m_llReadSize = m_llReadOff;
    m_llPendingRecBytes = 0;
}

void CReportSpool::reclaimDrainedLocked()
{
    if (1 != m_vecSeg.size() || m_llReadOff < m_vecSeg.back().llSize)
    {
        return;
    }

    const int64_t llFreed = m_vecSeg.back().llSize;
    const std::string strPath = segPathLocked(m_vecSeg.back().llNum);
    closeWriteSegLocked();
    if (0 != ::unlink(strPath.c_str()) && ENOENT != errno)
    {
        dlog_warn("classroom spool: 排空 unlink %s 失败: %s", strPath.c_str(), strerror(errno));
        ++m_stStat.u64IoFail;
        return;
    }
    if (m_fdRead >= 0)
    {
        ::close(m_fdRead);
        m_fdRead = -1;
    }
    m_vecSeg.pop_back();
    m_llUsedBytes -= llFreed;
    if (m_llUsedBytes < 0)
    {
        m_llUsedBytes = 0;
    }
    m_llWriteBytes = 0;
    m_llReadSegNum = -1;
    m_llReadOff = 0;
    m_llReadSize = 0;
    m_u32WriteRecIdx = 0;
}

bool CReportSpool::peek(CSpoolRecord_S &rstRec)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return false;
    }
    if (m_llPendingRecBytes > 0)
    {
        /* 上一条还没 commit 就再 peek：调用方（引擎补传）在等通道结果，这里重读同一条即可 */
        int64_t llDummy = 0;
        return (readRecordLocked(rstRec, llDummy) > 0);
    }

    for (int32_t i = 0; i < PEEK_LOOP_MAX; ++i)
    {
        if (!openReadSegLocked())
        {
            return false;
        }
        if (m_llReadOff >= m_llReadSize)
        {
            if (1 == m_vecSeg.size())
            {
                /* 唯一那段同时是写段：读完即"没有积压"。既不能回收（后面还要往里写），
                   也不能把游标归零（归零=把已 commit 的记录再补传一遍） */
                return false;
            }
            advanceToNextSegLocked(true);
            if (m_vecSeg.empty())
            {
                return false;
            }
            continue;
        }

        int64_t llRecBytes = 0;
        const int32_t nRet = readRecordLocked(rstRec, llRecBytes);
        if (nRet > 0)
        {
            m_llPendingRecBytes = llRecBytes;
            return true;
        }
        if (nRet < 0)
        {
            /* 只有校验/魔数不过才算"坏数据"，短尾不计：短尾是掉电的正常收尾 */
            ++m_stStat.u64Corrupt;
        }

        if (1 == m_vecSeg.size())
        {
            reclaimTailLocked();
            return false;
        }
        advanceToNextSegLocked(true);
    }

    ++m_u64PeekGiveup;
    dlog_warn("classroom spool: peek %d 个段后放弃（积压损坏?）", PEEK_LOOP_MAX);
    return false;
}

bool CReportSpool::commit()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    /* 停机窗口必须一口回绝 */
    if (!m_bReady)
    {
        return false;
    }
    if (m_llPendingRecBytes <= 0)
    {
        return false;
    }
    m_llReadOff += m_llPendingRecBytes;
    m_llPendingRecBytes = 0;
    m_stStat.u64Sent += 1;

    if (1 == m_vecSeg.size())
    {
        /* 只有一段且已读到写边界：整段回收（见 reclaimDrainedLocked 的"为什么必须归零"）*/
        reclaimDrainedLocked();
    }
    else if (m_llReadOff >= m_llReadSize)
    {
        /* 整段读完就地回收，不等下一次 peek：断网恢复后如果没人再 peek，
           空间就该已经还回来了（滞留环的价值在"还能再滞留"，不在"留着旧账"） */
        advanceToNextSegLocked(true);
    }

    /* 每一次推进都落盘 */
    writeCtlLocked();
    return true;
}

void CReportSpool::clear()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    /* 同 commit：停机不是"作废积压"的许可证。clear() 是唯一会 unlink 段文件公开口，
       少了这道守卫，"deinit（配置关掉，积压留在 flash 上等下次开机）"之后任何一次
       clear() 都会把那份积压真删掉，而 drop_bytes 记的是一台已经不在工作的环——
       重启后目录扫出来是空的，现场只会看到"滞留环从来没存过东西" */
    if (!m_bReady)
    {
        return;
    }
    closeWriteSegLocked();
    if (m_fdRead >= 0)
    {
        ::close(m_fdRead);
        m_fdRead = -1;
    }
    for (const auto &stSeg : m_vecSeg)
    {
        const std::string strPath = segPathLocked(stSeg.llNum);
        if (0 != ::unlink(strPath.c_str()) && ENOENT != errno)
        {
            dlog_warn("classroom spool: 清空 unlink %s 失败: %s", strPath.c_str(), strerror(errno));
        }
        m_stStat.u64DropBytes += static_cast<uint64_t>(stSeg.llSize);
    }
    m_stStat.u64DropSeg += m_vecSeg.size();
    m_vecSeg.clear();
    m_llUsedBytes = 0;
    m_llWriteBytes = 0;
    m_llReadSegNum = -1;
    m_llReadOff = 0;
    m_llReadSize = 0;
    m_llPendingRecBytes = 0;
    m_u32WriteRecIdx = 0;
    writeCtlLocked();   /* 把"游标已作废"这件事也落盘，否则重启后还会去夹一个指向已删段的旧值 */
    dlog_info("classroom spool: 积压已按配置清空");
}

int64_t CReportSpool::pendingBytes() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_llUsedBytes;
}

CSpoolStat_S CReportSpool::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    CSpoolStat_S stOut = m_stStat;
    stOut.nSegNum = static_cast<int32_t>(m_vecSeg.size());
    stOut.llUsedBytes = m_llUsedBytes;
    stOut.llMaxBytes = m_llMaxBytes;
    return stOut;
}

const char *CReportSpool::typeName(uint32_t u32Type)
{
    switch (u32Type)
    {
        case SPOOL_TYPE_REPORT:  return "report";
        case SPOOL_TYPE_SESSION: return "session";
        case SPOOL_TYPE_EVENT:   return "event";
        default:                 return "unknown";
    }
}

std::string CReportSpool::statLine() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    char szBuf[320] = {0};
    snprintf(szBuf, sizeof(szBuf),
             "rdy=%d segs=%d used=%lld/%lldKB ap=%llu fail=%llu sent=%llu drop=%llu/%lluKB "
             "crr=%llu torn=%llu io=%llu ctl=%llu/%llu",
             m_bReady ? 1 : 0, static_cast<int32_t>(m_vecSeg.size()),
             static_cast<long long>(m_llUsedBytes / 1024), static_cast<long long>(m_llMaxBytes / 1024),
             static_cast<unsigned long long>(m_stStat.u64Append),
             static_cast<unsigned long long>(m_stStat.u64AppendFail),
             static_cast<unsigned long long>(m_stStat.u64Sent),
             static_cast<unsigned long long>(m_stStat.u64DropSeg),
             static_cast<unsigned long long>(m_stStat.u64DropBytes / 1024),
             static_cast<unsigned long long>(m_stStat.u64Corrupt),
             static_cast<unsigned long long>(m_stStat.u64TornTail),
             static_cast<unsigned long long>(m_stStat.u64IoFail),
             static_cast<unsigned long long>(m_stStat.u64CtlWrite),
             static_cast<unsigned long long>(m_stStat.u64CtlBad));
    return std::string(szBuf);
}

std::string CReportSpool::statJson() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    char szBuf[640] = {0};
    snprintf(szBuf, sizeof(szBuf),
             "{\"ready\":%d,\"segs\":%d,\"used_kb\":%lld,\"max_kb\":%lld,\"append\":%llu,"
             "\"append_fail\":%llu,\"sent\":%llu,\"drop_seg\":%llu,\"drop_kb\":%llu,"
             "\"drop_rec\":%llu,\"corrupt\":%llu,\"torn_bytes\":%llu,\"io_fail\":%llu,"
             "\"ctl_write\":%llu,\"ctl_bad\":%llu}",
             m_bReady ? 1 : 0, static_cast<int32_t>(m_vecSeg.size()),
             static_cast<long long>(m_llUsedBytes / 1024), static_cast<long long>(m_llMaxBytes / 1024),
             static_cast<unsigned long long>(m_stStat.u64Append),
             static_cast<unsigned long long>(m_stStat.u64AppendFail),
             static_cast<unsigned long long>(m_stStat.u64Sent),
             static_cast<unsigned long long>(m_stStat.u64DropSeg),
             static_cast<unsigned long long>(m_stStat.u64DropBytes / 1024),
             static_cast<unsigned long long>(m_stStat.u64DropRec),
             static_cast<unsigned long long>(m_stStat.u64Corrupt),
             static_cast<unsigned long long>(m_stStat.u64TornTail),
             static_cast<unsigned long long>(m_stStat.u64IoFail),
             static_cast<unsigned long long>(m_stStat.u64CtlWrite),
             static_cast<unsigned long long>(m_stStat.u64CtlBad));
    return std::string(szBuf);
}

int32_t spoolDrainBeat(CReportSpool &rstSpool, const SpoolUploadFn &fnUpload,
                       int64_t llBudgetKb, CSpoolDrainStat_S &rstStat)
{
    rstStat = CSpoolDrainStat_S();
    if (!rstSpool.isReady() || !fnUpload || 0 >= rstSpool.pendingBytes())
    {
        return 0;
    }

    /* peek 的"放弃"计数在进入循环前取一次基线 */
    const uint64_t ullGiveupBase = rstSpool.peekGiveupNum();

    while (rstStat.llSentKb < llBudgetKb)
    {
        CSpoolRecord_S stRec;
        if (!rstSpool.peek(stRec))
        {
            break;                        /* 积压已清空 */
        }

        bool bSent = false;
        try
        {
            bSent = fnUpload(stRec.u32Type, stRec.strPayload);
        }
        /* 异常边界：调用方是引擎节拍线程，漏一个异常出去 =
           "帧还在进、积压永不落"的静默僵死。catch 后与"通道说没送达"同一处理：
           停拍、游标不动，这条记录仍在环里 */
        catch (const std::exception &stEx)
        {
            ++rstStat.nUploadThrow;
            rstStat.strThrowWhat = stEx.what();
            break;
        }
        catch (...)
        {
            ++rstStat.nUploadThrow;
            rstStat.strThrowWhat = "unknown exception";
            break;
        }

        if (!bSent)
        {
            /* 一失败就停，且不落回环（它本来就在环里，游标没动）。
               继续往下走就会把"网络又不通了"变成"跳过一条重发后面的"——顺序没了 */
            ++rstStat.nUploadError;
            break;
        }
        if (!rstSpool.commit())
        {
            rstStat.bCommitStuck = true;
            rstStat.u64StuckSeq = stRec.u64Seq;
            break;    /* 游标没推进＝下一条还会是它，硬继续只是原地打转 */
        }

        ++rstStat.nSent;
        /* 头也记账：预算管的是"这一拍读了多少 flash"，不是"发了多少正文" */
        rstStat.llSentKb += (static_cast<int64_t>(stRec.strPayload.size()) +
                            REC_HDR_SIZE + 1023) / 1024;
    }

    rstStat.nPeekGiveup = static_cast<int32_t>(rstSpool.peekGiveupNum() - ullGiveupBase);
    rstStat.u64PeekGiveupTotal = rstSpool.peekGiveupNum();
    return rstStat.nSent;
}

}   /* namespace Classroom */
