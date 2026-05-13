

#ifdef WIN32
//#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
//#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#if (_WIN32_WINNT == 0x0500)
#include <tpipv6.h>
#endif

#else
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h> // for close
#include <sys/time.h>
#include <sys/select.h>
#endif

#include <fcntl.h> // for open
#include <errno.h>
#include <string.h>
#include "network_socket.h"


int network_tcpSocket_CreateBindFd(network_Logbackmsg log,int LocalPort, char *LocalIp)
{
	int Fd = -1;
	if(LocalPort < 0 || LocalPort == 0)
	{
		network_printf_log(log,"<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>", Fd, LocalPort, LocalIp);
		return -1;
	}

    struct sockaddr_in LocalAddr = {0};
    memset(&LocalAddr,0,sizeof(struct sockaddr_in));

	LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_port = htons(LocalPort);
	if(LocalIp)
	{
        LocalAddr.sin_addr.s_addr = inet_addr((const char *)LocalIp);
	}
	else
	{
        LocalAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	}

	Fd =  socket(AF_INET, SOCK_STREAM, 0);
	if(Fd < 0)
	{
		network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>", Fd, strerror(errno), errno);
		return errno;
	}

	/*设置socket释放后可马上重用*/
	int opt = 1;
    if(setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
	{
		network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>", Fd, strerror(errno), errno);
		return errno;
	}
    if(setsockopt(Fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&opt, sizeof(opt)) < 0)
    {
        network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <SO_REUSEPORT> <FD : %d> <ERROR_S> <ERROR_D %s,%d>", Fd, strerror(errno), errno);
    }
	/*bind地址*/
	if(bind(Fd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr)) < 0)
	{
		network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_%s> <ERROR_%d> <LocalPort :%d> <LocalIp :%s>",
		      Fd, strerror(errno), errno, LocalPort, LocalIp);
		return errno;
	}

	return Fd;
}

int network_tcpSocket_CreateBindFdIPV6(network_Logbackmsg log,int LocalPort, char *LocalIp)
{
	int Fd = -1;
    if(LocalPort <= 0)
	{
		network_printf_log(log,"<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>\n", Fd, LocalPort, LocalIp);
		return -1;
	}
#ifndef WIN32
	struct sockaddr_in6 LocalAddr;
    memset(&LocalAddr,0, sizeof(struct sockaddr_in6));

	LocalAddr.sin6_family = PF_INET6;    // IPv6
	LocalAddr.sin6_port = htons(LocalPort);
	if(LocalIp)
	{
        inet_pton(AF_INET6, LocalIp, &LocalAddr.sin6_addr);  // IPv6
	}
	else
	{
        LocalAddr.sin6_addr	= in6addr_any;
	}

	Fd = socket(PF_INET6, SOCK_STREAM, 0);
	if(Fd < 0)
	{
		network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		return -1;
	}

	int opt = 1;

	if(setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		if(Fd > 2)
		{
			close(Fd);
		}
		return -1;
	}

#ifndef WIN32
	/* 设置ipv6的Socket不再接受IPv4的连接 */
	int on = 1;
	if (setsockopt(Fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
	{
		network_printf_log(log,"setsockopt");
	    return -1;
	}
#endif

	if(bind(Fd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr)) < 0)
	{
		network_printf_log(log,"<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_%s> <ERROR_%d> <LocalPort :%d> <LocalIp :%s>\n",
		      Fd, strerror(errno), errno, LocalPort, LocalIp);
		if(Fd > 2)
		{
			close(Fd);
		}
		return -1;
	}
#endif
	return Fd;
}



int network_tcpConnect_serverIPV4(network_Logbackmsg log,\
		const char* server_ip, int port, int Timeout)
{
    int sockfd = 0;

    /*
     * 创建的socket默认是阻塞的
     * */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if( sockfd == -1 )
    {
    	network_printf_log(log,"create socket error!!");
        return sockfd;
    }

    unsigned long ul = 1;
    ioctl(sockfd, FIONBIO, &ul);
	struct sockaddr_in serv_addr = { 0 };
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
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
			int ret = sockfd;
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&err), (socklen_t *)&len);
			if (err == 0){
				network_printf_log(log,"connect is successful : %u\n", errno);
			}else{
				network_printf_log(log,"connect is fail :%u ServIp:%s ServPort:%d\n", \
						errno, server_ip, port);
				ret = -1;
			}
			if(ret > 0)
			{
				ul = 0;
				ioctl(sockfd, FIONBIO, &ul); //设置为阻塞模式
				FD_CLR(sockfd, &set);
			}else
			{
				/* 关闭socket */
				close(sockfd);
				sockfd = -1;
			}
			return ret;
		}

		/* 关闭socket */
		close(sockfd);
		sockfd = -1;
		network_printf_log(log,"<RH_ConnetNonblock IS ERROR> <connect> "
				" <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <ServPort: %d> <ServIp :%s> RetSelect:%d\n",
			strerror(errno), errno, sockfd, port, server_ip, RetSelect);
		return -1;
	}
	ul = 0;
	ioctl(sockfd, FIONBIO, &ul); //设置为阻塞模式

    return sockfd;
}

