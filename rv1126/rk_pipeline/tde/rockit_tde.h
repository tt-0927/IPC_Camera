/*************************************************************************
	> File Name: rockit_tde.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月27日 星期五 14时25分21秒
 ************************************************************************/

#ifndef _ROCKIT_TDE_H
#define _ROCKIT_TDE_H

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "rk_debug.h"
#include "rk_mpi_mb.h"
#include "rk_comm_tde.h"
#include "rk_comm_video.h"
#include "rk_mpi_tde.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_cal.h"

typedef struct  _TDEPICINFO_ {
    
/*********************************必填参数**************************/
    /*区域信息*/
    int nX;
    int nY;
    int nW;
    int nH;

    /*图像的宽*/
    unsigned int unWidth;
    
    /*图像的高*/
    unsigned int unHeight;
    
    /*压缩模式 默认不压缩*/
    COMPRESS_MODE_E enCompressMode;
    
    /*图像格式*/
    PIXEL_FORMAT_E enPixFormat;

/***********************************选填参数***********************/
    /*MB的内存块*/
    MB_BLK Blk;
    /*内存块的虚拟地址*/
    char* pData;
    /*内存块的大小*/
    int nBuffsize;

}TdePicInfo_S;



/*开启TDE*/
int rkTde_open();

/*关闭TDE*/
int rkTde_close();

/*分配存储空间*/
int rkTde_alloc_pic( TdePicInfo_S* pstPicInfo );

/*用户空间的数据填充到MBK
 *
 *如果pCallBack为空, 拷贝数据的方式就是memcpy
 *
 * 如果不为空，就是调用pCallBack的方式进行拷贝 pData就是用户传入回调的pParam
 *比如你可以读取文件，这样就能减少一次拷贝
 *
 * */
int rkTde_load_data( TdePicInfo_S* pstPicInfo, void* pData, int nSize, int (*pCallBack)( char* pData, void* pParam), void* pParam );

/*释放空间*/
int rkTde_relese_pic( TdePicInfo_S* pstPicInfo );

TDE_HANDLE rkTde_createJob();
int rkTde_endJob( TDE_HANDLE tdeHandle );
int rkTde_copy_task( TDE_HANDLE tdeHandle, TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic  );
/*拷贝*/
int rkTde_copy( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic );

/*缩放*/
int rkTde_resize_task( TDE_HANDLE tdeHandle, TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic  );
/*缩放*/
int rkTde_resize( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic );

/*填充*/
int rkTde_full( TdePicInfo_S* pDstPic , unsigned unFillData);

/*旋转*/
int rkTde_rotate( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic, ROTATION_E enRotateAngle );

/*镜像, 抠图，融合*/
int rkTde_bitblit( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic, TDE_OPT_S* pstOpt );

/* 填充 */
int rkTde_full_task( TDE_HANDLE tdeHandle, TdePicInfo_S* pDstPic, unsigned int unFillData  );

#endif
