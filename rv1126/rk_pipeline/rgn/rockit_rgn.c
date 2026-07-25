/*************************************************************************
	> File Name: rockit_rgn.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月31日 星期二 10时04分47秒
 ************************************************************************/

#include<stdio.h>
#include"rockit_rgn.h"
#include "rk_mpi_mb.h"

const RK_U32 u32BGRA8888ColorTblUser[256] = 
{                                             
    // transparency/gray/red/earthy yellow /blue
    0x00ffffff, 0xff5e6060, 0xffe9491e, 0xfff4bc1f, 0xff1ca2dd, 0xff87bd43,
    0xffff1f1f, 0xff0000af, 0xff0000d7, 0xff0000ff, 0xff005f00, 0xff005f5f,
    0xff005f87, 0xff005faf, 0xff005fd7, 0xff005fff, 0xff008000, 0xff008080,
    0xff008700, 0xff00875f, 0xff008787, 0xff0087af, 0xff0087d7, 0xff0087ff,
    0xff00af00, 0xff00af5f, 0xff00af87, 0xff00afaf, 0xff00afd7, 0xff00afff,
    0xff00d700, 0xff00d75f, 0xff00d787, 0xff00d7af, 0xff00d7d7, 0xff00d7ff,
    0xff00ff00, 0xff00ff28, 0xff00ff5f, 0xff00ff87, 0xff00ffaf, 0xff00ffd7,
    0xff00ffff, 0xff00ffff, 0xff080808, 0xff121212, 0xff1c1c1c, 0xff262626,
    0xff303030, 0xff3a3a3a, 0xff444444, 0xff4e4e4e, 0xff585858, 0xff5f0000,
    0xff5f005f, 0xff5f0087, 0xff5f00af, 0xff5f00d7, 0xff5f00ff, 0xff5f5f00,
    0xff5f5f5f, 0xff5f5f87, 0xff5f5faf, 0xff5f5fd7, 0xff5f5fff, 0xff5f8700,
    0xff5f875f, 0xff5f8787, 0xff5f87af, 0xff5f87d7, 0xff5f87ff, 0xff5faf00,
    0xff5faf5f, 0xff5faf87, 0xff5fafaf, 0xff5fafd7, 0xff5fafff, 0xff5fd700,
    0xff5fd75f, 0xff5fd787, 0xff5fd7af, 0xff5fd7d7, 0xff5fd7ff, 0xff5fff00,
    0xff5fff5f, 0xff5fff87, 0xff5fffaf, 0xff5fffd7, 0xff5fffff, 0xff626262,
    0xff6c6c6c, 0xff767676, 0xff800000, 0xff800080, 0xff808000, 0xff808080,
    0xff808080, 0xff870000, 0xff87005f, 0xff870087, 0xff8700af, 0xff8700d7,
    0xff8700ff, 0xff875f00, 0xff875f5f, 0xff875f87, 0xff875faf, 0xff875fd7,
    0xff875fff, 0xff878700, 0xff87875f, 0xff878787, 0xff8787af, 0xff8787d7,
    0xff8787ff, 0xff87af00, 0xff87af5f, 0xff87af87, 0xff87afaf, 0xff87afd7,
    0xff87afff, 0xff87d700, 0xff87d75f, 0xff87d787, 0xff87d7af, 0xff87d7d7,
    0xff87d7ff, 0xff87ff00, 0xff87ff5f, 0xff87ff87, 0xff87ffaf, 0xff87ffd7,
    0xff87ffff, 0xff8a8a8a, 0xff949494, 0xff9e9e9e, 0xffa8a8a8, 0xffaf0000,
    0xffaf005f, 0xffaf0087, 0xffaf00af, 0xffaf00d7, 0xffaf00ff, 0xffaf5f00,
    0xffaf5f5f, 0xffaf5f87, 0xffaf5faf, 0xffaf5fd7, 0xffaf5fff, 0xffaf8700,
    0xffaf875f, 0xffaf8787, 0xffaf87af, 0xffaf87d7, 0xffaf87ff, 0xffafaf00,
    0xffafaf5f, 0xffafaf87, 0xffafafaf, 0xffafafd7, 0xffafafff, 0xffafd700,
    0xffafd75f, 0xffafd787, 0xffafd7af, 0xffafd7d7, 0xffafd7ff, 0xffafff00,
    0xffafff5f, 0xffafff87, 0xffafffaf, 0xffafffd7, 0xffafffff, 0xffb2b2b2,
    0xffbcbcbc, 0xffc0c0c0, 0xffc6c6c6, 0xffd0d0d0, 0xffd70000, 0xffd7005f,
    0xffd70087, 0xffd700af, 0xffd700d7, 0xffd700ff, 0xffd75f00, 0xffd75f5f,
    0xffd75f87, 0xffd75faf, 0xffd75fd7, 0xffd75fff, 0xffd78700, 0xffd7875f,
    0xffd78787, 0xffd787af, 0xffd787d7, 0xffd787ff, 0xffd7af00, 0xffd7af5f,
    0xffd7af87, 0xffd7afaf, 0xffd7afd7, 0xffd7afff, 0xffd7d700, 0xffd7d75f,
    0xffd7d787, 0xffd7d7af, 0xffd7d7d7, 0xffd7d7ff, 0xffd7ff00, 0xffd7ff5f,
    0xffd7ff87, 0xffd7ffaf, 0xffd7ffd7, 0xffd7ffff, 0xffdadada, 0xffe4e4e4,
    0xffeeeeee, 0xffff0000, 0xffff0028, 0xffff005f, 0xffff0087, 0xffff00af,
    0xffff00d7, 0xffff00ff, 0xffff00ff, 0xffff5f00, 0xffff5f5f, 0xffff5f87,
    0xffff5faf, 0xffff5fd7, 0xffff5fff, 0xffff8700, 0xffff875f, 0xffff8787,
    0xffff87af, 0xffff87d7, 0xffff87ff, 0xffffaf00, 0xffffaf5f, 0xffffaf87,
    0xffffafaf, 0xffffafd7, 0xffffafff, 0xffffd700, 0xffffd75f, 0xffffd787,
    0xffffd7af, 0xffffd7d7, 0xffffd7ff, 0xffffff00, 0xffffff28, 0xffffff5f,
    0xffffff87, 0xffffffaf, 0xffffffd7, 0xffffffff,
};

