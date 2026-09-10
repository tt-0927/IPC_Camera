/***
 * @FilePath     : task_sub_class.h
 * @Author       : huangjunda
 * @Date         : 2025-03-26 14:13:05
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-26 14:13:36
 * @Description  :
 */
#pragma once
#include "task.h"

namespace Task
{
#define TaskSubClass(interface)                         \
    class interface : public CTask                      \
    {                                                   \
    public:                                             \
        interface(std::string taskData = std::string()) \
            : CTask(taskData)                           \
        {                                               \
        }                                               \
        void handle() override;                         \
                                                        \
    private:                                            \
    };
}