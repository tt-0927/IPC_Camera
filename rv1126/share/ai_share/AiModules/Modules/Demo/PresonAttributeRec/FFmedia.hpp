/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-28
 * 
 * @brief 接流或者读取视频
 */


#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include <chrono>
#include <thread>
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <net/if.h> //struct ifreq
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/mii.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavcodec/avcodec.h>

#include "rk_debug.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_vo.h"
#include "rk_mpi_vdec.h"
#include "rk_mpi_vpss.h"
#include "rk_mpi_mb.h"
#include "rk_common.h"
#include "rk_comm_video.h"
#include "rk_comm_vpss.h"
#include "rockit_vdec.h"
#include "rockit_bind.h"
#include "rockit_venc.h"

#include "os_thr.h"
#include "os_que.h"
#include "media_ffmpeg.h"
};

#define ALIGN_UP(x, a)              ((x+a-1)&(~(a-1)))
#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))

// // yuv
// #define TEST_FMT_VO   RK_FMT_YUV420SP
// #define TEST_FMT_VPSS RK_FMT_YUV420SP

// bgr
/*#define TEST_FMT_VO   RK_FMT_BGR888
#define TEST_FMT_VPSS RK_FMT_BGR888*/

// rgb
#define TEST_FMT_VO   RK_FMT_RGB888
#define TEST_FMT_VPSS RK_FMT_RGB888

#define CMD_RESET_LT9611UXC _IO('L', 0)


//分屏模式定义
typedef enum
{
    /*单屏*/
    VO_MODE_1MUX,
    /*二分屏*/
    VO_MODE_2MUX,
    /*四分屏*/
    VO_MODE_4MUX,
    /*九分屏*/
    VO_MODE_9MUX,
    /*16分屏*/
    VO_MODE_16MUX,

}VO_MODE_E;

typedef struct
{
    uint32_t nWidth;                //图像宽
    uint32_t nHeight;               //图像高
    uint32_t nVirWidth;             //图像虚宽
    uint32_t nVirHeight;            //图像虚高
    PIXEL_FORMAT_E nPixelFormat;    //图像像素格式
    uint32_t nSize;                 //数据大小
    void *pData;                    //数据地址

}VideoFrame_S;

RK_BOOL g_bExit = RK_FALSE;

VO_LAYER g_nVoLayer = 0;
int g_nVencChn = 0;
RkVenc_S* g_pEncHandle = NULL;
/*保存ffmpeg读取到的视频队列*/
OS_QueHndl g_MediaQue;
OS_QueHndl g_videoQue;
OS_ThrHndl g_getMediaThrId;
OS_ThrHndl g_sendDecThrId;
OS_ThrHndl g_getVideoThrId;
OS_ThrHndl g_videoFrameHandlThrId;
OS_ThrHndl g_getVencVideoThrId;

/*媒体句柄*/
mediaFfmpeg_t *g_pMediaHandle;
/*视频解码句柄*/
RkVdec_S *g_pVideoHandle;

void cleanUp(int sig);

/*
*@description: 根据分屏模式使能VO通道
*@Author: cds
*@param[in]: nVoLayer 视频层
*@param[in]: nVoMode 分屏模式
*@return: 成功返回开启的通道个数，失败返回-1
*/
RK_S32 voStartChn(VO_LAYER nVoLayer,VO_MODE_E nVoMode);

/*
*@description: 根据分屏模式禁用VO通道
*@Author: cds
*@param[in]: nVoLayer 视频层
*@param[in]: nVoMode 分屏模式
*@return: 0成功 其他值失败
*/
RK_S32 voStopChn(VO_LAYER nVoLayer, VO_MODE_E nVoMode);

/*
*@description: VO输出口初始化
*@Author: cds
*@param[in]: nPort 1:HDMI0 2:HDMI1 3:MIPI0 4:MIPI1
*@param[in]: nVoDev 0~3 输出设备号
*@param[in]: nVoLayer 0~7 图层号
*@param[in]: nLayerMode 0:鼠标层 1：图形层 3：视频层 4：虚拟层 
*@param[in]: nResolution 0:1080P 1:4K 2:720P
*@return: 0成功 其他值失败
*/
int voPortInit(int nPort,VO_DEV nVoDev,VO_LAYER nVoLayer,VO_LAYER_MODE_E nLayerMode,int nResolution);

/*
*@description: VO输出口关闭
*@Author: cds
*@param[in]: nVoDev 0~3 输出设备号
*@param[in]: nVoLayer 0~7 图层号
*@return: 0成功 其他值失败
*/
int voPortUninit(VO_DEV nVoDev,VO_LAYER nVoLayer);

static int vdec_init(int nVdecChnId);

static int vdec_uninit();

static int vpssInit(VPSS_GRP nVpssGrp,RK_U32 nWidth,RK_U32 nHeight);

static int vpssUnInit();

static RK_S32 vdec_free(void *pParam);

static int vencInit(int nVencChn);

/*从VENC获取编码视频*/
static void* getStreamThr(void* param);

/*获取视频送队列线程*/
static void *getVideoThr(void *pParam);

/*读取视频队列送解码线程*/
static void *sendDecThr(void *pParam);

