

#ifndef _OS_EVENT_H_
#define _OS_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os.h"

/* ===========================================================================*/
/**
 *  OSA_EVENT_OPERATION - Different operations possible while retrieving
 *                        or setting events.
 *
 *  @ param OSA_EVENT_AND          :The event will be retreived if all 
 *                                        the events specified in the mask
 *                                        are active. The events will not be 
 *                                        consumed. 
 *                                        While setting events, the flag 
 *                                        specified will be ANDed with the
 *                                        current flag.
 *                                        a.设置事件标志位时，用&操作设置句柄中的事件标志位；
 *                                        b.等待事件时，用于两个事件以上都要发生的情况下，才真正的触发事件；
 *                                        	如果是只等待一个事件的发生，则使用AND和OR都一样的；
 *                                        	如：事件C是读事件，事件A和B都是写事件，事件C需要A和B都往缓冲区写内容了，
 *                                        	        事件C才去读事件，即事件A和事件B是&的关系，全部真才真
 *
 *  @ param OSA_EVENT_AND_CONSUME  :The event will be retreived if all 
 *                                        the events specified in the mask
 *                                        are active. All events in the 
 *                                        mask will be consumed. 
 *                                        Not valid while setting events.
 *
 *  @ param OSA_EVENT_OR           :The event will be retreived if any 
 *                                        the events specified in the mask
 *                                        are active. The events will not be 
 *                                        consumed.
 *                                        While setting events, the flag 
 *                                        specified will be ORed with the
 *                                        current flag.
 *
 *  @ param OSA_EVENT_OR_CONSUME   :The event will be retreived if any 
 *                                        the events specified in the mask
 *                                        are active. All active events in the 
 *                                        mask will be consumed. 
 *                                        Not valid while setting events.
 */
/* ===========================================================================*/
typedef enum OS_EVENT_OPERATION
{
   OS_EVENT_AND,			/*&操作下设置事件标志位*/
   OS_EVENT_AND_CONSUME,	/*用于清空事件标志位*/
   OS_EVENT_OR,			/*|操作下设置事件标志位*/
   OS_EVENT_OR_CONSUME		/*用于清空事件标志位*/

} OS_EVENT_OPERATION;


/* ===========================================================================*/
/**
 * @fn OSA_EventCreate() - Creates a new Event instance.
 *
 *  @ param pEvents              :Handle of the Event to be created.                                              
 */
/* ===========================================================================*/
int OS_EventCreate(OS_PTR *pEvents);


/* ===========================================================================*/
/**
 * @fn OSA_EventDelete() - Deletes a previously created Event instance.
 *
 *  @ param pEvents              :Handle of the Event to be deleted.                                              
 */
/* ===========================================================================*/
int OS_EventDelete(OS_PTR pEvents);


/* ===========================================================================*/
/**
 * @fn OSA_EventSet() - Signals the requested Event. Tasks waiting on
 *                            this event will wake up.
 *
 *  @ param pEvents           :Handle of previously created Event instance.
 *
 *  @ param uEventFlag        :Mask of Event IDs to set.
 *
 *  @ param eOperation        :Operation while setting events.                                    
 */
/* ===========================================================================*/   
int OS_EventSet(OS_PTR pEvents, unsigned long uEventFlag, OS_EVENT_OPERATION eOperation);



/* ===========================================================================*/
/**
 * @fn OSA_EventRetrieve() - Waits for event.
 *
 *  @ param pEvents                :Handle of previously created Event instance
 *
 *  @ param uRequestedEvents       :Mask of Event IDs to wait on
 *
 *  @ param eOperation             :Operation for the wait
 *
 *  @ param pRetreivedEvents       :Mask of the Event IDs retreived on success
 *
 *  @ param uTimeOut               :Time in millisec to wait for the event
 */
/* ===========================================================================*/
int OS_EventRetrieve(OS_PTR pEvents,
                      unsigned long uRequestedEvents,
                      OS_EVENT_OPERATION eOperation,
                      unsigned long *pRetrievedEvents,
                      unsigned long uTimeOut);             


#ifdef __cplusplus
}
#endif
#endif /* _OSA_EVENT_H_ */

