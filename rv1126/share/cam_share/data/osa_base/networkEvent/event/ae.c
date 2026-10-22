
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <time.h>
#endif

#include "ae.h"
#include "config.h"

#ifdef HAVE_EVPORT
#include "ae_evport.c"
#else
    #ifdef HAVE_EPOLL
    #include "ae_epoll.c"
    #else
        #ifdef HAVE_KQUEUE
        #include "ae_kqueue.c"
        #else
			#ifdef WIN32
			#include "win32select.c"
			#else
        	#include "ae_select.c"
			#endif
        #endif
    #endif
#endif


#define ev_timeradd(tvp, uvp, vvp)					\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#define	ev_timersub(tvp, uvp, vvp)					\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)
#define	ev_timercmpms(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :				\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define	ev_timercmps(tvp, uvp, cmp)					\
	(((tvp)->tv_sec cmp (uvp)->tv_sec) ?				\
	 1 : 0 )
#define	ev_timerclear(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0



static void evGetTime(struct timeval* tv)
{
    gettimeofday(tv, NULL);
}

static void evAddMillisecondsToNow(struct timeval* interval, struct timeval* time)
{
    struct timeval now;
    evGetTime(&now);
    ev_timeradd(&now,interval,time);
}



static int evEvent_addTimeList(evEventBase *base,void* data,int evType)
{
    //加入链表
    evEventList* listNode = (evEventList*)malloc(sizeof(evEventList));
    if (listNode == NULL)
    {
    	return -1;
    }
    memset(listNode,0,sizeof(evEventList));
    listNode->evData = data;
    listNode->evType = evType;
    listNode->prev = NULL;

    //lock
    pthread_mutex_lock(&base->lock);
    listNode->next = base->timeEventHead;
    if (listNode->next)
    {
    	listNode->next->prev = listNode;
    }
    base->timeEventHead = listNode;
    //unlock
    pthread_mutex_unlock(&base->lock);

	return 0;
}


static int evEvent_delTimeList(evEventBase *base, void* data)
{
	pthread_mutex_lock(&base->lock);
	evEventList* listHead = base->timeEventHead;
	evEventList* node = listHead;
	while(node)
	{
		if(node->evData == data)
		{
			node->isDel = 1;
			break;
		}
		node = node->next;
	}
	pthread_mutex_unlock(&base->lock);

    return 0;
}

///////////////////////////////////////////////

evEventBase *evEvent_init(int setsize)
{
	evEventBase *eventLoop = NULL;
    int i = 0;
    if ((eventLoop = malloc(sizeof(*eventLoop))) == NULL)
    {
    	printf("malloc error!!!\n");
    	goto err;
    }
	eventLoop->IOEvents = malloc(sizeof(evIOEvent)*setsize);
	eventLoop->fired = malloc(sizeof(evIOEvent)*setsize);
	if (eventLoop->IOEvents == NULL || eventLoop->fired == NULL)
	{
		goto err;
	}

    eventLoop->setsize = setsize;
    evGetTime(&(eventLoop->lastTime));
    eventLoop->timeEventHead = NULL;
    eventLoop->stop = 0;
    eventLoop->maxfd = -1;
    if (aeApiCreate(eventLoop) == -1)
    {
    	goto err;
    }

    for (i = 0; i < setsize; i++)
    {
        eventLoop->IOEvents[i].events = EV_NONE;
    }
    pthread_mutex_init(&eventLoop->lock, NULL);

    return eventLoop;

err:
    if (eventLoop)
    {
        free(eventLoop->fired);
        free(eventLoop);
    }
    return NULL;
}

void evEvent_unInit(evEventBase *base)
{
    aeApiFree(base);
    free(base->IOEvents);
    free(base->fired);
    free(base);
    pthread_mutex_destroy(&base->lock);
}

void evEvent_StopLoop(evEventBase *eventLoop)
{
    eventLoop->stop = 1;
}


