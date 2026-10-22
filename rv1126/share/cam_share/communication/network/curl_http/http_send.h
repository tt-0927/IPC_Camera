/*** 
 * @FilePath     : http_send.h
 * @Author       : huangjunda
 * @Date         : 2025-05-26 15:13:52
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-15 19:27:03
 * @Description  : 
 */
/*
 * @Author       : suzhl
 * @Date         : 2024-08-08 15:25:17
 * @LastEditors: lianghaoyao 709692194@qq.com
 * @LastEditTime: 2025-02-07 15:33:33
 * @FilePath     : http_send.h
 * @Description  : http发送
 */

#include "cJSON.h"
#include "dlog.h"
#include "curl.h"
#include <stdlib.h>
#include <string.h>

//用于访问http接口
int http_send(const char *url, const int type,const char *pToken, const char *input, char **output);

