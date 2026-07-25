/**
 * @FilePath     : event_vlm_manage.cpp
 * @Author       : cyc
 * @Date         : 2025-09-28 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-20 09:53:43
 * @Description  : 深度ai任务管理类实现
 */

 #ifdef SCENE_INTELLIGENT_ANALYSIS
#include "event_vlm_manage.hpp"
#include "convert_interface.h"
#include "event_define.h"
#include "user_manage.h"
#include "task_publish.h"

CEventVlmManager::CEventVlmManager()
{
}

CEventVlmManager::~CEventVlmManager()
{
}


/**
 * @brief 物理删除磁盘文件
 */
void CEventVlmManager::removeRecordFiles(const std::string& imagePath, const std::string& videoPath) 
{
    try {
        // 删除图片
        if (!imagePath.empty() && fs::exists(imagePath)) {
            fs::remove(imagePath);
            //dlog_debug("已物理删除过期报警图片: %s", imagePath.c_str());
        }
        
        // 删除视频备份 (安全校验：只删除包含备份路径标识的视频)
        if (!videoPath.empty() && videoPath.find(VLM_VIDEO) != std::string::npos) {
            if (fs::exists(videoPath)) {
                fs::remove(videoPath);
                //dlog_debug("已物理删除过期报警视频: %s", videoPath.c_str());
            }
        }
    } catch (const std::exception& e) {
        dlog_error("物理删除文件异常: %s", e.what());
    }
}

int CEventVlmManager::queryTextPresetTasks(const Alarm::TextPresetQueryFilter_S &stFilter,
                                           Alarm::TextPresetTaskManager_S &stResult)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    dlog_info("执行文字预设任务智能查询 - 任务名: [%s], 物体: [%s], 条件: [%s], 任务状态: %d",
              stFilter.strTaskNameFilter.c_str(),
              stFilter.strObjectNameFilter.c_str(),
              stFilter.strConditionNameFilter.c_str(),
              stFilter.enTaskStatusFilter);

    /* 获取所有任务配置 */
    Alarm::TextPresetTaskManager_S stTaskManager;
    int nRet = loadTasksFromConfig(stTaskManager);
    if (nRet != 0)
    {
        dlog_warn("获取任务管理配置失败，错误码: %d", nRet);
        return nRet;
    }

    /* 初始化返回结果 */
    stResult.strCurrentActiveTaskId = stTaskManager.strCurrentActiveTaskId;
    stResult.aTaskConfig.clear();

    int nFilteredCount = 0;

    /* 应用过滤条件 */
    for (const auto &task : stTaskManager.aTaskConfig)
    {
        std::string strRejectReason;

        if (applyFilter(task, stFilter, strRejectReason))
        {
            stResult.aTaskConfig.push_back(task);
            dlog_debug("任务 [%s] 通过智能过滤", task.strTaskName.c_str());
        }
        else
        {
            nFilteredCount++;
            dlog_debug("任务 [%s] 被过滤: %s", task.strTaskName.c_str(), strRejectReason.c_str());
        }
    }

    dlog_info("智能查询完成 - 原始任务数: %zu，过滤后任务数: %zu，被过滤: %d",
              stTaskManager.aTaskConfig.size(),
              stResult.aTaskConfig.size(),
              nFilteredCount);

    return 0;
}

