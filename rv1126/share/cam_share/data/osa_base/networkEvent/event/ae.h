
#ifndef _CORE_NETWORK_EVENT_INCLUDE_H__
#define _CORE_NETWORK_EVENT_INCLUDE_H__

#include <time.h>


#ifdef WIN32
#include <time.h>
#include <winsock2.h>
#include <windows.h>
//#include <winsock.h>    //struct timeval
#include "pthread.h"

#else
#include <sys/time.h>
#include <pthread.h>
#endif

#define EV_NONE 0       /* No events registered. */
#define EV_READABLE 1   /* Fire when descriptor is readable. */
#define EV_WRITABLE 2   /* Fire when descriptor is writable. */
#define EV_TIMEOUT 4	/* I/O time out */


#define EV_IO_EVENTS 1	//0001	//网络IO事件
#define EV_TIME_EVENTS 2	//0010	//定时器事件
#define EV_ALL_EVENTS (EV_IO_EVENTS|EV_TIME_EVENTS)	//0011	//所有事件
#define EV_DONT_WAIT 4		//0100	//若没有定时器事件，则超时为0，不需要等待直接返回
#define EV_CALL_AFTER_SLEEP 8	//1000

#define EV_STOP_EVENT -1		//结束事件


typedef struct _EV_EVENT_BASE_ evEventBase;

/* IO event structure */
typedef struct _EV_IO_EVENT_
{
	int fd;							//IO事件的句柄
	struct timeval rInterval;		//读取超时间隔
	struct timeval rTimeout;		//I/O读取超时
    int events;
    void (*evIOProc)(evEventBase *base, int fd, void *user, int events);
    void *user;
}evIOEvent;

/* Time event structure */
typedef struct EV_TIME_EVENT_
{
	struct timeval timeout;
	int (*evtimeProc)(evEventBase *base, void *user);
    void *user;
}evTimeEvent;


/* event list*/
typedef struct EV_TIME_EVENT_LIST_
{
	void* evData;
	int evType;	//事件类型 EV_IO_EVENTS/EV_TIME_EVENTS
	int isDel;	//是否删除该定时器，1-是
    struct EV_TIME_EVENT_LIST_ *prev;
    struct EV_TIME_EVENT_LIST_ *next;
}evEventList;

/*
 * 触发的事件
 * */
typedef struct _EV_TOUCH_EVENT_
{
    int fd;		//IO事件的句柄
    evIOEvent* event;
    int mask;	//触发类型，AE_(READABLE | WRITABLE | TIMEOUT)
}evTouchIOEvent;


/* State of an event based program */
typedef struct _EV_EVENT_BASE_
{
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    struct timeval lastTime;     /* Used to detect system clock skew */
    evIOEvent *IOEvents;	 		/* 注册的I/O事件 */
    evIOEvent *fired;		 		/* 触发的I/O事件 */
    evEventList *timeEventHead;		/* 定时器事件 */
    int stop;
    void *apidata; 	/* This is used for polling API specific data */
    pthread_mutex_t lock;
}evEventBase;


/*
 * 初始化事件驱动句柄
 * @param[in] setsize:最多能监听多少个fd句柄
 * @param[out] return:evEventBase 事件句柄
 * */
evEventBase *evEvent_init(int setsize);

/*
 * 反初始化事件驱动
 * @param[in] eventLoop:事件句柄
 * */
void evEvent_unInit(evEventBase *base);

/*
 * 停止事件驱动
 * */
void evEvent_StopLoop(evEventBase *eventLoop);

/*
 * 创建网络IO事件
 * @param[in] base:事件句柄
 * @param[in] fd:网络IO事件句柄
 * @param[in] events：设置该IO事件的属性，如可读、可写等属性，EV_(READABLE|WRITABLE|TIMEOUT)
 * @param[in] evIOProc：事件触发，注册的回调函数
 * @param[in] user：用户数据
 * */
int evEvent_addIO(evEventBase *base, int fd, int events,struct timeval* timeout,\
		void (*evIOProc)(evEventBase *base, int fd, void *user, int events),\
		void *user);

/*
 * 删除网络IO事件
 * */
int evEvent_delIO(evEventBase *base, int fd, int events);

/*
 * 创建定时器事件
 * @param[in] base:事件句柄
 * @param[in] timeout：超时时间
 * @param[in] evtimeProc：事件触发，注册的回调函数
 * @param[in] user：用户数据
 * @param[out] return :返回定时器句柄
 * */
evTimeEvent* evEvent_addTime(evEventBase *base, struct timeval* timeout,
		int (*evtimeProc)(evEventBase *base, void *user), void *user);

/*
 * 删除定时器事件
 * @param[in] base:事件句柄
 * @param[in] time：创建定时器的时候返回的句柄
 * */
int evEvent_delTime(evEventBase *base, evTimeEvent* time);

/*
 * 事件驱动循环
 * */
int evEvent_process(evEventBase *eventLoop, int flags);

/*
 * 事件驱动loop函数， 调用了evEvent_process(eventLoop,EV_ALL_EVENTS|EV_CALL_AFTER_SLEEP)
 * @param[in] eventLoop:事件句柄
 * 注意：默认需要添加一个定时器（保活定时器），保持事件驱动内部不会永久等待，如果调用该函数的时候，没有添加一个定时器，则会永久阻塞
 * */
void evEvent_loop(evEventBase *eventLoop);

/*
 * 获取当前使用的什么类型的IO多路复用接口
 * epoll,kqueue,evport,select
 * */
char *evEvent_getApiName(void);

#endif	//_CORE_NETWORK_EVENT_INCLUDE_H__
