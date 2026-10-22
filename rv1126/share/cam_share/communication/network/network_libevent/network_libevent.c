/* write by zhangjunbin on 2018/9/21
 *
 * 使用的libevent的版本为：libevent-2.1.8-stable
 *
 * 1>libevent不是线程安全的.
 *
 * 2>libevent的信号事件是不支持多线程的（因为源码里用了个全局变量）
 *
 * 3>对于不同的线程，使用不同的base，是可以的。
 *
 * 4>server的使用需要捕获signal(SIGPIPE, SIG_IGN);信号，因为当客户端主动断开，系统会上抛SIGPIPE信号，告诉上层应用，连接已断开，不要再写，
 * 	若不捕获该信号，进程会退出。或者
 * 	send()函数的最后一个参数可以设MSG_NOSIGNAL，禁止send()函数向系统发送异常消息
 * 	recv()设置此参数MSG_NOSIGNAL后,此操作不会被SIGPIPE 信号中断
 * 	返回值成功则返回接收到的字符数, 失败返回-1,错误原因存于errno 中.
 *
 *
 * 5>libevent注册的回调函数不能阻塞，否则会造成所有事件都阻塞
 *
 * 6>拓展深思：由于libevent内部采用的是Reactor模型（异步非阻塞IO），
 * 		a.目前采用的是read事件触发后，接收数据后直接回调处理函数了，如果回调函数处理有阻塞或者耗时，则会影响reactor模型的响应速度；
 * 		b.解决办法：新增工作线程池。read事件触发后，接收到数据后，将数据丢到工作线程池中工作，read事件就算完成，返回reactor模型，就不会担心回调处理阻塞了事件调度；
 *
 * */

#include <event2/listener.h>
#include <event2/thread.h>
#include <event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event_struct.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "network_libevent.h"
#include "os.h"
#include "os_thr.h"
#include "os_mutex.h"
#include "hashMap.h"
#include "os_que.h"

#define NETWORK_MAX_LEN (8192 + 1024)

#define NETWORK_CHECHK_PC_IDEENTIFIER_FIRST_BIT '@'
#define NETWORK_CHECHK_PC_IDEENTIFIER_SECOND_BIT '#'
#define NETWORK_CHECHK_PC_IDEENTIFIER_THIRD_BIT '$'
#define NETWORK_CHECHK_PC_IDEENTIFIER_FOURTH_BIT '&'

#define NETWORK_MSG_HEAD_VERSION (2015)

//-------------------------------------------------

typedef struct _LIBEVENT_ARGV_INFO
{
	struct event_base *base; // 基础事件调度句柄
	struct event accept;	 // 监听事件ipv4
	struct event acceptIPV6; // 监听事件ipv6

} network_event_t;

#define NETWORK_EVENT_READ_FLAG (0x1)	   // 0000 0001
#define NETWORK_EVENT_WRITE_FLAG (0x2)	   // 0000 0010
#define NETWORK_EVENT_HEART_FLAG (0x4)	   // 0000 0100
#define NETWORK_EVENT_RECONNECT_FLAG (0x8) // 0000 1000

typedef struct _NETWORK_SERVER_CLIENT_INFO_
{
	int socket;
	network_inparamClientNet_t clientInparam; // 客户端信息
	struct event readEv;					  // 读事件
	struct event writeEv;					  // 写事件（异步发送）
	struct event heartTimeoutEv;			  // 心跳定时器事件
	struct event reconnectEv;				  // 定时重连事件
	unsigned int eventFlag;					  // 事件是否启动，bit表示，1-启动，0-未启动事件

	/*libevent句柄*/
	network_event_t libeventHandle;
	OS_QueHndl asynchronousQue;	 // 异步发送队列
	void *serverHandle;			 // 服务端句柄
	network_HeadInfo_t headinfo; // 头信息

	// hashMap
	hashMapHandle_t *hashHandle; // hash map

	OS_MutexHndl socket_mutex; // socket发送锁
	OS_MutexHndl client_mutex;
} network_clientInfo_t;

typedef struct _NETWORK_SERVER_INFO_
{
	/*服务端传入参数*/
	network_inparamServerNet_t inparam;
	/*libevent句柄*/
	network_event_t libeventHandle;
	/*连接上服务器的客户端链表*/
	OS_listHndl clientList; // network_clientInfo_t
	OS_MutexHndl server_mutex;
} network_serverInfo_t;

typedef struct Network_Asynchronous_Info
{
	char *messege;
	int nLen;
	int code;
} network_AsynchronousInfo_t;

//-----------------------------------------

int networkClient_unusual_deal(network_clientInfo_t *handle);
/*内部使用发送函数*/
static int interior_networkSend_data(network_clientInfo_t *clientHandle, char *message, int nLen, int code);

