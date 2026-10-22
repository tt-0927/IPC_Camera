

#ifndef _OS_IPC_SHM_CORE_SOURCE_DISPATCHER_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_DISPATCHER_INCLUDE_

#include "os_atom.h"
#include "shm_base.h"
#include "condition_notifier.h"
#include "os_thr.h"
#include "rbtree_intkey.h"
#include "segment.h"
#include "os_mutex.h"

typedef struct _DISPATCHER_CALLBACK_PARAM_
{
	void* user;		//用户设置的回调参数
	char* data;		//接收的内容
	int size;		//数据的大小
	int cmd;		//order
	char* channelName;	//通道名称
}callBackParam_t;

typedef int(*dispatcher_deal)(callBackParam_t*);


typedef struct _SEGMENT_BLOCK_INDEX_
{
	segment_t* segments;
	Uint32 blockIndexes;
}segmentContainer_t;

typedef struct _SUBPACKET_INFO_
{
	int totalSize;
	char *buff;

}subpacketInfo_t;


typedef struct _DISPATCHER_INFO_
{
	rbtreeHandle_t* segmentMap;	//使用红黑树实现map
	OS_MutexHndl segmentMapMutex;	//红黑树的锁
	OS_ThrHndl thread_;
	Uint64 notiferId;			//notifier的hash值
	conditionNotifier_t* notifier;
	atomicUint32_t isShutdown;		//is shutdown,1-yes,0-no
	dispatcher_deal dealData;
	void* user;						//user set
	subpacketInfo_t subpacket;		//subpacket info
	int exit_;						//1-退出调度器，并释放资源
}dispatcher_t;



/*
 * 初始化一个调度器
 * @param[in] notifierName：调度器的名称
 * @param[in] dealData:获取到数据后上抛数据的回调接口
 * @param[in] user:用户设置的参数，回调函数带上来的参数
 * */
dispatcher_t* dispatcher_init(char* notifierName,dispatcher_deal dealData,void *user);

/*
 * 反初始化一个调度器
 * @param[in] notifierName：调度器的名称
 * */
int dispatcher_unInit(dispatcher_t* handle);

/*
 * 给调度器添加一个segment监听数据
 * @param[in] handle:dispatcher_t句柄
 * @param[in] channelName:通道名称
 * */
int dispatcher_AddSegment_toListener(dispatcher_t* handle,char *channelName);

/*
 * 在调度器删除一个segment监听数据
 * @param[in] handle:dispatcher_t句柄
 * @param[in] channelName:通道名称
 * */
int dispatcher_DelSegment_toListener(dispatcher_t* handle,char *channelName);


#endif //_OS_IPC_SHM_CORE_SOURCE_DISPATCHER_INCLUDE_




