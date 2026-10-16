/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 13:54:11
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-08-20 10:43:42
 * @FilePath: /hisi/share/ipc_share/control/business/storage/storage_manage_configure.cpp
 * @Description: 存储管理配置
 */

/* 本地 SD 存储能力裁剪：能力关闭（IPC_CAP_LOCAL_STORAGE=0）时本文件整体不参与编译 */
#if CAP_LOCAL_STORAGE

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

#endif /* CAP_LOCAL_STORAGE */