int CEventVlmManager::setTextPresetTask(const Alarm::TextPreset_S &stTaskRequest)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    dlog_info("执行文字预设任务操作，类型: %d，任务ID: %s",
              stTaskRequest.enOperationType,
              stTaskRequest.strTaskId.c_str());

    /* 获取当前任务管理配置 */
    Alarm::TextPresetTaskManager_S stTaskManager;
    int nRet = loadTasksFromConfig(stTaskManager);
    if (nRet != 0)
    {
        /* 如果配置不存在，使   用默认空配置 */
        dlog_warn("获取任务管理配置失败，使用默认配置");
        stTaskManager.aTaskConfig.clear();
        stTaskManager.strCurrentActiveTaskId.clear();
    }

    bool bSuccess = false;
    int taskIndex = findTaskIndex(stTaskManager, stTaskRequest.strTaskId);

    /* 根据操作类型处理 */
    switch (stTaskRequest.enOperationType)
    {
    case Alarm::TASK_OP_ADD:
    {
        if (taskIndex >= 0)
        {
            dlog_warn("任务已存在，无法添加: %s", stTaskRequest.strTaskId.c_str());
            return -1;
        }
        else
        {
            /* 检查是否要启用新任务 */
            if (stTaskRequest.bEnable)
            {
                /* 检查是否已有启用的任务 */
                for (const auto &task : stTaskManager.aTaskConfig)
                {
                    if (task.bEnable)
                    {
                        dlog_error("已有文字预设任务正在运行，任务ID: %s，任务名: %s。请先禁用现有任务再启用新任务。",
                                   task.strTaskId.c_str(),
                                   task.strTaskName.c_str());
                        return -2; // 返回-2表示已有任务在运行
                    }
                }
            }

            dlog_info("添加文字预设任务: %s (%s)", stTaskRequest.strTaskId.c_str(), stTaskRequest.strTaskName.c_str());

            stTaskManager.aTaskConfig.push_back(stTaskRequest);
            bSuccess = true;
        }
        break;
    }

    case Alarm::TASK_OP_UPDATE:
    {
        if (taskIndex < 0)
        {
            dlog_warn("任务不存在，无法更新: %s", stTaskRequest.strTaskId.c_str());
            return -1;
        }
        else
        {
            /* 检查是否要启用当前任务 */
            if (stTaskRequest.bEnable)
            {
                /* 检查是否已有其他启用的任务 */
                for (const auto &task : stTaskManager.aTaskConfig)
                {
                    if (task.bEnable && task.strTaskId != stTaskRequest.strTaskId)
                    {
                        dlog_error(
                            "已有其他文字预设任务正在运行，任务ID: %s，任务名: %s。请先禁用现有任务再启用此任务。",
                            task.strTaskId.c_str(),
                            task.strTaskName.c_str());
                        return -2; // 返回-2表示已有任务在运行
                    }
                }
            }

            dlog_info("更新文字预设任务: %s (%s)", stTaskRequest.strTaskId.c_str(), stTaskRequest.strTaskName.c_str());

            stTaskManager.aTaskConfig[taskIndex] = stTaskRequest;
            bSuccess = true;
        }
        break;
    }

    case Alarm::TASK_OP_DELETE:
    {
        if (taskIndex < 0)
        {
            dlog_warn("要删除的任务不存在: %s", stTaskRequest.strTaskId.c_str());
            return -1;
        }
        else
        {
            dlog_info("删除文字预设任务: %s (%s)",
                      stTaskRequest.strTaskId.c_str(),
                      stTaskManager.aTaskConfig[taskIndex].strTaskName.c_str());

            /* 如果删除的是当前激活任务，清除激活状态 */
            if (stTaskManager.strCurrentActiveTaskId == stTaskRequest.strTaskId)
            {
                dlog_info("删除的是当前激活任务，清除激活状态");
                stTaskManager.strCurrentActiveTaskId.clear();
            }

            stTaskManager.aTaskConfig.erase(stTaskManager.aTaskConfig.begin() + taskIndex);
            bSuccess = true;
        }
        break;
    }

    default:
    {
        dlog_error("未知的操作类型: %d", stTaskRequest.enOperationType);
        return -1;
    }
    }

    if (bSuccess)
    {
        /* 更新激活任务：查找第一个启用的任务作为激活任务 */
        std::string newActiveTaskId;
        for (const auto &task : stTaskManager.aTaskConfig)
        {
            if (task.bEnable)
            {
                newActiveTaskId = task.strTaskId;
                break; /* 只激活第一个启用的任务 */
            }
        }
        stTaskManager.strCurrentActiveTaskId = newActiveTaskId;

        /* 保存配置到文件 */
        nRet = saveTasksToConfig(stTaskManager);
        if (nRet == 0)
        {
            dlog_info(
                "任务配置已保存，共 %zu 个任务，当前激活任务: %s",
                stTaskManager.aTaskConfig.size(),
                stTaskManager.strCurrentActiveTaskId.empty() ? "无" : stTaskManager.strCurrentActiveTaskId.c_str());

            /* 更新事件布防时间 */
            nRet = updateEventSchedule(stTaskManager);
            if (nRet != 0)
            {
                dlog_warn("更新事件布防时间失败，错误码: %d", nRet);
            }
        }
        else
        {
            dlog_error("保存任务配置失败，错误码: %d", nRet);
        }
    }

    return nRet;
}

std::string CEventVlmManager::getCurrentActiveTaskId()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Alarm::TextPresetTaskManager_S stTaskManager;
    int nRet = loadTasksFromConfig(stTaskManager);
    if (nRet == 0)
    {
        return stTaskManager.strCurrentActiveTaskId;
    }

    return "";
}

int CEventVlmManager::getAllTasks(Alarm::TextPresetTaskManager_S &stTaskManager)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return loadTasksFromConfig(stTaskManager);
}

int CEventVlmManager::loadTasksFromConfig(Alarm::TextPresetTaskManager_S &stTaskManager)
{
    return CEventConfigure::instance()->get_configure(stTaskManager);
}

int CEventVlmManager::saveTasksToConfig(const Alarm::TextPresetTaskManager_S &stTaskManager)
{
    return CEventConfigure::instance()->set_configure(stTaskManager);
}

int CEventVlmManager::findTaskIndex(const Alarm::TextPresetTaskManager_S &stTaskManager, const std::string &strTaskId)
{
    auto it = std::find_if(stTaskManager.aTaskConfig.begin(),
                           stTaskManager.aTaskConfig.end(),
                           [&strTaskId](const Alarm::TextPreset_S &task)
                           {
                               return task.strTaskId == strTaskId;
                           });

    if (it != stTaskManager.aTaskConfig.end())
    {
        return std::distance(stTaskManager.aTaskConfig.begin(), it);
    }

    return -1;
}

