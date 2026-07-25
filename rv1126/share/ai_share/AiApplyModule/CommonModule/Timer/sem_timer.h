/*
 * @FilePath     : sem_timer.h
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-25 16:00:25
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-27 11:41:50
 * @Description  : 毫秒级别的定时器
 */
#ifndef __SEM_TIMER_H__
#define __SEM_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "os_sem.h"
#include "os_thr.h"
#include "stream_ontimer.h"

typedef struct _TIMER_S Timer_S;

typedef struct _TIMERNEEDPARAM_S
{
    /*定时帧率*/
    int nFrameRate;
    /*定时任务*/
    int (*pTaskFun)(void* pParam);
    /*任务参数*/
    void* pParam;
} TimerNeedParam_S;

typedef struct _TIMEREXPARAM_
{

    /*定时信号次数  处理任务超时时，可积累的超时次数 默认3*/
    int nSigCount;
    /*退出定时器释放任务参数  为NULL是不释放 默认NULL*/
    int (*release_user)(Timer_S* pHandle);
} TimerExParam_S;

struct _TIMER_S
{
    /*必须参数*/
    TimerNeedParam_S stNeedParam;
    /*功能参数*/
    TimerExParam_S   stExParam;

    /*处理任务线程ID*/
    OS_ThrHndl      stTasdDealId;
    /*退出*/
    int             nExit;
    /*定时器句柄*/
    SETTIMER_HANDLE timerHandle;
    /*定时发送信号*/
    OS_SemHndl      timeOutSem;
};

/*定时器帧率改变*/
int      Timer_set_Rate(Timer_S* pHandle, int nRate);
/*定时器初始化*/
int      Timer_init(Timer_S* pHandle);
/*定时器反初始化*/
int      Timer_uninit(Timer_S* pHandle);
/*分配定时器句柄*/
Timer_S* Timer_alloc(TimerNeedParam_S stNeedParam);
/*释放定时器句柄*/
int      Timer_release(Timer_S* pHandle);

#ifdef __cplusplus
}
#endif

#endif
