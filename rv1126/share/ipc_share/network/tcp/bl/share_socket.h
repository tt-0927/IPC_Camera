#ifndef	__RH_SOCKET_H__
#define	__RH_SOCKET_H__

#include <stdio.h>
#include <stdlib.h>
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
//#include <linux/if.h>
#include <sys/time.h>
#include <time.h>

#define RHLISTENQUENUM			100
#define RHADDRMAXLEN			16

#define RHRETSUCCESS			0
#define RHRETFAIL				-1
#ifdef __cplusplus
extern "C" {
#endif
// *****************************************************
// function	: 设置socket为阻塞模式
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_SetBlockFd(int Fd);


// *****************************************************
// function	: 设置socket为非阻塞模式
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_SetNonblockFd(int Fd);


// *****************************************************
// function	: 获取Socket协议栈发送缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 缓冲大小 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_GetSndBufSizeFd(int Fd);


// *****************************************************
// function	: 设置Socket协议栈发送缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / SndBufSize > 0
//******************************************************
int RH_SetSndBufSizeFd(int Fd , int SndBufSize);


// *****************************************************
// function	: 获取Socket协议栈接收缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 缓冲大小 / fail -1
// parameter: Fd > 0 /
//******************************************************
int RH_GetRcvBufSizeFd(int Fd);


// *****************************************************
// function	: 设置Socket协议栈接收缓冲大小
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / RcvBufSize > 0
//******************************************************
int RH_SetRcvBufSizeFd(int Fd , int RcvBufSize);


// *****************************************************
// function	: 设置Socket接收超时时间
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / TimeoutSec 大于等于 0 / TimeoutUsec 大于等于 0
// note		: 1.tv_sec 、tv_usec源接口  皆为long ；此函数不支持long型 精度
//			  2.此函数针对阻塞Socket
//******************************************************
int RH_SetRcvTimeoutFd(int Fd , int TimeoutSec,int TimeoutUsec);

// *****************************************************
// function	: 设置Socket发送超时时间
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / TimeoutSec 大于等于 0 / TimeoutUsec 大于等于 0
// note		: 1.tv_sec 、tv_usec源接口  皆为long ；此函数不支持long型 精度
//			  2.此函数针对阻塞Socket
//******************************************************
int RH_SetSndTimeoutFd(int Fd , int TimeoutSec,int TimeoutUsec);


// *****************************************************
// function	: 创建Tcp SOCKET Bind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: LocalPort > 0 /LocalIp 不为NULL
// note		: 当作为Client时 建议创建Socket不采用bind方式 ，再重连阶段 受服务器影响.

//            特殊需求再应用此函数(IP限定 端口限定 等....)  慎用!!!!!!!
//******************************************************
int RH_CreateTcpBindFd(int LocalPort,char *LocalIp); //特定需求 慎用!!!



int RH_CreateTcpBindIPV6Fd(int LocalPort, char *LocalIp);
// *****************************************************
// function	: 创建Tcp SOCKET NoBind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: NULL
//******************************************************
int RH_CreateTcpNoBindFd(void); //推荐


int RH_CreateTcpNoBindIPV6Fd(void);
// *****************************************************
// function	: 创建Tcp SOCKET Bind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: LocalPort > 0 /LocalIp 不为NULL
// note		:
// 特殊需求再应用此函数(IP限定 端口限定 等....)
//******************************************************
int RH_CreateUdpBindFd(int LocalPort,char *LocalIp); //特定需求 还好!!!


// *****************************************************
// function	: 创建Udp SOCKET NoBind 模式
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1
// parameter: NULL
//******************************************************
int RH_CreateUdpNoBindFd(void); //推荐


// *****************************************************
// function	: 以阻塞模式SOCKET 连接服务器
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / ServPort 大于等于 0 /ServIp 不为空 /Timeout 大于等于 0
// note		: 阻塞SOCKET
//******************************************************
int RH_ConnetBlockFd(int Fd,int ServPort,char *ServIp);


// *****************************************************
// function	: 以非阻塞模式SOCKET 连接服务器
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 / ServPort 大于等于 0 /ServIp 不为空 /Timeout 大于等于 0
// note		: 1.非阻塞SOCKET
// 			  2.Timeout 为0 为阻塞模式 当SOCKET异常返回
//            3.Timeout 大于0 为超时非阻塞
//******************************************************
int RH_ConnetNonblockFd(int Fd,int ServPort,char *ServIp ,int Timeout);


int RH_ConnetNonblockIPV6Fd(int Fd, int ServPort, char *ServIp , int Timeout);
// *****************************************************
// function	: 以已绑定socket 做监听操作
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
// note		: 1.SOCKET已做绑定操作  -----  RH_CreateTcpBindFd(int LocalPort,char *LocalIp)
//******************************************************
int RH_ListenTcpBindFd(int Fd);



// *****************************************************
// function	: 以未绑定socket 做监听操作
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0 /  LocalPort >0 /LocalIp为NULL 或 不为NULL
// note		: 1.SOCKET未做绑定操作  -----  RH_CreateTcpNoBindFd(void)
// 			  2.LocalIp 为NULL时绑定 INADDR_ANY
//            3.LocalIp 不为NULL时绑定 特定地址LocalIp
//******************************************************
int RH_ListenTcpNoBindFd(int Fd,int LocalPort,char *LocalIp);


// *****************************************************
// function	: 以阻塞模式SOCKET 获取客户端连接
// author 	: zhengyb		2014.9.1
// return   : Succes Fd / fail -1
// parameter: Fd > 0
// note		: 1.阻塞SOCKET
//			  2.此函数为阻塞 <慎用!!!>
//******************************************************
int RH_GetConnectBlockFd(int Fd);



// *****************************************************
// function	: 以非阻塞模式SOCKET 获取客户端连接
// author 	: zhengyb		2014.9.1
// return   : Succes fd / fail -1 /  超时 0
// parameter: Fd > 0 / Timeout 大于等于 0
// note		: 1.非阻塞SOCKET
//			  2.Timeout 为0时则此函数为阻塞，直到socket异常
//			  3.Timeout 为大于0时则等待时间未Timeout 秒
//******************************************************
int RH_GetConnectNonblockFd(int Fd ,int Timeout,char *AcceptIp);

int RH_GetConnectNonblockIPV6Fd(int Fd , int Timeout, char *AcceptIp, int ipLen);

// *****************************************************
// function	: 简单粗暴关闭socket
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_RoughClose(int Fd);  //简单粗暴 有弊端 但推荐



// *****************************************************
// function	: 非简单粗暴关闭socket
// author 	: zhengyb		2014.9.1
// return   : Succes 0 / fail -1
// parameter: Fd > 0
//******************************************************
int RH_NonroughClose(int Fd);



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
int RH_TcpSndNonblockFd(int Fd,char *SndBuf,int *SndLen,int Timeout);



// *****************************************************
// function	: 以阻塞模式TCP SOCKET 发送数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            SndBuf 不为NULL
//			  SndLen 不为NULL ；*SndLen等于要发送数据长度 ；函数返回后*SndLen为已发送长度
// note		: 1.阻塞SOCKET
//			  2.此函数配合 RH_SetSndTimeoutFd 函数设定发送超时 更佳
//******************************************************
int RH_TcpSndBlockFd(int Fd,char *SndBuf,int *SndLen);


// *****************************************************
// function	: 以非阻塞模式TCP SOCKET 接收数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            RcvBuf 不为NULL
//			  RcvLen 不为NULL ；*RcvLen 等于要发送数据长度 ；函数返回后*RcvLen 为已发送长度
//  		  Timeout 小于0 此函数为无超时阻塞模式 ；
//			  Timeout 等于0 此函数为即时返回非阻塞模式；
//			  Timeout 大于0 此函数为有超时非阻塞模式；
//			  Timeout 单位毫秒  !!!!!!!
// note		: 1.非阻塞SOCKET
//			  2.接收大数据 KB单位 例如整帧数据
//			  3.Timeout 作为Select 设定超时  并不是此函数调用超时
//******************************************************
int RH_TcpRcvNonblockFd(int Fd,char *RcvBuf,int *RcvLen,int Timeout);



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
int RH_TcpRcvBlockFd(int Fd,char *RcvBuf,int needlen,int *RcvLen);



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
int RH_UdpSndNonblockFd(int Fd,char *SndIp,int SndPort,char *SndBuf,int *SndLen,int Timeout);


// *****************************************************
// function	: 以阻塞模式UDP SOCKET 发送数据
// author 	: zhengyb		2014.9.1
// return   : Succes 0  fail -1
// parameter: Fd > 0
//            SndBuf 不为NULL
//			  SndLen 不为NULL ；*SndLen等于要发送数据长度 ；函数返回后*SndLen为已发送长度
//			  SndIp   不为NULL 发送目的地IP
//			  SndPort 不为NULL 发送目的地端口
// note		: 1.阻塞SOCKET
//			  2.此函数配合 RH_SetSndTimeoutFd 函数设定发送超时 更佳
//******************************************************
int RH_UdpSndBlockFd(int Fd,char *SndIp,int SndPort,char *SndBuf,int *SndLen);


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
int RH_UdpRcvNonblockFd(int Fd,char *SndIp,int *SndPort,char *RcvBuf,int *RcvLen,int Timeout);


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
int RH_UdpRcvBlockFd(int Fd,char *SndIp,int SndPort,char *RcvBuf,int *RcvLen);

int RH_UdpRcvBlockFd2(int Fd,char *SndIp,int SndPort,char *RcvBuf,int *RcvLen);


int32_t RH_Socket(char *file,char *func,int32_t domain, int32_t type, int32_t protocol);

int32_t RH_Close(char *file,char *func,int32_t fd);
int RH_GetPrivateError();

/* 获取本地回环地址 */
int get_localip(char* outip);


#ifdef __cplusplus
}
#endif
#if 0

