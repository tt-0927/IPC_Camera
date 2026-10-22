/*
 * share_udp.h
 *
 *  Created on: 2018年8月2日
 *      Author: lixiao
 */
#ifndef __SHARE_UDP_H__
#define __SHARE_UDP_H__


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "share_socket.h"
#include "pthread.h"
#include "list_use_lock.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#define UDP_LOCAL_IP "LOCAL_ANY"
#define MULTICASE_IP "225.123.123.123"

typedef void *Sdk_udp_Handle_t;




typedef struct __UDP_CLIENT_ASYNCHRONOUS__
{
	char ip[16];
	unsigned int port;
	char *dataBuf; 
	int nLen;
}UdpAsynchronous_Info_t,*UdpAsynchronous_Info_ptr;




typedef struct __UDP_CALLBACK_MESG__
{
	 void* InParam;//用来回调传出去的参数
	 int Code;//哪种命令
	 int nLen;
	 UdpAsynchronous_Info_ptr recvvalue;//接收的内容
	 Sdk_udp_Handle_t sOperHandle;
 
}UdpCallbackMsg_t,*UdpCallbackMsg_ptr;



typedef int(*UdpDealFunc)(UdpCallbackMsg_ptr);
typedef int(*UdpGetHeartMsg)(char *messege, int nLen,  Sdk_udp_Handle_t handle, void *inparam);//用于输出调试信息的函数指针
typedef int(*UdpLogbackmsg)(const char *format, ...);//用于输出调试信息的函数指针
typedef void *(*pthread_fun_udp) (void *);
typedef int(*UdpCheckHeadFunc)(char *messege);

typedef struct __UDP_MUTLCAST_INFO__
{
	int enable;
	int loopBack;
}UdpMuticast_t,*UdpMuticast_ptr;


typedef struct __UDP_BOARDCAST_INFO__
{
	int enable;
}UdpBroadcast_t,*UdpBroadcast_ptr;

typedef struct __UDP_CONNECT_INFO__
{
	int port;  //服务器为真,  客户端为 FALSE(不绑定端口,只发送)
	char* ip;
	char* multicastIp;
	int recvvalueLen;
	UdpBroadcast_t boardcastInfo;
	UdpMuticast_t muticastInfo;
}Udp_info_t,*Udp_info_ptr;


typedef struct __UDP_CILENT_NET__
{
	Udp_info_ptr udpConnectInfo;
	void *param;//创建传进去的参数，回调带上来
	UdpDealFunc cmdfun;
	UdpLogbackmsg logFun;
	UdpGetHeartMsg heartmsg;//服务器发送给客户端的信息
	UdpCheckHeadFunc checkHeadFunc;
}InparamClientNetUdp_t,*InparamClientNetUdp_ptr;

typedef struct __UDP_OPREATE_HANDLE__
{
	int socket;
	InparamClientNetUdp_t inparam;
	pthread_t send_tid;
	pthread_t rec_tid;//发送时用
	pthread_t client_tid;
	pthread_mutex_t lock;//互斥锁
	pthread_mutex_t netlock;//网络互斥锁
	List_LockHandle_t* asynchronous_reclist;//异步链表接收链表
	List_LockHandle_t* asynchronous_sendclist;//异步发送链表

}UdpOpreateHandle_t,*UdpOpreateHandle_ptr;





int udp_creatUdpSocket();
void udp_setSocketDst(Udp_info_t udpInfo,struct sockaddr_in *skt_addr);
int udp_bindPort_init(struct sockaddr_in skt_addr,int udpSocket);
int udp_joinMulticastGroup(Udp_info_t udpInfo,int sktFd);
int init_udp_client(InparamClientNetUdp_t netparm);

Sdk_udp_Handle_t udpServer_init_net(InparamClientNetUdp_t netparm);
int udp_clientSendToServer_func(UdpAsynchronous_Info_ptr asynchronous_sendmessege,Sdk_udp_Handle_t udp_handle);
int parse_mseg(const char *buf,char *field,int *actionCode,char *parseOut);
int parse_msegInt(const char *buf,char *field,int *actionCode,int *parseInt);

#endif
