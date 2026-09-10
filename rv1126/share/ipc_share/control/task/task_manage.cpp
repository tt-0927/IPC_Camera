/*** 
 * @FilePath     : task_manage.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-16 14:05:00
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-22 15:41:53
 * @Description  : 
 */

#include <map>
#include <memory>

#include "task_manage.h"

int CTaskManage::execute(int nActionCode, Task::Info_S stInfo)
{
    auto it = m_taskMap.find(nActionCode);
    if (it == m_taskMap.end())
    {
        dlog_error("没找到相关任务[%u]", nActionCode);
        return -1;
    }
    std::shared_ptr<Task::CTask> task = it->second;
    // std::shared_ptr<Task::CTask> task = it->second();
    if (task == nullptr)
    {
        dlog_error("任务指针为空");
        return -1;
    }
    /* 设置任务操作者IP */
    task->set_userIp(stInfo.strIp);
    // 判断是否是结果类数据
    if (task->is_result(stInfo.data))
    {
        /* 结果处理 */
        task->deal_result(stInfo.data);
        return 0;
    }
    // 不是则执行命令
    task->set_info(nActionCode, stInfo);
    task->handle(); // 执行任务
    return 0;
}
void CTaskManage::deal_result(int nActionCode, std::function<void(std::string)> fnDealFunc)
{
    auto it = m_taskMap.find(nActionCode);
    if (it == m_taskMap.end())
    {
        dlog_error("没找到相关任务[%u]", nActionCode);
        return;
    }
    // std::shared_ptr<Task::CTask> task = it->second();
    std::shared_ptr<Task::CTask> task = it->second;
    if (task == nullptr)
    {
        dlog_error("任务指针为空");
        return;
    }
    task->deal_result(fnDealFunc);
}

void CTaskManage::register_subscribe(int nActionCode, Task::ResultCallback fnResultCallback)
{
    auto it = m_taskMap.find(nActionCode);
    if (it == m_taskMap.end())
    {
        dlog_error("订阅失败，没找到相关任务[%u]", nActionCode);
        return;
    }
    std::shared_ptr<Task::CTask> task = it->second;
    if (task == nullptr)
    {
        dlog_error("任务指针为空");
        return;
    }
    task->register_subscribe(fnResultCallback);
}

void CTaskManage::register_subscribe(std::vector<int> actionCode, Task::ResultCallback fnResultCallback)
{
    for (auto &nActionCode : actionCode)
    {
        register_subscribe(nActionCode, fnResultCallback);
    }
}

int CTaskManage::publish(int nActionCode, const void *pData, int nLen)
{
    auto it = m_taskMap.find(nActionCode);
    if (it == m_taskMap.end())
    {
        dlog_error("发布失败，没找到相关任务[%u]", nActionCode);
        return -1;
    }
    std::shared_ptr<Task::CTask> task = it->second;
    if (task == nullptr)
    {
        dlog_error("任务指针为空");
        return -1;
    }
    task->publish(pData, nLen);
    return 0;
}
