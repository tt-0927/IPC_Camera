/**
 * @FilePath     : mpp_rgn.h
 * @Author       : zhouzirui
 * @Date         : 2025-05-08 16:35:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-01 16:50:14
 * @Description  : 海思区域模块封装
 */

#ifndef _MPP_RGN_H_
#define _MPP_RGN_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "ot_common_region.h"
#include "ss_mpi_region.h"

#define OVERLAY_MIN_HANDLE 0
#define COVER_MIN_HANDLE 24
#define COVEREX_MIN_HANDLE 40
#define LINEEX_MIN_HANDLE 60
#define CORNER_RECT_MIN_HANDLE 80
#define CORNER_RECTEX_MIN_HANDLE 100

#define REGION_OP_CHN (0x01L << 0)
#define REGION_OP_DEV (0x01L << 1)
#define REGION_DESTROY (0x01L << 2)

    /* 区域分配要填写的参数 */
    typedef struct _RgnNeed
    {
        /* 必要参数 */
        td_bool bIsShow;    /* 是否显示 */
        uint32_t unHandle;  /* 区域句柄 */
        uint32_t unOpFlag;  /* 操作标志（通道或者设备） */
        uint32_t unWidth;   /* Rgn宽度 */
        uint32_t unHeight;  /* Rgn高度 */
        uint32_t unModId;   /* 模式ID */
        uint32_t unDevId;   /* 设备ID */
        uint32_t unChnId;   /* 通道ID */
        uint32_t unType;    /* 功能类型 */
        uint32_t unStartX;  /* 起始坐标X */
        uint32_t unStartY;  /* 起始坐标Y */
        uint32_t unFgColor; /* 前景颜色 */
        uint32_t unBgColor; /* 背景颜色 */
        uint32_t unFgAlpha; /* 前景透明度0~255 */
        uint32_t unBgAlpha; /* 背景透明度0~255 */
        uint32_t unLayer;   /* 叠加层数 */
        /* overplay可选参数 */
        uint32_t unFontSize;  /* 字体大小 */
        uint32_t unHorMargin; /* 水平边距 */
        uint32_t unVerMargin; /* 垂直边距 */
        td_bool bIsFlicker;   /* 是否闪烁 */
        /* cover可选参数 */
        ot_point stuPoints[OT_QUAD_POINT_NUM];
        td_bool bIsRectangle; /* 是否是矩形 */
        td_bool bIsSolid;     /* 是否实心 */
        /* corner_rect可选参数 */
        td_u32 uHorLen; /* 角框水平线长 */
        td_u32 uVerLen; /* 角框竖直线长 */
        td_u32 uThick;   /* 角框线宽 [2, 16] */

    } HiRgnNeedParam_S;

    typedef struct _HiRgn HiRgn_S;

    struct _HiRgn
    {
        /* 必要参数 */
        td_bool bIsShow;    /* 是否显示 */
        td_bool bIsAttached; /* 是否已经绑定到通道 */
        uint32_t unHandle;  /* 区域句柄 */
        uint32_t unOpFlag;  /* 操作标志（通道或者设备） */
        uint32_t unWidth;   /* Rgn宽度 */
        uint32_t unHeight;  /* Rgn高度 */
        uint32_t unModId;   /* 模式ID */
        uint32_t unDevId;   /* 设备ID */
        uint32_t unChnId;   /* 通道ID */
        uint32_t unType;    /* 功能类型 */
        uint32_t unStartX;  /* 起始坐标X */
        uint32_t unStartY;  /* 起始坐标Y */
        uint32_t unFgColor; /* 前景颜色 */
        uint32_t unBgColor; /* 背景颜色 */
        uint32_t unFgAlpha; /* 前景透明度0~255 */
        uint32_t unBgAlpha; /* 背景透明度0~255 */
        uint32_t unLayer;   /* 叠加层数 */
        /* overplay可选参数 */
        uint32_t unFontSize;      /* 字体大小 */
        uint32_t unHorMargin;     /* 水平边距 */
        uint32_t unVerMargin;     /* 垂直边距 */
        td_bool bIsFlicker;       /* 是否闪烁 */
        ot_pixel_format enFormat; /* 像素格式 */
        /* cover可选参数 */
        ot_point stuPoints[OT_QUAD_POINT_NUM];
        td_bool bIsRectangle; /* 是否是矩形 */
        td_bool bIsSolid;     /* 是否实心 */
        /* corner_rect可选参数 */
        td_u32 uHorLen; /* 角框水平线长 */
        td_u32 uVerLen; /* 角框竖直线长 */
        td_u32 uThick;   /* 角框线宽 */

        /* 功能列表 */
        /* 创建一个RGN
         * inparam pHandle 区域句柄
         */
        int (*mppRgn_create)(HiRgn_S *pHandle);

        /* 销毁区域
         * inparam pHandle 区域句柄
         */
        int (*mppRgn_destroy)(HiRgn_S *pHandle);

        /* 将区域叠加到通道上
         * inparam pHandle 区域句柄
         */
        int (*mppRgn_attachToChn)(HiRgn_S *pHandle);

        /* 将区域从通道上撤出
         * inparam pHandle 区域句柄
         */
        int (*mppRgn_detachFromChn)(HiRgn_S *pHandle);

        /* 加载图片数据到区域上
         * inparam pHandle 区域句柄
         * inparam pImage  图片数据或者用户参数
         * inparam nSize   数据大小
         */
        int (*mppRgn_loadPicture)(HiRgn_S *pHandle, void *pImage, int nSize);

        /* 清空区域上的贴图
         * inparam pHandle 区域句柄
         */
        int (*mppRgn_clearPicture)(HiRgn_S *pHandle);

        /* 改变区域的位置
         * inparam pHandle 区域句柄
         * inparam nStartX X方向起始坐标
         * inparam nStartY Y方向起始坐标
         */
        int (*mppRgn_changePos)(HiRgn_S *pHandle, int nStartX, int nStartY);

        /* 改变区域的尺寸
         * inparam pHandle 区域句柄
         * inparam nWidth  长度
         * inparam nHeight 高度
         */
        int (*mppRgn_changeRect)(HiRgn_S *pHandle, int nWidth, int nHeight);

        /* 显示或者隐藏区域
         * inparam pHandle 区域句柄
         * inparam bIsShow   1:显示,0:隐藏
         */
        int (*mppRgn_showOrHide)(HiRgn_S *pHandle, td_bool bIsShow);

        /* 更新区域
         * inparam pHandle 区域句柄
         * inparam stParam 必填参数
         */
        int (*mppRgn_update)(HiRgn_S *pHandle, HiRgnNeedParam_S stParam);
    };

    /* 分配区域句柄
     * inparam stParam 必填参数
     */
    HiRgn_S *mppRgn_alloc(HiRgnNeedParam_S stParam);

    /* 释放区域句柄
     * inparam stHandle 区域句柄
     */
    void mppRgn_release(HiRgn_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
