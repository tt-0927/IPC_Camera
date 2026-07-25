/*
 * @FilePath     : data_manage.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:24:26
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-17 14:30:12
 * @Description  : 数据管理模块
 */
#pragma once

#include "IpcRet.h"
#include "upgrade_define.h"

/**
 * @brief 获取当前升级状态
 * @param [TiUpgradeRuslut_E*] penStatus: 升级状态
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
IpcRet_E dataManage_get_upgradeStatus(TiUpgradeRuslut_E *penStatus);

/**
 * @brief 设置当前升级状态
 * @param [TiUpgradeRuslut_E] enStatus: 升级状态
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
IpcRet_E dataManage_set_upgradeStatus(TiUpgradeRuslut_E enStatus);

/**
 * @brief 初始化数据管理
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
IpcRet_E dataManage_init();
