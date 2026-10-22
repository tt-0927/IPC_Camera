

#include "os_que.h"
#include "os_debug.h"

#ifdef WIN32
#include <winsock.h>    //struct timeval
#endif

int OS_queCreate(OS_QueHndl *hndl, Uint32 maxLen)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status=OS_SOK;

	hndl->curRd = hndl->curWr = 0;
	hndl->count = 0;
	hndl->len   = maxLen;
	hndl->queue = OS_memAlloc(sizeof(Int64)*hndl->len);

	if (hndl->queue==NULL)
	{
		OS_ERROR("OS_queCreate() = %d \r\n", status);
		return OS_EFAIL;
	}

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  
#ifndef WIN32
  status |= pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);	//采用绝对时间做超时
#endif

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
	status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

	if (status!=OS_SOK)
		OS_ERROR("OS_queCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

int OS_queDelete(OS_QueHndl *hndl)
{
	if(hndl->queue!=NULL)
	{
		OS_memFree(hndl->queue);
		hndl->queue = NULL;
	}
		

	pthread_cond_destroy(&hndl->condRd);
	pthread_cond_destroy(&hndl->condWr);
	pthread_mutex_destroy(&hndl->lock);  

	return OS_SOK;
}

int OS_queCopy(OS_QueHndl *hndl,OS_QueHndl *cphndl)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status=OS_SOK;

	hndl->curRd = cphndl->curRd;
	hndl->curWr = cphndl->curWr;
	hndl->count = cphndl->count;
	hndl->len   = cphndl->len;
	hndl->queue = OS_memAlloc(sizeof(Int64)*cphndl->len);

	if (hndl->queue==NULL)
	{
		OS_ERROR("OS_queCopy() = %d \r\n", status);
		return OS_EFAIL;
	}

	memcpy(hndl->queue,cphndl->queue,sizeof(Int64) * cphndl->len);

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  
#ifndef WIN32
  status |= pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);	//采用绝对时间做超时
#endif

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
	status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

	if (status!=OS_SOK)
		OS_ERROR("OS_queCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

//#include <iostream>
/*
 * 往队列中写入数据
 * */
int OS_quePut(OS_QueHndl *hndl, Int64 value, Int32 timeout)
{
	int status = OS_EFAIL;

	pthread_mutex_lock(&hndl->lock);
	while(1) 
	{
		if ( hndl->count < hndl->len )
		{
			hndl->queue[hndl->curWr] = value;
			hndl->curWr = (hndl->curWr+1)%hndl->len;
			hndl->count++;
			status = OS_SOK;
			pthread_cond_signal(&hndl->condRd);
			break;
		} 
		else
		{
	  		if(timeout == OS_TIMEOUT_NONE)
	    		break;

	  		status = pthread_cond_wait(&hndl->condWr, &hndl->lock);
		}
	}

	pthread_mutex_unlock(&hndl->lock);

  return status;
}

/*
 * 从队列中获取数据
 * */
int OS_queGet(OS_QueHndl *hndl, Int64 *value, Int32 timeout)
{
	int status = OS_EFAIL;

	pthread_mutex_lock(&hndl->lock);

	while(1) 
	{
		if(hndl->count > 0 )
		{

		  	if (value!=NULL) 
			{
		    	*value = hndl->queue[hndl->curRd];
		  	}
		  
		  	hndl->curRd = (hndl->curRd+1)%hndl->len;
		  	hndl->count--;
		  	status = OS_SOK;
		  	pthread_cond_signal(&hndl->condWr);
		  	break;
		}
		else
		{
	  		if (timeout == OS_TIMEOUT_NONE)
            {
	    		break;
            }else if(timeout > 0)
            {
              //超时等待
              int ret = 0;
              struct timespec outtime;
              memset(&outtime,0,sizeof(struct timespec));

#ifndef WIN32
              //初始化属性的时候需要，需要配置CLOCK_MONOTONIC，否则无法生效
              clock_gettime(CLOCK_MONOTONIC, &outtime);
              outtime.tv_sec += timeout/1000;
              //在outtime的基础上，增加ms毫秒
              //outtime.tv_nsec为纳秒，1微秒=1000纳秒
              //tv_nsec此值再加上剩余的毫秒数 ms%1000，有可能超过1秒。需要特殊处理
              Uint16  us = outtime.tv_nsec/1000 + 1000 * (timeout % 1000); //微秒
              outtime.tv_sec += us / 1000000;//us的值有可能超过1秒，
              us = us % 1000000;
              outtime.tv_nsec = us * 1000;//换算成纳秒
#else
              struct timeval now;
              gettimeofday(&now, NULL);
              int nsec = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
              outtime.tv_nsec = nsec % 1000000000;
              outtime.tv_sec = now.tv_sec + nsec / 1000000000 + timeout / 1000;
#endif
              ret = pthread_cond_timedwait(&hndl->condRd, &hndl->lock, &outtime);
              if(ret == 0)
              {
                  status = OS_SOK;	//在等待时间内有信号触发
              }else
              {
                  break;    //等待时间内没有信号触发，直接退出
              }

            }else
            {
                status = pthread_cond_wait(&hndl->condRd, &hndl->lock);
            }
		}
	}

	pthread_mutex_unlock(&hndl->lock);

  	return status;
}

/*
 * 获取当前队列中有多少个数据
 * */
Uint32 OS_queGetQueuedCount(OS_QueHndl *hndl)
{
	Uint32 queuedCount = 0;

	pthread_mutex_lock(&hndl->lock);
	queuedCount = hndl->count;
	pthread_mutex_unlock(&hndl->lock);
	return queuedCount;
}

/*
 * 从队列头中获取一个数据，但不读走数据，即读index不往前移位
 * 只是查看队列头中的数据是什么
 * */
int OS_quePeek(OS_QueHndl *hndl, Int64 *value)
{
	int status = OS_EFAIL;
	pthread_mutex_lock(&hndl->lock);
	if (hndl->count > 0 )
	{
	  if (value!=NULL)
	  {
	    *value = hndl->queue[hndl->curRd];
	    status = OS_SOK;
	  }
	}
	pthread_mutex_unlock(&hndl->lock);

	return status;
}

/*
 * 查看队列是否为空
 * 返回真即为空
 * 返回假即为有数据
 * */
int OS_queIsEmpty(OS_QueHndl *hndl)
{
    int isEmpty = 0;

	pthread_mutex_lock(&hndl->lock);
	if (hndl->count == 0)
	{
	  isEmpty = TRUE;
	}
	else
	{
	  isEmpty = FALSE;
	}
	pthread_mutex_unlock(&hndl->lock);

	return isEmpty;
}
int OS_queIsFull(OS_QueHndl *hndl)
{
    int isFull;
	pthread_mutex_lock(&hndl->lock);
	if ( hndl->count < hndl->len )
	{
		isFull = FALSE;
	}
	else
	{
		isFull = TRUE;
	}
	pthread_mutex_unlock(&hndl->lock);
	return isFull;
}
