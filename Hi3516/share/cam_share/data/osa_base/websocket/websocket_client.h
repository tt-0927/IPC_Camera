/*
 * @FilePath: websocket_client.h
 * @Author: yangwenyao
 * @Date: 2022-12-09 17:40:26
 * @LastEditors: lianghaoyao 709692194@qq.com
 * @LastEditTime: 2025-02-10 17:21:44
 * @Descripttion: 
 */


#ifndef _OS_SOURCE_WEBSOCKET_CLIENT_INFO_INCLUDE_
#define _OS_SOURCE_WEBSOCKET_CLIENT_INFO_INCLUDE_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "websocket_common.h"
#include "http_util.h"
#include "../reactor/ae.h"
#include "os_mutex.h"
#include "os_que.h"
#include "libwebsockets.h"
#include "os_thr.h"


typedef struct _WEBSOCKET_MESS_INFO_ websocketClient_t;

typedef void (*websocket_callback)(websocketClient_t* websocket,char* buff,int buffLen,void *user);
typedef void (*websocket_networkStatus)(websocketClient_t* websocket,int status,void *user);	//status:1-连接成功，2-断开连接

typedef struct _WEBSOCKET_MESS_INFO_
{
	char *url;				//请求的url地址
	int socket;				//通讯socket，0-正在连接中，< 0连接失败，>0 连接成功
	aeEventLoop* event;		//异步事件
	client_connect_t clientInfo;	//客户端连接信息
	int isreconnect;		//是否重连，1-是
	char *hostName;			//访问的服务器
	char serverip[64];		//访问的服务器ip地址

	websocket_callback dealFunc;			//上抛数据
	websocket_networkStatus networkStatus;	//上抛网络状态
	void *user;								//用户带入的参数

	OS_QueHndl asynchronousQue_;	//异步发送链表
	int asynchronousFlag;			//是否开启异步发送，1-是。0-否

	int userLibwebsocket;			//1-使用libwebsocket
	struct libwebsocket *libwebsocket;
	struct libwebsocket_context *libwebsocketContext;
	int libwebsocketClose;			//1-websocket服务器主动断开

	int exit;				//1-exit结束websocket

	OS_ThrHndl eventTid;
	OS_ThrHndl connectTid;
	OS_MutexHndl mutex;
}websocketClient_t;


websocketClient_t* websocket_client_init(char *url,websocket_callback call,websocket_networkStatus network,void *user,int isreconnect);

int websocket_client_unint(websocketClient_t* handle);

int websocket_client_send(websocketClient_t* handle,unsigned char *data, unsigned int dataLen, bool mod, Websocket_CommunicationType type);

int websocket_client_reconnect(websocketClient_t* handle);

#endif //_OS_SOURCE_WEBSOCKET_CLIENT_INFO_INCLUDE_

