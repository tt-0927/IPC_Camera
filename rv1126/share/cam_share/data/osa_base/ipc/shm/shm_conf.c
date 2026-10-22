
#include "os.h"
#include <stdio.h>
#include "shm_conf.h"
#include "stdio.h"

#define SHMCONF_STATE_SIZE 1024		//shm header size
#define SHMCONF_BLOCK_SIZE 64		//shm block header size

#define SHMCONF_BLOCK_NUM_16K  512
#define SHMCONF_MESSAGE_SIZE_16K  1024 * 16		//16KB

#define SHMCONF_BLOCK_NUM_128K  128
#define SHMCONF_MESSAGE_SIZE_128K  1024 * 128	//128KB

#define SHMCONF_BLOCK_NUM_512K  128
#define SHMCONF_MESSAGE_SIZE_512K  1024 * 512	//512KB

#define SHMCONF_BLOCK_NUM_1M  64
#define SHMCONF_MESSAGE_SIZE_1M 1024 * 1024 * 1	//1MB

#define SHMCONF_BLOCK_NUM_2M  32
#define SHMCONF_MESSAGE_SIZE_2M  1024 * 1024 * 2	//2MB

#define SHMCONF_BLOCK_NUM_4M  32
#define SHMCONF_MESSAGE_SIZE_4M  1024 * 1024 * 4	//4MB

#define SHMCONF_BLOCK_NUM_8M  32
#define SHMCONF_MESSAGE_SIZE_8M  1024 * 1024 * 8	//8MB

#define SHMCONF_BLOCK_NUM_16M  16
#define SHMCONF_MESSAGE_SIZE_16M  1024 * 1024 * 16	//18MB

#define SHMCONF_BLOCK_NUM_MORE 8
#define SHMCONF_MESSAGE_SIZE_MORE 1024 * 1024 * 32	//32MB


static Uint64 _shmConf_get_blockSize(Uint64 realBuffSize);
static Uint32 _shmConf_get_blockNumer(Uint64 ceilBuffSize);


int shmConf_update(shmConf_t* handle,Uint64 realBuffSize)
{
	if((handle == NULL) || (realBuffSize < 0))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	handle->blockBuffSize = _shmConf_get_blockSize(realBuffSize);
	handle->blockNum = _shmConf_get_blockNumer(handle->blockBuffSize);
	handle->managedShmSize = SHMCONF_STATE_SIZE + \
			(SHMCONF_BLOCK_SIZE + handle->blockBuffSize)*handle->blockNum;

	return 0;
}


static Uint64 _shmConf_get_blockSize(Uint64 realBuffSize)
{
	Uint64 ceiling_msg_size = SHMCONF_MESSAGE_SIZE_16K;
	if (realBuffSize <= SHMCONF_MESSAGE_SIZE_16K)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_16K;
	} else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_128K)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_128K;
	} else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_512K)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_512K;
	}else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_1M)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_1M;
	} else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_2M)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_2M;
	} else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_4M)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_4M;
	} else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_8M)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_8M;
	} else if (realBuffSize <= SHMCONF_MESSAGE_SIZE_16M)
	{
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_16M;
	} else
	{
		//percer buff size is 32MB
		ceiling_msg_size = SHMCONF_MESSAGE_SIZE_MORE;
	}
	return ceiling_msg_size;
}

static Uint32 _shmConf_get_blockNumer(Uint64 ceilBuffSize)
{
	Uint32 num = 0;
	switch (ceilBuffSize)
	{
		case SHMCONF_MESSAGE_SIZE_16K:
			num = SHMCONF_BLOCK_NUM_16K;
			break;
		case SHMCONF_MESSAGE_SIZE_512K:
			num = SHMCONF_BLOCK_NUM_512K;
			break;
		case SHMCONF_MESSAGE_SIZE_128K:
			num = SHMCONF_BLOCK_NUM_128K;
			break;
		case SHMCONF_MESSAGE_SIZE_1M:
			num = SHMCONF_BLOCK_NUM_1M;
			break;
		case SHMCONF_MESSAGE_SIZE_2M:
			num = SHMCONF_BLOCK_NUM_2M;
			break;
		case SHMCONF_MESSAGE_SIZE_4M:
			num = SHMCONF_BLOCK_NUM_4M;
			break;
		case SHMCONF_MESSAGE_SIZE_8M:
			num = SHMCONF_BLOCK_NUM_8M;
			break;
		case SHMCONF_MESSAGE_SIZE_16M:
			num = SHMCONF_BLOCK_NUM_16M;
			break;
		case SHMCONF_MESSAGE_SIZE_MORE:
			num = SHMCONF_BLOCK_NUM_MORE;
			break;
		default:
			printf("unknown ceiling_msg_size[%llu]\n", ceilBuffSize);
			break;
	}
	return num;
}





















