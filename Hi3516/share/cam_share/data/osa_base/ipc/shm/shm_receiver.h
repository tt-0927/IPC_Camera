/*** 
 * @FilePath     : shm_receiver.h
 * @Author       : huangjunda
 * @Date         : 2024-09-02 14:09:12
 * @LastEditors  : huangjunda
 * @LastEditTime : 2024-09-02 17:23:21
 * @Description  : 
 */

#ifndef _OS_IPC_SHM_CORE_SOURCE_RECEIVER_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_RECEIVER_INCLUDE_

#include <stdio.h>
#include "os_atom.h"
#include "dispatcher.h"


typedef struct _SHM_RECEIVER_INFO_
{
	dispatcher_t* dispatcherPtr;

}shmReceiver_t;



/*
 * 初始化接收
 * @param[in] notifierName：调度器的名称
 * @param[in] dealData:获取到数据后上抛数据的回调接口
 * @param[in] user:用户设置的参数，回调函数带上来的参数
 * */
shmReceiver_t* shmReceiver_init(char* notifierName,dispatcher_deal dealData,void *user);

/*
 * 反初始化接收
 * @param[in] notifierName：调度器的名称
 * */
int shmReceiver_unInit(shmReceiver_t* handle);

/*
 * 给调度器添加一个segment监听数据
 * @param[in] shmReceiver_t句柄
 * @param[in] channelName:通道名称
 * */
int shmReceiver_addSegment(shmReceiver_t* handle,char* channelName);

/*
 * 在调度器删除一个segment监听数据
 * @param[in] shmReceiver_t句柄
 * @param[in] channelName:通道名称
 * */
int shmReceiver_delSegment(shmReceiver_t* handle,char* channelName);




#endif //_OS_IPC_SHM_CORE_SOURCE_RECEIVER_INCLUDE_



