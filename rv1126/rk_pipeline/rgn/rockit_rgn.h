/*************************************************************************
	> File Name: rockit_rgn.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月31日 星期二 10时04分31秒
 ************************************************************************/

#ifndef _ROCKIT_RGN_H
#define _ROCKIT_RGN_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "rk_debug.h"
#include "rk_type.h"
#include "rk_mpi_rgn.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mmz.h"


#define OVERLAY_MIN_HANDLE 0
#define OVERLAYEX_MIN_HANDLE 20
#define COVER_MIN_HANDLE 40
#define MOSAIC_MIN_HANDLE 60
#define LINE_MIN_HANDLE 80

#define   RGN_ALIGN   16
#define   RGN_ALIGN_2   2

/*匹配 unLayer 叠加层数 */
#define ELEMENT_TYPE_PEOPLE_AI  1 /* 对应-ai事件画框 ELEMENT_TYPE_PEOPLE */


/*rgn分配要填写的参数*/
typedef struct _RgnNeed
{
        /* 必要参数 */
        RK_BOOL bIsShow;    /* 是否显示 */
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
        RK_BOOL bIsFlicker;   /* 是否闪烁 */
         /* cover可选参数 */
        RK_BOOL bIsRectangle; /* 是否是矩形 */
        POINT_S stuPoints[QUAD_POINT_NUM];
        RK_BOOL bIsSolid;     /* 是否实心 */

}RkRgnNeed_S;

typedef struct _RkRgn RkRgn_S;

struct _RkRgn
{

/*********************功能列表******************************************/
    /*创建一个RGN
     *inparam pHandle rgn句柄
     * */
    int ( *rockitRgn_create ) ( RkRgn_S* pHandle );

    /*释放rgn
     *inparam pHandle rgn句柄
     * */
    int ( *rockitRgn_destroy ) ( RkRgn_S* pHandle );

    /*将RGN区域叠加到通道上
     *inparam pHandle rgn句柄
     * */
    int ( *rockitRgn_attachToChn ) ( RkRgn_S* pHandle );

    /*将RGN区域从通道上撤出
     *inparam pHandle rgn句柄
     * */
    int ( *rockitRgn_detachFromChn ) ( RkRgn_S* pHandle );

    /*overLay导入图片数据
     *inparam   pHandle     句柄
     *inparam   pParam    图片数据或者用户参数
     *inparam   nSize     数据大小
    * */
    int ( *rockitRgn_overlay_loadPic ) ( RkRgn_S* pHandle, void* pParam ,int nSize);

    /* 清空区域上的贴图
        * inparam pHandle 区域句柄
    */
    int (*rockitRgn_clearPicture) ( RkRgn_S* pHandle);

    /*改变rgn的绑定通道
     *inparam pHandle 句柄
     *inparam enModId 模块号 -1不改变 ( MOD_ID_E )
     *inparam nDevId 设备号  -1不改变
     *inparam nChnId 通道号  
     * */
    int ( *rockitRgn_changbind ) ( RkRgn_S* pHandle, int nModId, int nDevId, int nChnId );

    /*改变rgn区域的位置
     *inparam   pHandle     句柄
     *inparam   nStartX          x方向位置
     *inparam   nStartY          y方向位置
                            画线要填写2 起始点和终点
                            在可选参数遮挡选择任意四边形时 填写4
     * */
    int ( *rockitRgn_changePos ) ( RkRgn_S* pHandle, int nStartX, int nStartY);

    /*显示或者隐藏rgn
     *inparam   pHandle 句柄
     *inparam   bShow   1显示 0隐藏
     * */
    int ( *rockitRgn_showOrHide ) ( RkRgn_S* pHandle, RK_BOOL bShow );

    /*改变voerlay的前景背景的透明度
     *inparam   pHandle     句柄
     *inparam   nFgAlpha    前景透明度
     *inparam   nBgAlpha    背景透明度
     * */
    int ( *rockitRgn_overlay_changeAlpha ) ( RkRgn_S* pHandle, int nFgAlpha, int nBgAlpha );
    
    /*改变显示的w h
     */
    int (*rockitrgn_change_rect) ( RkRgn_S* pHandle, int nWidth, int nHeight );

    /**
    * @Description: 获取RGN画布信息
    * @inparam pHandle rgn句柄封装指针
    * @outparam pstCanvasInfo 输出画布属性
    * @return: 成功返回0，失败返回RK_FAILURE
    */
    int (*rockitRgn_getCanvasInfo)(RkRgn_S* pHandle, RGN_CANVAS_INFO_S* pstCanvasInfo);

    /**
    * @Description: 提交RGN画布更新
    * @inparam pHandle rgn句柄封装指针
    * @return: 成功返回0，失败返回RK_FAILURE
    */
    int (*rockitRgn_updateCanvas)(RkRgn_S* pHandle);

    /* 更新区域
      * inparam pHandle 区域句柄
      * inparam stParam 必填参数
    */
    int (*RkRgn_update)(RkRgn_S *pHandle, RkRgnNeed_S stParam);


/************************必填参数****************************************/

        /* 必要参数 */
        RK_BOOL bIsShow;    /* 是否显示 */
        uint32_t unHandle;  /* 区域句柄 */
        uint32_t unOpFlag;  /* 操作标志（通道或者设备） */
        uint32_t unWidth;   /* Rgn宽度 */
        uint32_t unHeight;  /* Rgn高度 */
        uint32_t unModId;   /* 模式ID */
        uint32_t unDevId;   /* 设备ID */
        uint32_t unChnId;   /* 通道ID */
        uint32_t unType;    /* 功能类型 */
        RK_BOOL bUserColor; /* 是否自定义RGB 颜色值画板颜色 */
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
        RK_BOOL bIsFlicker;       /* 是否闪烁 */
        PIXEL_FORMAT_E enFormat; /* 像素格式 */
        /* cover可选参数 */
        POINT_S stuPoints[QUAD_POINT_NUM];
        RK_BOOL bIsRectangle; /* 是否是矩形 */
        RK_BOOL bIsSolid;     /* 是否实心 */
};

/*分配一个rgn句柄
 *inparam stParam 必填参数
 * */
RkRgn_S* rockitRgn_alloc( RkRgnNeed_S stParam );


/*分配一个rgn句柄
 *inparam stHandle 句柄
 * */
void rockitRgn_release(RkRgn_S* pHandle);


#ifdef __cplusplus
}
#endif
#endif
