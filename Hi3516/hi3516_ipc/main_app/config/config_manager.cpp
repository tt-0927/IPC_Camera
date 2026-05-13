/**
 * @FilePath     : config_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 11:12:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-13 19:52:19
 * @Description  : 全局配置管理器
 */

#include <stdexcept>

#include "config_manager.h"
#include "strategy_factory.h"
#include "dlog.h"

CConfigManager::CConfigManager() : m_strModelName(DEVICE_CODE)
{
    dlog_info("当前设备型号: %s", m_strModelName.c_str());
}

CConfigManager::~CConfigManager()
{
}

/**
* @brief   : 初始化所有配置
*/
int CConfigManager::init()
{
    /* 初始化视频配置 */
    // if (initVideoConfig() != 0)
    // {
    //     dlog_error("视频配置模块初始化失败!");
    //     return ERR;
    // }

    /* 初始化系统配置 */
    if (initSystemConfig() != 0)
    {
        dlog_error("系统配置模块初始化失败!");
        return ERR;
    }

    dlog_info("所有模块的配置初始化成功!");
    return OK;
}

int CConfigManager::initVideoConfig()
{
    /* 创建视频配置策略 */
    auto videoStrategy = CStrategyFactory::createVideoStrategy(m_strModelName);
    if (!videoStrategy)
    {
        dlog_error("创建视频配置策略失败!");
        return ERR;
    }

    /* 创建视频配置模块，并注入策略 */
    m_pVideoConfigModule = std::make_unique<CConfigModule<VecVideoConfig>>(VIDEO_CONFIG_FILE, std::move(videoStrategy));

    /* 初始化模块 */
    return m_pVideoConfigModule->init();
}

int CConfigManager::initSystemConfig()
{
    /* 创建设备信息策略 */
    auto videoStrategy = CStrategyFactory::createDeviceInfoStrategy(m_strModelName);
    if (!videoStrategy)
    {
        dlog_error("创建设备信息策略失败!");
        return ERR;
    }

    /* 创建设备信息模块，并注入策略 */
    m_pDeviceInfoConfigModule = std::make_unique<CConfigModule<System::DeviceInfo_S>>(DEVICE_INFO_CONFIG_FILE, std::move(videoStrategy));

    /* 初始化模块 */
    return m_pDeviceInfoConfigModule->init();
}
