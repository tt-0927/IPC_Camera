
#include <stdio.h>
#include "block.h"
#include <stdio.h>
#include "os_atom.h"
#include "os.h"

const Uint32 kRWLockFree = 0;		//free lock
const Uint32 kWriteExclusive = -1;	//Exclusive write lock
const Uint32 kMaxTryLockTimes = 5;	//get lock try num


Bool shmBlock_tryLock_write(atomicUint32_t *atomLock)
{
	/*
	 * 1>若当前block正在写数据，则当前值为-1，不可获取该block
	 * 2>若当前block正在被读数据，则当前值>0,则不可以获取该block
	 * 总结，通过一个引用的原子操作，可以避免一块内存不被同时写，且正在读的时候不被写，破坏数据！
	 * */
	Uint32 rwLockFree = kRWLockFree;
	if(!AO_CASB(atomLock,rwLockFree, kWriteExclusive))
	{
	    return FALSE; //lock failed
	}
	return TRUE;	//lock success
}

Bool shmBlock_tryLock_read(atomicUint32_t *atomLock)
{
	atomicUint32_t lock_num = 0;
	AO_GET(atomLock);
	lock_num = *atomLock;	//current lock num
	if (lock_num < kRWLockFree)
	{
		//current writing data
		return FALSE;	//get lock failed
	}

	Uint32 try_times = 0;
	while (!AO_CASB(atomLock,lock_num, lock_num + 1))
	{
		++try_times;
		if (try_times == kMaxTryLockTimes)
		{
			printf("fail to add read lock num, curr num:%d\n", lock_num);
			return FALSE; //get lock failed
		}

		AO_GET(atomLock);
		lock_num = *atomLock;	//current lock num
		if (lock_num < kRWLockFree)
		{
			printf("block is being written.\n");
			return FALSE;//get lock failed
		}
	}

	return TRUE;	//lock success
}



void shmBlock_releaseLock_write(atomicUint32_t *atomLock)
{
	AO_ADD(atomLock,1);
}

void shmBlock_releaseLock_read(atomicUint32_t *atomLock)
{
	AO_SUB(atomLock,1);
}

void shmBlock_reset_lock(atomicUint32_t *atomLock)
{
	Uint32 rwLockFree = kRWLockFree;
	AO_SET(atomLock,rwLockFree);
}

int shmBlock_trySet_timestamp(atomicUint32_t *atomLock,Uint32 beforeTimestamp,Uint32 timestamp)
{
	if(!AO_CASB(atomLock, beforeTimestamp, timestamp))
	{
	    return -1;
	}
	return 0;
}

int shmBlock_set_timestamp(atomicUint32_t *atomLock,Uint32 timestamp)
{
	AO_SET(atomLock,timestamp);
	return 0;
}

Uint32 shmBlock_get_timestamp(atomicUint32_t *atomLock)
{
	Uint32 wroteNum = 0;
	AO_GET(atomLock);
	wroteNum = *atomLock;
	return wroteNum;
}