/*读取媒体线程*/
void* getMediaThr(void*arg);

/*  随机根据ID生成颜色 */
cv::Scalar getColor(int nIdx);

void cleanUp(int sig)
{
    g_bExit = RK_TRUE;
}

/*
*@description: 根据分屏模式使能VO通道
*@Author: cds
*@param[in]: nVoLayer 视频层
*@param[in]: nVoMode 分屏模式
*@return: 成功返回开启的通道个数，失败返回-1
*/
RK_S32 voStartChn(VO_LAYER nVoLayer,VO_MODE_E nVoMode)
{
    RK_S32 i;
    RK_S32 nRtv = -1;
    RK_S32 nRet;
    RK_S32 nWndNum = 0;         //画面个数
    RK_U32 nSquare = 0;
    RK_U32 nWidth = 0;
    RK_U32 nHeight = 0;
    VO_CHN_ATTR_S stChnAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_PARAM_S stChnParam;
    VO_BORDER_S stBorder;

    switch (nVoMode)
    {
        case VO_MODE_1MUX:
            nWndNum = 1;
            nSquare = 1;
            break;
        case VO_MODE_4MUX:
            nWndNum = 4;
            nSquare = 2;
            break;
        case VO_MODE_9MUX:
            nWndNum = 9;
            nSquare = 3;
            break;
        case VO_MODE_16MUX:
            nWndNum = 16;
            nSquare = 4;
            break;
        case VO_MODE_2MUX:
            nWndNum = 2;
            break;
        default:
            printf("VO mode error\n");
            return RK_FAILURE;
    }

    nRet = RK_MPI_VO_GetLayerAttr(nVoLayer, &stLayerAttr);//获取通道属性
    if(nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_GetLayerAttr fail %d \n",nRet);
        return RK_FAILURE;
    }

    nWidth = stLayerAttr.stImageSize.u32Width;
    nHeight = stLayerAttr.stImageSize.u32Height;

    if(nVoMode == VO_MODE_2MUX)
    {
        for(i=0;i<nWndNum;i++)
        {
            stChnAttr.u32Priority = i;
            stChnAttr.stRect.s32X = ALIGN_BACK(0 + (i * nWidth/nWndNum),2);
            stChnAttr.stRect.s32Y = ALIGN_BACK(nHeight/4,2);
            stChnAttr.stRect.u32Width = ALIGN_BACK(nWidth/nWndNum,2);
            stChnAttr.stRect.u32Height = ALIGN_BACK(nHeight/2,2);
            stChnAttr.u32FgAlpha = 0;
            stChnAttr.u32BgAlpha = 0;

            /*设置通道属性*/
            nRet = RK_MPI_VO_SetChnAttr(nVoLayer, i, &stChnAttr);
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_SetChnAttr fail %x %d\n",nRet,i);
                return RK_FAILURE;
            }

            /*设置通道参数*/
            stChnParam.stAspectRatio.enMode = ASPECT_RATIO_NONE;
            stChnParam.stAspectRatio.stVideoRect.s32Y = stChnAttr.stRect.s32X;
            stChnParam.stAspectRatio.stVideoRect.s32X = stChnAttr.stRect.s32Y; 
            stChnParam.stAspectRatio.stVideoRect.u32Width = stChnAttr.stRect.u32Width;
            stChnParam.stAspectRatio.stVideoRect.u32Height = stChnAttr.stRect.u32Height;
            nRet = RK_MPI_VO_SetChnParam(nVoLayer, i, &stChnParam);
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_SetChnParam fail %x \n",nRet);
                return RK_FAILURE;
            }

            stBorder.stBorder.u32Color = 0xFF0000;//0xFFFAFA;
            stBorder.stBorder.u32TopWidth = 2;
            stBorder.stBorder.u32BottomWidth = 2;
            stBorder.stBorder.u32LeftWidth = 2;
            stBorder.stBorder.u32RightWidth = 2;
            stBorder.bBorderEn = RK_TRUE;
            nRet = RK_MPI_VO_SetChnBorder(nVoLayer, i, &stBorder);//设置边框属性
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_SetChnBorder fail %x \n",nRet);
                return RK_FAILURE;
            }
            nRet = RK_MPI_VO_EnableChn(nVoLayer, i);//使能通道
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_EnableChn fail %x \n",nRet);
                return RK_FAILURE;
            }
        }
    }
    else if(nVoMode == VO_MODE_1MUX)
    {
        stChnAttr.u32Priority = 0;
        stChnAttr.stRect.s32X = 0;
        stChnAttr.stRect.s32Y = 0;
        stChnAttr.stRect.u32Width = 1920;
        stChnAttr.stRect.u32Height = 1080;
        stChnAttr.u32FgAlpha = 0;
        stChnAttr.u32BgAlpha = 0;

        printf("u32Priority:%d\n",stChnAttr.u32Priority);
        printf("s32X:%d\n",stChnAttr.stRect.s32X);
        printf("s32Y:%d\n",stChnAttr.stRect.s32Y);
        printf("u32Width:%d\n",stChnAttr.stRect.u32Width);
        printf("u32Height:%d\n",stChnAttr.stRect.u32Height);

        /*设置通道属性*/
        nRet = RK_MPI_VO_SetChnAttr(nVoLayer, 0, &stChnAttr);
        if (nRet != RK_SUCCESS)
        {
            printf("RK_MPI_VO_SetChnAttr fail %x %d\n",nRet,0);
            return RK_FAILURE;
        }

        /*设置通道参数*/
        stChnParam.stAspectRatio.enMode = ASPECT_RATIO_AUTO;//ASPECT_RATIO_NONE;
        stChnParam.stAspectRatio.stVideoRect.s32Y = stChnAttr.stRect.s32X;
        stChnParam.stAspectRatio.stVideoRect.s32X = stChnAttr.stRect.s32Y; 
        stChnParam.stAspectRatio.stVideoRect.u32Width = stChnAttr.stRect.u32Width;
        stChnParam.stAspectRatio.stVideoRect.u32Height = stChnAttr.stRect.u32Height;
        nRet = RK_MPI_VO_SetChnParam(nVoLayer, 0, &stChnParam);
        if (nRet != RK_SUCCESS)
        {
            printf("RK_MPI_VO_SetChnParam fail %x \n",nRet);
            return RK_FAILURE;
        }

        stBorder.stBorder.u32Color = 0xFF0000;//0xFFFAFA;
        stBorder.stBorder.u32TopWidth = 2;
        stBorder.stBorder.u32BottomWidth = 2;
        stBorder.stBorder.u32LeftWidth = 2;
        stBorder.stBorder.u32RightWidth = 2;
        stBorder.bBorderEn = RK_FALSE;//RK_TRUE;//RK_FALSE;
        nRet = RK_MPI_VO_SetChnBorder(nVoLayer, 0, &stBorder);//设置边框属性
        if (nRet != RK_SUCCESS)
        {
            printf("RK_MPI_VO_SetChnBorder fail %x \n",nRet);
            return RK_FAILURE;
        }

        nRet = RK_MPI_VO_EnableChn(nVoLayer, 0);//使能通道
        if (nRet != RK_SUCCESS)
        {
            printf("RK_MPI_VO_EnableChn fail %x \n",nRet);
            return RK_FAILURE;
        }

        printf("----------------------------RK_MPI_VO_EnableChn %d\n",0);
    }
    else
    {
        for (i=0; i<nWndNum; i++)
        {
            stChnAttr.u32Priority = i;
            stChnAttr.stRect.s32X = ALIGN_BACK((nWidth/nSquare) * (i%nSquare), 2);
            stChnAttr.stRect.s32Y = ALIGN_BACK((nHeight/nSquare) * (i/nSquare), 2);
            stChnAttr.stRect.u32Width = ALIGN_BACK(nWidth/nSquare, 2);
            stChnAttr.stRect.u32Height = ALIGN_BACK(nHeight/nSquare, 2);
            stChnAttr.u32FgAlpha = 0;
            stChnAttr.u32BgAlpha = 0;

            printf("u32Priority:%d\n",stChnAttr.u32Priority);
            printf("s32X:%d\n",stChnAttr.stRect.s32X);
            printf("s32Y:%d\n",stChnAttr.stRect.s32Y);
            printf("u32Width:%d\n",stChnAttr.stRect.u32Width);
            printf("u32Height:%d\n",stChnAttr.stRect.u32Height);

            /*设置通道属性*/
            nRet = RK_MPI_VO_SetChnAttr(nVoLayer, i, &stChnAttr);
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_SetChnAttr fail %x %d\n",nRet,i);
                return RK_FAILURE;
            }

            /*设置通道参数*/
            stChnParam.stAspectRatio.enMode = ASPECT_RATIO_AUTO;//ASPECT_RATIO_NONE;
            stChnParam.stAspectRatio.stVideoRect.s32Y = stChnAttr.stRect.s32X;
            stChnParam.stAspectRatio.stVideoRect.s32X = stChnAttr.stRect.s32Y; 
            stChnParam.stAspectRatio.stVideoRect.u32Width = stChnAttr.stRect.u32Width;
            stChnParam.stAspectRatio.stVideoRect.u32Height = stChnAttr.stRect.u32Height;
            nRet = RK_MPI_VO_SetChnParam(nVoLayer, i, &stChnParam);
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_SetChnParam fail %x \n",nRet);
                return RK_FAILURE;
            }

            stBorder.stBorder.u32Color = 0xFF0000;//0xFFFAFA;
            stBorder.stBorder.u32TopWidth = 2;
            stBorder.stBorder.u32BottomWidth = 2;
            stBorder.stBorder.u32LeftWidth = 2;
            stBorder.stBorder.u32RightWidth = 2;
            stBorder.bBorderEn = RK_TRUE;//RK_FALSE;
            nRet = RK_MPI_VO_SetChnBorder(nVoLayer, i, &stBorder);//设置边框属性
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_SetChnBorder fail %x \n",nRet);
                return RK_FAILURE;
            }

            nRet = RK_MPI_VO_EnableChn(nVoLayer, i);//使能通道
            if (nRet != RK_SUCCESS)
            {
                printf("RK_MPI_VO_EnableChn fail %x \n",nRet);
                return RK_FAILURE;
            }

            printf("RK_MPI_VO_EnableChn %d\n",i);
        }
    }

    return nWndNum;
}

