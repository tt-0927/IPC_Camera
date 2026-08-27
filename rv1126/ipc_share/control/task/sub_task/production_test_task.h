/*** 
 * @FilePath     : production_test_task.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 11:25:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-08 11:25:00
 * @Description  : 产测任务
 */

#pragma once
#include "task_sub_class.h"

namespace Task
{
    namespace ProductionTest
    {
        TaskSubClass(GetItems)
        TaskSubClass(GetResult)
        TaskSubClass(SaveResult)
        TaskSubClass(UploadResult)
        TaskSubClass(ResetResult)
    }
}
