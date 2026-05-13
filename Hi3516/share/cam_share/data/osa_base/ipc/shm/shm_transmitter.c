
#include <stdio.h>
#include "os_atom.h"
#include "shm_transmitter.h"
#include "os_hashCode.h"
#include <stdio.h>


shmTransmitter_t* shmTransmitter_init(char* notifierName,char* channelName,int buffSize,int isNeedSubPackage)
{
	if((notifierName == NULL) || (channelName == NULL))
	{
		printf("this argument is NULL!!\n");
		return NULL;
	}

	shmTransmitter_t* handle = (shmTransmitter_t*)malloc(sizeof(shmTransmitter_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(shmTransmitter_t));

	//生成对应的hash值
	handle->channelId = (Uint64)BKDRHash(channelName,strlen(channelName));
	handle->notiferId = (Uint64)BKDRHash(notifierName,strlen(notifierName));
	handle->isNeedSubPackage = isNeedSubPackage;

	printf("name[%s] channellID:%llu\n",channelName,handle->channelId);
	printf("name[%s] notiferId:%llu\n",notifierName,handle->notiferId);

	//able segment and condition Notfier
	handle->segment = segment_init(channelName,WRITE_ONLY,buffSize);
	if(handle->segment == NULL)
	{
		printf("segment init error!!\n");
		return NULL;
	}
	handle->notifier = condition_notifier_init((key_t)handle->notiferId);

	//init mutex
	OS_mutexCreate(&(handle->mutex));

	return handle;
}

static int _shmTransmitter_Transmit(shmTransmitter_t* handle,char* data,Uint32 dataSize,Uint32 dataTotalSize,Uint32 offset,Uint32 FUs,Uint32 cmd)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int ret = 0;
	BlockBuff_t block;

	memset(&block,0,sizeof(BlockBuff_t));

	//get segment block
	if(segment_acquire_blockToWrite(handle->segment,dataSize,&block) < 0)
	{
		printf("get block to write error!!\n");
		return -1;
	}

	//cp data
	block.block->msgSize = dataSize;
	block.block->FUs = FUs;
	block.block->cmd = cmd;
	block.block->msgOffset = offset;
	block.block->msgTotalSize = dataTotalSize;
	memcpy(block.buff,data,dataSize);

	//release block lock
	segment_release_writtenBlock(handle->segment,block);

	readAbleInfo_t readable;
	memset(&readable,0,sizeof(readAbleInfo_t));
	readable.channelId = handle->channelId;
	readable.blockIndex = block.index;

	//notifier
	condition_notifier_notify(handle->notifier,readable);

	return 0;
}


static int shmTransmitter_Transmit_subpackage(shmTransmitter_t* handle,char* data,Uint32 dataSize,Uint32 cmd)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int ret = 0;
	int index = 0;
	BlockBuff_t block;
	int blockNum = 0;
	int cellSize = 0;
	char* dataPrt = NULL;
	Uint32 offset = 0;
	Uint32 FUs = 0;

	//上锁
	OS_mutexLock(&(handle->mutex));

	//分块
	blockNum = OS_ceil(dataSize,(handle->segment->shmConf.blockBuffSize));
	blockNum = blockNum/(handle->segment->shmConf.blockBuffSize);

	dataPrt = data;

	for(index = 0;index < blockNum;index++)
	{

		if(index == (blockNum - 1))
		{
			//剩余的数据
			cellSize = dataSize - ((handle->segment->shmConf.blockBuffSize)*(blockNum - 1));
		}else
		{
			cellSize = handle->segment->shmConf.blockBuffSize;
		}

		FUs = 0;//|reserve|ID(16)|S(1)|E(1)|Y(1)|
		if(blockNum > 1)
		{
			if((index == 0))				//start
			{
				FUs |= 0x1 << 0;	//Y
				FUs |= 0x1 << 2;	//S
			}else if(index == (blockNum - 1))	//end
			{
				FUs |= 0x1 << 0;	//Y
				FUs |= 0x1 << 1;	//E
			}else
			{
				FUs |= 0x1 << 0;	//Y
			}
			//后期考虑是否需要在每个包都添加一个唯一ID
		}

		//发送
		_shmTransmitter_Transmit(handle,dataPrt,cellSize,dataSize,offset,FUs,cmd);

		//指针指向下一个数据块
		dataPrt += cellSize;
		offset += cellSize;
	}


	//解锁
	OS_mutexUnlock(&(handle->mutex));

	return 0;
}

static int shmTransmitter_Transmit_noSubpackage(shmTransmitter_t* handle,char* data,Uint32 dataSize,Uint32 cmd)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int ret = 0;
	//发送
	ret = _shmTransmitter_Transmit(handle,data,dataSize,dataSize,0,0,cmd);

	return ret;
}




int shmTransmitter_Transmit(shmTransmitter_t* handle,char* data,Uint32 dataSize,Uint32 cmd)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int ret = 0;

	if(handle->isNeedSubPackage == 1)
	{
		//分包发送
		ret = shmTransmitter_Transmit_subpackage(handle,data,dataSize,cmd);
	}else
	{
		//不分包，直接发送
		ret = shmTransmitter_Transmit_noSubpackage(handle,data,dataSize,cmd);
	}

	return ret;
}







int shmTransmitter_unInit(shmTransmitter_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	if(segment_unInit(handle->segment) < 0)
	{
		printf("unInit segment is error!!\n");
		return -1;
	}

	if(condition_notifier_unInit(handle->notifier) < 0)
	{
		printf("unInit notifier error!!\n");
		return -1;
	}

	OS_mutexDelete(&(handle->mutex));

	free(handle);
	handle = NULL;

	return 0;
}















