/*
 * @Author       : EasonLu
 * @Date         : 2024-03-12 16:54:36
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-15 17:43:05
 * @FilePath     : bl_mqtt.c
 * @Description  : mqtt接口
 */
#include "bl_mqtt.h"
#include "dlog.h"
#include "edukit_network.h"
#include "os_thr.h"
#include <string.h>
#include <stdlib.h>

/* 默认mqtt配置信息 */
static const BlMqttExParam_S g_stDefaultMqtt = {
    .achClientID = "",
    .achURL = "oam.itc-pa.cn",
    .nPort = 1883,
    .achUserName = "ychjasp",
    .achPassword = "821006",
    .unKeepAlive = 20,
    .unConnectTimeout = 30,
    .bAutoReconnect = 1,
};

/**
 * @brief  接收消息回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [char] *pTopicName 订阅主题名称
 * @param  [int] nTopicLen 主题名称长度
 * @param  [MQTTAsync_message] *pMessage 消息结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static int bl_mqtt_recvMsg(
    void *pContext,
    char *pTopicName,
    int nTopicLen,
    MQTTAsync_message *pMessage)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle && pHandle->stNeedParam.pfnCallback)
    {
        BlMqttMsg_S stMsg = {
            .enMsgType = BL_MQTT_MSG_TOPIC,
            .pTopicName = pTopicName,
            .nTopicLen = nTopicLen,
            .pMsg = pMessage->payload,
            .nMsgLen = pMessage->payloadlen,
        };

        pHandle->stNeedParam.pfnCallback(stMsg);
    }
    /* 释放资源 */
    MQTTAsync_freeMessage(&pMessage);
    MQTTAsync_free(pTopicName);
    return 1;
}