ERROR INFO Linux version 2.6.32-71.el6.x86_64

/usr/include/asm-generic/errno.h
/usr/include/asm-generic/errno-base.h


#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 	 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */

#define	EDEADLK		35	/* Resource deadlock would occur */
#define	ENAMETOOLONG	36	/* File name too long */
#define	ENOLCK		37	/* No record locks available */
#define	ENOSYS		38	/* Function not implemented */
#define	ENOTEMPTY	39	/* Directory not empty */
#define	ELOOP		40	/* Too many symbolic links encountered */
#define	EWOULDBLOCK	EAGAIN	/* Operation would block */
#define	ENOMSG		42	/* No message of desired type */
#define	EIDRM		43	/* Identifier removed */
#define	ECHRNG		44	/* Channel number out of range */
#define	EL2NSYNC	45	/* Level 2 not synchronized */
#define	EL3HLT		46	/* Level 3 halted */
#define	EL3RST		47	/* Level 3 reset */
#define	ELNRNG		48	/* Link number out of range */
#define	EUNATCH		49	/* Protocol driver not attached */
#define	ENOCSI		50	/* No CSI structure available */
#define	EL2HLT		51	/* Level 2 halted */
#define	EBADE		52	/* Invalid exchange */
#define	EBADR		53	/* Invalid request descriptor */
#define	EXFULL		54	/* Exchange full */
#define	ENOANO		55	/* No anode */
#define	EBADRQC		56	/* Invalid request code */
#define	EBADSLT		57	/* Invalid slot */

