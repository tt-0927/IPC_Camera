#ifndef _PUBLIC_DEFINE_
#define _PUBLIC_DEFINE_

#ifdef WIN32
#include "windows.h"
#include "winsock.h"
#define sleep(p) Sleep(p * 1000)
#define __func__ __FUNCTION__
#define WITH_ALL __FUNCTION__, __FILE__, __LINE__
#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <sys/un.h>
#include <stdarg.h>
#include <unistd.h>

#define ioctlsocket ioctl
#define WITH_ALL __func__, __FILE__, __LINE__
#endif

#define MAX_IPV6_BUF_LEN 64

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef INT32
#define INT32 int
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

typedef unsigned char UINT8;

typedef unsigned int UNWORD;

typedef unsigned long int DWORD;

typedef unsigned int UINT;

typedef unsigned char BYTE;

/* basic data type definitions */
typedef unsigned char CL_U8;
typedef unsigned short CL_U16;
typedef unsigned int CL_U32;
typedef unsigned long CL_UL;

typedef signed char CL_S8;
typedef short CL_S16;
typedef int CL_S32;
typedef long CL_SL;

typedef float CL_FLOAT;
typedef double CL_DOUBLE;

typedef char CL_CHAR;

typedef struct _ScSystemTime
{
	UNWORD wYear;
	UNWORD wMonth;
	UNWORD wDayOfWeek;
	UNWORD wDay;
	UNWORD wHour;
	UNWORD wMinute;
	UNWORD wSecond;
	UNWORD wMilliseconds;
} ScSystemTime;

// 错误信息标识
typedef enum
{
	RET_SUCCESS                 = 0,   /* 成功 */
	RET_NULLRETURN              = -1,  /* 空返回 */
	RET_PARAMER_ERR             = -2,  /* 参数错误 */
	RET_NOTFOUND_PATH           = -3,  /* 访问路径不存在 */
	RET_NOT_EXIST               = -4,  /* 不存在 */
	RET_TYPE_ERR                = -5,  /* 类型错误 */
	RET_OPENFILE_FAILED         = -6,  /* 打开文件失败 */
	RET_INFO_LONG               = -7,  /* 信息太长 */
	RET_MEMORY_FAIL             = -8,  /* 内存分配失败 */
	RET_CREATE_FAILED           = -9,  /* 创建失败 */
	RET_NOTSUPPORT_VERSION      = -10, /* 不支持的版本 */
	RET_LOGIC_ERROR             = -11, /* 逻辑错误 */
	RET_MEMORY_FREE             = -12, /* 内存释放失败 */
	RET_PARSE_ERR               = -13, /* 解析错误 */
	RET_NODE_ERR                = -14, /* 节点错误 */
	RET_SEND_ERR                = -15, /* 发送失败 */
	RET_RECV_ERR                = -16, /* 接收失败 */
	RET_DIS_CONNECT             = -17, /* 断开连接 */
	RET_STOP_RUN                = -18, /* 已停止 */
	RET_FREAD_FAIL              = -19, /* 读文件失败 */
	RET_FWRITE_FAIL             = -20, /* 写文件失败 */
	RET_FILE_END                = -21, /* 文件尾 */
	RET_OUT_TIME                = -22, /* 超时 */
	RET_ERR_STRUCT              = -23, /* record发送的结构体长度错误 */
	RET_UKNOW_CMD               = -24, /* 未知命令 */
	RET_CONFLICT_CMD            = -25, /* record发送的命令与录制状态冲突 */
	RET_OSDTEXT_FAIL            = -26, /* osd设置字幕转换错误 */
	RET_NETWORKIP_FALL          = -27, /* 获取/设置网络IP错误 */
	RET_NETWORKMASK_FALL        = -28, /* 获取/设置网络掩码错误 */
	RET_NETWORKGATE_FALL        = -29, /* 获取/设置网络网关错误 */
	RET_NETWORKDNS_FALL         = -30, /* 获取/设置网络DNS 错误 */
	RET_SETSYSTIME_FALL         = -31, /* 设置系统时间错误 */
	RET_SETMOVIEMODE_FALL       = -32, /* 设置电影模式错误 */
	RET_REBOOT_FALL             = -33, /* 重启失败 */
	RET_SOCKET_FALL             = -34, /* socket 失败 */
	RET_SYSTEM_FALL             = -35, /* 系统命令错误 */
	RET_SET_MAIN_ENC            = -36, /* 设置编码参数错误 */
	RET_DEFINE                  = -37, /* 可自定义失败 */
	RET_SETUSEDEFMOVIEMODE_FALL = -38, /* 设置用户自定义电影模式错误 */
	RET_UNKNOW_FAIL             = -39, /* 未知错误 */
	RET_ADMINORGUEST            = -40, /* 管理员或者客人 */
	RET_XML_TO_STR_FAIL         = -41, /* xml转字符串失败 */
	RET_NULL_PTR                = -42  /* 空指针 */
} RetErr_t;

#endif
