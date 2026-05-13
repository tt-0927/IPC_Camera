
#include "stream_ontimer.h"
#include <stdio.h>
#include "dlog.h"
#include <stdlib.h>
#include <string.h>

On_Time_Handle_t* g_ontime_init_Handle = NULL;
int sStream_doSkipFrame(Stream_frameSkipContext_t *frameSkipCtx )
{
    /*if the target framerate has changed, first time case needs to be visited?*/
    if(frameSkipCtx->firstTime == 0)
    {
        frameSkipCtx->outCnt = 0;
        frameSkipCtx->inCnt = 0;

        frameSkipCtx->multipleCnt = frameSkipCtx->inputFrameRate * frameSkipCtx->outputFrameRate;
        frameSkipCtx->firstTime = 1;
    }

    if (frameSkipCtx->inCnt > frameSkipCtx->outCnt)
    {
        frameSkipCtx->outCnt += frameSkipCtx->outputFrameRate;
        /*skip this frame, return true*/
        return 1;
    }

    // out will also be multiple
    if (frameSkipCtx->inCnt == frameSkipCtx->multipleCnt)
    {
        // reset to avoid overflow
        frameSkipCtx->inCnt = frameSkipCtx->outCnt = 0;
    }

    frameSkipCtx->inCnt += frameSkipCtx->inputFrameRate;
    frameSkipCtx->outCnt += frameSkipCtx->outputFrameRate;

    /*display this frame, hence return false*/
    return 0;
}
void  pthread_deal_timer(union sigval v)
{

	List_CurNode_t CurNode = NULL;
	DataNode* pNode = NULL;
	StreamSetTimer_Info_t *Timeinfo = NULL;
	On_Time_Handle_t* ontime_init_Handle = (On_Time_Handle_t*)v.sival_ptr;
	List_Handle_t pList_handle = ontime_init_Handle->pList_Handle;
	pthread_mutex_lock(&(ontime_init_Handle->lock));

	for(CurNode = list_begin(pList_handle); CurNode != list_end(pList_handle); CurNode = list_next(pList_handle, CurNode))
	{
		pNode = (DataNode*)CurNode;
		Timeinfo = (StreamSetTimer_Info_t *)pNode->pData;

		if(sStream_doSkipFrame(&Timeinfo->outputSkip) == 0)
		{
			Timeinfo->funCall(Timeinfo->argv);
		}

	}
	pthread_mutex_unlock(&(ontime_init_Handle->lock));

	return ;
}
On_Time_Handle_t* steam_init_ontimer(int secondCount)
{
	On_Time_Handle_t* ontime_init_Handle = NULL;
	if(g_ontime_init_Handle == NULL)
	{
		List_Handle_t pList_handle = NULL;
		struct sigevent evp;
		int ret = 0;
		struct itimerspec ts;
		if(secondCount > 30 || secondCount < 1)
		{
			//return NULL;
		}

		pList_handle = list_create();
		if(pList_handle == NULL)
		{
			return ontime_init_Handle;
		}

		ontime_init_Handle = (On_Time_Handle_t *)malloc(sizeof(On_Time_Handle_t));
		if(ontime_init_Handle == NULL)
		{
			list_destory( pList_handle);
			return NULL;
		}

		pthread_mutex_init(&(ontime_init_Handle->lock), NULL);
		ontime_init_Handle->pList_Handle = pList_handle;
		ontime_init_Handle->sendCount = secondCount;
		memset(&evp, 0, sizeof(	struct sigevent));

	    evp.sigev_value.sival_ptr = &(ontime_init_Handle->timer);
	    evp.sigev_notify = SIGEV_THREAD;
	    evp.sigev_notify_function = pthread_deal_timer;
	    evp.sigev_value.sival_ptr = (void*)ontime_init_Handle; //作为handle()的参数

	    ret = timer_create(CLOCK_REALTIME , &evp, &(ontime_init_Handle->timer));
		if(ret != 0)
		{
			list_destory( pList_handle);
			free(ontime_init_Handle);
			ontime_init_Handle = NULL;
			return ontime_init_Handle;
		}

		 ts.it_interval.tv_sec = 0;
		 ts.it_interval.tv_nsec = 1000000000 / secondCount;
		 ts.it_value.tv_sec = 1;
		 ts.it_value.tv_nsec = 0;


		 ret = timer_settime( (ontime_init_Handle->timer), 0, &ts, NULL);
		if(ret != 0)
		{
			perror("timer_settime\n");
			list_destory( pList_handle);
			free(ontime_init_Handle);
			ontime_init_Handle = NULL;
		}
		g_ontime_init_Handle = ontime_init_Handle;

	}
	else
	{
		ontime_init_Handle = g_ontime_init_Handle;
	}


	return ontime_init_Handle;
}

//每秒出发多少次，最多30，超过30建议设置一般自行处理
SETTIMER_HANDLE stream_settimer(On_Time_Handle_t* ontime_init_Handle,StreamSetTimer_Info_t* info)
{
	if(ontime_init_Handle == NULL)
	{
		ontime_init_Handle = g_ontime_init_Handle;
	}
	if(ontime_init_Handle == NULL)
	{
		return NULL;
	}

	void* timer_set_handle = NULL;
	if(info->secondCount < 1 || info->secondCount > ontime_init_Handle->sendCount)
	{
		return timer_set_handle;
	}
	pthread_mutex_lock(&(ontime_init_Handle->lock));
	memset(&(info->outputSkip), 0, sizeof(Stream_frameSkipContext_t));
	info->outputSkip.firstTime = 0;
	info->outputSkip.inputFrameRate = ontime_init_Handle->sendCount;
	info->outputSkip.outputFrameRate = info->secondCount;
	timer_set_handle = list_push_back(ontime_init_Handle->pList_Handle, info);
	pthread_mutex_unlock(&(ontime_init_Handle->lock));
	return timer_set_handle;
}

void* stream_killtimer(On_Time_Handle_t* ontime_init_Handle, SETTIMER_HANDLE timerhandle)
{
	if(ontime_init_Handle == NULL)
	{
		ontime_init_Handle = g_ontime_init_Handle;
	}

	DataNode* pNode = timerhandle;
	void * pData = pNode->pData;
	if(pNode == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(ontime_init_Handle->lock));
	list_earse_data(ontime_init_Handle->pList_Handle,pNode->pData);
	pthread_mutex_unlock(&(ontime_init_Handle->lock));
	return pData;
}
