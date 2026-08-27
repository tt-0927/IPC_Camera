/*** 
 * @FilePath     : task_manage.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-09-30 14:05:00
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-09 09:19:05
 * @Description  : 
 */

#pragma once

#include <map>
#include <memory>
#include <vector>
#include <set>

#include "dlog.h"
#include "av_task.h"
#include "capture_task.h"
#include "preview_task.h"
#include "system_task.h"
#include "web_plugin_task.h"
#include "network_task.h"
// #include "ReplayTask.h"
#include "record_task.h"
#include "task.h"
#include "user_task.h"
#include "event_task.h"
#include "picture_task.h"
#include "register_task.h"

class CTaskManage
{
public:
    CTaskManage() = default;
    ~CTaskManage() = default;
    /**
     * @brief 任务绑定
     * @tparam TaskType  任务类
     * @param nActionCode 命令码
     */
    template <class TaskType>
    void bind(int nActionCode)
    {
        m_taskMap[nActionCode] = std::make_shared<TaskType>();
        m_taskMap[nActionCode]->set_actionCode(nActionCode);
        // m_taskMap[nActionCode] = []() -> std::shared_ptr<Task::CTask> {
        //     return std::make_shared<TaskType>();
        // };
    }
    /**
     * @brief 任务执行
     * @param nActionCode 命令码
     * @param stInfo 任务相关信息
     * @return int
     */
    int execute(int nActionCode, Task::Info_S stInfo);
    void deal_result(int nActionCode, std::function<void(std::string)> fnDealFunc);
    /**
     * @brief 注册订阅信息
     * @param nActionCode 命令码
     * @param fnResultCallback 回调函数
     */
    void register_subscribe(int nActionCode, Task::ResultCallback fnResultCallback);
    void register_subscribe(std::vector<int> actionCode, Task::ResultCallback fnResultCallback);
    int publish(int nActionCode, const void *pData, int nLen);

private:
    /**
     * @brief 任务map，<命令码，任务类>
     */
    std::map<int, std::shared_ptr<Task::CTask>> m_taskMap;
    // std::map<int, std::function<std::shared_ptr<Task::CTask>()>> m_taskMap;
    /**
     * @brief 订阅map, <命令码，回调函数>
     */
    // std::map<int, std::set<Task::ResultCallback, Task::ResultCallbackCompare>> m_subscribeMap;
};
