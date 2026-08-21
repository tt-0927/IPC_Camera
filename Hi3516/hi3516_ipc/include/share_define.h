/**
 * @FilePath     : share_define.h
 * @Author       : zhouzirui
 * @Date         : 2024-08-21 15:52:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-19 16:42:54
 * @Description  : 配置宏定义
 */
#ifndef _SHARE_DEFINE_H_
#define _SHARE_DEFINE_H_

#include "data_length.h"

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

/* 设备名称 */
#define DEVICE_NAME "Camera"

/* 不同设备类型信息：按型号维护完整设备画像
* 说明：
* 1. 每个型号分支必须显式定义完整的 DEVICE_PROFILE_* 字段，避免默认值掩盖缺失配置。
* 2. 当前 DEVICE_PROFILE_UPGRADE_ID / DEVICE_PROFILE_MQTT_PROJECT_ID /
*    DEVICE_PROFILE_MQTT_CODE 先统一使用占位值，后续可按型号分别替换成真实值。
* 3. 分支外只导出公共宏，业务代码统一通过 DEVICE_CODE / UPGRADE_ID /
*    MQTT_PROJECT_ID / MQTT_CODE / HARDWARE_VERSION / SYSTEM_VERSION 访问。
*/
#if defined(DEVICE_TV_3852HL)
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852HL"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "CQqKHCVoET6goksZZ4UBOBdXJs5MkmQb"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.0.5"
#elif defined(DEVICE_TV_3852TLW)
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852TLW"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "CQqKHCVoET6goksZZ4UBOBdXJs5MkmQb"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.0.5"
#elif defined(DEVICE_TV_3852TL4G)
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852TL4G"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "CQqKHCVoET6goksZZ4UBOBdXJs5MkmQb"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.0.5"
#elif defined(DEVICE_TV_3852TL)
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852TL"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "CQqKHCVoET6goksZZ4UBOBdXJs5MkmQb"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.0.5"
#elif defined(DEVICE_TV_3852H) || defined(DEVICE_TV_3852HZT)
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852H"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "McEUTTHyiAYYYKLjM2OQ2yZsF0QGGVIx"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.1.5"
#elif defined(DEVICE_TV_3852T)
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852T"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "CQqKHCVoET6goksZZ4UBOBdXJs5MkmQb"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.0.7"
#else
/* 未指定型号时，默认按 TV-3852T 兜底，保证公共设备画像字段完整 */
/* 设备型号 */
#define DEVICE_PROFILE_CODE             "TV-3852T"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define DEVICE_PROFILE_UPGRADE_ID       ("3852")
/* 设备运维管理平台-设备编号 */
#define DEVICE_PROFILE_MQTT_PROJECT_ID  978
/* 设备运维管理平台-设备唯一code码 */
#define DEVICE_PROFILE_MQTT_CODE        "CQqKHCVoET6goksZZ4UBOBdXJs5MkmQb"
/* 硬件版本 */
#define DEVICE_PROFILE_HARDWARE_VERSION "V1.0.0"
/* 系统版本 */
#define DEVICE_PROFILE_SYSTEM_VERSION   "V1.0.0"
#endif

/* 设备型号 */
#define DEVICE_CODE     DEVICE_PROFILE_CODE
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define UPGRADE_ID      DEVICE_PROFILE_UPGRADE_ID
/* 设备运维管理平台-设备编号 */
#define MQTT_PROJECT_ID DEVICE_PROFILE_MQTT_PROJECT_ID
/* 设备运维管理平台-设备唯一code码 */
#define MQTT_CODE       DEVICE_PROFILE_MQTT_CODE
/* 设备运维管理平台-设备型号（所有型号通用） */
#define MQTT_MODEL      DEVICE_CODE

/* 软件项目类型 */
#ifdef PROJECT_TYPE_ITC
    #define PROJECT_TYPE "itc"
#elif defined(PROJECT_TYPE_ZHONGXING)
    #define PROJECT_TYPE "中性"
#else
    #define PROJECT_TYPE "itc"
#endif

/* 厂商名称 */
#define MANUFACTURER_NAME "itc"

/* MQTT 设备运维管理平台 */
#define MQTT_COMPANY         "BL"         /* 公司名称 */
#define MQTT_DEVICE_NAME     DEVICE_CODE  /* 默认型号名称 */
#define MQTT_PRODUCT_NAME    "监控产品线" /* 产品线名称 */
#define MQTT_PRODUCT_ID      "94"         /* 产品线编号 */
#define MQTT_LOGIN           "shijue2bu"  /* 账号 */
#define MQTT_PASSWORD        "admin@123"  /* 密码 */

/* 软著版本号的前缀v1.0.0 */
#define PREFIX_VERSION       "V3.293"
/* 硬件版本：按型号定制 */
#define HARDWARE_VERSION     DEVICE_PROFILE_HARDWARE_VERSION
/* 系统版本：按型号定制 */
#define SYSTEM_VERSION       DEVICE_PROFILE_SYSTEM_VERSION
/* web版本 */
#define WEB_VERSION          "V1.1.3"

#if CAP_ALARM_IO
/* 报警输入GPIO个数 */
#define GPIO_OUTPUT_COUNT 1
/* 报警输出GPIO个数 */
#define GPIO_INPUT_COUNT  1
#else
/* 报警输入GPIO个数 */
#define GPIO_OUTPUT_COUNT 0
/* 报警输出GPIO个数 */
#define GPIO_INPUT_COUNT  0
#endif

#ifdef __cplusplus
}
#endif	// __cplusplus
#endif