int network_tcpConnect_serverIPV6(network_Logbackmsg log,const char* server_ip, int port,int Timeout)
{
#ifndef WIN32
    int sockfd = 0;
    struct sockaddr_in6 serv_addr = {0};

    memset(&serv_addr, 0, sizeof(struct sockaddr_in6));

	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_port = htons(port);
	if ( inet_pton(AF_INET6, server_ip, &serv_addr.sin6_addr) < 0 )
	{
		// IPv6
		network_printf_log(log,"inet_aton error!!");
		return -1;
	}

    /*
     * 创建的ipv6 socket默认是阻塞的
     * */
    sockfd = socket(PF_INET6, SOCK_STREAM, 0);
    if( sockfd == -1 )
    {
    	network_printf_log(log,"create ipv6 socket error!!");
        return sockfd;
    }

    /* 设置非阻塞，用于监测连接超时 */
    unsigned long ul = 1;
	ioctl(sockfd, FIONBIO, &ul);

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
			int ret = sockfd;
#ifndef WIN32
			int len = sizeof(int);
			int err = 0;
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&err), (socklen_t *)&len);
			if (err == 0)
			{
				network_printf_log(log,"connect is successful : %u\n", errno);
			}else
			{
				network_printf_log(log,\
						"connect is fail :%u ServIp:%s ServPort:%d\n",\
						errno, server_ip, port);
				ret = -1;
			}

#endif
			if(ret > 0)
			{
				ul = 0;
				ioctl(sockfd, FIONBIO, &ul); //设置为阻塞模式,可设置超时
				FD_CLR(sockfd, &set);
			}else
			{
				/* 关闭socket */
				close(sockfd);
				sockfd = -1;
			}
			return ret;
		}else
		{
			network_printf_log(log,"select error!!\n");
			/* 关闭socket */
			close(sockfd);
			sockfd = -1;
			return -1;
		}
	}

	ul = 0;
	ioctl(sockfd, FIONBIO, &ul); //设置为阻塞模式,可设置超时

    return sockfd;
#endif
    return 0;
}


int network_tcpConnect_server(network_Logbackmsg log,const char* server_ip, int port,int Timeout)
{
	int sockfd = 0;
	int isIpv6 = 0;

	//判断是否是ipv6，后期优化，通过正则表达式判断ipv4/ipv6
	if(strlen(server_ip) > 16)
	{
		isIpv6 = 1;
	}

	/*连接服务器*/
	if(isIpv6 == 1)
	{
		sockfd = network_tcpConnect_serverIPV6(log,server_ip, port,Timeout);
	}else
	{
		sockfd = network_tcpConnect_serverIPV4(log,server_ip,port,Timeout);
	}

	return sockfd;
}


int network_set_recvTimeout(network_Logbackmsg log, int Fd , int TimeoutSec, int TimeoutUsec)
{
	if(Fd < 0 || TimeoutSec < 0 || TimeoutUsec < 0)
	{
		network_printf_log(log,\
				"<network_set_recvTimeout IS ERROR>  <FD : %d>   <TimeoutSec :%d> <TimeoutUsec :%d>", \
				Fd, TimeoutSec, TimeoutUsec);
		return -1;
	}

	int ret = 0;
#ifdef WIN32
	int TimeWin = 1000 * TimeoutSec + TimeoutUsec/1000;	//ms
	ret = setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&TimeWin, sizeof(int));
#else
	struct timeval Time;
	Time.tv_sec = TimeoutSec;
	Time.tv_usec = TimeoutUsec;
	ret = setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&Time, sizeof(struct timeval));
#endif
	if(ret < 0)
	{
		network_printf_log(log,\
				"<network_set_recvTimeout IS ERROR> <SO_RCVTIMEO> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <TimeoutSec :%d> <TimeoutUsec :%d>",\
				Fd, strerror(errno), errno , TimeoutSec, TimeoutUsec);
		return errno;
	}
	return 0;
}

/*
 * 设置socket的接收超时时间，
 * 住：对于阻塞的socket有效，非阻塞的socket设置无效
 *
 * */
int network_set_sendTimeout(network_Logbackmsg log, int Fd , int TimeoutSec, int TimeoutUsec)
{
	if(Fd < 0 || TimeoutSec < 0 || TimeoutUsec < 0)
	{
		network_printf_log(log,\
				"<network_set_sendTimeout IS ERROR>  <FD : %d>   <TimeoutSec :%d> <TimeoutUsec :%d>", \
				Fd, TimeoutSec, TimeoutUsec);
		return -1;
	}

	int ret = 0;
#ifdef WIN32
	int TimeWin = 1000 * TimeoutSec + TimeoutUsec/1000;	//ms
	ret = setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&TimeWin, sizeof(int));
