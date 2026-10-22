/*
*  File Name        		: logcintrol.c
*  Created on       		: 2021-09-06
*  Author           		: yanzehui
*  description      		: 相关控制
*  Modify date      	: 2021-09-06
*  Modifier Author  	: yanzehui
*  description      : 用于详细说明此程序文件完成的主要功能
*/
#include "logcontrol.h"
#include "dlog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>


/* 获取目录下的log文件 */
BOOL getLogFiles(char *pIPpath,char *pLogPath,char *pMessage,char **pJsonData)
{
	if(pLogPath == NULL)
	{
		dlog(LOG_ERROR,"parameter error");
		return FALSE;
	}

	if(sizeof(pLogPath) > 1024)
	{
		dlog(LOG_ERROR,"pLogPath length is too long");
		return FALSE;
	}

	DIR *pDir = NULL;
	struct dirent *pPtr = NULL;

	/* 打开目录 */
	if ((pDir=opendir(pLogPath)) == NULL)
	{
		dlog(LOG_ERROR,"Open dir error...");
		return FALSE;
	}

	/* 先创建json空对象 */
	cJSON *pRoot = cJSON_CreateObject();
	if(pRoot == NULL)
	{
		dlog(LOG_ERROR,"cJSON_CreateObject fail");
		return FALSE;
	}


	/* 找到pMessage 中的保存下载的地址 */
	char aReceivePath[1024] = {0};
	if(json_get_char1(pMessage,"data","DownloadPath",aReceivePath,sizeof(aReceivePath)) == FALSE)
	{
		dlog(LOG_ERROR, "json_get_char() fail");
		return FALSE;
	}
	else
	{
		/* 添加数据 */
		cJSON_AddStringToObject(pRoot,"DownloadPath",aReceivePath);
	}

	/* 找到pMessage 中的保存Mac地址 */
	char aReceiveMac[64] = {0};
//	Network_data_head_S *pstHead = (Network_data_head_S *)pMessage->pHeadBuf;
//	sprintf(aReceiveMac,"%x:%x:%x:%x:%x:%x",pstHead->u8MACaddr[0]\
//																	,pstHead->u8MACaddr[1]\
//																	,pstHead->u8MACaddr[2]\
//																	,pstHead->u8MACaddr[3]\
//																	,pstHead->u8MACaddr[4]\
//																	,pstHead->u8MACaddr[5]);
//	/* 添加数据 */
//	cJSON_AddStringToObject(pRoot,"Mac",aReceiveMac);

	if(json_get_char1(pMessage,"data","FromMac",aReceiveMac,sizeof(aReceiveMac)) == FALSE)
	{
		dlog(LOG_ERROR, "json_get_char() fail");
		return FALSE;
	}
	else
	{
		/* 添加数据 */
		cJSON_AddStringToObject(pRoot,"ToMac",aReceiveMac);
	}

	/* 获取时间 */
	char aLogDate[64] = {0};
	json_get_char1(pMessage,"data","Date",aLogDate,sizeof(aLogDate));
	

	/* 添加数组 */
	cJSON *pArray = cJSON_CreateArray();
	if(pArray == NULL)
	{
		dlog(LOG_ERROR,"cJSON_CreateArray fail");
		cJSON_Delete(pRoot);
		return FALSE;
	}
	cJSON_AddItemToObject(pRoot,"logPaths",pArray);


	/* 设备类型 */
	char aDevType[12] = {0};
	json_get_char1(pMessage,"data","TypeName",aDevType,sizeof(aDevType));


	while ((pPtr=readdir(pDir)) != NULL)
	{
		/* 当前目录和上一级目录 */
		if(strcmp(pPtr->d_name,".")==0 || strcmp(pPtr->d_name,"..")==0)
		{
			continue;
		}
		/* 一般文件 */
		else if(pPtr->d_type & DT_REG)
		{
			if(strstr(pPtr->d_name,".log"))
			{
				/*获取到的日期为空，就不赛选日期了*/
				if(strlen(aLogDate) != 0)
				{
					if(strstr(pPtr->d_name,aLogDate) == NULL)
					{
						continue;
					}
				}
				
				if(strstr(pPtr->d_name,aDevType) == NULL)
				{
					continue;
				}
				/* 在数组上添加对象 */
				cJSON *pObj = NULL;
				pObj=cJSON_CreateObject();
				if(pObj == NULL)
				{
					dlog(LOG_ERROR,"cJSON_CreateObject fail");
					continue;
				}

				/* 拼接下载地址 */
				char aTemp[1024] = {0};
				sprintf(aTemp,"%s/%s",pIPpath,pPtr->d_name);

				/* 加入json数据 */
				cJSON_AddStringToObject(pObj,"path",aTemp);
				cJSON_AddItemToArray(pArray,pObj);
			}

		}
		/* 目录 */
		else if(pPtr->d_type & DT_DIR)
		{
//			memset(cPathBase,'\0',sizeof(cPathBase));
//			strncpy(cPathBase,pLogPath,sizeof(pLogPath));
//			strcat(cPathBase,"/");
//			strcat(cPathBase,pPtr->d_name);
			continue;
		}
	}

	*pJsonData = cJSON_Print(pRoot);

	/* 释放json空间 */
	if(pRoot)
	{
		cJSON_Delete(pRoot);
		pRoot = NULL;
	}

	closedir(pDir);
	return TRUE;
}
