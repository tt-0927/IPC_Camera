


#include "minHeapTimer.h"
#include "minheap_event.h"
#include <unistd.h>

int minHeap_timer_create(minTimer_t *handle)
{
	memset(handle,0,sizeof(minTimer_t));
    min_heap_ctor_(&(handle->_min_heap));
    OS_mutexCreate(&(handle->minHeap_mutex));
	return 0;
}

void minHeap_timer_destroy(minTimer_t *handle)
{
    unsigned int i = 0;
    OS_mutexLock(&(handle->minHeap_mutex));

    for (i = 0; i < handle->_min_heap.n; i++)
    {
        free(handle->_min_heap.p[i]);
    }
    min_heap_dtor_(&(handle->_min_heap));

    OS_mutexUnlock(&(handle->minHeap_mutex));
}


unsigned int minHeap_timer_add(minTimer_t *handle, unsigned long long interval, int(*fun)(void*), void* arg,
                              int flag, int exe_num)
{
    struct event * ev = (struct event*) malloc(sizeof(struct event));
	if (NULL == ev)
	{
		printf("malloc error!!!\n");
        return -1;
	}
	
    min_heap_elem_init_(ev);

    struct timeval now;
    gettime(&now);
    ev->ev_interval.tv_sec = interval / 1000;
    ev->ev_interval.tv_usec = (interval % 1000) * 1000;
    evutil_timeradd(&now, &(ev->ev_interval), &(ev->ev_timeout));
    ev->ev_flags = flag;
    ev->ev_callback = fun;
    ev->ev_arg = arg;
    ev->ev_exe_num = exe_num;
    ev->timer_id = ++handle->_timer_id;	//从1开始
 
    OS_mutexLock(&(handle->minHeap_mutex));
    min_heap_push_(&(handle->_min_heap), ev);
    OS_mutexUnlock(&(handle->minHeap_mutex));

    return ev->timer_id;
}

int minHeap_timer_remove(minTimer_t *handle,unsigned int timer_id)
{
    unsigned int i = 0;

    OS_mutexLock(&(handle->minHeap_mutex));
    for (i = 0; i < handle->_min_heap.n; i++)
    {
        if (timer_id == handle->_min_heap.p[i]->timer_id)
        {
            struct event * e = handle->_min_heap.p[i];
            min_heap_erase_(&(handle->_min_heap), handle->_min_heap.p[i]);
            free(e);
            break;//return 1
        }
    }
    OS_mutexUnlock(&(handle->minHeap_mutex));

    return 0;
}

int minHeap_timer_allremove(minTimer_t *handle)
{
    unsigned int i = 0;
    struct event * e;
    OS_mutexLock(&(handle->minHeap_mutex));
    for (i = 0; i < handle->_min_heap.n; i++)
    {
      
        e = handle->_min_heap.p[i];
        min_heap_erase_(&(handle->_min_heap), handle->_min_heap.p[i]);
        free(e);
//return 1    
    }
    OS_mutexUnlock(&(handle->minHeap_mutex));

    return 0;
}
 


int minHeap_timer_mod(minTimer_t *handle,unsigned int timer_id,unsigned long long interval,int flag, int exe_num)
{
	int ret = -1;
    unsigned int i = 0;

    OS_mutexLock(&(handle->minHeap_mutex));

    for (i = 0; i < handle->_min_heap.n; i++)
    {
        if (timer_id == handle->_min_heap.p[i]->timer_id)
        {
            struct event * event = handle->_min_heap.p[i];
            struct timeval now;
            gettime(&now);
            event->ev_interval.tv_sec = interval / 1000;
            event->ev_interval.tv_usec = (interval % 1000) * 1000;
            event->ev_flags = flag;
            event->ev_exe_num = exe_num;

            evutil_timeradd(&(now), &(event->ev_interval), &(event->ev_timeout));
            min_heap_adjust_(&(handle->_min_heap), event);
            ret = 0;
            break;
        }
    }

    OS_mutexUnlock(&(handle->minHeap_mutex));

    return ret;
}


static struct event* min_heap_top_mutex(OS_MutexHndl* mutex, min_heap_t* minHeap)
{
	struct event *event = NULL;
	OS_mutexLock(mutex);
	event = min_heap_top_(minHeap);
	OS_mutexUnlock(mutex);
	return event;
}


int minHeap_timer_process(minTimer_t *handle)
{
    struct event *event = NULL;
    struct timeval now;
 
    //while ((event = min_heap_top_(&(handle->_min_heap))) != NULL)
    while ((event = min_heap_top_mutex(&(handle->minHeap_mutex),&(handle->_min_heap))) != NULL)
    {
        gettime(&now);
        if (evutil_timercmp(&now, &(event->ev_timeout), < ))
        {
            break;
        }

        OS_mutexLock(&(handle->minHeap_mutex));
        min_heap_pop_(&(handle->_min_heap));	//弹出超时句柄
        OS_mutexUnlock(&(handle->minHeap_mutex));

        event->ev_callback(event->ev_arg);		//执行回调函数
 
        if (event->ev_flags == CYCLE_TIMER
                || (event->ev_flags == LIMIT_TIMER && --event->ev_exe_num > 0))
        {
            evutil_timeradd(&(event->ev_timeout), &(event->ev_interval), &(event->ev_timeout));

            OS_mutexLock(&(handle->minHeap_mutex));
            min_heap_push_(&(handle->_min_heap), event);
            OS_mutexUnlock(&(handle->minHeap_mutex));
        }
        else
        {
            free(event);	//定时器超时，执行完毕后，释放资源
        }
    }
 
    return 0;
}



static void* minHeap_timer_thr(void* argv)
{

	minTimer_t* handle = (minTimer_t*)argv;

	while(1)
	{
		//执行
		minHeap_timer_process(handle);

		//判断是否退出
		if(handle->isExit == 1)
		{
			printf("exit minheap timer!!!\n");
			break;
		}
		usleep(handle->precision*1000);	//定时器的精度
	}

	//释放资源
	minHeap_timer_destroy(handle);

	return NULL;
}


int minHeap_timer_init(minTimer_t *handle,int precision)
{
	int ret = 0;
	minHeap_timer_create(handle);

	handle->precision = precision;
	handle->isExit = 0;

	//creater thr
	ret = OS_thrCreate(&(handle->tid),minHeap_timer_thr,OS_DETACH,OS_THR_STACK_SIZE_DEFAULT,(void*)handle);
	if(ret < 0)
	{
		printf("creater thr error!!\n");
		minHeap_timer_destroy(handle);
		return -1;
	}

	return 0;
}


int minHeap_timer_unInit(minTimer_t *handle)
{
	handle->isExit = 1;
	return 0;
}