int CEventVlmManager::updateEventSchedule(const Alarm::TextPresetTaskManager_S &stTaskManager)
{
    /* 检查是否有启用的任务 */
    bool hasEnabledTask = std::any_of(stTaskManager.aTaskConfig.begin(),
                                      stTaskManager.aTaskConfig.end(),
                                      [](const Alarm::TextPreset_S &task)
                                      {
                                          return task.bEnable;
                                      });

    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::TEXT_PRESET;
    stEventSchedule.bStatus = hasEnabledTask;

    if (hasEnabledTask)
    {
        /* 使用第一个启用任务的布防时间 */
        for (const auto &task : stTaskManager.aTaskConfig)
        {
            if (task.bEnable)
            {
                stEventSchedule.defenseTime = task.aAlarmTime;
                break;
            }
        }
        dlog_info("文字预设事件布防时间已更新");
    }
    else
    {
        stEventSchedule.defenseTime.clear();
        dlog_info("文字预设事件布防已禁用");
    }

    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    if (nRet == 0)
    {
        CEventManage::instance()->update_event_schedule();
    }

    return nRet;
}

bool CEventVlmManager::applyFilter(const Alarm::TextPreset_S &task,
                                   const Alarm::TextPresetQueryFilter_S &stFilter,
                                   std::string &strRejectReason)
{
    /* 检查是否所有过滤条件都为空，如果是则返回全部记录 */
    bool bAllFiltersEmpty = stFilter.strTaskNameFilter.empty() && stFilter.strObjectNameFilter.empty()
                            && stFilter.strConditionNameFilter.empty()
                            && stFilter.enTaskStatusFilter == Alarm::TASK_STATUS_ALL;

    /* 1. 任务名称智能匹配 */
    if (!stFilter.strTaskNameFilter.empty())
    {
        if (!isStringMatch(task.strTaskName, stFilter.strTaskNameFilter))
        {
            strRejectReason = "任务名称不匹配";
            return false;
        }
    }

    /* 2. 物体名称智能匹配 */
    if (!stFilter.strObjectNameFilter.empty())
    {
        if (!isStringMatch(task.strObjectName, stFilter.strObjectNameFilter))
        {
            strRejectReason = "物体名称不匹配";
            return false;
        }
    }

    /* 3. 条件名称智能匹配 */
    if (!stFilter.strConditionNameFilter.empty())
    {
        if (!isStringMatch(task.strConditionName, stFilter.strConditionNameFilter))
        {
            strRejectReason = "条件名称不匹配";
            return false;
        }
    }

    /* 4. 任务状态精确匹配 */
    if (stFilter.enTaskStatusFilter != Alarm::TASK_STATUS_ALL)
    {
        if ((stFilter.enTaskStatusFilter == Alarm::TASK_STATUS_ENABLE && !task.bEnable)
            || (stFilter.enTaskStatusFilter == Alarm::TASK_STATUS_DISENABLE && task.bEnable))
        {
            strRejectReason = "任务状态不匹配";
            return false;
        }
    }

    return true;
}

/* ================ 智能匹配相关方法实现 ================ */

bool CEventVlmManager::isStringMatch(const std::string &source, const std::string &filter)
{
    if (filter.empty())
        return true;

    /* 智能判断匹配方式：
     * 1. 如果过滤字符串包含通配符(*、?)，使用通配符匹配
     * 2. 如果过滤字符串完全匹配源字符串，使用精确匹配
     * 3. 否则使用模糊匹配（包含关系）
     */

    /* 检查是否包含通配符 */
    if (filter.find('*') != std::string::npos || filter.find('?') != std::string::npos)
    {
        return wildcardMatch(source, filter);
    }

    /* 转换为小写进行不区分大小写的比较 */
    std::string lowerSource = toLower(source);
    std::string lowerFilter = toLower(filter);

    /* 精确匹配 */
    if (lowerSource == lowerFilter)
    {
        return true;
    }

    /* 模糊匹配（包含关系） */
    return lowerSource.find(lowerFilter) != std::string::npos;
}

std::string CEventVlmManager::toLower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(),
                   result.end(),
                   result.begin(),
                   [](unsigned char c)
                   {
                       return std::tolower(c);
                   });
    return result;
}

bool CEventVlmManager::wildcardMatch(const std::string &source, const std::string &pattern)
{
    std::string lowerSource = toLower(source);
    std::string lowerPattern = toLower(pattern);

    return wildcardMatchRecursive(lowerSource.c_str(), lowerPattern.c_str());
}

