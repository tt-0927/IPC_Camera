/*
 * @Description: 
 * @Version: 1.0
 * @Autor: fhs
 * @Date: 2022-11-05 10:12:30
 * @LastEditors: fhs
 * @LastEditTime: 2022-12-01 20:21:48
 */
#ifndef __SHARE_JSONBASE_H__
#define __SHARE_JSONBASE_H__

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
extern "C"
{
#endif

cJSON* json_init_head(void);
int json_delete_head(cJSON *jsonObjHead);
BOOL json_add_double(cJSON *jsonObj,char *nodeName,double addValue);
BOOL json_add_int(cJSON *jsonObj,char *nodeName,int addValue);
BOOL json_get_int(char *jsonItem,char *nodeName,int *Value);
BOOL json_get_int1(char *jsonItem,char *nodeName,char *nodeName1,int *Value);
BOOL json_add_char(cJSON *jsonObj,char *nodeName,char *string);
BOOL json_get_char(char *jsonItem,char *nodeName,char *Value,int nLen);

/*
* @description: 提取json数据中的字符串，第二层
* @param[in]: jsonItem: 数据内容
* @param[in]: nodeName: 第一层的键
* @param[in]: nodeName1: 第二层的键
* @param[out]: Value: 提取的值
* @return: 无
* @others
*/
BOOL json_get_char1(char *jsonItem,char *nodeName,char *nodeName1,char *Value,int nLen);
BOOL json_outOf_String(cJSON *jsonObj,char **outJson); //outJsons 输出为堆内存,需要调用free释放内存


BOOL json_GetObjectItem_int(cJSON *jsonObj,char *nodeName,int *nData);
BOOL json_GetObjectItem_string(cJSON *jsonObj,char *nodeName,char *pData,int nLen);

#ifdef __cplusplus
}
#endif

#endif
