#ifndef GETTIME_H
#define GETTIME_H
#ifdef __cplusplus
extern "C" {
#endif
#include<sys/time.h>
#include<string.h>
#include<stdio.h>
//#include "ScErr.h"
//#include "SCDef.h"
#define Max 50

int csTimeBegin(const char *str);
unsigned long csTimeEnd(const char *str1);
int FunCallBackTime(const char *str);

/*
说明：统计函数的调用次数
参数1：区分不同的函数
参数2：统计多长时间函数的调用次数，时间以秒为单位
返回值：返回-1为统计中，即返回值不为-1时，为调用的次数
后期封装新接口 该接口感觉效率不高
*/
int FrameRate(const char *Rate, long FlagTime);

//建议采用下面的接口，可减少性能

typedef void* COUNT_FRAME_HANDLE;
/*
说明：统计函数的调用次数
参数1：区分不同的函数
参数2：统计多长时间函数的调用次数，时间以秒为单位
参数3 display是否打印帧率
返回值：返回-1为统计中，即返回值不为-1时，为调用的次数
后期封装新接口 该接口感觉效率不高
*/
COUNT_FRAME_HANDLE count_framerate_init(const char *str ,long sectime, int display);

int count_framerate_deal(COUNT_FRAME_HANDLE handle);

int timeBegin(const char *str, void** handle);

unsigned long timeEnd(void** handle);

int free_timecountHandle(void** handle);

#ifdef __cplusplus
}
#endif
#endif