bool CEventVlmManager::wildcardMatchRecursive(const char *source, const char *pattern)
{
    /* 如果模式字符串结束 */
    if (*pattern == '\0')
    {
        return *source == '\0';
    }

    /* 如果遇到 * 通配符 */
    if (*pattern == '*')
    {
        /* 跳过连续的 * */
        while (*pattern == '*')
        {
            pattern++;
        }

        /* 如果 * 是最后一个字符，匹配成功 */
        if (*pattern == '\0')
        {
            return true;
        }

        /* 尝试匹配 * 后面的模式 */
        while (*source != '\0')
        {
            if (wildcardMatchRecursive(source, pattern))
            {
                return true;
            }
            source++;
        }

        return false;
    }

    /* 如果遇到 ? 通配符或字符匹配 */
    if (*pattern == '?' || *pattern == *source)
    {
        return wildcardMatchRecursive(source + 1, pattern + 1);
    }

    return false;
}

int CEventVlmManager::addRealAlarmPushRecord(const Alarm::RealAlarmPushRecord_S &stRecord)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    dlog_info("添加实时预警推送记录: 任务[%s], 条件[%s]",
              stRecord.strTaskName.c_str(),
              stRecord.strConditionName.c_str());

    /* 获取当前推送管理配置 */
    Alarm::RealAlarmPushManager_S stManager;
    int nRet = CEventConfigure::instance()->get_configure(stManager);
    if (nRet != 0)
    {
        /* 如果配置不存在，使用默认配置 */
        dlog_warn("获取实时预警推送配置失败，使用默认配置");
        stManager.aPushRecords.clear();
    }

    /* 添加新记录到列表开头 限制保存数量*/
    if ((int)stManager.aPushRecords.size() < MAX_RECORD_INFO_SIZE)
    {
       stManager.aPushRecords.insert(stManager.aPushRecords.begin(), stRecord);
    }
    else
    {
        const Alarm::RealAlarmPushRecord_S& oldestRecord = stManager.aPushRecords.back();
       // 清理相关文件
        removeRecordFiles(oldestRecord.strImagePath, oldestRecord.strVideoPath);

        stManager.aPushRecords.pop_back();                      // 挤掉最旧记录
        stManager.aPushRecords.insert(stManager.aPushRecords.begin(), stRecord);
    }
    
    /* 保存到配置文件 */
    nRet = CEventConfigure::instance()->set_configure(stManager);
    if (nRet == 0)
    {
        dlog_info("实时预警推送记录已保存，当前总数: %zu", stManager.aPushRecords.size());
        /*触发网页更新显示最新预警推送记录*/
        Alarm::RealAlarmPushManager_S stRecordInfo;
        stRecordInfo.bNotifyUpdate = true;
        TaskPublish::instance()->message(AC_GET_REAL_ALARM_PUSH_INFO, Convert::to_string(stRecordInfo));
    }
    else
    {
        dlog_error("保存实时预警推送记录失败，错误码: %d", nRet);
    }

    return nRet;
}

