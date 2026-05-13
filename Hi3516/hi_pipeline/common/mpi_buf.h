/**
 * @FilePath     : mpi_buf.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 11:08:14
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-03 11:26:33
 * @Description  : 数据buf结构体定义
 */

#ifndef _MPI_BUF_H_
#define _MPI_BUF_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>
#include "ot_type.h"

/*MPI Buf数据结构体定义*/
typedef struct MpiBuf
{
    td_uchar *pData; // 数据指针
    td_u32 u32Len;   // 实际数据长度
    td_u32 u32Size;  // 数据指针空间大小
} MpiBuf_S;

int mpiBuf_init(MpiBuf_S * pMpiBuf,td_u32 u32Size);

void mpiBuf_uninit(MpiBuf_S *pMpiBuf);

#ifdef __cplusplus
}
#endif
#endif