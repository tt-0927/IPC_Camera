
#ifndef OSA_DOUBLE_RING_BUFFER_H_
#define OSA_DOUBLE_RING_BUFFER_H_


#include <os_que.h>
#include <os_mutex.h>
#include <stdint.h>
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
  int64_t numBuf;
}OS_ringBufHndl;


int  OS_ringBufCreate(OS_ringBufHndl *hndl, int buf_num);
int  OS_ringBufInitBufInfo(OS_ringBufHndl *hndl,int64_t bufId,void* buffInfo);
int  OS_ringBufDelete(OS_ringBufHndl *hndl);

int  OS_ringBufGetFull(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout);
int  OS_ringBufPutEmpty(OS_ringBufHndl *hndl, int64_t bufId, uint32_t timeout);

int  OS_ringBufGetEmpty(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout);
int  OS_ringBufPutFull(OS_ringBufHndl *hndl, int64_t bufId, uint32_t timeout);

int  OS_ringBufSwitchFull (OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout);
int  OS_ringBufSwitchEmpty(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout);

uint32_t OS_ringBufFullCount(OS_ringBufHndl *hndl);
uint32_t OS_ringBufEmptyCount(OS_ringBufHndl *hndl);

/* 获取满队列中的队头的节点，只读不取 */
int OS_ringBufGetFullPeek(OS_ringBufHndl *hndl, int64_t *bufId);

void* OS_ringBufGetBufInfo(OS_ringBufHndl *hndl, int64_t bufId);



/*****************数据流，环形缓冲区******************/

typedef struct
{
    unsigned char *buffer;  //缓冲区
    uint32_t	size;           //大小
    uint32_t	in;             //入口位置
    uint32_t	out;            //出口位置
    pthread_mutex_t lock;	//互斥锁
    int init;               //是否初始化成功1-是
}OS_streamRingBufHndl;

//初始化缓冲区,size要求是2次幂
int OS_streamRingbufferCreate(OS_streamRingBufHndl *hndl,uint32_t size);

//释放缓冲区
int OS_streamRingbufferDelete(OS_streamRingBufHndl *hndl);

//缓冲区的长度
uint32_t OS_streamRingbufferLen(OS_streamRingBufHndl *hndl);

//从缓冲区中取数据长度
uint32_t OS_streamRingbufferGetLen(OS_streamRingBufHndl *hndl);

//从缓冲区中取数据
uint32_t OS_streamRingbufferGet(OS_streamRingBufHndl *hndl, unsigned char *buffer, uint32_t size);

//向缓冲区中存放数据
uint32_t OS_streamRingbufferPut(OS_streamRingBufHndl *hndl, unsigned char *buffer, uint32_t size);

//从缓冲区丢弃数据
uint32_t OS_streamRingbufferThrow(OS_streamRingBufHndl *hndl, uint32_t size);



/**************************************************/



#ifdef __cplusplus
}
#endif
#endif //OSA_DOUBLE_RING_BUFFER_H_

