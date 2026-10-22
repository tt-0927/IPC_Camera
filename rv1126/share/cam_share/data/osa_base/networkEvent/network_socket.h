
#ifndef _CORE_SOURCE_NETWORK_SOCKET_INCLUDE_
#define _CORE_SOURCE_NETWORK_SOCKET_INCLUDE_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*network_Logbackmsg)(const char *format, ...);//用于输出调试信息的函数指针

/*打印函数*/
#ifdef WIN32
#define network_printf_log(logFun,format,...) \
    if(logFun)\
    {\
        logFun("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,__VA_ARGS__);\
    }else{\
        printf("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,__VA_ARGS__);\
    }
#else
#define network_printf_log(logFun,format,args...)\
	if(logFun)\
	{\
		logFun("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,##args);\
	}else{\
		printf("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,##args);\
	}
#endif

/*
 * 创建ipv4 TCP服务，并绑定本机
 * */
int network_tcpSocket_CreateBindFd(network_Logbackmsg log,int LocalPort, char *LocalIp);

/*
 * 创建ipv4 TCP客户端端口并连接服务器
 * */
int network_tcpConnect_serverIPV4(network_Logbackmsg log,const char* server_ip, int port, int Timeout);

/*
 * IPV4服务器accept接受客户端连接
 * */
int network_tcpsocket_acceptIPV4(network_Logbackmsg log,int serverFd,char* clientIP,int* clientPort);

/*
 * 创建ipv6 TCP服务，并绑定本机
 * */
int network_tcpSocket_CreateBindFdIPV6(network_Logbackmsg log,int LocalPort, char *LocalIp);

/*
 * IPV6服务器accept接受客户端连接
 * */
int network_tcpsocket_acceptIPV6(network_Logbackmsg log,int serverFd,char* clientIP,int* clientPort);


/*
 * 创建ipv6 TCP客户端端口并连接服务器
 * */
int network_tcpConnect_serverIPV6(network_Logbackmsg log,const char* server_ip, int port,int Timeout);

/*
 * 客户端连接服务器，内部自动识别ipv4/ipv6
 * */
int network_tcpConnect_server(network_Logbackmsg log,const char* server_ip, int port,int Timeout);

/*
 * 设置发送/接受超时
 * */
int network_set_sendTimeout(network_Logbackmsg log, int Fd , int TimeoutSec, int TimeoutUsec);
int network_set_recvTimeout(network_Logbackmsg log, int Fd , int TimeoutSec, int TimeoutUsec);

/*
 * 接受tcp数据
 * */
int network_tcpSocket_recv(network_Logbackmsg log,int socket, char *RcvBuf, int *RcvLen, int Timeout);

/*
 * 发送tcp数据
 * */
int network_tcpSocket_send(network_Logbackmsg log, int socket, char *SndBuf, int *SndLen, int Timeout);


#ifdef __cplusplus
}
#endif
#endif //_CORE_SOURCE_NETWORK_SOCKET_INCLUDE_

