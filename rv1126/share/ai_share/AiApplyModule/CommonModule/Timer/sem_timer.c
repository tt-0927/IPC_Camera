/*
 * @FilePath     : sem_timer.c
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-25 16:00:25
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-10-30 14:40:50
 * @Description  : 毫秒级别的定时器
 */
#include "sem_timer.h"

/*定时信号发送*/
static int timer_signal(void* argv)
{
    Timer_S* pHandle = (Timer_S*)argv;
    OS_semSignal(&(pHandle->timeOutSem));
    return 0;
}

/*定时器帧率改变*/
int Timer_set_Rate(Timer_S* pHandle, int nRate)
{
    stream_change_timer(pHandle->timerHandle, nRate);
    return 0;
}

/*用户任务处理*/
static void* timer_taskDeal(void* pParam)
{
    Timer_S* pHandle = (Timer_S*)pParam;
    while (!pHandle->nExit)
    {
        OS_semWait(&pHandle->timeOutSem, -1, NULL);
        if (pHandle->nExit == 0)
        {
            /*处理任务*/
            pHandle->stNeedParam.pTaskFun(pHandle->stNeedParam.pParam);
        }
    }
    return NULL;
}

/*定时器初始化*/
int Timer_init(Timer_S* pHandle)
{
    StreamSetTimer_Info_t* info_timer = NULL;
    int                    nRet       = OS_semCreate(&(pHandle->timeOutSem), pHandle->stExParam.nSigCount, 0);
    info_timer                        = malloc(sizeof(StreamSetTimer_Info_t));
    if (info_timer)
    {
        info_timer->argv        = pHandle;
        info_timer->secondCount = pHandle->stNeedParam.nFrameRate;    // 帧率
        info_timer->funCall     = timer_signal;                       // 回调线程
        pHandle->timerHandle    = stream_settimer(NULL, info_timer);
    }
    OS_thrCreate(&pHandle->stTasdDealId, timer_taskDeal, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, (void*)pHandle);
    return 0;
}

/*定时器反初始化*/
int Timer_uninit(Timer_S* pHandle)
{
    if (pHandle)
    {
        if (pHandle->timerHandle)
        {
            stream_killtimer(NULL, pHandle->timerHandle);
        }
        pHandle->nExit = 1;
        OS_semSignal(&(pHandle->timeOutSem));
        OS_thrJoin(&pHandle->stTasdDealId);
        OS_semDelete(&(pHandle->timeOutSem));
    }
    return -1;
}

/*分配定时器句柄*/
Timer_S* Timer_alloc(TimerNeedParam_S stNeedParam)
{
    Timer_S* pHandle = NULL;
    pHandle          = (Timer_S*)malloc(sizeof(Timer_S));
    memset(pHandle, 0, sizeof(Timer_S));
    pHandle->stNeedParam.nFrameRate = stNeedParam.nFrameRate;
    pHandle->stNeedParam.pTaskFun   = stNeedParam.pTaskFun;
    pHandle->stNeedParam.pParam     = stNeedParam.pParam;
    pHandle->stExParam.nSigCount    = 3;
    pHandle->stExParam.release_user = NULL;
    return pHandle;
}

/*释放定时器句柄*/
int Timer_release(Timer_S* pHandle)
{
    if (!pHandle)
    {
        return -1;
    }
    if (pHandle->stExParam.release_user && pHandle->stNeedParam.pParam)
    {
        pHandle->stExParam.release_user(pHandle->stNeedParam.pParam);
        pHandle->stNeedParam.pParam = NULL;
    }
    free(pHandle);
    pHandle = NULL;
    return 0;
}
