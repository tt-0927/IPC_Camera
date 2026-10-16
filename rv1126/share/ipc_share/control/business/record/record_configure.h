/**
 * @FilePath     : record_configure.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 20:30:57
 * @Description  : 录制配置
 */

/* 本地 SD 存储能力裁剪：能力关闭（IPC_CAP_LOCAL_STORAGE=0）时本文件整体不参与编译 */
#if CAP_LOCAL_STORAGE

#include "Singleton.h"
#include "config_storage.h"
#include "record_define.h"
#include "system_define.h"
#include "config_storage_v2.h"

class RecordConfigure : public CSingleton<RecordConfigure>
{
    RecordConfigure();
public:
    ~RecordConfigure();    
    friend class CSingleton<RecordConfigure>;

    /* 录制高级参数配置 */
    int set_configure(const Record_NS::AdvancedParam_S &data);
    int get_configure(Record_NS::AdvancedParam_S &data) const;

    /* 录制计划配置 */
    int set_configure(const Record_NS::Schedule_S &data);
    int get_configure(Record_NS::Schedule_S &data) const;

private:
    /*高级录制参数配置*/
    VersionedConfigStorage<Record_NS::AdvancedParam_S, VersionedStorageType_E::Single> m_advancedParam;
    /*录制计划配置*/
    VersionedConfigStorage<Record_NS::Schedule_S, VersionedStorageType_E::Single> m_schedule;
};

#endif /* CAP_LOCAL_STORAGE */
