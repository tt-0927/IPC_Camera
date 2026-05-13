

#ifndef _OS_THR_H_
#define _OS_THR_H_

#include "os.h"
#ifdef __cplusplus
extern "C" {
#endif

#define OS_THR_PRI_MAX                 sched_get_priority_max(SCHED_FIFO)
#define OS_THR_PRI_MIN                 sched_get_priority_min(SCHED_FIFO)

#define OS_THR_PRI_DEFAULT             ( OS_THR_PRI_MIN + (OS_THR_PRI_MAX-OS_THR_PRI_MIN)/2 )

#define OS_THR_STACK_SIZE_DEFAULT      0

typedef void * (*OS_ThrEntryFunc)(void *);

typedef enum
{
	OS_DETACH,		/*分离线程*/
	OS_JOINABLE		/*可结合线程*/
}OS_ThrType_t;

typedef struct 
{
 	pthread_t	hndl;
} OS_ThrHndl;

/*
 * create thread
 * @in param hndl : thread handle
 * @in param entryFunc : 创建线程的函数
 * @in param pri : 设置线程是分离还是结合线程，取值范围看OS_ThrType_t
 * @in param stackSize : 自定义设置线程的堆栈大小，若赋值OS_THR_STACK_SIZE_DEFAULT则系统自动分配
 * @in param prm : 传给entryFunc函数的参数
 *
 * */
int OS_thrCreate(OS_ThrHndl *hndl, OS_ThrEntryFunc entryFunc, OS_ThrType_t pri, Uint32 stackSize, void *prm);

/*
 * 改变线程的优先级
 * @in param pri : 要设置线程的优先级
 * OS_THR_PRI_MIN < pri < OS_THR_PRI_MAX
 * */
int OS_thrChangePri(OS_ThrHndl *hndl, Uint32 pri);

/*
 * 线程内部自己调用，退出线程
 * @in param returnVal : 线程退出返回的值
 * */
int OS_thrExit(void *returnVal);

/*
 * 直接取消线程，线程会马上停止运行，但是用户申请的资源不会自动释放，有可能造成内存泄漏
 * @in param hndl : thread handle
 * */
int OS_thrDelete(OS_ThrHndl *hndl);


/*
 * 可结合线程属性（默认），需要使用该函数来释放资源
 * 分离属性的线程，使用该函数是无效的，分离线程退出后，线程资源会自动释放
 * 可结合线程调用该函数，会阻塞等待线程结束
 * */
int OS_thrJoin(OS_ThrHndl *hndl);

#ifdef __cplusplus
}
#endif
#endif /* _OS_THR_H_ */



