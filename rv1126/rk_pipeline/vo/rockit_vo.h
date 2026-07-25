/*************************************************************************
	> File Name: rockit_vo.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2023年05月22日 星期一 14时26分42秒
 ************************************************************************/

#ifndef _ROCKIT_VO_H
#define _ROCKIT_VO_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rk_debug.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_cal.h"
#include "rk_comm_video.h"
#include "rk_mpi_vo.h"
#include "rk_comm_vo.h"
/*视频层*/
#define VOLAYER_CLUSTER0 0
#define VOLAYER_CLUSTER1 1
#define VOLAYER_CLUSTER2 2
#define VOLAYER_CLUSTER3 3
#define VOLAYER_ESMART0  4
#define VOLAYER_ESMART1  5
#define VOLAYER_ESMART2  6
#define VOLAYER_ESMART3  7
/*通道参数*/
typedef struct _RkVoRect_S
{
    /*显示位置*/
    int nPosX;
    int nPosY;
    /*显示大小*/
    int nWidth;
    int nHeight;
}RkVoRect_S;

typedef struct _RkVoNeedParam
{
    /*VO设备号*/
    int nDev;
    /*图层数*/
    int nLayerNum;
    /*图层*/
    int aLayer[8];
    /*接口类型设置*/
    VO_INTF_TYPE_E enIntfType;
    /*输出时序设置*/
    VO_INTF_SYNC_E enIntfSync;
    /*分辨率*/
    int nDisWidth;
    int nDisHeight;
    /*画布*/
    int nImageWidth;
    int nImageHeight;
    /*各图层通道数量*/
    int aChnSum[8];
    /*通道数据*/
    RkVoRect_S *pChnRect[8];
}RkVoNeedParam_S;

typedef struct _RkVoExParam
{
    /*图层模式 默认ui图层*/
    VO_LAYER_MODE_E aLayerMode[8];
    /*图像帧直接送显，只有通道数为1时有效*/
    RK_BOOL aBypassFrame[8];
    /*使用通道0的送帧事件，触发拼接，拼接后立即送显*/
    RK_BOOL aLowDelay[8];
    /*视频帧缓存长度 默认6*/
    int aVoBufLen[8];
    /*图层样式 默认RK_FMT_RGB888*/
    PIXEL_FORMAT_E aPixFormat[8];
    /*图层帧率 默认30*/
    int aDisFrameRate[8];
    /*通道帧率 默认30*/
    int aChnFrameRate[8];
    /*图层合成方式 默认GPU*/
    VO_SPLICE_MODE_E aLayerSpliceMode[8];
    /*图层压缩模式 默认 COMPRESS_MODE_NONE*/
    COMPRESS_MODE_E aCompressMode[8];
    
    /*设备插拔回调 默认NULL*/
    RK_VOID* pPrivateData;
    void (*pVoCallBack)(RK_VOID *pPrivateData);
    /*启用hdmi屏幕输出格式 默认RK_FALSE*/
    RK_BOOL bHdmiFmt;
    /*输出颜色格式*/
    VO_HDMI_COLOR_FMT_E enColorFmt;
    /*输出量化范围*/
    VO_HDMI_QUANT_RANGE_E enQuantRange;
    /*输出模式*/
    VO_HDMI_MODE_E enHdmiMode;
    /* 背景颜色 */
    uint32_t nBgColor;

}RkVoExparam_S;

typedef struct _RkVo RkVo_S;
struct _RkVo
{
    /*必须参数*/
    RkVoNeedParam_S stNeedParam;
    /*功能参数*/
    RkVoExparam_S stExParam;
    /*回写初始化*/
    int (*rockitVo_WbcInit) (RkVo_S *pHandle);
    /*回写去初始化*/
    int (*rockitVo_WbcUninit) (RkVo_S *pHandle);
    /*送图像帧到图层通道 
    *@inparam nChn -1 直接送图层， 不为-1 表示送图层通道
    *@return 0 成功  -1 表示图层或者通道或者参数有误  其它失败
    * */
    int (*rockitVo_send_frame) (RkVo_S *pHandle, VIDEO_FRAME_INFO_S *pFrame, int nLayer, int nChn, int nMilliSec);
    /*隐藏某个图层的通道显示*/
    int (*rockitVo_isShowChn) ( RkVo_S *pHandle, int nVoLayer, int nChn, RK_BOOL bShow);
    /*重新初始化vo*/
    int (*rockit_vo_reset) (RkVo_S *pHandle, RkVoNeedParam_S *pNeedParam, RkVoExparam_S *pExParam);
    /*初始化vo*/
    int (*rockit_vo_init) (RkVo_S *pHandle);
    /*反初始化vo*/
    int (*rockit_vo_uninit) (RkVo_S *pHandle);
};
/*分配vo句柄*/
RkVo_S *rockit_vo_alloc(RkVoNeedParam_S stNeedParam);
/*释放vo句柄*/
int rockit_vo_release(RkVo_S* pHandle);
#endif
