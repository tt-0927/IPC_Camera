/**
 * @FilePath     : system_strategies.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 17:09:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 11:16:28
 * @Description  : 系统配置策略
 */

#pragma once

#include "config_strategy.h"
#include "system_define.h"
#include "share_define.h"
#include "system_manage.h"
#include "plugin_version_utils.h"
#include "dlog.h"

/**
* @brief   : 默认/未知型号的设备信息策略
* @note    : 当产品型号未知或不匹配时，保底配置
*/
class CDeviceInfoStrategyDefault : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充保底的默认设备信息...");

        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("无特殊定制化。");
    }
};

/**
* @brief   : 产品型号TV_3852T的设备信息策略
* @note    : 型号TV_3852T
*/
class CDeviceInfoStrategy_TV_3852T : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充型号TV_3852T的默认设备信息...");
        
        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("定制化型号TV_3852T的设备信息...");
    }
};

/**
* @brief   : 产品型号TV_3852H的设备信息策略
* @note    : 型号TV_3852H
*/
class CDeviceInfoStrategy_TV_3852H : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充型号TV_3852H的默认设备信息...");

        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("定制化型号TV_3852H的设备信息...");
    }
};

/**
* @brief   : 产品型号TV_3852TL的设备信息策略（智能垃圾站筒型海螺型有线）
* @note    : 型号TV_3852TL
*/
class CDeviceInfoStrategy_TV_3852TL : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充型号TV_3852TL的默认设备信息...");

        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("定制化型号TV_3852TL的设备信息...");
    }
};

/**
* @brief   : 产品型号TV_3852HL的设备信息策略（智能垃圾站海螺型有线）
* @note    : 型号TV_3852HL
*/
class CDeviceInfoStrategy_TV_3852HL : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充型号TV_3852HL的默认设备信息...");

        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("定制化型号TV_3852HL的设备信息...");
    }
};

/**
* @brief   : 产品型号TV_3852TL4G的设备信息策略（智能垃圾站筒型海螺型4G）
* @note    : 型号TV_3852TL4G
*/
class CDeviceInfoStrategy_TV_3852TL4G : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充型号TV_3852TL4G的默认设备信息...");

        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("定制化型号TV_3852TL4G的设备信息...");
    }
};

/**
* @brief   : 产品型号TV_3852TLW的设备信息策略（智能垃圾站筒型海螺型WIFI）
* @note    : 型号TV_3852TLW
*/
class CDeviceInfoStrategy_TV_3852TLW : public CConfigStrategy<System::DeviceInfo_S>
{
public:
    void fillDefault(System::DeviceInfo_S &config) override
    {
        dlog_info("填充型号TV_3852TLW的默认设备信息...");

        /* 可编辑的设备信息 */
        config.deviceName = DEVICE_NAME; /* 设备名称 */
        config.deviceID = 1;             /* 设备编号 */
        /* 设备固定属性 */
        config.strUnitTpye = DEVICE_CODE;                                       /* 设备型号 */
        config.serialNumber = SystemManage::instance()->get_cpu_serialNumber(); /* 设备序列号 */
        config.hardwareVersion = HARDWARE_VERSION;                              /* 硬件版本 */
        config.systemVersion = SYSTEM_VERSION;                                  /* 系统版本 */
        config.pluginVersion = PluginVersionUtils_NS::get_active_version(); /* 以板端当前生效插件文件为准，禁止使用编译期版本。 */
        config.webVersion = WEB_VERSION;                                        /* web版本 */
        /* 设备配置信息 */
        config.nAlarmInputCount = GPIO_INPUT_COUNT; /* 报警输入个数 */
        config.nAlarmOutputCount = GPIO_OUTPUT_COUNT; /* 报警输出个数 */
    }

    void customize(System::DeviceInfo_S &config) override
    {
        dlog_info("定制化型号TV_3852TLW的设备信息...");
    }
};