/* 获取区域句柄号 */
static RGN_HANDLE RkRgn_get_minHandle(RGN_TYPE_E enType)
{
    RGN_HANDLE unMinHandle;

    switch (enType)
    {
    case OVERLAY_RGN:
        unMinHandle = OVERLAY_MIN_HANDLE;
        break;
    case OVERLAY_EX_RGN:
        unMinHandle = OVERLAYEX_MIN_HANDLE;
        break;
    case COVER_RGN:
        unMinHandle = COVER_MIN_HANDLE;
        break;
    case MOSAIC_RGN:
        unMinHandle = MOSAIC_MIN_HANDLE;
        break;
    case LINE_RGN:
        unMinHandle = LINE_MIN_HANDLE;
        break;
    default:
        unMinHandle = -1;
        break;
    }

    return unMinHandle;
}

/* 填充mpp通道属性 */
static int mpp_load_chnParam(RkRgn_S *pHandle, MPP_CHN_S *stChn)
{
    if (NULL == pHandle)
    {
       printf("mpp_load_chnParam error");
        return RK_FAILURE;
    }

    stChn->enModId = (MOD_ID_E)pHandle->unModId;
    stChn->s32DevId = pHandle->unDevId;
    stChn->s32ChnId = pHandle->unChnId;

    return RK_SUCCESS;
}

/*填充rgn属性*/
static int rgn_load_param( RkRgn_S* pHandle, RGN_ATTR_S* pRgnAttr)
{
    if(NULL == pHandle)
    {
        printf("rgn_load_param error\n");
        return RK_FAILURE;
    }
    if( pHandle->unType == OVERLAY_RGN )
    {
        pRgnAttr->enType = OVERLAY_RGN;
        pRgnAttr->unAttr.stOverlay.enPixelFmt = pHandle->enFormat;
        pRgnAttr->unAttr.stOverlay.stSize.u32Width  = RK_ALIGN_UP(pHandle->unWidth, RGN_MIN_WIDTH);
        pRgnAttr->unAttr.stOverlay.stSize.u32Height = RK_ALIGN_UP(pHandle->unHeight, RGN_MIN_HEIGHT);
        pRgnAttr->unAttr.stOverlay.u32CanvasNum = RGN_MAX_BUF_NUM;

        if (pHandle->enFormat == RK_FMT_BGRA5551 || pHandle->enFormat == RK_FMT_ARGB1555) 
        {
            /*16bit 格式下, 关闭 CLUT*/
            pRgnAttr->unAttr.stOverlay.u32ClutNum = 0;
            pHandle->bUserColor = RK_FALSE; 
        }
        else if (pHandle->enFormat == RK_FMT_2BPP ) 
        {
        }
        else 
        {
            pRgnAttr->unAttr.stOverlay.u32ClutNum = pHandle->bUserColor ? 255 : 0;
            memcpy(pRgnAttr->unAttr.stOverlay.u32Clut, u32BGRA8888ColorTblUser, sizeof(u32BGRA8888ColorTblUser));
        }
    }
    else if( pHandle->unType == COVER_RGN )
    {
        pRgnAttr->enType = COVER_RGN;
    }
    else if( pHandle->unType == MOSAIC_RGN )
    {
        pRgnAttr->enType = MOSAIC_RGN;
    }
    else if( pHandle->unType == LINE_RGN )
    {
        pRgnAttr->enType = LINE_RGN;
    }
    return 0;
}