/*打印函数*/
#define network_printf_log(logFun, format, args...)                                 \
	if (logFun)                                                                     \
	{                                                                               \
		logFun("\033[31m[%s:%d]\033[0m" format "\r\n", __func__, __LINE__, ##args); \
	}                                                                               \
	else                                                                            \
	{                                                                               \
		printf("\033[31m[%s:%d]\033[0m" format "\r\n", __func__, __LINE__, ##args); \
	}

static int network_tcpSocket_CreateBindFd(network_Logbackmsg log, int LocalPort, char *LocalIp)
{
	int Fd = -1;
	if (LocalPort < 0 || LocalPort == 0)
	{
		network_printf_log(log, "<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>", Fd, LocalPort, LocalIp);
		return -1;
	}

	struct sockaddr_in LocalAddr;
	bzero(&LocalAddr, sizeof(LocalAddr));

	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_port = htons(LocalPort);
	if (LocalIp)
	{
		LocalAddr.sin_addr.s_addr = inet_addr((const char *)LocalIp);
	}
	else
	{
		LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	Fd = socket(AF_INET, SOCK_STREAM, 0);
	if (Fd < 0)
	{
		network_printf_log(log, "<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>", Fd, strerror(errno), errno);
		return errno;
	}

	/*设置socket释放后可马上重用*/
	int opt = 1;
	if (setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		network_printf_log(log, "<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>", Fd, strerror(errno), errno);
		return errno;
	}

	/*bind地址*/
	if (bind(Fd, (struct sockaddr *)&LocalAddr, sizeof(LocalAddr)) < 0)
	{
		network_printf_log(log, "<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_%s> <ERROR_%d> <LocalPort :%d> <LocalIp :%s>",
						   Fd, strerror(errno), errno, LocalPort, LocalIp);
		return errno;
	}

	return Fd;
}

static int network_tcpSocket_CreateBindFdIPV6(network_Logbackmsg log, int LocalPort, char *LocalIp)
{
	int Fd = -1;
	if (LocalPort < 0 || LocalPort == 0)
	{
		network_printf_log(log, "<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>\n", Fd, LocalPort, LocalIp);
		return -1;
	}

	struct sockaddr_in6 LocalAddr;
	bzero(&LocalAddr, sizeof(LocalAddr));

	LocalAddr.sin6_family = PF_INET6; // IPv6
	LocalAddr.sin6_port = htons(LocalPort);

	if (LocalIp)
	{
		inet_pton(AF_INET6, LocalIp, &LocalAddr.sin6_addr); // IPv6
	}
	else
	{
		LocalAddr.sin6_addr = in6addr_any;
	}

	Fd = socket(PF_INET6, SOCK_STREAM, 0);
	if (Fd < 0)
	{
		network_printf_log(log, "<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		return -1;
	}

	int opt = 1;

	if (setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		network_printf_log(log, "<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		if (Fd > 2)
		{
			close(Fd);
		}
		return -1;
	}

	int on = 1;
	if (setsockopt(Fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
	{
		network_printf_log(log, "setsockopt");
		return -1;
	}

	if (bind(Fd, (struct sockaddr *)&LocalAddr, sizeof(LocalAddr)) < 0)
	{
		network_printf_log(log, "<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_%s> <ERROR_%d> <LocalPort :%d> <LocalIp :%s>\n",
						   Fd, strerror(errno), errno, LocalPort, LocalIp);
		if (Fd > 2)
		{
			close(Fd);
		}
		return -1;
	}

	return Fd;
}

static int network_tcpConnect_serverIPV4(network_Logbackmsg log, const char *server_ip, int port)
{
	int sockfd = 0, status = 0;
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	status = inet_aton(server_ip, &server_addr.sin_addr);
	if (status == 0)
	{
		network_printf_log(log, "inet_aton error!!");
		return -1;
	}

	/*
	 * 创建的socket默认是阻塞的
	 * */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		network_printf_log(log, "create socket error!!");
		return sockfd;
	}

	status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (status == -1)
	{
		close(sockfd);
		return -1;
	}

	return sockfd;
}

static int network_tcpConnect_serverIPV6(network_Logbackmsg log, const char *server_ip, int port, int Timeout)
{
	int sockfd = 0, status = 0;
	struct sockaddr_in6 serv_addr = {0};
	unsigned long ul = 1;

	memset(&serv_addr, 0, sizeof(struct sockaddr_in6));

	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_port = htons(port);
	if (inet_pton(AF_INET6, server_ip, &serv_addr.sin6_addr) < 0)
	{
		// IPv6
		network_printf_log(log, "inet_aton error!!");
		return -1;
	}

	/*
	 * 创建的ipv6 socket默认是阻塞的
	 * */
	sockfd = socket(PF_INET6, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		network_printf_log(log, "create ipv6 socket error!!");
		return sockfd;
	}

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		struct timeval Time;
		fd_set set;
		int RetSelect = 0;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		Time.tv_sec = Timeout / 1000;
		Time.tv_usec = (Timeout % 1000) * 1000;
		RetSelect = select(sockfd + 1, NULL, &set, NULL, &Time);
		if (RetSelect > 0)
		{
			int len = sizeof(int);
			int err = 0;
			int ret = 0;
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)(&err), (socklen_t *)&len);
			if (err == 0)
			{
				network_printf_log(log, "RH_ConnetBlockFd is successful** : %u\n", errno);
				ret = -1;
			}
			else
			{
				network_printf_log(log, "RH_ConnetBlockFd is fail :%u ServIp:%s ServPort:%d\n", errno, server_ip, port);
				ret = -1;
			}

			ul = 0;
			ioctl(sockfd, FIONBIO, &ul); // 设置为阻塞模式,可设置超时
			FD_CLR(sockfd, &set);

			return ret;
		}
		else
		{
			network_printf_log(log, "select error!!\n");
			return -1;
		}
	}

	ul = 0;
	ioctl(sockfd, FIONBIO, &ul); // 设置为阻塞模式,可设置超时

	return sockfd;
}

static int network_tcpConnect_server(network_Logbackmsg log, const char *server_ip, int port, int Timeout)
{
	int sockfd = 0;
	int isIpv6 = 0;

	// 判断是否是ipv6，后期优化，通过正则表达式判断ipv4/ipv6
	if (strlen(server_ip) > 16)
	{
		isIpv6 = 1;
	}

	/*连接服务器*/
	if (isIpv6 == 1)
	{
		sockfd = network_tcpConnect_serverIPV6(log, server_ip, port, Timeout);
	}
	else
	{
		sockfd = network_tcpConnect_serverIPV4(log, server_ip, port);
	}

	return sockfd;
}

static int network_set_recvTimeout(network_Logbackmsg log, int Fd, int TimeoutSec, int TimeoutUsec)
{
	if (Fd < 0 || TimeoutSec < 0 || TimeoutUsec < 0)
	{
		network_printf_log(log,
						   "<network_set_recvTimeout IS ERROR>  <FD : %d>   <TimeoutSec :%d> <TimeoutUsec :%d>",
						   Fd, TimeoutSec, TimeoutUsec);
		return -1;
	}

	struct timeval Time;
	Time.tv_sec = TimeoutSec;
	Time.tv_usec = TimeoutUsec;

	if (setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&Time, sizeof(struct timeval)) < 0)
	{
		network_printf_log(log,
						   "<network_set_recvTimeout IS ERROR> <SO_RCVTIMEO> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <TimeoutSec :%d> <TimeoutUsec :%d>",
						   Fd, strerror(errno), errno, TimeoutSec, TimeoutUsec);
		return errno;
	}

	return 0;
}

/*
 * 设置socket的接收超时时间，
 * 住：对于阻塞的socket有效，非阻塞的socket设置无效
 *
 * */
static int network_set_sendTimeout(network_Logbackmsg log, int Fd, int TimeoutSec, int TimeoutUsec)
{
	if (Fd < 0 || TimeoutSec < 0 || TimeoutUsec < 0)
	{
		network_printf_log(log,
						   "<network_set_sendTimeout IS ERROR>  <FD : %d>   <TimeoutSec :%d> <TimeoutUsec :%d>",
						   Fd, TimeoutSec, TimeoutUsec);

		return -1;
	}

	struct timeval Time;
	Time.tv_sec = TimeoutSec;
	Time.tv_usec = TimeoutUsec;

	if (setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&Time, sizeof(struct timeval)) < 0)
	{
		network_printf_log(log,
						   "<network_set_sendTimeout IS ERROR> <SO_SNDTIMEO> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <TimeoutSec :%d> <TimeoutUsec :%d>",
						   Fd, strerror(errno), errno, TimeoutSec, TimeoutUsec);

		return errno;
	}

	return 0;
}

/*
 * 设置socket的接收超时时间，
 * 住：对于阻塞的socket有效，非阻塞的socket设置无效
 *
 * */
static int network_tcpSocket_send(network_Logbackmsg log, int socket, char *SndBuf, int *SndLen, int Timeout)
{
	int SndTotalLen = 0;
	int SndBytes = 0;
	int SndTempLen = *SndLen;
	*SndLen = 0;

	while (SndTotalLen < SndTempLen)
	{
		SndBytes = send(socket, SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0);
		if (SndBytes <= 0)
		{
			network_printf_log(log,
							   "<Snd > <sendlen :%d> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>",
							   SndTempLen - SndTotalLen, strerror(errno), errno, socket);
			return errno;
		}
		else
		{
			SndTotalLen += SndBytes;
			*SndLen = SndTotalLen;
		}
	}

	return 0;
}

static int network_tcpSocket_recv(network_Logbackmsg log, int socket, char *RcvBuf, int *RcvLen, int Timeout)
{

	int RcvTotalLen = 0;
	int RcvBytes = 0;
	int RcvTempLen = *RcvLen;
	int recvError = 0;

	*RcvLen = 0;
	while (RcvTotalLen < RcvTempLen)
	{

		RcvBytes = recv(socket, RcvBuf + RcvTotalLen, RcvTempLen - RcvTotalLen, 0);
		if (RcvBytes < 0)
		{
			if (11 == errno)
			{
				if (recvError++ < 3)
				{
					network_printf_log(log,
									   "<Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <sockfd : %d> num[%d] RcvTotalLen[%d] RcvTempLen[%d] continue",
									   strerror(errno), errno, socket, recvError, RcvTotalLen, RcvTempLen);
					continue;
				}
			}

			network_printf_log(log,
							   "<Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <sockfd : %d>",
							   strerror(errno), errno, socket);

			return errno;
		}
		else if (RcvBytes == 0)
		{
			return errno;
		}
		else
		{
			RcvTotalLen += RcvBytes;
			*RcvLen = RcvTotalLen;
		}
	}

	return 0;
}

//-----------------------------------server-----------------------------------------

static int network_check_netHead(network_HeadInfo_t *head)
{
	if ((head == NULL) || (head->identifier[0] != NETWORK_CHECHK_PC_IDEENTIFIER_FIRST_BIT) || (head->identifier[1] != NETWORK_CHECHK_PC_IDEENTIFIER_SECOND_BIT) || (head->identifier[2] != NETWORK_CHECHK_PC_IDEENTIFIER_THIRD_BIT) || (head->identifier[3] != NETWORK_CHECHK_PC_IDEENTIFIER_FOURTH_BIT))
	{
		return -1;
	}

	if (head->load_len <= 0)
	{
		return -1;
	}

	return 0;
}

static int network_init_head(network_HeadInfo_t *head)
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

void networkServer_read_cb(int sockfb, short events, void *arg)
{

	int ret = 0;
	network_clientInfo_t *client = (network_clientInfo_t *)arg;
	network_HeadInfo_t headbuf;
	struct event *ev = &(client->readEv);
	int recvLen = 0;
	int maxLongBufLen = 0;
	char *pRecvLongBuf = NULL;
	char recvshortBuf[NETWORK_MAX_LEN];
	char *recvPoint = NULL;
	network_CallbackMsg_t dealCmd;

	memset(&dealCmd, 0, sizeof(network_CallbackMsg_t));
	memset(&headbuf, 0, sizeof(network_HeadInfo_t));
	recvLen = sizeof(network_HeadInfo_t);

	// read header
	ret = network_tcpSocket_recv(client->clientInparam.logFun, sockfb, (char *)&headbuf, &recvLen, 0);
	if (recvLen != sizeof(network_HeadInfo_t))
	{
		network_printf_log(client->clientInparam.logFun, "socket[%d] recv header error recvlen[%d] client[%p]", sockfb, recvLen, client);
		networkClient_unusual_deal(client);
		return;
	}

	// check head
	ret = network_check_netHead(&headbuf);
	if (ret < 0)
	{
		network_printf_log(client->clientInparam.logFun, "this header is error!!");
		networkClient_unusual_deal(client);
		return;
	}
	client->headinfo.reserve = headbuf.reserve;

	if (headbuf.load_len >= NETWORK_MAX_LEN)
	{
		if (headbuf.load_len >= maxLongBufLen)
		{
			if (pRecvLongBuf)
			{
				free(pRecvLongBuf);
				pRecvLongBuf = NULL;
			}
			pRecvLongBuf = (char *)malloc(headbuf.load_len);
			if (pRecvLongBuf == NULL)
			{
				network_printf_log(client->clientInparam.logFun, "%s %s %d,malloc  long recvbuf is fail", __func__, __FILE__, __LINE__);
				return;
			}
			maxLongBufLen = headbuf.load_len;
		}

		recvPoint = pRecvLongBuf;
	}
	else
	{
		recvPoint = recvshortBuf;
		recvshortBuf[headbuf.load_len] = '\0';
	}
	recvLen = headbuf.load_len;

	/*接收数据*/
	ret = network_tcpSocket_recv(client->clientInparam.logFun, sockfb, recvPoint, &recvLen, 0);
	if (recvLen != headbuf.load_len)
	{
		network_printf_log(client->clientInparam.logFun, "recv body error recvlen[%d]", recvLen);
		networkClient_unusual_deal(client);
		return;
	}

	dealCmd.Code = headbuf.msg_code;
	dealCmd.InParam = client->clientInparam.param;
	dealCmd.recvvalue = recvPoint;
	dealCmd.nLen = headbuf.load_len;
	dealCmd.clientHandle = client;

	/*修改为心跳信息直接上抛，客户端，服务端都各自发送心跳*/

	// 判断是否是同步获取返回的请求
	hashKey key;
	key.i_key = headbuf.msg_code;
	netRpc_t *value = NULL;

	if ((headbuf.msg_code != NETWORK_NET_HEARTBIT_CMD) && (value = hashMap_find(client->hashHandle, key)))
	{
		// 同步返回
		value->pkt.dataSize = headbuf.load_len;
		value->pkt.data = malloc(headbuf.load_len);
		memcpy(value->pkt.data, recvPoint, headbuf.load_len);
		value->pkt.memtype = 1; // 需要用户调用释放函数释放内存:network_release_syncPkt();
		OS_semSignal(&(value->sem));
	}
	else if (client->clientInparam.cmdfun)
	{
		/*异步返回*/
		/*调用回调函数处理*/
		client->clientInparam.cmdfun(&dealCmd);
	}

EXIT:

	/*判断是否有大内存需要释放*/
	if (pRecvLongBuf)
	{
		free(pRecvLongBuf);
		pRecvLongBuf = NULL;
	}
}

/*
 * libevent回调写函数,异步读写
 * */
int network_freeAsynchronous_sendmessege(network_AsynchronousInfo_t *asynchronous_sendmessege)
{
	if (asynchronous_sendmessege)
	{
		if (asynchronous_sendmessege->messege)
		{
			free(asynchronous_sendmessege->messege);
		}
		free(asynchronous_sendmessege);
	}
	return 0;
}

/*使用写事件，若频率太高，会多占用一点cpu资源*/
void networkServer_wirte_cb(int sockfb, short events, void *arg)
{
	int listSize = 0;
	network_clientInfo_t *client = (network_clientInfo_t *)arg;
	struct event *writeEv = &(client->writeEv);
	network_AsynchronousInfo_t *pMessage = NULL;
	Int64 frameAddr;
	int ret = 0;

	if (client->clientInparam.asynchronous == 1)
	{
		if (OS_queGet(&(client->asynchronousQue), &frameAddr, OS_TIMEOUT_NONE) >= 0)
		{
			pMessage = (network_AsynchronousInfo_t *)frameAddr;
			if (pMessage)
			{
				interior_networkSend_data(client, pMessage->messege, pMessage->nLen, pMessage->code);

				// 释放内存
				network_freeAsynchronous_sendmessege(pMessage);
			}
		}

		/*	判断队列中是否还有数据，
		 * 	若有，则继续添加写事件
		 * 	若无，则不添加写事件，
		 * 	*/
		listSize = OS_queGetQueuedCount(&(client->asynchronousQue));
		if (listSize > 0)
		{
			/*有数据待发送，再次添加写事件*/
			event_add(writeEv, NULL);
		}
	}
}

void network_server_callBack(int sockfb, short events, void *arg)
{
	if ((events & EV_READ))
	{
		/*读事件*/
		networkServer_read_cb(sockfb, events, arg);
	}
	else if ((events & EV_WRITE))
	{
		/*写事件*/
		networkServer_wirte_cb(sockfb, events, arg);
	}
	else if ((events & EV_TIMEOUT))
	{
		/*	连接超时事件，
		 * 	若不del事件同时没有数据到来，一直会超时*/

		/* 客户端没有数据到来，断开连接，
		 * 该句柄是服务器端主动创建的内存资源，所以可以在该处主动释放客户端内存资源
		 * */
		network_clientInfo_t *client = (network_clientInfo_t *)arg;
		network_printf_log(client->clientInparam.logFun, "connection time out! ip[%s] port[%d]",
						   client->clientInparam.ip, client->clientInparam.nPort);
		/*从客户端链表中删除该客户端*/
		if (client->serverHandle)
		{
			network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)client->serverHandle;
			OS_mutexLock(&(pNetServerHandle->server_mutex));
			OS_listEarse(pNetServerHandle->clientList, client);
			OS_mutexUnlock(&(pNetServerHandle->server_mutex));
		}
		network_unInit_client((network_Handle_t *)client);
	}
	else
	{
		/* socket关闭或错误
		 * 该句柄是服务器端主动创建的内存资源，所以可以在该处主动释放客户端内存资源
		 * */
		network_clientInfo_t *client = (network_clientInfo_t *)arg;
		network_printf_log(client->clientInparam.logFun, "client connect error!! so close client!! ip[%s] port[%d]",
						   client->clientInparam.ip, client->clientInparam.nPort);
		/*从客户端链表中删除该客户端*/
		if (client->serverHandle)
		{
			network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)client->serverHandle;
			OS_mutexLock(&(pNetServerHandle->server_mutex));
			OS_listEarse(pNetServerHandle->clientList, client);
			OS_mutexUnlock(&(pNetServerHandle->server_mutex));
		}
		network_unInit_client((network_Handle_t *)client);
	}
}

