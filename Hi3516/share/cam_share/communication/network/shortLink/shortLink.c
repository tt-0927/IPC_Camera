#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include "share_socket.h"
#include "shortLink.h"
#include "dlog.h"
#define WEBRETSUCCESS 0
#define WEBADDRMAXLEN			16

#define WEBRETFAIL				-1

/* 服务端最大连接数 */
#define SERVER_MAX_LISTEN_NUM (56)


static int Web_GetPrivateError()
{
	int RetError = errno;
	if(errno != 0)
	{
		RetError = (RetError ^ -1) + 1;
	}
	else
	{
		RetError = -11;
	}
	return RetError;
}
int Web_TcpSndNonblockFd(int Fd, char *SndBuf, int *SndLen, int Timeout)
{
	if(Fd < 0 || NULL == SndBuf || SndLen == NULL) {
		return WEBRETFAIL;
	}

	if(*SndLen < 0 || *SndLen == 0)
	{
	}

	int RetSelect   = -1;
	int SndTotalLen = 0;
	int SndBytes	= 0;
	int SndTempLen  = *SndLen;
	*SndLen = 0;
	fd_set SndSet;
	struct timeval Time;
	FD_ZERO(&SndSet);
	FD_SET(Fd, &SndSet);

	while(SndTotalLen < SndTempLen) {
		if(Timeout < 0) {
			RetSelect = select(Fd + 1, NULL, &SndSet, NULL, NULL);
		} else {
			Time.tv_sec = Timeout / 1000;
			Time.tv_usec = 1000 * (Timeout % 1000);

			RetSelect = select(Fd + 1, NULL, &SndSet, NULL, &Time);
		}

		if(RetSelect < 0) {
			return Web_GetPrivateError();
		} else if(RetSelect == 0) {
			return Web_GetPrivateError();
		} else {
			if(FD_ISSET(Fd, &SndSet)) {
				SndBytes = send(Fd , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0);

				if(SndBytes < 0) {

					return Web_GetPrivateError();
				} else {
					SndTotalLen += SndBytes;
					*SndLen = SndTotalLen;
				}
			} else {

				return Web_GetPrivateError();
			}
		}
	}

	return 	WEBRETSUCCESS;
}
int Web_GetConnectNonblockFd(int Fd , int Timeout, char *AcceptIp)
{
	if(Fd < 0)
	{
		return WEBRETFAIL;
	}

	int ClnFd = -1;
	int RetSelect = -1;
	int Len = sizeof(struct sockaddr_in);

	fd_set set;

	struct timeval Time;
	Time.tv_sec  = Timeout;
	Time.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(Fd, &set);

	struct sockaddr_in ClnAddr;
	RetSelect = select(Fd + 1, &set, NULL, NULL, &Time);

	if(RetSelect > 0)
	{
		ClnFd = accept(Fd, (void *)&ClnAddr, (socklen_t *)&Len);

		if(ClnFd < 0)
		{
			return Web_GetPrivateError();
		}
		else
		{
			memcpy(AcceptIp, inet_ntoa(ClnAddr.sin_addr), WEBADDRMAXLEN);
			//printf("AcceptIp=%s\n", AcceptIp);
			return ClnFd;
		}

	} else if(0 == RetSelect)
	{

		return Web_GetPrivateError();
	}
	else
	{
		return Web_GetPrivateError();
	}

}
static int Web_TcpRcvNonblockFd(int Fd, char *RcvBuf, int *RcvLen, int Timeout)
{
	if(Fd < 0 || NULL == RcvBuf || RcvLen == NULL)
	{
		return WEBRETFAIL;
	}

	if(*RcvLen <= 0) {
		return WEBRETFAIL;
	}

	int RetSelect   = -1;
	int RcvTotalLen = 0;
	int RcvBytes	= 0;
	int RcvTempLen  = *RcvLen;
	*RcvLen = 0;
	fd_set RcvSet;
	struct timeval Time;
	FD_ZERO(&RcvSet);
	FD_SET(Fd, &RcvSet);

	//    Time.tv_sec = Timeout;
	//    Time.tv_usec = 0;
	*RcvLen = 0;
	while(RcvTotalLen < RcvTempLen) {

		//如果timeout =0 ,表示什么 ？
		if(Timeout < 0) {
			RetSelect = select(Fd + 1, &RcvSet, NULL, NULL, NULL);
		} else {
			Time.tv_sec = Timeout / 1000;
			Time.tv_usec = 1000 * (Timeout % 1000);

			RetSelect = select(Fd + 1, &RcvSet, NULL, NULL, &Time);
		}
		if(RetSelect < 0) {

			return Web_GetPrivateError();
		} else if(RetSelect == 0) {

			return -1;

		} else {
			if(FD_ISSET(Fd, &RcvSet)) {
				RcvBytes = recv(Fd , RcvBuf + RcvTotalLen, RcvTempLen - RcvTotalLen, 0);

				if(RcvBytes < 0) {

					return Web_GetPrivateError();
				} else if(RcvBytes == 0) {
					return Web_GetPrivateError();
				} else {
					RcvTotalLen += RcvBytes;
					*RcvLen = RcvTotalLen;
				}
			} else {
				return Web_GetPrivateError();
			}
		}
	}

	return WEBRETSUCCESS;
}
static int short_publiccheck_head(Short_Public_Head_t *head)
{
	if((head == NULL) || (head->check_start[0] != SHORT_CHECK_END_CODE)
	   || (head->check_start[1] != SHORT_CHECK_END_CODE)
	   || (head->check_start[2] != SHORT_CHECK_END_CODE)
	   || (head->check_start[3] != SHORT_CHECK_END_CODE))
	{

		return -1;
	}

	return 0;
}

