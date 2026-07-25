/***
 * @FilePath     : event_vlm_manage.hpp
 * @Author       : cyc
 * @Date         : 2025-09-28 15:00:00
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-28 15:00:00
 * @Description  : 深度ai任务管理类实现
 */

#pragma once

#ifdef SCENE_INTELLIGENT_ANALYSIS

#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include "alarm_define.h"
#include "event_configure.h"
#include "event_manage.h"
#include "Singleton.h"
#include "common_process.h"

namespace fs = std::filesystem;


/*最大记录数量*/
#define MAX_RECORD_INFO_SIZE 150

/**
 * @brief 深度ai任务管理类实现
 */
class CEventVlmManager : public CSingleton<CEventVlmManager>
{
public:
    CEventVlmManager();
    ~CEventVlmManager();
    friend class CSingleton<CEventVlmManager>;

    /**
     * @brief 删除关联的报警图片及视频备份文件
     * @param imagePath 待删除的报警图片完整物理路径
     * @param videoPath 待删除的视频分片（TS文件）完整物理路径
     */
    void removeRecordFiles(const std::string& imagePath, const std::string& videoPath);

    /**
     * @brief 查询文字预设任务（支持智能过滤）
     * @param stFilter 查询过滤条件
     * @param stResult 返回的查询结果
     * @return 成功返回0，失败返回错误码
     */
    int queryTextPresetTasks(const Alarm::TextPresetQueryFilter_S &stFilter, Alarm::TextPresetTaskManager_S &stResult);

    /**
     * @brief 设置文字预设任务（添加/更新/删除）
     * @param stTaskRequest 任务请求
     * @return 成功返回0，失败返回错误码
     */
    int setTextPresetTask(const Alarm::TextPreset_S &stTaskRequest);

    /**
     * @brief 获取当前激活的任务ID
     * @return 当前激活的任务ID
     */
    std::string getCurrentActiveTaskId();

    /**
     * @brief 获取所有任务配置
     * @param stTaskManager 返回的任务管理配置
     * @return 成功返回0，失败返回错误码
     */
    int getAllTasks(Alarm::TextPresetTaskManager_S &stTaskManager);

    /**
     * @brief 添加实时预警推送记录
     * @param stRecord 推送记录
     * @return 成功返回0，失败返回错误码
     */
    int addRealAlarmPushRecord(const Alarm::RealAlarmPushRecord_S &stRecord);

    /**
     * @brief 查询实时预警推送记录（支持智能过滤）
     * @param stFilter 查询过滤条件
     * @param stResult 返回的查询结果
     * @return 成功返回0，失败返回错误码
     */
    int queryRealAlarmPushRecords(const Alarm::RealAlarmPushQueryFilter_S &stFilter,
                                  Alarm::RealAlarmPushManager_S &stResult);

    /**
     * @brief 处理实时预警推送记录（统一支持单个和批量处理、删除、忽视操作）
     * @param stBatchRequest 批量处理请求（单个处理时TaskId数组只有一个元素）
     * @param stBatchResult 批量处理结果
     * @return 成功返回0，失败返回错误码
     */
    int processRealAlarmPushRecords(const Alarm::RealAlarmPushBatchRequest_S &stBatchRequest,
                                    Alarm::RealAlarmPushBatchResult_S &stBatchResult);

    /**
     * @brief 设置实时预警推送自动配置
     * @param stManager 推送管理配置
     * @return 成功返回0，失败返回错误码
     */
    int setRealAlarmPushAutoConfig(const Alarm::RealAlarmPushBatchRequest_S &stManager);

private:
    /**
     * @brief 从配置文件加载任务
     * @param stTaskManager 任务管理配置
     * @return 成功返回0，失败返回错误码
     */
    int loadTasksFromConfig(Alarm::TextPresetTaskManager_S &stTaskManager);

