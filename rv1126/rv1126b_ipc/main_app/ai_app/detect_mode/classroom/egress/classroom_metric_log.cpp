/**
 * @FilePath     : classroom_metric_log.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-21 16:18:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 16:48:00
 * @Description  : 心跳落点的实现。这里只有三件事：装配、落一行、把自己的状态说清楚
 */

#include "classroom_metric_log.h"

#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <exception>

#include "dlog.h"

namespace Classroom
{
namespace
{
/* dlog 的这几个入口收 char*（C 接口，不改公共代码就照它的意思办）。
   异常必须在这里接住：spdlog 打不开文件（目录不存在、只读、flash 满）是从 sink 构造函数
   抛 spdlog_ex，不是返回码——不接住，一次日志路径写错会让整机算法起不来 */
int initLevelLogSafe(const char *pName, const char *pFile, int nMaxBytes, int nMaxFiles,
                     char *pszErr, size_t uErrCap)
{
    pszErr[0] = '\0';
    try
    {
        return initLogLevelBySize(const_cast<char *>(pName), const_cast<char *>(pFile),
                                  LOG_USER, nMaxBytes, nMaxFiles);
    }
    catch (const std::exception &stEx)
    {
        snprintf(pszErr, uErrCap, "%s", stEx.what());
        return -1;
    }
    catch (...)
    {
        snprintf(pszErr, uErrCap, "unknown");
        return -1;
    }
}

/* 落点文件的状态：三态而不是 bool。"配置就不要"与"想要但没建成"在两处都要能分开读，
   前者是运维的选择、后者是故障，混成一个读数就会让人去查一个不存在的故障 */
const char *destName(bool bWantOwn, bool bOwn)
{
    if (!bWantOwn)
    {
        return "business_cfg";
    }
    return bOwn ? "own" : "business_initfail";
}

/* ---- 本进程内这只 sink 归谁 ---- */
bool g_bSinkTaken = false;        /* 名字已经落在 spdlog registry 里：收不回来，也不能再注册一次 */
bool g_bSinkUsable = false;       /* 而且它建出来了一个我读得回来的文件 */
CMetricLogParam_S g_stSinkGeo;    /* 建成时用的那套几何，仅 g_bSinkTaken 为真时有意义 */

bool sameGeom(const CMetricLogParam_S &stA, const CMetricLogParam_S &stB)
{
    return stA.strFile == stB.strFile && stA.nMaxKb == stB.nMaxKb && stA.nMaxFiles == stB.nMaxFiles;
}

bool sameParam(const CMetricLogParam_S &stA, const CMetricLogParam_S &stB)
{
    return stA.bOwnFile == stB.bOwnFile && stA.nHeadPeriodMs == stB.nHeadPeriodMs && sameGeom(stA, stB);
}
}

const char *CClassroomMetricLog::configure(const CMetricLogParam_S &stParam)
{
    /* 同一次装配可以被反复调：引擎每收到一份配置都会调它，绝大多数次参数一模一样，
       什么都不做直接返回 */
    if (m_bAsked && sameParam(stParam, m_stAsked))
    {
        return m_strHow.c_str();
    }

    m_stAsked = stParam;
    m_bAsked = true;
    m_bWantOwn = stParam.bOwnFile;
    /* 回抄周期与"要不要走自己的文件"都是纯运行期的判定，不碰任何句柄，当场生效。
       几何也先按**这次要求的**记下，但只到这里为止：sink 已建成时下面会用"实际在用的那份"
       覆盖回来。先记的原因是 m_stParam.strFile 不只是拿来打印——sinkAlive() 查的就是它，
       装配失败的那几条 return 如果跳过赋值，这个字段就还是编译期默认值，于是"要求过什么"
       和"在用什么"两边都不是，排查时比错一个数更误导 */
    m_stParam.bOwnFile = stParam.bOwnFile;
    m_stParam.nHeadPeriodMs = stParam.nHeadPeriodMs;
    m_stParam.strFile = stParam.strFile;
    m_stParam.nMaxKb = stParam.nMaxKb;
    m_stParam.nMaxFiles = stParam.nMaxFiles;

    if (g_bSinkTaken)
    {
        if (!g_bSinkUsable)
        {
            /* 名字占了、文件没成。这里**不能**再试一次 init：第二次必然抛在重名上，
               而抛的那一下已经把新路径的文件建出来了（sink 构造在注册之前），
               等于每试一次就多留一个空文件。如实说，交给重启 */
            m_bOwn = false;
            m_strHow = "mlog=business_initfail(sink_taken_unusable)";
            return m_strHow.c_str();
        }

        /* 复用：可能是本轮自己先前建的，也可能是上一个引擎实例建的。
           两种情况在这里没有区别，几何都以已生效的那份为准——keep= 是按它算的 */
        m_stParam.strFile = g_stSinkGeo.strFile;
        m_stParam.nMaxKb = g_stSinkGeo.nMaxKb;
        m_stParam.nMaxFiles = g_stSinkGeo.nMaxFiles;
        m_bGeoStale = !sameGeom(stParam, g_stSinkGeo);
        if (m_bGeoStale)
        {
            char szWant[128];
            snprintf(szWant, sizeof(szWant), "%s cap=%dKBx%d",
                     stParam.strFile.c_str(), stParam.nMaxKb, stParam.nMaxFiles);
            dlog_warn("classroom metric log: 落点建好后几何即固定，"
                      "仍在写 %s cap=%dKBx%d（重启后生效 %s）",
                      m_stParam.strFile.c_str(), m_stParam.nMaxKb, m_stParam.nMaxFiles, szWant);
        }
        m_bOwn = m_bWantOwn;
    }
    else
    {
        if (!m_bWantOwn)
        {
            /* 一次都还没建过又不要求自己的文件：连 init 都不做。
               注册一个没人写的 logger 等于白占一个文件句柄和 registry 里一个名字，
               而那名字后面再也拿不回来（见上面 g_bSinkTaken 那段） */
            m_strHow = "mlog=business_cfg(by_config)";
            return m_strHow.c_str();
        }

        char szErr[128];
        const int nRc = initLevelLogSafe(CLASSROOM_METRIC_LOGGER_NAME, stParam.strFile.c_str(),
                                         stParam.nMaxKb * 1024, stParam.nMaxFiles,
                                         szErr, sizeof(szErr));
        if (0 != nRc)
        {
            m_strHow = std::string("mlog=business_initfail(") + (szErr[0] ? szErr : "rc") + ")";
            /* 抛在 sink 构造那一步，名字还没进 registry，所以这次失败**可以**重试：
               m_bAsked 清回去（与 spoolApply 同一条纪律），目录往往是后来才挂上的 */
            m_bAsked = false;
            m_bOwn = false;
            return m_strHow.c_str();
        }

        /* init 返回 0 的那一刻名字就是它的了，接下来无论发生什么都不能再试第二次 */
        g_bSinkTaken = true;
        g_stSinkGeo = stParam;

        /* 建完先确认文件真在那儿。返回码只说明"没抛异常"，而这里要的是
           "这一路上有一个我读得回来的文件"。只查存在不查 size：异步 logger 此刻
           还在缓冲里，size 一定偏小，拿它判读会误判 */
        struct stat stInfo;
        g_bSinkUsable = (0 == ::stat(stParam.strFile.c_str(), &stInfo) && S_ISREG(stInfo.st_mode));
        if (!g_bSinkUsable)
        {
            m_bOwn = false;
            m_strHow = "mlog=business_initfail(no_file_after_init)";
            return m_strHow.c_str();
        }
        m_bOwn = true;
    }

    const int64_t llRingKb = static_cast<int64_t>(m_stParam.nMaxKb) * (m_stParam.nMaxFiles + 1);
    char szHow[224];
    snprintf(szHow, sizeof(szHow), "mlog=%s file=%s cap=%dKBx%d+1 ring=%lldKB est_keep=%.0fh%s",
             destName(m_bWantOwn, m_bOwn), m_stParam.strFile.c_str(),
             m_stParam.nMaxKb, m_stParam.nMaxFiles,
             static_cast<long long>(llRingKb),
             static_cast<double>(llRingKb) / CLASSROOM_METRIC_EST_KB_PER_H,
             m_bGeoStale ? " geo_stale=1(reboot to apply)" : "");
    m_strHow = szHow;
    return m_strHow.c_str();
}

void CClassroomMetricLog::write(const std::string &strLine, const std::string &strHead, int64_t llNowMs)
{
    const bool bOwn = m_bOwn && sinkAlive();
    if (m_bWantOwn && !bOwn)
    {
        ++m_u64Fallback;
        if (llNowMs - m_llLastWarnMs >= 60000)
        {
            /* 一条 warn 说清"为什么完整心跳又回到业务日志里了"，并且限速：
               落点一旦坏掉是每拍坏，不限速就是每 10s 一条 warn 把刚省下来的份额还回去 */
            m_llLastWarnMs = llNowMs;
            dlog_warn("classroom metric log: %s (fb=%llu)，完整行改走业务日志，"
                      "回落期间 head 行被抑制",
                      m_strHow.c_str(), static_cast<unsigned long long>(m_u64Fallback));
        }
    }

    if (bOwn)
    {
        dlog_user("%s", strLine.c_str());
    }
    else
    {
        dlog_info("%s", strLine.c_str());
    }

    if (0 == m_llFirstMs)
    {
        m_llFirstMs = llNowMs;
        m_llLastHeadMs = llNowMs - m_stParam.nHeadPeriodMs;   /* 第一拍就出一条 head，不等 */
    }
    ++m_u64Lines;
    m_u64Bytes += strLine.size();

    /* 回落时不回抄 head：整行已经在同一只环里了，再抄一条只是让那条心跳占两份 */
    if (bOwn && m_stParam.nHeadPeriodMs > 0 && llNowMs - m_llLastHeadMs >= m_stParam.nHeadPeriodMs)
    {
        m_llLastHeadMs = llNowMs;
        ++m_u64Head;
        dlog_info("%s", strHead.c_str());
    }
}

std::string CClassroomMetricLog::statLine(int64_t llNowMs) const
{
    const int64_t llRingKb = static_cast<int64_t>(m_stParam.nMaxKb) * (m_stParam.nMaxFiles + 1);
    const int64_t llSec = (m_llFirstMs > 0) ? (llNowMs - m_llFirstMs) / 1000 : 0;
    const int64_t llKb = static_cast<int64_t>(m_u64Bytes / 1024);

    char szBuf[192];
    /* 这一段里每个键名都要对着**心跳行已有的键表**挑过再落笔 */
    if (m_bOwn && llKb > 0 && llSec >= 600)
    {
        const double ldKbPerH = static_cast<double>(llKb) * 3600.0 / static_cast<double>(llSec);
        snprintf(szBuf, sizeof(szBuf),
                 "mlog=%s lines=%llu kb=%llu rate=%.0fKB/h keep=%.1fh fb=%llu head=%llu geo_stale=%d",
                 destName(m_bWantOwn, m_bOwn), static_cast<unsigned long long>(m_u64Lines),
                 static_cast<unsigned long long>(llKb), ldKbPerH,
                 (ldKbPerH > 0.0) ? static_cast<double>(llRingKb) / ldKbPerH : 0.0,
                 static_cast<unsigned long long>(m_u64Fallback),
                 static_cast<unsigned long long>(m_u64Head), m_bGeoStale ? 1 : 0);
    }
    else
    {
        snprintf(szBuf, sizeof(szBuf),
                 "mlog=%s lines=%llu kb=%llu keep=warming fb=%llu head=%llu geo_stale=%d",
                 destName(m_bWantOwn, m_bOwn), static_cast<unsigned long long>(m_u64Lines),
                 static_cast<unsigned long long>(llKb),
                 static_cast<unsigned long long>(m_u64Fallback),
                 static_cast<unsigned long long>(m_u64Head), m_bGeoStale ? 1 : 0);
    }
    return std::string(szBuf);
}

bool CClassroomMetricLog::sinkAlive() const
{
    struct stat stInfo;
    return (0 == ::stat(m_stParam.strFile.c_str(), &stInfo)) && (0 != S_ISREG(stInfo.st_mode));
}

}
