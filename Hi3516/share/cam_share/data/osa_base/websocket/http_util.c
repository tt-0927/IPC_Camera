
#include <stdio.h>
#include <string.h>
#include "http_util.h"
#include <stdlib.h>
#include <ctype.h>


int http_parse_url(char *url, const char **protocol, const char **ads, int *port,
	      const char **path)
{
	const char *end;
	char unix_skt = 0;

	/* cut up the location into address, port and path */
	*protocol = url;
	while (*url && (*url != ':' || url[1] != '/' || url[2] != '/'))
		url++;
	if (!*url) {
		end = url;
		url = (char *)*protocol;
		*protocol = end;
	} else {
		*url = '\0';
		url += 3;
	}
	if (*url == '+') /* unix skt */
		unix_skt = 1;

	*ads = url;
	if (!strcmp(*protocol, "http") || !strcmp(*protocol, "ws"))
		*port = 80;
	else if (!strcmp(*protocol, "https") || !strcmp(*protocol, "wss"))
		*port = 443;

	if (*url == '[') {
		++(*ads);
		while (*url && *url != ']')
			url++;
		if (*url)
			*url++ = '\0';
	} else
		while (*url && *url != ':' && (unix_skt || *url != '/'))
			url++;

	if (*url == ':') {
		*url++ = '\0';
		*port = atoi(url);
		while (*url && *url != '/')
			url++;
	}
	*path = "/";
	if (*url) {
		*url++ = '\0';
		if (*url)
			*path = url;
	}

	return 0;
}



static unsigned char hexchars[] = "0123456789ABCDEF";
/**
 * 16进制数转换成10进制数
 * 如：0xE4=14*16+4=228
 */
static int php_htoi(char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}


char *http_url_encode(char const *s, int len, int *new_length)
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;

    from = (unsigned char *)s;
    end  = (unsigned char *)s + len;
    start = to = (unsigned char *) calloc(1, 3*len+1);

    while (from < end)
    {
        c = *from++;

        if (c == ' ')
        {
            *to++ = '+';
        }
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') ||
                 (c > 'z'))
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];//将2进制转换成16进制表示
            to[2] = hexchars[c & 15];//将2进制转换成16进制表示
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }
    *to = 0;
    if (new_length)
    {
        *new_length = to - start;
    }

    return (char *) start;
}


int http_url_decode(char *str, int len)
{
    char *dest = str;
    char *data = str;

    while (len--)
    {
        if (*data == '+')
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2)))
        {
            *dest = (char) php_htoi(data + 1);
            data += 2;
            len -= 2;
        }
        else
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return dest - str;
}


static int http_count_param(const char* url,int urlLen)
{
	char *pParamStart = NULL;
	int count = 0;
	int i = 0;

	//参数列表从?开始
	pParamStart = (char *)memchr(url, '?', urlLen);
	if (pParamStart == NULL)
	{
		return 0;//没有参数
	}

	count = 1;	//只要有?，就会有一个参数
	for(i = 0 ;i < urlLen;i++)
	{
		if(url[i] == 0x26)//&
		{
			count++;
		}
	}

	return count;
}


int http_urlParam_parse(const char* url,int urlLen,httpKeyValue_t** paramOut,int *paramNum)
{
	int index = 0;
	char *pParamStart = NULL;
	int keyLen = 0;
	char *p = NULL;
	char *pStrEnd = NULL;
	char *pKeyStart = NULL;
	char *pKeyEnd = NULL;
	char *pValueEnd = NULL;
	int count = 0;
	httpKeyValue_t *pCurrent = NULL;
	httpKeyValue_t *param = NULL;

	//统计有多少个参数
	count = http_count_param(url,urlLen);
	if(count <= 0)
	{
		printf("this url no param!!!\n");
		return 0;
	}

	char *srcUrl = (char*)malloc(urlLen);
	if(srcUrl == NULL)
	{
		printf("malloc error!!\n");
		return -1;
	}
	memcpy(srcUrl,url,urlLen);

	param = (httpKeyValue_t*)malloc(sizeof(httpKeyValue_t)*count);
	if(param == NULL)
	{
		printf("malloc error!!\n");
		return -1;
	}
	memset(param,0,sizeof(httpKeyValue_t)*count);

	//跳过?
	pParamStart = (char *)memchr(srcUrl, '?', urlLen);
	if (pParamStart == NULL)
	{
		free(srcUrl);
		free(param);
		return 0;//没有参数
	}
	pParamStart += 1;

	pStrEnd = pParamStart + (urlLen - (pParamStart - srcUrl));
	p = pParamStart;

	while (p < pStrEnd)
	{
		pCurrent = &param[index++];

		pKeyStart = p;
		pValueEnd = (char *)strchr(p, '&');
		if (pValueEnd == NULL)
		{
			pValueEnd = pStrEnd;
			p = pStrEnd;
		}
		else
		{
			p = pValueEnd + 1;	//跳过&
		}

		pKeyEnd = (char *)strchr(pKeyStart, '=');
		if (pKeyEnd == NULL)  //no =
		{
			continue;
		}

		keyLen = (int)(pKeyEnd - pKeyStart);
		if (keyLen == 0) //empty key
		{
			continue;
		}

		pCurrent->keyLen = keyLen;
		pCurrent->key = (char*)malloc(keyLen);
		memcpy(pCurrent->key,pKeyStart,keyLen);

		pCurrent->valueLen = pValueEnd - pKeyEnd - 1;
		pCurrent->value = (char*)malloc(pCurrent->valueLen);
		memcpy(pCurrent->value,pKeyEnd+1,(pCurrent->valueLen));
	}

	*paramOut = param;
	*paramNum = count;

	if(srcUrl)
	{
		free(srcUrl);
		srcUrl = NULL;
	}

	return 0;
}



int http_release_param(httpKeyValue_t* param,int paramNum)
{
	if(param == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}

	int i = 0;

	for(i = 0;i < paramNum;i++)
	{
		if(param[i].key)
		{
			free(param[i].key);
			param[i].key = NULL;
		}

		if(param[i].value)
		{
			free(param[i].value);
			param[i].value = NULL;
		}

	}

	free(param);
	param = NULL;

	return 0;
}





