/*
*@description: 根据分屏模式禁用VO通道
*@Author: cds
*@param[in]: nVoLayer 视频层
*@param[in]: nVoMode 分屏模式
*@return: 0成功 其他值失败
*/
RK_S32 voStopChn(VO_LAYER nVoLayer, VO_MODE_E nVoMode)
{
    RK_S32 i;
    RK_S32 nRet = RK_SUCCESS;
    RK_S32 nWndNum = 40;        //画面个数

    switch (nVoMode)
    {
        case VO_MODE_1MUX:
            nWndNum = 1;
            break;
        case VO_MODE_4MUX:
            nWndNum = 4;
            break;
        case VO_MODE_9MUX:
            nWndNum = 9;
            break;
        case VO_MODE_16MUX:
            nWndNum = 16;
            break;
        case VO_MODE_2MUX:
            nWndNum = 2;
            break;
        default:
            printf("VO mode error\n");
            return RK_FAILURE;
    }

    for (i=0; i<nWndNum; i++)
    {
        nRet = RK_MPI_VO_DisableChn(nVoLayer, i);
        if (nRet != RK_SUCCESS)
        {
            printf("RK_MPI_VO_DisableChn failed %#x!\n", nRet);
            return RK_FAILURE;
        }
    }

    return nRet;
}

