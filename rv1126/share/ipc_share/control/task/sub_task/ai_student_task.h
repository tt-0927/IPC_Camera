/**
 * @FilePath     : ai_student_task.h
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-03 09:49:30
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2026-04-03 09:49:30
 * @Description  : 学生行为分析任务
 */

#pragma once
#ifdef ENABLE_AI_STUDENT
#include "task_sub_class.h"

namespace Task
{
    namespace AI_STUDENT
    {
        TaskSubClass(GetClassInfo)
        TaskSubClass(SetClassInfo)
        TaskSubClass(GetAttendanceRecordInfo);
        TaskSubClass(GetStudentBehaviorInfo);
        TaskSubClass(GetStudentFerformanceInfo);
    } /* namespace AI_STUDENT end */
} /* namespace Task end */

#endif