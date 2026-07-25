/*
 * @FilePath     : rockit_vpss.h
 * @Author       : wxz
 * @Date         : 2022-05-06 09:37:27
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-11-11 20:14:13
 * @Description  : vpss模块代码
 */

#ifndef ROCKIT_VPSS_H
#define ROCKIT_VPSS_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "rk_debug.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_cal.h"
#include "rk_comm_video.h"
#include "rk_comm_vpss.h"
#include "rk_mpi_vpss.h"

#include "mpi_common.h"

/*vpss组默认最大支持通道数*/
#define VPSSCHNMAX 4

typedef struct _RkVpssGrpAttr
{
    /*数据格式*/
    PIXEL_FORMAT_E enGrpPixelFormat;
    /*压缩模式*/
    COMPRESS_MODE_E enGrpComMode;

    /*vpss通道处理图像最大宽高*/
    int nMaxW;
    int nMaxH;
    /*输入源和目标帧率*/
    int nSrcFrameRate;
    int nDstFrameRate;
    /*是否开启backup帧
     * 如果需要手动获取帧，必须打开这个参数*/
    int nEnBackup;
}RkVpssGrpAttr_S;

typedef struct _RkVpssChnAttr
{
    /*数据格式*/
    PIXEL_FORMAT_E enChnPixelFormat;
    /*压缩模式*/
    COMPRESS_MODE_E enChnComMode;
    /*通道工作模式，直通或者用户模式
     * user模式：用户需要手动获取通道处理输出图像时设置，会带来性能开销
     * past模式：：在绑定VO时使用，将会把处理工作放于VO中一并处理，
     *             不会带来额外开销，预览、回放时建议设置为此模式*/
    VPSS_CHN_MODE_E enVpssChnMode;
    /*通道目标图像宽高*/
    int nWidth;
    int nHeight;

    /*存储深度
     * 如果需要手动获取帧，必须>0*/
    int nDepth;
    int nFrameBufCnt;
    /*输入源和目标帧率*/
    int nSrcFrameRate;
    int nDstFrameRate;
}RkVpssChnAttr_S;

typedef struct _RkVpssNeedParam
{
    /*组*/
    int nVpssGrp;

    /*每组下的通道总数*/
    int nVpssChnSum;

    /*组属性*/
    RkVpssGrpAttr_S stVpssGrpAttr;

    /*通道属性，默认4个，实际初始化数量以nVpssChnSum为准*/
    RkVpssChnAttr_S astVpssChnAttr[VPSS_MAX_CHN_NUM];
}RkVpssNeedParam_S;

typedef struct _RkVpss RkVpss_S;

struct _RkVpss
{
/************************必填参数****************************************/
    /*组*/
    int nVpssGrp;

    /*每组下的通道总数*/
    int nVpssChnSum;

    /*组属性*/
    RkVpssGrpAttr_S stVpssGrpAttr;

    /*通道属性，默认4个，实际初始化数量以nVpssChnSum为准*/
    RkVpssChnAttr_S astVpssChnAttr[VPSS_MAX_CHN_NUM];

/*********可选参数**********
 *该类型参数，在创建时可不用传入，有默认值，后续可在功能函数设置
 * ************************/
    VIDEO_PROC_DEV_TYPE_E enDevType;

/*********************功能列表******************************************/
    /*
     * *@description vpss资源初始化
     * *@Author: wxz
     * *@param[in] 
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockit_vpss_init) (RkVpss_S *pRkVpssHandle);

    /*获取vpss组属性*/
    int (*rockitVpss_get_grpAttr)( RkVpss_S* pHandle, RkVpssGrpAttr_S* pGrpAttr );
    /*设置vpss组属性*/
    int (*rockitVpss_set_grpAttr)( RkVpss_S* pHandle, RkVpssGrpAttr_S* pGrpAttr );
    /*
     * *@description 设置通道属性,缩放比例，通道模式
     * *@Author: wxz
     * *@param[in] pChnAttr:通道参数
     *              nVpssGrp：vpss组
     *              nVpssChn：设置通道
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockitVpss_set_chnAttr) (RkVpss_S *pRkVpssHandle, RkVpssChnAttr_S *pChnAttr, 
            int nVpssGrp, int nVpssChn);
    /*
     * *@description 获取通道属性,缩放比例，通道模式
     * *@Author: wxz
     * *@param[in] pChnAttr:通道参数
     *              nVpssGrp：vpss组
     *              nVpssChn：vpss通道
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockitVpss_get_chnAttr) (RkVpss_S *pRkVpssHandle, RkVpssChnAttr_S *pChnAttr, 
            int nVpssGrp, int nVpssChn);
    /**
     * @brief       : 设置通道电子放大
     * @author      : zhouzirui
     * @param        {RkVpss_S*} pRkVpssHandle  Vpss句柄
     * @param        {int} nVpssChn Vpss通道
     * @param        {int} nZoom 变焦倍数
     * @param        {int} x
     * @param        {int} y
     * @param        {RK_BOOL} bEnable 使能
     * @return       {*}
     */
    RK_S32 (*rockitVpss_set_chnSetZoom)( RkVpss_S* pRkVpssHandle, int nVpssChn, int nZoom, int x, int y, RK_BOOL bEnable); 

