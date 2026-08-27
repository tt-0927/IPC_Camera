#pragma once

#include <memory>

class CTaskManage;

/**
 * @brief TVSDK 回调注册/实现（从 tvsdk_server.cpp 拆分出来）
 * 
 * - 回调内部统一使用 action_code.h 中的命令码，通过 CTaskManage 执行
 * - tvsdk_server 负责设置 TaskManage 并调用 register
 */
namespace TvSdkCallbacks
{
    // 设置任务管理器实例（用于回调分发）
    void set_task_manage(const std::shared_ptr<CTaskManage>& taskManage);
    
    // 清空任务管理器引用
    void clear_task_manage();

    /**
     * @brief 注册所有 NetTVSDKServer 回调
     * @note 需要在 NET_serverInit 成功后调用
     */
    void register_all();
} // namespace TvSdkCallbacks