void network_server_heart(int sockfb, short events, void *argv)
{
	char HeartBitBUF[4096] = {0};
	network_clientInfo_t *client = (network_clientInfo_t *)argv;
	int sendLen = 0;
	int inputLen = sizeof(HeartBitBUF);

	if (client->clientInparam.heartmsg)
	{
		client->clientInparam.heartmsg(HeartBitBUF, sizeof(HeartBitBUF), client, client->clientInparam.param, &sendLen);
		if (sendLen <= 0 || sendLen > inputLen)
		{
			sendLen = inputLen;
		}
	}
	else
	{
		/*上层应用没有自定义的心跳包，则默认发心跳包*/
		sprintf(HeartBitBUF, "this is network heart message");
		sendLen = strlen(HeartBitBUF);
	}
	if (interior_networkSend_data(client, HeartBitBUF, sendLen, NETWORK_NET_HEARTBIT_CMD) < 0)
	{
		network_printf_log(client->clientInparam.logFun, "send heart error!!\n");
	}

	if (client->eventFlag & NETWORK_EVENT_HEART_FLAG)
	{
		struct timeval tv;
		evutil_timerclear(&tv);
		tv.tv_sec = NETWORK_HEART_INTERVAL_TIME;
		tv.tv_usec = 0;
		event_add(&(client->heartTimeoutEv), &tv);
	}

	return;
}