/*填充rgn通道属性*/
static int rgn_load_chnParam( RkRgn_S* pHandle, RGN_CHN_ATTR_S* pRgnChnAttr )
{
    if(NULL == pHandle)
    {
        printf("rgn_load_chnParam error\n");
        return RK_FAILURE;
    }
    if( pHandle->unType == OVERLAY_RGN )
    {
        /*区域通道属性*/
        pRgnChnAttr->bShow  = RK_TRUE;
        pRgnChnAttr->enType = OVERLAY_RGN;
        pRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32X            = pHandle->unStartX;
        pRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y            = pHandle->unStartY;

        /* 背景（BgAlpha）0是透明，否则会遮挡视频*/
        /* 前景（FgAlpha）255是不透明的，否则看不见画的线*/
        if (pHandle->unLayer == ELEMENT_TYPE_PEOPLE_AI) {
            pRgnChnAttr->unChnAttr.stOverlayChn.u32BgAlpha = 0;   /* 背景全透*/
            pRgnChnAttr->unChnAttr.stOverlayChn.u32FgAlpha = 255; /* 线条不透*/
            pRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32X      = 0;
            pRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y      = 0;
            pRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bEnable        = RK_FALSE;
            pRgnChnAttr->unChnAttr.stOverlayChn.u32ColorLUT[0] = 0x00FF00; // 绿色 (对应内存 0xAA)
            pRgnChnAttr->unChnAttr.stOverlayChn.u32ColorLUT[1] = 0xFF0000; // 红色 (对应内存 0xFF)

        } 
        else 
        {
            pRgnChnAttr->unChnAttr.stOverlayChn.u32BgAlpha = pHandle->unBgAlpha;
            pRgnChnAttr->unChnAttr.stOverlayChn.u32FgAlpha = pHandle->unFgAlpha;

            pRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bEnable        = RK_TRUE;
            pRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bForceIntra    = RK_FALSE;
            pRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bAbsQp         = RK_FALSE;
            pRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.s32Qp          = -20;
        }
        pRgnChnAttr->unChnAttr.stOverlayChn.u32Layer                = pHandle->unLayer;

    }
    else if( pHandle->unType == COVER_RGN  )
    {
        /*区域通道属性*/
        pRgnChnAttr->bShow  = RK_TRUE;
        pRgnChnAttr->enType = COVER_RGN;
        pRgnChnAttr->unChnAttr.stCoverChn.u32Color = pHandle->unBgColor;
        pRgnChnAttr->unChnAttr.stCoverChn.u32Layer = pHandle->unLayer;
        pRgnChnAttr->unChnAttr.stCoverChn.enCoordinate =RGN_ABS_COOR;
        pRgnChnAttr->unChnAttr.stCoverChn.stQuadRangle.bSolid = pHandle->bIsSolid;
        pRgnChnAttr->unChnAttr.stCoverChn.enCoverType = pHandle->bIsRectangle ?  AREA_RECT : AREA_QUAD_RANGLE;

        /*矩形*/
        if ( pRgnChnAttr->unChnAttr.stCoverChn.enCoverType == AREA_RECT ) 
        {
            pRgnChnAttr->unChnAttr.stCoverChn.stRect.s32X = pHandle->unStartX;
            pRgnChnAttr->unChnAttr.stCoverChn.stRect.s32Y = pHandle->unStartY;
            pRgnChnAttr->unChnAttr.stCoverChn.stRect.u32Width = pHandle->unWidth;
            pRgnChnAttr->unChnAttr.stCoverChn.stRect.u32Height = pHandle->unHeight;
        }
        /*任意四边形*/
        else if( pRgnChnAttr->unChnAttr.stCoverChn.enCoverType == AREA_QUAD_RANGLE ) 
        {
            for (int i = 0; i < QUAD_POINT_NUM; i++)
            {
                pRgnChnAttr->unChnAttr.stCoverChn.stQuadRangle.stPoint[i].s32X = pHandle->stuPoints[i].s32X;
                pRgnChnAttr->unChnAttr.stCoverChn.stQuadRangle.stPoint[i].s32Y = pHandle->stuPoints[i].s32Y;
            }
        }
    }
    else if(  pHandle->unType == MOSAIC_RGN   )
    {
        /*区域通道属性*/
        pRgnChnAttr->bShow = RK_TRUE;
        pRgnChnAttr->enType = MOSAIC_RGN;
        pRgnChnAttr->unChnAttr.stMosaicChn.stRect.s32X = pHandle->unStartX;
        pRgnChnAttr->unChnAttr.stMosaicChn.stRect.s32Y = pHandle->unStartY;
        pRgnChnAttr->unChnAttr.stMosaicChn.stRect.u32Width = pHandle->unWidth;
        pRgnChnAttr->unChnAttr.stMosaicChn.stRect.u32Height = pHandle->unHeight;
        pRgnChnAttr->unChnAttr.stMosaicChn.enBlkSize = MOSAIC_BLK_SIZE_16;
        pRgnChnAttr->unChnAttr.stMosaicChn.u32Layer = pHandle->unLayer;
    }
    else if(  pHandle->unType == LINE_RGN   )
    {
        /*区域通道属性*/
        pRgnChnAttr->bShow = RK_TRUE;
        pRgnChnAttr->enType = LINE_RGN;
        pRgnChnAttr->unChnAttr.stLineChn.stStartPoint.s32X = pHandle->stuPoints[0].s32X;
        pRgnChnAttr->unChnAttr.stLineChn.stStartPoint.s32Y = pHandle->stuPoints[0].s32Y;
        pRgnChnAttr->unChnAttr.stLineChn.stEndPoint.s32X = pHandle->stuPoints[1].s32X;
        pRgnChnAttr->unChnAttr.stLineChn.stEndPoint.s32Y = pHandle->stuPoints[1].s32Y;
        pRgnChnAttr->unChnAttr.stLineChn.u32Color = pHandle->unBgColor;
        pRgnChnAttr->unChnAttr.stLineChn.u32Thick = RGN_LINE_MAX_THICK/2;
    }
    return 0;
}


