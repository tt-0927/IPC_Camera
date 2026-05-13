
/******************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <errno.h>

#ifdef WIN32
#include <time.h>
#include <winsock.h>    //struct timeval
#include "pthread.h"
#else
#include <pthread.h>			/*for POSIX calls*/
#include <sys/time.h>
#endif


#include "os_event.h"


typedef struct {
    int bSignaled;
	unsigned long eFlags;
    pthread_mutex_t mutex;
    pthread_cond_t  condition;
} OS_THREAD_EVENT;


/* ========================================================================== */
/**
* @fn OSA_EventCreate function
*
*
*/
/* ========================================================================== */
int OS_EventCreate(OS_PTR *pEvents)
{
    int bReturnStatus = OS_EFAIL;
    OS_THREAD_EVENT *plEvent = NULL;

    plEvent = (OS_THREAD_EVENT *)OS_memAlloc(sizeof(OS_THREAD_EVENT));

	if(0 == plEvent) {
		bReturnStatus = OS_EFAIL;
		goto EXIT;
	}
    plEvent->bSignaled = 0;
	plEvent->eFlags = 0;

	if(0 != pthread_mutex_init(&(plEvent->mutex), NULL)){
		/*OSA_ERROR("Event Create:Mutex Init failed !");*/
		goto EXIT;  /*bReturnStatus = TIMM_OSAL_ERR_UNKNOWN*/
	}

	if(0 != pthread_cond_init(&(plEvent->condition), NULL)){
		/*TIMM_OSAL_Error("Event Create:Conditional Variable  Init failed !");*/
		pthread_mutex_destroy(&(plEvent->mutex));
		/*TIMM_OSAL_Free(plEvent);*/
	}
	else {
    	*pEvents = (OS_PTR)plEvent;
 		bReturnStatus = OS_SOK;
	}
EXIT:
    if ((OS_SOK != bReturnStatus) && (0 != plEvent)) {
        OS_memFree(plEvent);
	}
	return bReturnStatus;
}