static int networkServer_accept_client(network_serverInfo_t *serverHandle, int fd, int socketTimeOut, char *clientIp, int clientPort)
{
	int ret = 0;
	if (serverHandle == NULL)
	{
		network_printf_log(serverHandle->inparam.logFun, "this argument is NULL!!\n");
		return -1;
	}

	if (serverHandle->inparam.overtime != 0)
	{
		socketTimeOut = serverHandle->inparam.overtime;
	}
	else
	{
		socketTimeOut = (NETWORK_TIME_OUT) * 1000;
	}

	/*	对于阻塞的socket需要设置超时时间
	 *	设置socket超时时间
	 *	*/
	network_set_sendTimeout(serverHandle->inparam.logFun, fd, socketTimeOut / 1000, socketTimeOut % 1000);
	network_set_recvTimeout(serverHandle->inparam.logFun, fd, socketTimeOut / 1000, socketTimeOut % 1000);

	// 此处应该记录客户端信息到全局变量中
	network_clientInfo_t *client = (network_clientInfo_t *)malloc(sizeof(network_clientInfo_t));
	if (client == NULL)
	{
		network_printf_log(serverHandle->inparam.logFun, "malloc network_clientInfo_t error!!!");
		return -1;
	}
	memset(client, 0, sizeof(network_clientInfo_t));

	client->hashHandle = hashMap_init();

	/*客户端ip等信息*/
	client->socket = fd;
	client->clientInparam.nPort = clientPort;
	sprintf(client->clientInparam.ip, "%s", clientIp);

	/*回调函数*/
	client->clientInparam.cmdfun = serverHandle->inparam.cmdfun;
	client->clientInparam.heartmsg = serverHandle->inparam.heartmsg;
	client->clientInparam.logFun = serverHandle->inparam.logFun;
	client->clientInparam.statusFun = serverHandle->inparam.statusFun;

	/*异步发送*/
	client->clientInparam.asynchronous = serverHandle->inparam.asynchronous;
	client->clientInparam.asynchronous_listnum = serverHandle->inparam.asynchronous_listnum;

	/*用户自定义内容*/
	client->clientInparam.param = serverHandle->inparam.param;
	client->clientInparam.overtime = serverHandle->inparam.overtime;

	client->serverHandle = serverHandle; // 记录服务端句柄

	OS_mutexCreate(&(client->socket_mutex)); // socket发送锁
	OS_mutexCreate(&(client->client_mutex)); // 锁

	// 将动态创建的结构体作为event的回调参数
	client->eventFlag |= NETWORK_EVENT_READ_FLAG;
	event_assign(&(client->readEv), serverHandle->libeventHandle.base, fd, EV_READ | EV_TIMEOUT | EV_PERSIST | EV_CLOSED,
				 network_server_callBack, (void *)client);

	// 注册读事件，6s没读到数据，则表示网络断开等异常
	struct timeval tv = {NETWORK_TIME_OUT, 0};
	event_add(&(client->readEv), &tv);

	// 是否开启异步发送
	if (client->clientInparam.asynchronous == 1)
	{
		// 创建队列
		OS_queCreate(&(client->asynchronousQue), client->clientInparam.asynchronous_listnum);

		client->eventFlag |= NETWORK_EVENT_WRITE_FLAG;
		// 添加注册函数，第一次只触发一次，链表有数据再添加事件
		event_assign(&(client->writeEv), serverHandle->libeventHandle.base, fd, EV_WRITE,
					 network_server_callBack, (void *)client);
		event_add(&(client->writeEv), NULL);
	}

	/*默认是开启心跳*/
	{
		// 创建一个定时器，定时发送发送心跳数据
		/* Initalize one event */
		client->eventFlag |= NETWORK_EVENT_HEART_FLAG;
		event_assign(&(client->heartTimeoutEv), serverHandle->libeventHandle.base, -1, 0, network_server_heart, (void *)client);

		struct timeval timetv;
		evutil_timerclear(&timetv);
		timetv.tv_sec = 0; // 第一次的心跳是马上触发的
		event_add(&(client->heartTimeoutEv), &timetv);
	}

	/*上抛状态*/
	if (client->clientInparam.statusFun)
	{
		client->clientInparam.statusFun(NETWORK_STATUS_CONNECT, client, client->clientInparam.param);
	}

	/*将新的客户端信息插入链表中*/
	OS_mutexLock(&(serverHandle->server_mutex));
	OS_listPushBack(serverHandle->clientList, client);
	network_printf_log(serverHandle->inparam.logFun, "add client ip[%s] current number[%d]", clientIp, OS_listSize(serverHandle->clientList));
	OS_mutexUnlock(&(serverHandle->server_mutex));

	return 0;
}

/*	一个新客户端连接上服务器了
 *	当此函数被调用时，libevent已经帮我们accept了这个客户端。该客户端的
 *	文件描述符为fd
 */
void networkServer_accept_ipv4(int serverFd, short events, void *arg)
{
	int clientFd = 0; // 连接上的客户端
	int socketTimeOut = (NETWORK_TIME_OUT) * 1000;

	network_serverInfo_t *serverHandle = (network_serverInfo_t *)arg;

	// 接收客户端的请求
	struct sockaddr_in clientSocket;
	socklen_t len = sizeof(clientSocket);

	clientFd = accept(serverFd, (struct sockaddr *)&clientSocket, &len);

	/*客户端ip等信息*/
	char clientIp[32] = {0};
	int ipv4IPLen = sizeof(clientIp);
	int clientPort = 0;
	clientPort = ntohs(clientSocket.sin_port);
	inet_ntop(AF_INET, &(clientSocket.sin_addr), clientIp, ipv4IPLen);

	network_printf_log(serverHandle->inparam.logFun, "accept ipv4 a new client %d %d clientIp[%s] port[%d]",
					   clientFd, serverFd, clientIp, clientPort);

	// 添加事件到句柄中
	networkServer_accept_client(serverHandle, clientFd, socketTimeOut, clientIp, clientPort);
}

void networkServer_accept_ipv6(int serverFd, short events, void *arg)
{
	int ret = 0;
	int clientFd = 0; // 连接上的客户端
	int socketTimeOut = (NETWORK_TIME_OUT) * 1000;

	network_serverInfo_t *serverHandle = (network_serverInfo_t *)arg;

	// 接收客户端的请求
	struct sockaddr_in6 clientIPV6Addr;
	int len = sizeof(clientIPV6Addr);
	clientFd = accept(serverFd, (void *)&clientIPV6Addr, (socklen_t *)&len);

	/*客户端ip等信息*/
	char clientIp[128] = {0};
	int ipv6IPLen = sizeof(clientIp);
	int clientPort = 0; // sin6_port
	struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&clientIPV6Addr;
	inet_ntop(AF_INET6, &(addr->sin6_addr), clientIp, ipv6IPLen);
	clientPort = ntohs(addr->sin6_port);

	network_printf_log(serverHandle->inparam.logFun, "accept ipv6 a new client %d %d", clientFd, serverFd);

	// 添加事件到句柄中
	networkServer_accept_client(serverHandle, clientFd, socketTimeOut, clientIp, clientPort);
}

static int network_create_listener(network_serverInfo_t *server)
{
	int serverSocket = 0;
	serverSocket = network_tcpSocket_CreateBindFd(server->inparam.logFun, server->inparam.nPort, NULL);
	if (serverSocket < 0)
	{
		network_printf_log(server->inparam.logFun, "create socket error!!\n");
		return -1;
	}

	if (listen(serverSocket, 100) < 0)
	{
		network_printf_log(server->inparam.logFun, "listen ipv4 error:%d,error msg:=%s,port[%u]",
						   errno, strerror(errno), server->inparam.nPort);
		close(serverSocket);
		return -1;
	}

	// 添加监听客户端请求连接事件
	event_assign(&(server->libeventHandle.accept), server->libeventHandle.base, serverSocket, EV_READ | EV_PERSIST,
				 networkServer_accept_ipv4, server);
	event_add(&(server->libeventHandle.accept), NULL);

	// 是否开启ipv6
	if (server->inparam.ipv6 == 1)
	{
		int serverSocketIpv6 = 0;
		serverSocketIpv6 = network_tcpSocket_CreateBindFdIPV6(server->inparam.logFun, server->inparam.nPort, NULL);
		if (serverSocketIpv6 < 0)
		{
			network_printf_log(server->inparam.logFun, "network_tcpSocket_CreateBindFdIPV6 error!!!\n");
		}
		else
		{
			if (listen(serverSocketIpv6, 100) < 0)
			{
				network_printf_log(server->inparam.logFun, "listen ipv6 error:%d,error msg:=%s,port[%u]",
								   errno, strerror(errno), server->inparam.nPort);
				close(serverSocketIpv6);
				return -1;
			}

			// 创建成功ipv6
			// 添加监听客户端请求连接事件
			event_assign(&(server->libeventHandle.acceptIPV6), server->libeventHandle.base, serverSocketIpv6, EV_READ | EV_PERSIST,
						 networkServer_accept_ipv6, server);
			event_add(&(server->libeventHandle.acceptIPV6), NULL);
		}
	}

	return 0;
}