static int short_check_head(Short_Msg_Head_t *head)
{
	if((head == NULL) || (head->check_start[0] != SHORT_CHECK_START_CODE)
	   || (head->check_start[1] != SHORT_CHECK_START_CODE)
	   || (head->check_start[2] != SHORT_CHECK_START_CODE)
	   || (head->check_start[3] != SHORT_CHECK_START_CODE)
	   || (head->check_end[0] != SHORT_CHECK_END_CODE)
	   || (head->check_end[1] != SHORT_CHECK_END_CODE)
	   || (head->check_end[2] != SHORT_CHECK_END_CODE)
	   || (head->check_end[3] != SHORT_CHECK_END_CODE))
	{

		return -1;
	}

	return 0;
}
static int shortLink_publicinit_head(Short_Public_Head_t *head)
{
	if(head == NULL)
	{
		return -1;
	}
	memset(head, 0, sizeof(Short_Public_Head_t));
	head->check_start[0] = head->check_start[1] = head->check_start[2] = head->check_start[3] = SHORT_CHECK_END_CODE;
	return 0;
}
static int shortLink_init_head(Short_Msg_Head_t *head, int identifier)
{
	if(head == NULL)
	{
		return -1;
	}
	memset(head, 0, sizeof(Short_Msg_Head_t));
	head->check_start[0] = head->check_start[1] = head->check_start[2] = head->check_start[3] = SHORT_CHECK_START_CODE;
	head->check_end[0] = head->check_end[1] = head->check_end[2] = head->check_end[3] = SHORT_CHECK_END_CODE;
	head->identifier = identifier;
	return 0;
}
static int Web_CreateTcpNoBindFd(void)
{
	int Fd = -1;
	Fd = socket(AF_INET, SOCK_STREAM, 0);
	if(Fd < 0)
	{
		return Web_GetPrivateError();
	}

	return Fd;
}
int shortLinkClient_send_msg(ShortOperateHandle handle, char* message, int nLen, int code, int result)//操作句柄
{
	Short_Msg_Head_t head;
	int headLen = sizeof(Short_Msg_Head_t);
	int ret = 0;
	int socket = 0;
	if(handle == NULL || message == NULL)
	{
		//dlog(LOG_ERROR, "shortLinkServer_send_msg fail\n");
		return -1;
	}

	socket = *((int*)(handle));
	shortLink_init_head(&head, 0);
	head.cmd = code;
	head.nDataLen = nLen;
	head.return_code = result;

	ret = Web_TcpSndNonblockFd(socket, (char *)&head, &headLen, 1000);
	if(ret != WEBRETSUCCESS)
	{
		return -1;
	}

	ret = Web_TcpSndNonblockFd(socket, message, (int*)(&(head.nDataLen)), 1000);
	if(ret != WEBRETSUCCESS)
	{
		return -1;
	}
	return 0;

}
static int Web_SetBlockFd(int Fd)
{

	if(Fd < 0) {
		return WEBRETFAIL;
	}

	int32_t opts;
	opts = fcntl(Fd, F_GETFL);

	if(opts < 0) {
		return Web_GetPrivateError();
	}

	opts = opts &~ O_NONBLOCK;

	if(fcntl(Fd, F_SETFL, opts) < 0) {
		return Web_GetPrivateError();
	}

	return WEBRETSUCCESS;

}
void *ctrl_deal_ShortLinkSrvtThread(void* argv)
{
	dlog(LOG_DEBUG,"******************** Iphone start pthread ******************* what happen!?\n");
	int server_socket = -1;
	int client_socket = -1;
	unsigned short port = 3100;
	int ret  = -1;
	ShortLinkServer_Handle_t* handle = (ShortLinkServer_Handle_t*)argv;
	Short_Msg_Head_t short_head;
	Short_Public_Head_t public_head;
	ShortCallbackMsg_t pCallBackMsg;
	char  pStaitcBuf[2048] = {0};
	char * pMallocBuf = NULL;
	char * pRecvBuf = NULL;
	int nHeadLen = 0;
	char IP[16] = {0};
	if(handle == NULL)
	{
		goto Exit_pthread;
	}
	port = handle->port;

	//创建server socket bind;

	if(handle->socket != -1)
	{
		handle->socket = -1;
	}

	server_socket = -1;
	client_socket = -1;
	server_socket = RH_CreateTcpBindFd(port, NULL);
	if(server_socket < 0)
	{
	    dlog(LOG_ERROR,"RH_CreateTcpBindFd failed");
		goto Exit_pthread;
	}

	if(listen(server_socket , SERVER_MAX_LISTEN_NUM) < 0)
	{
	    dlog(LOG_ERROR,"listen failed");
		goto Exit_pthread;
	}

	//设置socket的阻塞，非阻塞模式
	if(Web_SetBlockFd(server_socket) != WEBRETSUCCESS)
	{
	    dlog(LOG_ERROR,"Web_SetBlockFd failed");
		goto Exit_pthread;
	}
	handle->socket = server_socket;
	while(1)
	{
			client_socket = Web_GetConnectNonblockFd(server_socket, 5, IP);
			if(client_socket < 0)
			{
				continue ;
			}
			handle->clientsocket = client_socket;
			if(handle->nType == PUBLIC_RESOURCE)
			{
				memset(&public_head, 0, sizeof(public_head));
				nHeadLen = sizeof(Short_Public_Head_t);
				ret =  Web_TcpRcvNonblockFd(client_socket, (char *)(&public_head), &nHeadLen, SHORTWAITIME);
				if(ret != WEBRETSUCCESS)
				{
					goto WAIT_ACCEPT;
				}
				if(short_publiccheck_head(&public_head) == -1)
				{
					goto WAIT_ACCEPT;
				}
				if(public_head.nDataLen >= 2048)
				{
					pMallocBuf = (char *)malloc(public_head.nDataLen);
					if(pMallocBuf == NULL)
					{
						goto WAIT_ACCEPT;
					}
					memset(pMallocBuf, 0, public_head.nDataLen);
					pRecvBuf = pMallocBuf;
				}
				else
				{
					memset(pStaitcBuf, 0, sizeof(pStaitcBuf));
					pRecvBuf = pStaitcBuf;
				}

				ret =  Web_TcpRcvNonblockFd(client_socket, pRecvBuf, (int*)(&(public_head.nDataLen)), SHORTWAITIME);
				if(ret != WEBRETSUCCESS)
				{
					goto WAIT_ACCEPT;
				}

				if(handle->dealCmd)
				{
					pCallBackMsg.nLen = public_head.nDataLen;
					pCallBackMsg.InParam = handle->param;
					pCallBackMsg.value = pRecvBuf;
					pCallBackMsg.sOperHandle = handle;

					sprintf(pCallBackMsg.ip, "%s", IP);
					handle->dealCmd(&pCallBackMsg);
				}

			}
			else
			{
				memset(&short_head, 0, sizeof(short_head));
				nHeadLen = sizeof(Short_Msg_Head_t);
				ret =  Web_TcpRcvNonblockFd(client_socket, (char *)(&short_head), &nHeadLen, SHORTWAITIME);
				if(ret != WEBRETSUCCESS)
				{
					goto WAIT_ACCEPT;
				}
				if(short_check_head(&short_head) == -1)
				{
					goto WAIT_ACCEPT;
				}

				if(short_head.nDataLen >= 2048)
				{
					pMallocBuf = (char *)malloc(short_head.nDataLen);
					if(pMallocBuf == NULL)
					{
						goto WAIT_ACCEPT;
					}
					memset(pMallocBuf, 0, short_head.nDataLen);
					pRecvBuf = pMallocBuf;
				}
				else
				{
					memset(pStaitcBuf, 0, sizeof(pStaitcBuf));
					pRecvBuf = pStaitcBuf;
				}

				ret =  Web_TcpRcvNonblockFd(client_socket, pRecvBuf, (int*)(&(short_head.nDataLen)), SHORTWAITIME);
				if(ret != WEBRETSUCCESS)
				{
					goto WAIT_ACCEPT;
				}
				if(handle->dealCmd)
				{
					pCallBackMsg.nLen = short_head.nDataLen;
					pCallBackMsg.Code = short_head.cmd;
					pCallBackMsg.InParam = handle->param;
					pCallBackMsg.value = pRecvBuf;
					pCallBackMsg.sOperHandle = handle;
					sprintf(pCallBackMsg.ip, "%s", IP);
					pCallBackMsg.result = short_head.return_code;
					handle->dealCmd(&pCallBackMsg);
				}
			}

		WAIT_ACCEPT:
		{
			if(client_socket > 2)
			{
				close(client_socket);
			}
			client_socket = -1;
			if(pMallocBuf)
			{
				free(pMallocBuf);
				pMallocBuf = NULL;
			}
		}

	}
Exit_pthread:
	dlog(LOG_DEBUG,"******************** Iphone exit pthread ******************* what happen!?\n");
	if(server_socket > 2)
	{
		close(server_socket);
	}
	if(client_socket > 2)
	{
		close(client_socket);
	}
	//printf_pthread_delete(__FILE__, (char *)__func__);
	pthread_detach(pthread_self());
	pthread_exit(0);
	return NULL;
}
ShortLinkServerHandle shortLink_create_netServer(int port, ShortLinkDealCmdFunc fun1, void * param, Connect_Type_t nType)
{
	dlog(LOG_DEBUG,"short link ****************** create server\n");
	if(port <= 0|| fun1 == NULL)
	{
		return NULL;
	}
	int ret = 0;
	ShortLinkServer_Handle_t* handle = NULL;
	handle = (ShortLinkServer_Handle_t*)malloc(sizeof(ShortLinkServer_Handle_t));

	if(handle == NULL)
	{
		return NULL;
	}
	//init handle
	memset(handle, 0, sizeof(ShortLinkServer_Handle_t));

	handle->dealCmd = fun1;
	handle->param = param;
	handle->port = port;
	handle->nType = nType;
	pthread_t tid;
//	printf_pthread_create(__FILE__, ("ShortLinkCreateServerThread"));
	ret = pthread_create(&tid, NULL, ctrl_deal_ShortLinkSrvtThread, (void *)(handle));

	if(ret != 0)
	{
		if( handle){
			free(handle);
			handle=NULL;
		}
		return NULL;
	}

	return handle;
}