/*
*@description: VO输出口初始化
*@Author: cds
*@param[in]: nPort 1:HDMI0 2:HDMI1 3:MIPI0 4:MIPI1
*@param[in]: nVoDev 0~3 输出设备号
*@param[in]: nVoLayer 0~7 图层号
*@param[in]: nLayerMode 0:鼠标层 1：图形层 3：视频层 4：虚拟层 
*@param[in]: nResolution 0:1080P 1:4K 2:720P
*@return: 0成功 其他值失败
*/
int voPortInit(int nPort,VO_DEV nVoDev,VO_LAYER nVoLayer,VO_LAYER_MODE_E nLayerMode,int nResolution)
{
    VO_PUB_ATTR_S           stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S   stLayerAttr;
    VO_FRAME_INFO_S         stFrameInfo;
    RK_S32                  nRet = RK_SUCCESS;
    int                     nIntfType;
    int                     nWidth = 1920;
    int                     nHeight = 1080;

    if(nPort == 1)//HDMI0
    {
        nIntfType = VO_INTF_HDMI;
    }
    else if(nPort == 2)//HDMI1
    {
        nIntfType = VO_INTF_HDMI1;
    }
    else if(nPort == 3)//MIPI
    {
        nIntfType = VO_INTF_MIPI;
    }
    else if(nPort == 4)//MIPI1
    {
        nIntfType = VO_INTF_MIPI1;
    }
    else if(nPort == 5)//VGA
    {
        nIntfType = VO_INTF_VGA;
    }
    else if(nPort == 6)//LCD
    {
        nIntfType = VO_INTF_LCD;
    }
    else if(nPort == 7)//LVDS
    {
        nIntfType = VO_INTF_LVDS;
    }
    else if(nPort == 8)//EDP
    {
        nIntfType = VO_INTF_EDP;
    }
    else if(nPort == 9)//EDP1
    {
        nIntfType = VO_INTF_EDP1;
    }
    else if(nPort == 10)//DP
    {
        nIntfType = VO_INTF_DP;
    }
    else if(nPort == 11)//DP1
    {
        nIntfType = VO_INTF_DP1;
    }
    else
    {
        nIntfType = VO_INTF_HDMI;
    }

    memset(&stVoPubAttr, 0, sizeof(VO_PUB_ATTR_S));
    nRet = RK_MPI_VO_GetPubAttr(nVoDev, &stVoPubAttr);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_GetPubAttr fail %x\n",nRet);
        return RK_FAILURE;
    }

    /*设置视频层绑定设备*/
    nRet = RK_MPI_VO_BindLayer(nVoLayer, nVoDev, nLayerMode);
    if (nRet) 
    {
        printf("RK_MPI_VO_BindLayer fail %x\n", nRet);
        return RK_FAILURE;
    }

    //1080P
    if(nResolution == 0)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_1080P60;
        nWidth = 1920;
        nHeight = 1080;
    }
    //4K
    else if(nResolution == 1)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_4096x2160_30;
        nWidth = 4096;
        nHeight = 2160;
    }
    //720P
    else if(nResolution == 2)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_720P60;
        nWidth = 1080;
        nHeight = 720;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_1080P60;
        nWidth = 1920;
        nHeight = 1080;
    }
    stVoPubAttr.enIntfType = nIntfType;
    stVoPubAttr.u32BgColor = 0xFF00;
    nRet = RK_MPI_VO_SetPubAttr(nVoDev, &stVoPubAttr);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_SetPubAttr fail %x\n",nRet);
        return RK_FAILURE;
    }
    /*开启显示设备*/
    nRet = RK_MPI_VO_Enable(nVoDev);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_Enable fail %x\n",nRet);
        return RK_FAILURE;
    }

    RK_U32 uBufLen;

    nRet = RK_MPI_VO_GetLayerDispBufLen(nVoLayer,&uBufLen);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_GetLayerDispBufLen fail %x\n",nRet);
    }

    printf("uBufLen=%d\n",uBufLen);

    nRet = RK_MPI_VO_SetLayerDispBufLen(nVoLayer, 8);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_SetLayerDispBufLen fail %x\n",nRet);
    }

    /*开启视频图层*/
    VO_CHN_ATTR_S            stChnAttr;
    VO_CHN_PARAM_S           stChnParam;
    VO_BORDER_S              stBorder;
    stLayerAttr.enPixFormat                  = TEST_FMT_VO;
    
    stLayerAttr.u32DispFrmRt                 = 60;
    stLayerAttr.stDispRect.s32X              = 0;
    stLayerAttr.stDispRect.s32Y              = 0;
    stLayerAttr.stDispRect.u32Width          = nWidth;
    stLayerAttr.stDispRect.u32Height         = nHeight;
    stLayerAttr.stImageSize.u32Width         = nWidth;
    stLayerAttr.stImageSize.u32Height        = nHeight;
    nRet = RK_MPI_VO_SetLayerAttr(nVoLayer, &stLayerAttr);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_SetLayerAttr fail %x\n",nRet);
        return RK_FAILURE;
    }
    nRet = RK_MPI_VO_EnableLayer(nVoLayer);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VO_EnableLayer fail %x\n",nRet);
        return RK_FAILURE;
    }

    return 0;
}