/*创建一个RGN
*inparam pHandle rgn句柄
* */
static int rockitRgn_create( RkRgn_S *pHandle )
{
    if(NULL == pHandle)
    {
        printf("rockit create error\n");
        return RK_FAILURE;
    }
    int nRet;
    /*rgn属性*/
    RGN_ATTR_S      stRgnAttr;
    memset(&stRgnAttr, 0, sizeof(RGN_ATTR_S));
    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /*填充属性*/
    rgn_load_param( pHandle, &stRgnAttr);

    nRet = RK_MPI_RGN_Create( unHandle, &stRgnAttr);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_Create (%d) failed with %#x!", unHandle, nRet);
        RK_MPI_RGN_Destroy( unHandle );
        return RK_FAILURE;
    }

    return 0;
}

/*释放rgn
 *inparam pHandle rgn句柄
 * */
static int rockitRgn_destroy( RkRgn_S* pHandle )
{
    if(NULL == pHandle)
    {
        printf("rockit destroy error\n");
        return RK_FAILURE;
    }
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;
    int nRet =  RK_MPI_RGN_Destroy( unHandle );
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_Destroy (%d) failed with %#x!", unHandle, nRet);
        
    }
    return 0;
}

/*将RGN区域叠加到通道上
    *inparam pHandle rgn句柄
    * */
static int rockitRgn_attachToChn( RkRgn_S* pHandle )
{
    if(NULL == pHandle)
    {
        printf("rockitRgn_attachToChn error\n");
        return RK_FAILURE;
    }
    int nRet;
    MPP_CHN_S stMppChn;
    RGN_CHN_ATTR_S stRgnChnAttr;
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));
    memset(&stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));

    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;
    
    mpp_load_chnParam(pHandle, &stMppChn);
    rgn_load_chnParam( pHandle, &stRgnChnAttr );

    nRet = RK_MPI_RGN_AttachToChn(unHandle, &stMppChn, &stRgnChnAttr);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_AttachToChn (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    return 0;
}

/*将RGN区域从通道上撤出
    *inparam pHandle rgn句柄
    * */
static int rockitRgn_detachFromChn( RkRgn_S* pHandle )
{
    if(NULL == pHandle)
    {
        printf("rockitRgn_detachFromChn error\n");
        return RK_FAILURE;
    }
    int nRet;

    MPP_CHN_S stMppChn;
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));


    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;
    
    mpp_load_chnParam(pHandle, &stMppChn);

    nRet = RK_MPI_RGN_DetachFromChn( unHandle, &stMppChn);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_DetachFromChn (%d) failed with %#x!", unHandle, nRet );
        return RK_FAILURE;
    }

    return 0;
}

/*overLay导入图片数据
 *inparam   pHandle     句柄
 *inparam   pParam    图片数据或者用户参数
 *inparam   nSize     数据大小
 * */
