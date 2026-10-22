
#ifndef _OS_CODE_PARSE_URL_H_
#define _OS_CODE_PARSE_URL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/*
 *	解析rtsp协议的地址
 *
 * */
int os_parsing_RTSPURL(char const* url, char* username, char* password, char* address,int* portNum, char* path);

#ifdef __cplusplus
}
#endif

#endif //_OS_CODE_PARSE_URL_H_



