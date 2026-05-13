

#include "os_que.h"
#ifdef WIN32
#include <winsock.h>    //struct timeval
#endif
#define OS_SOK      0  ///< Status : OK
#define OS_EFAIL   -1  ///< Status : Generic error
#define TRUE 1
#define FALSE 0
int OS_queCreate(OS_QueHndl *hndl, uint32_t maxLen)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status=OS_SOK;

	hndl->curRd = hndl->curWr = 0;
	hndl->count = 0;
	hndl->len   = maxLen;
	hndl->queue = malloc(sizeof(int64_t)*hndl->len);

	if (hndl->queue==NULL)
	{
		printf("OS_queCreate() = %d \r\n", status);
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
		printf("OS_queCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

int OS_queDelete(OS_QueHndl *hndl)
{
	if(hndl->queue!=NULL)
		free(hndl->queue);

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
	hndl->queue = malloc(sizeof(int64_t)*cphndl->len);

	if (hndl->queue==NULL)
	{
		printf("OS_queCopy() = %d \r\n", status);
		return OS_EFAIL;
	}

	memcpy(hndl->queue,cphndl->queue,sizeof(int64_t) * cphndl->len);

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);  
#ifndef WIN32
  status |= pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);	//采用绝对时间做超时
#endif

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
	status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

	if (status!=OS_SOK)
		printf("OS_queCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

void timespec_add_ms(struct timespec *ts, long ms) {
    long sec = ms / 1000;
    long nsec = (ms % 1000) * 1000000;

    ts->tv_sec += sec;
    ts->tv_nsec += nsec;

    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}


//#include <iostream>
/*
 * 往队列中写入数据
 * */
int OS_quePut(OS_QueHndl *hndl, int64_t value, int32_t timeout)
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
	  	    if(timeout == 0)
            {
	    		break;
            }
            else if(timeout > 0)
            {
                //超时等待
                int ret = 0;
                struct timespec outtime;
                memset(&outtime,0,sizeof(struct timespec));

#ifndef WIN32
                //初始化属性的时候需要，需要配置CLOCK_MONOTONIC，否则无法生效
                clock_gettime(CLOCK_MONOTONIC, &outtime);
				timespec_add_ms(&outtime,timeout);
#else
                struct timeval now;
                gettimeofday(&now, NULL);
				outtime.tv_sec = now.tv_sec + timeout / 1000;
				outtime.tv_nsec = (now.tv_usec * 1000) + (timeout % 1000) * 1000 * 1000;
				if(outtime.tv_nsec >= 1000000000)
				{
					outtime.tv_nsec -= 1000000000;
					outtime.tv_sec+=1;
				}
#endif
                ret = pthread_cond_timedwait(&hndl->condWr, &hndl->lock, &outtime);
                if(ret == 0)
                {
                    status = OS_SOK;	//在等待时间内有信号触发
                }else
                {
			        pthread_cond_signal(&hndl->condRd);
                    break;    //等待时间内没有信号触发，直接退出
                }
            }
            else
            {
	  		    status = pthread_cond_wait(&hndl->condWr, &hndl->lock);
            }

		}
	}

	pthread_mutex_unlock(&hndl->lock);

  return status;
}

/*
 * 从队列中获取数据
 * */
int OS_queGet(OS_QueHndl *hndl, int64_t *value, int32_t timeout)
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
	  		if (timeout == 0)
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
              timespec_add_ms(&outtime,timeout);
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
		  	        pthread_cond_signal(&hndl->condWr);
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
uint32_t OS_queGetQueuedCount(OS_QueHndl *hndl)
{
	uint32_t queuedCount = 0;

	pthread_mutex_lock(&hndl->lock);
	queuedCount = hndl->count;
	pthread_mutex_unlock(&hndl->lock);
	return queuedCount;
}

/*
 * 从队列头中获取一个数据，但不读走数据，即读index不往前移位
 * 只是查看队列头中的数据是什么
 * */
int OS_quePeek(OS_QueHndl *hndl, int64_t *value)
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