    /* 设置通道裁剪 */
    RK_S32 (*rockitVpss_set_chnCrop)(RkVpss_S *pRkVpssHandle, VPSS_CROP_INFO_S *pCropInfo, int nVpssChn);

    /*
     * *@description 手动获取vpss通道数据
     * *@Author: wxz
     * *@param[in]  pRkVpssHandle: Vpss句柄
     *              pstVideFrame:数据帧
     *              nVpssChn:Vpss通道
     *              nMilliSec:  -1为阻塞
     *                          0为非阻塞
     *                          大于0为等待时间，ms
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockitVpss_get_chnFrame) (RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
            int nVpssChn, int nMilliSec);

    /*
     * *@description 通道数据释放
     * *@Author: wxz
     * *@param[in]  pRkVpssHandle: Vpss句柄
     *              pstVideFrame:数据帧
     *              nVpssChn:Vpss通道
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockitVpss_release_chnFrame) (RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
            int nVpssChn);

    /*
    * *@description 获取vpss通道帧数据缓存区和缓存区大小
    * *@Author: wxz
    * *@param[in]  pRkVpssHandle: Vpss句柄
    *              pstVideFrame:数据帧
    *              data :     缓存区
    *              int size:  缓存区大小
    * *@return 成功返回0,失败返回-1
    * */
    int (*rockitVpss_get_chnFrameData) (RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
        unsigned char ** data, int* size);

    /*
     * *@description 手动往vpss发送图像
     * *@Author: wxz
     * *@param[in]  pRkVpssHandle: Vpss句柄
     *              pstVideFrame:数据帧
     *              nMilliSec:  -1为阻塞等待
     *                          0为非阻塞
     *                          大于0为等待时间，ms
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockitVpss_send_frame) (RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
            int nMilliSec);

    /*
     * *@description vpss资源反初始化
     * *@Author: wxz
     * *@param[in] 
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockit_vpss_uninit) (RkVpss_S *pRkVpssHandle);

    /*
     * *@description vpss获取通道号fd
     * *@Author: fhs
     * *@param[in] pRkVpssHandle: Vpss句柄
     *             nVpssGrp：vpss组
     *             nVpssChn：vpss通道
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockit_get_chnfd) (RkVpss_S *pRkVpssHandle,int nVpssGrp, int nVpssChn);

    /*
     * *@description vpss获取通道号fd
     * *@Author: fhs
     * *@param[in] pRkVpssHandle: Vpss句柄
     *             nVpssGrp：vpss组
     *             nVpssChn：vpss通道
     * *@return 成功返回0,失败返回-1
     * */
    int (*rockit_close_chnfd) (RkVpss_S *pRkVpssHandle,int nVpssGrp, int nVpssChn);


};

/*
 * *@description 申请RkVpss句柄
 * *@Author: wxz
 * *@param[in] 
 * *@return RkVpss_S
 * */
RkVpss_S *rockit_vpss_alloc(RkVpssNeedParam_S stNeedParam);

/*
 * *@description 注销RkVpss句柄
 * *@Author: wxz
 * *@param[in] 
 * *@return 成功返回0,失败返回-1
 * */
void rockit_vpss_release(RkVpss_S* pVpssHdle);

#endif

