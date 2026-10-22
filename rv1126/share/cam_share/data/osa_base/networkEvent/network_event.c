

/* write by zhangjunbin on 2018/9/21
 *
 * 1>event注册的回调函数不能阻塞，否则会造成所有事件都阻塞
 *
 * 2>拓展深思：由于event内部采用的是Reactor模型（异步非阻塞IO），
 * 		a.目前采用的是read事件触发后，接收数据后直接回调处理函数了，如果回调函数处理有阻塞或者耗时，则会影响reactor模型的响应速度；
 * 		b.解决办法：新增工作线程池。read事件触发后，接收到数据后，将数据丢到工作线程池中工作，read事件就算完成，返回reactor模型，就不会担心回调处理阻塞了事件调度；
 *
 * */

#include <fcntl.h> // for open
#include <errno.h>
#include "os.h"
#include "os_thr.h"
#include "os_mutex.h"
#include "hashMap.h"
#include "os_que.h"
#include "event/ae.h"
#include "network_event.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h> // for close
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#endif

#define NETWORK_MAX_LEN (8192+1024)

#define NETWORK_CHECHK_PC_IDEENTIFIER_FIRST_BIT	    '@'
#define NETWORK_CHECHK_PC_IDEENTIFIER_SECOND_BIT	 	'#'
#define NETWORK_CHECHK_PC_IDEENTIFIER_THIRD_BIT 		'$'
#define NETWORK_CHECHK_PC_IDEENTIFIER_FOURTH_BIT		'&'

#define NETWORK_MSG_HEAD_VERSION			(2015)


//-------------------------------------------------

typedef struct _EVENT_ARGV_INFO
{
	evEventBase* base;

}network_event_t;

#define NETWORK_EVENT_READ_FLAG		(0x1)	//0000 0001
#define NETWORK_EVENT_WRITE_FLAG 	(0x2)	//0000 0010
#define NETWORK_EVENT_HEART_FLAG 	(0x4)	//0000 0100
#define NETWORK_EVENT_RECONNECT_FLAG (0x8)	//0000 1000

typedef struct NETWORK_HEAD_INFO
{
	/* 默认的头信息 */
	network_HeadInfo_t stDefaultRecvHead;
	network_HeadInfo_t stDefaultSendHead;

	/* 自定义头信息 */
	void *pstCustomRecvHead;
	void *pstCustomSendHead;
	int nCustomHeadSize;

}NetworkHead_S;


typedef struct _NETWORK_SERVER_CLIENT_INFO_
{
	int socket;
	network_inparamClientNet_t clientInparam;	//客户端信息

	evTimeEvent* heartTimeoutID;				//定时器句柄
	evTimeEvent* reconnectTimeoutID;			//定时重连事件
	unsigned int eventFlag;						//事件是否启动，bit表示，1-启动，0-未启动事件

	/*event句柄*/
	network_event_t eventHandle;
	OS_QueHndl asynchronousQue;					//异步发送队列
	void *serverHandle;	//服务端句柄
	NetworkHead_S headInfo;						//头信息

	//hashMap
	hashMapHandle_t* hashHandle;				//hash map

	OS_MutexHndl socket_mutex;					//socket发送锁
	OS_MutexHndl client_mutex;

	void *serverClientParam;                    //用户设置客户端信息结构体
	OS_MutexHndl clientParam_mutex;             //信息锁

	/* 线程句柄 */
	OS_ThrHndl reatorTid;
	int isHaveReatorTid;

}network_clientInfo_t;


typedef struct _NETWORK_SERVER_INFO_
{
	int acceptIPV4Socket;
	int acceptIPV6Socket;
	/*服务端传入参数*/
	network_inparamServerNet_t inparam;
	/*libevent句柄*/
	network_event_t eventHandle;
	/*连接上服务器的客户端链表*/
	OS_listHndl clientList;	//network_clientInfo_t
	OS_MutexHndl server_mutex;
	/* 线程句柄 */
	OS_ThrHndl reatorTid;
}network_serverInfo_t;

typedef struct Network_Asynchronous_Info
{
	char *messege;
	int nLen;
	int code;
}network_AsynchronousInfo_t;

//-----------------------------------------


int networkClient_unusual_deal(network_clientInfo_t *handle);
/*内部使用发送函数*/
static int interior_networkSend_data(network_clientInfo_t *clientHandle,char *message,int nLen,int code);

void network_client_callback(evEventBase *base,int sockfb, void *arg, int events);

static int ae_time_live(evEventBase *l,void *data);

//-----------------------------------server-----------------------------------------


int network_set_ClientParam(network_Handle_t clienthandle, void*clientParam)
{
	network_clientInfo_t* pclientHandle = (network_clientInfo_t*)clienthandle;
	if(pclientHandle == NULL)
	{
		return -1;
	}
	OS_mutexLock(&(pclientHandle->clientParam_mutex));
	pclientHandle->serverClientParam = clientParam;
	OS_mutexUnlock(&(pclientHandle->clientParam_mutex));
	return 0;
}
void* network_get_ClientParam(network_Handle_t clienthandle)
{
	network_clientInfo_t* pclientHandle = (network_clientInfo_t*)clienthandle;
	void *clientParam = NULL;
	if(pclientHandle == NULL)
	{
		return NULL;
	}
	OS_mutexLock(&(pclientHandle->clientParam_mutex));
	clientParam = pclientHandle->serverClientParam;
	OS_mutexUnlock(&(pclientHandle->clientParam_mutex));
	return clientParam;
}


static int network_check_netHead_default(network_HeadInfo_t *head)
{
	if ((head == NULL) || (head->identifier[0] != NETWORK_CHECHK_PC_IDEENTIFIER_FIRST_BIT)
		|| (head->identifier[1] != NETWORK_CHECHK_PC_IDEENTIFIER_SECOND_BIT)
		|| (head->identifier[2] != NETWORK_CHECHK_PC_IDEENTIFIER_THIRD_BIT)
		|| (head->identifier[3] != NETWORK_CHECHK_PC_IDEENTIFIER_FOURTH_BIT))
	{
		return -1;
	}

	if (head->load_len <= 0)
	{
		return -1;
	}

	return 0;
}

static int networkHead_check(network_clientInfo_t *clientHandle,\
		void *pHeadBuf,int *pnBodySize,int *pnCode)
{
	if((clientHandle == NULL) || (pHeadBuf == NULL) \
			|| (pnBodySize == NULL) || (pnCode == NULL))
	{
		printf("this arugment is null!!!\n");
		return -1;
	}

	int ret = 0;
	if(clientHandle->clientInparam.headCheck)
	{
		ret = clientHandle->clientInparam.headCheck(clientHandle,\
				pHeadBuf,pnBodySize,pnCode,\
				clientHandle->clientInparam.param);
	}else
	{
		network_HeadInfo_t* pDefaultHeadbuf = (network_HeadInfo_t*)pHeadBuf;
		ret = network_check_netHead_default(pDefaultHeadbuf);
		if(ret < 0)
		{
			return ret;
		}
		clientHandle->headInfo.stDefaultRecvHead.reserve = pDefaultHeadbuf->reserve;
		*pnBodySize = pDefaultHeadbuf->load_len;
		*pnCode = pDefaultHeadbuf->msg_code;
	}

	return ret;
}



static int network_init_head_default(network_HeadInfo_t *head)
{
	if (head == NULL)
	{
		return -1;
	}

	memset(head, 0, sizeof(network_HeadInfo_t));
	head->identifier[0] = NETWORK_CHECHK_PC_IDEENTIFIER_FIRST_BIT;
	head->identifier[1] = NETWORK_CHECHK_PC_IDEENTIFIER_SECOND_BIT;
	head->identifier[2] = NETWORK_CHECHK_PC_IDEENTIFIER_THIRD_BIT;
	head->identifier[3] = NETWORK_CHECHK_PC_IDEENTIFIER_FOURTH_BIT;
	head->version = NETWORK_MSG_HEAD_VERSION;

	return 0;
}


