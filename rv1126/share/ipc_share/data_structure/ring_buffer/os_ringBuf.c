
#include <stdio.h>
#include "os_ringBuf.h"
#define OS_SOK      0  ///< Status : OK
#define OS_EFAIL   -1  ///< Status : Generic error
#define TRUE 1
#define FALSE 0


int OS_ringBufCreate(OS_ringBufHndl *hndl, int buf_num)
{
    int status = OS_SOK;
    int i = 0;
    if(hndl == NULL)
        return OS_EFAIL;

    if (buf_num > OS_RING_BUF_NUM_MAX)
        return OS_EFAIL;
    memset(hndl, 0, sizeof(OS_ringBufHndl));

    status = OS_queCreate(&hndl->emptyQue, buf_num);
    if(status != OS_SOK)
    {
        printf("OS_bufCreate() = %d \r\n", status);
        return status;
    }
    status = OS_queCreate(&hndl->fullQue, buf_num);
    if (status != OS_SOK)
    {
        OS_queDelete(&hndl->emptyQue);
        printf("OS_bufCreate() = %d \r\n", status);
        return status;
    }
    hndl->numBuf = buf_num;
    for (i = 0; i < hndl->numBuf; i++)
    {
        hndl->bufInfo[i] = NULL;
        OS_quePut(&hndl->emptyQue, i, -1);
    }
    return status;
}

int OS_ringBufInitBufInfo(OS_ringBufHndl *hndl, int64_t bufId, void *buffInfo)
{
    int status = OS_SOK;
    if(hndl == NULL)
    {
        printf("this argument is null!!\n");
        return OS_EFAIL;
    }
    if (bufId >= hndl->numBuf || bufId < 0)
    {
        printf("1 this buffid[%lld] > numBuf[%lld]!!!\n",bufId,hndl->numBuf);
        return OS_EFAIL;
    }
    hndl->bufInfo[bufId] = buffInfo;
    return status;
}

int OS_ringBufDelete(OS_ringBufHndl *hndl)
{
    int status = OS_SOK;
    if(hndl == NULL)
    {
        printf("this argument is null!!\n");
        return OS_EFAIL;
    }

    status = OS_queDelete(&hndl->emptyQue);
    status |= OS_queDelete(&hndl->fullQue);
    return status;
}

int OS_ringBufGetFull(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout)
{
    int status = OS_SOK;
    if (hndl == NULL || bufId == NULL)
    {
        printf("this arugment is null!!\n");
        return OS_EFAIL;
    }

    status = OS_queGet(&hndl->fullQue, bufId, timeout);
    if (status != OS_SOK)
    {
        *bufId = OS_RING_BUF_ID_INVALID;
    }
    return status;
}

int OS_ringBufPutEmpty(OS_ringBufHndl *hndl, int64_t bufId, uint32_t timeout)
{
    int status = OS_SOK;
    if(hndl == NULL)
    {
        printf("this argument is null!!!\n");
        return OS_EFAIL;
    }
    if (bufId >= hndl->numBuf || bufId < 0)
    {
        printf("2 this buffid[%lld] > numBuf[%lld]!!!\n",bufId,hndl->numBuf);
        return OS_EFAIL;
    }

    status = OS_quePut(&hndl->emptyQue, bufId, timeout);
    return status;
}

int OS_ringBufGetEmpty(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout)
{
    int status = OS_SOK;
    if(hndl == NULL || bufId == NULL)
    {
        printf("this argument is null111!!\n");
        return OS_EFAIL;
    }

    status = OS_queGet(&hndl->emptyQue, bufId, timeout);
    if (status != OS_SOK)
    {
        *bufId = OS_RING_BUF_ID_INVALID;
    }
    return status;
}

int OS_ringBufPutFull(OS_ringBufHndl *hndl, int64_t bufId, uint32_t timeout)
{
    int status = OS_SOK;
    if(hndl == NULL)
    {
        printf("this argument is null!!!\n");
        return OS_EFAIL;
    }

    if (bufId >= hndl->numBuf || bufId < 0)
    {
        printf("3 this buffid[%lld] > numBuf[%lld]!!!\n",bufId,hndl->numBuf);
        return OS_EFAIL;
    }

    status = OS_quePut(&hndl->fullQue, bufId, timeout);
    return status;
}

