/***
 * @FilePath     : task_publish.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-02-13 17:41:05
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-27 17:41:05
 * @Description  :
 */
#pragma once
#include <string>

#include "task_manage.h"
#include "Singleton.h"

class TaskPublish : public CSingleton<TaskPublish>
{
    TaskPublish() = default;

public:
    ~TaskPublish() = default;
    friend class CSingleton<TaskPublish>;

    void set_manage(std::shared_ptr<CTaskManage> &pTaskManage)
    {
        if (pTaskManage == nullptr)
        {
            return;
        }
        m_pTaskManage = pTaskManage;
    }
    int message(int nActionCode, std::string data)
    {
        if (m_pTaskManage == nullptr)
        {
            return -1;
        }

        Json::Object *pJsonRoot = Json::init();
        Json::add(pJsonRoot, "ActionCode", nActionCode);
        Json::add(pJsonRoot, "DeviceName", "");
        Json::add(pJsonRoot, "UserName", "");

        Json::Object *pJsonData = Json::init(data);
        if (pJsonData)
        {
            Json::add(pJsonRoot, "Data", pJsonData);
        }

        data = Json::to_string(pJsonRoot);
        Json::deinit(pJsonRoot);
        return message(nActionCode, data.c_str(), data.size());
    }
    int message(int nActionCode, const void *pData, int nLen)
    {
        if (m_pTaskManage == nullptr)
        {
            return -1;
        }
        return m_pTaskManage->publish(nActionCode, pData, nLen);
    }

private:
    std::shared_ptr<CTaskManage> m_pTaskManage = nullptr;
};