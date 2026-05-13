

#ifndef OS_SHARE_CORE_SOURCE_BUFFER_REF_INCLUDE_
#define OS_SHARE_CORE_SOURCE_BUFFER_REF_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "os_atom.h"

/* 内存数据释放的回调函数 */
typedef void (*bufferRefFreeInterface)(void *user, unsigned char *data);


typedef struct BufferInner
{
    /* 计数引用 */
    os_atomic_uint refcount;

    unsigned char *data;
    int size;

    bufferRefFreeInterface freeInterface;
    void *user;
}bufferInner_S;


typedef struct BufferRef_
{
	bufferInner_S* buffer;
}bufferRefHndl_S;


/* 创建引用缓冲区
 * data：内存数据
 * size:内存大小
 * free：内存无人引用后，释放的回调函数
 * user:回调函数带入的用户参数
 * return：成功返回引用缓冲区句柄，返回NULL，创建失败
 *  */
bufferRefHndl_S *bufferRef_create(unsigned char *data, int size, \
							bufferRefFreeInterface freeInterface, \
                              void *user);

/* 引用数据
 * buf：引用的数据缓冲区句柄
 * return:成功返回引用缓冲区句柄，返回NULL，引用失败
 * */
bufferRefHndl_S *bufferRef_ref(bufferRefHndl_S *buf);

/* 解引用数据
 * buf：引用的数据缓冲区句柄
 * return:无
 * */
void bufferRef_unref(bufferRefHndl_S **buf);





#ifdef __cplusplus
}
#endif

#endif //OS_SHARE_CORE_SOURCE_BUFFER_REF_INCLUDE_

