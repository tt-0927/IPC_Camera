/*
 * @FilePath     : time_keep.c
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-27 11:39:17
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-11 08:51:22
 * @Description  : 秒级以上的，比较耗性能
 */

#include <time_keep.h>
#include "dlog.h"

/*计时停止 返回已经计时时间*/
int TimeKeep_stop(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return 0;
    }
    int nTimeMs      = pHandle->nTimeMs;
    pHandle->nStatus = 2;
    pHandle->nTimeMs = 0;
    return nTimeMs;
}

/*计时暂停 返回已经计时时间*/
int TimeKeep_pause(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return 0;
    }
    pHandle->nStatus = 0;
    return pHandle->nTimeMs;
}

/*计时等待*/
int TImeKeep_wait(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return 0;
    }
    OS_semWait(&(pHandle->TimeSem), -1, NULL);
    return 0;
}

/*计时开始*/
int TImeKeep_start(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return 0;
    }
    pHandle->nStatus = 1;
    return 0;
}

/*重新计时*/
int TImeKeep_resume(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return 0;
    }
    pHandle->nTimeMs = 0;
    pHandle->nStatus = 1;
    return 0;
}

void* time_thr(void* pParam)
{
    TimeKeep_S* pHandle = (TimeKeep_S*)pParam;
    while (!pHandle->nExit)
    {
        usleep(10000);
        if (pHandle->nStatus == 1 && pHandle->nTimeMs < pHandle->stNeedParam.nTimeMs)
        {
            pHandle->nTimeMs += 10;
        }

        if (pHandle->nTimeMs >= pHandle->stNeedParam.nTimeMs && pHandle->stNeedParam.nTimeMs != 0)
        {
            pHandle->nStatus = 2;
            /*发送信号*/
            OS_semSignal(&(pHandle->TimeSem));
            if (pHandle->stExParam.pUserFun && pHandle->nExit == 0)
            {
                // dlog(LOG_INFO,"执行定时器任务函数-----pHandle->nTimeMs=%d-------pHandle->stNeedParam.nTimeMs=%d-------\n",pHandle->nTimeMs,pHandle->stNeedParam.nTimeMs);
                /* 定时器任务函数 */
                pHandle->stExParam.pUserFun(pHandle->stExParam.pParam);
            }
        }
    }
    return NULL;
}

/*反初始化*/
int TimeKeep_init(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return -1;
    }
    OS_semCreate(&(pHandle->TimeSem), 3, 0);
    OS_thrCreate(&(pHandle->Time_ThrId), time_thr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, pHandle);
    return 0;
}

/*初始化*/
int TimeKeep_uninit(TimeKeep_S* pHandle)
{
    if (!pHandle)
    {
        return -1;
    }
    pHandle->nExit = 1;
    OS_thrJoin(&pHandle->Time_ThrId);
    OS_semDelete(&(pHandle->TimeSem));
    return 0;
}

/*分配*/
TimeKeep_S* TimeKeep_alloc(TimeKeepNeedParam_S stNeedParam)
{
    TimeKeep_S* pHandle = (TimeKeep_S*)malloc(sizeof(TimeKeep_S));
    if (NULL == pHandle)
    {
        return NULL;
    }
    
    memset(pHandle, 0, sizeof(TimeKeep_S));
    memcpy(&pHandle->stNeedParam, &stNeedParam, sizeof(TimeKeepNeedParam_S));
    return pHandle;
}

/*释放*/
int TimeKeep_release(TimeKeep_S* pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}