static int rockitRgn_overlay_loadPic( RkRgn_S* pHandle, void* pParam ,int nSize)
{

    if(NULL == pHandle)
    {
        printf("rkRgn handle NULL\n");
        return RK_FAILURE;
    }

    if(NULL == pParam)
    {
        printf("pic data NULL\n");
        return RK_FAILURE;
    }

    int nRet = 0;
    RGN_CANVAS_INFO_S stCanvasInfo;
    memset(&stCanvasInfo, 0, sizeof(RGN_CANVAS_INFO_S));
    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    rockitRgn_detachFromChn(pHandle );

    /* 获取区域的显示画布信息 */
    nRet = RK_MPI_RGN_GetCanvasInfo( unHandle, &stCanvasInfo);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_GetCanvasInfo (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    /* 复制图片数据 */
    memcpy( (void*)stCanvasInfo.u64VirAddr, pParam, nSize );
     
    /*更新显示画布*/
    nRet = RK_MPI_RGN_UpdateCanvas( unHandle );
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_UpdateCanvas (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    rockitRgn_attachToChn( pHandle );

    return RK_SUCCESS;

}


/* 清空区域上的贴图 */
static int rockitRgn_clearPicture(RkRgn_S *pHandle)
{
    if (NULL == pHandle)
    {
        printf("传入参数错误");
        return RK_FAILURE;
    }

    int nRet = 0;
    RGN_CANVAS_INFO_S stCanvasInfo;
    memset(&stCanvasInfo, 0, sizeof(RGN_CANVAS_INFO_S));
    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 获取区域的显示画布信息 */
    nRet = RK_MPI_RGN_GetCanvasInfo( unHandle, &stCanvasInfo);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_GetCanvasInfo (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    /* 计算画布字节大小 */
    uint32_t unCanvasSize = 0;
    if(pHandle->enFormat == RK_FMT_ARGB1555 || pHandle->enFormat == RK_FMT_BGRA5551){
        unCanvasSize = stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight * 2 ;   
    }
    if(pHandle->enFormat ==  RK_FMT_BGR888){
        unCanvasSize = stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight * 3 ;   
    }
    else if(pHandle->enFormat == RK_FMT_BGRA8888){
        unCanvasSize = stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight * 4 ;   
    }
    else if(pHandle->enFormat == RK_FMT_2BPP){
        unCanvasSize = stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight >> 2;   
    }
    else{
        unCanvasSize = stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight;        
    }

    /* 将画布内容清零 - 实现透明背景 */
    if (stCanvasInfo.u64VirAddr)
    {
        memset((void*)stCanvasInfo.u64VirAddr, 0, unCanvasSize);
    }

    /* 更新显示画布 */
    nRet = RK_MPI_RGN_UpdateCanvas( unHandle );
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_UpdateCanvas (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    return RK_SUCCESS;
}


/*改变rgn的绑定通道
 *inparam pHandle 句柄
 *inparam enModId 模块号 -1不改变 ( MOD_ID_E )
 *inparam nDevId 设备号  -1不改变
 *inparam nChnId 通道号  
 *inparam nOldChnId 旧通道号
 * */
static int rockitRgn_changbind ( RkRgn_S* pHandle, int nModId, int nDevId, int nChnId )
{
    if(NULL == pHandle)
    {
        printf("rockitRgn changbind error\n");
        return RK_FAILURE;
    }
    int nRet = 0;
    MPP_CHN_S stMppChn;
	RGN_CHN_ATTR_S stChnAttr;
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));
    memset(&stChnAttr,0,sizeof(RGN_CHN_ATTR_S));
     
    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    mpp_load_chnParam(pHandle, &stMppChn);
    rgn_load_chnParam(pHandle, &stChnAttr);
    
    /*获取通道的显示属性*/
    nRet = RK_MPI_RGN_GetDisplayAttr( unHandle, &stMppChn, &stChnAttr);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }   
    /*解绑*/
    nRet = RK_MPI_RGN_DetachFromChn( unHandle, &stMppChn);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_AttachToChn (%d) failed with %#x!", unHandle, nRet);
        return RK_FAILURE;
    }
    
    /*赋值*/
    if(  -1 != nModId )
    {
        stMppChn.enModId = (MOD_ID_E)nModId;
    }
    if( -1 == nDevId )
    {
        stMppChn.s32DevId = nDevId;
    }
    stMppChn.s32ChnId = nChnId;
    
    /*绑定*/
    nRet = RK_MPI_RGN_AttachToChn( unHandle, &stMppChn, &stChnAttr);
    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_AttachToChn (%d) failed with %#x!", unHandle, nRet);
        return RK_FAILURE;
    }

    return nRet;
}

