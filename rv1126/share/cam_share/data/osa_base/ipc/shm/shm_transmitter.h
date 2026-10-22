

#ifndef _OS_IPC_SHM_CORE_SOURCE_TRANSMITTER_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_TRANSMITTER_INCLUDE_


#include "segment.h"
#include "condition_notifier.h"
#include "os_mutex.h"

typedef struct _SHM_TRANSMITTER_INFO_
{
	Uint64 notiferId;				//notifier的hash值,信号量控制的共享内存识别id
	Uint64 channelId;				//创建共享内存通道名称对应的hash值，用于区分各个共享模块，共享内存的id号，用于创建
	segment_t* segment;				//传输的数据共享内存
	conditionNotifier_t* notifier;	//调度通知句柄，会创建一个单独的共享内存
	OS_MutexHndl mutex;				//因为要分片发送，所以要锁控制

	/*
	 * 是否需要分包。
	 * 0-不需要，如果发送的包大于初始化分配的空间，则发送方会重新创建新的共享内存，将原来的共享内存销毁，
	 * 		优点：不用mutex锁，发送更快，并发发送效率高。
	 * 		缺点：若接收方接收不及时，则会无法接收还在共享内存的数据，造成丢数据的问题，然后从新的共享内存中继续获取数据。
	 * 			如果接收方速度快，不阻塞，发生上诉问题的概率很小。
	 *
	 * 1-需要分包
	 * 		优点：只要创建了共享内存，中途不会重新创建共享内存。大于初始化分配的空间数据包会分包发送。则需要上锁。
	 *		缺点：多个发送，并发发送速度较低，因为存在竞争锁资源。
	 * */
	int isNeedSubPackage;

}shmTransmitter_t;


/*
 * 初始化一个共享内存
 * @param[in] notifierName:调度名称
 * @param[in] channelName:通道的名称，即共享内存的别名，用户自定义，用于各个进程共享一块共享内存
 * @param[in] buffSize:数据包最大的大小是多少
 * @param[in] isNeedSubPackage:是否需要分包发送
 * @param[out] return: success -> shmTransmitter_t句柄，failed -> NULL
 * */
shmTransmitter_t* shmTransmitter_init(char* notifierName,char* channelName,int buffSize,int isNeedSubPackage);

/*
 * 发送一个数据
 * @param[in] handle:shmTransmitter_t句柄
 * @param[in] data:要发送的数据
 * @param[in] dataSize:要发送的数据的大小
 * @param[out] return: success -> 0，failed -> -1
 * */
int shmTransmitter_Transmit(shmTransmitter_t* handle,char* data,Uint32 dataSize,Uint32 cmd);

/*
 * 销毁一个共享内存
 * @param[in] handle:shmTransmitter_t句柄
 * @param[out] return: success -> 0，failed -> -1
 * */
int shmTransmitter_unInit(shmTransmitter_t* handle);




#endif //_OS_IPC_SHM_CORE_SOURCE_TRANSMITTER_INCLUDE_

