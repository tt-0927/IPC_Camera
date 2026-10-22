#ifndef __SHARE_ICONV_H__
#define __SHARE_ICONV_H__
#include "iconv.h"



typedef struct __SHARE_CODE_CONVERT__
{
	char fromCharSet[256];
	char toCharSet[256];
	char inBuf[256];
	char outBuf[256];
	int inLen;
	int outLen;
}codeConvert_t;


//int share_code_convert(codeConvert_t* convertInfo,char *outBuf,char *inBuf);
/* 此函数转码纯英文数字会导致程序崩溃 */
int share_code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen);

int share_iconv_convert(char *from_charset, char *to_charset,char  * pcFrom, char * pcTo, int iMaxToLen);
#endif
