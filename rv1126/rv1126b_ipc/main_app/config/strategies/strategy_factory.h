/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-12-29 10:11:46
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-04-24 10:12:15
 * @FilePath: /RV1126B/rv1126b_ipc/main_app/config/strategies/strategy_factory.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @FilePath     : strategy_factory.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 11:21:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-11 11:05:59
 * @Description  : 策略工厂
 */

#pragma once

#include <string>
#include <memory>
#include "config_strategy.h"
#include "system_strategies.h"

/**
* @brief   : 策略工厂
* @note    : 根据输入的产品型号，创建并返回相应的配置策略实例
*/
class CStrategyFactory
{
public:
    /**
    * @brief   : 创建设备信息配置策略
    * @param   {const std::string&} strModelName : 产品型号名称
    * @return  {std::unique_ptr<CConfigStrategy<DeviceInfo_S>>} 
    *          一个指向具体设备信息策略的智能指针
    */
    static std::unique_ptr<CConfigStrategy<System::DeviceInfo_S>> createDeviceInfoStrategy(const std::string& strModelName)
    {
        if (strModelName == "TV-3882TI")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3882TI>();
        }
        else if (strModelName == "TV-3881T")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3881T>();
        }
        else
        {
            dlog_warn("未知的设备型号 [%s], 使用默认设备信息策略。", strModelName.c_str());
            return std::make_unique<CDeviceInfoStrategyDefault>();
        }
    }

};
