/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 13:54:11
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-08-20 10:43:42
 * @FilePath: /hisi/share/ipc_share/control/business/storage/storage_manage_configure.cpp
 * @Description: 存储管理配置
 */
#include "storage_manage_configure.h"

CStorageManageConfigure::CStorageManageConfigure() : m_storageManageParam(STORAGE_MANAGE_CONFIG_FILE)
{
}

CStorageManageConfigure::~CStorageManageConfigure()
{
}

int CStorageManageConfigure::set_configure(const StorageManage_NS::StorageManage_S &data)
{
    return m_storageManageParam.set(data);
}

int CStorageManageConfigure::get_configure(StorageManage_NS::StorageManage_S &data) const
{
    return m_storageManageParam.get(data);
}