int OS_ringBufSwitchFull(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout)
{
    int status = OS_SOK;
    int64_t newBufId = 0;
    status = OS_ringBufGetEmpty(hndl, &newBufId, 0);

    if (status == OS_SOK)
    {
        if(*bufId != OS_RING_BUF_ID_INVALID)
        {
            OS_ringBufPutFull(hndl, *bufId, timeout);
        }
        *bufId = newBufId;
    }
    return status;
}

int OS_ringBufSwitchEmpty(OS_ringBufHndl *hndl, int64_t *bufId, uint32_t timeout)
{
    int status = OS_SOK;
    int64_t newBufId = 0;
    status = OS_ringBufGetFull(hndl, &newBufId, 0);
    if (status==OS_SOK)
    {
        if (*bufId != OS_RING_BUF_ID_INVALID)
        {
            OS_ringBufPutEmpty(hndl, *bufId, timeout);
        }
        *bufId = newBufId;
    }
    return status;
}

void *OS_ringBufGetBufInfo(OS_ringBufHndl *hndl, int64_t bufId)
{
    if (hndl == NULL)
    {
        printf("this argument is null!!!\n");
        return NULL;
    }

    if (bufId >= hndl->numBuf)
    {
        printf("4 this buffid[%lld] > numBuf[%lld]!!!\n",bufId,hndl->numBuf);
        return NULL;
    }

    return hndl->bufInfo[bufId];
}

uint32_t OS_ringBufFullCount(OS_ringBufHndl *hndl)
{
    if (hndl == NULL)
    {
        printf("this argument is null!!!\n");
        return 0;
    }
    return OS_queGetQueuedCount(&hndl->fullQue);
}

uint32_t OS_ringBufEmptyCount(OS_ringBufHndl *hndl)
{
    if (hndl == NULL)
    {
        printf("this argument is null!!!\n");
        return 0;
    }
    return OS_queGetQueuedCount(&hndl->emptyQue);
}

int OS_ringBufGetFullPeek(OS_ringBufHndl *hndl, int64_t *bufId)
{
    int status = OS_SOK;
    if (hndl == NULL || bufId == NULL)
    {
        printf("this arugment is null!!\n");
        return OS_EFAIL;
    }

    status = OS_quePeek(&hndl->fullQue, bufId);
    if (status != OS_SOK)
    {
        *bufId = OS_RING_BUF_ID_INVALID;
    }
    return status;
}


/***********************数据流，环形缓冲区**********************************/


//判断x是否是2的次方
#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
//取a和b中最小值
#define min(a, b) (((a) < (b)) ? (a) : (b))


int OS_streamRingbufferCreate(OS_streamRingBufHndl *hndl, uint32_t size)
{
    int status = OS_SOK;
    if(hndl == NULL)
        return OS_EFAIL;

    if (!is_power_of_2(size))
    {
        printf("size must be power of 2.\n");
        return OS_EFAIL;
    }

    memset(hndl, 0, sizeof(OS_streamRingBufHndl));
    hndl->buffer = (unsigned char*)malloc(size);
    if(hndl->buffer == NULL)
    {
        printf("malloc error!!!\n");
        return -1;
    }

    hndl->size = size;
    hndl->in = 0;
    hndl->out = 0;

    pthread_mutexattr_t mutex_attr;
    status |= pthread_mutexattr_init(&mutex_attr);
    status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
    if(status != OS_SOK)
    {
        printf("pthread_mutex_init() = %d \r\n", status);
    }
    pthread_mutexattr_destroy(&mutex_attr);
    hndl->init = 1;
    return status;
}

int OS_streamRingbufferDelete(OS_streamRingBufHndl *hndl)
{
    if(hndl == NULL)
    {
        printf("this argument is null!!!\n");
        return -1;
    }
    if(hndl->buffer)
    {
        free(hndl->buffer);
        hndl->buffer = NULL;
    }
    hndl->size = 0;
    hndl->in = 0;
    hndl->out = 0;
    pthread_mutex_destroy(&hndl->lock);
    return 0;
}