void *network_create_server(void *argv)
{
	int ret = 0;
	network_serverInfo_t *serverHandle = (network_serverInfo_t *)argv;

	/*
	 * 只有使用libevent的线程，才能释放base循环
	 * (event_base_dispatch()才会退出)
	 * */
	evthread_use_pthreads();

	struct event_base *base = event_base_new();
	serverHandle->libeventHandle.base = base;

	// 开启监听
	network_create_listener(serverHandle);

	/*调用loop循环检测事件*/
	event_base_dispatch(base);

	// 删除监听事件
	event_del(&(serverHandle->libeventHandle.accept));
	event_del(&(serverHandle->libeventHandle.acceptIPV6));

	event_base_free(base); // 释放基础事件句柄
	serverHandle->libeventHandle.base = NULL;

	network_printf_log(serverHandle->inparam.logFun, "exit server success!!");

	return NULL;
}

// 初始化服务器
network_Handle_t network_init_server(network_inparamServerNet_t inparam)
{
	OS_ThrHndl tid;
	int ret = 0;
	network_serverInfo_t *serverHandle = (network_serverInfo_t *)malloc(sizeof(network_serverInfo_t));
	if (serverHandle == NULL)
	{
		network_printf_log(inparam.logFun, "network init server error!!");
		return NULL;
	}

	memset(serverHandle, 0, sizeof(network_serverInfo_t));
	if (inparam.asynchronous_listnum <= 0 || inparam.asynchronous_listnum > 50)
	{
		inparam.asynchronous_listnum = 15;
	}

	OS_mutexCreate(&(serverHandle->server_mutex));
	memcpy(&(serverHandle->inparam), &inparam, sizeof(network_inparamServerNet_t));

	serverHandle->clientList = OS_listCreate();

	ret = OS_thrCreate(&tid, network_create_server, OS_DETACH, OS_THR_STACK_SIZE_DEFAULT, serverHandle);
	if (ret < 0)
	{
		network_printf_log(inparam.logFun, "create pthread error!!");
		if (serverHandle)
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
int network_unInit_server(network_Handle_t *server_Handle)
{
	OS_listHndl pListClient = NULL;
	OS_listNode_t pFindNode = NULL;
	OS_DataNode *pNode = NULL;
	int ret = 0;
	network_clientInfo_t *client = NULL;
	network_serverInfo_t *serverHandle = (network_serverInfo_t *)server_Handle;
	if (serverHandle == NULL)
	{
		return -1;
	}

	/*关闭监听socket*/
	if (serverHandle->libeventHandle.base)
	{
		event_base_loopbreak(serverHandle->libeventHandle.base);
	}

	/*	遍历所有的客户端并释放客户端
	 * */
	if (serverHandle->clientList)
	{
		OS_mutexLock(&(serverHandle->server_mutex));

		pListClient = serverHandle->clientList;
		for (pFindNode = OS_listBegin(pListClient); pFindNode != OS_listEnd(pListClient); pFindNode = OS_listNext(pListClient, pFindNode))
		{
			pNode = (OS_DataNode *)pFindNode;
			client = (network_clientInfo_t *)pNode->pData;
			/*删除链表节点*/
			OS_listEarse(serverHandle->clientList, client);
			network_unInit_client((network_Handle_t *)client);
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
	if (serverHandle)
	{
		free(serverHandle);
		serverHandle = NULL;
	}

	return 0;
}

int interior_networkSend_data(network_clientInfo_t *clientHandle, char *message, int nLen, int code)
{
	int ret = 0;
	int sendLen = 0;
	network_HeadInfo_t headinfo;

	if ((clientHandle == NULL) || (clientHandle->socket <= 0) || (message == NULL) || (nLen <= 0))
	{
		network_printf_log(clientHandle->clientInparam.logFun, "this argumeng is error socket[%d]!!", clientHandle->socket);
		return -1;
	}

	network_init_head(&headinfo);
	headinfo.load_len = nLen;
	headinfo.msg_code = code;
	headinfo.reserve = clientHandle->headinfo.reserve;

	OS_mutexLock(&(clientHandle->socket_mutex));

	/*send head*/
	sendLen = sizeof(network_HeadInfo_t);
	ret = network_tcpSocket_send(clientHandle->clientInparam.logFun, clientHandle->socket, (char *)&headinfo, &sendLen, 0);
	if (ret < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun, "send to header error!!");
		ret = -1;
		goto EXIT;
	}

	/*send bady*/
	sendLen = headinfo.load_len;
	ret = network_tcpSocket_send(clientHandle->clientInparam.logFun, clientHandle->socket, message, &sendLen, 0);
	if (ret < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun, "send to header error!!");
		ret = -1;
		goto EXIT;
	}

EXIT:
	OS_mutexUnlock(&(clientHandle->socket_mutex));

	return ret;
}

int network_send_data(network_Handle_t client_Handle, char *message, int nLen, int code)
{
	int ret = 0;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if ((clientHandle->socket <= 0) || (message == NULL) || (nLen <= 0))
	{
		network_printf_log(clientHandle->clientInparam.logFun, "this argument is error socket[%d] nLen[%d]!!", clientHandle->socket, nLen);
		return -1;
	}

	/*判断是否是异步发送*/
	if (clientHandle->clientInparam.asynchronous == 1)
	{
		int size = 0;
		network_AsynchronousInfo_t *asynchronous_messege = NULL;

		size = OS_queGetQueuedCount(&(clientHandle->asynchronousQue));
		if (OS_queIsFull(&(clientHandle->asynchronousQue)))
		{
			network_printf_log(clientHandle->clientInparam.logFun, "net_send_msg listsize is too large, pleaase check network port:%d",
							   clientHandle->clientInparam.nPort);
			return -1;
		}

		asynchronous_messege = (network_AsynchronousInfo_t *)malloc(sizeof(network_AsynchronousInfo_t));
		if (asynchronous_messege == NULL)
		{
			return -1;
		}
		asynchronous_messege->messege = malloc(nLen);
		if (asynchronous_messege->messege == NULL)
		{
			free(asynchronous_messege);
			return -1;
		}
		memcpy(asynchronous_messege->messege, message, nLen);
		asynchronous_messege->code = code;
		asynchronous_messege->nLen = nLen;

		if (OS_EFAIL == OS_quePut(&(clientHandle->asynchronousQue), (Int64)(asynchronous_messege), OS_TIMEOUT_NONE))
		{
			network_printf_log(clientHandle->clientInparam.logFun, "\033[31m channel put queue is faile!!\n \033[0m");
			network_freeAsynchronous_sendmessege(asynchronous_messege);
			return -1;
		}

		/*
		 * 判断缓冲区是否是第一次有数据，若是，则需要重新添加写事件
		 * */
		if (size == 0)
		{
			if (clientHandle->eventFlag & NETWORK_EVENT_WRITE_FLAG)
			{
				/*重新添加写事件*/
				event_add(&(clientHandle->writeEv), 0);
			}
		}
	}
	else
	{
		/*直接发送*/
		ret = interior_networkSend_data(clientHandle, message, nLen, code);
	}

	return ret;
}

int network_send_data_sync(network_Handle_t clientHandle, char *message, int nLen, int code, int timeOut, netPacket_t *outPkt)
{
	int ret = 0;
	network_clientInfo_t *client_Handle = (network_clientInfo_t *)clientHandle;
	if ((client_Handle->socket <= 0) || (message == NULL) || (nLen <= 0))
	{
		network_printf_log(client_Handle->clientInparam.logFun, "this argument is error socket[%d] nLen[%d]!!", client_Handle->socket, nLen);
		return -1;
	}

	/* 以命令code作为key，不需要修改协议头
	 * 若要实现针对每条请求都返回对应的请求，则需要修改协议头（增加字段key），跟现在的协议需要做兼容，改动大
	 * 若要在协议头增加字段，最好增加身份字段，即identity，便于接收端识别身份
	 */
	hashKey key;
	key.i_key = code;
	netRpc_t value;
	memset(&value, 0, sizeof(netRpc_t));
	OS_semCreate(&(value.sem), 1, 0);

	// TODO hashMap加锁
	ret = hashMap_insert(client_Handle->hashHandle, key, &value);
	if (ret < 0)
	{
		network_printf_log(client_Handle->clientInparam.logFun, "send data sync,insert hashmap error code[%d]!!!\n", code);
		return -1;
	}

	// 发送
	ret = network_send_data(client_Handle, message, nLen, code);
	if (ret < 0)
	{
		network_printf_log(client_Handle->clientInparam.logFun, "send data error code[%d]!!!\n", code);
		ret = -1;
		goto EXIT;
	}

	// 等待处理返回
	ret = OS_semWait(&(value.sem), timeOut, NULL);
	if (ret < 0)
	{
		network_printf_log(client_Handle->clientInparam.logFun, "wait sem timeout[%d]!!!\n", timeOut);
		ret = -1;
	}
	else
	{
		// 返回成功，返回数据
		memcpy(outPkt, &(value.pkt), sizeof(netPacket_t));
	}

EXIT:
	// 删除该key值、信号量
	hashMap_delete(client_Handle->hashHandle, key);
	OS_semDelete(&(value.sem));
	return ret;
}

int network_release_syncPkt(network_Handle_t clientHandle, netPacket_t *pkt)
{
	int ret = 0;
	network_clientInfo_t *client_Handle = (network_clientInfo_t *)clientHandle;
	if (client_Handle == NULL)
	{
		printf("this argument is null!!!\n");
		return -1;
	}
	if (pkt == NULL)
	{
		network_printf_log(client_Handle->clientInparam.logFun, "this pkt is null!!!\n");
	}

	if ((pkt->memtype == 1) && (pkt->data))
	{
		free(pkt->data);
		pkt->data = NULL;
	}
	return 0;
}

int network_serverSendData_toAllClient(network_Handle_t serverHandle, char *message, int nLen, int code)
{
	network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)serverHandle;
	OS_listHndl pListClient = NULL;
	OS_listNode_t pFindNode = NULL;
	OS_DataNode *pNode = NULL;
	int ret = 0;
	network_clientInfo_t *client = NULL;

	if (pNetServerHandle == NULL || message == NULL || nLen <= 0)
	{
		printf("this argument is NULL!!");
		return -1;
	}

	pListClient = pNetServerHandle->clientList;

	OS_mutexLock(&(pNetServerHandle->server_mutex));

	for (pFindNode = OS_listBegin(pListClient); pFindNode != OS_listEnd(pListClient); pFindNode = OS_listNext(pListClient, pFindNode))
	{
		pNode = (OS_DataNode *)pFindNode;
		client = (network_clientInfo_t *)pNode->pData;
		ret = network_send_data(client, message, nLen, code);
		if (ret < 0)
		{
			network_printf_log(pNetServerHandle->inparam.logFun, "send data to client error!!");
		}
	}

	OS_mutexUnlock(&(pNetServerHandle->server_mutex));

	return 0;
}

int network_serverGetClient_lock(network_Handle_t serverHandle)
{
	network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)serverHandle;
	if (pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return -1;
	}
	return OS_mutexLock(&(pNetServerHandle->server_mutex));
}