int CEventVlmManager::queryRealAlarmPushRecords(const Alarm::RealAlarmPushQueryFilter_S &stFilter,
                                                Alarm::RealAlarmPushManager_S &stResult)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    /* 检查是否为默认查询全部的情况 */
    bool bQueryAll = stFilter.strTaskNameFilter.empty() && stFilter.strObjectNameFilter.empty()
                     && stFilter.strConditionNameFilter.empty()
                     && stFilter.enDealStatusFilter == Alarm::PUSH_DEAL_STATUS_ALL
                     && stFilter.stStartTime.stDate.nYear <= 0 && stFilter.stEndTime.stDate.nYear <= 0;

    if (bQueryAll)
    {
        dlog_info("查询实时预警推送记录 - 查询全部记录（无过滤条件）");
    }
    else
    {
        dlog_info("查询实时预警推送记录 - 任务名: [%s], 目标: [%s], 条件: [%s], 处理状态: %d",
                  stFilter.strTaskNameFilter.c_str(),
                  stFilter.strObjectNameFilter.c_str(),
                  stFilter.strConditionNameFilter.c_str(),
                  stFilter.enDealStatusFilter);
    }

    /* 获取所有推送记录 */
    Alarm::RealAlarmPushManager_S stManager;
    int nRet = CEventConfigure::instance()->get_configure(stManager);
    if (nRet != 0)
    {
        dlog_warn("获取实时预警推送配置失败，错误码: %d", nRet);
        return nRet;
    }

    /* 更新自动配置项 */
    stResult.bAutoLaestAlarm = stManager.bAutoLaestAlarm;
    stResult.bAutoPlay = stManager.bAutoPlay;

    stResult.aPushRecords.clear();

    int nFilteredCount = 0;

    /* 应用过滤条件 */
    for (const auto &record : stManager.aPushRecords)
    {
        std::string strRejectReason;

        if (applyRealAlarmPushFilter(record, stFilter, strRejectReason))
        {
            stResult.aPushRecords.push_back(record);
            dlog_debug("记录 [%s] 通过过滤", record.strTaskName.c_str());
        }
        else
        {
            nFilteredCount++;
            dlog_debug("记录 [%s] 被过滤: %s", record.strTaskName.c_str(), strRejectReason.c_str());
        }
    }

    dlog_info("实时预警推送查询完成 - 原始记录数: %zu，过滤后记录数: %zu，被过滤: %d",
              stManager.aPushRecords.size(),
              stResult.aPushRecords.size(),
              nFilteredCount);

    /* 分页处理 */
    stResult.nTotalCount = static_cast<int>(stResult.aPushRecords.size());
    stResult.bHasMore = false;

    if (stResult.nTotalCount == 0)
    {
        return 0;
    }

    /* 游标模式（优先） */
    if (!stFilter.strCursor.empty())
    {
        int nPageSize = stFilter.nPageSize > 0 ? stFilter.nPageSize : stResult.nTotalCount;

        auto it = std::find_if(stResult.aPushRecords.begin(), stResult.aPushRecords.end(),
            [&](const Alarm::RealAlarmPushRecord_S &r) { return r.strTaskId == stFilter.strCursor; });

        if (it != stResult.aPushRecords.end())
        {
            int nOffset = static_cast<int>(std::distance(stResult.aPushRecords.begin(), it)) + 1;
            if (nOffset >= stResult.nTotalCount)
            {
                stResult.aPushRecords.clear();
            }
            else
            {
                int nEnd = std::min(nOffset + nPageSize, stResult.nTotalCount);
                stResult.bHasMore = (nEnd < stResult.nTotalCount);
                std::vector<Alarm::RealAlarmPushRecord_S> vstPaged(
                    stResult.aPushRecords.begin() + nOffset,
                    stResult.aPushRecords.begin() + nEnd);
                stResult.aPushRecords.swap(vstPaged);
            }
        }
        else
        {
            stResult.aPushRecords.clear();
        }
    }
    /* Offset 分页（向后兼容） */
    else if (stFilter.nPageSize > 0)
    {
        int nStart = stFilter.nPageIndex * stFilter.nPageSize;
        if (nStart >= stResult.nTotalCount)
        {
            stResult.aPushRecords.clear();
        }
        else
        {
            int nEnd = std::min(nStart + stFilter.nPageSize, stResult.nTotalCount);
            stResult.bHasMore = (nEnd < stResult.nTotalCount);
            std::vector<Alarm::RealAlarmPushRecord_S> vstPaged(
                stResult.aPushRecords.begin() + nStart,
                stResult.aPushRecords.begin() + nEnd);
            stResult.aPushRecords.swap(vstPaged);
        }
    }

    return 0;
}

bool CEventVlmManager::applyRealAlarmPushFilter(const Alarm::RealAlarmPushRecord_S &record,
                                                const Alarm::RealAlarmPushQueryFilter_S &stFilter,
                                                std::string &strRejectReason)
{
    /* 检查是否所有过滤条件都为空，如果是则返回全部记录 */
    bool bAllFiltersEmpty = stFilter.strTaskNameFilter.empty() && stFilter.strObjectNameFilter.empty()
                            && stFilter.strConditionNameFilter.empty()
                            && stFilter.enDealStatusFilter == Alarm::PUSH_DEAL_STATUS_ALL
                            && stFilter.stStartTime.stDate.nYear <= 0 && stFilter.stEndTime.stDate.nYear <= 0;

    if (bAllFiltersEmpty)
    {
        /* 所有过滤条件都为空，返回全部记录 */
        return true;
    }

    /* 1. 任务名称智能匹配 */
    if (!stFilter.strTaskNameFilter.empty())
    {
        if (!isStringMatch(record.strTaskName, stFilter.strTaskNameFilter))
        {
            strRejectReason = "任务名称不匹配";
            return false;
        }
    }

    /* 2. 目标名称智能匹配 */
    if (!stFilter.strObjectNameFilter.empty())
    {
        if (!isStringMatch(record.strObjectName, stFilter.strObjectNameFilter))
        {
            strRejectReason = "目标名称不匹配";
            return false;
        }
    }

    /* 3. 条件名称智能匹配 */
    if (!stFilter.strConditionNameFilter.empty())
    {
        if (!isStringMatch(record.strConditionName, stFilter.strConditionNameFilter))
        {
            strRejectReason = "条件名称不匹配";
            return false;
        }
    }

    /* 4. 处理状态匹配 - 修复逻辑，支持查询全部状态 */
    if (stFilter.enDealStatusFilter != Alarm::PUSH_DEAL_STATUS_ALL)
    {
        /* 如果过滤状态不是"全部"，则进行精确匹配 */
        if (stFilter.enDealStatusFilter != record.enDealStatus)
        {
            strRejectReason = std::string("处理状态不匹配，期望: ") + std::to_string(stFilter.enDealStatusFilter)
                              + ", 实际: " + std::to_string(record.enDealStatus);
            return false;
        }
    }
    /* 如果过滤状态是"全部"(PUSH_DEAL_STATUS_ALL = 3)，则不进行状态过滤，所有记录都通过 */

    /* 5. 时间范围过滤 */
    if (stFilter.stStartTime.stDate.nYear > 0 || stFilter.stEndTime.stDate.nYear > 0)
    {
        /* 将时间转换为可比较的格式 */
        auto recordTime = std::make_tuple(record.stAlarmTime.stDate.nYear,
                                          record.stAlarmTime.stDate.nMonth,
                                          record.stAlarmTime.stDate.nDay,
                                          record.stAlarmTime.stTime.nHour,
                                          record.stAlarmTime.stTime.nMinute,
                                          record.stAlarmTime.stTime.nSecond);

        if (stFilter.stStartTime.stDate.nYear > 0)
        {
            auto startTime = std::make_tuple(stFilter.stStartTime.stDate.nYear,
                                             stFilter.stStartTime.stDate.nMonth,
                                             stFilter.stStartTime.stDate.nDay,
                                             stFilter.stStartTime.stTime.nHour,
                                             stFilter.stStartTime.stTime.nMinute,
                                             stFilter.stStartTime.stTime.nSecond);

            if (recordTime < startTime)
            {
                strRejectReason = "时间早于开始时间";
                return false;
            }
        }

        if (stFilter.stEndTime.stDate.nYear > 0)
        {
            auto endTime = std::make_tuple(stFilter.stEndTime.stDate.nYear,
                                           stFilter.stEndTime.stDate.nMonth,
                                           stFilter.stEndTime.stDate.nDay,
                                           stFilter.stEndTime.stTime.nHour,
                                           stFilter.stEndTime.stTime.nMinute,
                                           stFilter.stEndTime.stTime.nSecond);

            if (recordTime > endTime)
            {
                strRejectReason = "时间晚于结束时间";
                return false;
            }
        }
    }

    return true;
}

