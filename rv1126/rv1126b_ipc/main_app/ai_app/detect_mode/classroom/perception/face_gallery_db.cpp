/**
 * @FilePath     : face_gallery_db.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-21 15:23:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 14:23:00
 * @Description  : 平台名单库底库适配器
 */

#include "face_gallery_db.h"

#include <list>
#include <string>

#include "face_sqlite.hpp"

#include "dlog.h"

namespace Classroom
{
CSqliteFaceGallery::CSqliteFaceGallery() = default;

/* 定义在这里而不是 = default 在头文件：头文件里 CFaceSqlite 只是前向声明，
   shared_ptr 的删除器要在类型完整的地方实例化，否则 GCC 会给出一个"删除不完整类型"的错 */
CSqliteFaceGallery::~CSqliteFaceGallery() = default;

bool CSqliteFaceGallery::dbOpen() const
{
    return nullptr != m_spDb;
}

bool CSqliteFaceGallery::setSource(const std::string &strTable, int32_t nReloadSec,
                                   int32_t nMaxEnroll)
{
    if (!strTable.empty() && !galleryTableOk(strTable))
    {
        /* 不回显表名本身：它是配置文件里来的任意字节，进日志等于把注入面搬到日志里。
           长度 + 这一条理由足够定位到是哪个键配错了 */
        dlog_error("classroom gallery: 表名被拒 (含危险字符, len=%u), 沿用原 source",
                   static_cast<unsigned int>(strTable.size()));
        return false;
    }

    const int32_t nCap = (nMaxEnroll > 0) ? nMaxEnroll : CLASSROOM_GALLERY_MAX_ENROLL;
    const int32_t nSec = (nReloadSec > 0) ? nReloadSec : 0;
    if (strTable == m_strTable && nSec == m_stWatch.intervalSec() && nCap == m_nMaxEnroll)
    {
        return true;
    }

    dlog_info("classroom gallery: sqlite 来源 \"%s\" -> \"%s\"(safe), reload=%ds cap=%d rows=%d",
              m_strTable.c_str(), strTable.c_str(), nSec, nCap, m_stStore.rowNum());

    m_strTable = strTable;
    m_nMaxEnroll = nCap;
    m_stWatch.configure(nSec);
    m_stWatch.reset();

    /* 换表即作废旧库：与文件库同一条纪律（把上一个组的名单留给新场景用，
       表现是"甲班的人被记成乙班到课"） */
    m_stStore = CFaceGalleryStore();
    m_nLastRows = 0;
    return true;
}

bool CSqliteFaceGallery::ensureDb()
{
    if (nullptr != m_spDb)
    {
        return true;
    }
    if (m_bDbTried)
    {
        return false;
    }
    m_bDbTried = true;

    /* 走到这里说明库文件已经存在（poll 的 due 先 stat 过一遍），因此不会因"我们想看一眼"
       就在没有名单库的设备上凭空创建一个 db 文件——CFaceSqlite 的构造里 sqlite3_open
       对不存在的文件是**创建**而不是报错，这一步的顺序是本类存在的最大风险点 */
    m_spDb = std::make_shared<FaceDataDB_NS::CFaceSqlite>();
    dlog_info("classroom gallery: 已创建 sqlite 句柄 %s",
              FaceDataDB_NS::CFaceSqlite::dbFilePath());
    return nullptr != m_spDb;
}

bool CSqliteFaceGallery::poll(int64_t llNowMs)
{
    /* 路径只从 CFaceSqlite 拿一份：本类自己抄一份的话，平台改路径后我们看的是一个不存在的文件，
       失效形态是"底库永远不更新"，比打开失败更难查 */
    if (!m_stWatch.due(FaceDataDB_NS::CFaceSqlite::dbFilePath(), llNowMs))
    {
        return false;
    }
    if (!ensureDb())
    {
        return false;
    }
    if (!reload())
    {
        ++m_stStat.u64ReloadFail;
        if (1 == m_stStat.u64ReloadFail || 0 == (m_stStat.u64ReloadFail % 20))
        {
            dlog_warn("classroom gallery: sqlite 重载失败 (fails=%llu), 底库 rows=%d",
                      static_cast<unsigned long long>(m_stStat.u64ReloadFail), m_stStore.rowNum());
        }
        return false;
    }

    m_stWatch.commit();
    ++m_stStat.u64Reload;
    dlog_info("classroom gallery: sqlite 已重载, rows=%d persons=%d dim=%d "
              "reject(nofeat=%llu dim=%llu bad=%llu zero=%llu full=%llu)",
              m_nLastRows, m_stStore.enrollNum(), m_stStore.dim(),
              static_cast<unsigned long long>(m_stStat.u64RejectNoFeat),
              static_cast<unsigned long long>(m_stStat.u64RejectDim),
              static_cast<unsigned long long>(m_stStat.u64RejectBad),
              static_cast<unsigned long long>(m_stStat.u64RejectZero),
              static_cast<unsigned long long>(m_stStat.u64RejectFull));
    return true;
}

bool CSqliteFaceGallery::reload()
{
    if (nullptr == m_spDb)
    {
        return false;
    }

    std::list<FaceDataDB_NS::FaceLibsInfo_S> listRow;
    const IpcRet_E enRet = m_strTable.empty()
                         ? m_spDb->getAllData(listRow)
                         : m_spDb->searchDataByTable(m_strTable, listRow);
    if (listRow.empty())
    {
        /* 表还不存在（平台没建过名单组）与"确实一个人都没有"在这里分不开：
           前者返回负码，后者返回 OK + 空表。只有前者算失败——空名单是合法状态，
           把它当失败会让底库永远停在"上一次的内容"上 */
        return (enRet >= OK);
    }

    CFaceGalleryStore stTmp;
    CGalleryStat_S stAcc = m_stStat;

    /* 维度普查（与文件库同一口径）：取本批**多数行**的维度当锚点，并列取先出现者。
       只靠"第一行定死"时，库里一条半行粘贴的坏记录（或旧模型留下的低维行）会先占住维度，
       之后整班真行全被判 dim 拒——最终"认上 1 行"凑够非零，连"整份失败保留旧库"那条都不触发 */
    int32_t nPreferredDim = 0;
    {
        std::list<int32_t> listDim;             /* 维度种类个位数，线性找比 map 分配便宜 */
        std::list<int32_t> listNum;
        for (const FaceDataDB_NS::FaceLibsInfo_S &stRow : listRow)
        {
            if (stRow.vfData.empty())
            {
                continue;
            }
            const int32_t nDim = static_cast<int32_t>(stRow.vfData.size());
            std::list<int32_t>::iterator itNum = listNum.begin();
            bool bFound = false;
            for (std::list<int32_t>::iterator itDim = listDim.begin(); itDim != listDim.end();
                 ++itDim, ++itNum)
            {
                if (*itDim == nDim)
                {
                    ++(*itNum);
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                listDim.push_back(nDim);
                listNum.push_back(1);
            }
        }
        int32_t nBestNum = 0;
        std::list<int32_t>::iterator itNum = listNum.begin();
        for (std::list<int32_t>::iterator itDim = listDim.begin(); itDim != listDim.end();
             ++itDim, ++itNum)
        {
            if (*itNum > nBestNum)              /* 严格大于：并列取先出现者 */
            {
                nBestNum = *itNum;
                nPreferredDim = *itDim;
            }
        }
    }
    stTmp.beginLoad(nPreferredDim);

    int32_t nCandidate = 0;
    int32_t nAccepted = 0;
    int32_t nNoFeat = 0;
    int32_t nOverCap = 0;

    for (const FaceDataDB_NS::FaceLibsInfo_S &stRow : listRow)
    {
        ++nCandidate;

        if (stTmp.rowNum() >= m_nMaxEnroll)
        {
            ++stAcc.u64RejectFull;
            ++nOverCap;
            continue;
        }
        if (stRow.vfData.empty())
        {
            /* "照片已录入但还没建模"（ModelState 0/2）是常态而不是错误，单计一档：
               与 u64RejectBad 混计会让一份刚建好的名单库读数全红，现场分不出该找谁 */
            ++stAcc.u64RejectNoFeat;
            ++nNoFeat;
            continue;
        }

        /* 身份键用「表名#行 ID」。表内 ID 是 INTEGER PRIMARY KEY，跨表会撞号；
           而 FaceLibsInfo_S 里没有学号/工号这类平台侧稳定人标识（只有 Name 与手机号），
           能同时满足"稳定"和"唯一"的就只剩这个组合。名字单独放 strName，
           上报里出现的是它而不是这个键（姓名是个人信息，键不是） */
        const std::string strKey = stRow.strFaceLibName + "#" + std::to_string(stRow.nId);
        if (stTmp.add(strKey, stRow.strName, stRow.vfData.data(),
                      static_cast<int32_t>(stRow.vfData.size()), m_nMaxEnroll, stAcc))
        {
            ++nAccepted;
        }
    }

    const int32_t nFormatBad = nCandidate - nAccepted - nNoFeat - nOverCap;
    /* 「一行都没认上」且**是格式原因**才保留旧库。
       全是"没建模"时接受空库：那时把旧名单留着，会让已经离开这个班的人继续被认出来，
       那是比"暂时认不出人"更糟的一类错（人到课名单里有已销号的人） */
    if (0 == nAccepted && nFormatBad > 0)
    {
        dlog_error("classroom gallery: sqlite rows=%d 但 %d 不可用 (格式/dim), 保留旧 rows=%d dim=%d",
                   nCandidate, nFormatBad, m_stStore.rowNum(), m_stStore.dim());
        m_stStat = stAcc;
        return false;
    }

    stTmp.finish();
    m_stStore = stTmp;
    m_nLastRows = stTmp.rowNum();
    m_stStat = stAcc;
    return true;
}

bool CSqliteFaceGallery::match(const float *pafFeat, int32_t nDim, int64_t llNowMs,
                               CFaceMatch_S &rstOut)
{
    (void)llNowMs;    /* 新鲜度已由 poll 的重载节奏决定，比对这一路不看时刻 */
    return galleryMatchChecked(m_stStore, pafFeat, nDim, m_stStat, rstOut);
}
}
