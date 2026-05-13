/**
 * @FilePath     : web_plugin_task.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-06-11 15:24:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-11 15:24:22
 * @Description  : PC端的插件任务
 */

#pragma once
#include "task_sub_class.h"

namespace Task
{
    namespace WebPlugin
    {
        TaskSubClass(GetParam)
        TaskSubClass(SetParam)
    }    // namespace WebPlugin
}    // namespace Task