/*
*@description: VO输出口关闭
*@Author: cds
*@param[in]: nVoDev 0~3 输出设备号
*@param[in]: nVoLayer 0~7 图层号
*@return: 0成功 其他值失败
*/
int voPortUninit(VO_DEV nVoDev,VO_LAYER nVoLayer)
{
    RK_S32                  nRet = RK_SUCCESS;

    /*禁止图层*/
    printf("VO Layer(%d) disable\n",nVoLayer);
    nRet = RK_MPI_VO_DisableLayer(nVoLayer);
    if (nRet != RK_SUCCESS)
    {
        printf("[%d] RK_MPI_VO_DisableLayer fail with %x\n",__LINE__,nRet);
    }

    /*禁用视频输出设备*/
    printf("VO DEV(%d) disable\n",nVoDev);
    nRet = RK_MPI_VO_Disable(nVoDev);
    if (nRet != RK_SUCCESS)
    {
        printf("[%d] RK_MPI_VO_Disable fail with %x\n",__LINE__,nRet);
    }

    /*解除图层和显示输出设备的绑定关系*/
    printf("VO Layer(%d) unBind DEV(0)\n",nVoLayer);
    nRet = RK_MPI_VO_UnBindLayer(nVoLayer, nVoDev);
    if (nRet != RK_SUCCESS)
    {
        printf("[%d] RK_MPI_VO_UnBindLayer fail with %x\n",__LINE__,nRet);
    }

    return 0;
}

static int vdec_init(int nVdecChnId)
{
    int nRet;
    RkVdecNeedParam_S stVdecNeedParam;
    memset(&stVdecNeedParam, 0, sizeof(RkVdecNeedParam_S));
    stVdecNeedParam.unSrcWidth = 1920;//g_pMediaHandle->demuxParam.width;
    stVdecNeedParam.unSrcHeight = 1080;//g_pMediaHandle->demuxParam.height;
    stVdecNeedParam.nChnIndex = nVdecChnId;
    stVdecNeedParam.eInputMode = VIDEO_MODE_FRAME;
    stVdecNeedParam.eCompressMode = COMPRESS_MODE_NONE;
    stVdecNeedParam.eOutputPixFmt = RK_FMT_YUV420SP;
    stVdecNeedParam.enCodecId = RK_VIDEO_ID_AVC;

    g_pVideoHandle = rockitVdec_alloc(stVdecNeedParam);
    g_pVideoHandle->stExParam.unFrameBufferCnt = 16;
    g_pVideoHandle->stExParam.unSendBufferCnt = 16;
    g_pVideoHandle->stExParam.bEnableColmv = RK_TRUE;
    nRet = g_pVideoHandle->rockitVdec_init(g_pVideoHandle);
    if (nRet != RK_SUCCESS)
    {
        printf("vdec chn %d init fail [0x%x]\n", stVdecNeedParam.nChnIndex, nRet);
    }
}

static int vdec_uninit()
{
    if (g_pVideoHandle)
    {
        g_pVideoHandle->rockitVdec_uninit(g_pVideoHandle);
        rockitVdec_release(g_pVideoHandle);
        g_pVideoHandle = NULL;
    }
}

