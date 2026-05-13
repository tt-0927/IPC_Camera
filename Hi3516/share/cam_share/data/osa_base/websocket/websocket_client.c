
#include <netdb.h>
#include "websocket_client.h"
#include "http_util.h"
#include "os_thr.h"

typedef struct ASYN_BUFFER_INFO
{
	char *date;
	int size;
	bool mod;
	Websocket_CommunicationType type;
}asynBuffer_s;

static int asynBuffer_free(asynBuffer_s* frame)
{
	if(frame)
	{
		if(frame->date)
		{
			free(frame->date);
			frame->date = NULL;
		}
		free(frame);frame = NULL;
	}
	return 0;
}


static void websocket_client_callback(struct aeEventLoop *handle,int fd,void *user,int mask);
static void websocket_asyn_write(websocketClient_t* handle,void* libwebsocket);

static int websocket_get_socket(websocketClient_t* handle)
{
	int socket = 0;
	OS_mutexLock(&(handle->mutex));
	socket = handle->socket;
	OS_mutexUnlock(&(handle->mutex));
	return socket;
}

static int websocket_set_socket(websocketClient_t* handle,int socket)
{
	OS_mutexLock(&(handle->mutex));
	handle->socket = socket;
	OS_mutexUnlock(&(handle->mutex));
	return 0;
}

//保活定时器
static int websocket_event_time_live(struct aeEventLoop *l,long long id,void *data)
{
	return 500;	//1000ms
}

static void *websocket_event_loop_thr(void* argv)
{
	websocketClient_t* handle = (websocketClient_t*)argv;
	aeEventLoop* event = handle->event;

	//默认需要添加一个定时器，保持事件驱动内部不会永久等待
	//保活定时器
	aeCreateTimeEvent(event,500,websocket_event_time_live,NULL,NULL);

	printf("start event loop!!![%p]\n\n\n",event);

	aeMain(event);
	aeDeleteEventLoop(event);

	printf("=====>exit reator event[%p]!!!!\n",event);
	return NULL;
}


static int websocket_event_init(websocketClient_t* handle)
{
	aeEventLoop* event = NULL;
	event = aeCreateEventLoop(50);
	if(event == NULL)
	{
		printf("aeCreateEventLoop error!!\n");
		return -1;
	}
	handle->event = event;

	OS_thrCreate(&(handle->eventTid),websocket_event_loop_thr,OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT,(void*)handle);

	return 0;
}

///////////////////////////////////////////////////