int network_serverGetClient_unLock(network_Handle_t serverHandle)
{
	network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)serverHandle;
	if (pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return -1;
	}
	return OS_mutexUnlock(&(pNetServerHandle->server_mutex));
}

OS_listNode_t network_server_listBegin(network_Handle_t serverHandle, network_Handle_t *client)
{
	network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)serverHandle;
	if (pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return NULL;
	}

	OS_listNode_t pFindNode = NULL;
	OS_DataNode *pNode = NULL;
	OS_listHndl pListClient = NULL;
	pListClient = pNetServerHandle->clientList;

	pFindNode = OS_listBegin(pListClient);
	if (pFindNode)
	{
		pNode = (OS_DataNode *)pFindNode;
		*client = (network_Handle_t *)pNode->pData;
	}

	return pFindNode;
}

OS_listNode_t network_server_listEnd(network_Handle_t serverHandle)
{
	network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)serverHandle;
	if (pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return NULL;
	}
	return OS_listEnd(pNetServerHandle->clientList);
}

OS_listNode_t network_server_listNext(network_Handle_t serverHandle, OS_listNode_t listNode, network_Handle_t *client)
{
	network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)serverHandle;
	if (pNetServerHandle == NULL)
	{
		printf("this argument is NULL!!");
		return NULL;
	}

	OS_listNode_t pFindNode = NULL;
	OS_DataNode *pNode = NULL;
	OS_listHndl pListClient = NULL;
	pListClient = pNetServerHandle->clientList;

	pFindNode = OS_listNext(pListClient, listNode);
	if (pFindNode)
	{
		pNode = (OS_DataNode *)pFindNode;
		*client = (network_clientInfo_t *)pNode->pData;
	}

	return pFindNode;
}

//---------------------client------------------------

int networkClient_unusual_deal(network_clientInfo_t *handle)
{
	int ret = 0;
	network_clientInfo_t *client = handle;
	if (client == NULL)
	{
		network_printf_log(client->clientInparam.logFun, "client handle is NULL!!");
		return -1;
	}

	if (client->serverHandle != NULL)
	{
		/*	客户端主动连接服务器端，
		 * 	服务端内部创建的客户端，并不是上层用户主动创建的客户端，上层用户拿不到内部创建的客户端内存地址。
		 * 	所以可以在内部释放客户端的内存资源。
		 * */
		if (client->clientInparam.nReconnect == 1)
		{
			/*重连服务器*/
			ret = network_client_reconnect((network_Handle_t *)client);
		}
		else
		{
			/*否则关闭连接*/
			network_printf_log(client->clientInparam.logFun, "server stop client!!");
			/*从客户端链表中删除该客户端*/
			if (client->serverHandle)
			{
				network_serverInfo_t *pNetServerHandle = (network_serverInfo_t *)client->serverHandle;
				OS_mutexLock(&(pNetServerHandle->server_mutex));
				OS_listEarse(pNetServerHandle->clientList, client);
				OS_mutexUnlock(&(pNetServerHandle->server_mutex));
			}
			ret = network_unInit_client((network_Handle_t *)client);
		}
	}
	else
	{
		/*
		 * 上层用户主动创建的客户端，去连接服务器
		 * 所以在该处程序不能主动释放客户端的内存资源，
		 * 必须上层用户主动调用network_unInit_client()释放内存资源，
		 * 否则若程序内部释放资源，同时上层用户正在调用发送接口发送数据，则会导致段错误！！！
		 * */
		if (client->clientInparam.nReconnect == 1)
		{
			/*重连服务器*/
			network_printf_log(client->clientInparam.logFun, "reconnect server!!");
			ret = network_client_reconnect((network_Handle_t *)client);
		}
		else
		{
			/* 关闭链接
			 * 上层用户主动创建的客户端内存资源，不可在该处程序内部主动释放内存资源！！！
			 * */
			network_printf_log(client->clientInparam.logFun, "client stop to connect server!!");
			/*删除相关事件*/
			/*上抛状态*/
			if (client->clientInparam.statusFun)
			{
				client->clientInparam.statusFun(NETWOKR_STATUS_DISCONNECT, client, client->clientInparam.param);
			}
			/*删除事件*/
			if (client->eventFlag & NETWORK_EVENT_READ_FLAG)
			{
				event_del(&(client->readEv));
			}
			if (client->eventFlag & NETWORK_EVENT_HEART_FLAG)
			{
				event_del(&(client->heartTimeoutEv));
			}
			if (client->eventFlag & NETWORK_EVENT_WRITE_FLAG)
			{
				event_del(&(client->writeEv));
			}
			if (client->eventFlag & NETWORK_EVENT_RECONNECT_FLAG)
			{
				event_del(&(client->reconnectEv));
			}
			client->eventFlag = 0; // 清空所有事件

			if (client->libeventHandle.base)
			{
				event_base_loopbreak(client->libeventHandle.base);
				client->libeventHandle.base = NULL;
			}
			// 关闭socket
			if (client->socket > 0)
			{
				close(client->socket);
				client->socket = -1;
			}
		}
	}

	return ret;
}