static int networkHead_init(network_clientInfo_t *clientHandle,\
		void **pHeadBuf,int *pnHeadSize,int nBodySize,int nCode)
{
	if(clientHandle == NULL)
	{
		printf("this handle is null!!!\n");
		return -1;
	}

	if(clientHandle->clientInparam.headInit)
	{
		*pHeadBuf = clientHandle->headInfo.pstCustomSendHead;
		*pnHeadSize = clientHandle->headInfo.nCustomHeadSize;

		clientHandle->clientInparam.headInit(clientHandle,\
				*pHeadBuf,nBodySize,nCode,\
				clientHandle->clientInparam.param);
	}else
	{
		/* 使用默认的头初始化 */
		network_init_head_default(&(clientHandle->headInfo.stDefaultSendHead));
		clientHandle->headInfo.stDefaultSendHead.load_len = nBodySize;
		clientHandle->headInfo.stDefaultSendHead.msg_code = nCode;
		*pHeadBuf = &(clientHandle->headInfo.stDefaultSendHead);
		*pnHeadSize = sizeof(network_HeadInfo_t);
	}

	return 0;
}


static int networkHead_getBuf(network_clientInfo_t *clientHandle,\
		void **pHeadBuf,int *pnHeadSize)
{
	if(clientHandle == NULL)
	{
		printf("this handle is null!!!\n");
		return -1;
	}

	int ret = 0;
	if(clientHandle->headInfo.pstCustomRecvHead)
	{
		*pHeadBuf = clientHandle->headInfo.pstCustomRecvHead;
		*pnHeadSize = clientHandle->headInfo.nCustomHeadSize;
	}else
	{
		*pHeadBuf = &(clientHandle->headInfo.stDefaultSendHead);
		*pnHeadSize = sizeof(network_HeadInfo_t);
	}

	return ret;
}


void networkServer_read_cb(int sockfb,short events, void *arg)
{
	network_clientInfo_t *client = (network_clientInfo_t*)arg;
	int recvLen = 0;
	int maxLongBufLen = 0;
	char * pRecvLongBuf = NULL;
	char recvshortBuf[NETWORK_MAX_LEN];
	char *recvPoint = NULL;
	network_CallbackMsg_t dealCmd;
	memset(&dealCmd,0,sizeof(network_CallbackMsg_t));

	void *pHeadBuf = NULL;
	int nHeadSize = 0;
	if(networkHead_getBuf(client,&pHeadBuf,&nHeadSize) < 0)
	{
		network_printf_log(client->clientInparam.logFun,\
				"socket[%d] get head buff error client[%p]",\
				sockfb,client);
		networkClient_unusual_deal(client);
		return ;
	}

    //read header
	recvLen = nHeadSize;
	network_tcpSocket_recv(client->clientInparam.logFun,\
			sockfb, (char*)pHeadBuf, &recvLen,0);
	if(recvLen != nHeadSize)
	{
		network_printf_log(client->clientInparam.logFun,\
				"socket[%d] recv header error recvlen[%d] client[%p]",\
				sockfb,recvLen,client);
		networkClient_unusual_deal(client);
		return ;
	}

	//check head
	int nBodySize = 0;
	int nCode = 0;
	if(networkHead_check(client,pHeadBuf,&nBodySize,&nCode) < 0)
	{
		network_printf_log(client->clientInparam.logFun,\
				"this header is error!!");
		networkClient_unusual_deal(client);
		return ;
	}

	if (nBodySize >= NETWORK_MAX_LEN)
	{
		if(nBodySize >= maxLongBufLen)
		{
			if (pRecvLongBuf)
			{
				free(pRecvLongBuf);
				pRecvLongBuf = NULL;
			}
			pRecvLongBuf = (char *)malloc(nBodySize);
			if (pRecvLongBuf == NULL)
			{
				network_printf_log(client->clientInparam.logFun,\
						"malloc long recvbuf is fail");
				return ;
			}
			maxLongBufLen = nBodySize;
		}

		recvPoint = pRecvLongBuf;
	}
	else
	{
		recvPoint = recvshortBuf;
		recvshortBuf[nBodySize] = '\0';
	}
	recvLen = nBodySize;

	/*接收数据*/
	network_tcpSocket_recv(client->clientInparam.logFun,\
			sockfb, recvPoint, &recvLen,0);
	if(recvLen != nBodySize)
	{
		network_printf_log(client->clientInparam.logFun,\
				"recv body error recvlen[%d]",recvLen);
		networkClient_unusual_deal(client);
        goto EXIT;
	}

	dealCmd.Code = nCode;
	dealCmd.InParam = client->clientInparam.param;
	dealCmd.recvvalue = recvPoint;
	dealCmd.nLen = nBodySize;
	dealCmd.clientHandle = client;
	dealCmd.nHeadSize = nHeadSize;
	dealCmd.pHeadBuf = pHeadBuf;

	/*修改为心跳信息直接上抛，客户端，服务端都各自发送心跳*/

	//判断是否是同步获取返回的请求
	hashKey key;
	key.i_key = nCode;
	netRpc_t* value = NULL;

	if((nCode != NETWORK_NET_HEARTBIT_CMD) && \
			(value = hashMap_find(client->hashHandle,key)))
	{
		//同步返回
		value->pkt.dataSize = nBodySize;
		value->pkt.data = malloc(nBodySize);
		memcpy(value->pkt.data,recvPoint,nBodySize);
		//需要用户调用释放函数释放内存:network_release_syncPkt();
		value->pkt.memtype = 1;
		OS_semSignal(&(value->sem));

	}else if(client->clientInparam.cmdfun)
	{
		/*异步返回*/
		/*调用回调函数处理*/
		client->clientInparam.cmdfun(&dealCmd);
	}

EXIT:
	/*判断是否有大内存需要释放*/
	if(pRecvLongBuf)
	{
		free(pRecvLongBuf);
		pRecvLongBuf = NULL;
	}
}

/*
 * libevent回调写函数,异步读写
 * */
int network_freeAsynchronous_sendmessege(network_AsynchronousInfo_t * asynchronous_sendmessege)
{
	if(asynchronous_sendmessege)
	{
		if(asynchronous_sendmessege->messege)
		{
			 free(asynchronous_sendmessege->messege);
		}
		free(asynchronous_sendmessege);
	}
	return 0;
}


/*使用写事件，若频率太高，会多占用一点cpu资源*/
void networkServer_wirte_cb(int sockfb,short events, void *arg)
{
	int listSize = 0;
	network_clientInfo_t *client = (network_clientInfo_t*)arg;
	network_AsynchronousInfo_t *pMessage = NULL;
	Int64 frameAddr = 0;

	if(client->clientInparam.asynchronous == 1)
	{
		if(OS_queGet(&(client->asynchronousQue),&frameAddr,OS_TIMEOUT_NONE) >= 0)
		{
			pMessage = (network_AsynchronousInfo_t *)frameAddr;
			if(pMessage)
			{
				interior_networkSend_data(client,pMessage->messege,pMessage->nLen,pMessage->code);

				//释放内存
				network_freeAsynchronous_sendmessege(pMessage);
			}
		}

		/*	判断队列中是否还有数据，
		 * 	若有，则继续添加写事件
		 * 	若无，则不添加写事件，
		 * 	*/
	    OS_mutexLock(&(client->socket_mutex));
		listSize = OS_queGetQueuedCount(&(client->asynchronousQue));
		if((listSize == 0) && (client->eventHandle.base))
		{
			//没有数据可以发送了，删除写事件
			evEvent_delIO(client->eventHandle.base,sockfb,EV_WRITABLE);
		}
	    OS_mutexUnlock(&(client->socket_mutex));
		if(listSize > 20)
		{
			network_printf_log(client->clientInparam.logFun,"client ip[%s] write que size[%d]",\
    			client->clientInparam.ip,listSize);
		}
	}

}


