/*
*  File Name        		: logcintrol.h
*  Created on       		: 2021-09-06
*  Author          	 	: yanzehui
*  description      		: 相关控制
*  Modify date      	: 2021-09-06
*  Modifier Author  	: yanzehui
*  description     	 	: 用于详细说明此程序文件完成的主要功能
*/
#ifndef _OS_SHARE_LOG_CONTROL_INCLUDE_
#define _OS_SHARE_LOG_CONTROL_INCLUDE_

#include "cJSON.h"

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifdef __cplusplus
extern "C"{
#endif
/*
* @description: 	获取目录下的log文件
* @param[in]:  		pIPpath: http的下载地址，除了下载的文件名
* @param[in]:  		pLogPath: log文件的目录
* @param[in]:  		pMessage: 请求数据
* @param[out]:  	pJsonData: 以json数据的形式返回
* @return:			FALSE-失败  TRUE-成功
* @others 			注意：pJsonData，不用的时候要释放。
*/
BOOL getLogFiles(char *pIPpath,char *pLogPath,char *pMessage,char **pJsonData);

#ifdef __cplusplus
}
#endif

#endif //_OS_SHARE_LOG_CONTROL_INCLUDE_
