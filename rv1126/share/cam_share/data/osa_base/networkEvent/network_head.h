
/*
 * 文件名：network_head.h
 * 作者：zhangjunbin
 * 用途：网络收发的头信息处理
 *
 * */

#ifndef _CORE_SOURCE_NETWORK_SOCKET_HEAD_INCLUDE_
#define _CORE_SOURCE_NETWORK_SOCKET_HEAD_INCLUDE_

#include "stdio.h"

#define NET_CHECK_STANDARD_FIRST_BIT ('B')
#define NET_CHECK_STANDARD_SECOND_BIT ('L')

/* 返回的code */
#define SDK_STANDARD_CODE 0xffffff;

//强制1字节对齐
#pragma pack(1)
typedef struct networkHeadStandard
{
	//公司名称BL
	char u16CompanyName[2];
	//项目代号
	unsigned short u16ProjectCode;
	//数据长度
	unsigned short u16DataLen;
	//扩展字段，代表头要扩展几个字节，1代表可扩展1字节，最多可扩展256，不用填0即可
	unsigned char u16ExpansionLen;
}NetworkHeadStandard_S;
#pragma pack()


int networkHead_standard_init(void* handle, void *pHeadBuf,\
		int nBodySize,int nCode,void *user);

int networkHead_standard_check(void* handle,void *pHeadBuf,\
		int *pnBodySize,int *pnCode,void *user);




#endif //_CORE_SOURCE_NETWORK_SOCKET_HEAD_INCLUDE_