/**
 * @brief  mqtt连接成功回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [MQTTAsync_successData] *pResponse 成功数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static void bl_mqtt_connSuccess(
    void *pContext,
    MQTTAsync_successData *pResponse)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle)
    {
        pHandle->bConnected = 1;
        if (pHandle->stNeedParam.pfnCallback)
        {
            BlMqttMsg_S stMsg = {
                .enMsgType = BL_MQTT_MSG_CONNECT_SUCCESS,
                .pTopicName = NULL,
                .nTopicLen = 0,
                .pMsg = NULL,
                .nMsgLen = 0,
            };
            pHandle->stNeedParam.pfnCallback(stMsg);
        }
    }
    dlog_debug("mqtt连接成功");
}

/**
 * @brief  mqtt连接失败回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [MQTTAsync_failureData] *pResponse 失败数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static void bl_mqtt_connFailure(
    void *pContext,
    MQTTAsync_failureData *pResponse)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle)
    {
        pHandle->bConnected = 0;
        if (pHandle->stNeedParam.pfnCallback)
        {
            BlMqttMsg_S stMsg = {
                .enMsgType = BL_MQTT_MSG_CONNECT_FAILURE,
                .pTopicName = NULL,
                .nTopicLen = 0,
                .pMsg = NULL,
                .nMsgLen = 0,
            };
            pHandle->stNeedParam.pfnCallback(stMsg);
        }
    }
    dlog_error("mqtt连接失败，错误码[%d] 错误原因：%s",
               pResponse->code, pResponse->message);
}

/**
 * @brief  mqtt订阅topic成功回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [MQTTAsync_successData] *pResponse 成功数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static void bl_mqtt_subsSuccess(
    void *pContext,
    MQTTAsync_successData *pResponse)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle)
    {
        if (pHandle->stNeedParam.pfnCallback)
        {
            BlMqttMsg_S stMsg = {
                .enMsgType = BL_MQTT_MSG_SUBSCRIBE_SUCCESS,
                .pTopicName = NULL,
                .nTopicLen = 0,
                .pMsg = NULL,
                .nMsgLen = 0,
            };
            pHandle->stNeedParam.pfnCallback(stMsg);
        }
    }
    dlog_debug("订阅主题成功");
}

/**
 * @brief  mqtt订阅Topic失败回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [MQTTAsync_failureData] *pResponse 失败数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static void bl_mqtt_subsFailure(
    void *pContext,
    MQTTAsync_failureData *pResponse)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle)
    {
        if (pHandle->stNeedParam.pfnCallback)
        {
            BlMqttMsg_S stMsg = {
                .enMsgType = BL_MQTT_MSG_SUBSCRIBE_FAILURE,
                .pTopicName = NULL,
                .nTopicLen = 0,
                .pMsg = NULL,
                .nMsgLen = 0,
            };
            pHandle->stNeedParam.pfnCallback(stMsg);
        }
    }
    dlog_error("订阅主题失败，错误码[%d] 错误原因：%s",
               pResponse->code, pResponse->message);
}

/**
 * @brief  mqtt发布信息到指定topic成功回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [MQTTAsync_successData] *pResponse 成功数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static void bl_mqtt_pubsSuccess(
    void *pContext,
    MQTTAsync_successData *pResponse)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle)
    {
        if (pHandle->stNeedParam.pfnCallback)
        {
            BlMqttMsg_S stMsg = {
                .enMsgType = BL_MQTT_MSG_PUBLISH_SUCCESS,
                .pTopicName = NULL,
                .nTopicLen = 0,
                .pMsg = NULL,
                .nMsgLen = 0,
            };
            pHandle->stNeedParam.pfnCallback(stMsg);
        }
    }
    dlog_debug("发布消息成功");
}

/**
 * @brief  mqtt发布信息到指定Topic失败回调函数
 * @param  [void] *pContext 用户自定义参数
 * @param  [MQTTAsync_failureData] *pResponse 失败数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static void bl_mqtt_pubsFailure(
    void *pContext,
    MQTTAsync_failureData *pResponse)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)pContext;
    if (pHandle)
    {
        if (pHandle->stNeedParam.pfnCallback)
        {
            BlMqttMsg_S stMsg = {
                .enMsgType = BL_MQTT_MSG_PUBLISH_FAILURE,
                .pTopicName = NULL,
                .nTopicLen = 0,
                .pMsg = NULL,
                .nMsgLen = 0,
            };
            pHandle->stNeedParam.pfnCallback(stMsg);
        }
    }
    dlog_error("发布消息失败，错误码[%d] 错误原因：%s",
               pResponse->code, pResponse->message);
}

static int bl_mqtt_subscribe(BlMqtt_S *pHandle, char *pTopic, int nQos)
{
    if (NULL == pHandle || NULL == pTopic)
    {
        return -1;
    }
    int nRet = 0;
    MQTTAsync_responseOptions stOpts = MQTTAsync_responseOptions_initializer;
    stOpts.onSuccess = bl_mqtt_subsSuccess;
    stOpts.onFailure = bl_mqtt_subsFailure;
    stOpts.context = pHandle;
    nRet = MQTTAsync_subscribe(pHandle->stClient, pTopic, nQos, &stOpts);
    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("订阅主题失败，错误码：%d", nRet);
    }
    return nRet;
}

static int bl_mqtt_publish(BlMqtt_S *pHandle, char *pTopicName, char *pMsg, int nMsgLen, int nQos)
{
    if (NULL == pHandle || NULL == pTopicName || NULL == pMsg)
    {
        return -1;
    }
    int nRet = 0;
    MQTTAsync_responseOptions stOpts = MQTTAsync_responseOptions_initializer;
    stOpts.context = pHandle;
    stOpts.onSuccess = bl_mqtt_pubsSuccess;
    stOpts.onFailure = bl_mqtt_pubsFailure;
    nRet = MQTTAsync_send(pHandle->stClient, pTopicName, nMsgLen, pMsg, nQos, 0, &stOpts);
    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("发布消息失败，错误码：%d", nRet);
    }
    return nRet;
}

static void connlost(void *context, char *cause)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)context;
    MQTTAsync client = (MQTTAsync)pHandle->stClient;

    dlog_debug("重新连接mqtt服务器");

    /* 设置默认的连接值(MQTT 3.1.1 的TCP连接) */
    MQTTAsync_connectOptions stConnOpts = MQTTAsync_connectOptions_initializer;

    /* 更新连接参数 */
    stConnOpts.keepAliveInterval = pHandle->stExParam.unKeepAlive;
    stConnOpts.connectTimeout = pHandle->stExParam.unConnectTimeout;
    stConnOpts.username = pHandle->stExParam.achUserName;
    stConnOpts.password = pHandle->stExParam.achPassword;
    stConnOpts.cleansession = 1;
    stConnOpts.onSuccess = bl_mqtt_connSuccess;
    stConnOpts.onFailure = bl_mqtt_connFailure;
    stConnOpts.context = pHandle;
    stConnOpts.automaticReconnect = true;

    /* 开始连接 */
    int nRet = MQTTAsync_connect(pHandle->stClient, &stConnOpts);
    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("mqtt连接失败，错误码：%d", nRet);
    }
}

