/**************************************************************************************************
*													注意事项
*
*					1. Nonblock 设置超时无效  Block 设置超时有效
*					2. Select  与阻塞，非阻塞没有任何关系。
*																																					write by zhengyb
****************************************************************************************************/
#include <net/if.h>
#include <sys/socket.h>
#include "share_socket.h"

#define SENDMTU 		1450
#define SOCKETISCLOSE	(-2)

int RH_GetPrivateError()
{
	int RetError = errno;

	if(errno != 0) {
		RetError = (RetError ^ -1) + 1;
	} else {
		RetError = -11;
	}
	return RetError;
}

// *****************************************************
// function	: 创建Tcp SOCKET Bind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: LocalPort > 0 /LocalIp 为NULL时服务器接受任何IP
// note		: 当作为Client时 建议创建Socket不采用bind方式 ，再重连阶段 受服务器影响.

//******************************************************
//存在一个漏洞，socket可能为0  非负数都是合法的文件描述符，但是0,1,2会被初始化为标准输入输出
int RH_CreateTcpBindFd(int LocalPort, char *LocalIp)
{
	int Fd = -1;

	if(LocalPort < 0 || LocalPort == 0) {
		printf("<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>\n", Fd, LocalPort, LocalIp);
		return RHRETFAIL;
	}

	struct sockaddr_in LocalAddr;

	bzero(&LocalAddr, sizeof(LocalAddr));

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



	Fd = RH_Socket(__FILE__, (char *)__func__, AF_INET, SOCK_STREAM, 0);

	if(Fd < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	int opt = 1;

	if(setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		if(Fd > 2)
		{
			close(Fd);
		}
		return RH_GetPrivateError();
	}
    if(setsockopt(Fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEPORT> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
    }


	if(bind(Fd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr)) < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_%s> <ERROR_%d> <LocalPort :%d> <LocalIp :%s>\n",
		      Fd, strerror(errno), errno, LocalPort, LocalIp);
		if(Fd > 2)
		{
			close(Fd);
		}
		return RH_GetPrivateError();
	}

	return Fd;
}

