/**
 * @FilePath     : control_manage.h
 * @Author       : huangjunda
 * @Date         : 2025-03-27 20:01:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 10:42:25
 * @Description  : 控制事务任务管理
 */

#pragma once

#include <signal.h>
#include <memory>

#include "task_manage.h"
#include "task_publish.h"

#include "dlog.h"
#include "Singleton.h"
#include "IpcRet.h"

#ifdef ENABLE_TVSDK_SRC
    #include "tvsdk_server.h"
#endif

class ControlManage : public CSingleton<ControlManage>
{
    ControlManage() = default;

public:
    ~ControlManage() = default;
    friend class CSingleton<ControlManage>;
    /*** 
     * @description : 初始化入口（日志、任务、硬件等）
     * @author      : huangjunda
     * @return       {*}
     */    
    IpcRet_E init();
    /*** 
     * @description : 去初始化入口（日志、任务、硬件等）
     * @author      : huangjunda
     * @return       {*}
     */ 
    IpcRet_E deinit();

#ifdef ENABLE_TVSDK_SRC
    /**
     * @brief TVSDK：推送告警（事件模块使用）
     * @param lCommand 告警命令码/类型（NET_TV_ALARM_*）
     * @param pAlarmInfo 告警结构体指针
     * @param dwBufLen 告警结构体长度
     * @return 0 成功，负值失败
     */
    int tvsdk_push_alarm(int lCommand, const void *pAlarmInfo, int dwBufLen, const void *pAlarmer = nullptr);

    /**
     * @brief TVSDK：获取当前在线客户端数量
     * @return >=0 在线客户端数量，<0 表示 TVSDK 服务不可用或获取失败
     */
    int tvsdk_get_client_count() const;
#endif

private:
    /*** 
     * @description : 初始化事务
     * @author      : huangjunda
     * @return       {*}
     */    
    int init_business();
    /*** 
     * @description : 去初始化事务
     * @author      : huangjunda
     * @return       {*}
     */    
    void deinit_business();
    /*** 
     * @description : 初始化服务
     * @author      : huangjunda
     * @return       {*}
     */    
    int init_server(std::shared_ptr<CTaskManage> &pTaskManage);
    /*** 
     * @description : 去初始化服务
     * @author      : huangjunda
     * @return       {*}
     */    
    void deinit_server();
    /*** 
     * @description : 绑定任务
     * @author      : huangjunda
     * @param        {shared_ptr<CTaskManage>} &pTaskManage
     * @return       {*}
     */    
    void bind_task(std::shared_ptr<CTaskManage> &pTaskManage);

    // 成员变量
    bool m_initialized = false;

#ifdef ENABLE_TVSDK_SRC
    std::unique_ptr<CTvSdkServer> m_pTvSdkServer;
#endif
};