/*改变rgn区域的位置
 *inparam   pHandle     句柄
 *inparam   pX          x方向位置
 *inparam   pY          y方向位置
 *inparam   nSize       表示px py数组的长度，
                        画线要填写2 起始点和终点
                        在可选参数遮挡选择任意四边形时 填写4
 *inparam   nChn        要改变的通道号
* */
static int rockitRgn_changePos( RkRgn_S* pHandle, int nStartX, int nStartY)
{
    if (RK_NULL == pHandle) {
        printf("input parameter is null. it is invalid!");
        return RK_FAILURE;
    }   

    int nRet = 0;
    MPP_CHN_S stMppChn;
	RGN_CHN_ATTR_S stChnAttr;
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));
    memset(&stChnAttr,0,sizeof(RGN_CHN_ATTR_S));

    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    mpp_load_chnParam(pHandle, &stMppChn);
    rgn_load_chnParam(pHandle, &stChnAttr);
    
    /*获取通道的显示属性*/
    nRet = RK_MPI_RGN_GetDisplayAttr( unHandle, &stMppChn, &stChnAttr); 
    if (RK_SUCCESS != nRet) 
    {
        printf(" x y RK_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }   

    switch (stChnAttr.enType) 
    {
        case OVERLAY_RGN: 
        {
            stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = nStartX;
            stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = nStartY;
        } 
        break;
        case COVER_RGN: 
        {
            /*矩形*/
            if( stChnAttr.unChnAttr.stCoverChn.enCoverType == AREA_RECT  )
            {
                stChnAttr.unChnAttr.stCoverChn.stRect.s32X = nStartX;
                stChnAttr.unChnAttr.stCoverChn.stRect.s32Y = nStartY;
            }
            /*任意四边形*/
            else if( stChnAttr.unChnAttr.stCoverChn.enCoverType == AREA_QUAD_RANGLE ) 
            {
                for (int i = 0; i < QUAD_POINT_NUM; i++)
                {
                stChnAttr.unChnAttr.stCoverChn.stQuadRangle.stPoint[i].s32X = pHandle->stuPoints[i].s32X;
                stChnAttr.unChnAttr.stCoverChn.stQuadRangle.stPoint[i].s32Y = pHandle->stuPoints[i].s32Y;
                }
            }
        } 
        break;
        case MOSAIC_RGN: 
        {
            stChnAttr.unChnAttr.stMosaicChn.stRect.s32X = nStartX;
            stChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = nStartY;
        } 
        break;
        case LINE_RGN: 
        {
            stChnAttr.unChnAttr.stLineChn.stStartPoint.s32X = pHandle->stuPoints[0].s32X;
            stChnAttr.unChnAttr.stLineChn.stStartPoint.s32Y = pHandle->stuPoints[0].s32Y;
            stChnAttr.unChnAttr.stLineChn.stEndPoint.s32X   = pHandle->stuPoints[1].s32X;
            stChnAttr.unChnAttr.stLineChn.stEndPoint.s32Y   = pHandle->stuPoints[1].s32Y;
        } 
        break;
        default:
        break;
    }   

    /*设置通道的显示属性*/
    nRet = RK_MPI_RGN_SetDisplayAttr( unHandle, &stMppChn, &stChnAttr);

    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!", unHandle, nRet);
        return RK_FAILURE;
    }   

    return nRet;
}

/*显示或者隐藏rgn
*inparam   pHandle 句柄
*inparam   bShow   1显示 0隐藏
*inparam   nChn    要显示或者隐藏的通道号
* */
static int rockitRgn_showOrHide( RkRgn_S* pHandle, RK_BOOL bShow)
{
    if (RK_NULL == pHandle) {
        printf("input parameter is null. it is invalid!");
        return RK_FAILURE;
    }   

    int nRet = 0;
    MPP_CHN_S stMppChn;
	RGN_CHN_ATTR_S stChnAttr;
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));
    memset(&stChnAttr,0,sizeof(RGN_CHN_ATTR_S));

    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    mpp_load_chnParam(pHandle, &stMppChn);
    rgn_load_chnParam(pHandle, &stChnAttr);

    nRet = RK_MPI_RGN_GetDisplayAttr( unHandle, &stMppChn, &stChnAttr);
    if (RK_SUCCESS != nRet) 
    {
        printf("isshow RK_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }   

    stChnAttr.bShow = bShow;
    
    nRet = RK_MPI_RGN_SetDisplayAttr( unHandle, &stMppChn, &stChnAttr);

    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!", unHandle, nRet);
        return RK_FAILURE;
    }   

    return RK_SUCCESS;
}

/*改变overlay的前景背景的透明度
 *inparam   pHandle     句柄
 *inparam   nFgAlpha    前景透明度
 *inparam   nBgAlpha    背景透明度
 *inparam   nChn        要改变overlay的前景背景的透明度的通道号
 * */
