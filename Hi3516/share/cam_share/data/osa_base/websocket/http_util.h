
#ifndef _OS_SOURCE_HTTP_FUNC_URL_PARSE_
#define _OS_SOURCE_HTTP_FUNC_URL_PARSE_



//客户端连接信息
typedef struct _CLEINT_CONNECT_INFO_
{
	int port;				//请求的端口号
	const char 	*protocol;	//协议ws,wss
	const char 	*address;	//请求的主机
	const int 	*path;		//请求的路径,最开头是没有'/',要加上

}client_connect_t;

int http_parse_url(char *url, const char **protocol, const char **ads, int *port,
	      const char **path);


typedef struct _HTTP_PARAM_INFO_
{
	char *key;
	char *value;
	int keyLen;
	int valueLen;
}httpKeyValue_t;


char *http_url_encode(char const *s, int len, int *new_length);

int http_url_decode(char *str, int len);

int http_urlParam_parse(const char* url,int urlLen,httpKeyValue_t** paramOut,int *paramNum);

int http_release_param(httpKeyValue_t* param,int paramNum);




#endif //_OS_SOURCE_HTTP_FUNC_URL_PARSE_

