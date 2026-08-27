#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <sys/time.h>
#include "list_use_lock.h"
List_LockHandle_t* list_lockAndCreate()
{

	List_LockHandle_t* pList_Handle = malloc(sizeof(*pList_Handle));
	if(pList_Handle == NULL)
	{
		printf("list_lockAndCreate is fai\n");
		return NULL;
	}
	memset(pList_Handle, 0, sizeof(*pList_Handle));
	pList_Handle->pList_Handle = list_create();
	if(pList_Handle->pList_Handle == NULL)
	{
		printf("list_create is fai\n");
		return NULL;
	}
	pthread_mutex_init(&(pList_Handle->lock), NULL);
	pthread_cond_init(&(pList_Handle->condl),NULL);
	return pList_Handle;
}

List_NodeData_t list_lockAndPop_back(List_LockHandle_t* pHeadHandle)
{
	List_NodeData_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_pop_back(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_NodeData_t list_lockAndPop_front(List_LockHandle_t* pHeadHandle)
{
	List_NodeData_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_pop_front(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndPush_back(List_LockHandle_t* pHeadHandle, void* pData)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_push_back(pHeadHandle->pList_Handle, pData);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndPush_front(List_LockHandle_t* pHeadHandle, void* pData)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_push_front(pHeadHandle->pList_Handle, pData);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

int list_lockAndGet_size(List_LockHandle_t* pHeadHandle)
{
	int size = 0;
	if(pHeadHandle == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	size = list_size(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return size;
}

int list_lockAndDestory(List_LockHandle_t* pList_handle)
{
	if(pList_handle == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&(pList_handle->lock));
	list_destory(pList_handle->pList_Handle);
	pthread_mutex_unlock(&(pList_handle->lock));
	pthread_mutex_destroy(&(pList_handle->lock));
	pthread_cond_destroy(&(pList_handle->condl));
	free(pList_handle);
	return 0;
}
int list_lockAndClear(List_LockHandle_t* pList_handle)
{
	int ret = 0;
	if(pList_handle == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&(pList_handle->lock));
	ret = list_clear(pList_handle->pList_Handle);
	pthread_mutex_unlock(&(pList_handle->lock));
	return ret;
}
int list_lockAndearse_data(List_LockHandle_t* pHeadHandle, void* pData)
{

	int ret = 0;
	if(pHeadHandle == NULL || pData == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	ret = list_earse_data(pHeadHandle->pList_Handle, pData);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return ret;

}

/*****************************************************************************/
static int list_wait_signal(List_LockHandle_t* pHeadHandle, int time)
{
	struct timespec abstime = {2, 0};
	struct timeval now;
	int ret = 0;
	if(pHeadHandle == NULL)
	{
		return -1;
	}
	if(time > 0)
	{
		gettimeofday(&now, NULL);
		abstime.tv_sec = now.tv_sec + time / 1000;
		abstime.tv_nsec = (now.tv_usec * 1000) + (time % 1000) * 1000 * 1000;
		if(abstime.tv_nsec >= 1000000000)
		{
			abstime.tv_nsec -= 1000000000;
			abstime.tv_sec+=1;
		}
		ret = pthread_cond_timedwait(&(pHeadHandle->condl), &(pHeadHandle->lock), &abstime);

	}
	else
	{
		ret= pthread_cond_wait(&(pHeadHandle->condl), &(pHeadHandle->lock));
	}

	return ret;
}

List_NodeData_t list_lockAndgetData(List_LockHandle_t* pHeadHandle, List_CurNode_t pCurNode)
{
	List_NodeData_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_getdata(pCurNode);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndbegin(List_LockHandle_t* pHeadHandle)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_begin(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndnext(List_LockHandle_t* pHeadHandle, List_CurNode_t pCurNode)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_next(pHeadHandle->pList_Handle, pCurNode);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndend(List_LockHandle_t *pHeadHandle)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_end(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

 List_NodeData_t list_lockAndPop_backSignal(List_LockHandle_t* pHeadHandle, int time)
{
	List_NodeData_t pNodeData = NULL;
	int size = 0;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	if((size = list_size(pHeadHandle->pList_Handle)) <= 0)
	{
		list_wait_signal(pHeadHandle,  time);
	}

	pNodeData = list_pop_back(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_NodeData_t list_lockAndPop_frontSignal(List_LockHandle_t* pHeadHandle, int time)
{
	List_NodeData_t pNodeData = NULL;
	int size = 0;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	if((size = list_size(pHeadHandle->pList_Handle)) <= 0)
	{
		list_wait_signal(pHeadHandle,  time);
	}
	pNodeData = list_pop_front(pHeadHandle->pList_Handle);
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndPush_backSignal(List_LockHandle_t* pHeadHandle, void* pData)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_push_back(pHeadHandle->pList_Handle, pData);
	pthread_cond_signal(&(pHeadHandle->condl));
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}

List_CurNode_t list_lockAndPush_frontSignal(List_LockHandle_t* pHeadHandle, void* pData)
{
	List_CurNode_t pNodeData = NULL;
	if(pHeadHandle == NULL)
	{
		return NULL;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pNodeData = list_push_front(pHeadHandle->pList_Handle, pData);
	pthread_cond_signal(&(pHeadHandle->condl));
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return pNodeData;
}
int list_cond_signal(List_LockHandle_t* pHeadHandle)
{
	if(pHeadHandle == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&(pHeadHandle->lock));
	pthread_cond_signal(&(pHeadHandle->condl));
	pthread_mutex_unlock(&(pHeadHandle->lock));
	return 0;
}
