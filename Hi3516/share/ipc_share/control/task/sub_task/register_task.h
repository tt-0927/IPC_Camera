/*** 
 * @FilePath     : register_task.h
 * @Author       : huangjunda
 * @Date         : 2025-07-08 14:17:20
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-08 14:34:06
 * @Description  : 激活操作
 */

#pragma once

#include "task_sub_class.h"

namespace Task
{
    namespace Register
    {
        TaskSubClass(GetReisterInfo)
		TaskSubClass(SetRegisterEg)
        TaskSubClass(SetActivationPasswd)
        TaskSubClass(GetTimeInfo)
        TaskSubClass(ManaualConfigTime)
        TaskSubClass(ManaualConfigNetWork)
        TaskSubClass(GetActivationInfo)
        TaskSubClass(AutoConfigNetwork)
        
    } // namespace Register
} // namespace Task