static int vpssInit(VPSS_GRP nVpssGrp,RK_U32 nWidth,RK_U32 nHeight)
{
    RK_S32 nRet = RK_SUCCESS;
    //VPSS_CHN nVpssChn[VPSS_MAX_CHN_NUM] = { VPSS_CHN0, VPSS_CHN1, VPSS_CHN2, VPSS_CHN3 };
    VPSS_GRP_ATTR_S stGrpVpssAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CROP_INFO_S stChnCropInfo;

    stGrpVpssAttr.u32MaxW = nWidth;
    stGrpVpssAttr.u32MaxH = nHeight;
    stGrpVpssAttr.enPixelFormat = TEST_FMT_VPSS;
    stGrpVpssAttr.enCompressMode = COMPRESS_MODE_NONE;//COMPRESS_AFBC_16x16;//COMPRESS_AFBC_16x16;//COMPRESS_MODE_NONE;
    stGrpVpssAttr.stFrameRate.s32SrcFrameRate = -1;
    stGrpVpssAttr.stFrameRate.s32DstFrameRate = -1;

    nRet = RK_MPI_VPSS_CreateGrp(nVpssGrp, &stGrpVpssAttr);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VPSS_CreateGrp failed! %x \n",nRet);
        return nRet;
    }

    /*设置 VPSS 的硬件设备类型*/
    nRet = RK_MPI_VPSS_SetVProcDev( nVpssGrp, VIDEO_PROC_DEV_GPU);
    if (nRet != RK_SUCCESS)
    {
        printf("RK_MPI_VPSS_CreateGrp failed! %x \n",nRet);
        return nRet;
    }

    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));

    //u32Depth深度为0时，表示不保留通道图像，全部丢弃。
    //u32Depth深度仅在通道非绑定模式下（无绑定后级模块）时生效
    stVpssChnAttr.u32Depth = 3;//队列深度不为0才能手动获得数据

    stVpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;//VPSS_CHN_MODE_AUTO;//VPSS_CHN_MODE_USER;
    stVpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;//COMPRESS_AFBC_16x16;//COMPRESS_MODE_NONE;
    stVpssChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stVpssChnAttr.enPixelFormat = TEST_FMT_VPSS;
    stVpssChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssChnAttr.stFrameRate.s32DstFrameRate = -1;

    /*使能通道0，分辨率为1920x1080*/
    stVpssChnAttr.u32Width = nWidth;
    stVpssChnAttr.u32Height = nHeight;
    /*设置通道参数*/
    nRet = RK_MPI_VPSS_SetChnAttr(nVpssGrp, VPSS_CHN0, &stVpssChnAttr);
    if (nRet != RK_SUCCESS) 
    {
        return nRet;
    }

    /*使能vpss通道*/
    nRet = RK_MPI_VPSS_EnableChn(nVpssGrp, VPSS_CHN0);
    if (nRet != RK_SUCCESS)
    {
        return nRet;
    }


    /*使能通道1，分辨率为640x640*/
    stVpssChnAttr.u32Width = 1920;
    stVpssChnAttr.u32Height = 1024;
    stVpssChnAttr.stFrameRate.s32SrcFrameRate = 30;
    stVpssChnAttr.stFrameRate.s32DstFrameRate = 18;
    stVpssChnAttr.enPixelFormat = TEST_FMT_VPSS;

    // RK_FMT_YUV420SP         = RK_VIDEO_FMT_YUV,        /* YYYY... UV...            */
    // RK_FMT_YUV420SP_10BIT,
    // RK_FMT_YUV422SP,                                   /* YYYY... UVUV...          */f
    // RK_FMT_YUV422SP_10BIT,                             ///< Not part of ABI
    // RK_FMT_YUV420P,                                    /* YYYY... UUUU... VVVV     */
    // RK_FMT_YUV420P_VU,                                 /* YYYY... VVVV... UUUU     */
    // RK_FMT_YUV420SP_VU,                                /* YYYY... VUVUVU...        */
    // RK_FMT_YUV422P,                                    /* YYYY... UUUU... VVVV     */
    // RK_FMT_YUV422SP_VU,                                /* YYYY... VUVUVU...        */
    // RK_FMT_YUV422_YUYV,                                /* YUYVYUYV...              */
    // RK_FMT_YUV422_UYVY,                                /* UYVYUYVY...              */
    // RK_FMT_YUV400SP,                                   /* YYYY...                  */
    // RK_FMT_YUV440SP,                                   /* YYYY... UVUV...          */
    // RK_FMT_YUV411SP,                                   /* YYYY... UV...            */
    // RK_FMT_YUV444,                                     /* YUVYUVYUV...             */
    // RK_FMT_YUV444SP,                                   /* YYYY... UVUVUVUV...      */
    // RK_FMT_YUV444P,                                    /* YYYY... UUUU... VVVV     */
    // RK_FMT_YUV422_YVYU,                                /* YVYUYVYU...              */
    // RK_FMT_YUV422_VYUY,                                /* VYUYVYUY...              */
    // RK_FMT_YUV_BUTT,

    // RK_FMT_RGB565          = RK_VIDEO_FMT_RGB,         /* 16-bit RGB               */
    // RK_FMT_BGR565,                                     /* 16-bit RGB               */
    // RK_FMT_RGB555,                                     /* 15-bit RGB               */
    // RK_FMT_BGR555,                                     /* 15-bit RGB               */
    // RK_FMT_RGB444,                                     /* 12-bit RGB               */
    // RK_FMT_BGR444,                                     /* 12-bit RGB               */
    // RK_FMT_RGB888,                                     /* 24-bit RGB               */
    // RK_FMT_BGR888,                                     /* 24-bit RGB               */
    // RK_FMT_RGB101010,                                  /* 30-bit RGB               */
    // RK_FMT_BGR101010,                                  /* 30-bit RGB               */
    // RK_FMT_ARGB1555,                                   /* 16-bit RGB               */
    // RK_FMT_ABGR1555,                                   /* 16-bit RGB               */
    // RK_FMT_ARGB4444,                                   /* 16-bit RGB               */
    // RK_FMT_ABGR4444,                                   /* 16-bit RGB               */
    // RK_FMT_ARGB8565,                                   /* 24-bit RGB               */
    // RK_FMT_ABGR8565,                                   /* 24-bit RGB               */
    // RK_FMT_ARGB8888,                                   /* 32-bit RGB               */
    // RK_FMT_ABGR8888,                                   /* 32-bit RGB               */
    // RK_FMT_BGRA8888,                                   /* 32-bit RGB               */
    // RK_FMT_RGBA8888,                                   /* 32-bit RGB               */
    // RK_FMT_RGBA5551,                                   /* 16-bit RGB               */
    // RK_FMT_BGRA5551,                                   /* 16-bit RGB               */
    // RK_FMT_BGRA4444,                                   /* 16-bit RGB               */
    // RK_FMT_RGBA4444,                                   /* 16-bit RGB               */
    // RK_FMT_XBGR8888,                                   /* 32-bit RGB               */
    // RK_FMT_RGB_BUTT,

    /*设置通道参数*/
    nRet = RK_MPI_VPSS_SetChnAttr(nVpssGrp, VPSS_CHN1, &stVpssChnAttr);
    if (nRet != RK_SUCCESS) 
    {
        return nRet;
    }

    /*使能vpss通道*/
    nRet = RK_MPI_VPSS_EnableChn(nVpssGrp, VPSS_CHN1);
    if (nRet != RK_SUCCESS)
    {
        return nRet;
    }

    /*开启vpss*/
    nRet = RK_MPI_VPSS_StartGrp(nVpssGrp);
    if (nRet != RK_SUCCESS) 
    {
        return nRet;
    }
    
    return RK_SUCCESS;
}

