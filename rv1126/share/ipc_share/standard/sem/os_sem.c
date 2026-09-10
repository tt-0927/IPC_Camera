
#include <time.h>
#include "os_sem.h"
#include <errno.h>
#define OS_SOK      0  ///< Status : OK
#define OS_EFAIL   -1  ///< Status : Generic error

#ifdef WIN32
#include <winsock.h>    //struct timeval
#endif


int OS_semCreate(OS_SemHndl *hndl, uint32_t maxCount, uint32_t initVal)
{
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;
  int status=OS_SOK;

  status |= pthread_mutexattr_init(&mutex_attr);
  status |= pthread_condattr_init(&cond_attr);
#ifndef WIN32
  status |= pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);	//采用绝对时间做超时
#endif

  status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
  status |= pthread_cond_init(&hndl->cond, &cond_attr);

  hndl->count = initVal;
  hndl->maxCount = maxCount;

  if(hndl->maxCount==0)
    hndl->maxCount=1;

  if(hndl->count>hndl->maxCount)
    hndl->count = hndl->maxCount;

  if(status!=OS_SOK)
    printf("OS_semCreate() = %d \r\n", status);

  pthread_condattr_destroy(&cond_attr);
  pthread_mutexattr_destroy(&mutex_attr);

  return status;
}

int OS_semWait(OS_SemHndl *hndl, int32_t timeout, uint32_t *remainVal)
{
  int status = OS_EFAIL;

  pthread_mutex_lock(&hndl->lock);

  while(1) {
    if(hndl->count > 0)
    {
	  if( hndl->count != 0)
		hndl->count--;
      if(remainVal)
      {
    	  *remainVal = hndl->count;	//返回剩余有多少信号量
      }
      status = OS_SOK;
      break;
    } else
    {
      if(timeout==0)
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
		unsigned long long us = outtime.tv_nsec/1000 + 1000 * (timeout % 1000); //微秒
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
		ret = pthread_cond_timedwait(&hndl->cond, &hndl->lock, &outtime);
		if(ret == 0)
		{
			status = OS_SOK;	//在等待时间内有信号触发
			if( hndl->count != 0)
				hndl->count--;
		}
		if(remainVal)
		{
			*remainVal = hndl->count;	//返回剩余有多少信号量
		}
    /* 超时返回超时码 */
    if (ETIMEDOUT == ret)
    {
      status = 110;
    }
    break;
      }else
      {
    	  pthread_cond_wait(&hndl->cond, &hndl->lock);
    	  status = OS_SOK;
      }
    }
  }

  pthread_mutex_unlock(&hndl->lock);

  return status;
}

int OS_semSignal(OS_SemHndl *hndl)
{
  int status = OS_SOK;

  pthread_mutex_lock(&hndl->lock);

  if(hndl->count<hndl->maxCount) {
    hndl->count++;
    status |= pthread_cond_signal(&hndl->cond);
  }

  pthread_mutex_unlock(&hndl->lock);

  return status;
}
int OS_semBroad(OS_SemHndl *hndl)
{
  int status = OS_SOK;

  pthread_mutex_lock(&hndl->lock);

  status |= pthread_cond_broadcast(&hndl->cond);

  pthread_mutex_unlock(&hndl->lock);

  return status;
}
int OS_semWaitAll( OS_SemHndl *hndl )
{
  pthread_mutex_lock(&hndl->lock);
  pthread_cond_wait(&hndl->cond, &hndl->lock);
  pthread_mutex_unlock(&hndl->lock);
   return 0;
}

int OS_semDelete(OS_SemHndl *hndl)
{
  pthread_cond_destroy(&hndl->cond);
  pthread_mutex_destroy(&hndl->lock);

  return OS_SOK;
}


