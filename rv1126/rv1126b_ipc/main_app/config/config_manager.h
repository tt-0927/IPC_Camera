/**
 * @FilePath     : config_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 11:22:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-11 11:02:15
 * @Description  : 全局配置管理器
 */

#pragma once

#include <string>
#include <memory>

#include "IpcRet.h"
#include "config_module.h"
#include "strategy_factory.h"
#include "share_define.h"

/**
* @brief   : 全局配置管理器
* @note    : 负责初始化所有业务配置模块（视频、音频、网络等）。
*          是系统配置功能的唯一入口。
*/
class CConfigManager : public CSingleton<CConfigManager>
{
    CConfigManager();
public:
    virtual ~CConfigManager();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CConfigManager>;

    /**
    * @brief   : 初始化所有配置
    * @note    : 这是外部调用的主入口函数。它会依次初始化所有配置模块。
    * @return  {int} 0：成功，非0：任意一个模块初始化失败
    */
    int init();

private:
    /**
     * @brief   : 初始化系统配置模块
     * @return   {int} 0：成功，非0：失败
     */
    int initSystemConfig();

private:
    /* 当前设备型号 */
    std::string m_strModelName;

    /* 指向设备信息配置模块的智能指针 */
    std::unique_ptr<CConfigModule<System::DeviceInfo_S>> m_pDeviceInfoConfigModule;
};