void network_server_callBack(evEventBase *eventLoop,int sockfb, void *arg,int events)
{
	if((events & EV_READABLE))
	{
		/*读事件*/
		networkServer_read_cb(sockfb,events,arg);

	}else if((events & EV_WRITABLE))
	{
		/*写事件*/
		networkServer_wirte_cb(sockfb,events,arg);
	}
	else
    {
    	/* socket关闭或错误/连接超时事件，
		 * 该句柄是服务器端主动创建的内存资源，所以可以在该处主动释放客户端内存资源
		 * */
    	network_clientInfo_t *client = (network_clientInfo_t*)arg;
    	network_printf_log(client->clientInparam.logFun,"client connect error!! so close client!! ip[%s] port[%d] event[%d]",\
    			client->clientInparam.ip,client->clientInparam.nPort,events);
        /*从客户端链表中删除该客户端*/
        if(client->serverHandle)
        {
        	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)client->serverHandle;
        	OS_mutexLock(&(pNetServerHandle->server_mutex));
        	OS_listEarse(pNetServerHandle->clientList, client);
        	OS_mutexUnlock(&(pNetServerHandle->server_mutex));
        }
        network_unInit_client((network_Handle_t)client);
    }

}



int network_server_heart(evEventBase *base, void *argv)
{
	char HeartBitBUF[4096] = {0};
	network_clientInfo_t *client = (network_clientInfo_t*)argv;
	int sendLen = 0;
	int inputLen = sizeof(HeartBitBUF);

	if(client->clientInparam.heartmsg)
	{
		client->clientInparam.heartmsg(HeartBitBUF, sizeof(HeartBitBUF), client, client->clientInparam.param, &sendLen);
		if(sendLen <= 0 || sendLen > inputLen)
		{
			sendLen = inputLen;
		}
	}else
	{
		/*上层应用没有自定义的心跳包，则默认发心跳包*/
		sprintf(HeartBitBUF,"this is network heart message");
		sendLen = strlen(HeartBitBUF);
	}
	if(interior_networkSend_data(client,HeartBitBUF,sendLen,NETWORK_NET_HEARTBIT_CMD) < 0)
	{
		network_printf_log(client->clientInparam.logFun,"send heart error!!\n");
	}

	return NETWORK_HEART_INTERVAL_TIME*1000;
}


static int networkServer_accept_client(network_serverInfo_t *serverHandle,int fd,int socketTimeOut,char* clientIp,int clientPort)
{
	if(serverHandle == NULL)
	{
		network_printf_log(serverHandle->inparam.logFun,"this argument is NULL!!\n");
		return -1;
	}

    if(serverHandle->inparam.overtime != 0)
    {
    	socketTimeOut = serverHandle->inparam.overtime;
    }else
    {
    	socketTimeOut = (NETWORK_TIME_OUT)*1000;
    }

    /*	对于阻塞的socket需要设置超时时间
     *	设置socket超时时间
     *	*/
    network_set_sendTimeout(serverHandle->inparam.logFun,fd,socketTimeOut/1000, socketTimeOut%1000);
    network_set_recvTimeout(serverHandle->inparam.logFun,fd,socketTimeOut/1000, socketTimeOut%1000);

    //此处应该记录客户端信息到全局变量中
    network_clientInfo_t *client = (network_clientInfo_t*)malloc(sizeof(network_clientInfo_t));
    if(client == NULL)
    {
    	network_printf_log(serverHandle->inparam.logFun,"malloc network_clientInfo_t error!!!");
    	return -1;
    }
    memset(client,0,sizeof(network_clientInfo_t));

    client->hashHandle = hashMap_init();

    /*客户端ip等信息*/
    client->socket = fd;
    client->clientInparam.nPort = clientPort;
    sprintf(client->clientInparam.ip,"%s",clientIp);

    /*回调函数*/
    client->clientInparam.cmdfun = serverHandle->inparam.cmdfun;
    client->clientInparam.heartmsg = serverHandle->inparam.heartmsg;
    client->clientInparam.logFun = serverHandle->inparam.logFun;
    client->clientInparam.statusFun = serverHandle->inparam.statusFun;

    /* 自定义协议头 */
    client->clientInparam.headInit = serverHandle->inparam.headInit;
    client->clientInparam.headCheck = serverHandle->inparam.headCheck;
    client->headInfo.nCustomHeadSize = serverHandle->inparam.headSize;
    if(client->headInfo.nCustomHeadSize > 0)
    {
    	client->headInfo.pstCustomRecvHead = \
    			malloc(client->headInfo.nCustomHeadSize);
    	client->headInfo.pstCustomSendHead = \
    			malloc(client->headInfo.nCustomHeadSize);
    }

    /*异步发送*/
    client->clientInparam.asynchronous = serverHandle->inparam.asynchronous;
    client->clientInparam.asynchronous_listnum = serverHandle->inparam.asynchronous_listnum;

    /*用户自定义内容*/
    client->clientInparam.param = serverHandle->inparam.param;
    client->clientInparam.overtime = serverHandle->inparam.overtime;

    client->serverHandle = serverHandle;	//记录服务端句柄
    OS_mutexCreate(&(client->socket_mutex));	//socket发送锁
    OS_mutexCreate(&(client->client_mutex));
    OS_mutexCreate(&(client->clientParam_mutex));

    client->eventHandle = serverHandle->eventHandle;

    struct timeval tv = {NETWORK_TIME_OUT,0};

    //注册事件
    client->eventFlag |= NETWORK_EVENT_READ_FLAG;
    evEvent_addIO(serverHandle->eventHandle.base,fd,EV_READABLE | EV_TIMEOUT, &tv,\
			network_server_callBack,(void*)client);

	//是否开启异步发送
	if(client->clientInparam.asynchronous == 1)
	{
		//创建队列
		OS_queCreate(&(client->asynchronousQue),client->clientInparam.asynchronous_listnum);
		client->eventFlag |= NETWORK_EVENT_WRITE_FLAG;
		evEvent_addIO(serverHandle->eventHandle.base,fd, EV_WRITABLE, NULL,\
				network_server_callBack,(void*)client);
	}

    /*默认是开启心跳*/
   	{
   		//创建一个定时器，定时发送发送心跳数据
   		/* Initalize one event */
   		tv.tv_sec = 1;
   		tv.tv_usec = 0;
   		client->eventFlag |= NETWORK_EVENT_HEART_FLAG;
   		client->heartTimeoutID = evEvent_addTime(serverHandle->eventHandle.base,\
   				&tv, network_server_heart, (void*)client);
   	}

	/*上抛状态*/
	if(client->clientInparam.statusFun)
	{
		client->clientInparam.statusFun(NETWORK_STATUS_CONNECT,client,client->clientInparam.param);
	}

    /*将新的客户端信息插入链表中*/
	OS_mutexLock(&(serverHandle->server_mutex));
	OS_listPushBack(serverHandle->clientList, client);
	network_printf_log(serverHandle->inparam.logFun,"add client ip[%s] current number[%d]",clientIp,OS_listSize(serverHandle->clientList));
	OS_mutexUnlock(&(serverHandle->server_mutex));
    return 0;
}



/*	一个新客户端连接上服务器了
 *	当此函数被调用时，libevent已经帮我们accept了这个客户端。该客户端的
 *	文件描述符为fd
 */