static int vpssUnInit()
{
    int nRet;
    VPSS_GRP nVpssGrp = 0;
    VPSS_CHN nVpssChn[VPSS_MAX_CHN_NUM] = { VPSS_CHN0, VPSS_CHN1, VPSS_CHN2, VPSS_CHN3 };

    nRet = RK_MPI_VPSS_StopGrp(nVpssGrp);
    if (nRet != RK_SUCCESS)
    {
        return nRet;
    }

    for (RK_S32 i = 0; i < VPSS_MAX_CHN_NUM; i++)
    {
        nRet = RK_MPI_VPSS_DisableChn(nVpssGrp, nVpssChn[i]);
        if (nRet != RK_SUCCESS)
        {
            return nRet;
        }
    }

    nRet = RK_MPI_VPSS_DestroyGrp(nVpssGrp);
    if (nRet != RK_SUCCESS)
    {
        return nRet;
    }

    return RK_SUCCESS;
}

static RK_S32 vdec_free(void *pParam)
{
    if (pParam)
    {
        free(pParam);
        pParam = NULL;
    }
    return 0;
}

static int vencInit(int nVencChn)
{
	RkVencNeedParam_S stParam;
	/******************初始化VENC********************************/
    memset( &stParam, 0, sizeof(RkVencNeedParam_S)  );
    /*编码初始化*/
    stParam.unWidth = 1920;
    stParam.unHeight = 1080;
    stParam.unVirWidth = 1920;
    stParam.unVirHeight = 1080;
    stParam.ePixFormat = TEST_FMT_VPSS;//RK_FMT_YUV420SP;//RK_FMT_BGR888;
    stParam.eCodec = RK_VIDEO_ID_AVC;
    stParam.unChnIndex = nVencChn;
    stParam.nInFrameRate = 30;
    stParam.nOutFrameRate = 30;
    stParam.nGop = 30;
    stParam.enCompressMode = COMPRESS_MODE_NONE;
    g_pEncHandle = rockitVenc_alloc(stParam);
    g_pEncHandle->stExParam.nBitRate     = 4096;
    g_pEncHandle->rockitVenc_init(g_pEncHandle);
}