/* ========================================================================== */
/**
* @fn TIMM_OSAL_EventDelete function
*
*
*/
/* ========================================================================== */
int OS_EventDelete(OS_PTR pEvents)
{
    int bReturnStatus = OS_SOK;
    OS_THREAD_EVENT *plEvent = (OS_THREAD_EVENT *)pEvents;

	if(0 == plEvent){
		bReturnStatus = OS_EFAIL;
		goto EXIT;
	}

	if(0 != pthread_mutex_lock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Delete: Mutex Lock failed !");*/
		bReturnStatus = OS_EFAIL;
	}
    if(0 != pthread_cond_destroy(&(plEvent->condition))){
		/*OSAL_ERROR("Event Delete: Conditional Variable Destroy failed !");*/
		bReturnStatus = OS_EFAIL;
	}

	if(0 != pthread_mutex_unlock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Delete: Mutex Unlock failed !");*/
		bReturnStatus = OS_EFAIL;
	}

	if(0 != pthread_mutex_destroy(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Delete: Mutex Destory failed !");*/
		bReturnStatus = OS_EFAIL;
	}

    OS_memFree(plEvent);
EXIT:
	return bReturnStatus;
}


/* ========================================================================== */
/**
* @fn OSA_EventSet function
*
*
*/
/* ========================================================================== */
int OS_EventSet(OS_PTR pEvents, unsigned long uEventFlags, OS_EVENT_OPERATION eOperation)
{
    int bReturnStatus = OS_EFAIL;
    OS_THREAD_EVENT *plEvent = (OS_THREAD_EVENT *)pEvents;

	if(0 == plEvent){
		bReturnStatus = OS_EFAIL;
		goto EXIT;
	}

	if(0 != pthread_mutex_lock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Set: Mutex Lock failed !");*/
		bReturnStatus = OS_EFAIL;
		goto EXIT;
	}

    switch (eOperation) {
    case OS_EVENT_AND:
        plEvent->eFlags = plEvent->eFlags & uEventFlags;
		break;
    case OS_EVENT_OR:
        plEvent->eFlags = plEvent->eFlags | uEventFlags;
		break;
    default:
    	/*OSAL_ERROR("Event Set: Bad eOperation !");*/
        bReturnStatus = OS_EFAIL;
        pthread_mutex_unlock(&plEvent->mutex);
        goto EXIT;
    }

    plEvent->bSignaled = TRUE;

	if(0 != pthread_cond_signal(&plEvent->condition)) {
		/*OSAL_ERROR("Event Set: Condition Variable Signal failed !");*/
		bReturnStatus = OS_EFAIL;
		pthread_mutex_unlock(&plEvent->mutex);
		goto EXIT;
	}

	if(0 != pthread_mutex_unlock(&plEvent->mutex)){
		/*OSAL_ERROR("Event Set: Mutex Unlock failed !");*/
		bReturnStatus = OS_EFAIL;
	}
	else
		bReturnStatus = OS_SOK;

EXIT:
	return bReturnStatus;


}

/* ========================================================================== */
/**
* @fn OSA_EventRetrieve function
*
*Spurious  wakeups  from  the  pthread_cond_timedwait() or pthread_cond_wait() functions  may  occur.
*
*A representative sequence for using condition variables is shown below
*
*Thread A (Retrieve Events)							|Thread B (Set Events)
*------------------------------------------------------------------------------------------------------------
*1) Do work up to the point where a certain condition 	|1)Do work
*  must occur (such as "count" must reach a specified 	|2)Lock associated mutex
*  value)											|3)Change the value of the global variable
*2) Lock associated mutex and check value of a global 	|  that Thread-A is waiting upon.
*  variable										|4)Check value of the global Thread-A wait
*3) Call pthread_cond_wait() to perform a blocking wait 	|  variable. If it fulfills the desired
*  for signal from Thread-B. Note that a call to 			|  condition, signal Thread-A.
*  pthread_cond_wait() automatically and atomically 		|5)Unlock mutex.
*  unlocks the associated mutex variable so that it can 	|6)Continue 
*  be used by Thread-B.							|
*4) When signalled, wake up. Mutex is automatically and 	|
*  atomically locked.								|
*5) Explicitly unlock mutex							|
*6) Continue										|	
*
* ========================================================================== */
int OS_EventRetrieve(OS_PTR pEvents,
                      unsigned long uRequestedEvents,
                      OS_EVENT_OPERATION eOperation,
                      unsigned long *pRetrievedEvents,
                      unsigned long uTimeOutMsec)
{
	int bReturnStatus = OS_EFAIL;
	struct timespec timeout;
	struct timeval now;
	unsigned timeout_us;
	unsigned isolatedFlags;
	int status = -1;
	int and_operation;
	OS_THREAD_EVENT *plEvent = (OS_THREAD_EVENT *)pEvents;

	if(0 == plEvent)
	{
		bReturnStatus = OS_EFAIL;
		goto EXIT;
	}

	/* Lock the mutex for access to the eFlags global variable*/
	if(0 != pthread_mutex_lock(&(plEvent->mutex)))
	{
		/*OSL_ERROR("Event Retrieve: Mutex Lock failed !");*/
		bReturnStatus = OS_EFAIL;
		goto EXIT;
	}


	/*
	 * 	a.当有两个事件以上是&关系的，要两个事件都触发了，才能真正触发后续事件；
	 * 	b.当只有一个事件在等待，则参数eOperation是 AND或OR的结果都一样；
	 * */

	/*Check the eOperation and put it in a variable*/
	and_operation = ((OS_EVENT_AND == eOperation) || (OS_EVENT_AND_CONSUME ==  eOperation));

	/* Isolate the flags. The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation*/
	isolatedFlags = plEvent->eFlags & uRequestedEvents;

	/*Check if it is the AND operation. If yes then, all the flags must match*/
	/*先判断&操作下事件是否触发*/
	if(and_operation)
	{
		isolatedFlags =  (isolatedFlags == uRequestedEvents);
	}

	/*&操作下的事件已触发*/
	if(isolatedFlags)
	{

		/*We have got required combination of the eFlags bits and will return it back*/
		/*返回已触发后的事件标志位*/
		*pRetrievedEvents = plEvent->eFlags;
		bReturnStatus = OS_SOK;

	}else	/*&操作下的事件未触发，根据参数uTimeOutMsec是否等待事件*/
	{

		/*Required combination of bits is not yet available*/
		if ( OS_NO_SUSPEND == uTimeOutMsec)	//不等待事件
		{
			*pRetrievedEvents = 0;
			bReturnStatus = OS_SOK;

		}else if (OS_SUSPEND == uTimeOutMsec)	//阻塞等待事件
		{

			/*	Wait till we get the required combination of bits. We we get the required
			*	bits then we go out of the while loop
			*	如果是需要等待两个以上的事件都已经发生了，则是事件之间是&关系，则检测一个事件发生后，再次轮询等待事件发生
			*/
			while(!isolatedFlags)
			{

				/*Wait on the conditional variable for another thread to set the eFlags and signal*/
				pthread_cond_wait(&(plEvent->condition), &(plEvent->mutex));

				/* eFlags set by some thread. Now, isolate the flags.
				 * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
				 */
				/*条件变量触发后，再次获取事件标志位*/
				isolatedFlags = plEvent->eFlags & uRequestedEvents;

				/*Check if it is the AND operation. If yes then, all the flags must match*/
				/* 如果需要等待两个事件以上都已经发生的情况下，进行两个以上事件&判断，
				 * 即，等待事件C需要事件A和事件B都发生的情况下，事件C才真正的触发，
				 * 如果，当前只有A事件发生，则再次循环等待其余事件的发生。
				 */
				if(and_operation)
				{
					isolatedFlags =  (isolatedFlags == uRequestedEvents);
				}
			}

			/* Obtained the requested combination of bits on eFlags*/
			/*返回触发后的事件标志位*/
			*pRetrievedEvents = plEvent->eFlags;
			bReturnStatus = OS_SOK;

		}else		//给定超时事件等待事件
		{

			/* Calculate uTimeOutMsec in terms of the absolute time. uTimeOutMsec is in milliseconds*/
			gettimeofday(&now, NULL);
			timeout_us = now.tv_usec + 1000 * uTimeOutMsec;
			timeout.tv_sec = now.tv_sec + timeout_us / 1000000;
			timeout.tv_nsec = (timeout_us % 1000000) * 1000;

			while(!isolatedFlags)
			{
				/* Wait till uTimeOutMsec for a thread to signal on the conditional variable*/
				status = pthread_cond_timedwait(&(plEvent->condition), &(plEvent->mutex), &timeout);

				/*Timedout or error and returned without being signalled*/
				if (0 != status)
				{
					if(ETIMEDOUT == status)
           	    		bReturnStatus = OS_SOK;
       	    		*pRetrievedEvents = 0;
            	    break;
	            }

				/* eFlags set by some thread. Now, isolate the flags.
				 * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
				 */
				isolatedFlags = plEvent->eFlags & uRequestedEvents;

				/*Check if it is the AND operation. If yes then, all the flags must match*/
				if(and_operation)
				{
					isolatedFlags =  (isolatedFlags == uRequestedEvents);
				}

			}
		}
	}

	/*If we have got the required combination of bits, we will have to reset the eFlags if CONSUME is mentioned
	*in the eOperations
	*/
	/*事件已触发的情况下，需要根据eOperation参数是否是OS_EVENT_AND_CONSUME或OS_EVENT_OR_CONSUME来对整个事件标志位eFlag清空*/
	if (isolatedFlags && ((eOperation == OS_EVENT_AND_CONSUME) || (eOperation == OS_EVENT_OR_CONSUME)))
	{
		plEvent->eFlags =  0;
	}

	/*Manually unlock the mutex*/
	if(0 != pthread_mutex_unlock(&(plEvent->mutex)))
	{
		/*OSAL_ERROR("Event Retrieve: Mutex Unlock failed !");*/
		bReturnStatus = OS_EFAIL;
	}

EXIT:
    return bReturnStatus;

}