int CEventVlmManager::processRealAlarmPushRecords(const Alarm::RealAlarmPushBatchRequest_S &stBatchRequest,
                                                  Alarm::RealAlarmPushBatchResult_S &stBatchResult)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const char *operationNames[] = { "处理", "删除", "忽视" };
    const char *opName = (stBatchRequest.enOperationType >= 0 && stBatchRequest.enOperationType <= 2)
                             ? operationNames[stBatchRequest.enOperationType]
                             : "未知";

    bool bIsBatch = stBatchRequest.aTaskIds.size() > 1;

    dlog_info("%s%s实时预警推送记录，操作类型: %d(%s)，任务数量: %zu",
              bIsBatch ? "批量" : "单个",
              opName,
              stBatchRequest.enOperationType,
              opName,
              stBatchRequest.aTaskIds.size());

    /* 初始化结果 */
    stBatchResult.nTotalCount = stBatchRequest.aTaskIds.size();
    stBatchResult.nSuccessCount = 0;
    stBatchResult.nFailureCount = 0;
    stBatchResult.aFailedTaskIds.clear();
    stBatchResult.aFailureReasons.clear();

    if (stBatchRequest.aTaskIds.empty())
    {
        dlog_warn("任务ID数组为空，无需处理");
        return 0;
    }

    /* 获取当前推送管理配置 */
    Alarm::RealAlarmPushManager_S stManager;
    int nRet = CEventConfigure::instance()->get_configure(stManager);
    if (nRet != 0)
    {
        dlog_error("获取实时预警推送配置失败，错误码: %d", nRet);
        return nRet;
    }

    /* 获取当前在线用户列表 */
    std::vector<::User::OnlineUser_S> vecOnlineUsers;
    vecOnlineUsers = CUserManage::instance()->get_online_users();

    /* 收集要删除的索引（仅用于删除操作，从大到小排序） */
    std::vector<int> indicesToDelete;

    /* 逐个处理任务ID */
    for (const std::string &strTaskId : stBatchRequest.aTaskIds)
    {
        if (strTaskId.empty())
        {
            stBatchResult.nFailureCount++;
            stBatchResult.aFailedTaskIds.push_back(strTaskId);
            stBatchResult.aFailureReasons.push_back("任务ID为空");
            continue;
        }

        /* 查找该任务ID对应的记录索引 */
        int recordIndex = findRecordIndexByTaskId(stManager, strTaskId);

        if (recordIndex < 0)
        {
            stBatchResult.nFailureCount++;
            stBatchResult.aFailedTaskIds.push_back(strTaskId);
            stBatchResult.aFailureReasons.push_back("未找到对应的记录");
            dlog_warn("未找到任务ID [%s] 对应的记录", strTaskId.c_str());
            continue;
        }

        /* 执行操作 */
        bool bProcessed = false;
        std::string strFailureReason;

        switch (stBatchRequest.enOperationType)
        {
        case Alarm::PUSH_OP_PROCESS:
            {
                /* 获取当前处理用户 */
                std::string strProcessUser = getCurrentProcessUser(vecOnlineUsers, stBatchRequest.strProcessRemark);
                /* 创建处理记录并添加到记录列表 */
                Alarm::RealAlarmProcessRecord_S processRecord =
                createProcessRecord(strProcessUser, stBatchRequest.strProcessRemark);
                stManager.aPushRecords[recordIndex].aProcessRecords.push_back(processRecord);
                /* 更新记录状态 */
                stManager.aPushRecords[recordIndex].enDealStatus = Alarm::PUSH_DEAL_STATUS_PROCESSED;

                bProcessed = true;
                dlog_info("任务 [%s] 已被用户 [%s] 标记为已处理", strTaskId.c_str(), strProcessUser.c_str());
                break;
            }
        case Alarm::PUSH_OP_DELETE:
            indicesToDelete.push_back(recordIndex);
            bProcessed = true;
            dlog_debug("任务 [%s] 标记为待删除", strTaskId.c_str());
            break;

        case Alarm::PUSH_OP_IGNORE:
            if (stManager.aPushRecords[recordIndex].enDealStatus == Alarm::PUSH_DEAL_STATUS_NONE)
            {
                /* 获取当前处理用户 */
                std::string strProcessUser = getCurrentProcessUser(vecOnlineUsers, stBatchRequest.strProcessRemark);

                /* 检查该用户是否已经处理过此记录 */
                if (!isUserAlreadyProcessed(stManager.aPushRecords[recordIndex].aProcessRecords, strProcessUser))
                {
                     /* 处理备注 */
                    std::string strTmpProcessRemark = stBatchRequest.strProcessRemark.empty() ? "已做忽略处理" : stBatchRequest.strProcessRemark; 
                    /* 创建处理记录并添加到记录列表 */
                    Alarm::RealAlarmProcessRecord_S processRecord =
                        createProcessRecord(strProcessUser, strTmpProcessRemark);
                    stManager.aPushRecords[recordIndex].aProcessRecords.push_back(processRecord);

                    /* 更新记录状态 */
                    stManager.aPushRecords[recordIndex].enDealStatus = Alarm::PUSH_DEAL_STATUS_IGNORE;

                    bProcessed = true;
                    dlog_info("任务 [%s] 已被用户 [%s] 标记为已忽略", strTaskId.c_str(), strProcessUser.c_str());
                }
                else
                {
                    strFailureReason = "用户 [" + strProcessUser + "] 已经处理过此记录";
                    dlog_warn("任务 [%s] 忽略失败: %s", strTaskId.c_str(), strFailureReason.c_str());
                }
            }
            else
            {
                strFailureReason =
                    "记录已被处理过，当前状态: " + std::to_string(stManager.aPushRecords[recordIndex].enDealStatus);
            }
            break;

            case Alarm::PUSH_OP_CANCEL_IGNORE:
            if (stManager.aPushRecords[recordIndex].enDealStatus == Alarm::PUSH_DEAL_STATUS_IGNORE)
            {
                /* 清除备注 */
                stManager.aPushRecords[recordIndex].aProcessRecords.clear();
                /* 更新记录状态 */
                stManager.aPushRecords[recordIndex].enDealStatus = Alarm::PUSH_DEAL_STATUS_NONE;
                bProcessed = true;
                dlog_info("任务 [%s] 已取消忽略", strTaskId.c_str());
            }
            else
            {
                strFailureReason =
                    "记录未忽略，无法取消，当前状态: " + std::to_string(stManager.aPushRecords[recordIndex].enDealStatus);
            }
            break;

        default:
            strFailureReason = "未知的操作类型: " + std::to_string(stBatchRequest.enOperationType);
            break;
        }

        if (bProcessed)
        {
            stBatchResult.nSuccessCount++;
        }
        else
        {
            stBatchResult.nFailureCount++;
            stBatchResult.aFailedTaskIds.push_back(strTaskId);
            stBatchResult.aFailureReasons.push_back(strFailureReason);
            dlog_warn("处理任务 [%s] 失败: %s", strTaskId.c_str(), strFailureReason.c_str());
        }
    }

    /* 执行删除操作（从后往前删除以避免索引变化） */
    if (!indicesToDelete.empty())
    {
        std::sort(indicesToDelete.rbegin(), indicesToDelete.rend()); /* 倒序排列 */
        for (int idx : indicesToDelete)
        {
            dlog_debug("删除索引 %d 的记录", idx);
            stManager.aPushRecords.erase(stManager.aPushRecords.begin() + idx);
        }
    }

    /* 保存配置 */
    if (stBatchResult.nSuccessCount > 0)
    {
        nRet = CEventConfigure::instance()->set_configure(stManager);
        if (nRet != 0)
        {
            dlog_error("保存实时预警推送配置失败，错误码: %d", nRet);
            return nRet;
        }
    }

    dlog_info("%s%s完成 - 总数: %d，成功: %d，失败: %d",
              bIsBatch ? "批量" : "单个",
              opName,
              stBatchResult.nTotalCount,
              stBatchResult.nSuccessCount,
              stBatchResult.nFailureCount);

    return 0;
}