void networkServer_accept_ipv4(evEventBase *eventLoop,int serverFd, void *arg, int events)
{
	int clientFd = 0;
    /*客户端ip等信息*/
    char clientIp[32] = {0};
    int clientPort = 0;
	int socketTimeOut = (NETWORK_TIME_OUT)*1000;
	network_serverInfo_t *serverHandle = (network_serverInfo_t*)arg;

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsaData);
#endif

	//接收客户端的请求
	clientFd = network_tcpsocket_acceptIPV4(serverHandle->inparam.logFun,serverFd,clientIp,&clientPort);
	//添加事件到句柄中
	networkServer_accept_client(serverHandle,clientFd,socketTimeOut,clientIp,clientPort);
}


void networkServer_accept_ipv6(evEventBase *eventLoop,int serverFd, void *arg, int events)
{
#ifndef WIN32
	/*客户端ip等信息*/
	char clientIp[128] = {0};
	int clientPort = 0;			//sin6_port
	int clientFd = 0;			//连接上的客户端
	int socketTimeOut = (NETWORK_TIME_OUT)*1000;
	network_serverInfo_t *serverHandle = (network_serverInfo_t*)arg;

	//接收客户端的请求
	clientFd = network_tcpsocket_acceptIPV6(serverHandle->inparam.logFun,serverFd,clientIp,&clientPort);
	//添加事件到句柄中
	networkServer_accept_client(serverHandle,clientFd,socketTimeOut,clientIp,clientPort);
#endif
}


static int network_create_listener(network_serverInfo_t* server)
{
	int serverSocket = 0;
	serverSocket = network_tcpSocket_CreateBindFd(server->inparam.logFun,server->inparam.nPort,NULL);
	if(serverSocket < 0)
	{
		network_printf_log(server->inparam.logFun,"create socket error!!\n");
		return -1;
	}
	printf("serverSocket: %d---------------------------\n", serverSocket);

	if(listen(serverSocket , 100) < 0)
	{
		network_printf_log(server->inparam.logFun,"listen ipv4 error:%d,error msg:=%s,port[%u]", \
				errno, strerror(errno), server->inparam.nPort);
		close(serverSocket);
		return -1;
	}

    //添加监听客户端请求连接事件
	evEvent_addIO(server->eventHandle.base, serverSocket, EV_READABLE, NULL,\
			networkServer_accept_ipv4, (void*)server);
	server->acceptIPV4Socket = serverSocket;

	//是否开启ipv6
	if(server->inparam.ipv6 == 1)
	{
		int serverSocketIpv6 = 0;
		serverSocketIpv6 = network_tcpSocket_CreateBindFdIPV6(server->inparam.logFun,server->inparam.nPort,NULL);
		if(serverSocketIpv6 < 0)
		{
			network_printf_log(server->inparam.logFun,"network_tcpSocket_CreateBindFdIPV6 error!!!\n");
		}else
		{
			if(listen(serverSocketIpv6 , 100) < 0)
			{
				network_printf_log(server->inparam.logFun,"listen ipv6 error:%d,error msg:=%s,port[%u]", \
						errno, strerror(errno), server->inparam.nPort);
				close(serverSocketIpv6);
				return -1;
			}

			server->acceptIPV6Socket = serverSocketIpv6;
		    //添加监听客户端请求连接事件
			evEvent_addIO(server->eventHandle.base, serverSocketIpv6, EV_READABLE,NULL,\
					networkServer_accept_ipv6, (void*)server);
		}
	}

	return 0;
}


static int ae_time_live(evEventBase *l,void *data)
{
	/* 保活使用，防止ae模块进去永久等待,1s */
	return 1000;
}

void *network_create_server(void *argv)
{
	network_serverInfo_t *serverHandle = (network_serverInfo_t *)argv;

	evEventBase* base = evEvent_init(1024);
    serverHandle->eventHandle.base = base;
    network_printf_log(serverHandle->inparam.logFun,"event name:%s\n",evEvent_getApiName());

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsaData);
#endif

    //开启监听
    network_create_listener(serverHandle);

    /* ae保活 */
	struct timeval time;
	time.tv_sec = 1;
	time.tv_usec = 0;
	evTimeEvent* timeHandle = evEvent_addTime(base,&time,ae_time_live,NULL);

    /*调用loop循环检测事件*/
    evEvent_loop(base);

    evEvent_delTime(base,timeHandle);
    //删除监听事件
    evEvent_delIO(base,serverHandle->acceptIPV4Socket,EV_READABLE);
    evEvent_delIO(base,serverHandle->acceptIPV6Socket,EV_READABLE);

    evEvent_unInit(base);			//释放基础事件句柄

    network_printf_log(serverHandle->inparam.logFun,"exit server success!!");
	return NULL;
}


//初始化服务器
network_Handle_t network_init_server(network_inparamServerNet_t inparam)
{
	network_serverInfo_t *serverHandle = (network_serverInfo_t*)malloc(sizeof(network_serverInfo_t));
	if(serverHandle == NULL)
	{
		network_printf_log(inparam.logFun,"network init server error!!");
		return NULL;
	}

	memset(serverHandle,0,sizeof(network_serverInfo_t));
	if(inparam.asynchronous_listnum <= 0 || inparam.asynchronous_listnum > 50)
	{
		inparam.asynchronous_listnum = 15;
	}

	OS_mutexCreate(&(serverHandle->server_mutex));
	memcpy(&(serverHandle->inparam),&inparam,sizeof(network_inparamServerNet_t));

	serverHandle->clientList = OS_listCreate();

	int ret = OS_thrCreate(&(serverHandle->reatorTid),network_create_server,\
			OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT,serverHandle);
	if(ret < 0)
	{
		network_printf_log(inparam.logFun,"create pthread error!!");
		if(serverHandle)
		{
			free(serverHandle);
			serverHandle = NULL;
		}
	}

	return serverHandle;
}

/*
 * 注销服务端
 * */
int network_unInit_server(network_Handle_t server_Handle)
{
	OS_listHndl pListClient = NULL;
	OS_listNode_t pFindNode = NULL;
	OS_DataNode* pNode = NULL;
	network_clientInfo_t *client = NULL;
	network_serverInfo_t *serverHandle = (network_serverInfo_t *)server_Handle;
	if(serverHandle == NULL)
	{
		return -1;
	}

	/*关闭监听socket*/
	if(serverHandle->eventHandle.base)
	{
		evEvent_StopLoop(serverHandle->eventHandle.base);
	}

	/* 等待reator线程退出 */
	OS_thrJoin(&(serverHandle->reatorTid));

	/*	遍历所有的客户端并释放客户端
	 * */
	if(serverHandle->clientList)
	{
		OS_mutexLock(&(serverHandle->server_mutex));

		pListClient = serverHandle->clientList;
		for(pFindNode = OS_listBegin(pListClient); pFindNode != OS_listEnd(pListClient); pFindNode = OS_listNext(pListClient, pFindNode))
		{
			pNode = (OS_DataNode*)pFindNode;
			client = (network_clientInfo_t*)pNode->pData;
			/*删除链表节点*/
			OS_listEarse(serverHandle->clientList, client);
            network_unInit_client((network_Handle_t)client);
		}
		/*销毁客户端链表*/
		OS_listDestory(serverHandle->clientList);

		OS_mutexUnlock(&(serverHandle->server_mutex));
		serverHandle->clientList = NULL;
	}

	/*
	 * 销毁锁
	 * */
	OS_mutexDelete(&(serverHandle->server_mutex));

	/*
	 * free
	 * */
	if(serverHandle)
	{
		free(serverHandle);
		serverHandle = NULL;
	}

#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}



int interior_networkSend_data(network_clientInfo_t *clientHandle,\
		char *message,int nLen,int code)
{
	int ret = 0;
	int sendLen = 0;

	if((clientHandle == NULL) || \
		(clientHandle->socket <= 0) || \
		(message == NULL) || (nLen <= 0))
	{
		network_printf_log(clientHandle->clientInparam.logFun,\
				"this argumeng is error socket[%d]!!",\
				clientHandle->socket);
		return -1;
	}

	void *pHeadBuf = NULL;
	int nHeadSize = 0;
	networkHead_init(clientHandle,&pHeadBuf,&nHeadSize,nLen,code);
	if(pHeadBuf == NULL)
	{
		network_printf_log(clientHandle->clientInparam.logFun,\
				"this pHeadBuf is null socket[%d]!!",clientHandle->socket);
		return -1;
	}

	OS_mutexLock(&(clientHandle->socket_mutex));

#if 0


	/*send head*/
	sendLen = nHeadSize;
	ret = network_tcpSocket_send(clientHandle->clientInparam.logFun,\
			clientHandle->socket , (char *)pHeadBuf, &sendLen, 0);
	if(ret < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun,\
				"send to header error!!");
		ret = -1;
		goto EXIT;
	}

	/*send bady*/
	sendLen = nLen;
	ret = network_tcpSocket_send(clientHandle->clientInparam.logFun,\
			clientHandle->socket , message, &sendLen, 0);
	if(ret < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun,\
				"send to header error!!");
		ret = -1;
		goto EXIT;
	}

