/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-13 10:09:35
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-25 09:51:20
 * @FilePath: /hisi/share/ipc_share/control/task/sub_task/storage_manage_task.cpp
 * @Description: 存储管理任务
 */
#include "storage_manage_task.h"
#include "storage_manage.h"
#include "convert_interface.h"
#include "action_code.h"
#include "storage_manage_configure.h"

/* 获取储存参数 */
void Task::StorageManage::GetStorageManageInfo::handle()
{
    StorageManage_NS::StorageManage_S stStorageManageParam;
    CStorageManage::instance()->get_storageManage_param(stStorageManageParam);
    result(Convert::to_string(stStorageManageParam), 0);
}

/* 设置储存参数 */
void Task::StorageManage::SetStorageManageInfo::handle()
{
    StorageManage_NS::StorageManage_S stStorageManageParam;
    Convert::to_struct(m_taskData, stStorageManageParam);
    CStorageManage::instance()->update_storageManage_param(stStorageManageParam);
    result(Convert::to_string(stStorageManageParam), 0);
}

/* 设置储存参数 */
void Task::StorageManage::FormatSdCard::handle()
{
    bool bIsInitSdCard = false;
    Convert::to_struct(m_taskData, bIsInitSdCard);
    int nRet = CStorageManage::instance()->format_sd_card(bIsInitSdCard);
    std::string strRet = nRet < 0 ? "格式化失败" : "格式化成功";
    result(Convert::to_string(strRet), nRet);
}
