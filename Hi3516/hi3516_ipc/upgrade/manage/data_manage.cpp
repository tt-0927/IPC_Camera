/**
 * @FilePath     : data_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-01 08:49:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-01 09:21:26
 * @Description  : 数据管理模块
 */

#include "data_manage.h"

#include "dlog.h"

static Upgrade_Handle_t gs_stUpgradeHandle;

/* 获取当前升级状态 */
IpcRet_E dataManage_get_upgradeStatus(TiUpgradeRuslut_E *penStatus)
{
    if (nullptr == penStatus)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM_NULL;
    }

    pthread_mutex_lock(&(gs_stUpgradeHandle.mutex));
    *penStatus = gs_stUpgradeHandle.enStatus;
    pthread_mutex_unlock(&(gs_stUpgradeHandle.mutex));
    return OK;
}

/* 设置当前升级状态 */
IpcRet_E dataManage_set_upgradeStatus(TiUpgradeRuslut_E enStatus)
{
    pthread_mutex_lock(&(gs_stUpgradeHandle.mutex));
    gs_stUpgradeHandle.enStatus = enStatus;
    pthread_mutex_unlock(&(gs_stUpgradeHandle.mutex));

    return OK;
}

/* 初始化数据管理 */
IpcRet_E dataManage_init()
{
    pthread_mutex_init(&(gs_stUpgradeHandle.mutex), NULL);
    gs_stUpgradeHandle.enStatus = TI_UPGRADE_NULL;

    return OK;
}
