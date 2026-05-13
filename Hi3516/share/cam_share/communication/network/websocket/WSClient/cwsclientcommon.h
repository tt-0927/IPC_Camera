/*
 * @FilePath     : cwsclientcommon.h
 * @Author       : Xiezhh
 * @Date         : 2024-08-01
 * @Description  : CWSClient类的结构体定义
 */
#ifndef WSCLIENTCOMMON_H
#define WSCLIENTCOMMON_H

#include <stdio.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <list>

#include "WSBase.h"

/* 初始化参数 */
struct WSClientParame {
    /* WSBase的参数 */
    wss_NS::WebSocketParams_S stParames;

    /* token */
    std::string strToken;
    /* 是否启用SSL */
    bool bSSL = false;
    /* 是否使用HTTP Bearer鉴权 */
    bool bBearerAuth = false;
    /* 本地发送缓冲区的元素个数 */
    int nWriteRingBufElementCount = 8;
};

/* 发送消息结构体 */
struct WSMsg {
    /* 数据 */
    uint8_t* pBuffer;
    /* 数据大小 */
    size_t nLen; 
    /* 发送数据类型 */
    int enType;
};

/* 析构消息体的函数 */
static void deleteWSMsg(void *pMsg)
{
    if(pMsg)
    {
        WSMsg *pTmp = (WSMsg*)pMsg;
        if(pTmp->pBuffer)
        {
            free(pTmp->pBuffer);
            pTmp->pBuffer = NULL;
        }
        pTmp->nLen = 0;
    }
}

#endif //WSCLIENTCOMMON_H