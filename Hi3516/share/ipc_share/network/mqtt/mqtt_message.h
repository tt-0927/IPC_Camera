/**
 * @FilePath     : mqtt_message.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-21 10:39:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-21 17:33:26
 * @Description  : MQTT 消息结构体定义
 */

#pragma once

#include <string>
#include <functional>

/**
 * @brief MQTT 消息结构体
 * @note  复用 HTTP-SDK 命令格式，增加 RequestId 用于请求-响应关联
 */
struct MqttMessage_S
{
    std::string str_command;    /* 命令名，如 "NET_TV_GET_FACECAPTUREINFO" */
    std::string str_request_id; /* 请求唯一标识，UUID 格式 */
    std::string str_data;       /* JSON 格式业务数据 */
};

/* 命令处理器类型定义 */
using CommandHandler = std::function<void(const MqttMessage_S &)>;