void network_client_callback(int sockfb, short events, void *arg)
{
	if (events & EV_READ)
	{
		/*读事件*/
		networkServer_read_cb(sockfb, events, arg);
	}
	else if (events & EV_WRITE)
	{
		/*写事件*/
		networkServer_wirte_cb(sockfb, events, arg);
	}
	else if (events & EV_TIMEOUT)
	{
		/*	连接超时事件，
		 * 	若不del事件同时没有数据到来，一直会超时*/

		/*客户端没有数据到来，判断是否需要重连*/
		network_clientInfo_t *client = (network_clientInfo_t *)arg;
		network_printf_log(client->clientInparam.logFun, "recv server time out!! ip[%s] port[%d] isReconnect[%d] overTime[%d]",
						   client->clientInparam.ip, client->clientInparam.nPort,
						   client->clientInparam.nReconnect, client->clientInparam.overtime);

		networkClient_unusual_deal(client);
	}
	else
	{
		/*socket 发生异常错误*/
		network_clientInfo_t *client = (network_clientInfo_t *)arg;
		network_printf_log(client->clientInparam.logFun, "client connect server (socket[%d])error!! ip[%s] port[%d] isReconnect[%d] overTime[%d]",
						   client->socket, client->clientInparam.ip, client->clientInparam.nPort,
						   client->clientInparam.nReconnect, client->clientInparam.overtime);

		networkClient_unusual_deal(client);
	}
}

void *network_create_client(void *argv)
{
	int ret = 0;
	int socketTimeOut = (NETWORK_TIME_OUT) * 1000;
	network_Status_t connectStatus = 0;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)argv;
	int isIpv6 = 0;
	int sockfd = 0;

	/*
	 * 只有使用libevent的线程，才能释放base循环
	 * (event_base_dispatch()才会退出)
	 * */
	evthread_use_pthreads();

	// init hashMap
	clientHandle->hashHandle = hashMap_init();

	/*连接服务器*/
	sockfd = network_tcpConnect_server(clientHandle->clientInparam.logFun,
									   clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort, 3000);
	if (sockfd == -1)
	{
		/*后面有重连逻辑*/
		network_printf_log(clientHandle->clientInparam.logFun, "tcp_connect error!!");
		connectStatus = NETWORK_STATUS_ERROR;
	}
	else
	{
		connectStatus = NETWORK_STATUS_CONNECT;
	}

	if (clientHandle->clientInparam.overtime != 0)
	{
		socketTimeOut = clientHandle->clientInparam.overtime;
	}
	else
	{
		socketTimeOut = (NETWORK_TIME_OUT) * 1000;
	}

	/*设置socket超时事件*/
	network_set_sendTimeout(clientHandle->clientInparam.logFun, sockfd, socketTimeOut / 1000, socketTimeOut % 1000);
	network_set_recvTimeout(clientHandle->clientInparam.logFun, sockfd, socketTimeOut / 1000, socketTimeOut % 1000);

	clientHandle->socket = sockfd; // 连接上就赋值到全局变量

	struct event_base *base = event_base_new();
	clientHandle->libeventHandle.base = base;

	clientHandle->eventFlag |= NETWORK_EVENT_READ_FLAG;
	event_assign(&(clientHandle->readEv), base, sockfd, EV_READ | EV_TIMEOUT | EV_PERSIST,
				 network_client_callback, (void *)clientHandle);
	// 注册事件
	struct timeval tv = {NETWORK_TIME_OUT, 0};
	event_add(&(clientHandle->readEv), &tv);

	// 是否开启异步发送
	if (clientHandle->clientInparam.asynchronous == 1)
	{
		// 创建队列
		OS_queCreate(&(clientHandle->asynchronousQue), clientHandle->clientInparam.asynchronous_listnum);

		// 注册一个写事件
		clientHandle->eventFlag |= NETWORK_EVENT_WRITE_FLAG;
		// 添加注册函数，第一次只触发一次，链表有数据再添加事件
		event_assign(&(clientHandle->writeEv), clientHandle->libeventHandle.base, sockfd, EV_WRITE,
					 network_client_callback, (void *)clientHandle);
		event_add(&(clientHandle->writeEv), NULL);
	}

	/*默认是开启心跳*/
	{
		// 创建一个定时器，定时发送发送心跳数据
		/* Initalize one event */
		clientHandle->eventFlag |= NETWORK_EVENT_HEART_FLAG;
		event_assign(&(clientHandle->heartTimeoutEv), clientHandle->libeventHandle.base, -1, 0, network_server_heart, (void *)clientHandle);

		struct timeval timetv;
		evutil_timerclear(&timetv);
		timetv.tv_sec = 0; // 第一次的心跳是马上触发的
		event_add(&(clientHandle->heartTimeoutEv), &timetv);
	}

	/*上抛状态*/
	if (clientHandle->clientInparam.statusFun)
	{
		clientHandle->clientInparam.statusFun(connectStatus, clientHandle, clientHandle->clientInparam.param);
	}

	/*事件调度*/
	event_base_dispatch(base);

	network_printf_log(clientHandle->clientInparam.logFun, "client exit loop success!!");
	return NULL;
}

network_Handle_t network_init_client(network_inparamClientNet_t inparam)
{
	if ((strcmp(inparam.ip, "") == 0) || (inparam.nPort == 0))
	{
		network_printf_log(inparam.logFun, "this argument ip or port is error!!!");
		return NULL;
	}

	int ret = 0;
	OS_ThrHndl tid;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)malloc(sizeof(network_clientInfo_t));
	if (clientHandle == NULL)
	{
		network_printf_log(inparam.logFun, "client malloc error!!");
		return NULL;
	}
	memset(clientHandle, 0, sizeof(network_clientInfo_t));
	if (inparam.asynchronous_listnum <= 0 || inparam.asynchronous_listnum > 50)
	{
		inparam.asynchronous_listnum = 15;
	}

	OS_mutexCreate(&(clientHandle->client_mutex));
	OS_mutexCreate(&(clientHandle->socket_mutex));
	memcpy(&(clientHandle->clientInparam), &inparam, sizeof(network_inparamClientNet_t));

	ret = OS_thrCreate(&tid, network_create_client, OS_DETACH, OS_THR_STACK_SIZE_DEFAULT, (void *)clientHandle);
	if (ret < 0)
	{
		network_printf_log(inparam.logFun, "create pthread error!!");
		if (clientHandle)
		{
			free(clientHandle);
			clientHandle = NULL;
		}
	}

	return clientHandle;
}

static int network_reconnect_server(network_Handle_t *client_Handle)
{
	int reconnectSocket = 0;
	int socketTimeOut = 0;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if (clientHandle == NULL)
	{
		network_printf_log(clientHandle->clientInparam.logFun, "this argument is null!!!\n");
		return -1;
	}

	/*重新连接server*/
	reconnectSocket = network_tcpConnect_server(clientHandle->clientInparam.logFun,
												clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort, 3000);
	clientHandle->socket = reconnectSocket;

	if (reconnectSocket < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun, "reconnect server ip[%s] port[%d] error!!",
						   clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort);
	}
	else
	{
		/*上抛状态*/
		if (clientHandle->clientInparam.statusFun)
		{
			network_printf_log(clientHandle->clientInparam.logFun, "reconnect server success socket[%d] ip[%s] port[%d]!!!\n",
							   reconnectSocket, clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort);
			clientHandle->clientInparam.statusFun(NETWORK_STATUS_CONNECT, clientHandle, clientHandle->clientInparam.param);
		}

		if (clientHandle->clientInparam.overtime != 0)
		{
			socketTimeOut = clientHandle->clientInparam.overtime;
		}
		else
		{
			socketTimeOut = (NETWORK_TIME_OUT) * 1000;
		}

		/*设置socket超时事件*/
		network_set_sendTimeout(clientHandle->clientInparam.logFun, reconnectSocket, socketTimeOut / 1000, socketTimeOut % 1000);
		network_set_recvTimeout(clientHandle->clientInparam.logFun, reconnectSocket, socketTimeOut / 1000, socketTimeOut % 1000);
	}

	/*重新添加事件*/
	event_assign(&(clientHandle->readEv), clientHandle->libeventHandle.base, reconnectSocket,
				 EV_READ | EV_TIMEOUT | EV_PERSIST,
				 network_client_callback, (void *)clientHandle);
	// 注册事件
	struct timeval tv = {NETWORK_TIME_OUT, 0};
	event_add(&(clientHandle->readEv), &tv);

	// 是否开启异步发送
	if (clientHandle->clientInparam.asynchronous == 1)
	{
		// 创建链表
		//		if(clientHandle->asynchronouList == NULL)
		//		{
		//			clientHandle->asynchronouList = list_lockAndCreate();
		//		}

		// 注册一个写事件
		// 添加注册函数，第一次只触发一次，链表有数据再添加事件
		event_assign(&(clientHandle->writeEv), clientHandle->libeventHandle.base, reconnectSocket, EV_WRITE,
					 network_client_callback, (void *)clientHandle);
		event_add(&(clientHandle->writeEv), NULL);
	}

	/*默认是开启心跳*/
	if (reconnectSocket > 0)
	{
		// 创建一个定时器，定时发送发送心跳数据
		/* Initalize one event */
		event_assign(&(clientHandle->heartTimeoutEv), clientHandle->libeventHandle.base, -1, 0, network_server_heart, (void *)clientHandle);

		struct timeval timetv;
		evutil_timerclear(&timetv);
		timetv.tv_sec = 0; // 第一次的心跳是马上触发的
		event_add(&(clientHandle->heartTimeoutEv), &timetv);
	}

	return 0;
}

