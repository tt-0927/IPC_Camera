


#ifndef _OS_IPC_SHM_CORE_SOURCE_CONDITION_NOTIFIER_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_CONDITION_NOTIFIER_INCLUDE_

#include "os_atom.h"
#include "shm_base.h"
#include <sys/ipc.h>


#define kBufLength (4096)


typedef struct _READABDLE_INFO_
{
	  Uint32 blockIndex;
	  Uint64 channelId;	//参与该共享内存的模块名称对应的hash值

}readAbleInfo_t;



typedef struct _Indicator_
{
	atomicUint64_t nextSeq;
	readAbleInfo_t infos[kBufLength];
    Uint64 seqs[kBufLength];
}Indicator;


typedef struct _SHM_CONDITION_NOTIFIER_
{
	  key_t key;			//shm key
	  void* managedShm;		//shm addr
	  size_t shmSize;		//shm size
	  Indicator* indicator;	//indicator
	  Uint64 nextSeq;		//next block seq
	  atomicUint32_t isShutdown;	//is shutdown 1-yes,0-no

}conditionNotifier_t;



/*
 * 初始化条件变量
 * @param[in] key :shm key
 * @param[out] return :success -> conditionNotifier_t句柄，failed -> NULL
 * */
conditionNotifier_t* condition_notifier_init(key_t key);

/*
 * 销毁条件变量
 * @param[in] handle :conditionNotifier_t句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
int condition_notifier_unInit(conditionNotifier_t* handle);

/*
 * 停止条件变量
 * @param[in] handle :conditionNotifier_t句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
int condition_notifier_shutdown(conditionNotifier_t* handle);

/*
 * 公布一个消息
 * @param[in] handle :conditionNotifier_t句柄
 * @param[in] info :readAbleInfo_t消息的句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
int condition_notifier_notify(conditionNotifier_t* handle,const readAbleInfo_t info);

/*
 * 监听消息
 * @param[in] handle :conditionNotifier_t句柄
 * @param[in] timeout_ms :监听超时
 * @param[in] info :readAbleInfo_t消息的句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
int condition_notifier_listen(conditionNotifier_t* handle,int timeout_ms, readAbleInfo_t* info);



#endif //_OS_IPC_SHM_CORE_SOURCE_CONDITION_NOTIFIER_INCLUDE_




