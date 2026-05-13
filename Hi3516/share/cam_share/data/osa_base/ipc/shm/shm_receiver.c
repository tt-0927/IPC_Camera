
#include "shm_receiver.h"





shmReceiver_t* shmReceiver_init(char* notifierName,dispatcher_deal dealData,void *user)
{
	shmReceiver_t* handle = (shmReceiver_t*)malloc(sizeof(shmReceiver_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(shmReceiver_t));

	//创建调度器
	handle->dispatcherPtr = dispatcher_init(notifierName,dealData,user);
	if(handle->dispatcherPtr == NULL)
	{
		printf("dispatcher_init error!!\n");
		if(handle)
		{
			free(handle);
			handle = NULL;
		}
		return NULL;
	}

	return handle;
}

int shmReceiver_addSegment(shmReceiver_t* handle,char* channelName)
{
	if((handle == NULL) || (channelName == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int ret = 0;
	//add segment to dispatcher
	ret = dispatcher_AddSegment_toListener(handle->dispatcherPtr,channelName);
	if(ret < 0)
	{
		printf("dispatcher_AddSegment_toListener error!!\n");
		return -1;
	}

	return 0;
}

int shmReceiver_delSegment(shmReceiver_t* handle,char* channelName)
{
	if((handle == NULL) || (channelName == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int ret = 0;
	//add segment to dispatcher
	ret = dispatcher_DelSegment_toListener(handle->dispatcherPtr,channelName);
	if(ret < 0)
	{
		printf("dispatcher_DelSegment_toListener error!!\n");
		return -1;
	}

	return 0;
}


int shmReceiver_unInit(shmReceiver_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is null!!!\n");
		return -1;
	}

	if(handle->dispatcherPtr)
	{
		dispatcher_unInit(handle->dispatcherPtr);
		handle->dispatcherPtr = NULL;
	}

	free(handle);
	handle = NULL;

	return 0;
}






















