

#ifndef _OS_IPC_SHM_CORE_SOURCE_SEGMENT_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_SEGMENT_INCLUDE_

#include "os_atom.h"
#include "shm_base.h"
#include "block.h"
#include "shm_conf.h"
#include <sys/ipc.h>


typedef enum _SHM_ReadWriteMode_
{
	READ_ONLY = 0,
	WRITE_ONLY,
}ReadWriteMode;


typedef struct _SHM_SEGMENT_INFO_
{
	int init;				//is init,0-uninit,1-init
	char channelName[SHM_CHANNLE_NAME_LEN_MAX];	//通道名称，用于生成id，用于区分不同的共享内存
	key_t id;				//创建segment对象时（channel_id）传入的id，用于区分不同的共享内存
	ReadWriteMode mode;		//this process role,read or write
	shmConf_t shmConf;		//shm conf
	shmHead_t* header;		//shm header
	blockHead_t* blocks;	//指向一个Block对象数组
	void* ptrManagedShm;	//指向共享内存有效地址，共享内存映射到用户空间的首地址
	unsigned char* blockBufAddrs[1024];

}segment_t;



/*
 * 数据段的初始化
 * @param[in] channelName:通道名称，用于生成id，标识不同的共享内存
 * @param[in] mode:角色，读或写，详细见ReadWriteMode
 * @param[in] realBuffSize：预计的blockBuff的大小，读角色设置为0即可。写角色默认是16KB一块buff
 * @param[out] return:success->shmSegment_t数据段句柄,failed -> NULL
 * */
segment_t* segment_init(char* channelName,ReadWriteMode mode,int realBuffSize);

/*
 * 数据段的销毁
 * @param[in] handle:数据段的句柄
 * @param[out] return:0-success,-1-faile
 * */
int segment_unInit(segment_t* handle);

/*
 * 请求一块buff用于写数据
 * @param[in] handle:数据段的句柄
 * @param[in] msg_size:数据的大小
 * @param[in] writable_block:存储带出的block块信息
 * @param[out] return:0-success,-1-faile
 * */
int segment_acquire_blockToWrite(segment_t* handle,size_t msg_size,BlockBuff_t* writable_block);

/*
 * 释放一块buff
 * @param[in] handle:数据段的句柄
 * @param[in] writable_block:存储带出的block块信息
 * @param[out] return:0-success,-1-faile
 * */
int segment_release_writtenBlock(segment_t* handle,const BlockBuff_t writable_block);

/*
 * 请求一块buff用于读数据
 * @param[in] handle:数据段的句柄
 * @param[in] readable_block:存储上抛得数据
 * @param[out] return:0-success,-1-faile
 * */
int segment_acquire_blockToRead(segment_t* handle,BlockBuff_t* readable_block);

/*
 * 释放一块buff
 * @param[in] handle:数据段的句柄
 * @param[in] writable_block:读取得block数据块
 * @param[out] return:0-success,-1-faile
 * */
int segment_release_readBlock(segment_t* handle,const BlockBuff_t readable_block);




#endif //_OS_IPC_SHM_CORE_SOURCE_SEGMENT_INCLUDE_