static int bl_mqtt_init(BlMqtt_S *pHandle)
{
    if (NULL == pHandle)
    {
        return -1;
    }

    /* 拼接服务器的链接 */
    char achServerUrl[2048] = {0};
    snprintf(achServerUrl, sizeof(achServerUrl), "tcp://%s:%d",
             pHandle->stExParam.achURL, pHandle->stExParam.nPort);
    int nRet = 0;
    nRet = MQTTAsync_create(&pHandle->stClient, achServerUrl,
                            pHandle->stExParam.achClientID,
                            MQTTCLIENT_PERSISTENCE_NONE, NULL);

    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("创建mqtt客户端失败，错误码：%d", nRet);
        goto EXIT;
    }

    /* 设置回调函数 */
    nRet = MQTTAsync_setCallbacks(pHandle->stClient, pHandle, connlost,
                                  bl_mqtt_recvMsg, NULL);
    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("设置mqtt回调函数失败，错误码：%d", nRet);
        goto EXIT;
    }

    /* 设置默认的连接值(MQTT 3.1.1 的TCP连接) */
    MQTTAsync_connectOptions stConnOpts = MQTTAsync_connectOptions_initializer;

    /* 更新连接参数 */
    stConnOpts.keepAliveInterval = pHandle->stExParam.unKeepAlive;
    stConnOpts.connectTimeout = pHandle->stExParam.unConnectTimeout;
    stConnOpts.username = pHandle->stExParam.achUserName;
    stConnOpts.password = pHandle->stExParam.achPassword;
    stConnOpts.cleansession = 1;
    stConnOpts.onSuccess = bl_mqtt_connSuccess;
    stConnOpts.onFailure = bl_mqtt_connFailure;
    stConnOpts.context = pHandle;
    stConnOpts.automaticReconnect = true;

    dlog_debug("正在连接mqtt服务器[%s]", achServerUrl);
    /* 开始连接 */
    nRet = MQTTAsync_connect(pHandle->stClient, &stConnOpts);
    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("mqtt连接失败，错误码：%d", nRet);
        goto EXIT;
    }

    return 0;

EXIT:
    MQTTAsync_destroy(&pHandle->stClient);
    pHandle->bConnected = 0;
    return nRet;
}

static int bl_mqtt_uninit(BlMqtt_S *pHandle)
{
    if (NULL == pHandle)
    {
        return -1;
    }

    int nRet = 0;
    /* 设置默认的连接值(MQTT 3.1.1 的TCP连接) */
    MQTTAsync_disconnectOptions stDiscOpts = MQTTAsync_disconnectOptions_initializer;

    stDiscOpts.onSuccess = NULL;
    stDiscOpts.onFailure = NULL;
    stDiscOpts.context = pHandle;
    nRet = MQTTAsync_disconnect(pHandle->stClient, &stDiscOpts);
    if (MQTTASYNC_SUCCESS != nRet)
    {
        dlog_error("mqtt断开连接失败，错误码：%d", nRet);
    }
    /* NOTE 直接销毁，理论上需要等待断开连接成功后才能进行销毁 */
    MQTTAsync_destroy(&pHandle->stClient);
    pHandle->bConnected = 0;

    return nRet;
}

BlMqtt_S *bl_mqtt_alloc(BlMqttNeedParam_S *pstNeedParam, BlMqttExParam_S *pstExParam)
{
    BlMqtt_S *pHandle = (BlMqtt_S *)malloc(sizeof(BlMqtt_S));
    if (NULL == pHandle)
    {
        dlog_error("分配mqtt句柄失败");
        return NULL;
    }
    memset(pHandle, 0, sizeof(BlMqtt_S));

    if (pstNeedParam)
    {
        memcpy(&pHandle->stNeedParam, pstNeedParam, sizeof(BlMqttNeedParam_S));
    }

    BlMqttExParam_S *pTmpMqtt = pstExParam ? pstExParam : &g_stDefaultMqtt;
    memcpy(&pHandle->stExParam, pTmpMqtt, sizeof(BlMqttExParam_S));
    /* 判断是否填写ClientID */
    if (0 == strlen(pHandle->stExParam.achClientID))
    {
        /* 填入mac地址 */
        char achMac[18] = {0};
        if (0 != ReachMacAddr(ETH0_INTERFACE, achMac))
        {
            /* 获取eth0失败，再获取一遍eth1 */
            if (0 != ReachMacAddr(ETH1_INTERFACE, achMac))
            {
                /* 获取mac地址失败 */
                dlog_error("获取mac地址失败，mqtt初始化失败");
                return NULL;
            }
        }
        memcpy(pHandle->stExParam.achClientID, achMac, sizeof(achMac));
    }

    pHandle->stClient = NULL;

    /* 设置功能函数 */
    pHandle->init = bl_mqtt_init;
    pHandle->uninit = bl_mqtt_uninit;
    pHandle->subscribe = bl_mqtt_subscribe;
    pHandle->publish = bl_mqtt_publish;

    return pHandle;
}

int bl_mqtt_release(BlMqtt_S *pstMqtt)
{
    if (pstMqtt)
    {
        /* TODO 释放其他资源 */

        free(pstMqtt);
        pstMqtt = NULL;
    }
    return 0;
}