std::string CEventVlmManager::getCurrentProcessUser(const std::vector<::User::OnlineUser_S> &vecOnlineUsers,
                                                    const std::string &strProcessRemark)
{
    /* 如果有在线用户，优先选择第一个在线用户 */
    if (!vecOnlineUsers.empty())
    {
        /* 可以根据业务逻辑选择合适的用户，这里选择第一个 */
        for (const auto &user : vecOnlineUsers)
        {
            if (!user.strUsername.empty())
            {
                dlog_debug("选择在线用户: [%s] 作为处理者", user.strUsername.c_str());
                return user.strUsername;
            }
        }
    }

    /* 如果没有在线用户或用户名为空，返回系统默认用户 */
    dlog_debug("未找到合适的在线用户，使用系统默认用户");
    return "系统";
}

bool CEventVlmManager::isUserAlreadyProcessed(const std::vector<Alarm::RealAlarmProcessRecord_S> &aProcessRecords,
                                              const std::string &strProcessUser)
{
    /* 检查该用户是否已经在处理记录中 */
    for (const auto &record : aProcessRecords)
    {
        if (record.strProcessUser == strProcessUser)
        {
            dlog_debug("用户 [%s] 已经处理过此记录", strProcessUser.c_str());
            return true;
        }
    }

    return false;
}

