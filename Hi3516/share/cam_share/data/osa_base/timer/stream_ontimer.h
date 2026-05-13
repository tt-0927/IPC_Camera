#ifndef STREAM_ONTIMER_H
#define STREAM_ONTIMER_H
#include<pthread.h>
#include <signal.h>
#include <time.h>
#include "list_base.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef  void* SETTIMER_HANDLE;
typedef int(*onTimeFunCallBack)(void* argv);
//指定的某一位数置1
#define setbit(x,y)  x|=(1<<y)
//指定的某一位数置0
#define clrbit(x,y)  x&=~(1<<y)
//指定的某一位数取反
#define reversebit(x,y)  x^=(1<<y)
//获取的某一位的值
#define getbit(x,y)   ((x) >> (y)&1)
typedef struct Stream_frameSkipContext {
   int inputFrameRate;
   int outputFrameRate;
   int firstTime;
   int inCnt;
   int outCnt;
   int multipleCnt;
} Stream_frameSkipContext_t;
typedef struct StreamSetTimer_Info
{
	int secondCount;//每秒发送次数
	onTimeFunCallBack funCall;
	void* argv;//用户自己传递参数用于回调使用
	Stream_frameSkipContext_t outputSkip;//内部使用参数
}StreamSetTimer_Info_t;
typedef struct On_Time_Handle
{
	List_Handle_t pList_Handle;
	pthread_mutex_t lock;
	pthread_t tid;
	timer_t timer;
	int start_timer;
	int sendCount;

}On_Time_Handle_t;

/*
 *
 * int secondCount 每秒最多触发多少次 最多是30次
 */
On_Time_Handle_t *steam_init_ontimer(int secondCount);

//每秒出发多少次，最多30，超过30建议设置一般自行处理（建议如果是60 ，可以连续设置两次一样可达到600的效果）
SETTIMER_HANDLE stream_settimer(On_Time_Handle_t *ontime_init_Handle, StreamSetTimer_Info_t* info);

void* stream_killtimer(On_Time_Handle_t *ontime_init_Handle, SETTIMER_HANDLE timerhandle);

int sStream_doSkipFrame(Stream_frameSkipContext_t *frameSkipCtx );

#ifdef __cplusplus
}
#endif
#endif
