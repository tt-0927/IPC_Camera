/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-05 20:39:29
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-03-12 14:11:24
 * @FilePath: /hisi/share/ipc_share/control/task/sub_task/retrieval_task.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @file retrieval_task.h
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-16
 * 
 * @brief 检索任务处理
 */

#pragma once
#include "task_sub_class.h"
#include "event_search.h"
#include "event_database_manage.h"


namespace Task
{
    namespace Retrieval
    {
        TaskSubClass(SearchByRecordType)
        TaskSubClass(SearchByRecordTS)
        TaskSubClass(SearchByImageType)
        TaskSubClass(DownloadImageFileInfo)
        TaskSubClass(SearchByDate)
        TaskSubClass(SearchByTime)
        TaskSubClass(SearchByVehicle)
        TaskSubClass(AddNormalEvent)
        TaskSubClass(DelNormalEvent)
    } /* namespace Event end */
} /* namespace Task end */