Alarm::RealAlarmProcessRecord_S CEventVlmManager::createProcessRecord(const std::string &strProcessUser,
                                                                      const std::string &strProcessRemark)
{
    Alarm::RealAlarmProcessRecord_S processRecord;

    /* 设置处理用户 */
    processRecord.strProcessUser = strProcessUser;

    /* 设置处理备注 */
    processRecord.strProcessRemark = strProcessRemark;

    /* 设置处理时间为当前时间 */
    std::time_t now = std::time(nullptr);
    std::tm *localTime = std::localtime(&now);

    processRecord.stProcessTime.stDate.nYear = localTime->tm_year + 1900;
    processRecord.stProcessTime.stDate.nMonth = localTime->tm_mon + 1;
    processRecord.stProcessTime.stDate.nDay = localTime->tm_mday;
    processRecord.stProcessTime.stTime.nHour = localTime->tm_hour;
    processRecord.stProcessTime.stTime.nMinute = localTime->tm_min;
    processRecord.stProcessTime.stTime.nSecond = localTime->tm_sec;

    dlog_debug("创建处理记录: 用户[%s], 备注[%s], 时间[%04d-%02d-%02d %02d:%02d:%02d]",
               processRecord.strProcessUser.c_str(),
               processRecord.strProcessRemark.c_str(),
               processRecord.stProcessTime.stDate.nYear,
               processRecord.stProcessTime.stDate.nMonth,
               processRecord.stProcessTime.stDate.nDay,
               processRecord.stProcessTime.stTime.nHour,
               processRecord.stProcessTime.stTime.nMinute,
               processRecord.stProcessTime.stTime.nSecond);

    return processRecord;
}

int CEventVlmManager::findRecordIndexByTaskId(const Alarm::RealAlarmPushManager_S &stManager,
                                              const std::string &strTaskId)
{
    /* 查找第一个匹配的记录（通常是最新的） */
    for (size_t i = 0; i < stManager.aPushRecords.size(); ++i)
    {
        if (stManager.aPushRecords[i].strTaskId == strTaskId)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int CEventVlmManager::setRealAlarmPushAutoConfig(const Alarm::RealAlarmPushBatchRequest_S &stConfigRequest)
{
    dlog_info("设置实时预警推送自动配置 - 自动更新最新报警: %s, 自动播放: %s",
              stConfigRequest.bAutoLaestAlarm ? "启用" : "禁用",
              stConfigRequest.bAutoPlay ? "启用" : "禁用");

    /* 获取当前配置 */
    Alarm::RealAlarmPushManager_S stCurrentManager;
    int nRet = CEventConfigure::instance()->get_configure(stCurrentManager);
    if (nRet != 0)
    {
        dlog_warn("获取当前实时预警推送配置失败，使用默认配置");
        stCurrentManager.aPushRecords.clear();
    }

    /* 只更新自动配置项，保持现有记录不变 */
    stCurrentManager.bAutoLaestAlarm = stConfigRequest.bAutoLaestAlarm;
    stCurrentManager.bAutoPlay = stConfigRequest.bAutoPlay;

    /* 保存配置 */
    nRet = CEventConfigure::instance()->set_configure(stCurrentManager);
    if (nRet == 0)
    {
        dlog_info("实时预警推送自动配置已保存 - 自动更新最新报警: %s, 自动播放: %s",
                  stCurrentManager.bAutoLaestAlarm ? "启用" : "禁用",
                  stCurrentManager.bAutoPlay ? "启用" : "禁用");
    }
    else
    {
        dlog_error("保存实时预警推送自动配置失败，错误码: %d", nRet);
    }

    return nRet;
}
#endif
