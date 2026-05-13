

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "shm_transmitter.h"





int main(int argc,char* argv[])
{
	int ret = 0;
	int index = 0;
	shmTransmitter_t* handle = NULL;
	int buffsize = 16000;

	if(argc > 1)
	{
		buffsize = atoi(argv[1]);
	}
	printf("buffsize[%d]\n\n\n\n",buffsize);

	handle = shmTransmitter_init("notifier","stream",buffsize,1);
	if(handle == NULL)
	{
		printf("shmTransmitter_init error!!\n");
		return -1;
	}


	char data[128] = {0};
	FILE *file = NULL;
	int n = 0;
	char *sendPtr = NULL;
	int sendSize = 0;
	char buff[48*1000] = {0};

	if(argc > 2)
	{
		printf("=============open file:%s\n",argv[1]);
		file = fopen (argv[1], "rb");
		struct stat st;
		fstat (fileno (file), &st);
		n = st.st_size;
	}

	unsigned int starttime = OS_getSysTimeInMsec();
	int flag = 0;
	//发送数据
	while(flag++ < 10)
	{
		if(file != NULL)
		{
			sendSize = fread (buff, 1, sizeof (buff), file);
			if( sendSize <= 0)
			{
				printf("read file end!!!sendSize[%d]\n",sendSize);
				break;
			}

			sendPtr = buff;

			printf("index[%d] send size[%d]\n",index++,sendSize);

		}else
		{
			sleep(1);
			//usleep(100*1000);

			sprintf(data,"you have new message[%d]",index++);
			printf("send data:%s\n",data);

			sendPtr = data;
			sendSize = strlen(sendPtr)+1;
		}

		ret = shmTransmitter_Transmit(handle,sendPtr,sendSize,30032);
		if(ret < 0)
		{
			printf("send data to shm error!!\n");
		}

	}

	unsigned int endtime = OS_getSysTimeInMsec();
	printf("send time[%d]\n",endtime - starttime);

	sleep(1);

	printf("shm send uninit!!!!!\n");
	shmTransmitter_unInit(handle);


	while(1)
	{
		sleep(60);
	}

	return 0;
}



