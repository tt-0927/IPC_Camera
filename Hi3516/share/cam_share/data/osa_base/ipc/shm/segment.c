
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "segment.h"
#include "shm_base.h"

static int _shm_init(segment_t* handle);
static int _shm_openAndCreate(segment_t* handle);
static int _shm_openOnly(segment_t* handle);
static int _shm_remove(segment_t* handle);
static int _shm_destroy(segment_t* handle);
static int _shm_reset(segment_t* handle);
static int _shm_remap(segment_t* handle);
static int _shm_recreate(segment_t* handle);
static Uint32 _shm_get_nextWritableBlockIndex(segment_t* handle);



int segment_acquire_blockToWrite(segment_t* handle,size_t msg_size,BlockBuff_t* writable_block)
{
	if((handle == NULL) || (writable_block == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	if ((handle->init == 0) && (_shm_init(handle) < 0))
	{
		printf("init failed, can't write now.\n");
		return -1;
	}

	int result = 0;
	if (shmConf_get_needRemap(handle->header))
	{
		printf("====================> this shm has been modify! so need remap!!! <====================\n");
		//其他进程已经更新了共享内存的大小。现在需要重新映射最新的共享内存
		result = _shm_remap(handle);
	}

	//判断获取的buff是否大于预设的大小
	if (msg_size > (handle->shmConf.blockBuffSize))
	{
		printf("====================> this msg size > current block size,so need recreate!!!<====================\n");
		//重新创建共享内存
		shmConf_update(&(handle->shmConf),msg_size);
		result = _shm_recreate(handle);
	}

	if (result < 0)
	{
		printf("segment update failed.\n");
		return -1;
	}

	Uint32 index = _shm_get_nextWritableBlockIndex(handle);
	writable_block->index = index;
	writable_block->block = &(handle->blocks[index]);
	writable_block->buff = handle->blockBufAddrs[index];
	return 0;
}

int segment_release_writtenBlock(segment_t* handle,const BlockBuff_t writable_block)
{
	if((handle == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	Uint32 index = writable_block.index;
	if (index >= handle->shmConf.blockNum)
	{
		return 0;
	}
	//release write lock
	shmBlock_releaseLock_write(&(handle->blocks[index].lockNum));
	return 0;
}

int segment_acquire_blockToRead(segment_t* handle,BlockBuff_t* readable_block)
{
	if((handle == NULL) || (readable_block == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	if ((handle->init == 0) && (_shm_init(handle) < 0))
	{
		printf("init failed, can't read now.\n");
		return -1;
	}

	Uint32 index = readable_block->index;
	if (index >= handle->shmConf.blockNum)
	{
		printf("invalid block_index[%d]", index);
		return -1;
	}

	int result = 0;
	if (shmConf_get_needRemap(handle->header))
	{
		printf("====================> this shm has been modify! so need remap!!! <====================\n");
		result = _shm_remap(handle);
	}

	if (result < 0)
	{
		printf("segment update failed.\n");
		return -1;
	}

	if (!shmBlock_tryLock_read(&(handle->blocks[index].lockNum)))
	{
		return -1;
	}

	readable_block->block = &(handle->blocks[index]);
	readable_block->buff = handle->blockBufAddrs[index];
	return 0;
}

int segment_release_readBlock(segment_t* handle,const BlockBuff_t readable_block)
{
	if((handle == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	Uint32 index = readable_block.index;
	if (index >= handle->shmConf.blockNum)
	{
		return 0;
	}
	shmBlock_releaseLock_read(&(handle->blocks[index].lockNum));
	return 0;
}


segment_t* segment_init(char* channelName,ReadWriteMode mode,int realBuffSize)
{
	//creat handle
	segment_t *handle = (segment_t*)malloc(sizeof(segment_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(segment_t));

	Uint64 channel = 0;
	snprintf(handle->channelName,sizeof(handle->channelName),"%s",channelName);
	channel = (Uint64)BKDRHash(channelName,strlen(channelName));

	handle->id = (key_t)channel;
	handle->mode = mode;

	if(handle->mode == WRITE_ONLY)
	{
		//init shm conf
		if(shmConf_update(&(handle->shmConf),realBuffSize) < 0)
		{
			printf("shmConf_update error!!\n");
			if(handle)
			{
				free(handle);
				handle = NULL;
			}
			return NULL;
		}
	}

	//init shm
	_shm_init(handle);

	return handle;
}


int segment_unInit(segment_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	_shm_destroy(handle);

	free(handle);
	handle = NULL;
	return 0;
}



static int _shm_init(segment_t* handle)
{
	int ret = 0;
	if(handle == NULL)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}
	if(handle->init == 1)
	{
		printf("this shm is init!\n");
		return 0;
	}

	if(handle->mode == READ_ONLY)
	{
		ret = _shm_openOnly(handle);
	}else
	{
		ret = _shm_openAndCreate(handle);
	}

	return ret;
}



static int _shm_openAndCreate(segment_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	// create managed_shm_
	int retry = 0;
	int shmid = 0;
	while (retry < 2)
	{
		shmid = shmget(handle->id, handle->shmConf.managedShmSize, 0644 | IPC_CREAT | IPC_EXCL);
		if (shmid != -1)
		{
			break;
		}
		if (EINVAL == errno)
		{
			printf("need larger space, recreate.\n");
			_shm_reset(handle);
			_shm_remove(handle);
			++retry;
		} else if (EEXIST == errno)
		{
			printf("shm already exist, open only.\n");
			return _shm_openOnly(handle);
		} else
		{
			break;
		}
	}

	if (shmid == -1)
	{
		printf("create shm failed, error code: %s\n",strerror(errno));
		return -1;
	}

	// attach managed_shm_
	handle->ptrManagedShm = shmat(shmid, NULL, 0);
	if (handle->ptrManagedShm == (void*)-1)
	{
		printf("attach shm failed.\n");
		shmctl(shmid, IPC_RMID, 0);
		return -1;
	}

	//create field header
	handle->header = (shmHead_t*)handle->ptrManagedShm;
	memset(handle->header,0,sizeof(shmHead_t));
	handle->header->blockBuffSize = handle->shmConf.blockBuffSize;

	//create field block
	handle->blocks = (blockHead_t*)((char *)(handle->ptrManagedShm) + sizeof(shmHead_t));

	//create block buff
	int i = 0;
	for(i = 0; i < handle->shmConf.blockNum;i++)
	{
		handle->blockBufAddrs[i] = (char *)(handle->ptrManagedShm) + sizeof(shmHead_t) + \
				sizeof(blockHead_t)*(handle->shmConf.blockNum) + i*(handle->shmConf.blockBuffSize);
	}

	handle->init = 1;
	shmHead_increase_ReferenceCounts(handle->header);

	return 0;
}

static int _shm_openOnly(segment_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	// get managed_shm_
	int shmid = shmget(handle->id, 0, 0644);
	if (shmid == -1)
	{
		printf("get shm failed.\n");
		return -1;
	}

	// attach managed_shm_
	handle->ptrManagedShm = shmat(shmid, NULL, 0);
	if ((handle->ptrManagedShm) == (void*)-1)
	{
		printf("attach shm failed.\n");
		return -1;
	}

	// get field state_
	handle->header = (shmHead_t*)handle->ptrManagedShm;
	if (handle->header == NULL)
	{
		printf("get state failed.\n");
		shmdt(handle->ptrManagedShm);
		handle->ptrManagedShm = NULL;
		return -1;
	}


	// check block size
	if(handle->mode == WRITE_ONLY)
	{
		if(handle->shmConf.blockBuffSize > handle->header->blockBuffSize)
		{
			printf("=============> need block size > current block size, so need recreate!!! <=============\n");
			//重新创建共享内存
			if(_shm_recreate(handle) < 0)
			{
				printf("_shm_recreate error!!\n");
				return -1;
			}
		}
	}
	shmConf_update(&(handle->shmConf),handle->header->blockBuffSize);

	// get field blocks_
	handle->blocks = (blockHead_t*)((char *)(handle->ptrManagedShm) + sizeof(shmHead_t));
	if (handle->blocks == NULL)
	{
		printf("get blocks failed.\n");
		handle->header = NULL;
		shmdt(handle->ptrManagedShm);
		handle->ptrManagedShm = NULL;
		return -1;
	}

	 // get block buf
	int i = 0;
	for(i = 0; i < handle->shmConf.blockNum;i++)
	{
		Uint8* addr = (char *)(handle->ptrManagedShm) + sizeof(shmHead_t) + \
				sizeof(blockHead_t)*(handle->shmConf.blockNum) + i*(handle->shmConf.blockBuffSize);
		if(addr == NULL)
		{
			printf("this addr is NULL!!!\n");
			break;
		}
		handle->blockBufAddrs[i] = addr;
	}

	if (i != handle->shmConf.blockNum)
	{
		printf("open only failed.\n");
		handle->header = NULL;
		handle->blocks = NULL;
		shmdt(handle->ptrManagedShm);
		handle->ptrManagedShm = NULL;
		shmctl(shmid, IPC_RMID, 0);
		return -1;
	}

	handle->init = 1;
	shmHead_increase_ReferenceCounts(handle->header);

	return 0;
}

static int _shm_remove(segment_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int shmid = shmget(handle->id, 0, 0644);
	if (shmid == -1 || shmctl(shmid, IPC_RMID, 0) == -1)	//delete shm
	{
		printf("remove shm failed, error code: %s\n", strerror(errno));
		return -1;
	}

	printf("remove success.\n");
	return 0;
}


static int _shm_destroy(segment_t* handle)
{
	if (handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	if(handle->init != 1)
	{
		return 0;
	}

	shmHead_decrease_ReferenceCounts((handle->header));
	Uint32 reference_counts = shmHead_get_referenceCounts(handle->header);
	_shm_reset(handle);	//作用是将指定的共享内存段从当前进程空间中脱离出去
	if(reference_counts == 0)
	{
		return _shm_remove(handle);
	}

	printf("destroy,reference_counts[%d]\n",reference_counts);
	return 0;
}

static int _shm_reset(segment_t* handle)
{
	if (handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	handle->header = NULL;
	handle->blocks = NULL;

	if (handle->ptrManagedShm != NULL)
	{
		shmdt(handle->ptrManagedShm); //decatenation
		handle->ptrManagedShm = NULL;
	}
	return 0;
}


static int _shm_remap(segment_t* handle)
{
	if (handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	handle->init = 0;
	_shm_reset(handle);
	return _shm_openOnly(handle);
}


static int _shm_recreate(segment_t* handle)
{
	if (handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
	handle->init = 0;
	shmConf_set_needRemap(handle->header);

	_shm_reset(handle);
	_shm_remove(handle);
	return _shm_openAndCreate(handle);
}


static Uint32 _shm_get_nextWritableBlockIndex(segment_t* handle)
{
	if (handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	Uint32 try_idx = shmHead_get_wroteNum(handle->header);
	Uint32 max_mod_num = handle->shmConf.blockNum - 1;
	Uint32 beforeTimestamp = 0;

	while (1)
	{
		if (try_idx >= handle->shmConf.blockNum)
		{
			try_idx &= max_mod_num;	//超过最大值，再循环一次
		}

		beforeTimestamp = shmBlock_get_timestamp(&(handle->blocks[try_idx].timestamp));
		if (shmBlock_tryLock_write(&(handle->blocks[try_idx].lockNum)))
		{
			//add timestamp
			shmBlock_set_timestamp(&(handle->blocks[try_idx].timestamp),OS_getSysTimeInMsec());
			shmHead_increase_WroteNum(handle->header);
			return try_idx;
		}

		/*
		 * 情况有二：
		 * 1>有程序正在读写该数据块，不可用，属正常现象
		 * 2>若有程序读写该数据块的时候，异常退出了，导致该数据块的锁一直没有得到释放，则需要回收该数据块
		 * 		针对该情况，则每次写该数据块的时候，打上一次时间戳，记录最后写该数据块的时间，每次获取失败则做一次比较，
		 * 		若该数据块未使用时间超过一定时间，则触发回收机制。
		 * */
		Uint32 currentTimestamp = OS_getSysTimeInMsec();
		Uint32 timestampCal = 0;
		timestampCal = abs(currentTimestamp - beforeTimestamp);
		if(timestampCal > 60*1000)	//time out > 60s
		{
			//recycling block
			if(shmBlock_trySet_timestamp(&(handle->blocks[try_idx].timestamp),beforeTimestamp,OS_getSysTimeInMsec()) == 0)
			{
				printf("==============> <<Recycling>> reset this block[%d] timestampCal[%d] <==============\n",try_idx,timestampCal);
				shmBlock_reset_lock(&(handle->blocks[try_idx].lockNum));	//reset
				continue;	//continue try get this block
			}
		}

		shmHead_increase_WroteNum(handle->header); //跳过不能写的数据块
		printf("==============>diffence timestamp[%d] lockNum[%d] try_idx[%d] <==============\n",\
							timestampCal,handle->blocks[try_idx].lockNum,try_idx);
		++try_idx;
	}
}















