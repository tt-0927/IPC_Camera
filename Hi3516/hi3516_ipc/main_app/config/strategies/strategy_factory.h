/**
 * @FilePath     : strategy_factory.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 11:21:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-27 11:07:21
 * @Description  : 策略工厂
 */

#pragma once

#include <string>
#include <memory>
#include "config_strategy.h"
#include "video_strategies.h"
#include "system_strategies.h"

/**
* @brief   : 策略工厂
* @note    : 根据输入的产品型号，创建并返回相应的配置策略实例
*/
class CStrategyFactory
{
public:
    /**
    * @brief   : 创建视频配置策略
    * @param   {const std::string&} strModelName : 产品型号名称
    * @return  {std::unique_ptr<CConfigStrategy<VecVideoConfig>>} 
    *          一个指向具体视频策略的智能指针
    */
    static std::unique_ptr<CConfigStrategy<VecVideoConfig>> createVideoStrategy(const std::string& strModelName)
    {
        if (strModelName == "TV-3852T" || strModelName == "TV-3852H" ||
            strModelName == "TV-3852TL" || strModelName == "TV-3852HL" ||
            strModelName == "TV-3852TL4G" || strModelName == "TV-3852TLW")
        {
            return std::make_unique<CVideoStrategy_TV_3852T>();
        }
        else
        {
            dlog_warn("未知的设备型号 [%s], 使用默认视频策略。", strModelName.c_str());
            return std::make_unique<CVideoStrategyDefault>();
        }
    }

    /**
    * @brief   : 创建设备信息配置策略
    * @param   {const std::string&} strModelName : 产品型号名称
    * @return  {std::unique_ptr<CConfigStrategy<DeviceInfo_S>>} 
    *          一个指向具体设备信息策略的智能指针
    */
    static std::unique_ptr<CConfigStrategy<System::DeviceInfo_S>> createDeviceInfoStrategy(const std::string& strModelName)
    {
        if (strModelName == "TV-3852T")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3852T>();
        }
        else if (strModelName == "TV-3852H")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3852H>();
        }
        else if (strModelName == "TV-3852TL")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3852TL>();
        }
        else if (strModelName == "TV-3852HL")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3852HL>();
        }
        else if (strModelName == "TV-3852TL4G")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3852TL4G>();
        }
        else if (strModelName == "TV-3852TLW")
        {
            return std::make_unique<CDeviceInfoStrategy_TV_3852TLW>();
        }
        else
        {
            dlog_warn("未知的设备型号 [%s], 使用默认设备信息策略。", strModelName.c_str());
            return std::make_unique<CDeviceInfoStrategyDefault>();
        }
    }

};
