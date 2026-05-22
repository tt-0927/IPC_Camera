/**
 * @FilePath     : mqtt_topic_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-21 10:39:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-21 17:33:50
 * @Description  : MQTT Topic 宏定义
 */

#pragma once

#include <string>

/* MQTT 默认端口 */
#define MQTT_DEFAULT_PORT   1883

/* MQTT QoS 级别定义 */
#define MQTT_QOS_COMMAND    1   /* 命令下发：至少一次 */
#define MQTT_QOS_EVENT      0   /* 事件推送：最多一次 */
#define MQTT_QOS_RESPONSE   1   /* 命令响应：至少一次 */

/* MQTT Topic 前缀 */
#define MQTT_TOPIC_PREFIX(sn)       (std::string("device/") + (sn))

/* 平台 → 设备：命令下发 */
#define MQTT_TOPIC_COMMAND(sn)      (MQTT_TOPIC_PREFIX(sn) + "/command")

/* 设备 → 平台：事件上报 */
#define MQTT_TOPIC_EVENT(sn)        (MQTT_TOPIC_PREFIX(sn) + "/event")

/* 设备 → 平台：命令响应 */
#define MQTT_TOPIC_RESPONSE(sn)     (MQTT_TOPIC_PREFIX(sn) + "/response")
