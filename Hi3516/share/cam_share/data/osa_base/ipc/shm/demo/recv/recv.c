

#include <stdio.h>
#include <stdlib.h>
#include "shm_receiver.h"




int deal_fun(callBackParam_t* param)
{
	static int index = 0;
	FILE *file = (FILE*)param->user;

	if(file)
	{
		fwrite(param->data,1,param->size,file);
		fflush(file);	//把最后的内容刷进文件中
		printf("get recv dataSize[%d] index[%d]\n",param->size,index++);

	}else
	{
		printf("get recv dataSize[%d] data[%s] index[%d] channelName[%s] cmd[%d]\n",\
				param->size,param->data,index++,param->channelName,param->cmd);
	}

	return 0;
}

int main(int argc,char* argv[])
{
	int ret = 0;
	shmReceiver_t *handle = NULL;
	FILE *file = NULL;

	if(argc > 1)
	{
		file = fopen("./getdata.mp4","w+");
		if(file == NULL)
		{
			printf("fopen file error!!\n");
			return -1;
		}
		printf("open file success!!!\n");
	}

	handle = shmReceiver_init("notifier",deal_fun,file);
	if(handle == NULL)
	{
		printf("shmReceiver_init error!!\n");
		return -1;
	}

	//add segment
	ret = shmReceiver_addSegment(handle,"stream");
	if(ret < 0)
	{
		printf("shmReceiver_addSegment error!!\n");
		return -1;
	}
	printf("add segment is success!!\n");

	sleep(5);

//	printf("del segment!!!!!!!!!!\n\n");
//	ret = shmReceiver_delSegment(handle,"stream");
//	if(ret < 0)
//	{
//		printf("shmReceiver_delSegment error!!\n");
//		return -1;
//	}
//
//	sleep(3);
//	ret = shmReceiver_unInit(handle);
//	if(ret < 0)
//	{
//		printf("shmReceiver_unInit error!!\n");
//		return -1;
//	}
//	printf("uninit shm recv success!!!\n");

	while(1)
	{
		sleep(60);
	}

	return 0;
}




