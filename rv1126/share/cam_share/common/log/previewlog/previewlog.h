/*
*  File Name        : previewlog.h
*  Created on       : 2021-12-25
*  Author           : EasonLu
*  description      : MDS预览日志时调用的日志记录按行读取操作
*  Modify date      : 2021-12-25
*  Modifier Author  : EasonLu
*  description      : 用于详细说明此程序文件完成的主要功能
*/
#ifndef PREVIEWLOG_H
#define PREVIEWLOG_H

#include <stdio.h>
#include <stdlib.h>
#include "return_code.h"
#include "dlog.h"
#include "network_event.h"

#define READ_PAGE_SIZE 900

#define PIS_LOG_DIR ("/opt/course/log/")

typedef struct{
    int nBeginSize;
    int nEndSize;
}RowBuffer_S;

#ifdef __cplusplus
extern "C"
{
#endif

/*
* @description:读取对应路径日志，从指定字节数位置起，读取完整的多少行
* @param[in]:pRow-段落字节数结构体指针
* @param[in]:pLogPath-日志路径字符串指针
* @param[in]:nLine-裁剪的行数
* @return:无
* @others:其他说明
*/
RETURN_STATUS_E readRowBufferSize(RowBuffer_S *pRow, const char *pLogPath,int nLine);

/*
* @description:读取对应路径日志中指定区间的数据
* @param[in]:pRow-段落字节数结构体指针
* @param[in]:pLogPath-日志路径字符串指针
* @param[in]:pData-存放读取的日志数据
* @return:无
* @others:其他说明
*/
RETURN_STATUS_E getBufferSizeLog(RowBuffer_S *pRow, const char *pLogPath,char *pData);

/*
* @description:响应MDS的日志请求
* @param[in]:pCallBack-回调报文数据
* @param[in]:pJsonBuf-接收处理的日志数据
* @return:无
* @others:其他说明
*/
RETURN_STATUS_E getDataFromCallback(char *pCallBack,char **pJsonBuf);

/*
* @description:计算行数
* @param[in]:pData-需要计算的数据
* @param[in]:nLength-数据的长度
* @return:无
* @others:其他说明
*/
int countCR(char *pData,int nLength);

/*
* @description: 响应MDS的日志请求，响应SHARE_CMD_RESPONSE_MDS_GET_LOG指令
* @param[in]:pCallBack-播控传的报文
* @param[in]:pClinetHandle-与主播控通讯的句柄
* @param[in]:pClinetHandle-与备播控通讯的句柄,默认没有
* @return:无
* @others:其他说明
*/
RETURN_STATUS_E responseMDSLogRequest(char *pCallBack,\
                                      network_Handle_t pClinetHandle,\
                                      network_Handle_t pClinetHandleBackup);

#ifdef __cplusplus
}
#endif

#endif // PREVIEWLOG_H
