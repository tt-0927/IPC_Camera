#ifndef _LIST_USE_LOCK_H__
#define _LIST_USE_LOCK_H__
#include "list_base.h"
#include<pthread.h>
//这个文件主要是基于链表进行锁操作的，外界无需再加锁

typedef struct _List_LockHandle
{
	List_Handle_t pList_Handle;
	pthread_mutex_t lock;
	pthread_cond_t  condl;
}List_LockHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
List_LockHandle_t* list_lockAndCreate();

List_NodeData_t list_lockAndPop_back(List_LockHandle_t* pHeadHandle);

List_NodeData_t list_lockAndPop_front(List_LockHandle_t* pHeadHandle);

List_CurNode_t list_lockAndPush_back(List_LockHandle_t* pHeadHandle, void* pData);

List_CurNode_t list_lockAndPush_front(List_LockHandle_t* pHeadHandle, void* pData);

int list_lockAndGet_size(List_LockHandle_t* pHeadHandle);

int list_lockAndDestory(List_LockHandle_t* pList_handle);

int list_lockAndClear(List_LockHandle_t* pList_handle);

int list_lockAndearse_data(List_LockHandle_t* pHeadHandle, void* pData);



List_NodeData_t list_lockAndgetData(List_LockHandle_t* pHeadHandle, List_CurNode_t pCurNode);

List_CurNode_t list_lockAndbegin(List_LockHandle_t* pHeadHandle);

List_CurNode_t list_lockAndnext(List_LockHandle_t* pHeadHandle, List_CurNode_t pCurNode);

List_CurNode_t list_lockAndend(List_LockHandle_t* pHeadHandle);

List_NodeData_t list_lockAndPop_backSignal(List_LockHandle_t* pHeadHandle, int time);

List_NodeData_t list_lockAndPop_frontSignal(List_LockHandle_t* pHeadHandle, int time);

List_CurNode_t list_lockAndPush_backSignal(List_LockHandle_t* pHeadHandle, void* pData);

List_CurNode_t list_lockAndPush_frontSignal(List_LockHandle_t* pHeadHandle, void* pData);


int list_cond_signal(List_LockHandle_t* pHeadHandle);
#ifdef __cplusplus
}
#endif

#endif