/*获取视频送队列线程*/
static void *getVideoThr(void *pParam)
{
    int nRet;
    VIDEO_FRAME_INFO_S stVideFrame;

    while (!g_bExit)
    {
		nRet = RK_MPI_VPSS_GetChnFrame(0, VPSS_CHN1, &stVideFrame,-1);
        if(RK_SUCCESS != nRet)
        {
            printf("RK_MPI_VPSS_GetChnFrame failed with %#x\n", nRet);
            usleep(10000);
        }
        else
        {
            if(OS_queIsFull(&g_videoQue))   
            {
                printf("2队列满\n");
            }
            else
            {
                RK_VOID *data = RK_MPI_MB_Handle2VirAddr(stVideFrame.stVFrame.pMbBlk);
                int nSize = RK_MPI_MB_GetSize(stVideFrame.stVFrame.pMbBlk);

                VideoFrame_S* pVideFrame = (VideoFrame_S*)malloc(sizeof(VideoFrame_S));
                pVideFrame->nSize = nSize;
                pVideFrame->pData = malloc(nSize);
                memcpy(pVideFrame->pData,data,nSize);

                pVideFrame->nWidth = stVideFrame.stVFrame.u32Width;
                pVideFrame->nHeight = stVideFrame.stVFrame.u32Height;
                pVideFrame->nVirWidth = stVideFrame.stVFrame.u32VirWidth;
                pVideFrame->nVirHeight = stVideFrame.stVFrame.u32VirHeight;
                pVideFrame->nPixelFormat = stVideFrame.stVFrame.enPixelFormat;

                OS_quePut(&g_videoQue, (Int64)pVideFrame, 0);
            }

            int nRet = RK_MPI_VPSS_ReleaseChnFrame(0, VPSS_CHN1,&stVideFrame);
            if(RK_SUCCESS != nRet)
            {
                printf("vpss releaseChnFream error\n");
            }
        }

    }

    return NULL;
}


/*读取视频队列送解码线程*/
static void *sendDecThr(void *pParam)
{
    Int64 naddr = 0;
    OS_QueHndl *queHndl = (OS_QueHndl *)&g_MediaQue;

    while (!g_bExit)
    {
		naddr = 0;
        int nstatus = OS_queGet(queHndl, &naddr, -1);
        if (nstatus == 0 && naddr != 0)
        {
            RkMediaData_S *pMedata = (RkMediaData_S *)naddr;
            /*送解码*/
            g_pVideoHandle->rockitVdec_send_stream(pMedata);
            
            free(pMedata);
        }
    }
    while (0 == OS_queGet(queHndl, &naddr, 0))
    {
        RkMediaData_S *pMedata = (RkMediaData_S *)naddr;
        if( pMedata->pData )
        {
            free( pMedata->pData );
        }
        free(pMedata);
    }

    return NULL;
}

/*读取媒体线程*/
void* getMediaThr(void*arg)
{
    int nRet;
    int nIsFile = 0;
    char* pUrl = (char*)arg;

    /*打开文件*/
    g_pMediaHandle = media_open_url(pUrl, 0, NULL, NULL, NULL);
    if (NULL == g_pMediaHandle)
    {
        printf( "player open file is fial\n");
        return NULL;
    }

    mediaParam_S stDemuxParam;

    media_get_demuxInfo(g_pMediaHandle,&stDemuxParam);

    printf("url:%s\n",stDemuxParam.url);
    printf("frameRate:%f\n",stDemuxParam.frameRate);
    printf("width:%d\n",stDemuxParam.width);
    printf("height:%d\n",stDemuxParam.height);
    printf("sampleRate:%d\n",stDemuxParam.sampleRate);
    printf("channel:%d\n",stDemuxParam.channel);
    printf("track:%d\n",stDemuxParam.track);
    printf("duration:%lld\n",stDemuxParam.duration);

    if(stDemuxParam.frameRate <= 0)
    {
        stDemuxParam.frameRate = 25;
    }
    double fVideoDuration = 1/stDemuxParam.frameRate;

    if(!strncmp(pUrl,"rtsp://",7) || !strncmp(pUrl,"rtmp://",7) || !strncmp(pUrl,"http://",7))
    {
        nIsFile = 0;
    }
    else
    {
        nIsFile = 1;
    }

    mediaPacket_t *pkt = (mediaPacket_t *)malloc(sizeof(mediaPacket_t));
    while (!g_bExit)
    {
        nRet = media_get_frame(g_pMediaHandle, pkt);
        if (nRet >= 0)
        {
            if (pkt->type == MEDIA_TYPE_VIDEO)
            {
                if(OS_queIsFull(&g_MediaQue))
                {
                    printf("1队列满\n");
                    /*判断是文件添加延时防止一下子读完*/
                    if(nIsFile)
                    {
                        av_usleep((int64_t)(fVideoDuration*AV_TIME_BASE));
                    }
                }
                else
                {
                    RkMediaData_S *pMedata = (RkMediaData_S *)malloc(sizeof(RkMediaData_S));
                    pMedata->pHandle = g_pVideoHandle;
                    pMedata->pData = pkt->data;
                    pMedata->nSize = pkt->size;
                    pMedata->pFreeFunCB = vdec_free;
                    pMedata->bEos = RK_FALSE;
                    pMedata->nTimeMs = 500;
                    pMedata->nPts = 0;
                    OS_quePut(&g_MediaQue, (Int64)pMedata, -1);

                    /*判断是文件添加延时防止一下子读完*/
                    if(nIsFile)
                    {
                        av_usleep((int64_t)(fVideoDuration*AV_TIME_BASE));
                    }
                    continue;
                }
            }
        }
        else
        {
            break;
        }
        
        media_unpaket(g_pMediaHandle, pkt);
    }

    return NULL;
}

/*  随机根据ID生成颜色 */
cv::Scalar getColor(int nIdx)
{
    nIdx += 3;
    return cv::Scalar(37 * nIdx % 255, 17 * nIdx % 255, 29 * nIdx % 255);
}
