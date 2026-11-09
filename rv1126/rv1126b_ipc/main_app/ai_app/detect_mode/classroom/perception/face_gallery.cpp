/**
 * @FilePath     : face_gallery.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-21 14:16:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 14:16:00
 * @Description  : 人脸底库：纯比对内核 + 文件底库
 */

#include "face_gallery.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sys/stat.h>

#include "dlog.h"

namespace Classroom
{
namespace
{
/* 姓名字段字节上限。超过它基本可以断定这行格式坏了或字段错位（特征串漏了分隔符挤进姓名），
   而不是"真有个 32 个汉字的姓名"——后者在名单里不存在，而前者会静默吃掉一次比对 */
const size_t GALLERY_NAME_MAX_BYTES = 96;

/* 统计节流：底库文件迟迟不到时，每轮都打一条 warn 会把日志刷满，
   而"文件不存在"恰恰是最需要被看见的一条——所以打第一次与之后每 20 次 */
const uint32_t GALLERY_LOG_EVERY = 20;

const char CHR_FIELD_SEP = '|';
const char CHR_FEAT_SEP  = ',';
const char CHR_COMMENT   = '#';

bool isBlankCh(char ch)
{
    return (' ' == ch || '\t' == ch || '\r' == ch || '\n' == ch);
}

std::string trimField(const std::string &strIn)
{
    size_t nBeg = 0;
    size_t nEnd = strIn.size();
    while (nBeg < nEnd && isBlankCh(strIn[nBeg]))
    {
        ++nBeg;
    }
    while (nEnd > nBeg && isBlankCh(strIn[nEnd - 1]))
    {
        --nEnd;
    }
    return strIn.substr(nBeg, nEnd - nBeg);
}

/**
 * @brief   : 解析 "f0,f1,f2" 形式的特征串
 * @param    {std::vector<float>&} rvec：OUT=解析出的浮点，调用方复用同一块容量
 * @return   {bool} false=有一个字符解析不出、含非有限值、或有未被消费的残串
 */
bool parseFeatList(const std::string &strIn, std::vector<float> &rvec)
{
    rvec.clear();

    size_t nPos = 0;
    while (nPos < strIn.size())
    {
        const char *pchCur = strIn.c_str() + nPos;
        char *pchEnd = nullptr;
        const float fVal = strtof(pchCur, &pchEnd);
        if (pchEnd == pchCur)
        {
            return false;                       /* 一个字符都没吃掉：这里根本不是数 */
        }
        if (!std::isfinite(fVal))
        {
            return false;
        }
        rvec.push_back(fVal);

        nPos = static_cast<size_t>(pchEnd - strIn.c_str());
        while (nPos < strIn.size() && isBlankCh(strIn[nPos]))
        {
            ++nPos;
        }
        if (nPos >= strIn.size())
        {
            break;
        }
        if (CHR_FEAT_SEP != strIn[nPos])
        {
            return false;                       /* 数字后面既不是分隔符也不是空白 = 残串 */
        }
        ++nPos;                                 /* 吃掉逗号；尾随逗号由下一轮循环的边界判断放过 */
    }

    return !rvec.empty();
}

/* 去重计数：同一 personId 的多张照片算一个人。n ≤ 上限（256）行，O(n²) 的字符串比较
   只在重载时跑一次，不值得为它引一个 unordered_set（那是一份运行期哈希表分配） */
int32_t countDistinctId(const std::vector<std::string> &vecId)
{
    int32_t nNum = 0;
    for (size_t nRow = 0; nRow < vecId.size(); ++nRow)
    {
        bool bSeen = false;
        for (size_t nPrev = 0; nPrev < nRow; ++nPrev)
        {
            if (vecId[nPrev] == vecId[nRow])
            {
                bSeen = true;
                break;
            }
        }
        if (!bSeen)
        {
            ++nNum;
        }
    }
    return nNum;
}
}    // namespace

void CGalleryWatch::configure(int32_t nReloadSec)
{
    const int32_t nSec = (nReloadSec > 0) ? nReloadSec : 0;
    if (nSec != m_nReloadSec)
    {
        reset();
    }
    m_nReloadSec = nSec;
}

void CGalleryWatch::reset()
{
    m_llLastPollMs = 0;
    m_llMTimeMs = 0;
    m_llSizeByte = -1;
    m_llPendMTimeMs = 0;
    m_llPendSizeByte = -1;
}

bool CGalleryWatch::due(const char *pchPath, int64_t llNowMs)
{
    if (nullptr == pchPath || '\0' == pchPath[0] || llNowMs <= 0)
    {
        return false;
    }

    /* m_llLastPollMs==0 = 从没判过 → 首次必读。间隔 0 = 只读这一次（配置成"启动时加载"的用法） */
    const int64_t llIntervalMs = static_cast<int64_t>(m_nReloadSec) * 1000;
    if (0 != m_llLastPollMs &&
        (0 == llIntervalMs || (llNowMs - m_llLastPollMs) < llIntervalMs))
    {
        return false;
    }
    m_llLastPollMs = llNowMs;

    struct stat stFile;
    if (0 != ::stat(pchPath, &stFile) || !S_ISREG(stFile.st_mode))
    {
        ++m_u64StatFail;
        if (1 == m_u64StatFail || 0 == (m_u64StatFail % GALLERY_LOG_EVERY))
        {
            dlog_warn("classroom gallery: %s 不可读 (fails=%llu)",
                      pchPath, static_cast<unsigned long long>(m_u64StatFail));
        }
        return false;
    }

    /* 粒度只到秒：POSIX 的 st_mtime 就是秒，取纳秒要为它引一层特性宏，而这里的判据是
       "平台或运维改过这个文件没有"，秒足够；同秒内的二次修改由 size 兜住一半。
       乘 1000 只是把口径统一到毫秒成员上（少这一步就是"每次都算变了"） */
    const int64_t llMTimeMs = static_cast<int64_t>(stFile.st_mtime) * 1000;
    const int64_t llSizeByte = static_cast<int64_t>(stFile.st_size);
    m_llPendMTimeMs = llMTimeMs;
    m_llPendSizeByte = llSizeByte;

    /* 只看 mtime 会漏"同一秒内原子替换且长度相同"，只看 size 会漏"改姓名不改长度"，
       所以两个量都判（头文件那条口径就在这里落地） */
    return !(llMTimeMs == m_llMTimeMs && llSizeByte == m_llSizeByte);
}

void CGalleryWatch::commit()
{
    m_llMTimeMs = m_llPendMTimeMs;
    m_llSizeByte = m_llPendSizeByte;
}

bool CFaceGalleryStore::add(const std::string &strPersonId, const std::string &strName,
                           const float *pafFeat, int32_t nDim, int32_t nMaxEnroll,
                           CGalleryStat_S &rstStat)
{
    if (nullptr == pafFeat || nDim <= 0 || strPersonId.empty())
    {
        ++rstStat.u64RejectBad;
        return false;
    }

    const int32_t nCap = (nMaxEnroll > 0) ? nMaxEnroll : CLASSROOM_GALLERY_MAX_ENROLL;
    if (rowNum() >= nCap)
    {
        ++rstStat.u64RejectFull;
        return false;
    }

    /* 维度由**本批多数行**定死（调用方扫一遍后经 beginLoad(nPreferredDim) 传入；
       没指定时退回"第一行"）。之后要么同维要么被拒*/
    if (0 == m_nDim)
    {
        m_nDim = (m_nPreferredDim > 0) ? m_nPreferredDim : nDim;
    }
    if (nDim != m_nDim)
    {
        ++rstStat.u64RejectDim;
        rstStat.nDimSeen = nDim;
        return false;
    }

    double dSumSq = 0.0;
    for (int32_t i = 0; i < nDim; ++i)
    {
        if (!std::isfinite(pafFeat[i]))
        {
            ++rstStat.u64RejectBad;
            return false;
        }
        dSumSq += static_cast<double>(pafFeat[i]) * static_cast<double>(pafFeat[i]);
    }

    /* 双精度累加后降回单精度 */
    const float fNorm = static_cast<float>(std::sqrt(dSumSq));
    if (!(fNorm > 1e-6f))
    {
        /* 判据写成 !(>) 而不是 (<)：NaN 模长也要落到这一档（上面已挡 NaN 元素，
           这里是挡"全是次正规小数"导致平方下溢成一堆 0 的行） */
        ++rstStat.u64RejectZero;
        return false;
    }

    m_vecId.push_back(strPersonId);
    m_vecName.push_back(strName);
    for (int32_t i = 0; i < nDim; ++i)
    {
        m_vecFeat.push_back(pafFeat[i] / fNorm);
    }

    /* 表变了，去重人数作废：宁可读到 -1（"还不知道"）也不读一个旧数字。
       这条纪律的实际作用是——加载中途若有人问应到人数，兜底路径会去用课表而不是这个半成品 */
    m_bFinished = false;
    m_nEnrollNum = -1;
    return true;
}

void CFaceGalleryStore::beginLoad(int32_t nPreferredDim)
{
    m_vecId.clear();
    m_vecName.clear();
    m_vecFeat.clear();
    m_nDim = 0;
    m_nPreferredDim = (nPreferredDim > 0) ? nPreferredDim : 0;
    m_nEnrollNum = -1;
    m_bFinished = false;
}

void CFaceGalleryStore::finish()
{
    m_nEnrollNum = countDistinctId(m_vecId);
    m_bFinished = true;
}

bool CFaceGalleryStore::bestMatch(const float *pafProbe, int32_t nDim, CFaceMatch_S &rstOut) const
{
    rstOut = CFaceMatch_S();

    if (nullptr == pafProbe || nDim <= 0 || nDim != m_nDim)
    {
        return false;
    }

    const int32_t nRow = rowNum();
    if (0 == nRow || m_vecFeat.size() != static_cast<size_t>(nRow) * static_cast<size_t>(nDim))
    {
        /* 第二条是内部不变量：只在 add/finish 被跳过时成立，例如调用方 beginLoad 后中途抛错。
           真触发就是代码 bug，静默返回"没比上"比让它出现在读数上更难查，故打一条 error */
        dlog_error("classroom gallery: store 不一致, rows=%d dim=%d feat=%u",
                   nRow, m_nDim, static_cast<unsigned int>(m_vecFeat.size()));
        return false;
    }

    double dProbeSumSq = 0.0;
    for (int32_t i = 0; i < nDim; ++i)
    {
        if (!std::isfinite(pafProbe[i]))
        {
            return false;
        }
        dProbeSumSq += static_cast<double>(pafProbe[i]) * static_cast<double>(pafProbe[i]);
    }
    const float fProbeNorm = static_cast<float>(std::sqrt(dProbeSumSq));
    if (!(fProbeNorm > 1e-6f))
    {
        return false;
    }

    int32_t nBest = -1;
    float fBest = -2.0f;                        /* 余弦下界 -1，-2 = 还没比过任何一行 */
    for (int32_t nRowIdx = 0; nRowIdx < nRow; ++nRowIdx)
    {
        const float *pafTpl = &m_vecFeat[static_cast<size_t>(nRowIdx) * static_cast<size_t>(nDim)];
        double dDot = 0.0;
        for (int32_t i = 0; i < nDim; ++i)
        {
            dDot += static_cast<double>(pafTpl[i]) * static_cast<double>(pafProbe[i]);
        }
        const float fScore = static_cast<float>(dDot / fProbeNorm);
        /* 严格大于：并列时保留靠前的行。不写清这一条，将来有人把 == 也算进去，
           同一个人的两张模板就会谁被命名取决于装载顺序 */
        if (fScore > fBest)
        {
            fBest = fScore;
            nBest = nRowIdx;
        }
    }

    if (nBest < 0)
    {
        return false;
    }
    rstOut.bHit = true;
    rstOut.strPersonId = m_vecId[static_cast<size_t>(nBest)];
    rstOut.strName = m_vecName[static_cast<size_t>(nBest)];
    rstOut.fScore = fBest;
    return true;
}

void CFileFaceGallery::setSource(const std::string &strPath, int32_t nReloadSec, int32_t nMaxEnroll)
{
    const int32_t nCap = (nMaxEnroll > 0) ? nMaxEnroll : CLASSROOM_GALLERY_MAX_ENROLL;
    if (strPath == m_strPath && m_stWatch.intervalSec() == ((nReloadSec > 0) ? nReloadSec : 0) &&
        nCap == m_nMaxEnroll)
    {
        return;
    }

    dlog_info("classroom gallery: 来源 %s -> %s, reload=%ds cap=%d rows=%d",
              m_strPath.empty() ? "(none)" : m_strPath.c_str(),
              strPath.empty() ? "(none)" : strPath.c_str(),
              nReloadSec, nCap, m_stStore.rowNum());

    m_strPath = strPath;
    m_nMaxEnroll = nCap;
    m_stWatch.configure(nReloadSec);

    /* 旧库一并作废：见头文件 setSource 的 @note。基准清零由 configure 在间隔变化时做，
       但"同间隔换路径"也必须清零，所以这里显式再配一次 */
    m_stWatch.reset();
    m_stStore = CFaceGalleryStore();
    m_nLastRows = 0;
}

bool CFileFaceGallery::poll(int64_t llNowMs)
{
    if (m_strPath.empty())
    {
        return false;
    }
    if (!m_stWatch.due(m_strPath.c_str(), llNowMs))
    {
        return false;
    }
    if (!reload())
    {
        ++m_stStat.u64ReloadFail;
        return false;
    }

    /* 重载成功才确认基准：坏文件下一轮还会重试 */
    m_stWatch.commit();
    ++m_stStat.u64Reload;
    dlog_info("classroom gallery: %s 已重载, rows=%d persons=%d dim=%d cap=%d "
              "reject(bad=%llu dim=%llu zero=%llu full=%llu)",
              m_strPath.c_str(), m_nLastRows, m_stStore.enrollNum(), m_stStore.dim(), m_nMaxEnroll,
              static_cast<unsigned long long>(m_stStat.u64RejectBad),
              static_cast<unsigned long long>(m_stStat.u64RejectDim),
              static_cast<unsigned long long>(m_stStat.u64RejectZero),
              static_cast<unsigned long long>(m_stStat.u64RejectFull));
    return true;
}

bool CFileFaceGallery::reload()
{
    std::ifstream ifs(m_strPath.c_str());
    if (!ifs.is_open())
    {
        return false;
    }

    /* 先填一张临时表、成功才换：整批替换中途出问题时底库保持上一次的内容。
       计数同理先累在局部，最后无论成败都并回成员——被拒的行是诊断信息，重载失败更要看得见 */
    CFaceGalleryStore stTmp;
    CGalleryStat_S stAcc = m_stStat;

    /* 行缓冲定长：单行占的内存由这个数决定，而不是由文件决定 */
    std::vector<char> vecLine(CLASSROOM_GALLERY_LINE_MAX, '\0');
    std::string strLine;
    std::string strId;
    std::string strName;
    std::vector<float> vecFeat;                 /* 逐行复用容量：一行 512 float，避免每行一次分配 */
    int32_t nCandidate = 0;                     /* 形如数据行的条数（含 '|' 且非注释） */
    int32_t nAccepted = 0;

    /* 读一行到 strLine。**两趟共用这一处**：超长行/空行/文件尾的语义只写一遍，
       否则维度普查那趟与入库那趟迟早分叉。
       bCount=false 的普查趟不记 reject：同一行会在入库趟被记一次，重复计会让读数翻倍 */
    auto readLine = [&](bool bCount) -> bool
    {
        for (;;)
        {
            ifs.getline(vecLine.data(), static_cast<std::streamsize>(vecLine.size()));
            const std::streamsize nGot = ifs.gcount();

            /* 判"超长"必须看 fail()，不能只看 gcount == size-1。
               两种情形都给出 gcount == 8191：① 这一行真的比缓冲长 ⇒ getline 塞满 8191 个字符
               还没找到分隔符，**failbit 置位**；② 这一行正好 8191 字节 + 分隔符 ⇒ getline 读到
               8191 个字符后**吃掉了分隔符，failbit 不置位**。旧判据只比 gcount，把 ② 也当成坏行
               静默丢掉——名单库里一行 ≈ personId|名字|512 维 base64（约 2.7KB），
               名字特别长（或将来维度上调）时正好撞上 8191 的那一行就凭空消失，
               而名单少一个人是"到课人数对不上"这类客诉的第一来源。
               fail() 只在 ① 置位，正好把两种情形分开。 */
            if (ifs.fail() && !ifs.eof())
            {
                /* 读满 = 这一行比上限长。必须把本行剩下的部分丢掉再续：不丢的话下一轮读到的
                   是这一行的尾巴，一个字段残缺的"假行"可能被当成一条真记录加进库——
                   那比丢一行糟得多 */
                if (bCount)
                {
                    ++stAcc.u64RejectBad;
                }
                ifs.clear();
                ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (ifs.eof())
                {
                    return false;
                }
                continue;
            }
            (void)nGot;
            if (nGot <= 0)
            {
                /* 一个字符都没提取 = 一上来就到文件尾（中间的空行 gcount 是 1，只提取到分隔符，
                   落到下面是一个空的 strLine，被"不含字段分隔符"那条规则跳过） */
                if (!ifs.eof())
                {
                    ifs.clear();
                    continue;
                }
                return false;
            }

            /* 长度按**缓冲区里的终止符**取，不按 gcount()：getline() 在写完正文后还会补一个 '\0'，
               而 gcount() 数的是"提取到的字符数"——行分隔符被提取但**不写进缓冲区**，
               于是 gcount() 恰好比正文字节数多 1，那个多出来的位置读的正是刚补上的 '\0'。
               拿它当长度 assign，每一行末尾就带一个隐形的 NUL：最后一个字段（特征串）
               因此被 parseFeatList 判成"数字后面有残串"，一行都认不进来。
               真机现象是"名单明明传上去了，rows=0"，而日志只说 none parsed——
               这条不是理论问题，是主机用例第一次真读文件就撞上的 */
            strLine.assign(vecLine.data());
            return true;
        }
    };

    /* 解析 strLine 到 strId/strName/vecFeat：1=数据行，0=注释/非数据行（不计候选），-1=坏行 */
    auto parseLine = [&]() -> int
    {
        const size_t nFirst = strLine.find(CHR_FIELD_SEP);
        if (std::string::npos == nFirst || CHR_COMMENT == strLine[0])
        {
            return 0;                           /* 注释与不含分隔符的行不计入候选，不算错误 */
        }
        const size_t nSecond = strLine.find(CHR_FIELD_SEP, nFirst + 1);
        if (std::string::npos == nSecond)
        {
            return -1;
        }
        strId = trimField(strLine.substr(0, nFirst));
        strName = trimField(strLine.substr(nFirst + 1, nSecond - nFirst - 1));
        if (strId.empty() || strName.size() > GALLERY_NAME_MAX_BYTES)
        {
            return -1;
        }
        if (!parseFeatList(trimField(strLine.substr(nSecond + 1)), vecFeat))
        {
            return -1;
        }
        return 1;
    };

    /* 第一趟：维度普查，取**本批多数行**的维度当锚点（并列时取先出现者）。
       只靠"第一行定死"时，一条半行粘贴（比如 3 个浮点）会先占住维度，之后几百条真行
       全被 dim 拒；而"认上了 1 行"又不够触发下面"整份失败、保留旧库"那条，
       最终留下一份只有 1 个人的库——比"没有底库"更难查（rows 非 0，日志也不报错） */
    typedef struct _DimCnt_ { int32_t nDim; int32_t nNum; } CDimCnt_S;
    std::vector<CDimCnt_S> vecDimCnt;
    while (readLine(false))
    {
        if (1 != parseLine())
        {
            continue;
        }
        const int32_t nDim = static_cast<int32_t>(vecFeat.size());
        bool bFound = false;
        for (CDimCnt_S &rstCnt : vecDimCnt)
        {
            if (rstCnt.nDim == nDim)
            {
                ++rstCnt.nNum;
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            CDimCnt_S stCnt;
            stCnt.nDim = nDim;
            stCnt.nNum = 1;
            vecDimCnt.push_back(stCnt);
        }
    }
    int32_t nPreferredDim = 0;
    int32_t nBestNum = 0;
    for (const CDimCnt_S &rstCnt : vecDimCnt)
    {
        /* 严格大于：并列时保留先出现的那一档（与旧口径"第一行定死"在并列输入下同解） */
        if (rstCnt.nNum > nBestNum)
        {
            nBestNum = rstCnt.nNum;
            nPreferredDim = rstCnt.nDim;
        }
    }
    if (!vecDimCnt.empty() && nPreferredDim != vecDimCnt[0].nDim)
    {
        /* 只在"锚点确实被改掉"时打一条：正常名单这一行不出现，省得每轮重载刷日志 */
        dlog_warn("classroom gallery: %s 首行维度 %d 不是本批多数（多数=%d），"
                  "按多数行定维度，首行记一次 dim 拒",
                  m_strPath.c_str(), vecDimCnt[0].nDim, nPreferredDim);
    }

    /* 回卷重读：两趟都走同一个 readLine，长行/空行/文件尾的语义不会分叉 */
    ifs.clear();
    ifs.seekg(0, std::ios::beg);
    if (!ifs.good())
    {
        dlog_error("classroom gallery: %s 回卷失败, 保留旧库 (rows=%d)",
                   m_strPath.c_str(), m_stStore.rowNum());
        m_stStat = stAcc;
        return false;
    }
    stTmp.beginLoad(nPreferredDim);

    while (readLine(true))
    {
        const int nParse = parseLine();
        if (0 == nParse)
        {
            continue;
        }
        ++nCandidate;

        if (stTmp.rowNum() >= m_nMaxEnroll)
        {
            ++stAcc.u64RejectFull;              /* 满了就只数不解析：省掉后半截文件的 strtod */
            continue;
        }
        if (nParse < 0)
        {
            ++stAcc.u64RejectBad;
            continue;
        }
        if (stTmp.add(strId, strName, vecFeat.data(), static_cast<int32_t>(vecFeat.size()),
                      m_nMaxEnroll, stAcc))
        {
            ++nAccepted;
        }
    }
    ifs.close();

    /* 「读到了候选行却一行都没认」当失败处理：这种文件多半是整份格式坏了，
       此时清空底库会让考勤从"能认人"退化成"谁也认不出"，而保留旧库只是名单可能过期一档。
       真要把名单清空就交一份没有数据行的文件（nCandidate==0），那条路径明确接受 */
    if (nCandidate > 0 && 0 == nAccepted)
    {
        dlog_error("classroom gallery: %s 有 %d 行数据但无一解析成功, 保留旧库 "
                   "(rows=%d dim=%d)", m_strPath.c_str(), nCandidate, m_stStore.rowNum(),
                   m_stStore.dim());
        m_stStat = stAcc;
        return false;
    }

    stTmp.finish();
    m_stStore = stTmp;
    m_nLastRows = stTmp.rowNum();
    m_stStat = stAcc;
    return true;
}

bool galleryMatchChecked(const CFaceGalleryStore &rstStore, const float *pafFeat, int32_t nDim,
                         CGalleryStat_S &rstStat, CFaceMatch_S &rstOut)
{
    rstOut = CFaceMatch_S();
    if (nullptr == pafFeat || nDim <= 0 || rstStore.empty())
    {
        return false;
    }
    if (nDim != rstStore.dim())
    {
        ++rstStat.u64ProbeDimBad;
        rstStat.nDimSeen = nDim;
        if (1 == rstStat.u64ProbeDimBad)
        {
            /* 只打第一次：维数不符是恒失效（每张脸都同维），一次就说清了；
               而它一旦被每次比对的日志淹没，就再没人看得见"库是 128 维"这件事 */
            dlog_error("classroom gallery: probe dim %d != 底库 dim %d, 比对必失败. "
                       "底库由另一版本模型生成", nDim, rstStore.dim());
        }
        return false;
    }
    return rstStore.bestMatch(pafFeat, nDim, rstOut);
}

bool CFileFaceGallery::match(const float *pafFeat, int32_t nDim, int64_t llNowMs,
                             CFaceMatch_S &rstOut)
{
    (void)llNowMs;    /* 文件库没有"新鲜度"概念：重载时刻已由 poll 决定，接口带它是给 sqlite 版判过期 */
    return galleryMatchChecked(m_stStore, pafFeat, nDim, m_stStat, rstOut);
}

bool galleryTableOk(const std::string &strTable)
{
    if (strTable.empty() || strTable.size() > 63)
    {
        return false;
    }
    for (size_t i = 0; i < strTable.size(); ++i)
    {
        const unsigned char uc = static_cast<unsigned char>(strTable[i]);
        const bool bAsciiOk = (('A' <= uc && uc <= 'Z') || ('a' <= uc && uc <= 'z') ||
                               ('0' <= uc && uc <= '9') || '_' == uc);
        /* 只放 ASCII 字母数字下划线与 UTF-8 高位字节（名单组名是中文是常态）。
           落在这一条外的是空格、引号、分号、控制字符——公共仓把表名拼进
           SELECT * FROM "%s" 的双引号里，一个引号就能把查询改成任意语句。
           长度 63 是给 sqlite 的标识符习惯留余量，也挡住"配置里粘了一整段文本" */
        if (!bAsciiOk && uc < 0x80)
        {
            return false;
        }
    }
    return true;
}
}