static int isValidIP(char *ip)
{
	struct in_addr addr;
	if (inet_aton(ip, &addr) == 0)
	{
		dlog(LOG_ERROR, "Invalid address\n");
		return -1;
	}
	return 0;
}

static int Web_ConnetNoBlockFd(int Fd, int ServPort, char *ServIp , int Timeout)
{
	if(Fd < 0 || ServPort < 1 || ServIp == NULL )
	{
		return -1;
	}

	unsigned long ul = 1;
	 ioctl(Fd, FIONBIO, &ul);
	struct sockaddr_in serv_addr = { 0 };
	//bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(ServPort);
	//inet_aton((const char *)ServIp, (struct in_addr *)&serv_addr.sin_addr);
	if(isValidIP(ServIp) != 0)
	{
		return -1;
	}
	serv_addr.sin_addr.s_addr = inet_addr(ServIp);
	if (connect(Fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		struct timeval Time;
		fd_set set;
		int RetSelect = 0;
		FD_ZERO(&set);
		FD_SET(Fd, &set);
		Time.tv_sec = Timeout / 1000;
		Time.tv_usec = (Timeout % 1000) * 1000;
		RetSelect = select(Fd + 1, NULL, &set, NULL, &Time);
		if (RetSelect > 0)
		{
			 int len = sizeof(int);
			 int err = 0;
			 int ret = 0;
			 getsockopt(Fd, SOL_SOCKET, SO_ERROR, (void*)(&err), (socklen_t *)&len);
			if (err == 0)
			{
				ret = WEBRETSUCCESS;
			}
			else
			{
				ret = Web_GetPrivateError();
			}
			ul = 0;
			ioctl(Fd, FIONBIO, &ul); //设置为阻塞模式
			FD_CLR(Fd, &set);
			return ret;
		}

		return Web_GetPrivateError();
	}
	return RHRETSUCCESS;
}
 int Web_ConnetBlockFd(int Fd, int ServPort, char *ServIp) //,int Timeout)
{
	if(Fd < 0 || ServPort < 1 || ServIp == NULL) {
		return WEBRETFAIL;
	}

	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port	= htons(ServPort);
	inet_aton((const char *)ServIp, (struct in_addr *)&serv_addr.sin_addr);

	if(connect(Fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		return Web_GetPrivateError();
	}

	return WEBRETSUCCESS;
}
int shortLinkServer_send_msg(ShortOperateHandle handle, char* message, int nLen, int code, int result)//操作句柄
{
	Short_Msg_Head_t head;
	Short_Public_Head_t public_head;
	void *send_headBak = NULL;
	ShortLinkServer_Handle_t* server_handle = NULL;
	int headLen = 0;
	int ret = 0;
	if(handle == NULL || message == NULL)
	{
		//dlog(LOG_ERROR, "shortLinkServer_send_msg fail\n");
		return -1;
	}

	server_handle = (ShortLinkServer_Handle_t*)handle;

	if(server_handle->nType == PUBLIC_RESOURCE)
	{
		headLen = sizeof(Short_Public_Head_t);
		shortLink_publicinit_head(&public_head);
		public_head.nDataLen = nLen;
		send_headBak = (void *)(&public_head);
	}
	else
	{
		headLen = sizeof(Short_Msg_Head_t);
		shortLink_init_head(&head, 0);
		head.cmd = code;
		head.nDataLen = nLen;
		head.return_code = result;
		send_headBak = (void*)(&head);
	}
    
	ret = Web_TcpSndNonblockFd(server_handle->clientsocket, send_headBak, &headLen, 1000);
	if(ret != WEBRETSUCCESS)
	{
		return -1;
	}
	ret = Web_TcpSndNonblockFd(server_handle->clientsocket, message, (int*)(&nLen), 1000);
	if(ret != WEBRETSUCCESS)
	{
		return -1;
	}
	return 0;

}
int shortLink_creat_netClient(ShortLink_Send_t *pshortHandle)
{
	int client_socket = -1;
	int ret = 0;
	Short_Msg_Head_t short_head;
	ShortCallbackMsg_t pCallBackMsg;
	int waittime = SHORTWAITIME;
	char *pRecvBuf = NULL;
	int nHeadLen = 0;
	if(pshortHandle  == NULL || pshortHandle->iP == NULL || pshortHandle->message == NULL
			|| pshortHandle->iP == NULL || pshortHandle->port <= 0 || pshortHandle->nLen <= 0
			|| pshortHandle->dealcmd == NULL)
	{
		//dlog(LOG_ERROR, "shortLink_creat_netClient param is NULL\n");
		return -1;
	}
	client_socket = 	Web_CreateTcpNoBindFd();

	ret =  Web_ConnetNoBlockFd(client_socket, pshortHandle->port, pshortHandle->iP, 4000);

	if(ret != WEBRETSUCCESS) {
		goto  CLIENT_EXIT;
	}
	ret = shortLinkClient_send_msg(&client_socket,pshortHandle->message, pshortHandle->nLen, pshortHandle->code, 0);
	if(ret == -1)
	{
		ret = -1;
		goto  CLIENT_EXIT;
	}

	if(pshortHandle->waitTime > 0)
	{
		waittime = pshortHandle->waitTime;
	}

	memset(&short_head, 0, sizeof(short_head));
	nHeadLen = sizeof(Short_Msg_Head_t);
	ret =  Web_TcpRcvNonblockFd(client_socket, (char *)(&short_head), &nHeadLen, waittime);
	if(ret != WEBRETSUCCESS)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}

	if(short_check_head(&short_head) == -1)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}
	pRecvBuf = (char*)malloc(short_head.nDataLen);
	if(pRecvBuf == NULL)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}

	ret =  Web_TcpRcvNonblockFd(client_socket, (char *)pRecvBuf,(int*)(&(short_head.nDataLen)), waittime);
	if(ret != WEBRETSUCCESS)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}
	if(pshortHandle->dealcmd)
	{
		pCallBackMsg.nLen = short_head.nDataLen;
		pCallBackMsg.Code = short_head.cmd;
		pCallBackMsg.InParam = pshortHandle->parm;
		pCallBackMsg.value = pRecvBuf;
		pCallBackMsg.sOperHandle = &client_socket;
		pCallBackMsg.result = short_head.return_code;
		pshortHandle->dealcmd(&pCallBackMsg);
	}

	CLIENT_EXIT:
	if(client_socket)
	{
		close(client_socket);
	}
	if(pRecvBuf)
	{
		free(pRecvBuf);
		pRecvBuf = NULL;
	}
	return ret;
}