uint32_t OS_streamRingbufferLen(OS_streamRingBufHndl *hndl)
{
    if((hndl == NULL) || (hndl->init != 1))
    {
        printf("this arugment is null!!!\n");
        return 0;
    }

    uint32_t len = 0;
    pthread_mutex_lock(&(hndl->lock));
    len = (hndl->in - hndl->out);
    pthread_mutex_unlock(&(hndl->lock));
    return len;
}


//从缓冲区中取数据长度
static uint32_t __ring_buffer_get_len(OS_streamRingBufHndl *ring_buf)
{
    if(ring_buf == NULL)
    {
        printf("this argument is null!!!\n");
        return 0;
    }
    return ring_buf->in - ring_buf->out;
}

//从缓冲区中取数据长度
uint32_t OS_streamRingbufferGetLen(OS_streamRingBufHndl *hndl)
{
    if(((hndl == NULL) || (hndl->init != 1)))
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    uint32_t ret = 0;
    pthread_mutex_lock(&(hndl->lock));
    ret = __ring_buffer_get_len(hndl);
    //buffer中没有数据
    if (hndl->in == hndl->out)
    {
        hndl->in = hndl->out = 0;
    }
    pthread_mutex_unlock(&(hndl->lock));

    return ret;
}

//从缓冲区中取数据
static uint32_t __ring_buffer_get(OS_streamRingBufHndl *ring_buf, unsigned char * buffer, uint32_t size)
{
    if(ring_buf == NULL || (buffer == NULL))
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    uint32_t len = 0;
    size = min(size, ring_buf->in - ring_buf->out);

    /* first get the data from fifo->out until the end of the buffer */
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1)), len);

    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    return size;
}

uint32_t OS_streamRingbufferGet(OS_streamRingBufHndl *hndl, unsigned char *buffer, uint32_t size)
{
    if(((hndl == NULL) || (hndl->init != 1)) || (buffer == NULL))
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    uint32_t ret = 0;
    pthread_mutex_lock(&(hndl->lock));
    ret = __ring_buffer_get(hndl, buffer, size);
    //buffer中没有数据
    if (hndl->in == hndl->out)
    {
        hndl->in = hndl->out = 0;
    }
    pthread_mutex_unlock(&(hndl->lock));
    return ret;
}

//向缓冲区中存放数据
static uint32_t __ring_buffer_put(OS_streamRingBufHndl *ring_buf, unsigned char *buffer, uint32_t size)
{
    if(ring_buf == NULL || (buffer == NULL))
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    uint32_t len = 0;
    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    len = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
    memcpy(ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer, len);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, buffer + len, size - len);
    ring_buf->in += size;
    return size;
}

uint32_t OS_streamRingbufferPut(OS_streamRingBufHndl *hndl, unsigned char *buffer, uint32_t size)
{
    if(((hndl == NULL) || (hndl->init != 1)) || (buffer == NULL))
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    uint32_t ret = 0;
    pthread_mutex_lock(&(hndl->lock));
    ret = __ring_buffer_put(hndl, buffer, size);
    pthread_mutex_unlock(&(hndl->lock));
    return ret;
}

//从缓冲区丢弃数据
static uint32_t __ring_buffer_throw(OS_streamRingBufHndl *ring_buf, uint32_t size)
{
    if(ring_buf == NULL)
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    size  = min(size, ring_buf->in - ring_buf->out);
    ring_buf->out += size;
    return size;
}

uint32_t OS_streamRingbufferThrow(OS_streamRingBufHndl *hndl, uint32_t size)
{
    if(((hndl == NULL) || (hndl->init != 1)))
    {
        printf("this argument is null!!!\n");
        return 0;
    }

    uint32_t ret;
    pthread_mutex_lock(&(hndl->lock));
    ret = __ring_buffer_throw(hndl, size);
    //buffer中没有数据
    if (hndl->in == hndl->out)
    {
        hndl->in = hndl->out = 0;
    }
    pthread_mutex_unlock(&(hndl->lock));
    return ret;
}
