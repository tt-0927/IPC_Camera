/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-12-29 10:11:46
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-06-01 16:54:18
 * @FilePath: /RV1126B/rv1126b_ipc/include/share_define.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @FilePath     : share_define.h
 * @Author       : zhouzirui
 * @Date         : 2024-08-21 15:52:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-19 09:08:19
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

/* 不同设备类型信息 */
#ifdef DEVICE_TV_3882TI
/* 设备代码 */
#define DEVICE_CODE     "TV-3882TI"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define UPGRADE_ID      ("882")
/* 设备运维管理平台-设备编号 */
#define MQTT_PROJECT_ID 2062
/* 设备运维管理平台-设备型号 */
#define MQTT_MODEL      DEVICE_CODE
/* 设备运维管理平台-设备唯一code码 */
#define MQTT_CODE       "F4FU9aWf3ZxU47QYvatS9Yo4VBWWJlfF"
#elif DEVICE_TV_3881T
/* 设备代码 */
#define DEVICE_CODE     "TV-3881T"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define UPGRADE_ID      ("881")
/* 设备运维管理平台-设备编号 */
#define MQTT_PROJECT_ID 1722
/* 设备运维管理平台-设备型号 */
#define MQTT_MODEL      DEVICE_CODE
/* 设备运维管理平台-设备唯一code码 */
#define MQTT_CODE       "sGhe1x9c9CiaqH0xBa3pTQk0Vle5y0WN"
#else
/* 设备代码 */
#define DEVICE_CODE     "TV-3881T"
/* 升级包前缀校验ID，不能超过3个字符, 修改后需要重新编译所有程序，包括mkpack */
#define UPGRADE_ID      ("881")
/* 设备运维管理平台-设备编号 */
#define MQTT_PROJECT_ID 1722
/* 设备运维管理平台-设备型号 */
#define MQTT_MODEL      DEVICE_CODE
/* 设备运维管理平台-设备唯一code码 */
#define MQTT_CODE       "sGhe1x9c9CiaqH0xBa3pTQk0Vle5y0WN"
#endif

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

/* 公共定义版本号，确保每个程序的公共定义版本一致，否则进行二进制流传输时会出现段错误 */
#define SHARE_DEFINE_VERSION ("V1.0.6")
/* 软著版本号的前缀v1.0.0 */
#define PREFIX_VERSION       "V3.357"
/* 固件版本 */
#define FIRMWARE_VERSION     "V1.0.6"
/* 硬件版本 */
#define HARDWARE_VERSION     "V1.0.0"
/* 系统版本 */
#define SYSTEM_VERSION       "V1.0.6"
/* 插件版本 */
#define PLUG_VERSION         "V2.0.10"
/* web版本 */
#define WEB_VERSION          "V1.0.1"
/* 报警输入GPIO个数 */
#define GPIO_OUTPUT_COUNT 2
/* 报警输出GPIO个数 */
#define GPIO_INPUT_COUNT  2

#ifdef __cplusplus
}
#endif	// __cplusplus
#endif