int evEvent_addIO(evEventBase *base, int fd, int events,struct timeval* timeout,\
		void (*evIOProc)(evEventBase *base, int fd, void *user, int events),\
		void *user)
{
	if((base == NULL) || (evIOProc == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
    if (fd >= base->setsize)
    {
        printf("fd[%d] > setSize[%d] error!!\n",fd,base->setsize);
        return -1;
    }

    evIOEvent *fe = &base->IOEvents[fd];
    if (aeApiAddEvent(base, fd, events) == -1)
    {
    	printf("aeApiAddEvent error events[%d] events[%d]!\n",fe->events,events);
        return -1;
    }

    fe->events |= events;
    fe->evIOProc = evIOProc;
    fe->user = user;

    //若有读取超时，则需要加入超时队列
    if( (events & EV_READABLE) )
    {
    	if((events & EV_TIMEOUT) && timeout){
			fe->rInterval = *timeout;
			evAddMillisecondsToNow(&(fe->rInterval),&(fe->rTimeout));//update io event time
	    	evEvent_addTimeList(base,(void*)fe,EV_IO_EVENTS);//add timeout list
    	}
    }

    if (fd > base->maxfd){
    	base->maxfd = fd;
    }

    return 0;
}

int evEvent_delIO(evEventBase *base, int fd, int events)
{
	if(base == NULL)
	{
		printf("this arguemnt is null!!!\n");
		return -1;
	}
    if (fd >= base->setsize)
    {
    	return -1;
    }
    evIOEvent *fe = &base->IOEvents[fd];
    if (fe->events == EV_NONE)
    {
    	return -1;
    }
    int evMask = events;

    //删除定时器
    if((evMask & EV_READABLE) || (evMask & EV_TIMEOUT))
    {
    	evMask |= EV_TIMEOUT;//清空超时标志
    	evEvent_delTimeList(base,(void*)fe);
    	ev_timerclear(&fe->rInterval);
    	ev_timerclear(&fe->rTimeout);
    }
    aeApiDelEvent(base, fd, evMask);
    fe->events = fe->events & (~evMask);
    if (fd == base->maxfd && fe->events == EV_NONE)
    {
        /* Update the max fd */
        int j = 0;
        for (j = base->maxfd-1; j >= 0; j--)
        {
            if (base->IOEvents[j].events != EV_NONE)
            {
            	break;
            }
        }
        base->maxfd = j;
    }

    return 0;
}


evTimeEvent* evEvent_addTime(evEventBase *base, struct timeval* timeout,
		int (*evtimeProc)(evEventBase *base, void *user), void *user)
{
	if((base == NULL) || (evtimeProc == NULL))
	{
		printf("this argument is null!!\n");
		return NULL;
	}
	evTimeEvent * te = (evTimeEvent*)malloc(sizeof(evTimeEvent));
    if (te == NULL)
    {
    	return NULL;
    }
    memset(te,0,sizeof(evTimeEvent));

    /*时间是距离当前增加多长事件*/
    evAddMillisecondsToNow(timeout,&(te->timeout));
    te->evtimeProc = evtimeProc;
    te->user = user;

    // printf("add time list!!!!\n");
    //加入链表
    if(evEvent_addTimeList(base,te,EV_TIME_EVENTS) < 0)
    {
    	printf("add time list error!!!\n");
    	free(te);te = NULL;
    	return NULL;
    }

    return te;
}

int evEvent_delTime(evEventBase *base, evTimeEvent* time)
{
	if(base == NULL || (time == NULL))
	{
		printf("argument is NULL!!!\n");
		return -1;
	}

	if(evEvent_delTimeList(base,(void*)time) < 0)
	{
		printf("del time event!!!\n");
		return -1;
	}
    return 0;
}


static int evNextNearestTimer(evEventBase *eventLoop,struct timeval* time)
{
	evTimeEvent* timeTmp = NULL;
	evIOEvent* ioTime = NULL;
	struct timeval Timeout;
	struct timeval TimeMin;
	int getTime = 0;

	pthread_mutex_lock(&eventLoop->lock);
	evEventList* te = eventLoop->timeEventHead;

    while(te)
    {
    	if(te->evType == EV_IO_EVENTS)
    	{
    		ioTime = (evIOEvent*)te->evData;
    		Timeout = ioTime->rTimeout;
    	}else
    	{
    		timeTmp = (evTimeEvent*)te->evData;
    		Timeout = timeTmp->timeout;
    	}

        if ((!getTime) || (ev_timercmpms(&(Timeout),&(TimeMin),<=)))
        {
        	getTime = 1;
        	TimeMin = Timeout;
        }
        te = te->next;
    }
	pthread_mutex_unlock(&eventLoop->lock);

	if(getTime)
	{
		memcpy(time,&TimeMin,sizeof(struct timeval));
	}else
	{
		ev_timerclear(time);
		return -1;
	}
    return 0;
}


/* Process time events */
static int evEvent_processTime(evEventBase *base)
{
    int processed = 0;
	evEventList *timeList = NULL;
	evTimeEvent* te = NULL;
	evIOEvent* io = NULL;
	struct timeval new;
    struct timeval now;
    evGetTime(&now);

    /*
     * 防止时间变动
     * 若时间发生倒退，则全部定时器触发，早触发，好过延迟好久触发（或不触发）
     * 若时间发生超前，则不需理会
     * */
    /* 2022年8月22号，改为只检测秒，忽略微秒
     * 防止同步时间ms秒时间差，导致不断触发事件（主要是重连） 
     */
    if( ev_timercmps(&now,&(base->lastTime),<) )
    {
    	timeList = base->timeEventHead;
        while(timeList)
        {
        	if(timeList->evType == EV_IO_EVENTS){
        		io = (evIOEvent*)timeList->evData;
				ev_timerclear(&(io->rTimeout));
        	}else{
				te = (evTimeEvent*)timeList->evData;
				ev_timerclear(&(te->timeout));
        	}
        	timeList = timeList->next;
        }
    }
    base->lastTime = now;

    pthread_mutex_lock(&base->lock);
    timeList = base->timeEventHead;
    pthread_mutex_unlock(&base->lock);

    int retval = 0;
    evEventList* next = NULL;
    while(timeList)
    {
    	/* del time */
    	if(timeList->isDel == 1)
    	{
    		pthread_mutex_lock(&base->lock);
    		next = timeList->next;
            if (timeList->prev){
            	timeList->prev->next = timeList->next;
            }else{
            	base->timeEventHead = timeList->next;
            }
            if (timeList->next){
            	timeList->next->prev = timeList->prev;
            }
			if(timeList->evType == EV_TIME_EVENTS){
				if(timeList->evData){//释放数据
					free(timeList->evData);
					timeList->evData = NULL;
				}
			}
			free(timeList);
			timeList = next;
			pthread_mutex_unlock(&base->lock);
			continue;
    	}

    	evGetTime(&now);
    	if(timeList->evType == EV_IO_EVENTS)
    	{
			io = (evIOEvent*)timeList->evData;
			if( ev_timercmpms(&now,&(io->rTimeout),>=) )
			{
				if(io->evIOProc){
					io->evIOProc(base,io->fd,io->user,EV_TIMEOUT);
					evAddMillisecondsToNow(&(io->rInterval),&(io->rTimeout));
				}
				processed++;
			}
    	}else
    	{
			te = (evTimeEvent*)timeList->evData;
			if( ev_timercmpms(&now,&(te->timeout),>=) )
			{
				if(te->evtimeProc){
					retval = te->evtimeProc(base,te->user);
					if (retval != EV_STOP_EVENT){
						new.tv_sec = retval/1000;
						new.tv_usec = (retval%1000)*1000;
						evAddMillisecondsToNow(&(new),&(te->timeout));
					}
				}
				processed++;
			}
    	}
        timeList = timeList->next;
    }
    //printf("00000000time index[%d]\n",processed);

    return processed;
}


int evEvent_process(evEventBase *eventLoop, int flags)
{
    int processed = 0, numevents = 0;

    /* Nothing to do? return ASAP
       AE_TIME_EVENT:所有类型事件都会被执行
       AE_FILE_EVENTS:所以文件事件都会被执行
       AE_TIME_EVENTS:所有时间事件都会被执行
       */
    if (!(flags & EV_TIME_EVENTS) && !(flags & EV_IO_EVENTS))
    {
    	return 0;
    }

    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
    if (eventLoop->maxfd != -1 ||
        ((flags & EV_TIME_EVENTS) && !(flags & EV_DONT_WAIT)))
    {
        int j = 0;
        int getTimeout = 0;
        struct timeval tv,*tvp = NULL;
        if ((flags & EV_TIME_EVENTS) && !(flags & EV_DONT_WAIT))
        {
        	//找到最近的时间事件
        	getTimeout = evNextNearestTimer(eventLoop,&tv);
        }

        if (getTimeout >= 0)
        {
    		struct timeval now;
    		evGetTime(&now);
    		if (ev_timercmpms(&(tv), &now, <=)){
    			ev_timerclear(&(tv));
    		}else{
            	ev_timersub(&(tv), &now, &tv);
    		}
        	tvp = &tv;
        } else
        {
            /* EV_DONT_WAIT 不需要等待，直接返回 */
            if (flags & EV_DONT_WAIT){
            	ev_timerclear(&(tv));
                tvp = &tv;
            } else{
            	printf(" wait forever!!!\n");
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }

        numevents = aeApiPoll(eventLoop, tvp);

        //执行事件
        for (j = 0; j < numevents; j++)
        {
        	evIOEvent *fe = &eventLoop->IOEvents[eventLoop->fired[j].fd];
            int mask = eventLoop->fired[j].events;
            int fd = eventLoop->fired[j].fd;
            if(fe->evIOProc)
            {
            	fe->evIOProc(eventLoop,fd,fe->user,mask);
            	if((fe->events & EV_READABLE) && (fe->events & EV_TIMEOUT)){
            		//update io event time
            		evAddMillisecondsToNow(&(fe->rInterval),&(fe->rTimeout));
            	}
            }
            processed++;
        }
    }

    /* Check time events */
    if (flags & EV_TIME_EVENTS)
    {
        processed += evEvent_processTime(eventLoop);
    }

    return processed; /* return the number of processed IO/time events */
}


void evEvent_loop(evEventBase *eventLoop)
{
    eventLoop->stop = 0;
    while (!eventLoop->stop)
    {
    	evEvent_process(eventLoop, EV_ALL_EVENTS|EV_CALL_AFTER_SLEEP);
    }
}


char *evEvent_getApiName(void)
{
    return aeApiName();
}