#else
	char *sendBuf = (char*)malloc(nHeadSize+nLen+1);
	memcpy(sendBuf,(char *)pHeadBuf,nHeadSize);
	memcpy(sendBuf+nHeadSize,message,nLen);

	sendLen = nLen+nHeadSize;
	ret = network_tcpSocket_send(clientHandle->clientInparam.logFun,\
			clientHandle->socket , sendBuf, &sendLen, 0);
	if(ret < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun,\
				"send to header error!!");
		ret = -1;
		goto EXIT;
	}
	free(sendBuf);

#endif




EXIT:
	OS_mutexUnlock(&(clientHandle->socket_mutex));

	return ret;
}



int network_send_data(network_Handle_t client_Handle,char *message,int nLen,int code)
{
	int nRet = 0;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if((clientHandle->socket <= 0) || (message == NULL) || (nLen <= 0))
	{
		network_printf_log(clientHandle->clientInparam.logFun,"this argument is error socket[%d] nLen[%d]!!",clientHandle->socket,nLen);
		return -1;
	}

	/*判断是否是异步发送*/
	if(clientHandle->clientInparam.asynchronous == 1)
	{
		int size = 0;
		network_AsynchronousInfo_t * asynchronous_messege = NULL;

		size = OS_queGetQueuedCount(&(clientHandle->asynchronousQue));
		if(OS_queIsFull(&(clientHandle->asynchronousQue)))
		{
			network_printf_log(clientHandle->clientInparam.logFun,"net_send_msg listsize is too large,size:%d pleaase check network port:%d",size, \
					clientHandle->clientInparam.nPort);
			return -1;
		}

		asynchronous_messege = (network_AsynchronousInfo_t*)malloc(sizeof(network_AsynchronousInfo_t));
		if(asynchronous_messege == NULL)
		{
			return -1;
		}
		asynchronous_messege->messege = malloc(nLen);
		if(asynchronous_messege->messege == NULL)
		{
			free(asynchronous_messege);
			return -1;
		}

		memcpy(asynchronous_messege->messege, message, nLen);
		asynchronous_messege->code = code;
		asynchronous_messege->nLen = nLen;

		OS_mutexLock(&(clientHandle->socket_mutex));
		nRet = OS_quePut(&(clientHandle->asynchronousQue), (Int64)(asynchronous_messege), OS_TIMEOUT_NONE);
		if( OS_EFAIL == nRet)
		{
			network_printf_log(clientHandle->clientInparam.logFun,"\033[31m channel put queue is faile!!\n \033[0m");
			network_freeAsynchronous_sendmessege(asynchronous_messege);
			OS_mutexUnlock(&(clientHandle->socket_mutex));
			return nRet;
		}

		/*
		 * 判断缓冲区是否是第一次有数据，若是，则需要重新添加写事件
		 * */
        
		size = OS_queGetQueuedCount(&(clientHandle->asynchronousQue));
		if(size == 0 || size == 1)
		{
			if(clientHandle->eventFlag & NETWORK_EVENT_WRITE_FLAG)
			{
				//注册一个写事件
		        evEvent_addIO(clientHandle->eventHandle.base,\
		        		clientHandle->socket, EV_WRITABLE,NULL,\
		    			network_client_callback,(void*)clientHandle);
			}
		}
	    OS_mutexUnlock(&(clientHandle->socket_mutex));

	}else
	{
		/*直接发送*/
		nRet = interior_networkSend_data(clientHandle,message,nLen,code);
	}

	return nRet;
}

int network_send_data_sync(network_Handle_t clientHandle,char *message,int nLen,int code,int timeOut,netPacket_t* outPkt)
{
	int ret = 0;
	network_clientInfo_t *client_Handle = (network_clientInfo_t *)clientHandle;
	if((client_Handle->socket <= 0) || (message == NULL) || (nLen <= 0))
	{
		network_printf_log(client_Handle->clientInparam.logFun,"this argument is error socket[%d] nLen[%d]!!",client_Handle->socket,nLen);
		return -1;
	}

	/* 以命令code作为key，不需要修改协议头
	 * 若要实现针对每条请求都返回对应的请求，则需要修改协议头（增加字段key），跟现在的协议需要做兼容，改动大
	 * 若要在协议头增加字段，最好增加身份字段，即identity，便于接收端识别身份
	 */
	hashKey key;
	key.i_key = code;
	netRpc_t value;
	memset(&value,0,sizeof(netRpc_t));
	OS_semCreate(&(value.sem),1,0);

	//TODO hashMap加锁
	ret = hashMap_insert(client_Handle->hashHandle,key,&value);
	if(ret < 0)
	{
		network_printf_log(client_Handle->clientInparam.logFun,"send data sync,insert hashmap error code[%d]!!!",code);
		return -1;
	}

	//发送
	ret = network_send_data(client_Handle,message,nLen,code);
	if(ret < 0)
	{
		network_printf_log(client_Handle->clientInparam.logFun,"send data error code[%d]!!!",code);
		ret = -1;
		goto EXIT;
	}

	//等待处理返回
	ret = OS_semWait(&(value.sem),timeOut,NULL);
	if(ret < 0)
	{
		network_printf_log(client_Handle->clientInparam.logFun,"wait sem timeout[%d]!!! code[%d]",timeOut, code);
		ret = -1;
	}else
	{
		//返回成功，返回数据
		memcpy(outPkt,&(value.pkt),sizeof(netPacket_t));
	}

EXIT:
	//删除该key值、信号量
	hashMap_delete(client_Handle->hashHandle,key);
	OS_semDelete(&(value.sem));
	return ret;
}


int network_release_syncPkt(network_Handle_t clientHandle,netPacket_t* pkt)
{
	network_clientInfo_t *client_Handle = (network_clientInfo_t *)clientHandle;
	if(client_Handle == NULL)
	{
		printf("this argument is null!!!\n");
		return -1;
	}
	if(pkt == NULL)
	{
		network_printf_log(client_Handle->clientInparam.logFun,"this pkt is null!!!\n");
	}

	if((pkt->memtype == 1) && (pkt->data))
	{
		free(pkt->data);
		pkt->data = NULL;
	}
	return 0;
}



