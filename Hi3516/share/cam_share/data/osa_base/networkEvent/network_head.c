
/*
 * 文件名：network_head.c
 * 作者：zhangjunbin
 * 用途：网络收发的头信息处理
 *
 * */

#include "network_head.h"


int networkHead_standard_init(void* handle, void *pHeadBuf,\
		int nBodySize,int nCode,void *user)
{
	if(pHeadBuf == NULL)
	{
		printf("this argument is null!!");
		return -1;
	}

	NetworkHeadStandard_S* pHeader = \
			(NetworkHeadStandard_S*)pHeadBuf;
	pHeader->u16CompanyName[0] = NET_CHECK_STANDARD_FIRST_BIT;
	pHeader->u16CompanyName[1] = NET_CHECK_STANDARD_SECOND_BIT;
	pHeader->u16DataLen = nBodySize;
	pHeader->u16ExpansionLen = 0;
	pHeader->u16ProjectCode = 0x5508;	/* 项目代号 */

	return 0;
}

int networkHead_standard_check(void* handle,void *pHeadBuf,\
		int *pnBodySize,int *pnCode,void *user)
{
	NetworkHeadStandard_S* pHeader = \
			(NetworkHeadStandard_S*)pHeadBuf;

	if ((pHeader == NULL) || \
		(pHeader->u16CompanyName[0] != \
					NET_CHECK_STANDARD_FIRST_BIT)	|| \
		(pHeader->u16CompanyName[1] != \
					NET_CHECK_STANDARD_SECOND_BIT))
	{
		printf("this compang name is error!!!\n");
		return -1;
	}
	*pnBodySize = pHeader->u16DataLen;
	if(*pnBodySize <= 0)
	{
		printf("this body size <= 0 is error!!!\n");
		return -1;
	}
	/* 标准协议，把命令码放到了body里面，在上层应用解析 */
	*pnCode = SDK_STANDARD_CODE;
	return 0;
}