static void network_reconnect_event(int sockfb, short events, void *argv)
{
	network_Handle_t *client = (network_Handle_t *)argv;

	// 重连
	network_reconnect_server(client);
	return;
}

/*
 * 客户端重连服务器
 * */
int network_client_reconnect(network_Handle_t *client_Handle)
{
	int reconnectSocket = 0;
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if (clientHandle == NULL)
	{
		return -1;
	}

	/*上抛状态*/
	if (clientHandle->clientInparam.statusFun)
	{
		clientHandle->clientInparam.statusFun(NETWORK_STATUS_RECONNECT, clientHandle, clientHandle->clientInparam.param);
	}

	/*删除事件*/
	if (clientHandle->eventFlag & NETWORK_EVENT_READ_FLAG)
	{
		event_del(&(clientHandle->readEv));
	}

	if (clientHandle->eventFlag & NETWORK_EVENT_HEART_FLAG)
	{
		event_del(&(clientHandle->heartTimeoutEv));
	}

	if (clientHandle->eventFlag & NETWORK_EVENT_WRITE_FLAG)
	{
		event_del(&(clientHandle->writeEv));
	}

	if (clientHandle->eventFlag & NETWORK_EVENT_RECONNECT_FLAG)
	{
		event_del(&(clientHandle->reconnectEv));
	}

	/*关闭当前的socket*/
	if (clientHandle->socket > 0)
	{
		close(clientHandle->socket);
		clientHandle->socket = -1;
	}

#if 1
	/*
	 * 创建一个定时器，定时重连，不能马上重连，若不加定时器，且链接的服务器异常，则会发生不停歇的重连
	 */
	clientHandle->eventFlag |= NETWORK_EVENT_RECONNECT_FLAG;
	event_assign(&(clientHandle->reconnectEv), clientHandle->libeventHandle.base, -1, 0, network_reconnect_event, (void *)clientHandle);
	int socketTimeOut = 0;
	struct timeval timetv;
	evutil_timerclear(&timetv);

	if (clientHandle->clientInparam.overtime != 0)
	{
		socketTimeOut = clientHandle->clientInparam.overtime;
	}
	else
	{
		socketTimeOut = (NETWORK_TIME_OUT) * 1000;
	}
	timetv.tv_sec = socketTimeOut / 1000;
	timetv.tv_usec = socketTimeOut % 1000;
	event_add(&(clientHandle->reconnectEv), &timetv);

#else

	/*重新连接server*/
	reconnectSocket = network_tcpConnect_server(clientHandle->clientInparam.logFun,
												clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort, 3000);
	clientHandle->socket = reconnectSocket;

	if (reconnectSocket < 0)
	{
		network_printf_log(clientHandle->clientInparam.logFun, "reconnect server ip[%s] port[%d] error!!",
						   clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort);
	}
	else
	{
		/*上抛状态*/
		if (clientHandle->clientInparam.statusFun)
		{
			network_printf_log(clientHandle->clientInparam.logFun, "reconnect server success socket[%d] ip[%s] port[%d]!!!\n",
							   reconnectSocket, clientHandle->clientInparam.ip, clientHandle->clientInparam.nPort);
			clientHandle->clientInparam.statusFun(NETWORK_STATUS_CONNECT, clientHandle, clientHandle->clientInparam.param);
		}
	}

	/*重新添加事件*/
	event_assign(&(clientHandle->readEv), clientHandle->libeventHandle.base, reconnectSocket,
				 EV_READ | EV_TIMEOUT | EV_PERSIST,
				 network_client_callback, (void *)clientHandle);
	// 注册事件
	struct timeval tv = {NETWORK_TIME_OUT, 0};
	event_add(&(clientHandle->readEv), &tv);

	// 是否开启异步发送
	if (clientHandle->clientInparam.asynchronous == 1)
	{
		// 创建链表
		if (clientHandle->asynchronouList == NULL)
		{
			clientHandle->asynchronouList = list_lockAndCreate();
		}

		// 注册一个写事件
		// 添加注册函数，第一次只触发一次，链表有数据再添加事件
		event_assign(&(clientHandle->writeEv), clientHandle->libeventHandle.base, reconnectSocket, EV_WRITE,
					 network_client_callback, (void *)clientHandle);
		event_add(&(clientHandle->writeEv), NULL);
	}

	/*默认是开启心跳*/
	if (reconnectSocket > 0)
	{
		// 创建一个定时器，定时发送发送心跳数据
		/* Initalize one event */
		event_assign(&(clientHandle->heartTimeoutEv), clientHandle->libeventHandle.base, -1, 0, network_server_heart, (void *)clientHandle);

		struct timeval timetv;
		evutil_timerclear(&timetv);
		timetv.tv_sec = 0; // 第一次的心跳是马上触发的
		event_add(&(clientHandle->heartTimeoutEv), &timetv);
	}
#endif

	return 0;
}

/*
 * 销毁一个客户端
 * */
int network_unInit_client(network_Handle_t *client_Handle)
{
	network_clientInfo_t *clientHandle = (network_clientInfo_t *)client_Handle;
	if (clientHandle == NULL)
	{
		return 0;
	}

	/*上抛状态*/
	if (clientHandle->clientInparam.statusFun)
	{
		clientHandle->clientInparam.statusFun(NETWOKR_STATUS_DISCONNECT, clientHandle, clientHandle->clientInparam.param);
	}

	/*删除事件*/
	if (clientHandle->eventFlag & NETWORK_EVENT_READ_FLAG)
	{
		event_del(&(clientHandle->readEv));
	}

	if (clientHandle->eventFlag & NETWORK_EVENT_HEART_FLAG)
	{
		event_del(&(clientHandle->heartTimeoutEv));
	}

	if (clientHandle->eventFlag & NETWORK_EVENT_WRITE_FLAG)
	{
		event_del(&(clientHandle->writeEv));
	}

	if (clientHandle->eventFlag & NETWORK_EVENT_RECONNECT_FLAG)
	{
		event_del(&(clientHandle->reconnectEv));
	}
	clientHandle->eventFlag = 0; // 清空所有事件

	if (clientHandle->libeventHandle.base)
	{
		event_base_loopbreak(clientHandle->libeventHandle.base);
		clientHandle->libeventHandle.base = NULL;
	}

	// 关闭socket
	if (clientHandle->socket > 0)
	{
		close(clientHandle->socket);
		clientHandle->socket = -1;
	}

	/*
	 * 释放异步链表
	 * */
	if (clientHandle->clientInparam.asynchronous == 1)
	{
		// 释放数据
		network_AsynchronousInfo_t *pMessage = NULL;
		Int64 frameAddr = 0;
		while (!OS_queIsEmpty(&(clientHandle->asynchronousQue)))
		{
			if (OS_queGet(&(clientHandle->asynchronousQue), &frameAddr, OS_TIMEOUT_NONE) >= 0)
			{
				pMessage = (network_AsynchronousInfo_t *)frameAddr;
				if (pMessage)
				{
					// 释放内存
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
	if (clientHandle->hashHandle)
	{
		hashMap_uninit(clientHandle->hashHandle);
	}

	/*
	 * 释放锁资源
	 * */
	OS_mutexDelete(&(clientHandle->socket_mutex));
	OS_mutexDelete(&(clientHandle->client_mutex));

	/*
	 * 最后释放资源
	 * */
	if (clientHandle)
	{
		free(clientHandle);
		clientHandle = NULL;
	}

	return 0;
}
