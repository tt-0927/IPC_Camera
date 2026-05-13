/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AE_H__
#define __AE_H__

#include <time.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0       /* No events registered. */
#define AE_READABLE 1   /* Fire when descriptor is readable. */
#define AE_WRITABLE 2   /* Fire when descriptor is writable. */
#define AE_BARRIER 4    /* With WRITABLE, never fire the event if the
                           READABLE event already fired in the same event
                           loop iteration. Useful when you want to persist
                           things to disk before sending replies, and want
                           to do that in a group fashion. */

#define AE_FILE_EVENTS 1	//0001
#define AE_TIME_EVENTS 2	//0010
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)	//0011
#define AE_DONT_WAIT 4		//0100
#define AE_CALL_AFTER_SLEEP 8	//1000

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure */
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

/* Time event structure */
typedef struct aeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    aeTimeProc *timeProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    struct aeTimeEvent *prev;
    struct aeTimeEvent *next;
} aeTimeEvent;

/* A fired event */
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;

/* State of an event based program */
typedef struct aeEventLoop {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    long long timeEventNextId;
    time_t lastTime;     /* Used to detect system clock skew */
    aeFileEvent *events; /* Registered events */
    aeFiredEvent *fired; /* Fired events */
    aeTimeEvent *timeEventHead;
    int stop;
    void *apidata; /* This is used for polling API specific data */
    aeBeforeSleepProc *beforesleep;
    aeBeforeSleepProc *aftersleep;
} aeEventLoop;

/* Prototypes */

/*
 * 创建事件驱动句柄
 * @param[in] setsize:最多能监听多少个fd句柄
 * @param[out] return:aeEventLoop 事件句柄
 * */
aeEventLoop *aeCreateEventLoop(int setsize);

/*
 * 删除事件驱动
 * @param[in] eventLoop:事件句柄
 * */
void aeDeleteEventLoop(aeEventLoop *eventLoop);

/*
 * 停止事件驱动
 * */
void aeStop(aeEventLoop *eventLoop);

/*
 * 创建IO事件
 * @param[in] eventLoop:事件句柄
 * @param[in] fd:IO事件句柄
 * @param[in] mask：设置该IO事件的属性，如可读、可写等属性，AE_(READABLE|WRITABLE|BARRIER)
 * @param[in] proc：事件触发，注册的回调函数
 * @param[in] clientData：用户数据
 * */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);

/*
 * 删除文件事件
 * */
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);

/*
 * 获取文件事件属性
 * */
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);

/*
 * 创建定时器事件
 * @param[in] eventLoop:事件句柄
 * @param[in] milliseconds：超时时间，ms
 * @param[in] proc：事件触发，注册的回调函数
 * @param[in] clientData：用户数据
 * @param[in] finalizerProc：超时结束后，会调用该回调函数
 * @param[out] return :返回定时器句柄
 * */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);

/*
 * 删除定时器事件
 * @param[in] eventLoop:事件句柄
 * @param[in] id：创建定时器的时候返回的句柄id
 * */
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);

/*
 * 事件驱动循环
 * */
int aeProcessEvents(aeEventLoop *eventLoop, int flags);

/*
 * 调用epoll_wait返回可用的文件描述符
 * */
int aeWait(int fd, int mask, long long milliseconds);

/*
 * 事件驱动main函数， 调用了aeProcessEvents(eventLoop,AE_ALL_EVENTS|AE_CALL_AFTER_SLEEP)
 * @param[in] eventLoop:事件句柄
 * 注意：默认需要添加一个定时器（保活定时器），保持事件驱动内部不会永久等待，如果调用该函数的时候，没有添加一个定时器，则会永久阻塞
 * */
void aeMain(aeEventLoop *eventLoop);

char *aeGetApiName(void);

/*
 * 设置事件触发的时候，会调用设置的回调函数
 * @param[in] eventLoop:事件句柄
 * @param[in] beforesleep:事件触发执行的初始化函数
 * */
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);

/*
 * 设置事件结束后，会调用设置的回调函数
 * @param[in] eventLoop:事件句柄
 * @param[in] beforesleep:事件结束后执行的回调函数
 * */
void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep);

/*
 * 获取该事件驱动循环监听的最大句柄数
 * @param[in] eventLoop:事件句柄
 * */
int aeGetSetSize(aeEventLoop *eventLoop);

/*
 * 设置该事件驱动监听的最大句柄数
 * @param[in] eventLoop:事件句柄
 * @param[in] setsize:监听的最大句柄数
 * */
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);




#endif