    /**
     * @brief 保存任务到配置文件
     * @param stTaskManager 任务管理配置
     * @return 成功返回0，失败返回错误码
     */
    int saveTasksToConfig(const Alarm::TextPresetTaskManager_S &stTaskManager);

    /**
     * @brief 查找任务索引
     * @param stTaskManager 任务管理配置
     * @param strTaskId 任务ID
     * @return 找到返回索引，未找到返回-1
     */
    int findTaskIndex(const Alarm::TextPresetTaskManager_S &stTaskManager, const std::string &strTaskId);

    /**
     * @brief 更新事件布防时间
     * @param stTaskManager 任务管理配置
     * @return 成功返回0，失败返回错误码
     */
    int updateEventSchedule(const Alarm::TextPresetTaskManager_S &stTaskManager);

    /**
     * @brief 应用过滤条件
     * @param task 任务配置
     * @param stFilter 过滤条件
     * @param strRejectReason 拒绝原因（输出参数）
     * @return 是否通过过滤
     */
    bool applyFilter(const Alarm::TextPreset_S &task,
                     const Alarm::TextPresetQueryFilter_S &stFilter,
                     std::string &strRejectReason);

    /**
     * @brief 应用实时预警推送过滤条件
     * @param record 推送记录
     * @param stFilter 过滤条件
     * @param strRejectReason 拒绝原因（输出参数）
     * @return 是否通过过滤
     */
    bool applyRealAlarmPushFilter(const Alarm::RealAlarmPushRecord_S &record,
                                  const Alarm::RealAlarmPushQueryFilter_S &stFilter,
                                  std::string &strRejectReason);

    /**
     * @brief 根据任务ID查找记录索引
     * @param stManager 推送管理配置
     * @param strTaskId 任务ID
     * @return 找到返回索引，未找到返回-1
     */
    int findRecordIndexByTaskId(const Alarm::RealAlarmPushManager_S &stManager, const std::string &strTaskId);

    /**
     * @brief 获取当前操作用户信息
     * @param vecOnlineUsers 在线用户列表
     * @param strProcessRemark 处理备注
     * @return 处理用户名，如果没有找到则返回"系统"
     */
    std::string getCurrentProcessUser(const std::vector<::User::OnlineUser_S> &vecOnlineUsers,
                                      const std::string &strProcessRemark);

    /**
     * @brief 检查用户是否已经处理过该记录
     * @param aProcessRecords 已处理记录列表
     * @param strProcessUser 处理用户
     * @return 如果用户已处理过返回true，否则返回false
     */
    bool isUserAlreadyProcessed(const std::vector<Alarm::RealAlarmProcessRecord_S> &aProcessRecords,
                                const std::string &strProcessUser);

    /**
     * @brief 创建处理记录
     * @param strProcessUser 处理用户
     * @param strProcessRemark 处理备注
     * @return 处理记录结构体
     */
    Alarm::RealAlarmProcessRecord_S createProcessRecord(const std::string &strProcessUser,
                                                        const std::string &strProcessRemark);

    /* ================ 智能匹配相关私有方法 ================ */

    /**
     * @brief 智能字符串匹配
     * @param source 源字符串
     * @param filter 过滤字符串
     * @return 是否匹配
     */
    bool isStringMatch(const std::string &source, const std::string &filter);

    /**
     * @brief 转换字符串为小写
     * @param str 输入字符串
     * @return 小写字符串
     */
    std::string toLower(const std::string &str);

    /**
     * @brief 通配符匹配
     * @param source 源字符串
     * @param pattern 包含通配符的模式字符串
     * @return 是否匹配
     */
    bool wildcardMatch(const std::string &source, const std::string &pattern);

    /**
     * @brief 递归通配符匹配实现
     * @param source 源字符串
     * @param pattern 模式字符串
     * @return 是否匹配
     */
    bool wildcardMatchRecursive(const char *source, const char *pattern);

private:
    /* 线程安全锁 */
    std::mutex m_mutex;
};
#endif
