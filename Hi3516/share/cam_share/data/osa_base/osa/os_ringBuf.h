
#ifndef OSA_DOUBLE_RING_BUFFER_H_
#define OSA_DOUBLE_RING_BUFFER_H_


#include <os_que.h>
#include <os_mutex.h>
#ifdef __cplusplus
extern "C" {
#endif


#define OS_RING_BUF_NUM_MAX       (256)
#define OS_RING_BUF_ID_INVALID    (-1)

typedef struct
{
  void* bufInfo[OS_RING_BUF_NUM_MAX];
  OS_QueHndl emptyQue;
  OS_QueHndl fullQue;
  Int64 numBuf;
}OS_ringBufHndl;


int  OS_ringBufCreate(OS_ringBufHndl *hndl, int buf_num);
int  OS_ringBufInitBufInfo(OS_ringBufHndl *hndl,Int64 bufId,void* buffInfo);
int  OS_ringBufDelete(OS_ringBufHndl *hndl);

int  OS_ringBufGetFull(OS_ringBufHndl *hndl, Int64 *bufId, Uint32 timeout);
int  OS_ringBufPutEmpty(OS_ringBufHndl *hndl, Int64 bufId, Uint32 timeout);

int  OS_ringBufGetEmpty(OS_ringBufHndl *hndl, Int64 *bufId, Uint32 timeout);
int  OS_ringBufPutFull(OS_ringBufHndl *hndl, Int64 bufId, Uint32 timeout);

int  OS_ringBufSwitchFull (OS_ringBufHndl *hndl, Int64 *bufId, Uint32 timeout);
int  OS_ringBufSwitchEmpty(OS_ringBufHndl *hndl, Int64 *bufId, Uint32 timeout);

Uint32 OS_ringBufFullCount(OS_ringBufHndl *hndl);
Uint32 OS_ringBufEmptyCount(OS_ringBufHndl *hndl);

/* 获取满队列中的队头的节点，只读不取 */
int OS_ringBufGetFullPeek(OS_ringBufHndl *hndl, Int64 *bufId);

void* OS_ringBufGetBufInfo(OS_ringBufHndl *hndl, Int64 bufId);



/*****************数据流，环形缓冲区******************/

typedef struct
{
    unsigned char *buffer;  //缓冲区
    Uint32	size;           //大小
    Uint32	in;             //入口位置
    Uint32	out;            //出口位置
    pthread_mutex_t lock;	//互斥锁
    int init;               //是否初始化成功1-是
}OS_streamRingBufHndl;

//初始化缓冲区,size要求是2次幂
int OS_streamRingbufferCreate(OS_streamRingBufHndl *hndl,Uint32 size);

//释放缓冲区
int OS_streamRingbufferDelete(OS_streamRingBufHndl *hndl);

//缓冲区的长度
Uint32 OS_streamRingbufferLen(OS_streamRingBufHndl *hndl);

//从缓冲区中取数据
Uint32 OS_streamRingbufferGet(OS_streamRingBufHndl *hndl, unsigned char *buffer, Uint32 size);

//向缓冲区中存放数据
Uint32 OS_streamRingbufferPut(OS_streamRingBufHndl *hndl, unsigned char *buffer, Uint32 size);

//从缓冲区丢弃数据
Uint32 OS_streamRingbufferThrow(OS_streamRingBufHndl *hndl, Uint32 size);



/**************************************************/



#ifdef __cplusplus
}
#endif
#endif //OSA_DOUBLE_RING_BUFFER_H_

