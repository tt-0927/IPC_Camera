/**
 * @FilePath     : svp_ld.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-10-27 09:13:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-28 15:13:50
 * @Description  : 物品检测封装（遗留/拿取）
 */

#ifndef _MPP_SVP_LD_H_
#define _MPP_SVP_LD_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "sample_common_ive.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_sys.h"
#include "ot_common_svp.h"
#include "ss_mpi_ive.h"
#include "mpi_common.h"

/* 最大检测区域数 */
#define SVP_LD_MAX_REGION_NUM    8

/* 多边形最大顶点数 */
#define SVP_LD_MAX_POLYGON_POINTS    4

/* 点坐标结构 */
typedef struct _HiLdPoint_S
{
    td_float fX;
    td_float fY;
} HiLdPoint_S;

/* 多边形区域定义 */
typedef struct _HiLdPolygon_S
{
    td_bool bEnable;                                /* 是否启用该区域 */
    td_u32 u32PointNum;                             /* 有效顶点数 [3, SVP_LD_MAX_POLYGON_POINTS] */
    HiLdPoint_S aPoints[SVP_LD_MAX_POLYGON_POINTS]; /* 多边形顶点坐标 */
} HiLdPolygon_S;

/* 海思 物品检测必需参数 */
typedef struct _HiLdNeedParam_S
{
    /* 图像宽，必须为16字节对齐，范围：[64, 1920] */
    td_u32 u32Width;
    /* 图像高，必须为偶数，范围：[64, 1080] */
    td_u32 u32Height;
    /* 检测区域数组 */
    HiLdPolygon_S stRegions[SVP_LD_MAX_REGION_NUM];
    /* 有效区域数量 */
    td_u32 u32RegionNum;
} HiLdNeedParam_S;

/* 海思 物品检测扩展参数 */
typedef struct _HiLdExParam_S
{
    /* 差分阈值：用于二值化差分图像，范围：[1, 255]，默认：20 */
    td_u8 u8DiffThreshold;
    /* 是否手动更新参考帧，默认：TD_FALSE(自动更新) */
    td_bool bManualUpdate;
} HiLdExParam_S;

/* 单个区域灵敏度结果 */
typedef struct _HiLdRegionResult_S
{
    td_bool bValid;         /* 该区域是否有效计算 */
    td_u32 u32Sensitivity;  /* 灵敏度值 [0, 100] */
    td_u64 u64ST;           /* 全图变化量 */
    td_u64 u64S1;           /* 区域变化量 */
} HiLdRegionResult_S;

typedef struct _HiLd_S HiLd_S;
struct _HiLd_S
{
    // info /**********************必需参数***************************/
    HiLdNeedParam_S stNeedParam;
    HiLdExParam_S stExParam;

    // info /**********************辅助参数***************************/
    ot_svp_src_img stRefFrame;                           /* 参考帧(背景) */
    ot_svp_src_img stCurFrame;                           /* 当前帧 */
    ot_svp_dst_img stDiffFrame;                          /* 差分图像(ABS模式，U8C1) */
    ot_svp_dst_img stIntegFrame;                         /* 全图积分图(用于计算ST) */
    ot_svp_dst_img stMaskedImg;                          /* 用于AND操作的临时图像缓冲区 */
    ot_svp_dst_img stMaskFrame[SVP_LD_MAX_REGION_NUM];   /* 各区域mask图像 */
    ot_svp_dst_img stRegionInteg[SVP_LD_MAX_REGION_NUM]; /* 各区域积分图(用于计算S1) */

    /* 控制参数 */
    ot_ive_sub_ctrl stSubCtrl;          /* 减法控制 */
    ot_ive_integ_ctrl stIntegCtrl;      /* 积分图控制 */

    /* 计算结果 */
    HiLdRegionResult_S stResults[SVP_LD_MAX_REGION_NUM];

    td_bool bFirstFrame; /* 是否为第一帧标志 */
    td_bool bInited;     /* 是否已初始化 */

    // info /**********************功能列表***************************/
    /* 物品检测初始化 */
    int (*svpLd_init)(HiLd_S *pHandle);

    /* 物品检测去初始化 */
    int (*svpLd_uninit)(HiLd_S *pHandle);

    /* 送帧给LD进行检测处理 */
    int (*svpLd_sendFrame)(HiLd_S *pHandle, ot_video_frame_info *pFrameInfo);

    /* 获取物品检测结果(仅灵敏度) */
    int (*svpLd_getResult)(HiLd_S *pHandle, HiLdRegionResult_S *pResults, td_u32 u32MaxNum);

    /* 手动更新参考帧 */
    int (*svpLd_updateRef)(HiLd_S *pHandle, ot_video_frame_info *pFrameInfo);

    /* 打印结果 */
    void (*svpLd_printResult)(HiLd_S *pHandle);
};

/**
 * @brief   : 分配物品检测模块句柄
 * @param    {HiLdNeedParam_S} stNeedParam 必须参数
 * @return   {HiLd_S *} 成功返回句柄，失败返回NULL
 */
HiLd_S *svpLd_alloc(HiLdNeedParam_S stNeedParam);

/**
 * @brief   : 释放物品检测模块句柄
 * @param    {HiLd_S} *pHandle 句柄
 */
void svpLd_release(HiLd_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
