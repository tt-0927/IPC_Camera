/*
 * @FilePath: block.h
 * @Author: yangwenyao
 * @Date: 2022-11-29 10:39:43
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-11-29 16:57:48
 * @Descripttion: 
 */


#ifndef _OS_IPC_SHM_CORE_SOURCE_BLOCK_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_BLOCK_INCLUDE_

#include "os_atom.h"
#include "shm_base.h"



typedef struct _SHM_BLOCKHEAD_INFO_
{
	atomicUint32_t lockNum;		//atom lock
	atomicUint32_t timestamp;	//write timestamp,Recycling mechanism
	Uint32 msgSize;				//current message size
	Uint32 msgTotalSize;		//total message Size
	Uint32 FUs;					//FragmentationUints:|reserve|ID(16)|S(1)|E(1)|Y(1)|
								//ID(16)-->identity��S(1)-->start subpacket, E(1)-->end subpacket,Y(1)-->this message is subpacket
	Uint32 msgOffset;			//package seq number
	Uint32 cmd;					//order

}blockHead_t;


typedef struct _SHM_RW_BLOCK_INFO_
{
	Uint32 index;
	blockHead_t *block;
	unsigned char *buff;
}BlockBuff_t;


int shmBlock_tryLock_write(atomicUint32_t *atomLock);

int shmBlock_tryLock_read(atomicUint32_t *atomLock);

void shmBlock_releaseLock_write(atomicUint32_t *atomLock);

void shmBlock_releaseLock_read(atomicUint32_t *atomLock);

void shmBlock_reset_lock(atomicUint32_t *atomLock);

int shmBlock_trySet_timestamp(atomicUint32_t *atomLock,Uint32 beforeTimestamp,Uint32 timestamp);

int shmBlock_set_timestamp(atomicUint32_t *atomLock,Uint32 timestamp);

Uint32 shmBlock_get_timestamp(atomicUint32_t *atomLock);


#endif //_OS_IPC_SHM_CORE_SOURCE_BLOCK_INCLUDE_