#else
	struct timeval Time;
	Time.tv_sec = TimeoutSec;
	Time.tv_usec = TimeoutUsec;
	ret = setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&Time, sizeof(struct timeval));
#endif
	if(ret < 0)
	{
		network_printf_log(log,\
				"<network_set_sendTimeout IS ERROR> <SO_SNDTIMEO> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <TimeoutSec :%d> <TimeoutUsec :%d>",
				Fd, strerror(errno), errno , TimeoutSec, TimeoutUsec);

		return errno;
	}
	return 0;
}

/*
 * 设置socket的接收超时时间，
 * 住：对于阻塞的socket有效，非阻塞的socket设置无效
 *
 * */
int network_tcpSocket_send(network_Logbackmsg log, int socket, char *SndBuf, int *SndLen, int Timeout)
{
	int SndTotalLen = 0;
	int SndBytes	= 0;
	int SndTempLen  = *SndLen;
	*SndLen = 0;

	while(SndTotalLen < SndTempLen)
	{
		SndBytes = send(socket , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0);
		if(SndBytes <= 0)
		{
			network_printf_log(log,\
					"<Snd > <sendlen :%d> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>",\
					SndTempLen - SndTotalLen, strerror(errno), errno, socket);
			return errno;

		}else
		{
			SndTotalLen += SndBytes;
			*SndLen = SndTotalLen;
		}
	}
	return 0;
}

int network_tcpSocket_recv(network_Logbackmsg log,int socket, char *RcvBuf, int *RcvLen, int Timeout)
{
	int RcvTotalLen = 0;
	int RcvBytes	= 0;
	int RcvTempLen  = *RcvLen;
	int recvError = 0;

	*RcvLen = 0;
	while(RcvTotalLen < RcvTempLen)
	{

		RcvBytes = recv(socket , RcvBuf + RcvTotalLen, RcvTempLen - RcvTotalLen, 0);
		if(RcvBytes < 0)
		{
			if(11 == errno)
			{
				// ! 由recv失败3次则退出，改为10次，避免频繁重连问题
				if(recvError++ < 10)
				{
					network_printf_log(log,\
							"<Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <sockfd : %d> num[%d] RcvTotalLen[%d] RcvTempLen[%d] continue",
							strerror(errno), errno, socket,recvError,RcvTotalLen,RcvTempLen);
					continue;
				}
			}

			network_printf_log(log,\
					"<Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <sockfd : %d>",
					strerror(errno), errno, socket);

			return errno;

		}else if(RcvBytes == 0)
		{
			return errno;

		} else
		{
			RcvTotalLen += RcvBytes;
			*RcvLen = RcvTotalLen;
		}

	}
	return 0;
}


int network_tcpsocket_acceptIPV4(network_Logbackmsg log,int serverFd,char* clientIP,int* clientPort)
{
	int clientFd = -1;
	struct sockaddr_in clientSocket;
    int len = sizeof(clientSocket);
	clientFd = accept(serverFd, (struct sockaddr*)&clientSocket, (socklen_t*)&len);

    /*客户端ip等信息*/
    char Ip[32] = {0};
    int port = 0;
    port = ntohs(clientSocket.sin_port);
    memcpy(Ip, inet_ntoa(clientSocket.sin_addr), 16);
    if(clientIP)
    {
    	memcpy(clientIP,Ip,16);
    }
    if(clientPort)
    {
    	*clientPort = port;
    }

	network_printf_log(log,"accept ipv4 a new client %d %d clientIp[%s] port[%d]", \
			clientFd,serverFd,Ip,port);
	return clientFd;
}

int network_tcpsocket_acceptIPV6(network_Logbackmsg log,int serverFd,char* clientIP,int* clientPort)
{
	int clientFd = -1;

#ifndef WIN32
	//接收客户端的请求
	struct sockaddr_in6 clientIPV6Addr;
	int len = sizeof(clientIPV6Addr);
	clientFd = accept(serverFd, (void *)&clientIPV6Addr, (socklen_t *)&len);

	/*客户端ip等信息*/
    char Ip[128] = {0};
    int port = 0;			//sin6_port
    struct sockaddr_in6 *addr = (struct sockaddr_in6*)&clientIPV6Addr;
    inet_ntop(AF_INET6, &(addr->sin6_addr), Ip, sizeof(Ip));
    port = ntohs(addr->sin6_port);
    if(clientIP)
    {
    	memcpy(clientIP,Ip,strlen(Ip)+1);
    }
    if(clientPort)
    {
    	*clientPort = port;
    }

	network_printf_log(log,"accept ipv6 a new client %d %d ip[%s] port[%d]", \
			clientFd,serverFd,Ip,port);
#endif
	return clientFd;
}