int network_serverSendData_toAllClient(network_Handle_t serverHandle,char *message,int nLen,int code)
{
	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)serverHandle;
	OS_listHndl pListClient = NULL;
	OS_listNode_t pFindNode = NULL;
	OS_DataNode* pNode = NULL;
	int ret = 0;
	network_clientInfo_t *client = NULL;

	if(pNetServerHandle == NULL || message == NULL || nLen <= 0)
	{
		printf("this argument is NULL!!");
		return -1;
	}

	pListClient = pNetServerHandle->clientList;

	OS_mutexLock(&(pNetServerHandle->server_mutex));

	for(pFindNode = OS_listBegin(pListClient); pFindNode != OS_listEnd(pListClient); pFindNode = OS_listNext(pListClient, pFindNode))
	{
		pNode = (OS_DataNode*)pFindNode;
		client = (network_clientInfo_t*)pNode->pData;
		ret = network_send_data(client, message, nLen, code);
		if(ret < 0)
		{
			network_printf_log(pNetServerHandle->inparam.logFun,"send data to client error!!");
		}
	}

	OS_mutexUnlock(&(pNetServerHandle->server_mutex));
	return 0;
}


int network_serverGetClient_lock(network_Handle_t serverHandle)
{
	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)serverHandle;
	if(pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return -1;
	}
	return OS_mutexLock(&(pNetServerHandle->server_mutex));
}

int network_serverGetClient_unLock(network_Handle_t serverHandle)
{
	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)serverHandle;
	if(pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return -1;
	}
	return OS_mutexUnlock(&(pNetServerHandle->server_mutex));
}

OS_listNode_t network_server_listBegin(network_Handle_t serverHandle,network_Handle_t* client)
{
	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)serverHandle;
	if(pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return NULL;
	}

	OS_listNode_t pFindNode = NULL;
	OS_DataNode* pNode = NULL;
	OS_listHndl pListClient = NULL;
	pListClient = pNetServerHandle->clientList;

	pFindNode = OS_listBegin(pListClient);
	if(pFindNode)
	{
		pNode = (OS_DataNode*)pFindNode;
		*client = (network_Handle_t*)pNode->pData;
	}

	return pFindNode;
}


OS_listNode_t network_server_listEnd(network_Handle_t serverHandle)
{
	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)serverHandle;
	if(pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return NULL;
	}
	return OS_listEnd(pNetServerHandle->clientList);
}

OS_listNode_t network_server_listNext(network_Handle_t serverHandle,OS_listNode_t listNode,network_Handle_t* client)
{
	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)serverHandle;
	if(pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return NULL;
	}

	OS_listNode_t pFindNode = NULL;
	OS_DataNode* pNode = NULL;
	OS_listHndl pListClient = NULL;
	pListClient = pNetServerHandle->clientList;

	pFindNode = OS_listNext(pListClient, listNode);
	if(pFindNode)
	{
		pNode = (OS_DataNode*)pFindNode;
		*client = (network_clientInfo_t*)pNode->pData;
	}

	return pFindNode;
}




//---------------------client------------------------


int networkClient_unusual_deal(network_clientInfo_t *handle)
{
	int ret = 0;
	network_clientInfo_t *client = handle;
	if(client == NULL)
	{
		network_printf_log(client->clientInparam.logFun,"client handle is NULL!!");
		return -1;
	}

	if(client->serverHandle != NULL)
	{
		/*	客户端主动连接服务器端，
		 * 	服务端内部创建的客户端，并不是上层用户主动创建的客户端，上层用户拿不到内部创建的客户端内存地址。
		 * 	所以可以在内部释放客户端的内存资源。
		 * */
		if(client->clientInparam.nReconnect == 1)
		{
			/*重连服务器*/
            ret = network_client_reconnect((network_Handle_t)client);
		}else
		{
			/*否则关闭连接*/
			network_printf_log(client->clientInparam.logFun,"server stop client!!");
	        /*从客户端链表中删除该客户端*/
	        if(client->serverHandle)
	        {
	        	network_serverInfo_t* pNetServerHandle = (network_serverInfo_t*)client->serverHandle;
	        	OS_mutexLock(&(pNetServerHandle->server_mutex));
	        	OS_listEarse(pNetServerHandle->clientList, client);
	        	OS_mutexUnlock(&(pNetServerHandle->server_mutex));
	        }
            ret = network_unInit_client((network_Handle_t)client);
		}

	}else
	{
		/*
		 * 上层用户主动创建的客户端，去连接服务器
		 * 所以在该处程序不能主动释放客户端的内存资源，
		 * 必须上层用户主动调用network_unInit_client()释放内存资源，
		 * 否则若程序内部释放资源，同时上层用户正在调用发送接口发送数据，则会导致段错误！！！
		 * */
		if(client->clientInparam.nReconnect == 1)
		{
			/*重连服务器*/
			network_printf_log(client->clientInparam.logFun,"reconnect server!!");
            ret = network_client_reconnect((network_Handle_t)client);
		}else
		{
			/* 关闭链接
			 * 上层用户主动创建的客户端内存资源，不可在该处程序内部主动释放内存资源！！！
			 * */
			network_printf_log(client->clientInparam.logFun,"client stop to connect server!!");
			/*删除相关事件*/
			/*上抛状态*/
			if(client->clientInparam.statusFun)
			{
				client->clientInparam.statusFun(NETWOKR_STATUS_DISCONNECT,client,client->clientInparam.param);
			}
			/*删除事件*/
			if((client->eventFlag & NETWORK_EVENT_READ_FLAG) && (client->socket > 0))
			{
				evEvent_delIO(client->eventHandle.base,client->socket,EV_READABLE);
			}
		    if((client->eventFlag & NETWORK_EVENT_WRITE_FLAG) && (client->socket > 0))
		    {
		    	evEvent_delIO(client->eventHandle.base,client->socket,EV_WRITABLE);
		    }
		    if((client->eventFlag & NETWORK_EVENT_HEART_FLAG) && (client->heartTimeoutID))
		    {
		    	evEvent_delTime(client->eventHandle.base,client->heartTimeoutID);
		    	client->heartTimeoutID = NULL;
		    }
		    client->eventFlag = 0;	//清空所有事件

		    if((!client->serverHandle) && (client->eventHandle.base))
		    {
		    	evEvent_StopLoop(client->eventHandle.base);
		    }
			//关闭socket
		    if(client->socket > 0)
		    {
				close(client->socket);
				client->socket = -1;
		    }
		}
	}

	return ret;
}



void network_client_callback(evEventBase *base,int sockfb, void *arg, int events)
{
	if(events & EV_READABLE)
	{
		/*读事件*/
		networkServer_read_cb(sockfb,events,arg);

	}else if(events & EV_WRITABLE)
	{
		/*写事件*/
		networkServer_wirte_cb(sockfb,events,arg);
	}
	else
    {
    	/*socket 发生异常错误/或接收超时*/
    	network_clientInfo_t *client = (network_clientInfo_t*)arg;
    	network_printf_log(client->clientInparam.logFun,"client connect server (socket[%d]) time out!! ip[%s] port[%d] isReconnect[%d] overTime[%d] events[%d]",\
    			client->socket,client->clientInparam.ip,client->clientInparam.nPort,\
				client->clientInparam.nReconnect,client->clientInparam.overtime,events);

    	networkClient_unusual_deal(client);
    }
}



