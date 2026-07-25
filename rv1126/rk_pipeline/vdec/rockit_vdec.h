/*************************************************************************
	> File Name: rockit_vdec.h
	> Author: 
	> Mail: 
	> Created Time: Tue 10 May 2022 09:07:18 AM CST
 ************************************************************************/

#ifndef _ROCKIT_VDEC_H
#define _ROCKIT_VDEC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/types.h>
#include "rk_debug.h"
#include "rk_mpi_vdec.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_cal.h"
#include "rk_comm_video.h"
#include "rk_mpi_vo.h"

/*解码句柄*/
typedef struct _RkVdec RkVdec_S;

/*用户图片信息 建议rgb888/nv12 ,若绑定后级模块看后级模式支持的格式*/
typedef struct _USERPIC_{
    
    int unWidth;
    int unHeight;
    PIXEL_FORMAT_E ePixFmt;
    char* pData;

}UserPic_S;

/*发送数据到解码的结构体*/
typedef struct _RkMediaData
{
    /*解码句柄*/
    RkVdec_S* pHandle;
    
    /*要解码数据*/
    uint8_t* pData;
    
    /*数据大小*/
    int nSize;

    /*释放数据的回调函数，空 自己外部释放  非空 解码内部自动释放*/
    RK_MPI_MB_FREE_CB pFreeFunCB;
    
    /*最后一帧标志位 0 不是  1是*/
    RK_BOOL bEos;

    /*超时时间 -1 阻塞  0 非阻塞  >0 超时*/
    int nTimeMs;
    uint64_t nPts;

}RkMediaData_S;


/*解码必需参数*/
typedef struct _RKVDECNEEDPARAM_{
    
    unsigned int unSrcWidth;
    unsigned int unSrcHeight;
    
    /*解码通道*/
    int          nChnIndex;
    
    /*解码格式ID*/
    RK_CODEC_ID_E enCodecId;
    
    /*流的输入模式*/
    VIDEO_MODE_E         eInputMode;
    
    /*解码后格式*/
    PIXEL_FORMAT_E eOutputPixFmt;
    
    /*压缩模式 false: 不启用  true: 启用*/
    COMPRESS_MODE_E eCompressMode;

}RkVdecNeedParam_S;

/*解码功能参数*/
typedef struct _RKVDECEXPARAM_{
    
    /*解码的内存块个数 默认4个*/
    unsigned int unFrameBufferCnt;
    
    /*发送码流缓冲区存储的码流包个数 默认4个*/
    unsigned int unSendBufferCnt;
    
    /*只有使用用户模式才有用 默认false 私有模式  */
    RK_BOOL bEnableMbPool;
    
    /*隔行扫描 默认是false*/
    RK_BOOL     bEnableDei;
    
    /*h264要不要解b帧,H.265 码流支持时域运动矢量预测 默认是false*/
    RK_BOOL     bEnableColmv;
    
    /*显示模式 默认是回显模式VIDEO_DISPLAY_MODE_PLAYBACK */
    VIDEO_DISPLAY_MODE_E eDisPlayMode;

}RkvdecExParam_S;

struct _RkVdec
{

    /*发送数据到解码
    * 当pFreeFunCB为空时，解码会进行一次拷贝，用户要自己手动释放data
    * 当pFreeFunCB不为空是，解码仅引用data，内部会自行调用释放回调
    * 为避免出现 RK_ERR_VDEC_BUF_FULL 错误，建议将超时时间设置为通道帧间隔的4倍
    */
    int (*rockitVdec_send_stream) (RkMediaData_S* pMediaData);
    
    /*获取解码后的缓存块*/
    int (*rockitVdec_get_frame) (RkVdec_S* pHandle, VIDEO_FRAME_INFO_S *pstFrame, int nTimeout);
    
    /*获取缓存块的虚拟地址*/
    int (*rockitVdec_get_frameVir) (VIDEO_FRAME_INFO_S* pstFrame, uint8_t** pData, int nSize);
    
    /*释放解码后的缓冲块*/
    int (*rockitVdec_release_frame) (RkVdec_S* pHandle, VIDEO_FRAME_INFO_S* pstFrame);
    
    /* 重置解码器通道队列数据 */
    int (*rockitVdec_reset_vdecChn) (RkVdec_S* pHandle);


    /*获取解码通道属性*/
    int (*rockitVdec_get_chnAttr)( RkVdec_S* pHandle, VDEC_CHN_ATTR_S* pstAttr, VDEC_CHN_PARAM_S *pstParam );
    
    /*改变解码通道属性*/
    int (*rockitVdec_set_chnAttr)( RkVdec_S* pHandle, VDEC_CHN_ATTR_S* pstAttr, VDEC_CHN_PARAM_S *pstParam );
    
    /*改变解码通道宽高*/
    int (*rockitVdec_change_wh)( RkVdec_S* pHandle, unsigned int unWidth, unsigned int unHeight);

    /*改变解码通道的解码格式*/
    int (*rockitVdec_change_codecId)( RkVdec_S* pHandle, RK_CODEC_ID_E eCodecId );

    /*设置插入的图片*/
    int (*rockitVdec_set_userPic)( RkVdec_S* pHandle, UserPic_S* pUserPic  );

    /*显示插入用户图片
    *inparam pHandle 句柄
    *inparam bInstant 1 立即插入， 0 延迟插入
    * */
    int (*rockitVdec_show_userPic)( RkVdec_S* pHandle, RK_BOOL bInstant );
    
    int (*rockitVdec_disShow_userPic)( RkVdec_S* pHandle );

    /*初始化解码句柄*/
    int (*rockitVdec_init) (RkVdec_S *pHandle);
    
    /*反初始化解码句柄*/
    int (*rockitVdec_uninit) (RkVdec_S *pHandle);
    
    
/********************必需参数*********************/
    RkVdecNeedParam_S stNeedParam;

/********************功能参数*********************/
    RkvdecExParam_S   stExParam;

/********************辅助参数*********************/

    /*解码通道的句柄 可以使用select*/
    int          nChnFd;
    MB_POOL     nPool;
};



/*分配一个解码句柄*/
RkVdec_S* rockitVdec_alloc( RkVdecNeedParam_S stParam );

/*释放一个解码句柄*/
void rockitVdec_release(RkVdec_S* pHandle);

#ifdef __cplusplus
}
#endif
#endif
