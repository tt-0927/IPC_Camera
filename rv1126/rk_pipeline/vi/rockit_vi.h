/*** 
 * @FilePath     : rockit_vi.h
 * @Author       : luoyk 
 * @Date         : 2022-06-27 14:51:53
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-12 17:04:08
 * @Description  : 
 */

#ifndef _ROCKIT_VI_H
#define _ROCKIT_VI_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <sys/poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "rk_debug.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_sys.h"
#include "rk_common.h"
#include "rk_comm_vi.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_gdc.h"


/*用户图片信息 建议yuv*/
typedef struct _VIUSERPIC_{
    
    int unWidth;
    int unHeight;
    PIXEL_FORMAT_E ePixFmt;
    char* pData;

}ViUserPic_S;

/*rk vi必需参数*/
typedef struct _RKVINEEDPARAM_S{

    /*设备ID*/
    int nDevId;
    /*通道号*/
    int nChannel;
    /*通道设备名*/
    char aEnityName[64];
    /*GDC文件路径*/
    char sGdcFecFile[128];
    int nWidth;
    int nHeight;
    /*图像格式*/
    PIXEL_FORMAT_E enPixelFormat;
    /*压缩模式*/
    COMPRESS_MODE_E enCompressMode;

    /*内存类型*/
    VI_V4L2_MEMORY_TYPE enMemType;

}RkViNeedParam_S;

typedef struct _RKVIEXPARAM_S{

    int nPipeId;
    /*帧率控制  默认 -1*/
    int nSrcFrameRate;
    int nDstFrameRate;
    /*获取图像的队列深度*/
    int nDepth;

    /*输出通道的缓冲块数*/
    int nBufCount;

    /*缩放宽高*/
    int nScaWidth;
    int nScaHeight;

}RkViExParam_S;

typedef struct _RKVI_S RkVi_S;
struct _RKVI_S{
    
    /*获取采集帧*/
    int (*rockitVi_getChnFrame)( RkVi_S* pHandle, int nChn,  VIDEO_FRAME_INFO_S* pFrame, int nTimeoutMs );
    
    /*释放采集帧*/
    int (*rockitVi_releaseChnFrame)( RkVi_S* pHandle, int nChn,  VIDEO_FRAME_INFO_S* pFrame );
 
    /*禁用通道*/
    int (*rockitVi_disable_chn) ( RkVi_S* pHandle );
    
    /*启用通道*/
    int (*rockitVi_enable_chn) ( RkVi_S* pHandle );
    
    /*暂停通道*/
    int (*rockitVi_pause_chn) ( RkVi_S* pHandle );
    
    /*恢复通道*/
    int (*rockitVi_resume_chn) ( RkVi_S* pHandle );
    
    /*设置vi的属性*/
    int (*rockitVi_set_attr)( RkVi_S* pHandle, RkViNeedParam_S stNeedParam, RkViExParam_S stExParam  );

    /*初始化vi*/
    int (*rockitVi_init) ( RkVi_S* pHandle );

    /*反初始化vi*/
    int (*rockitVi_uninit) ( RkVi_S* pHandle );

    /**********************必需参数***************************/
    RkViNeedParam_S stNeedParam;
    RkViExParam_S stExParam;
    /*辅助参数*/
    int nFd;
};

/*分配vi句柄*/
RkVi_S* rockitVi_alloc( RkViNeedParam_S stNeedParam );

/*释放vi句柄*/
int rockitVi_release( RkVi_S*pHandle );


#ifdef __cplusplus
}
#endif
#endif