void* network_create_client(void *argv)
{
	int socketTimeOut = (NETWORK_TIME_OUT)*1000;
    network_Status_t connectStatus = NETWOKR_STATUS_DISCONNECT;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)argv;
	int sockfd = 0;

	//init hashMap
	clientHandle->hashHandle = hashMap_init();
    if(clientHandle->clientInparam.overtime != 0)
    {
    	socketTimeOut = clientHandle->clientInparam.overtime;
    }else
    {
    	socketTimeOut = (NETWORK_TIME_OUT)*1000;
    }

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsaData);
#endif

	/*连接服务器*/
	sockfd = network_tcpConnect_server(clientHandle->clientInparam.logFun,\
				clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort,2000);
    if( sockfd == -1 )
    {
    	/*后面有重连逻辑*/
    	network_printf_log(clientHandle->clientInparam.logFun,"tcp_connect error!!");
        connectStatus = NETWORK_STATUS_ERROR;
    }else
    {
    	connectStatus = NETWORK_STATUS_CONNECT;

        /*设置socket超时事件*/
        network_set_sendTimeout(clientHandle->clientInparam.logFun,sockfd,socketTimeOut/1000, socketTimeOut%1000);
        network_set_recvTimeout(clientHandle->clientInparam.logFun,sockfd,socketTimeOut/1000, socketTimeOut%1000);
    }

    //是否开启异步发送
    if(clientHandle->clientInparam.asynchronous == 1)
    {
		//创建队列
    	OS_queCreate(&(clientHandle->asynchronousQue),clientHandle->clientInparam.asynchronous_listnum);
    }

	clientHandle->socket = sockfd;	//连接上就赋值到全局变量
	evEventBase* base = evEvent_init(1024);
	clientHandle->eventHandle.base = base;

	network_printf_log(clientHandle->clientInparam.logFun,"event name:%s\n",evEvent_getApiName());


	struct timeval timeout = {NETWORK_TIME_OUT,0};
    if(sockfd > 0)
    {
		//注册读事件
		clientHandle->eventFlag |= NETWORK_EVENT_READ_FLAG;
		evEvent_addIO(base,sockfd,EV_READABLE | EV_TIMEOUT, &timeout,\
				network_client_callback,(void*)clientHandle);

		//是否开启异步发送
		if(clientHandle->clientInparam.asynchronous == 1)
		{
			clientHandle->eventFlag |= NETWORK_EVENT_WRITE_FLAG;
			evEvent_addIO(base,sockfd, EV_WRITABLE,NULL,\
					network_client_callback,(void*)clientHandle);
		}

		//创建一个定时器，定时发送发送心跳数据
		/* Initalize one event */
		timeout.tv_sec = 1;	//第一次心跳1s触发
		timeout.tv_usec = 0;
		clientHandle->eventFlag |= NETWORK_EVENT_HEART_FLAG;
		clientHandle->heartTimeoutID = evEvent_addTime(base,\
				&timeout, network_server_heart, (void*)clientHandle);

    }else
    {
    	if(clientHandle->clientInparam.nReconnect == 1)
    	{
			//开启重连
            network_client_reconnect((network_Handle_t)clientHandle);
    	}
    }

	/*上抛状态*/
	if(clientHandle->clientInparam.statusFun)
	{
		clientHandle->clientInparam.statusFun(connectStatus,\
				clientHandle,clientHandle->clientInparam.param);
	}

    /* ae保活 */
    struct timeval time;
    time.tv_sec = 1;
    time.tv_usec = 0;
    evTimeEvent* timeHandle = evEvent_addTime(base,&time,ae_time_live,NULL);

    /*事件调度*/
	evEvent_loop(base);

    evEvent_delTime(base,timeHandle);

	/*delect event*/
	evEvent_unInit(base);

    network_printf_log(clientHandle->clientInparam.logFun,"client exit loop success!!");
    return NULL;
}

network_Handle_t network_init_client(network_inparamClientNet_t inparam)
{
	if((strcmp(inparam.ip,"") == 0) || (inparam.nPort == 0))
	{
		network_printf_log(inparam.logFun,"this argument ip or port is error!!!");
		return NULL;
	}

	network_clientInfo_t *clientHandle = (network_clientInfo_t*)malloc(sizeof(network_clientInfo_t));
	
	if(clientHandle == NULL)
	{
		network_printf_log(inparam.logFun,"client malloc error!!");
		return NULL;
	}

	memset(clientHandle,0,sizeof(network_clientInfo_t));
	if(inparam.asynchronous_listnum <= 0 || inparam.asynchronous_listnum > 50)
	{
		inparam.asynchronous_listnum = 15;
	}

	OS_mutexCreate(&(clientHandle->clientParam_mutex));
	OS_mutexCreate(&(clientHandle->client_mutex));
	OS_mutexCreate(&(clientHandle->socket_mutex));
	memcpy(&(clientHandle->clientInparam),&inparam,sizeof(network_inparamClientNet_t));

	/* 创建自定义头信息 */
	if(clientHandle->clientInparam.headSize > 0)
	{
		clientHandle->headInfo.nCustomHeadSize = \
				clientHandle->clientInparam.headSize;
		clientHandle->headInfo.pstCustomRecvHead = \
				malloc(clientHandle->clientInparam.headSize);
		clientHandle->headInfo.pstCustomSendHead = \
				malloc(clientHandle->clientInparam.headSize);
	}

	clientHandle->isHaveReatorTid = 1;
	int ret = OS_thrCreate(&(clientHandle->reatorTid),network_create_client,\
			OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT,(void*)clientHandle);
	if(ret < 0)
	{
		network_printf_log(inparam.logFun,"create pthread error!!");
		if(clientHandle)
		{
			free(clientHandle);
			clientHandle = NULL;
		}
		clientHandle->isHaveReatorTid = 0;
	}

	return clientHandle;
}



static int network_reconnect_server(network_Handle_t *client_Handle)
{
	int reconnectSocket = 0;
	int socketTimeOut = 0;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if(clientHandle == NULL)
	{
		network_printf_log(clientHandle->clientInparam.logFun,"this argument is null!!!\n");
		return -1;
	}

	/*重新连接server*/
	reconnectSocket = network_tcpConnect_server(clientHandle->clientInparam.logFun,\
			clientHandle->clientInparam.ip,clientHandle->clientInparam.nPort,2000);
	clientHandle->socket = reconnectSocket;

	if(reconnectSocket < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun,"reconnect server ip[%s] port[%d] error!!",\
				clientHandle->clientInparam.ip,clientHandle->clientInparam.nPort);
		return -1;
	}else
	{
		/*上抛状态*/
		if(clientHandle->clientInparam.statusFun)
		{
			network_printf_log(clientHandle->clientInparam.logFun,"reconnect server success socket[%d] ip[%s] port[%d]!!!\n",\
					reconnectSocket,clientHandle->clientInparam.ip,clientHandle->clientInparam.nPort);
			clientHandle->clientInparam.statusFun(NETWORK_STATUS_CONNECT,clientHandle,clientHandle->clientInparam.param);
		}

	    if(clientHandle->clientInparam.overtime != 0)
	    {
	    	socketTimeOut = clientHandle->clientInparam.overtime;
	    }else
	    {
	    	socketTimeOut = (NETWORK_TIME_OUT)*1000;
	    }

	    /*设置socket超时事件*/
	    network_set_sendTimeout(clientHandle->clientInparam.logFun,\
	    		reconnectSocket,socketTimeOut/1000, socketTimeOut%1000);
	    network_set_recvTimeout(clientHandle->clientInparam.logFun,\
	    		reconnectSocket,socketTimeOut/1000, socketTimeOut%1000);
	}

	struct timeval timeout = {NETWORK_TIME_OUT,0};
    //注册事件
	clientHandle->eventFlag |= NETWORK_EVENT_READ_FLAG;
	evEvent_addIO(clientHandle->eventHandle.base,\
    		reconnectSocket,EV_READABLE | EV_TIMEOUT,&timeout,\
			network_client_callback,(void*)clientHandle);

    //是否开启异步发送
    if(clientHandle->clientInparam.asynchronous == 1)
    {
    	//注册一个写事件
    	clientHandle->eventFlag |= NETWORK_EVENT_WRITE_FLAG;
    	evEvent_addIO(clientHandle->eventHandle.base,\
        		reconnectSocket, EV_WRITABLE,NULL,\
    			network_client_callback,(void*)clientHandle);
    }

	//创建一个定时器，定时发送发送心跳数据
	/* Initalize one event */
	struct timeval timetv = {1,0};
	clientHandle->eventFlag |= NETWORK_EVENT_HEART_FLAG;
	clientHandle->heartTimeoutID = evEvent_addTime(clientHandle->eventHandle.base,\
			&timetv, network_server_heart, (void*)clientHandle);

	return 0;
}


