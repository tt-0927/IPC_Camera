/**
 * @file mqtt_communicate.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-05-15
 *
 * @brief 与运维平台mqtt通讯
 */
#pragma once
#include "path_define.h"
#include "system_define.h"
#include "cJSON.h"
#include "bl_event.h"

extern "C"
{
#include "bl_mqtt.h"
}

/**
 * @brief mqtt初始化
 * @return int
 */
int mqtt_init(System::LogServerInfo_S stLogServerInfo);
/**
 * @brief mqtt反初始化
 * @param stMqtt mqtt句柄
 * @return int
 */
int mqtt_deinit();
/**
 * @brief 推送mqtt消息
 * @param stMqtt mqtt句柄
 * @param stMsg mqtt信息结构体
 * @return int
 */
int mqtt_publish(MqttMsg_S stMsg);
/**
 * @brief mqtt回调
 * @param stMsg mqtt信息结构体
 * @return int
 */
int mqtt_callback(BlMqttMsg_S stMsg);
/**
 * @brief 加载配置信息
 * @return int
 */
int mqtt_load_config();
/**
 * @brief mqtt测试
 * @return int
 */
int mqtt_test(System::LogServerInfo_S stLogServerInfo);
