
#ifndef _OS_H_
#define _OS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef WIN32
//#pragma comment(lib,"pthreadVC2.lib")
#include "pthread.h"
#else
#include <pthread.h>
#endif




#define OS_DEBUG_MODE // enable OSA_printf, OSA_assert
#define OS_DEBUG_FILE // enable printf's during OSA_fileXxxx
#define OS_PRF_ENABLE // enable profiling APIs

#define OS_SOK      0  ///< Status : OK
#define OS_EFAIL   -1  ///< Status : Generic error

#ifndef _TI_STD_TYPES
#define _TI_STD_TYPES

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifndef __cplusplus
#define Bool int
#endif
//#define Bool int

/* unsigned quantities */
typedef unsigned long long Uint64;      ///< Unsigned 64-bit integer
typedef unsigned int Uint32;            ///< Unsigned 32-bit integer
typedef unsigned short Uint16;          ///< Unsigned 16-bit integer
typedef unsigned char Uint8;            ///< Unsigned  8-bit integer
typedef char 	Int8;            ///< Unsigned  8-bit integer

/* signed quantities */
typedef long long Int64;               ///< Signed 64-bit integer
typedef int Int32;               ///< 

typedef unsigned int Uns;              ///< Unsigned int

typedef   void *  OS_PTR;

#define OS_SUSPEND     (0xFFFFFFFF)
#define OS_NO_SUSPEND  (0)



#endif /* _TI_STD_TYPES */

#ifndef KB
#define KB ((Uint32)1024)
#endif

#ifndef MB
#define MB (KB*KB)
#endif

#define OS_TIMEOUT_NONE        ((Uint32) 0)  // no timeout
#define OS_TIMEOUT_FOREVER     ((Uint32)-1)  // wait forever

#define OS_memAlloc(size)      (void*)malloc((size))
#define OS_memFree(ptr)        free(ptr)

#define OS_align(value, align)   ((( (value) + ( (align) - 1 ) ) / (align) ) * (align) )

#define OS_floor(value, align)   (( (value) / (align) ) * (align) )
#define OS_ceil(value, align)    OS_align(value, align)

#define OS_SNPRINTF(sbuf,...)                                               \
                                do {                                           \
                                    snprintf (sbuf, sizeof(sbuf) - 1,          \
                                              __VA_ARGS__);                    \
                                    sbuf[sizeof(sbuf) - 1] = 0;                \
                                } while (0)

#define OS_ARRAYSIZE(array)             ((sizeof(array)/sizeof((array)[0])))

#define OS_ARRAYINDEX(elem,array)       ((elem) - &((array)[0]))

#define OS_ARRAYISVALIDENTRY(elem,array) ((OSA_ARRAYINDEX(elem,array) <   \
                                             OSA_ARRAYSIZE(array))           \
                                             ? TRUE                            \
                                             : FALSE)
#define OS_DIV(num,den)                  (((den) != 0) ? ((num)/(den)) : 0)

#define OS_ISERROR(status)               ((status < 0) ? TRUE : FALSE)


typedef struct _OS_DATE_TIME_INFO_
{
	int year;
	int month;
	int mday;
	int hours;
	int min;
	int sec;
}OS_DateTimeInfo_t;


/*
 * 封装win与linx的usleep函数
 * 单位ms
*/
Int32 os_msleep(Uint64 ms);

/*
 *	获取时间戳相对于第一次调用时的相对时间戳（毫秒级的系统时间）
 *	@out param: 返回毫秒级的相对时间（时间戳）
 */
Uint32 OS_getCurTimeInMsec();

/*
 *	获取系统时间戳（毫秒级的系统时间）
 *	@out param: 返回毫秒级的系统时间（时间戳）
 */
Uint32 OS_getSysTimeInMsec();


/*
 *	获取获取从UTC1970-1-1 0:0:0到现在的毫秒数,系统时间戳（毫秒级的系统时间）
 *	@out param: 返回毫秒级的系统时间（时间戳）
 */
Uint64 OS_getRealTimeInMsec();

Int32 OS_getSys_DateTime(OS_DateTimeInfo_t *dtinfo);

#ifndef WIN32

/*
 *	程序等待 msecs 毫秒
 *	@in param msecs : 等待多少毫秒
 *	@out param:	无返回
 */
void   OS_waitMsecs(Uint32 msecs);


/*
 *	捕捉系统上抛的信号，同时执行handler函数指针（信号处理函数）
 *	包括linux系统的所有信号，如SIGHUP信号
 *
 *	@in param sinId : 要捕捉的系统信号如SIGHUP
 *	@in param handler :信号处理函数
 *	@out param:	返回OSA_SOK
 */
int OS_attachSignalHandler(int sigId, void (*handler)(int ) );

/*
 *	用于创建进程
 *	@in param pCmd : 执行命令
 *	@out param:	0
 */
int OS_forkApp(char* pCmd);

#endif

/*
 *	将十六进制数转为十进制数
 */
int os_xstrtoi(char *hex);

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp);
#endif

/*
 * 帧率打印
*/
int frameRate(const char *Rate, long FlagTime);

#ifdef __cplusplus
}
#endif
#endif /* _OS_H_ */



