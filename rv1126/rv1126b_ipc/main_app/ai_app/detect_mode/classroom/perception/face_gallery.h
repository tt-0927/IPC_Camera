/**
 * @FilePath     : face_gallery.h
 * @Author       : leiyy
 * @Date         : 2026-10-21 14:16:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 14:16:00
 * @Description  : 人脸底库
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_face_port.h"

namespace Classroom
{
/* 底库行数上限。**按模板行数计而不是按人数计** */
#define CLASSROOM_GALLERY_MAX_ENROLL (256)

/* 单行字节数上限。8192 = 一行 512 维（每元素最多 ~15 字符，含 NaN/负号/科学计数）再加上
   ID 与姓名的余量。挡的是"文件被写成一行几百万个浮点"这类损坏；
   **行数**由 nMaxEnroll 与"超上限只计数、不再解析"两件事一起封住，不靠这一条 */
#define CLASSROOM_GALLERY_LINE_MAX (8192)

/**
 * @brief 底库观测计数（回答"为什么名单上没名字"，与账本的 u64NoMatch 互补）
 */
typedef struct _GalleryStat_
{
    uint64_t u64Reload = 0;        /* 成功整批重载的次数 */
    uint64_t u64ReloadFail = 0;    /* 重载失败（文件读不出、库打不开）：底库保持上一次的内容 */
    uint64_t u64RejectDim = 0;     /* 维度与本库不一致的行：配错模型 / 混了别的版本的底库 */
    uint64_t u64RejectBad = 0;     /* 行本身坏了：字段缺失、含非数、含 NaN、姓名长到不像姓名 */
    /* 行在、特征为空。sqlite 库里"照片已录入但还没建模"就落到这一档（ModelState=0/2），
       它是**正常状态**而不是错误：与 u64RejectBad 混计会让一份刚建好的名单库读数全红，
       现场就分不出"平台还没建模"和"底库文件坏了"这两件完全不同的处置 */
    uint64_t u64RejectNoFeat = 0;
    uint64_t u64RejectZero = 0;    /* 模长为 0 的特征（除模会 NaN，比"不像"更该单独一档） */
    uint64_t u64RejectFull = 0;    /* 超上限被丢的行：读数一到上限就该问平台侧为什么下发这么多 */
    /**
     * 探针维度与本库维度不一致的次数。**这一档必须独立存在**：库里是 128 维（公共仓那套写死的长度）
     * 而模型出 512 维时，比对结果是"每次都返回未命中"，在考勤侧读起来与"这班人今天都没来"
     * 完全一样。没有这一档，现场只能靠"底库行数对、却一次都没命中"反推
     */
    uint64_t u64ProbeDimBad = 0;
    int32_t  nDimSeen = 0;         /* 最近一次被拒行的维度，0=没遇到。归因用（512/128 一眼看出） */
} CGalleryStat_S;

/**
 * @brief   : 底库的纯计算内核（两个实现方共用，也是主机单测的直接对象）
 * @note    : 非线程安全，设计为节拍线程独占（见文件头线程模型）
 */
class CFaceGalleryStore
{
public:
    CFaceGalleryStore() = default;

    /**
     * @brief   : 开始一次整批重载：先清表，之后 add() 逐行填，最后 finish()
     * @param    {int32_t} nPreferredDim：本批**多数行**的维度（调用方先扫一遍统计）；
     *            0 = 不指定，退回"首个入参行定死"的老口径。给了非 0 值时，
     *            首行即使维度不符也只记一次 `u64RejectDim`，不会把整批真行挡在门外
     */
    void beginLoad(int32_t nPreferredDim = 0);

    /**
     * @brief   : 加一个模板行
     * @param    {const std::string&} strPersonId：平台侧稳定 ID，空则拒绝（名单去重靠它）
     * @param    {const std::string&} strName：展示名，可空
     * @param    {const float*} pafFeat：特征向量
     * @param    {int32_t} nDim：本行特征长度
     * @param    {int32_t} nMaxEnroll：行数上限（来自配置，<=0 时用 CLASSROOM_GALLERY_MAX_ENROLL）
     * @return   {bool} false 表示本行没进库，原因已计入 rstStat
     */
    bool add(const std::string &strPersonId, const std::string &strName,
             const float *pafFeat, int32_t nDim, int32_t nMaxEnroll, CGalleryStat_S &rstStat);

    /* 整批填完：算出"去重后的人数"，供应到人数的兜底来源用（<0 = 从没加载过） */
    void finish();

    /**
     * @brief   : 比对。**取全局最优的一条**，次优一律不报
     * @param    {const float*} pafProbe：探针特征，**不必自己归一化**（本函数内除一次模，
     *            库内已是单位长模板，两边都单位长之后点积才等于余弦——不除模的话
     *            账本那道 fMinMatchScore 门就是套在一个未归一的量上）
     * @param    {int32_t} nDim：探针特征长度，与本库不一致时整次比对判无效
     * @param    {CFaceMatch_S&} rstOut：命中时填 ID / 姓名 / 相似度；未命中时只填 bHit=false
     * @return   {bool} false=库空、维度不符或没有任何行。调用方按"没比上"处理，
     *             **不得**据此推断"不是本班学生"（访客、临时插班都会没对上）
     * @note     : bHit=true 只表示「库里有一行最优」，**不表示过了相似度门**：门限在账本侧
     *            （CAttendParam_S::fMinMatchScore）。store 刻意不设门，是为了同一次比对
     *            既能被考勤用、也能在单测里逐值断言分数——两处各设一道门迟早不一致
     */
    bool bestMatch(const float *pafProbe, int32_t nDim, CFaceMatch_S &rstOut) const;

