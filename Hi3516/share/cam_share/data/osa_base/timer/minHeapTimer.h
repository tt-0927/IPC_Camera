

#ifndef _TIMER_MINHEAP_MYTIMER_H
#define _TIMER_MINHEAP_MYTIMER_H
 
#include "minheap.h"
#include "os_thr.h"
#include "os_mutex.h"

#define LIMIT_TIMER 1 //有限次数定时器
#define CYCLE_TIMER 2 //循环定时器

 

typedef struct _MIN_TIMER_INFO_
{
	struct min_heap _min_heap;
	OS_MutexHndl minHeap_mutex;			//保证线程安全
	unsigned int _timer_id;
	unsigned long long precision;		//精度，单位ms
	OS_ThrHndl tid;
	int isExit;					//1-销毁定时器

}minTimer_t;




/*
 * 初始化最小堆定时器
 * @param[in] handle :定时器句柄
 * @param[in] precision :定时器精度，单位ms，是否能精度到1ms，取决于运行平台
 * */
int minHeap_timer_init(minTimer_t *handle,int precision);

/*
 * 反初始化最小堆定时器
 * @param[in] handle :定时器句柄
 * @param[in] precision :定时器精度，单位ms，是否能精度到1ms，取决于运行平台
 * */
int minHeap_timer_unInit(minTimer_t *handle);

/**************************************
 * input: interval: 每次执行的时间隔间, 单位是毫秒。ms
 *        fun arg : 回调函数以及参数。
 *        flag    : 循环定时器还是有限次数定时器，如果是相对定时器
 *        exe_num : 只有在有限次数定时器才有效，表示执行的次数。最少为1次
 * return: 生成定时器的ID
**************************************/
unsigned int minHeap_timer_add(minTimer_t *handle, unsigned long long interval, int (*fun)(void*), void* arg, int flag, int exe_num);

/***************************************
 * description:
 * 删除已经加入的定时器
	 * 1>加入得定时器已经超时了，则内部会释放资源，不需要调用该函数，即使调用了，内部也会遍历查找定时器id，会查不到的
 * 2>加入的定时器没有超时，可以调用该接口删除定时器
***************************************/
int minHeap_timer_remove(minTimer_t *handle,unsigned int timer_id);
int minHeap_timer_allremove(minTimer_t *handle);

/***************************************
 * description:
 * 修改未超时的定时器，已超时的定时器修改会失败，因为无对应的定时器
 * return:修改成功返回0，未找到定时器返回-1
***************************************/
int minHeap_timer_mod(minTimer_t *handle,unsigned int timer_id,unsigned long long interval,int flag, int exe_num);

/***************************************
 * description:
 * 定时器的循环处理函数，由定时器的拥有者进行循环调用。它的最小时间间隔决定了定时器的精度。
***************************************/
int minHeap_timer_process(minTimer_t *handle);

 

#endif // _TIMER_MINHEAP_MYTIMER_H