int RH_CreateTcpBindIPV6Fd(int LocalPort, char *LocalIp)
{
	int Fd = -1;

	if(LocalPort < 0 || LocalPort == 0) {
		printf("<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>\n", Fd, LocalPort, LocalIp);
		return RHRETFAIL;
	}

	struct sockaddr_in6 LocalAddr;

	bzero(&LocalAddr, sizeof(LocalAddr));


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



	Fd = RH_Socket(__FILE__, (char *)__func__, PF_INET6, SOCK_STREAM, 0);

	if(Fd < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	int opt = 1;

	if(setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S> <ERROR_D %s,%d>\n", Fd, strerror(errno), errno);
		if(Fd > 2)
		{
			close(Fd);
		}
		return RH_GetPrivateError();
	}
	int on = 1;
	if (setsockopt(Fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
	{
	    perror("setsockopt");
	    return -1;
	}

	if(bind(Fd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr)) < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_%s> <ERROR_%d> <LocalPort :%d> <LocalIp :%s>\n",
		      Fd, strerror(errno), errno, LocalPort, LocalIp);
		if(Fd > 2)
		{
			close(Fd);
		}
		return RH_GetPrivateError();
	}

	return Fd;
}
// *****************************************************
// function	: 创建Tcp SOCKET NoBind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: NULL
//******************************************************
int RH_CreateTcpNoBindFd(void)
{
	int Fd = -1;
	Fd = RH_Socket(__FILE__, (char *)__func__, AF_INET, SOCK_STREAM, 0);

	if(Fd < 0) {
		printf("<RH_CreateTcpNoBindFd IS ERROR> <FD : %d> <ERROR_S :%s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return Fd;
}

int RH_CreateTcpNoBindIPV6Fd(void)
{
	int Fd = -1;
	Fd = RH_Socket(__FILE__, (char *)__func__,PF_INET6, SOCK_STREAM, 0);

	if(Fd < 0) {
		printf("<RH_CreateTcpNoBindFd IS ERROR> <FD : %d> <ERROR_S :%s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return Fd;
}
// *****************************************************
// function	: 创建Tcp SOCKET Bind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: LocalPort > 0 /LocalIp 不为NULL
// note		:
// 特殊需求再应用此函数(IP限定 端口限定 等....)
//******************************************************

int RH_CreateUdpBindFd(int LocalPort, char *LocalIp)
{
	int Fd = -1;

	if(LocalPort < 0 || LocalPort == 0) {
		printf("<RH_CreateTcpFd IS ERROR>  <FD : %d> <LocalPort :%d > <LocalIp :%s>\n", Fd, LocalPort, LocalIp);
		return RHRETFAIL;
	}

	struct sockaddr_in LocalAddr;

	bzero(&LocalAddr, sizeof(LocalAddr));

	LocalAddr.sin_family = AF_INET;

	if(LocalIp != NULL)
	{
		LocalAddr.sin_addr.s_addr = inet_addr((const char *)LocalIp);
	}
	else
	{
		LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	LocalAddr.sin_port = htons(LocalPort);

	Fd = RH_Socket(__FILE__, (char *)__func__, AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(Fd < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S : %s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	int opt = 1;

	if(setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <SO_REUSEADDR> <FD : %d> <ERROR_S: %s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}


	if(bind(Fd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr)) < 0) {
		printf("<RH_CreateTcpBindFd IS ERROR> <BIND> <FD : %d> <ERROR_S :%s> <ERROR_D: %d> <LocalPort :%d> <LocalIp :%s>\n",
		      Fd, strerror(errno), errno, LocalPort, LocalIp);
		return RH_GetPrivateError();
	}

	return Fd;
}



// *****************************************************
// function	: 创建Udp SOCKET NoBind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: NULL
//******************************************************
int RH_CreateUdpNoBindFd(void)
{
	int Fd = -1;
	Fd = RH_Socket(__FILE__, (char *)__func__, AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(Fd < 0) {
		printf("<RH_CreateUdpNoBindFd IS ERROR> <FD : %d> <ERROR_S : %s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return Fd;
}


// *****************************************************
// function	: 设置socket为非阻塞模式
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_SetNonblockFd(int Fd)
{

	if(Fd < 0) {
		printf("<ZL_SETNonblockFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	int32_t opts;
	opts = fcntl(Fd, F_GETFL);

	if(opts < 0) {
		printf("<ZL_SETNonblockFd IS ERROR> <F_GETFL> <FD : %d> <ERROR_S : %s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	opts = opts | O_NONBLOCK;

	if(fcntl(Fd, F_SETFL, opts) < 0) {
		printf("<ZL_SETNonblockFd IS ERROR> <F_SETFL> <FD : %d> <ERROR_S :%s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}

// *****************************************************
// function	: 设置socket为阻塞模式
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_SetBlockFd(int Fd)
{

	if(Fd < 0) {
		printf("<ZL_SETBlockFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	int32_t opts;
	opts = fcntl(Fd, F_GETFL);

	if(opts < 0) {
		printf("<ZL_SETBockFd IS ERROR> <F_GETFL> <FD : %d> <ERROR_S: %s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	opts = opts &~ O_NONBLOCK;

	if(fcntl(Fd, F_SETFL, opts) < 0) {
		printf("<ZL_SETBockFd IS ERROR> <F_SETFL> <FD : %d> <ERROR_S :%s> <ERROR_D: %d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;

}


// *****************************************************
// function	: 获取Socket协议栈发送缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 缓冲大小 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_GetSndBufSizeFd(int Fd)
{
	if(Fd < 0) {
		printf("<RH_GetSndBufSizeFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	int SndBufSize = 0;
	int OptLen = sizeof(int);

	if(getsockopt(Fd, SOL_SOCKET, SO_SNDBUF, &SndBufSize , (unsigned int *)&OptLen) < 0) {
		printf("<RH_GetSndBufSizeFd IS ERROR> <SO_SNDBUF> <FD : %d> <ERROR_S :%s> <ERROR_D :%d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return SndBufSize;
}

// *****************************************************
// function	: 设置Socket协议栈发送缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / SndBufSize > 0
//******************************************************

int RH_SetSndBufSizeFd(int Fd , int SndBufSize)
{
	if(Fd < 0 || SndBufSize < 0 || SndBufSize == 0) {
		printf("<RH_SetSndBufSizeFd IS ERROR>  <FD : %d>  <SndBufSize :%d>\n", Fd, SndBufSize);
		return RHRETFAIL;
	}

	int OptLen = 1;

	if(setsockopt(Fd, SOL_SOCKET, SO_SNDBUF, &SndBufSize , sizeof(OptLen)) < 0) {
		printf("<RH_SetSndBufSizeFd IS ERROR> <SO_SNDBUF> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <SndBufSize :%d>\n",
		      Fd, strerror(errno), errno, SndBufSize);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}

// *****************************************************
// function	: 获取Socket协议栈接收缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 缓冲大小 / fail -1
// parameter: Fd > 0 /
//******************************************************
int RH_GetRcvBufSizeFd(int Fd)
{
	if(Fd < 0) {
		printf("<RH_GetSndBufSizeFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	int RcvBufSize = 0;
	int OptLen = sizeof(int);

	if(getsockopt(Fd, SOL_SOCKET, SO_RCVBUF, &RcvBufSize , (unsigned int *)&OptLen) < 0) {
		printf("<RH_GetRcvBufSizeFd IS ERROR> <SO_RCVBUF> <FD : %d> <ERROR_S :%s> <ERROR_D :%d>\n", Fd, strerror(errno), errno);
		return RH_GetPrivateError();
	}

	return RcvBufSize;
}

// *****************************************************
// function	: 设置Socket协议栈接收缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / RcvBufSize > 0
//******************************************************
int RH_SetRcvBufSizeFd(int Fd , int RcvBufSize)
{
	if(Fd < 0 || RcvBufSize < 0 || RcvBufSize == 0) {
		printf("<RH_SetSndBufSizeFd IS ERROR>  <FD : %d>  <SndBufSize :%d>\n", Fd, RcvBufSize);
		return RHRETFAIL;
	}

	int OptLen = 1;

	if(setsockopt(Fd, SOL_SOCKET, SO_RCVBUF, &RcvBufSize , sizeof(OptLen)) < 0) {
		printf("<RH_SetRcvBufSizeFd IS ERROR> <SO_RCVBUF> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <SndBufSize :%d>\n",
		      Fd, strerror(errno), errno, RcvBufSize);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}


// *****************************************************
// function	: 设置Socket接收超时时间,非阻塞socket设置无效
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / TimeoutSec 大于等于 0 / TimeoutUsec 大于等于 0
// note		: 1.tv_sec 、tv_usec源接口  皆为long ；此函数不支持long型 精度
//			  2.此函数针对阻塞Socket
//******************************************************

int RH_SetRcvTimeoutFd(int Fd , int TimeoutSec, int TimeoutUsec)
{
	if(Fd < 0 || TimeoutSec < 0 || TimeoutUsec < 0) {
		printf("<RH_SetRcvTimeoutFd IS ERROR>  <FD : %d>   <TimeoutSec :%d> <TimeoutUsec :%d>\n", Fd, TimeoutSec, TimeoutUsec);
		return RHRETFAIL;
	}

	struct timeval Time;

	Time.tv_sec = TimeoutSec;

	Time.tv_usec = TimeoutUsec;

	if(setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&Time, sizeof(struct timeval)) < 0) {
		printf("<RH_SetRcvTimeoutFd IS ERROR> <SO_RCVTIMEO> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <TimeoutSec :%d> <TimeoutUsec :%d>\n",
		      Fd, strerror(errno), errno , TimeoutSec, TimeoutUsec);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}


// *****************************************************
// function	: 设置Socket发送超时时间
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / TimeoutSec 大于等于 0 / TimeoutUsec 大于等于 0
// note		: 1.tv_sec 、tv_usec源接口  皆为long ；此函数不支持long型 精度
//			  2.此函数针对阻塞Socket
//******************************************************

int RH_SetSndTimeoutFd(int Fd , int TimeoutSec, int TimeoutUsec)
{
	if(Fd < 0 || TimeoutSec < 0 || TimeoutUsec < 0) {
		printf("<RH_SetSndBufSizeFd IS ERROR>  <FD : %d>   <TimeoutSec :%d> <TimeoutUsec :%d>\n", Fd, TimeoutSec, TimeoutUsec);
		return RHRETFAIL;
	}

	struct timeval Time;

	Time.tv_sec = TimeoutSec;

	Time.tv_usec = TimeoutUsec;

	if(setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&Time, sizeof(struct timeval)) < 0) {
		printf("<RH_SetSndTimeoutFd IS ERROR> <SO_SNDTIMEO> <FD : %d> <ERROR_S :%s> <ERROR_D :%d> <TimeoutSec :%d> <TimeoutUsec :%d>\n",
		      Fd, strerror(errno), errno , TimeoutSec, TimeoutUsec);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}





// *****************************************************
// function	: 以阻塞模式SOCKET 连接服务器
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / ServPort 大于等于 0 /ServIp 不为空 /Timeout 大于等于 0
// note		: 阻塞SOCKET
//******************************************************
int RH_ConnetBlockFd(int Fd, int ServPort, char *ServIp) //,int Timeout)
{
	if(Fd < 0 || ServPort < 1 || ServIp == NULL) {
		printf("<RH_ConnetNonblock IS ERROR>  <FD : %d> <ServPort: %d> <ServIp :%s>\n", Fd, ServPort, ServIp);
		return RHRETFAIL;
	}

#if 0

	if(RH_SetSndTimeoutFd(Fd, Timeout, 0) < 0) {
		printf("<RH_ConnetNonblock IS ERROR> <RH_SetSndTimeoutFd> <FD : %d> <ServPort: %d> <ServIp :%s> <Timeout :%d>\n",
		       Fd, ServPort, ServIp, Timeout);
		return RHRETFAIL;
	}

#endif
	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port	= htons(ServPort);
	inet_aton((const char *)ServIp, (struct in_addr *)&serv_addr.sin_addr);

	if(connect(Fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		printf("<RH_ConnetNonblock IS ERROR> <connect>  <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <ServPort: %d> <ServIp :%s>\n",
		      strerror(errno), errno, Fd, ServPort, ServIp);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}

// *****************************************************
// function	: 以非阻塞模式SOCKET 连接服务器
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / ServPort 大于等于 0 /ServIp 不为空 /Timeout 大于等于 0
// note		: 1.非阻塞SOCKET
// 			  2.Timeout 为0 为阻塞模式 当SOCKET异常返回
//            3.Timeout 大于0 为超时非阻塞
//******************************************************

int RH_ConnetNonblockFd(int Fd, int ServPort, char *ServIp , int Timeout)
{
	if(Fd < 0 || ServPort < 1 || ServIp == NULL )
	{
		printf("<RH_ConnetNonblock IS ERROR>  <FD : %d> <ServPort: %d> <ServIp :%s>\n", Fd, ServPort, ServIp);
		return RHRETFAIL;
	}

	unsigned long ul = 1;
	 ioctl(Fd, FIONBIO, &ul);
	struct sockaddr_in serv_addr = { 0 };
	//bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(ServPort);
	//inet_aton((const char *)ServIp, (struct in_addr *)&serv_addr.sin_addr);
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
				printf("RH_ConnetBlockFd is successful** : %u\n", errno);
				ret = RHRETSUCCESS;

			}
			else
			{
				// printf("RH_ConnetBlockFd is fail :%u ServIp:%s ServPort:%d\n", errno, ServIp, ServPort);
				ret = RH_GetPrivateError();
			}
			ul = 0;
			ioctl(Fd, FIONBIO, &ul); //设置为阻塞模式
			FD_CLR(Fd, &set);
			return ret;
		}
		printf("<RH_ConnetNonblock IS ERROR> <connect> "
				" <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <ServPort: %d> <ServIp :%s> RetSelect:%d\n",
			strerror(errno), errno, Fd, ServPort, ServIp, RetSelect);
		return RH_GetPrivateError();
	}
	ul = 0;
	ioctl(Fd, FIONBIO, &ul); //设置为阻塞模式
	return RHRETSUCCESS;
}


int RH_ConnetNonblockIPV6Fd(int Fd, int ServPort, char *ServIp , int Timeout)
{
	if(Fd < 0 || ServPort < 1 || ServIp == NULL )
	{
		printf("<RH_ConnetNonblock IS ERROR>  <FD : %d> <ServPort: %d> <ServIp :%s>\n", Fd, ServPort, ServIp);
		return RHRETFAIL;
	}

	unsigned long ul = 1;
	 ioctl(Fd, FIONBIO, &ul);
	 struct sockaddr_in6 serv_addr = { 0 };
	//bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin6_family  = AF_INET6;
	serv_addr.sin6_port = htons(ServPort);
    if ( inet_pton(AF_INET6, ServIp, &serv_addr.sin6_addr) < 0 ) {                 // IPv6
    	return RHRETFAIL;
    }
	//inet_aton((const char *)ServIp, (struct in_addr *)&serv_addr.sin_addr);
	//serv_addr.sin_addr.s_addr = inet_addr(ServIp);
	if (connect(Fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
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
				printf("RH_ConnetBlockFd is successful** : %u\n", errno);
				ret = RHRETSUCCESS;

			}
			else
			{
				printf("RH_ConnetBlockFd is fail :%u ServIp:%s ServPort:%d\n", errno, ServIp, ServPort);
				ret = RH_GetPrivateError();
			}
			ul = 0;
			ioctl(Fd, FIONBIO, &ul); //设置为阻塞模式
			FD_CLR(Fd, &set);
			return ret;
		}
		printf("<RH_ConnetNonblock IS ERROR> <connect> "
				" <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <ServPort: %d> <ServIp :%s> RetSelect:%d\n",
			strerror(errno), errno, Fd, ServPort, ServIp, RetSelect);
		return RH_GetPrivateError();
	}
	ul = 0;
	ioctl(Fd, FIONBIO, &ul); //设置为阻塞模式
	return RHRETSUCCESS;
}

// *****************************************************
// function	: 以未绑定socket 做监听操作
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 /  LocalPort >0 /LocalIp为NULL 或 不为NULL
// note		: 1.SOCKET未做绑定操作  -----  RH_CreateTcpNoBindFd(void)
// 			  2.LocalIp 为NULL时绑定 INADDR_ANY
//            3.LocalIp 不为NULL时绑定 特定地址LocalIp
//******************************************************
int RH_ListenTcpNoBindFd(int Fd, int LocalPort, char *LocalIp)
{
	char TempUrl[RHADDRMAXLEN] = "LocalIp NULL!";
	int opt = 1;

	if(Fd < 0 || LocalPort < 1) {
		printf("<RH_ListenTcpNoBindFd IS ERROR>  <FD : %d> <LocalPort: %d>\n", Fd, LocalPort);
		return RHRETFAIL;
	}

	struct sockaddr_in				LocalAddr;

	bzero(&LocalAddr, sizeof(struct sockaddr_in));

	LocalAddr.sin_family 		= AF_INET;

	LocalAddr.sin_port 			= htons(LocalPort);

	struct linger so_linger;

	so_linger.l_onoff = 1;

	so_linger.l_linger = 0;

	if(NULL != LocalIp) {
		LocalAddr.sin_addr.s_addr = inet_addr((const char *)LocalIp);
		memset(TempUrl , 0 , RHADDRMAXLEN);
		memcpy(TempUrl, LocalIp, RHADDRMAXLEN);
	} else {
		LocalAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	}

	if(setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		printf("<RH_ListenTcpNoBindFd IS ERROR> <SO_REUSEADDR>  <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <LocalPort: %d> <LocalIp :%s>\n",
		      strerror(errno), errno, Fd, LocalPort, TempUrl);
		return RH_GetPrivateError();
	}

	if(setsockopt(Fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0) {
		printf("<RH_ListenTcpNoBindFd IS ERROR> <SO_DONTLINGER>  <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <LocalPort: %d> <LocalIp :%s>\n",
		      strerror(errno), errno, Fd, LocalPort, TempUrl);
		//	return RH_GetPrivateError();
	}


	if(bind(Fd, (struct sockaddr *)&LocalAddr, (socklen_t)sizeof(LocalAddr)) < 0) {
		printf("<RH_ListenTcpNoBindFd IS ERROR> <bind>  <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <LocalPort: %d> <LocalIp :%s>\n",
		      strerror(errno), errno, Fd, LocalPort, TempUrl);
		return RH_GetPrivateError();

	}

	if(listen(Fd, RHLISTENQUENUM) < 0) {
		printf("<RH_ListenTcpNoBindFd IS ERROR> <listen> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <LocalPort: %d> <LocalIp :%s>\n",
		      strerror(errno), errno, Fd, LocalPort, TempUrl);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}


// *****************************************************
// function	: 以已绑定socket 做监听操作
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
// note		: 1.SOCKET已做绑定操作  -----  RH_CreateTcpBindFd(int LocalPort,char *LocalIp)
//******************************************************
int RH_ListenTcpBindFd(int Fd)
{
	if(Fd < 0) {
		printf("<RH_ListenTcpBindFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	if(listen(Fd, RHLISTENQUENUM) < 0) {
		printf("<RH_ListenTcpBindFd IS ERROR> <listen> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		      strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}


// *****************************************************
// function	: 以阻塞模式SOCKET 获取客户端连接
// author 	: zhengyb		2014.9.1
// return   : Succes Fd / fail -1
// parameter: Fd > 0
// note		: 1.阻塞SOCKET
//			  2.此函数为阻塞 <慎用!!!>
//******************************************************
int RH_GetConnectBlockFd(int Fd)
{

	if(Fd < 0) {
		printf( "<RH_GetConnectBlockFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	int ClnFd = -1;
	int Len = sizeof(struct sockaddr_in);
	struct sockaddr_in ClnAddr;
	ClnFd = accept(Fd, (void *)&ClnAddr, (socklen_t *)&Len);

	if(ClnFd < 0) {
		printf( "<RH_GetConnectBlockFd IS ERROR> <accept> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		      strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	} else {
		printf( "<RH_GetConnectBlockFd IS OK> <accept> <FD : %d> <ClientAddr :%s>\n", Fd, inet_ntoa(ClnAddr.sin_addr));
		return ClnFd;
	}
}



// *****************************************************
// function	: 以非阻塞模式SOCKET 获取客户端连接
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1 /  超时 0
// parameter: Fd > 0 / Timeout 大于等于 0
// note		: 1.非阻塞SOCKET
//			  2.Timeout 为0时则此函数为阻塞，直到socket异常
//			  3.Timeout 为大于0时则等待时间未Timeout 秒
//******************************************************
int RH_GetConnectNonblockFd(int Fd , int Timeout, char *AcceptIp)
{
	if(Fd < 0) {
		printf( "<RH_GetConnectNonblockFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
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

	if(RetSelect > 0) {
		ClnFd = accept(Fd, (void *)&ClnAddr, (socklen_t *)&Len);

		if(ClnFd < 0) {
			printf( "<RH_GetConnectNonblockFd IS ERROR> <listen> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else {
			printf( "<RH_GetConnectNonblockFd IS OK> <accept> <FD : %d> <ClientAddr :%s>\n", Fd, inet_ntoa(ClnAddr.sin_addr));
			memcpy(AcceptIp, inet_ntoa(ClnAddr.sin_addr), RHADDRMAXLEN);
			return ClnFd;
		}

	} else if(0 == RetSelect) {
//		nslog(NS_ERROR, "<RH_GetConnectNonblockFd IS ERROR> <Connect timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
//		      strerror(errno), errno, Fd, Timeout);
		return RH_GetPrivateError();
	} else {
		printf( "<RH_GetConnectNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",  strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	}

}
int RH_GetConnectNonblockIPV6Fd(int Fd , int Timeout, char *AcceptIp, int ipLen)
{
	if(Fd < 0) {
		printf( "<RH_GetConnectNonblockFd IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
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

	struct sockaddr_in6 ClnAddr;
	Len = sizeof(struct sockaddr_in6);
	RetSelect = select(Fd + 1, &set, NULL, NULL, &Time);

	if(RetSelect > 0) {
		ClnFd = accept(Fd, (void *)&ClnAddr, (socklen_t *)&Len);

		if(ClnFd < 0) {
			printf( "<RH_GetConnectNonblockFd IS ERROR> <listen> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else {
			//nslog(NS_INFO, "<RH_GetConnectNonblockFd IS OK> <accept> <FD : %d> <ClientAddr :%s>\n", Fd, inet_ntoa(ClnAddr.sin_addr));
			struct sockaddr_in6 *addrbak = (struct sockaddr_in6*)&ClnAddr;
				inet_ntop(AF_INET6, &(addrbak->sin6_addr), AcceptIp, ipLen);
			return ClnFd;
		}

	} else if(0 == RetSelect) {
//		nslog(NS_ERROR, "<RH_GetConnectNonblockFd IS ERROR> <Connect timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
//		      strerror(errno), errno, Fd, Timeout);
		return RH_GetPrivateError();
	} else {
		printf( "<RH_GetConnectNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",  strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	}

}
// *****************************************************
// function	: 简单粗暴关闭socket
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_RoughClose(int Fd)
{
	if(Fd < 0) {
		printf( "<RH_RoughClose IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	if(shutdown(Fd, SHUT_RDWR) < 0) {

		printf( "<RH_RoughClose IS ERROR> <shutdown> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		      strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}


// *****************************************************
// function	: 非简单粗暴关闭socket
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_NonroughClose(int Fd)
{
	if(Fd < 0) {
		printf( "<RH_RoughClose IS ERROR>  <FD : %d>\n", Fd);
		return RHRETFAIL;
	}

	if(RH_Close(__FILE__, (char *)__func__, Fd) < 0) {

		printf( "<RH_NonroughClose IS ERROR> <close> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		      strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	}

	return RHRETSUCCESS;
}



// *****************************************************
// function	: 以非阻塞模式TCP SOCKET 发送数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            SndBuf 不为NULL
//			  SndLen 不为NULL ；*SndLen等于要发送数据长度 ；函数返回后*SndLen为已发送长度
//  		  Timeout 小于0 此函数为无超时阻塞模式 ；
//			  Timeout 等于0 此函数为即时返回非阻塞模式；
//			  Timeout 大于0 此函数为有超时非阻塞模式；
//			  Timeout 单位毫秒  !!!!!!!
// note		: 1.非阻塞SOCKET
//			  2.发送大数据 KB单位 例如整帧数据
//			  3.Timeout 作为Select 设定超时  并不是此函数调用超时
//******************************************************
int RH_TcpSndNonblockFd(int Fd, char *SndBuf, int *SndLen, int Timeout)
{

	if(Fd < 0 || NULL == SndBuf || SndLen == NULL) {
		printf( "<RH_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%p> <Timeout :%d>\n",
		      Fd, SndBuf, SndLen, Timeout);
		return RHRETFAIL;
	}

	if(*SndLen < 0 || *SndLen == 0) {
		printf( "<RH_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%d> <Timeout :%d>\n",
		      Fd, SndBuf, *SndLen, Timeout);
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

	//    Time.tv_sec = Timeout;
	//    Time.tv_usec = 0;

	//int sendlenNum = 0;
	while(SndTotalLen < SndTempLen) {
		if(Timeout < 0) {
			RetSelect = select(Fd + 1, NULL, &SndSet, NULL, NULL);
		} else {
			Time.tv_sec = Timeout / 1000;
			Time.tv_usec = 1000 * (Timeout % 1000);

			RetSelect = select(Fd + 1, NULL, &SndSet, NULL, &Time);
		}


		if(RetSelect < 0) {
			printf( "<RH_TcpSndNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else if(RetSelect == 0) {
			//nslog(NS_ERROR, "<RH_TcpSndNonblockFd IS ERROR> <Snd timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
			      ///strerror(errno), errno, Fd, Timeout);
			//return RH_GetPrivateError();
				return RH_GetPrivateError();
		} else {
			if(FD_ISSET(Fd, &SndSet)) {
		/*		if(SndTempLen - *SndLen < SENDMTU)
				{
					sendlenNum = SndTempLen - *SndLen;
				}
				else
				{
					sendlenNum = SENDMTU;
				}
				SndBytes = send(Fd , SndBuf + SndTotalLen, sendlenNum, 0);
		*/
				SndBytes = send(Fd , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0);
				if(SndBytes <= 0) {
					printf( "<RH_TcpSndNonblockFd IS ERROR> <Snd > <sendlen :%d> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
					      SndTempLen - SndTotalLen, strerror(errno), errno, Fd);
					return RH_GetPrivateError();
				} else {
					SndTotalLen += SndBytes;
					*SndLen = SndTotalLen;
				}
			} else {
				printf( "<RH_TcpSndNonblockFd IS ERROR> <FD_ISSET> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
				      strerror(errno), errno, Fd);
				return RH_GetPrivateError();
			}
		}
	}

	return 	RHRETSUCCESS;
}


// *****************************************************
// function	: 以阻塞模式TCP SOCKET 发送数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            SndBuf 不为NULL
//			  SndLen 不为NULL ；*SndLen等于要发送数据长度 ；函数返回后*SndLen为已发送长度
// note		: 1.非阻塞SOCKET
//			  2.此函数配合 RH_SetSndTimeoutFd 函数设定发送超时 更佳
//******************************************************

int RH_TcpSndBlockFd(int Fd, char *SndBuf, int *SndLen)
{
	if(Fd < 0 || NULL == SndBuf || SndLen == NULL) {
		printf( "<RH_TcpSndBlockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%p>\n",
		      Fd, SndBuf, SndLen);
		return RHRETFAIL;
	}

	if(*SndLen < 0 || *SndLen == 0) {
		printf( "<RH_TcpSndBlockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%d>\n",
		      Fd, SndBuf, *SndLen);
	}

	int SndTotalLen = 0;
	int SndBytes	= 0;
	int SndTempLen  = *SndLen;
	*SndLen = 0;

	while(SndTotalLen < SndTempLen) {

		SndBytes = send(Fd , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0);

		if(SndBytes <= 0) {
			printf( "<RH_TcpSndBlockFd IS ERROR> <Snd> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else {
			SndTotalLen += SndBytes;
			*SndLen = SndTotalLen;
		}
	}

	return RHRETSUCCESS;

}

// *****************************************************
// function	: 以select方式TCP SOCKET 接收数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1  clientClose -2
// parameter: Fd > 0
//            RcvBuf 不为NULL
//			  RcvLen 不为NULL ；*RcvLen 等于要发送数据长度 ；函数返回后*RcvLen 为已发送长度
//  		  Timeout 小于0 此函数为无超时阻塞模式 ；
//			  Timeout 等于0 此函数为即时返回非阻塞模式；
//			  Timeout 大于0 此函数为有超时非阻塞模式；
//			  Timeout 单位毫秒  !!!!!!!
// note		: 1.阻塞或者非阻塞socket都可以使用
//			  2.接收大数据 KB单位 例如整帧数据
//			  3.Timeout 作为Select 设定超时  并不是此函数调用超时
//******************************************************
int RH_TcpRcvNonblockFd(int Fd, char *RcvBuf, int *RcvLen, int Timeout)
{

	if(Fd < 0 || NULL == RcvBuf || RcvLen == NULL) {
		printf( "\n\n%s,%s,%d,<RH_TcpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%p> <Timeout :%d>\n",__FILE__,__func__,__LINE__,
		      Fd, RcvBuf, RcvLen, Timeout);
		return RHRETFAIL;
	}

	if(*RcvLen <= 0) {
		printf( "<RH_TcpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d> <Timeout :%d>\n",
		      Fd, RcvBuf, *RcvLen, Timeout);
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
			printf( "<RH_TcpRcvNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);

			return RH_GetPrivateError();
		} else if(RetSelect == 0) {
			//	nslog(NS_ERROR, "<RH_TcpRcvNonblockFd IS ERROR> <Rcv timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
			//      strerror(errno), errno, Fd, Timeout);
			return -1;
			//return RH_GetPrivateError();
		} else {
			if(FD_ISSET(Fd, &RcvSet)) {
				RcvBytes = recv(Fd , RcvBuf + RcvTotalLen, RcvTempLen - RcvTotalLen, 0);

				//断网无法检测到,会返回-1
				if(RcvBytes < 0) {
					printf( "<RH_TcpRcvNonblockFd IS ERROR> <Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
					      strerror(errno), errno, Fd);
					return RH_GetPrivateError();
				} else if(RcvBytes == 0) {
		//			nslog(NS_ERROR, "<RH_TcpRcvNonblockFd IS ERROR> <Rcv Close> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		//			      strerror(errno), errno, Fd);
				    //return RH_GetPrivateError();
					return SOCKETISCLOSE;
				} else {
					RcvTotalLen += RcvBytes;
					*RcvLen = RcvTotalLen;
				}
			} else {
				printf( "<RH_TcpRcvNonblockFd IS ERROR> <FD_ISSET> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
				      strerror(errno), errno, Fd);
				return RH_GetPrivateError();
			}
		}
	}

	return RHRETSUCCESS;
}

// *****************************************************
// function	: 以阻塞模式TCP SOCKET 接收数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            RcvBuf 不为NULL
//			  RcvLen 不为NULL ；*RcvLen等于要接收数据长度 ；函数返回后*RcvLen为已接收长度
// note		: 1.阻塞SOCKET
//			  2.此函数配合 RH_SetRcvTimeoutFd 函数设定发送超时 更佳
//******************************************************
int RH_TcpRcvBlockFd(int Fd, char *RcvBuf, int RcvLen , int *readlen)
{
	if(Fd < 0 || NULL == RcvBuf || readlen == NULL) {
		printf( "<RH_TcpRcvBlockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%p>\n",    Fd, RcvBuf, readlen);
		return RHRETFAIL;
	}

	if(RcvLen < 0 || RcvLen == 0) {
		printf( "<RH_TcpRcvBlockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d>\n",	      Fd, RcvBuf, RcvLen);
		return RHRETFAIL;
	}

	int RcvTotalLen = 0;
	int RcvBytes	= 0;
	int timeout_cnt = 0;
	char *TempBuf = NULL;
	int RcvTempLen  = 0;
	*readlen = 0;

	while(RcvTotalLen < RcvLen) {
//		nslog(NS_INFO, "--RH_TcpRcvBlockFd");
		TempBuf = RcvBuf + RcvTotalLen;
		RcvTempLen = RcvLen - RcvTotalLen;
		RcvBytes = recv(Fd , TempBuf, RcvTempLen, 0);

		//printf( "<RH_TcpSndBlockFd IS Begin> <RcvAdress : %p> <RcvTempLen :%d><RcvLen :%d><readlen :%d><Rcv> <FD : %d>\n",
		//	 TempBuf,RcvTempLen, RcvLen, *readlen, Fd);

		if(RcvBytes < 0) {
			if(11 == errno) {
				timeout_cnt++;

				if(timeout_cnt < 2) {
					  struct sockaddr_in flocal;
					  int faddr_len = sizeof( struct sockaddr_in);

					  memset(&flocal , 0 ,sizeof(struct sockaddr_in));
					  getpeername(Fd,(struct sockaddr*)&flocal, (socklen_t *)&faddr_len);
					  printf( "recv failed timeout_cnt=%d errno=%d-<%s> port:%d\n",
							timeout_cnt, errno, strerror(errno), ntohs(flocal.sin_port));
					continue;
				}
			}
			printf( "<RH_TcpRcvBlockFd IS ERROR> <RcvAdress : %p> <RcvTempLen :%d> <RcvLen :%d><readlen :%d><Rcv> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			     TempBuf,RcvTempLen, RcvLen, *readlen, strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else if(RcvBytes == 0) {
			printf( "ret =%d ---\n", RcvBytes);
			return -1;
		} else {
			RcvTotalLen += RcvBytes;
			*readlen = RcvTotalLen;
		}
	}

	return RHRETSUCCESS;
}

// *****************************************************
// function	: 以非阻塞模式UDP SOCKET 发送数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            SndBuf 不为NULL
//			  SndLen 不为NULL ；*SndLen等于要发送数据长度 ；函数返回后*SndLen为已发送长度
//  		  Timeout 小于0 此函数为无超时阻塞模式 ；
//			  Timeout 等于0 此函数为即时返回非阻塞模式；
//			  Timeout 大于0 此函数为有超时非阻塞模式；
//			  Timeout 单位毫秒  !!!!!!!
//			  SndIp   不为NULL 发送目的地IP
//			  SndPort 不为NULL 发送目的地端口
// note		: 1.非阻塞SOCKET
//			  2.发送大数据 KB单位 例如整帧数据
//			  3.Timeout 作为Select 设定超时  并不是此函数调用超时
//******************************************************
int RH_UdpSndNonblockFd(int Fd, char *SndIp, int SndPort, char *SndBuf, int *SndLen, int Timeout)
{

	if(Fd < 0 || NULL == SndBuf || NULL == SndLen || NULL == SndIp || SndPort == 0 || SndPort < 0) {
		printf( "<RH_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%p> <Timeout :%d> <SndIp :%s> <SndPort :%d>\n",
		      Fd, SndBuf, SndLen, Timeout, SndIp, SndPort);
		return RHRETFAIL;
	}

	if(*SndLen < 0 || *SndLen == 0) {
		printf( "<RH_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%d> <Timeout :%d>\n",
		      Fd, SndBuf, *SndLen, Timeout);
	}

	struct sockaddr_in SndAddr ;

	bzero(&SndAddr, sizeof(struct sockaddr_in));

	SndAddr.sin_family = AF_INET;

	SndAddr.sin_port = htons(SndPort);

	// !!!!??? zhengyb
	//	SrvAddr.sin_addr.s_addr=htonl(ip);
	SndAddr.sin_addr.s_addr = inet_addr((const char *)SndIp);

	int RetSelect	= -1;

	int SndTotalLen = 0;

	int SndBytes	= 0;

	int SndTempLen	= *SndLen;

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
			printf( "<RH_TcpSndNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else if(RetSelect == 0) {
			printf( "<RH_TcpSndNonblockFd IS ERROR> <Snd timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
			      strerror(errno), errno, Fd, Timeout);
			return RH_GetPrivateError();
		} else {
			if(FD_ISSET(Fd, &SndSet)) {
				SndBytes = sendto(Fd , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0, (struct sockaddr *)&SndAddr, sizeof(SndAddr));

				if(SndBytes < 0) {
					printf( "<RH_TcpSndNonblockFd IS ERROR> <Snd> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
					      strerror(errno), errno, Fd);
					return RH_GetPrivateError();
				} else {
					SndTotalLen += SndBytes;
					*SndLen = SndTotalLen;
				}
			} else {
				printf( "<RH_TcpSndNonblockFd IS ERROR> <FD_ISSET> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
				      strerror(errno), errno, Fd);
				return RH_GetPrivateError();
			}
		}
	}

	return	RHRETSUCCESS;

}


// *****************************************************
// function	: 以阻塞模式UDP SOCKET 发送数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            SndBuf 不为NULL
//			  SndLen 不为NULL ；*SndLen等于要发送数据长度 ；函数返回后*SndLen为已发送长度
//			  SndIp   不为NULL 发送目的地IP
//			  SndPort 不为NULL 发送目的地端口
// note		: 1.非阻塞SOCKET
//			  2.此函数配合 RH_SetSndTimeoutFd 函数设定发送超时 更佳
//******************************************************

int RH_UdpSndBlockFd(int Fd, char *SndIp, int SndPort, char *SndBuf, int *SndLen)
{

	if(Fd < 0 || NULL == SndBuf || NULL == SndLen || NULL == SndIp || SndPort == 0 || SndPort < 0) {
		printf( "<RH_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%p> <SndIp :%s> <SndPort :%d>\n",
		      Fd, SndBuf, SndLen, SndIp, SndPort);
		return RHRETFAIL;
	}

	if(*SndLen < 0 || *SndLen == 0) {
		printf( "<RH_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%d> \n",
		      Fd, SndBuf, *SndLen);
	}

	struct sockaddr_in SndAddr ;

	bzero(&SndAddr, sizeof(struct sockaddr_in));

	SndAddr.sin_family = AF_INET;

	SndAddr.sin_port = htons(SndPort);

	// !!!!??? zhengyb
	//	SrvAddr.sin_addr.s_addr=htonl(ip);
	SndAddr.sin_addr.s_addr = inet_addr((const char *)SndIp);

	int SndTotalLen = 0;

	int SndBytes	= 0;

	int SndTempLen  = *SndLen;

	*SndLen = 0;

	while(SndTotalLen < SndTempLen) {
		SndBytes = sendto(Fd , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, 0, (struct sockaddr *)&SndAddr, sizeof(SndAddr));

		if(SndBytes < 0) {
			printf( "<RH_TcpSndBlockFd IS ERROR> <Snd> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else {
			SndTotalLen += SndBytes;
			*SndLen = SndTotalLen;
		}
	}

	return RHRETSUCCESS;


}

// *****************************************************
// function	: 以非阻塞模式UDP SOCKET 接收数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            RcvBuf 不为NULL
//			  RcvLen 不为NULL ；*RcvLen等于要接收数据长度 ；函数返回后*RcvLen为已接收长度
//  		  Timeout 小于0 此函数为无超时阻塞模式 ；
//			  Timeout 等于0 此函数为即时返回非阻塞模式；
//			  Timeout 大于0 此函数为有超时非阻塞模式；
//			  Timeout 单位毫秒  !!!!!!!
//			  RcvIp   不为NULL 则获取接收数据包的缘地址 ；否则依然
//			  RcvPort  不为NULL 则获取接收数据包的缘端口 ；否则依然
// note		: 1.非阻塞SOCKET
//			  2.接收大数据 KB单位 例如整帧数据
//			  3.Timeout 作为Select 设定超时  并不是此函数调用超时
//******************************************************
int RH_UdpRcvNonblockFd(int Fd, char *RcvIp, int *RcvPort, char *RcvBuf, int *RcvLen, int Timeout)
{

	if(Fd < 0 || NULL == RcvBuf || RcvLen == NULL) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%p> <Timeout :%d>\n",
		      Fd, RcvBuf, RcvLen, Timeout);
		return RHRETFAIL;
	}

	if(*RcvLen < 0 || *RcvLen == 0) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d> <Timeout :%d>\n",
		      Fd, RcvBuf, *RcvLen, Timeout);
	}


	int RetSelect	= -1;
	int RcvTotalLen = 0;
	int RcvBytes	= 0;
	int RcvTempLen	= *RcvLen;
	*RcvLen = 0;
	fd_set RcvSet;
	struct timeval Time;
	FD_ZERO(&RcvSet);
	FD_SET(Fd, &RcvSet);

	struct sockaddr_in ClnAddr ;
	int Size = sizeof(ClnAddr);
	int SetClnAddr = 0;
	bzero(&ClnAddr, sizeof(struct sockaddr_in));

	char TempSndIp[RHADDRMAXLEN] = {0};
	//	int  TempSndPort = 0;


	while(RcvTotalLen < RcvTempLen) {


		if(Timeout < 0) {
			RetSelect = select(Fd + 1, NULL, &RcvSet, NULL, NULL);
		} else {
			Time.tv_sec = Timeout / 1000;
			Time.tv_usec = 1000 * (Timeout % 1000);

			RetSelect = select(Fd + 1, NULL, &RcvSet, NULL, &Time);
		}


		if(RetSelect < 0) {
			printf( "<RH_UdpRcvNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else if(RetSelect == 0) {
			printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
			      strerror(errno), errno, Fd, Timeout);
			return RH_GetPrivateError();
		} else {
			if(FD_ISSET(Fd, &RcvSet)) {
				RcvBytes = recvfrom(Fd , RcvBuf + RcvTotalLen, RcvTempLen - RcvTotalLen, 0, (struct sockaddr *)&ClnAddr, (socklen_t *)&Size);

				if(RcvBytes < 0) {
					printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
					      strerror(errno), errno, Fd);
					return RH_GetPrivateError();
				} else if(RcvBytes == 0) {
					printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv Close> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
					      strerror(errno), errno, Fd);
					return RH_GetPrivateError();
				} else {
					if(0 == SetClnAddr) {
						if(RcvIp != NULL && RcvPort != NULL) {
							if(ClnAddr.sin_family == AF_INET) {
								if(inet_ntop(AF_INET, &ClnAddr.sin_addr, TempSndIp, sizeof(TempSndIp)) == NULL) {
									printf( "<RH_UdpRcvNonblockFd IS ERROR> <inet_ntop SndIp> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
									      strerror(errno), errno, Fd);
								} else {
									memcpy(RcvIp, TempSndIp, RHADDRMAXLEN);
								}

								if(ntohs(ClnAddr.sin_port) != 0) {
									*RcvPort = ntohs(ClnAddr.sin_port);
								}
							} else {
								printf( "<RH_UdpRcvNonblockFd IS ERROR> <sa_family > <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
								      strerror(errno), errno, Fd);
							}

							SetClnAddr = 1;
						}
					}

					RcvTotalLen += RcvBytes;
					*RcvLen = RcvTotalLen;
				}
			} else {
				printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD_ISSET> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
				      strerror(errno), errno, Fd);
				return RH_GetPrivateError();
			}
		}
	}

	return RHRETSUCCESS;

}


// *****************************************************
// function	: 以阻塞模式UDP SOCKET 接收数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            RcvBuf 不为NULL
//			  RcvLen 不为NULL ；*RcvLen等于要接收数据长度 ；函数返回后*RcvLen为已接收长度
//			  RcvIp   不为NULL 则获取接收数据包的缘地址 ；否则依然
//			  RcvPort  不为NULL 则获取接收数据包的缘端口 ；否则依然
// note		: 1.阻塞SOCKET
//			  2.此函数配合 RH_SetRcvTimeoutFd 函数设定发送超时 更佳
//******************************************************
int RH_UdpRcvBlockFd(int Fd, char *RcvIp, int RcvPort, char *RcvBuf, int *RcvLen)
{

	if(Fd < 0 || NULL == RcvBuf || RcvLen == NULL) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d>\n",
		      Fd, RcvBuf, *RcvLen);
		return RHRETFAIL;
	}

	if(*RcvLen < 0 || *RcvLen == 0) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d>\n",
		      Fd, RcvBuf, *RcvLen);
	}


	//	int RetSelect	= -1;
	int RcvTotalLen = 0;
	int RcvBytes	= 0;
	int RcvTempLen	= *RcvLen;
	*RcvLen = 0;

	struct sockaddr_in ClnAddr ;
	int Size = sizeof(ClnAddr);
	int SetClnAddr = 0;
	bzero(&ClnAddr, sizeof(struct sockaddr_in));

	char TempSndIp[RHADDRMAXLEN] = {0};
	//	int  TempSndPort = 0;

	while(RcvTotalLen < RcvTempLen) {


		RcvBytes = recvfrom(Fd , RcvBuf + RcvTotalLen, RcvTempLen - RcvTotalLen, 0, (struct sockaddr *)&ClnAddr, (socklen_t *)&Size);

		if(RcvBytes < 0) {
			printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else if(RcvBytes == 0) {
			printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv Close> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
			      strerror(errno), errno, Fd);
			return RH_GetPrivateError();
		} else {
			if(0 == SetClnAddr) {
				if(RcvIp != NULL && RcvPort > 0) {
					if(ClnAddr.sin_family == AF_INET) {
						if(inet_ntop(AF_INET, &ClnAddr.sin_addr, TempSndIp, sizeof(TempSndIp)) == NULL) {
							printf( "<RH_UdpRcvNonblockFd IS ERROR> <inet_ntop SndIp> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
							      strerror(errno), errno, Fd);
						} else {
							memcpy(RcvIp, TempSndIp, RHADDRMAXLEN);
						}

						if(ntohs(ClnAddr.sin_port) != 0) {
							RcvPort = ntohs(ClnAddr.sin_port);
						}
					} else {
						printf("<RH_UdpRcvNonblockFd IS ERROR> <sa_family > <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
						      strerror(errno), errno, Fd);
					}

					SetClnAddr = 1;
				}
			}

			RcvTotalLen += RcvBytes;
			*RcvLen = RcvTotalLen;
		}
	}

	return RHRETSUCCESS;
}

int RH_UdpRcvBlockFd2(int Fd, char *RcvIp, int RcvPort, char *RcvBuf, int *RcvLen)
{

	if(Fd < 0 || NULL == RcvBuf || RcvLen == NULL) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d>\n",
		      Fd, RcvBuf, *RcvLen);
		return RHRETFAIL;
	}

	if(*RcvLen < 0 || *RcvLen == 0) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <FD : %d> <RcvBuf :%p> <RcvLen :%d>\n",
		      Fd, RcvBuf, *RcvLen);
	}


	//	int RetSelect	= -1;
	int RcvBytes	= 0;
	int RcvTempLen	= *RcvLen;
	*RcvLen = 0;

	struct sockaddr_in ClnAddr ;
	int Size = sizeof(ClnAddr);
	bzero(&ClnAddr, sizeof(struct sockaddr_in));

	char TempSndIp[RHADDRMAXLEN] = {0};
	//	int  TempSndPort = 0;

	RcvBytes = recvfrom(Fd , RcvBuf, RcvTempLen, 0, (struct sockaddr *)&ClnAddr, (socklen_t *)&Size);
	if(RcvBytes < 0) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv Error> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		      strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	} else if(RcvBytes == 0) {
		printf( "<RH_UdpRcvNonblockFd IS ERROR> <Rcv Close> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
		      strerror(errno), errno, Fd);
		return RH_GetPrivateError();
	} else {
		if(RcvIp != NULL && RcvPort > 0) {
			if(ClnAddr.sin_family == AF_INET) {
				if(inet_ntop(AF_INET, &ClnAddr.sin_addr, TempSndIp, sizeof(TempSndIp)) == NULL) {
					printf( "<RH_UdpRcvNonblockFd IS ERROR> <inet_ntop SndIp> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
					      strerror(errno), errno, Fd);
				} else {
					memcpy(RcvIp, TempSndIp, RHADDRMAXLEN);
				}

				if(ntohs(ClnAddr.sin_port) != 0) {
					RcvPort = ntohs(ClnAddr.sin_port);
				}
			} else {
				printf( "<RH_UdpRcvNonblockFd IS ERROR> <sa_family > <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
				      strerror(errno), errno, Fd);
			}
		}
		*RcvLen = RcvBytes;
	}

	return RHRETSUCCESS;
}



static unsigned int  i = 0;
static unsigned int  m = 0;
int32_t RH_Socket(char *file, char *func, int32_t domain, int32_t type, int32_t protocol)
{
	int fd = -1;
	fd =  socket(domain, type, protocol);

	if(1)
	{
		if(i == 0xffffffff)
		{
			i = 0;
		}
		i++;
	}

	return fd;
}

int32_t RH_Close(char *file, char *func, int32_t fd)
{
	if(fd < 0) {
		//nslog(NS_ERROR, "[%s:%s] ,fd [%d] is invalid\n", file, func, fd);
		return 0;
	}

	if(1) {
		if(m == 0xffffffff) {
			m = 0;
		}

		m++;
		//nslog(NS_DEBUG, "[%s:%s] close socke[%d],close num=[%d]open num=[%d]\n", file, func, fd, m, i);
	}

	close(fd);
	fd = -1;
	return errno;
}

int Getlocalip(char* outip)
{
	int i=0;
	int sockfd;
	struct ifconf ifconf;
	char buf[512] = {0};
	struct ifreq *ifreq;
	char* ip = NULL;
	//初始化ifconf
	ifconf.ifc_len = 512;
	ifconf.ifc_buf = buf;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
			return -1;
	}
	ioctl(sockfd, SIOCGIFCONF, &ifconf);    //获取所有接口信息
	close(sockfd);
	//接下来一个一个的获取IP地址
	ifreq = (struct ifreq*)buf;
	for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i>0;i--)
	{
		ip = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
		if(strcmp(ip,"127.0.0.1")==0)  //排除127.0.0.1，继续下一个
		{
				ifreq++;
				continue;
		}
		strcpy(outip,ip);
		return 0;
	}
	return -1;
}