    bool     empty() const { return m_vecId.empty(); }
    int32_t  rowNum() const { return static_cast<int32_t>(m_vecId.size()); }
    int32_t  dim() const { return m_nDim; }
    /* <0 = 还没加载成功过（与"加载过但库里 0 人"是两件事，后者上报应到 0 人是有意义的） */
    int32_t  enrollNum() const { return m_bFinished ? m_nEnrollNum : -1; }

private:
    /* 特征按行打平存放：一次重载一块连续缓冲（比对时顺序扫，缓存友好） */
    std::vector<std::string> m_vecId;
    std::vector<std::string> m_vecName;
    std::vector<float>       m_vecFeat;
    int32_t m_nDim = 0;
    /* 本批首选维度（beginLoad 传入，0=不指定） */
    int32_t m_nPreferredDim = 0;
    int32_t m_nEnrollNum = -1;
    bool    m_bFinished = false;   /* finish() 至少成功跑过一次 */
};

/**
 * @brief   : 底库来源的"现在该不该重读"探测（文件库与 sqlite 库共用同一份判据）
 */
class CGalleryWatch
{
public:
    /**
     * @brief   : 设定重载节奏；间隔变了就把基准清零，使下一次 due 一定为 true
     * @param    {int32_t} nReloadSec：<=0 表示只读一次（首次），此后恒 false
     */
    void configure(int32_t nReloadSec);

    /* 清零基准：换来源（换文件、换表）后必须让下一次 due 必为 true，与间隔有没有变无关 */
    void reset();

    /* 已生效的间隔秒（负值按 0 归一后的结果），配置层判"这次没变"要读它而不是读入参 */
    int32_t intervalSec() const { return m_nReloadSec; }

    /**
     * @brief   : 到点且文件与上次**确认过的**那份不一样时返回 true
     * @param    {const char*} pchPath：来源文件绝对路径（**刻意不用 std::string 形参**：
     *            poll 每拍都要调这里，传字符串就是调用点上每拍隐式构造一个临时串，
     *            而这类路径长度超过 SSO，等于节拍线程上一拍一次堆分配）
     */
    bool due(const char *pchPath, int64_t llNowMs);

    /* 把上一次 due 看到的 (mtime,size) 记为新基准 */
    void commit();

    uint64_t statFailNum() const { return m_u64StatFail; }

private:
    int32_t m_nReloadSec = 0;
    int64_t m_llLastPollMs = 0;    /* 0 = 从没判过（首次必读）。真实时刻恒 >1000ms，不会撞这个哨兵 */
    int64_t m_llMTimeMs = 0;       /* 已确认的那份文件的修改时刻（秒粒度 × 1000） */
    int64_t m_llSizeByte = -1;     /* -1 = 还没有任何已确认的基准 */
    int64_t m_llPendMTimeMs = 0;   /* 本次 due 看到、待 commit 的值 */
    int64_t m_llPendSizeByte = -1;
    uint64_t m_u64StatFail = 0;
};

/**
 * @brief 文件底库：每行「身份ID|姓名|f0,f1,...」的文本库
 */
class CFileFaceGallery : public IFaceGallery
{
public:
    CFileFaceGallery() = default;

    /**
     * @brief   : 设定来源与轮询节奏（配置换版时调，不在节拍里调）
     * @param    {const std::string&} strPath：底库文件绝对路径，空=关闭
     * @param    {int32_t} nReloadSec：最短重载间隔秒，<=0 表示只在启动时读一次
     */
    void setSource(const std::string &strPath, int32_t nReloadSec, int32_t nMaxEnroll);

    /**
     * @brief   : 到点且文件变了就重载（判据见 CGalleryWatch）
     * @return   {bool} true=本次真的重载了（调用方可据此打一条 info）
     */
    bool poll(int64_t llNowMs) override;

    bool match(const float *pafFeat, int32_t nDim, int64_t llNowMs, CFaceMatch_S &rstOut) override;
    bool available() const override { return !m_stStore.empty(); }
    int32_t enrollNum() const override { return m_stStore.enrollNum(); }
    const char *name() const override { return "file"; }

    CGalleryStat_S stat() const { return m_stStat; }
    int32_t nLastRows() const { return m_nLastRows; }
    int32_t dim() const { return m_stStore.dim(); }

private:
    /* 读一遍文件并整批替换。返回 false 时底库保持原样（宁可用旧库，不可用空库装死） */
    bool reload();

    std::string m_strPath;
    int32_t     m_nMaxEnroll = CLASSROOM_GALLERY_MAX_ENROLL;
    int32_t     m_nLastRows = 0;
    CGalleryWatch m_stWatch;
    CFaceGalleryStore m_stStore;
    CGalleryStat_S    m_stStat;
};

/**
 * @brief   : 名单组表名是否可作为 SQL 的一部分拼进查询
 * @return   {bool} false=含引号/分号/控制字符等，配置层据此拒绝整份配置
 */
bool galleryTableOk(const std::string &strTable);

/**
 * @brief   : 两个底库实现方共用的比对尾部：判维 → 计数 → 转交 store
 * @return   {bool} false=库空、维数不符或没有任何行
 */
bool galleryMatchChecked(const CFaceGalleryStore &rstStore, const float *pafFeat, int32_t nDim,
                         CGalleryStat_S &rstStat, CFaceMatch_S &rstOut);
}
