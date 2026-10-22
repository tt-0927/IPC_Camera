

#ifndef _OS_PRF_H_
#define _OS_PRF_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os.h"

#define OS_PRF_ENABLE

#define OS_PRF_PRINT_DEFAULT   (OS_PRF_PRINT_TIME|OS_PRF_PRINT_VALUE)

#define OS_PRF_PRINT_ALL       (0xFFFF)

#define OS_PRF_PRINT_TIME      (0x0001)
#define OS_PRF_PRINT_VALUE     (0x0002)
#define OS_PRF_PRINT_MIN_MAX   (0x0004)
#define OS_PRF_PRINT_COUNT     (0x0008)

/*
Profile info  : <name>
======================
Iterations    :
Avg Time (ms) :
Max Time (ms) :
Min Time (ms) :
Avg Value     :
Avg Value/sec :
Max Value     :
Min Value     :
*/

#ifdef OS_PRF_ENABLE

typedef struct 
{

  Uint32 totalTime;		//统计的总时间
  Uint32 maxTime;		//统计过程中最长时间
  Uint32 minTime;		//统计过程中最短时间

  Uint32 totalValue;
  Uint32 maxValue;
  Uint32 minValue;

  Uint32 count;
  Uint32 curTime;

} OS_PrfHndl;

typedef struct 
{
	Uint32 pre_time;
	Uint32 cur_time;
	Uint32 timePrintf;  //多长时间打印一次ms
	Uint32 count;
	Uint32 flag;
	char msg[128];
} OS_FrameHndl_t;

/*
 * 打印帧率
 * @in Param hndl : 帧率句柄，定义一个OS_FrameHndl_t即可
 *
 * */
int OS_prfFps(OS_FrameHndl_t *hndl);


/*
 * 下面的函数组用于统计程序的运行时间等信息
 * */

/*
 * 开始统计
 * @in param hndl : 统计句柄
 *
 * */
int OS_prfBegin(OS_PrfHndl *hndl);

/*
 * 结束统计
 * @in param hndl : 统计句柄
 * @in param value : 计数值
 *
 * */
int OS_prfEnd(OS_PrfHndl *hndl, Uint32 value);

/*
 * 重置句柄内容都为0
 * @in param hndl : 统计句柄
 *
 * */
int OS_prfReset(OS_PrfHndl *hndl);

/*
 * 打印统计信息
 * @in param hndl : 统计句柄
 * @in param name : 统计的名称
 * @in param flags : 具体打印什么信息
 * 					取值范围：OS_PRF_PRINT_TIME
 * 					OS_PRF_PRINT_VALUE
 * 					OS_PRF_PRINT_MIN_MAX
 * 					OS_PRF_PRINT_COUNT
 * 					OS_PRF_PRINT_ALL
 * 					OS_PRF_PRINT_DEFAULT
 *
 * */
int OS_prfPrint(OS_PrfHndl *hndl, char *name, Uint32 flags);

#else

typedef struct {

  int rsv;

} OS_PrfHndl;

#define OS_prfBegin(hndl)
#define OS_prfEnd(hndl, value)
#define OS_prfReset(hndl)
#define OS_prfPrint(hndl, name, flags)

#endif

#ifdef __cplusplus
}
#endif
#endif /* _OS_PRF_H_ */



