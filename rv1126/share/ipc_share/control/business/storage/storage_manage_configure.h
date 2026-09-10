/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 13:54:23
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-08-20 10:43:37
 * @FilePath: /hisi/share/ipc_share/control/business/storage/storage_manage_configure.h
 * @Description: 存储管理配置
 */
#pragma once

#include "storage_manage_define.h"
#include "storage_manage_convert.h"
#include "Singleton.h"
#include "config_storage.h"

class CStorageManageConfigure : public CSingleton<CStorageManageConfigure>
{
    CStorageManageConfigure();
public:
    ~CStorageManageConfigure();
    friend class CSingleton<CStorageManageConfigure>;

    /**
     * @brief   : 设置存储管理参数
     * @param    {StorageManage_S} &data：存储管理参数
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const StorageManage_NS::StorageManage_S &data);

    /**
     * @brief   : 获取存储管理参数划
     * @param    {StorageManage_S} &data：存储管理参数
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(StorageManage_NS::StorageManage_S &data) const;

private:
    /* 存储管理参数 */
    ConfigStorage<StorageManage_NS::StorageManage_S, StorageType_E::Single> m_storageManageParam;
};