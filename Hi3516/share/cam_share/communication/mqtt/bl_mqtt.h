/*
 * @Author       : EasonLu
 * @Date         : 2024-03-12 16:54:30
 * @LastEditors  : EasonLu
 * @LastEditTime : 2024-03-13 16:51:45
 * @FilePath     : bl_mqtt.h
 * @Description  : mqtt接口
 */
#ifndef __BL_SHARE_MQTT_H_
#define __BL_SHARE_MQTT_H_

#include "MQTTAsync.h"

typedef struct bl_mqtt BlMqtt_S;

typedef enum _BL_MQTT_MSG_TYPE_E_
{
    BL_MQTT_MSG_TOPIC = 0,
    BL_MQTT_MSG_CONNECT_SUCCESS = 1,
    BL_MQTT_MSG_CONNECT_FAILURE = 2,
    BL_MQTT_MSG_SUBSCRIBE_SUCCESS = 3,
    BL_MQTT_MSG_SUBSCRIBE_FAILURE = 4,
    BL_MQTT_MSG_DISCONNECT_SUCCESS = 5,
    BL_MQTT_MSG_DISCONNECT_FAILURE = 6,
    BL_MQTT_MSG_PUBLISH_SUCCESS = 7,
    BL_MQTT_MSG_PUBLISH_FAILURE = 8,
} BlMqttMsg_E;

/* mqtt消息数据结构 */
typedef struct bl_mqtt_msg
{
    BlMqttMsg_E enMsgType;
    char *pTopicName;
    int nTopicLen;
    char *pMsg;
    int nMsgLen;
} BlMqttMsg_S;

/* 订阅主题的信息回调（收到订阅主题发送的信息时，会调用此回调） */
typedef int (*fnCallback)(BlMqttMsg_S stMsg);

/* 必要参数 */
typedef struct bl_mqtt_need_param
{
    /* 信息回调函数 */
    fnCallback pfnCallback;
} BlMqttNeedParam_S;

/* 额外参数 */
typedef struct bl_mqtt_ex_param
{
    /* 当前连接唯一标识（默认为mac地址） */
    char achClientID[64];
    /* 连接mqtt的url（IP地址或域名，设置域名时需本地能解析，否则无法连接）（默认为ITC的调试公网:oam.itc-pa.cn ） */
    char achURL[1024];
    /* 连接mqtt的端口号（默认为1883） */
    int nPort;
    /* 连接mqtt的用户名（默认为ychjasp） */
    char achUserName[32];
    /* 连接mqtt的密码（默认为821006） */
    char achPassword[32];
    /* 连接心跳时间（默认：20秒） */
    unsigned int unKeepAlive;
    /* 连接超时时间（默认：30秒） */
    unsigned int unConnectTimeout;
    /* 自动重连（默认：开启） */
    int bAutoReconnect;

    /* ===== LWT 遗嘱配置（可选，全零表示不启用 LWT） ===== */
    /* LWT Topic，空字符串表示不启用 LWT */
    char achWillTopic[256];
    /* LWT 消息内容（JSON 字符串） */
    char achWillMessage[512];
    /* LWT QoS 等级（0/1/2），默认 1 */
    int  nWillQos;
    /* LWT 是否 retain，默认 1 */
    int  bWillRetain;
} BlMqttExParam_S;

/* MQTT服务器配置信息，默认配置ITC的运营平台 */
struct bl_mqtt
{
    /******************** 功能 ********************/

    /**
     * @brief  发布消息到指定Topic
     * @param  [BlMqtt_S] *pHandle - mqtt句柄
     * @param  [char] *pTopicName - Topic名称
     * @param  [char] *pMsg - 消息内容
     * @param  [int] nMsgLen - 消息长度
     * @param  [int] nQos - 消息质量(0,1,2，一般为1)
     * @return [*]
     * @author EasonLu
     * @note
     */
    int (*publish)(BlMqtt_S *pHandle, char *pTopicName, char *pMsg, int nMsgLen, int nQos);

    /**
     * @brief  订阅Topic
     * @param  [BlMqtt_S] *pHandle - mqtt句柄
     * @param  [char] *pTopicName - Topic名称
     * @param  [int] nQos - 消息质量(0,1,2，一般为1)
     * @return [*]
     * @author EasonLu
     * @note   订阅必须要等待连接完成才能进行订阅，否则订阅失败，可查看BlMqtt_S的bConnected标志位
     */
    int (*subscribe)(BlMqtt_S *pHandle, char *pTopicName, int nQos);

    /**
     * @brief  初始化
     * @param  [BlMqtt_S] *pHandle - mqtt句柄
     * @return [*]
     * @author EasonLu
     * @note   建立TCP连接耗时较长
     */
    int (*init)(BlMqtt_S *pHandle);

    /**
     * @brief  反初始化
     * @param  [BlMqtt_S] *pHandle
     * @return [*]
     * @author EasonLu
     * @note
     */
    int (*uninit)(BlMqtt_S *pHandle);
    /******************** 属性 ********************/
    /* 必要参数 */
    BlMqttNeedParam_S stNeedParam;
    /* 额外参数 */
    BlMqttExParam_S stExParam;

    /* 连接成功标记位 */
    int bConnected;

    /* 第三方库数据结构 */
    MQTTAsync stClient;
};

/**
 * @brief  分配mqtt句柄
 * @param  [BlMqttNeedParam_S] *pstNeedParam - mqtt必要配置信息
 * @param  [BlMqttExParam_S] *pstExParam - mqtt额外配置信息
 * @return [*]
 * @author EasonLu
 * @note   配置信息为空时，使用默认配置
 */
BlMqtt_S *bl_mqtt_alloc(BlMqttNeedParam_S *pstNeedParam,
                        BlMqttExParam_S *pstExParam);

/**
 * @brief  释放mqtt句柄
 * @param  [BlMqtt_S] *pstMqtt
 * @return [*]
 * @author EasonLu
 * @note
 */
int bl_mqtt_release(BlMqtt_S *pstMqtt);

#endif // __BL_SHARE_MQTT_H_
