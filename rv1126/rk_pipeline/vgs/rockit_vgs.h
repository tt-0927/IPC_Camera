/*************************************************************************
	> File Name: rockit_tde.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月27日 星期五 14时25分21秒
 ************************************************************************/

#ifndef _ROCKIT_VGS_H
#define _ROCKIT_VGS_H

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "rk_debug.h"
#include "rk_mpi_mb.h"
#include "rk_comm_video.h"
#include "rk_mpi_vgs.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_cal.h"

typedef struct  _VGSFRAME_S_ {
    
/*********************************必填参数**************************/

    /*图像的宽*/
    unsigned int unWidth;
    unsigned int unVirWidth;
    
    /*图像的高*/
    unsigned int unHeight;
    unsigned int unVirHeight;
    
    /*压缩模式 默认不压缩*/
    COMPRESS_MODE_E enCompressMode;
    
    /*图像格式*/
    PIXEL_FORMAT_E ePixFormat;

/***********************************选填参数***********************/
    /*MB的内存块 为NULL时会自动分配*/
    MB_BLK Blk;
    char* pData;
    int nBuffsize;
}VgsFrame_S;

int rockitVgs_alloc_blk( VgsFrame_S* pstPicInfo );
int rockitVgs_relese_blk( VgsFrame_S* pstPicInfo );

/*缩放*/
int rockitVgs_scale( VgsFrame_S* pSrcPic, VgsFrame_S* pDstPic);
int rockitVgs_scaleTask( VgsFrame_S* pSrcPic, VgsFrame_S* pDstPic);

/*打OSD*/
int rockitVgs_osd( VgsFrame_S* pSrcPic, RECT_S* pstSrcRect, VgsFrame_S* pDstPic);

/* 批量添加双OSD叠加（使用RK_MPI_VGS_AddOsdTaskArray函数） */
int rockitVgs_osd_double(VgsFrame_S* pSrcPic1, RECT_S* pstRect1,
                           VgsFrame_S* pSrcPic2, RECT_S* pstRect2,
                           VgsFrame_S* pDstPic);

/*裁剪*/
int rockitVgs_crop( VgsFrame_S* pSrcPic, RECT_S stRect, VgsFrame_S* pDstPic);

/*马赛克*/
int rockitVgs_mosaic(VgsFrame_S *pSrcPic, RECT_S stRect, VgsFrame_S *pDstPic);

#endif
