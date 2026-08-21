/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-13 10:09:45
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-23 16:50:47
 * @FilePath: /hisi/share/ipc_share/control/task/sub_task/storage_manage_task.h
 * @Description: 存储管理任务
 */
#pragma once

#include "task_sub_class.h"

namespace Task
{
    namespace StorageManage
    {
        TaskSubClass(GetStorageManageInfo)
        TaskSubClass(SetStorageManageInfo)
        TaskSubClass(FormatSdCard)
        TaskSubClass(GetSdCardStatus);
    } /* namespace Storage end */
} /* namespace Task end */