#define	EBFONT		59	/* Bad font file format */
#define	ENOSTR		60	/* Device not a stream */
#define	ENODATA		61	/* No data available */
#define	ETIME		62	/* Timer expired */
#define	ENOSR		63	/* Out of streams resources */
#define	ENONET		64	/* Machine is not on the network */
#define	ENOPKG		65	/* Package not installed */
#define	EREMOTE		66	/* Object is remote */
#define	ENOLINK		67	/* Link has been severed */
#define	EADV		68	/* Advertise error */
#define	ESRMNT		69	/* Srmount error */
#define	ECOMM		70	/* Communication error on send */
#define	EPROTO		71	/* Protocol error */
#define	EMULTIHOP	72	/* Multihop attempted */
#define	EDOTDOT		73	/* RFS specific error */
#define	EBADMSG		74	/* Not a data message */
#define	EOVERFLOW	75	/* Value too large for defined data type */
#define	ENOTUNIQ	76	/* Name not unique on network */
#define	EBADFD		77	/* File descriptor in bad state */
#define	EREMCHG		78	/* Remote address changed */
#define	ELIBACC		79	/* Can not access a needed shared library */
#define	ELIBBAD		80	/* Accessing a corrupted shared library */
#define	ELIBSCN		81	/* .lib section in a.out corrupted */
#define	ELIBMAX		82	/* Attempting to link in too many shared libraries */
#define	ELIBEXEC	83	/* Cannot exec a shared library directly */
#define	EILSEQ		84	/* Illegal byte sequence */
#define	ERESTART	85	/* Interrupted system call should be restarted */
#define	ESTRPIPE	86	/* Streams pipe error */
#define	EUSERS		87	/* Too many users */
#define	ENOTSOCK	88	/* Socket operation on non-socket */
#define	EDESTADDRREQ	89	/* Destination address required */
#define	EMSGSIZE	90	/* Message too long */
#define	EPROTOTYPE	91	/* Protocol wrong type for socket */
#define	ENOPROTOOPT	92	/* Protocol not available */
#define	EPROTONOSUPPORT	93	/* Protocol not supported */
#define	ESOCKTNOSUPPORT	94	/* Socket type not supported */
#define	EOPNOTSUPP	95	/* Operation not supported on transport endpoint */
#define	EPFNOSUPPORT	96	/* Protocol family not supported */
#define	EAFNOSUPPORT	97	/* Address family not supported by protocol */
#define	EADDRINUSE	98	/* Address already in use */
#define	EADDRNOTAVAIL	99	/* Cannot assign requested address */
#define	ENETDOWN	100	/* Network is down */
#define	ENETUNREACH	101	/* Network is unreachable */
#define	ENETRESET	102	/* Network dropped connection because of reset */
#define	ECONNABORTED	103	/* Software caused connection abort */
#define	ECONNRESET	104	/* Connection reset by peer */
#define	ENOBUFS		105	/* No buffer space available */
#define	EISCONN		106	/* Transport endpoint is already connected */
#define	ENOTCONN	107	/* Transport endpoint is not connected */
#define	ESHUTDOWN	108	/* Cannot send after transport endpoint shutdown */
#define	ETOOMANYREFS	109	/* Too many references: cannot splice */
#define	ETIMEDOUT	110	/* Connection timed out */
#define	ECONNREFUSED	111	/* Connection refused */
#define	EHOSTDOWN	112	/* Host is down */
#define	EHOSTUNREACH	113	/* No route to host */
#define	EALREADY	114	/* Operation already in progress */
#define	EINPROGRESS	115	/* Operation now in progress */
#define	ESTALE		116	/* Stale NFS file handle */
#define	EUCLEAN		117	/* Structure needs cleaning */
#define	ENOTNAM		118	/* Not a XENIX named type file */
#define	ENAVAIL		119	/* No XENIX semaphores available */
#define	EISNAM		120	/* Is a named type file */
#define	EREMOTEIO	121	/* Remote I/O error */
#define	EDQUOT		122	/* Quota exceeded */

#define	ENOMEDIUM	123	/* No medium found */
#define	EMEDIUMTYPE	124	/* Wrong medium type */
#define	ECANCELED	125	/* Operation Canceled */
#define	ENOKEY		126	/* Required key not available */
#define	EKEYEXPIRED	127	/* Key has expired */
#define	EKEYREVOKED	128	/* Key has been revoked */
#define	EKEYREJECTED	129	/* Key was rejected by service */

/* for robust mutexes */
#define	EOWNERDEAD	130	/* Owner died */
#define	ENOTRECOVERABLE	131	/* State not recoverable */

#define ERFKILL		132	/* Operation not possible due to RF-kill */


#endif


#endif





