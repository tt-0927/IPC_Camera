/**
 * @FilePath     : crypto_init.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-26 16:25:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-10
 * @Description  : 密码学模块业务层初始化
 */
#pragma once

#include "IpcRet.h"
#include "Singleton.h"

/**
 * @brief   : 密码学模块业务层初始化类
 * @note    : 作为业务层入口，负责注入具体 Provider 并初始化 CCryptoManager
 */
class CCryptoInit : public CSingleton<CCryptoInit>
{
public:
    CCryptoInit();
    ~CCryptoInit();

    /**
     * @brief   : 初始化密码学模块
     * @return   {IpcRet_E} OK: 成功，ERR: 失败
     */
    IpcRet_E init();

    /**
     * @brief   : 去初始化密码学模块
     * @return   {IpcRet_E} OK: 成功
     */
    IpcRet_E deinit();

private:
    bool m_bInitialized = false;
};
