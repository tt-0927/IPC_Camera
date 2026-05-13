/*
 * @FilePath     : upgrade_logic.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:22:29
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-17 14:30:24
 * @Description  : 升级逻辑模块
 */
#pragma once

#include "IpcRet.h"
#include "upgrade_define.h"

/**
 * @brief 开始升级
 * @param [UpgradeInfo_S] stUpgradeInfo: 升级信息
 * @return [*] BlError_E::OK 成功  其他失败
 * @note
 */
IpcRet_E upgrade_start(UpgradeInfo_S stUpgradeInfo);
