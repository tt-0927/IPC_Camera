/**
 * @FilePath     : mpi_buf.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 11:13:30
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-03 11:24:16
 * @Description  : 数据buf结构体封装
 */

#include "mpi_buf.h"
#include "mpi_common.h"

int mpiBuf_init(MpiBuf_S * pMpiBuf,td_u32 u32Size)
{
    pMpiBuf->pData = (td_uchar *)calloc(u32Size, sizeof(td_uchar));
    if (pMpiBuf->pData == NULL)
    {
        mpi_log("内存分配失败");
        return TD_FAILURE;
    }
    pMpiBuf->u32Size = u32Size;
    pMpiBuf->u32Len = TD_NULL;

    return TD_SUCCESS;
}

void mpiBuf_uninit(MpiBuf_S *pMpiBuf)
{
    if (pMpiBuf->pData)
    {
        free(pMpiBuf->pData);
        pMpiBuf->pData = NULL;
    }
    pMpiBuf->u32Size = TD_NULL;
    pMpiBuf->u32Len = TD_NULL;
}