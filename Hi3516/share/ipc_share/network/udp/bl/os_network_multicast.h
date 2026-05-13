/*************************************************************************
UDP接口文件
  @File Name: os_network_multicast.h
@Author: luoyongkang
@Created Time: 2021年03月29日 星期一 
************************************************************************/
#ifndef _OS_NETWORK_MULTICAST_H_
#define _OS_NETWORK_MULTICAST_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "os_thr.h"
#include "os_ringBuf.h"
#include "os_sem.h"
#include <fcntl.h>
#include <errno.h>
#include "os_que.h"
#define DATASIZE (1024*10)

typedef struct os_netwoek{
	char ip[16];
	int port;
}Network_S ;
typedef struct user_recv_{
	int     code;
	char    data[DATASIZE];							/*接收到的数据*/
	int     lenght;							/*长度*/
	void*   user;							/*用户传入的参数*/
	Network_S stNetwork; /*源地址*/
    Network_S stDstNet;  /*目的地址*/
}UserRecv_S;

typedef enum networktype_{
	UNICAST,	//单播
	MULTICAST,	//组播
	SPECAST,	//特定网卡接受组播
	BROADCAST, //广播
}NetworkType_EN;



//typedef void* (*HandleData)(UserRecv_S*  recv_info);
typedef void* (*HandleData)(void*  recv_info);

/*网络初始化结构体*/
typedef struct os_networmulticast{
	NetworkType_EN			entype;					/*网络类型*/
	Network_S				stsrc_ip_info;			/*源地址*/
	Network_S				stdst_ip_info;			/*目的地址*/
	char					amcast_ip[16];			/*组播地址*/
	int                 	nmcast_port;			/*组播端口*/
	HandleData				fnhandledata;			/*数据处理函数*/
	int						nfd;					/*socke描述符*/
	struct sockaddr_in		stmucast_addr;			/*组播发送sockaddr_in*/
	struct sockaddr_in		stunque_addr;			/*单播发送sockaddr_in*/
	struct sockaddr_in		stbroadcast_addr;		/*广播发送sockaddr_in*/
	int						nflags;					/*阻塞方式*/
	struct timeval			sttimeout;				/*超时时间*/
	int						nrecv_opt;				/*接受缓冲区*/
	int						nsend_opt;				/*发送缓冲区*/
	int						nread_size;             /*要接受的字节长度，不要超过DATASIZEt */
	UserRecv_S				stuser_recv_handle;
	int                 	loopBack;               /* 是否开启本地回环 */
	
	OS_QueHndl 				sthndl;   				/*组播发送的缓冲区*/
	OS_streamRingBufHndl 	astRingBufHndl[4];		/*单播发送的缓存区*/

    int 					nasync_deal;	 		/*是否异步处理- 0:同步  非0:异步*/
	int 					nasynchronous;	 		/*是否异步发送*/
	int 					nasynchronous_size; 	/*异步存储数据的个数synchronous_size*/
	int 					nasynchronous_sendsize; /*发送字节的大小*/
	int 					nExit;           		/*异步发送线程退出*/
	OS_ThrHndl 				handle_thr;				/*异步发送线程*/
	OS_ThrHndl 				recv_thr;				/*接送线程*/
}NetworkMulticast_S;



/*组播初始化,初始化后会自动接受数据并处理
 *@network_info  传入传出参数，用于保存初始化网络数据
 *返回值： 成功：0    失败：-1
 * */
int os_networkmulticast_init( NetworkMulticast_S* stpnetwork_info);

/*
* @description: 单播初始化,初始化后会自动接受数据并处理
* @param[in]: network_info: 传入传出参数，用于保存初始化网络数据
* @return: 成功：0    失败：-1
* @others：其他说明
*/
int os_networkunque_init( NetworkMulticast_S* stpnetwork_info);

/*组播发送数据
 *@data				发送的数据
 *@lenght			发送的长度
 *@network_info		初始化后的os_networkmulticast参数
 *返回值：			成功：发送的字节   失败：-1
 * */
int os_networkmulticast_send(const char *pdata, int nlenght, NetworkMulticast_S* stpnetwork_info);

/*单播发送数据
 *@data				发送的数据
 *@lenght			发送的长度
 *@ip				接受方的ip
 *@port				接受方的端口
 *返回值：			成功：发送的字节   失败：-1
 * */
int os_networkunque_ip_send(const char *pdata, int nlenght, NetworkMulticast_S* stpnetwork_info, char* pip, int nport);

/*单播发送数据
 *@data				发送的数据
 *@lenght			发送的长度
 *@network_info		初始化后的os_networkmulticast参数
 *返回值：			成功：发送的字节   失败：-1
 * */
int os_networkunque_send(const char *pdata, int nlenght, NetworkMulticast_S* stpnetwork_info);

/*组播退出
 *@network_info  初始化后的os_networkmulticast参数
 *返回值： 成功：0    失败：-1
 * */
int os_networkmulticast_exit( NetworkMulticast_S* stpnetwork_info);

/*
* @description: 单播退出
* @param[in]: network_info: 初始化后的os_networkunque参数
* @return: 成功：0    失败：-1
* @others：其他说明
*/
int os_networkunque_exit( NetworkMulticast_S* stpnetwork_info);


/*广播发送*/
int os_networkbroadcast_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info);
/**
 * @description: 广播发送
 * @param {char} *pdata
 * @param {int} nlenght
 * @param {NetworkMulticast_S*} pNetwork_info
 * @param {int} nPort
 * @return {*}
 * @author: fhs
 */
int os_networkbroadcast_port_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info,int nPort);
#ifdef __cplusplus
}
#endif
#endif
