/*
 * @FilePath     : upgrade_logic.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:22:29
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-12-24 20:10:12
 * @Description  : 升级逻辑模块
 */

#pragma once

#include "IpcRet.h"
#include "upgrade_define.h"

 extern "C" {
    #include "md5lib.h"
 }

/**
 * @brief 开始升级
 * @param [UpgradeInfo_S] stUpgradeInfo: 升级信息
 * @return [*] BlError_E::OK 成功  其他失败
 * @note
 */
IpcRet_E upgrade_start(UpgradeInfo_S stUpgradeInfo);