int shortLink_creat_netClient_sync(ShortLink_Send_t *pshortHandle,ShortCallbackMsg_t* outPkt)
{
	int client_socket = -1;
	int ret = 0;
	Short_Msg_Head_t short_head;
	ShortCallbackMsg_t pCallBackMsg;
	int waittime = SHORTWAITIME;
	char *pRecvBuf = NULL;
	int nHeadLen = 0;
	if(pshortHandle  == NULL || pshortHandle->iP == NULL \
			|| pshortHandle->message == NULL \
			|| pshortHandle->iP == NULL || pshortHandle->port <= 0 \
			|| pshortHandle->nLen <= 0)
	{
		//dlog(LOG_ERROR, "shortLink_creat_netClient param is NULL\n");
		return -1;
	}
	client_socket = Web_CreateTcpNoBindFd();

	ret = Web_ConnetNoBlockFd(client_socket, pshortHandle->port, pshortHandle->iP, 4000);
	if(ret != WEBRETSUCCESS) {
		goto  CLIENT_EXIT;
	}
	ret = shortLinkClient_send_msg(&client_socket,pshortHandle->message, pshortHandle->nLen, pshortHandle->code, 0);
	if(ret == -1)
	{
		ret = -1;
		goto  CLIENT_EXIT;
	}

	if(pshortHandle->waitTime > 0)
	{
		waittime = pshortHandle->waitTime;
	}

	memset(&short_head, 0, sizeof(short_head));
	nHeadLen = sizeof(Short_Msg_Head_t);
	ret =  Web_TcpRcvNonblockFd(client_socket, (char *)(&short_head), &nHeadLen, waittime);
	if(ret != WEBRETSUCCESS)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}

	if(short_check_head(&short_head) == -1)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}
	pRecvBuf = (char*)malloc(short_head.nDataLen);
	if(pRecvBuf == NULL)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}

	ret =  Web_TcpRcvNonblockFd(client_socket, (char *)pRecvBuf,\
			(int*)(&(short_head.nDataLen)), waittime);
	if(ret != WEBRETSUCCESS)
	{
		ret = -1;
		goto CLIENT_EXIT;
	}

	int isReleaseMem = 1;
	if(pshortHandle->dealcmd)
	{
		pCallBackMsg.nLen = short_head.nDataLen;
		pCallBackMsg.Code = short_head.cmd;
		pCallBackMsg.InParam = pshortHandle->parm;
		pCallBackMsg.value = pRecvBuf;
		pCallBackMsg.sOperHandle = &client_socket;
		pCallBackMsg.result = short_head.return_code;
		pshortHandle->dealcmd(&pCallBackMsg);
	}else
	{
		/* 同步返回 */
		isReleaseMem = 0;
		pCallBackMsg.nLen = short_head.nDataLen;
		pCallBackMsg.Code = short_head.cmd;
		pCallBackMsg.InParam = pshortHandle->parm;
		pCallBackMsg.value = pRecvBuf;
		pCallBackMsg.sOperHandle = NULL;
		pCallBackMsg.result = short_head.return_code;
		*outPkt = pCallBackMsg;
	}

CLIENT_EXIT:
	if(client_socket)
	{
		close(client_socket);
	}

	if((isReleaseMem == 1) && (pRecvBuf))
	{
		free(pRecvBuf);
		pRecvBuf = NULL;
	}

	return ret;
}

int shortLink_netClient_release_syncPacket(ShortCallbackMsg_t* outPkt)
{
	if(outPkt == NULL)
	{
		return -1;
	}
	if(outPkt->value)
	{
		free(outPkt->value);
		outPkt->value = NULL;
	}
	return 0;
}