static int rockitRgn_overlay_changeAlpha( RkRgn_S* pHandle, int nFgAlpha, int nBgAlpha )
{
    if (RK_NULL == pHandle) {
        printf("input parameter is null. it is invalid!");
        return RK_FAILURE;
    }   

    int nRet = 0;
    MPP_CHN_S stMppChn;
	RGN_CHN_ATTR_S stChnAttr;
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));
    memset(&stChnAttr,0,sizeof(RGN_CHN_ATTR_S));

    /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    mpp_load_chnParam(pHandle, &stMppChn);
    rgn_load_chnParam(pHandle, &stChnAttr);

    nRet = RK_MPI_RGN_GetDisplayAttr( unHandle, &stMppChn, &stChnAttr);
    if (RK_SUCCESS != nRet) 
    {
        printf("alpha RK_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }   
    
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = nFgAlpha;
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = nBgAlpha;
    
    nRet = RK_MPI_RGN_SetDisplayAttr( unHandle, &stMppChn, &stChnAttr);

    if (RK_SUCCESS != nRet) 
    {
        printf("RK_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }   
    return RK_SUCCESS;

}


/*改变显示的宽高*/
static int rockitrgn_change_rect( RkRgn_S* pHandle, int nWidth, int nHeight )
{
    if (RK_NULL == pHandle) {
        printf("rockitrgn_change_rect input parameter is null. it is invalid!");
        return RK_FAILURE;
    } 

    int nRet = 0;
    RGN_ATTR_S  stRgnAttr;
    MPP_CHN_S stMppChn;
    RGN_CHN_ATTR_S stRgnChnAttr;
    memset(&stRgnAttr, 0, sizeof(RGN_ATTR_S));
    memset(&stMppChn, 0, sizeof(MPP_CHN_S));
    memset(&stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));

    pHandle->unWidth = nWidth;
    pHandle->unHeight = nHeight;

     /* 区域句柄号 */
    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;
    
    mpp_load_chnParam(pHandle, &stMppChn);
    rgn_load_chnParam( pHandle, &stRgnChnAttr );

    /* 修改尺寸 */
    if (OVERLAY_RGN == pHandle->unType)
    {

        nRet = RK_MPI_RGN_GetAttr(unHandle, &stRgnAttr);
        if (RK_SUCCESS != nRet) 
        {
            printf("RK_MPI_RGN_GetAttr (%d)) failed with 0X%x!, change rect\n", unHandle, nRet);
            return RK_FAILURE;
        }
         
        /*相等，直接返回*/
        if(stRgnAttr.unAttr.stOverlay.stSize.u32Width  == pHandle->unWidth && stRgnAttr.unAttr.stOverlay.stSize.u32Height == pHandle->unHeight)
        {
            return RK_SUCCESS;
        }


        nRet = RK_MPI_RGN_DetachFromChn( unHandle, &stMppChn);
        if (RK_SUCCESS != nRet) 
        {
            printf("RGN_DetachFromChn (%d)) failed with 0X%x!, change rect\n", unHandle, nRet);
            return RK_FAILURE;
        }

        nRet = rockitRgn_destroy( pHandle );
        if (RK_SUCCESS != nRet) 
        {
            printf("rockitRgn_destroy (%d) failed with 0x%x! change rect\n", unHandle, nRet );
            return RK_FAILURE;
        }
        

        nRet = rockitRgn_create( pHandle );
        if (RK_SUCCESS != nRet) 
        {
            printf("rockitRgn_create (%d) failed with 0x%x! change rect\n", unHandle, nRet );
            return RK_FAILURE;
        }

        nRet = RK_MPI_RGN_AttachToChn( unHandle, &stMppChn, &stRgnChnAttr);
        if (RK_SUCCESS != nRet) 
        {
            printf("RK_MPI_RGN_AttachToChn (%d) failed with 0x%x! change rect\n", unHandle, nRet );
            return RK_FAILURE;
        }
    }
    else if (COVER_RGN == pHandle->unType)
    {
         nRet = RK_MPI_RGN_GetDisplayAttr( unHandle, &stMppChn, &stRgnChnAttr);
        if (RK_SUCCESS != nRet) 
        {
            printf("alpha RK_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
            return RK_FAILURE;
        }   
        
        rgn_load_chnParam( pHandle, &stRgnChnAttr );

        nRet = RK_MPI_RGN_SetDisplayAttr( unHandle, &stMppChn, &stRgnChnAttr);
            if (RK_SUCCESS != nRet) 
        {
            printf("alpha RK_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n", unHandle, nRet);
            return RK_FAILURE;
        }  
   
    }

    return 0;
}

/* 获取RGN画布信息
 * @inparam pHandle rgn句柄结构体
 * @outparam pstCanvasInfo 画布信息结构体
*/
static int rockitRgn_getCanvasInfo(RkRgn_S* pHandle, RGN_CANVAS_INFO_S* pstCanvasInfo)
{
    if (NULL == pHandle || NULL == pstCanvasInfo)
    {
        printf("rockit getCanvasInfo error: pointer is null\n");
        return RK_FAILURE;
    }

    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    int nRet = RK_MPI_RGN_GetCanvasInfo(unHandle, pstCanvasInfo);
    if (RK_SUCCESS != nRet)
    {
        printf("RK_MPI_RGN_GetCanvasInfo (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    return 0;
}

/* 提交RGN画布更新
 * @inparam pHandle rgn句柄结构体
*/
static int rockitRgn_updateCanvas(RkRgn_S* pHandle)
{
    if (NULL == pHandle)
    {
        printf("rockit updateCanvas error: pointer is null\n");
        return RK_FAILURE;
    }

    RGN_HANDLE unHandle = RkRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    int nRet = RK_MPI_RGN_UpdateCanvas(unHandle);
    if (RK_SUCCESS != nRet)
    {
        printf("RK_MPI_RGN_UpdateCanvas (%d) failed with %#x!\n", unHandle, nRet);
        return RK_FAILURE;
    }

    return 0;
}

/* 更新区域 */
static int RkRgn_update(RkRgn_S *pHandle, RkRgnNeed_S stParam)
{
    if(pHandle ==NULL)
    {
        printf("RkRgn_update pHandle is NULL\n");
        return RK_FAILURE;
    }
        

    pHandle->bIsShow = stParam.bIsShow;
    pHandle->unHandle = stParam.unHandle;
    pHandle->unOpFlag = stParam.unOpFlag;
    pHandle->unModId = stParam.unModId;
    pHandle->unDevId = stParam.unDevId;
    pHandle->unChnId = stParam.unChnId;
    pHandle->unType = stParam.unType;
    pHandle->unLayer = stParam.unLayer;
    pHandle->unBgColor = stParam.unBgColor;

    //RK_LOGE("unHandle:%d--unModId:%d--unDevId:%d--unChnId:%d--unLayer:%d",stParam.unHandle,stParam.unModId,stParam.unDevId,stParam.unChnId,stParam.unLayer);

    if (OVERLAY_RGN == pHandle->unType)
    {
        pHandle->unStartX = stParam.unStartX;
        pHandle->unStartY = stParam.unStartY;
        pHandle->unWidth = stParam.unWidth;
        pHandle->unHeight = stParam.unHeight;

        pHandle->unFgColor = stParam.unFgColor;
        pHandle->unFgAlpha = stParam.unFgAlpha;
        pHandle->unBgAlpha = stParam.unBgAlpha;

        pHandle->unFontSize = stParam.unFontSize;
        pHandle->unHorMargin = stParam.unHorMargin;
        pHandle->unVerMargin = stParam.unVerMargin;
        pHandle->bIsFlicker = stParam.bIsFlicker;
        pHandle->bUserColor = RK_TRUE;
        
        if(pHandle->unLayer == ELEMENT_TYPE_PEOPLE_AI )
            //pHandle->enFormat = RK_FMT_BGRA5551;
            pHandle->enFormat = RK_FMT_2BPP;
        else
            pHandle->enFormat = RK_FMT_BGRA8888;
    }
    else if (COVER_RGN == pHandle->unType )
    {
        pHandle->bIsRectangle = stParam.bIsRectangle;
        if (stParam.bIsRectangle)
        {
            pHandle->unStartX = stParam.unStartX;
            pHandle->unStartY = stParam.unStartY;
            pHandle->unWidth = stParam.unWidth;
            pHandle->unHeight = stParam.unHeight;
        }
        else
        {
            for (int i = 0; i < QUAD_POINT_NUM; i++)
            {
                pHandle->stuPoints[i].s32X = stParam.stuPoints[i].s32X;
                pHandle->stuPoints[i].s32Y = stParam.stuPoints[i].s32Y;
            }
        }

        pHandle->bIsSolid = stParam.bIsSolid;

        pHandle->enFormat = RK_FMT_BGR888;
    }

    return RK_SUCCESS;
}

/*分配一个rgn句柄
 *inparam stParam 必填参数
 * */
RkRgn_S* rockitRgn_alloc( RkRgnNeed_S stParam )
{
    RkRgn_S *pHandle = ( RkRgn_S* ) malloc ( sizeof(RkRgn_S) );
    memset(pHandle, 0, sizeof(RkRgn_S));

    RkRgn_update(pHandle, stParam);

    pHandle->rockitRgn_create                   = rockitRgn_create;
    pHandle->rockitRgn_destroy                  = rockitRgn_destroy;
    pHandle->rockitRgn_attachToChn              = rockitRgn_attachToChn;
    pHandle->rockitRgn_detachFromChn            = rockitRgn_detachFromChn;
    pHandle->rockitRgn_overlay_loadPic          = rockitRgn_overlay_loadPic;
    pHandle->rockitRgn_clearPicture             = rockitRgn_clearPicture;
    pHandle->rockitRgn_changbind                = rockitRgn_changbind;
    pHandle->rockitRgn_changePos                = rockitRgn_changePos;
    pHandle->rockitRgn_showOrHide               = rockitRgn_showOrHide;
    pHandle->rockitRgn_overlay_changeAlpha      = rockitRgn_overlay_changeAlpha;
    pHandle->rockitrgn_change_rect              = rockitrgn_change_rect;
    pHandle->rockitRgn_getCanvasInfo            = rockitRgn_getCanvasInfo;
    pHandle->rockitRgn_updateCanvas             = rockitRgn_updateCanvas;
    pHandle->RkRgn_update                       = RkRgn_update;

    return pHandle;
}

/*释放rgn句柄
 *inparam stHandle 句柄
 * */
void rockitRgn_release(RkRgn_S* pHandle)
{
    if(pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}












