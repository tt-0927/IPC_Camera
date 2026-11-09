/**
 * @FilePath     : face_gallery_db.h
 * @Author       : leiyy
 * @Date         : 2026-10-21 15:23:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 14:23:00
 * @Description  : 平台名单库底库
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "face_gallery.h"

/* 前向声明而不是 #include "face_sqlite.hpp"：见文件头那段"为什么单独一个头"。
   代价是本类的析构必须落到 .cpp 里（shared_ptr 的删除器要在类型完整处实例化） */
namespace FaceDataDB_NS
{
class CFaceSqlite;
}

namespace Classroom
{
/**
 * @brief   : 只读接平台名单库
 */
class CSqliteFaceGallery : public IFaceGallery
{
public:
    CSqliteFaceGallery();
    ~CSqliteFaceGallery() override;

    CSqliteFaceGallery(const CSqliteFaceGallery &) = delete;
    CSqliteFaceGallery &operator=(const CSqliteFaceGallery &) = delete;

    /**
     * @brief   : 设定读哪张表与节奏（配置换版时调，不在节拍里调）
     * @param    {const std::string&} strTable：名单组表名。**空串=全部名单组**（getAllData），配成空是更少一个坑的默认
     * @return   {bool} false=表名不能安全拼进 SQL（见 galleryTableOk），调用方应保持上一份来源
     */
    bool setSource(const std::string &strTable, int32_t nReloadSec, int32_t nMaxEnroll);

    /* 到点且库文件被改过就整表重载。true=本次真的重载了 */
    bool poll(int64_t llNowMs) override;

    bool match(const float *pafFeat, int32_t nDim, int64_t llNowMs, CFaceMatch_S &rstOut) override;
    bool available() const override { return !m_stStore.empty(); }
    int32_t enrollNum() const override { return m_stStore.enrollNum(); }
    const char *name() const override { return "sqlite"; }

    CGalleryStat_S stat() const { return m_stStat; }
    int32_t nLastRows() const { return m_nLastRows; }
    int32_t dim() const { return m_stStore.dim(); }
    /* 库文件是否打开成功。false 时读数会一直是"底库不可用"，这条给现场一个明确归因 */
    bool dbOpen() const;

private:
    bool reload();
    /* 懒建：构造本类不等于碰磁盘，第一次真正要读库时才建 CFaceSqlite */
    bool ensureDb();

    std::shared_ptr<FaceDataDB_NS::CFaceSqlite> m_spDb;

    std::string m_strTable;
    int32_t     m_nMaxEnroll = CLASSROOM_GALLERY_MAX_ENROLL;
    int32_t     m_nLastRows = 0;
    bool        m_bDbTried = false;      /* 已经试过一次建库：失败就不反复试（每次试都可能创建文件） */
    CGalleryWatch m_stWatch;
    CFaceGalleryStore m_stStore;
    CGalleryStat_S    m_stStat;
};
}
