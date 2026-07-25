/*
 * @FilePath     : time_keep.h
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-27 11:39:17
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-27 14:34:00
 * @Description  : 秒级以上的，比较耗性能
 */

#ifndef __TIME_KEEP_H__
#define __TIME_KEEP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>

#include "os_sem.h"
#include "os_thr.h"
typedef struct _TIMEKEEP_S TimeKeep_S;

typedef struct TIMEKEEPNEEDPARAM_S
{
    /*计时时间 毫秒, 必须是10ms的倍数*/
    int nTimeMs;
} TimeKeepNeedParam_S;

typedef struct TIMERKEEPEXPARAM_S
{
    void* pParam;
    int (*pUserFun)(void* pParam);
} TimeKeepExParam_S;

struct _TIMEKEEP_S
{
    TimeKeepNeedParam_S stNeedParam;
    TimeKeepExParam_S   stExParam;

    /*0 暂停 1 开始 2 结束*/
    int        nStatus;
    /*已计时时间*/
    int        nTimeMs;
    /*时间信号*/
    OS_SemHndl TimeSem;
    OS_ThrHndl Time_ThrId;
    /*退出标志*/
    int        nExit;
};

/*计时停止*/
int TimeKeep_stop(TimeKeep_S* pHandle);
/*计时结束*/
int TimeKeep_pause(TimeKeep_S* pHandle);
/*计时等待*/
int TImeKeep_wait(TimeKeep_S* pHandle);
/*计时开始*/
int TImeKeep_start(TimeKeep_S* pHandle);
/*重新计时*/
int TImeKeep_resume(TimeKeep_S* pHandle);

int         TimeKeep_init(TimeKeep_S* pHandle);
int         TimeKeep_uninit(TimeKeep_S* pHandle);
TimeKeep_S* TimeKeep_alloc(TimeKeepNeedParam_S stNeedParam);
int         TimeKeep_release(TimeKeep_S* pHandle);

#ifdef __cplusplus
}
#endif

#endif