static int network_reconnect_event(evEventBase *eventLoop, void *argv)
{
	network_clientInfo_t *client = (network_clientInfo_t*)argv;

	//重连
	if(network_reconnect_server(argv) >= 0)
	{
		//connect success
		evEvent_delTime(eventLoop,client->reconnectTimeoutID);
		client->reconnectTimeoutID = NULL;
	}

	int socketTimeOut = 0;
    if(client->clientInparam.overtime != 0)
    {
    	socketTimeOut = client->clientInparam.overtime;
    }else
    {
    	socketTimeOut = (NETWORK_TIME_OUT)*1000;
    }
	return socketTimeOut;
}



/*
 * 客户端重连服务器
 * */
int network_client_reconnect(network_Handle_t client_Handle)
{
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if(clientHandle == NULL)
	{
		return -1;
	}

	/*上抛状态*/
	if(clientHandle->clientInparam.statusFun)
	{
		clientHandle->clientInparam.statusFun(NETWORK_STATUS_RECONNECT,clientHandle,clientHandle->clientInparam.param);
	}

	/*删除事件*/
	if(clientHandle->eventFlag & NETWORK_EVENT_READ_FLAG)
	{
		if(clientHandle->socket > 0)
		{
			evEvent_delIO(clientHandle->eventHandle.base,clientHandle->socket,EV_READABLE);
		}
	}
	if(clientHandle->eventFlag & NETWORK_EVENT_WRITE_FLAG)
	{
		if(clientHandle->socket > 0)
		{
			evEvent_delIO(clientHandle->eventHandle.base,clientHandle->socket,EV_WRITABLE);
		}
	}
	if(clientHandle->eventFlag & NETWORK_EVENT_HEART_FLAG)
	{
		if(clientHandle->heartTimeoutID)
		{
			evEvent_delTime(clientHandle->eventHandle.base,clientHandle->heartTimeoutID);
			clientHandle->heartTimeoutID = NULL;
		}
	}
	if(clientHandle->eventFlag & NETWORK_EVENT_RECONNECT_FLAG)
	{
		if(clientHandle->reconnectTimeoutID)
		{
			evEvent_delTime(clientHandle->eventHandle.base,clientHandle->reconnectTimeoutID);
			clientHandle->reconnectTimeoutID = NULL;
		}
	}

	/*关闭当前的socket*/
	if(clientHandle->socket > 0)
	{
		close(clientHandle->socket);
		clientHandle->socket = -1;
	}

	/*
	 * 创建一个定时器，定时重连，不能马上重连，若不加定时器，且链接的服务器异常，则会发生不停歇的重连
	*/
	struct timeval timeout;
	int socketTimeOut = 0;
	clientHandle->eventFlag |= NETWORK_EVENT_RECONNECT_FLAG;
    if(clientHandle->clientInparam.overtime != 0)
    {
    	socketTimeOut = clientHandle->clientInparam.overtime;
    }else
    {
    	socketTimeOut = (NETWORK_TIME_OUT)*1000;
    }
    timeout.tv_sec = socketTimeOut/1000;
    timeout.tv_usec = (socketTimeOut%1000)*1000;
	clientHandle->reconnectTimeoutID = evEvent_addTime(clientHandle->eventHandle.base,\
			&timeout,network_reconnect_event, (void*)clientHandle);

	return 0;
}



/*
 * 销毁一个客户端
 * */
int network_unInit_client(network_Handle_t client_Handle)
{
	network_clientInfo_t* clientHandle = (network_clientInfo_t*)client_Handle;
	if(clientHandle == NULL)
	{
		return 0;
	}

	/*上抛状态*/
	if(clientHandle->clientInparam.statusFun)
	{
		clientHandle->clientInparam.statusFun(NETWOKR_STATUS_DISCONNECT,\
				clientHandle,clientHandle->clientInparam.param);
	}

	/*删除事件*/
	if((clientHandle->eventFlag & NETWORK_EVENT_READ_FLAG) && (clientHandle->socket > 0))
	{
		evEvent_delIO(clientHandle->eventHandle.base,clientHandle->socket,EV_READABLE);
	}
	if((clientHandle->eventFlag & NETWORK_EVENT_WRITE_FLAG) && (clientHandle->socket > 0))
	{
		evEvent_delIO(clientHandle->eventHandle.base,clientHandle->socket,EV_WRITABLE);
	}
	if(clientHandle->heartTimeoutID)
	{
		evEvent_delTime(clientHandle->eventHandle.base,clientHandle->heartTimeoutID);
		clientHandle->heartTimeoutID = NULL;
	}
    if(clientHandle->reconnectTimeoutID)
    {
    	evEvent_delTime(clientHandle->eventHandle.base,clientHandle->reconnectTimeoutID);
    	clientHandle->reconnectTimeoutID = NULL;
    }
    clientHandle->eventFlag = 0;	//清空所有事件

    if((!clientHandle->serverHandle) && clientHandle->eventHandle.base)
    {
    	evEvent_StopLoop(clientHandle->eventHandle.base);

    	/* 销毁客户端需要等待reator线程退出，
    	 * 如果是服务端连上的客户端，
    	 * 则不需要等待reator线程的退出 */
    	if(clientHandle->isHaveReatorTid)
    	{
    		OS_thrJoin(&(clientHandle->reatorTid));
    	}
    }

	//关闭socket
    if(clientHandle->socket > 0)
    {
		close(clientHandle->socket);
		clientHandle->socket = -1;
    }

    /*
     * 释放异步链表
     * */
    if(clientHandle->clientInparam.asynchronous == 1)
    {
    	//释放数据
    	network_AsynchronousInfo_t *pMessage = NULL;
    	Int64 frameAddr = 0;
    	while(!OS_queIsEmpty(&(clientHandle->asynchronousQue)))
    	{
    		if(OS_queGet(&(clientHandle->asynchronousQue),&frameAddr,OS_TIMEOUT_NONE) >= 0)
			{
				pMessage = (network_AsynchronousInfo_t *)frameAddr;
				if(pMessage)
				{
					//释放内存
					network_freeAsynchronous_sendmessege(pMessage);
					pMessage = NULL;
				}
			}
    	}
    	OS_queDelete(&(clientHandle->asynchronousQue));
    }

    /*
     * 释放hashmap
     * */
    if(clientHandle->hashHandle)
    {
    	hashMap_uninit(clientHandle->hashHandle);
    }

    /*
     * 释放锁资源
     * */
    OS_mutexDelete(&(clientHandle->socket_mutex));
    OS_mutexDelete(&(clientHandle->client_mutex));
    OS_mutexDelete(&(clientHandle->clientParam_mutex));

    /* 释放自定义头信息内存 */
    if(clientHandle->headInfo.pstCustomRecvHead)
    {
    	free(clientHandle->headInfo.pstCustomRecvHead);
    	clientHandle->headInfo.pstCustomRecvHead = NULL;
    }
    if(clientHandle->headInfo.pstCustomSendHead)
    {
    	free(clientHandle->headInfo.pstCustomSendHead);
    	clientHandle->headInfo.pstCustomSendHead = NULL;
    }
    clientHandle->headInfo.nCustomHeadSize = 0;

    /*
     * 最后释放资源
     * */
    if(clientHandle)
    {
    	free(clientHandle);
    	clientHandle = NULL;
    }
#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}






