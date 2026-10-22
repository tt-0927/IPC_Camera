#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "iconv.h"
#include "share_iconv.h"


int share_code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);

	if((iconv_t) - 1 == cd) {
		perror("iconv_open error:");
		return -1;
	}

	memset(outbuf, 0, outlen);

	if(iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen) == (size_t) - 1) {
		//perror("iconv error: ");
		iconv_close(cd);
		return -1;
	}

	iconv_close(cd);
	return 0;
}


static int ChangeCode( const char* pFromCode, const char* pToCode, const char* pInBuf,
                 size_t* piInLen,  char* pOutBuf,  size_t* piOutLen )
{
    int iRet;
    char **pin = &pInBuf;
    char **pout = &pOutBuf;
    iconv_t hIconv;

    //printf("%s: outlen=%d, inlen=%d\n", __FUNCTION__, *piOutLen, *piInLen);

    //打开字符集转换
    hIconv = iconv_open( pToCode, pFromCode );
    if ( -1 == (int)hIconv )
    {
        perror("iconv_open");
        return -1;
    }
    //开始转换
   // printf("%s: 1 outlen=%d\n", __FUNCTION__, *piOutLen);
    iRet = iconv( hIconv, pin, piInLen, pout, piOutLen );
    if ( -1 == iRet )
    {
        //perror("iconv");
        iconv_close( hIconv );
        **pout = '\0';
        return -1;
    }
   //printf("%s: 2 outlen=%d\n", __FUNCTION__, *piOutLen);

    //关闭字符集转换
    iconv_close( hIconv );

    **pout = '\0';
    return iRet;
}

int share_iconv_convert(char *from_charset, char *to_charset,char  * pcFrom, char * pcTo, int iMaxToLen)
{
    //printf("share_iconv_convert %s\n",pcFrom);
    char * psInBuf = NULL;
    char * psOutBuf = NULL;
    size_t iInLen = 0;
    size_t iOutLen = 0;
    int iRet;

    iInLen = strlen(pcFrom)+1;
    psInBuf = (char *)malloc(iInLen);
    if ( NULL == psInBuf )
    {
        return 0;
    }
    memset(psInBuf, 0x0, iInLen);
    memcpy(psInBuf, pcFrom, iInLen -1);

    iOutLen = iMaxToLen;
    psOutBuf = (char *)malloc(iOutLen);
    if ( NULL == psOutBuf )
    {
        if(psInBuf)
        {
            free(psInBuf);
            psInBuf = NULL;
        }
        return 0;
    }
    memset(psOutBuf, 0x0, iOutLen);

    iRet = ChangeCode( "UTF-8", "GB2312",  psInBuf, &iInLen, psOutBuf, &iOutLen );
    if ( 0 != iRet )
    {
        printf("ChangeCode: Error\n");

        if(psInBuf)
        {
            free(psInBuf);
            psInBuf = NULL;
        }

        
    

        if(iOutLen > 0)
        {
            if(psOutBuf)
            {
                free(psOutBuf);
                psOutBuf = NULL;
            }
        }
        return 0;
    }
    snprintf(pcTo,iOutLen,"%s",psOutBuf);
    //printf("%s: iOutLen = %d\n", __FUNCTION__, iOutLen);

    if(psInBuf)
    {
        free(psInBuf);
        psInBuf = NULL;
    }
    
    if(psOutBuf)
    {
        free(psOutBuf);
        psOutBuf = NULL;
    }
    

    return iOutLen;

}