static int
callback_dumb_increment(struct libwebsocket_context *this,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{

	websocketClient_t* handle = (websocketClient_t*)user;

	switch (reason)
	{
	case LWS_CALLBACK_CLOSED:
		fprintf(stderr, "LWS_CALLBACK_CLOSED\n");
		handle->libwebsocketClose = 1;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
//		printf("recv:[%s]size[%d]\n",(char*)in,len);
		handle->dealFunc(handle,(char*)in,len,handle->user);
		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		/*
		 * start the ball rolling,
		 * LWS_CALLBACK_CLIENT_WRITEABLE will come next service
		 */
//		libwebsocket_callback_on_writable(this, wsi);//使能写
		break;

	case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
		return 1;
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		/* we will send our packet... */
		websocket_asyn_write(handle,wsi);

//		/* get notified as soon as we can write again */
//		libwebsocket_callback_on_writable(this, wsi);
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */
static struct libwebsocket_protocols protocols[] =
{
	{
		"dumb-increment-protocol",
		callback_dumb_increment,
		0,
		1024*1024,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

static int libwebsocket_connect(websocketClient_t* handle,char *path)
{
	if(handle == NULL)
	{
		printf("this handle is null!!!!\n");
		return -1;
	}

	int ret = 1;	//1-表示连接成功
	int ietf_version = -1; /* latest */
	struct libwebsocket *libwebsocket = NULL;
	struct libwebsocket_context *context = NULL;
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(struct lws_context_creation_info));

	int use_ssl = 0;
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		printf("Creating libwebsocket context failed\n");
		if(handle->networkStatus)
		{
			handle->networkStatus(handle,-1,handle->user);
		}
	}

	/* create a client websocket using dumb increment protocol */
	libwebsocket = libwebsocket_client_connect_extended(context, \
			handle->serverip, handle->clientInfo.port, use_ssl,\
			path, handle->hostName, handle->hostName,\
			protocols[0].name, ietf_version,(void*)handle);
	if (libwebsocket == NULL) {
		printf("libwebsocket dumb connect failed\n");
		ret = -1;
	}

	OS_mutexLock(&(handle->mutex));
	handle->libwebsocketContext = context;
	handle->libwebsocket = libwebsocket;
	OS_mutexUnlock(&(handle->mutex));

	if(handle->networkStatus)
	{
		handle->networkStatus(handle,ret,handle->user);
	}

	if(ret >= 0)
	{
		int socket = libwebsocket_get_socket_fd(handle->libwebsocket);
		websocket_set_socket(handle,socket);
		printf("<libwebsocket> =====================> connect server success!! socket[%d]<=====================\n",socket);
		while (!handle->exit && (!handle->libwebsocketClose))
		{
			libwebsocket_service(context, 10);
		}
	}

	websocket_set_socket(handle,-1);
	OS_mutexLock(&(handle->mutex));
	handle->libwebsocketContext = NULL;
	handle->libwebsocket = NULL;
	OS_mutexUnlock(&(handle->mutex));

	//释放内存
	libwebsocket_context_destroy(context);
	printf("<libwebsocket> =====================> exit websocket <=====================\n");

	return ret;
}


static int myWebsocket_connect(websocketClient_t* handle,char *path)
{
	int ret = 0;
	int socket = 0;
	socket = webSocket_clientLinkToServer(handle->serverip,handle->clientInfo.port,handle->hostName,path);
	if(socket < 0)
	{
		printf("client connect server error!!\n");
		handle->socket = -2;	//表示连接失败
		ret = 2;
		goto EXIT;
	}
	//设置监听事件
	int mask;
	if(handle->asynchronousFlag == 1)
	{
		//异步发送
		mask = AE_READABLE | AE_WRITABLE;
	}else
	{
		mask = AE_READABLE;
	}
	ret = aeCreateFileEvent((handle->event),socket,\
			mask,websocket_client_callback,(void*)handle);
	if(ret < 0)
	{
		printf("aeCreateFileEvent error!!!\n");
		handle->socket = -2;	//表示连接失败
		ret = 2;
		goto EXIT;
	}
	ret = 1;	//连接成功
	websocket_set_socket(handle,socket);

	printf("<websocket> =====================> connect server success!! socket[%d] <=====================\n",socket);

EXIT:
	//上抛状态
	if(handle->networkStatus)
	{
		handle->networkStatus(handle,ret,handle->user);
	}
	return 0;
}


static void *websocket_connect_server(void* argv)
{
	websocketClient_t* handle = (websocketClient_t*)argv;
	int ret = 0;
	char *path = NULL;
	char* dynPath = NULL;
	int socket = 0;

	//判断是域名还是ip地址
	int hostNameLen = 0;
	if (inet_addr(handle->clientInfo.address) == INADDR_NONE)
	{
		//解析域名
		int i = 0;
		struct hostent *p = NULL;
		p = gethostbyname(handle->clientInfo.address);
		if(p == NULL)
		{
			//获取失败，没联网或者其他原因,若没联网，则要超时20+s才会超时返回
			ret = 2;
			handle->socket = -2;	//表示连接失败
			if(handle->networkStatus)
			{
				handle->networkStatus(handle,ret,handle->user);
			}
			return NULL;
		}

		for (i = 0; p->h_addr_list[i]; i++)
		{
			printf("%s \n", (char*)inet_ntoa(*(struct in_addr *)p->h_addr_list[i]));
			sprintf(handle->serverip,"%s",(char*)inet_ntoa(*(struct in_addr *)p->h_addr_list[i]));
		}

		hostNameLen = strlen(handle->clientInfo.address)+1;
		handle->hostName = (char*)malloc(hostNameLen);
		memset(handle->hostName,0,hostNameLen);
		snprintf(handle->hostName,hostNameLen,"%s",handle->clientInfo.address);
	}else
	{
		//ip地址
		sprintf(handle->serverip,"%s",handle->clientInfo.address);

		hostNameLen = strlen(handle->clientInfo.address)+32;	//32位端口号
		handle->hostName = (char*)malloc(hostNameLen);
		memset(handle->hostName,0,hostNameLen);
		snprintf(handle->hostName,hostNameLen,"%s:%d",handle->clientInfo.address,handle->clientInfo.port);
	}

	//request path
	if (handle->clientInfo.path[0] != '/')
	{
		int len = 0;
		len = strlen(handle->clientInfo.path) + 2;
		char* dynPath = (char*)malloc(len);
		memset(dynPath,0,len);

		dynPath[0] = '/';
		strncpy(dynPath + 1, handle->clientInfo.path, strlen(handle->clientInfo.path));
		path = dynPath;
	}else
	{
		path = handle->clientInfo.path;
	}

	if(handle->userLibwebsocket == 1)
	{
		libwebsocket_connect(handle,path);
	}else
	{
		myWebsocket_connect(handle,path);
	}

	if(dynPath)
	{
		free(dynPath);
		dynPath = NULL;
	}
	return NULL;
}


websocketClient_t* websocket_client_init(char *url,websocket_callback call,websocket_networkStatus network,void *user,int isreconnect)
{
	int ret = 0;
	websocketClient_t* handle = (websocketClient_t*)malloc(sizeof(websocketClient_t));
	if(handle == NULL)
	{
		printf("malloc errir!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(websocketClient_t));

	handle->userLibwebsocket = 1;

	//init que
	OS_queCreate(&(handle->asynchronousQue_),75);
	handle->asynchronousFlag = 1;

	handle->dealFunc = call;
	handle->networkStatus = network;
	handle->user = user;
	handle->isreconnect = isreconnect;

	if(handle->userLibwebsocket == 0)
	{
		//创建reactor
		websocket_event_init(handle);
	}
	OS_mutexCreate(&(handle->mutex));

	int urlLen = 0;
	urlLen = strlen(url)+1;
	handle->url = (char*)malloc(urlLen);
	if(handle->url == NULL)
	{
		printf("malloc error!!\n");
		free(handle);
		handle = NULL;
		return NULL;
	}
	memset(handle->url,0,urlLen);
	memcpy(handle->url,url,strlen(url));

	//解析客户端信息
	http_parse_url(handle->url,&(handle->clientInfo.protocol),\
						&(handle->clientInfo.address),&(handle->clientInfo.port),&(handle->clientInfo.path));

	OS_thrCreate(&(handle->connectTid),websocket_connect_server,OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT,(void*)handle);

	return handle;
}


static int websocket_client_unusual(websocketClient_t* handle)
{
	if(handle == NULL)
	{
		printf("this arugment is NULL!!\n");
		return -1;
	}

	//上抛状态
	if(handle->networkStatus)
	{
		handle->networkStatus(handle,2,handle->user);
	}

	if(handle->isreconnect == 1)
	{
		printf("0000000000000000reconnect!!!!!!!!!!!!!!!!\n");
		//重连
		websocket_client_reconnect(handle);
	}else
	{
		printf("11111111111111111close!!!!!!!!!!!!!!!!\n");
		//del event
		aeDeleteFileEvent(handle->event,handle->socket,AE_READABLE);

		//断开
		if(handle->socket)
		{
			close(handle->socket);
			handle->socket = -2;
		}
	}

	return 0;
}


static void websocket_asyn_write(websocketClient_t* handle,void* libwebsocket)
{
	if(handle == NULL)
	{
		printf("this argument is null!!!\n");
		return ;
	}

	int ret = 0;
	int frameAddr = 0;
	asynBuffer_s* frame = NULL;

	if(!OS_queIsEmpty(&(handle->asynchronousQue_)))
	{
		if(OS_queGet(&(handle->asynchronousQue_), (Int64 *)(&frameAddr),  OS_TIMEOUT_NONE) < 0)
		{
			printf("get que error!!!!\n");
			return ;
		}
		frame = (asynBuffer_s*)frameAddr;
		if(frame)
		{
			//send
			if(handle->userLibwebsocket == 1)
			{
				unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
				memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], frame->date, frame->size);
				ret = libwebsocket_write(libwebsocket, &buf[LWS_SEND_BUFFER_PRE_PADDING], frame->size, LWS_WRITE_BINARY);
			}else
			{
				ret = webSocket_send(handle->socket,frame->date,frame->size,frame->mod,frame->type);
			}
			if(ret < 0)
			{
				printf("send data error socket[%d] error[%d]!! dataLen[%d]\n\n",handle->socket,ret,frame->size);
			}
		}
		asynBuffer_free(frame);frame = NULL;
	}

}



static void websocket_client_callback(struct aeEventLoop *event,int fd,void *user,int mask)
{
	//读取科大讯飞返回的数据
	int ret = 0;
	unsigned char *readbuff = NULL;
	unsigned int readLen = 0;

	websocketClient_t* handle = (websocketClient_t*)user;

	//printf("event call back!!mask[0x%x]\n",mask);

	if(mask&AE_READABLE)
	{
		//可读
		ret = webSocket_recv(fd,&readbuff,&readLen,3000);
		if((ret < 0) || (readLen == 0))
		{
			printf("==>recv server error[%d] readLen[%d]!!<===\n",ret,readLen);
			//断开服务器连接
			websocket_client_unusual(handle);
			goto EXIT;
		}

		//上抛数据
		handle->dealFunc(handle,readbuff,readLen,handle->user);

	}else if(mask&AE_WRITABLE)
	{
		//异步写
		//printf("write write write write write write write!!!!\n");
		websocket_asyn_write(handle,NULL);
	}else
	{
		//异常，需要断开网络
		websocket_client_unusual(handle);
	}

EXIT:

	if(readbuff)
	{
		free(readbuff);
		readbuff = NULL;
	}

	return ;
}



int clear_que(websocketClient_t* handle)
{
	int frameAddr = 0;
	asynBuffer_s* frame = NULL;

	while(!OS_queIsEmpty(&(handle->asynchronousQue_)))
	{
		if(OS_queGet(&(handle->asynchronousQue_), (Int64 *)(&frameAddr),  OS_TIMEOUT_NONE) < 0)
		{
			printf("get que error!!!!\n");
			return -1;
		}
		frame = (asynBuffer_s*)frameAddr;
		asynBuffer_free(frame);frame = NULL;
	}
	return 0;
}



int websocket_client_unint(websocketClient_t* handle)
{
	if(handle == NULL)
	{
		printf("<websocket> this argument is NULL!!\n");
		return -1;
	}

	handle->exit = 1;
	if(handle->userLibwebsocket == 0)
	{
		//销毁reactor
		aeStop(handle->event);
		//等待线程退出
		OS_thrJoin(&(handle->eventTid));
	}
	//等待线程退出
	OS_thrJoin(&(handle->connectTid));

	//clear
	clear_que(handle);

	//exit
	OS_mutexLock(&(handle->mutex));
	if(handle->url)
	{
		free(handle->url);
		handle->url = NULL;
	}
	if(handle->hostName)
	{
		free(handle->hostName);
		handle->hostName = NULL;
	}
	OS_mutexUnlock(&(handle->mutex));
	OS_mutexDelete(&(handle->mutex));
	OS_queDelete(&(handle->asynchronousQue_));
	free(handle);

	printf("websocket uninit success!!!\n");

	return 0;
}


int websocket_client_reconnect(websocketClient_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	//connect
	int ret = 0;
	char *path = NULL;
	char* dynPath = NULL;
	if (handle->clientInfo.path[0] != '/')
	{
		int len = 0;
		len = strlen(handle->clientInfo.path) + 2;
		char* dynPath = (char*)malloc(len);
		memset(dynPath,0,len);

		dynPath[0] = '/';
		strncpy(dynPath + 1, handle->clientInfo.path, strlen(handle->clientInfo.path));
		path = dynPath;
	}else
	{
		path = handle->clientInfo.path;
	}

	if(handle->userLibwebsocket != 1)
	{
		//先关闭旧的socket
		int socket = 0;
		socket = websocket_get_socket(handle);
		if(socket > 0)
		{
			close(socket);
			socket = -2;
			websocket_set_socket(handle,socket);
		}

		socket = webSocket_clientLinkToServer(handle->serverip,handle->clientInfo.port,handle->hostName,path);
		if(socket < 0)
		{
			printf("============>client connect server error!!<=============\n");
			ret = -1;
			goto EXIT;
		}
		websocket_set_socket(handle,socket);

	}else
	{
		//libwebsocket


	}

	//上抛状态
	if(handle->networkStatus)
	{
		handle->networkStatus(handle,1,handle->user);
	}

	printf("===> websocket reconnect success!!! <====\n");

EXIT:
	if(dynPath)
	{
		free(dynPath);
		dynPath = NULL;
	}

	return ret;
}

int websocket_client_send(websocketClient_t* handle,unsigned char *data, unsigned int dataLen, bool mod, Websocket_CommunicationType type)
{
	if((handle == NULL) || (data == NULL) || (dataLen <= 0))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
	int ret = 0;

	OS_mutexLock(&(handle->mutex));

	if(handle->socket < 0)
	{
		OS_mutexUnlock(&(handle->mutex));
		return -2;	//发送失败，需要重连
	}

	if(handle->asynchronousFlag == 1)
	{
		asynBuffer_s* frame = (asynBuffer_s*)malloc(sizeof(asynBuffer_s));
		if(frame == NULL)
		{
			printf("malloc error!!!\n");
			OS_mutexUnlock(&(handle->mutex));
			return -1;
		}
		frame->date = (char*)malloc(dataLen);
		if(frame->date == NULL)
		{
			free(frame);
			printf("malloc error!!!\n");
			OS_mutexUnlock(&(handle->mutex));
			return -1;
		}
		frame->size = dataLen;
		memcpy(frame->date,data,frame->size);
		frame->mod = mod;
		frame->type = type;
		if(OS_quePut(&(handle->asynchronousQue_), (Int32)(frame),  OS_TIMEOUT_NONE) < 0)
		{
			printf("this que is full!!!!\n");
			asynBuffer_free(frame);frame = NULL;
		}

		if((handle->userLibwebsocket == 1) && (handle->libwebsocketContext) && (handle->libwebsocket))
		{
			libwebsocket_callback_on_writable(handle->libwebsocketContext, handle->libwebsocket);
		}

	}else
	{
		if((handle->userLibwebsocket == 1) && (handle->libwebsocket))
		{
			unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
												  LWS_SEND_BUFFER_POST_PADDING];
			memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], data, dataLen);
			ret = libwebsocket_write(handle->libwebsocket, &buf[LWS_SEND_BUFFER_PRE_PADDING], dataLen, LWS_WRITE_BINARY);
		}else
		{
			ret = webSocket_send(handle->socket,data,dataLen,mod,type);
		}
		if(ret < 0)
		{
			printf("send data error socket[%d] error[%d]!! dataLen[%d] data[%s]\n\n\n\n",handle->socket,ret,dataLen,data);
		}
	}

	OS_mutexUnlock(&(handle->mutex));

	return ret;
}



