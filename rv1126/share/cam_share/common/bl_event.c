/*
 * @Author       : EasonLu
 * @Date         : 2024-03-20 15:25:17
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-14 16:02:57
 * @FilePath     : bl_event.c
 * @Description  : 事件接口
 */
#include "bl_event.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* 运维平台的型号，由对接文档提供 */
#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME "TE-8524T"
#endif

int bl_event_encode_msgCode(
    MqttLogType_E enType,
    MqttLogLevel_E enLevel,
    int *pOutCode)
{
    if (pOutCode == NULL)
    {
        return -1;
    }
    *pOutCode = BL_OPERATION_CUSTOM;
    *pOutCode += enType * 10;
    *pOutCode += enLevel;
    return 0;
}

int bl_event_decode_msgCode(
    int nCode,
    MqttLogType_E *pOutType,
    MqttLogLevel_E *pOutLevel)
{
    if (pOutType == NULL || pOutLevel == NULL)
    {
        return -1;
    }
    *pOutType = (nCode / 10) % 10;
    *pOutLevel = nCode % 10;
    return 0;
}

int bl_mqtt_enc_msgCode(
    MqttMsg_S stMsg,
    int *pOutCode)
{
    if (pOutCode == NULL)
    {
        return -1;
    }
    *pOutCode = BL_OPERATION_CUSTOM;
    *pOutCode += stMsg.enSource * 100;
    *pOutCode += stMsg.enType * 10;
    *pOutCode += stMsg.enLevel;
    return 0;
}

int bl_mqtt_dec_msgCode(int nCode, MqttMsg_S *pGetMsg)
{
    if (pGetMsg == NULL)
    {
        return -1;
    }
    nCode -= BL_OPERATION_CUSTOM;
    pGetMsg->enLevel = nCode % 10;
    pGetMsg->enType = (nCode / 10) % 10;
    pGetMsg->enSource = (nCode / 100) % 100; /* 有可能占两位 */
    return 0;
}