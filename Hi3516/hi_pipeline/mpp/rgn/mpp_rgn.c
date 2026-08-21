/**
 * @FilePath     : mpp_rgn.c
 * @Author       : zhouzirui
 * @Date         : 2025-05-08 16:35:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-30 15:14:08
 * @Description  : 区域管理
 */

#include "mpp_rgn.h"
#include "mpi_common.h"

/* 填充区域属性 */
static int rgn_load_param(HiRgn_S *pHandle, ot_rgn_attr *pRgnAttr)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("rgn_load_param error");
        return TD_FAILURE;
    }

    if (OT_RGN_OVERLAY == pHandle->unType)
    {
        pRgnAttr->type = pHandle->unType;
        pRgnAttr->attr.overlay.pixel_format = pHandle->enFormat;
        if (OT_PIXEL_FORMAT_ARGB_1555 == pHandle->enFormat || OT_PIXEL_FORMAT_ARGB_4444 == pHandle->enFormat)
        {
            pRgnAttr->attr.overlay.bg_color = 0xFFFF;
        }
        else
        {
            pRgnAttr->attr.overlay.bg_color = pHandle->unBgColor;
        }
        pRgnAttr->attr.overlay.size.width = MPI_ALIGN_UP(pHandle->unWidth, OT_RGN_ALIGN);
        pRgnAttr->attr.overlay.size.height = MPI_ALIGN_UP(pHandle->unHeight, OT_RGN_ALIGN);
        pRgnAttr->attr.overlay.canvas_num = 1; /* 区域的画布数量, OVERLAY支持1 */
        // pRgnAttr->attr.overlay.clut = 0;
    }
    else if (OT_RGN_COVER == pHandle->unType || OT_RGN_COVEREX == pHandle->unType
             || OT_RGN_CORNER_RECT == pHandle->unType || OT_RGN_CORNER_RECTEX == pHandle->unType)
    {
        pRgnAttr->type = pHandle->unType;
    }

    return TD_SUCCESS;
}

/* 填充区域通道属性 */
static int rgn_load_chnParam(HiRgn_S *pHandle, ot_rgn_chn_attr *pChnAttr)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("rgn_load_chnParam error");
        return TD_FAILURE;
    }

    pChnAttr->type = pHandle->unType;
    pChnAttr->is_show = pHandle->bIsShow;
    if (OT_RGN_OVERLAY == pHandle->unType)
    {
        pChnAttr->attr.overlay_chn.point.x = MPI_ALIGN_UP(pHandle->unStartX, OT_RGN_ALIGN);
        pChnAttr->attr.overlay_chn.point.y = MPI_ALIGN_UP(pHandle->unStartY, OT_RGN_ALIGN);
        pChnAttr->attr.overlay_chn.fg_alpha = pHandle->unFgAlpha;
        pChnAttr->attr.overlay_chn.bg_alpha = pHandle->unBgAlpha;
        pChnAttr->attr.overlay_chn.layer = pHandle->unLayer;
        pChnAttr->attr.overlay_chn.qp_info.enable = TD_FALSE;
        pChnAttr->attr.overlay_chn.qp_info.is_abs_qp = TD_FALSE;
        pChnAttr->attr.overlay_chn.qp_info.qp_val = 0;
        pChnAttr->attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;
    }
    else if (OT_RGN_COVER == pHandle->unType)
    {
        pChnAttr->attr.cover_chn.cover.type = pHandle->bIsRectangle ? OT_COVER_RECT : OT_COVER_QUAD;
        if (OT_COVER_RECT == pChnAttr->attr.cover_chn.cover.type)
        {
            pChnAttr->attr.cover_chn.cover.rect_attr.is_solid = pHandle->bIsSolid;
            pChnAttr->attr.cover_chn.cover.rect_attr.thick = OT_RGN_COVER_MIN_THICK;
            pChnAttr->attr.cover_chn.cover.rect_attr.rect.x = pHandle->unStartX;
            pChnAttr->attr.cover_chn.cover.rect_attr.rect.y = pHandle->unStartY;
            pChnAttr->attr.cover_chn.cover.rect_attr.rect.width = pHandle->unWidth;
            pChnAttr->attr.cover_chn.cover.rect_attr.rect.height = pHandle->unHeight;
        }
        else if (OT_COVER_QUAD == pChnAttr->attr.cover_chn.cover.type)
        {
            pChnAttr->attr.cover_chn.cover.quad_attr.is_solid = pHandle->bIsSolid;
            pChnAttr->attr.cover_chn.cover.quad_attr.thick = OT_RGN_COVER_MIN_THICK;
            for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
            {
                pChnAttr->attr.cover_chn.cover.quad_attr.point[i].x = pHandle->stuPoints[i].x;
                pChnAttr->attr.cover_chn.cover.quad_attr.point[i].y = pHandle->stuPoints[i].y;
            }
        }
        pChnAttr->attr.cover_chn.cover.color = pHandle->unBgColor;
        pChnAttr->attr.cover_chn.layer = pHandle->unLayer;
        pChnAttr->attr.cover_chn.coord = OT_COORD_ABS;
    }
    else if (OT_RGN_COVEREX == pHandle->unType)
    {
        pChnAttr->attr.coverex_chn.coverex.type = pHandle->bIsRectangle ? OT_COVER_RECT : OT_COVER_QUAD;
        if (OT_COVER_RECT == pChnAttr->attr.coverex_chn.coverex.type)
        {
            pChnAttr->attr.coverex_chn.coverex.rect_attr.is_solid = pHandle->bIsSolid;
            pChnAttr->attr.coverex_chn.coverex.rect_attr.thick = OT_RGN_COVER_MIN_THICK;
            pChnAttr->attr.coverex_chn.coverex.rect_attr.rect.x = pHandle->unStartX;
            pChnAttr->attr.coverex_chn.coverex.rect_attr.rect.y = pHandle->unStartY;
            pChnAttr->attr.coverex_chn.coverex.rect_attr.rect.width = pHandle->unWidth;
            pChnAttr->attr.coverex_chn.coverex.rect_attr.rect.height = pHandle->unHeight;
        }
        else if (OT_COVER_QUAD == pChnAttr->attr.coverex_chn.coverex.type)
        {
            pChnAttr->attr.coverex_chn.coverex.quad_attr.is_solid = pHandle->bIsSolid;
            pChnAttr->attr.coverex_chn.coverex.quad_attr.thick = OT_RGN_COVER_MIN_THICK;
            for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
            {
                pChnAttr->attr.coverex_chn.coverex.quad_attr.point[i].x = pHandle->stuPoints[i].x;
                pChnAttr->attr.coverex_chn.coverex.quad_attr.point[i].y = pHandle->stuPoints[i].y;
            }
        }
        pChnAttr->attr.coverex_chn.coverex.color = pHandle->unBgColor;
        pChnAttr->attr.coverex_chn.layer = pHandle->unLayer;
        pChnAttr->attr.coverex_chn.coord = OT_COORD_ABS;
    }
    else if (OT_RGN_CORNER_RECT == pHandle->unType || OT_RGN_CORNER_RECTEX == pHandle->unType)
    {
        pChnAttr->attr.corner_rect_chn.corner_rect.rect.x = pHandle->unStartX; /* 起始位置 */
        pChnAttr->attr.corner_rect_chn.corner_rect.rect.y = pHandle->unStartY;
        pChnAttr->attr.corner_rect_chn.corner_rect.rect.width = pHandle->unWidth; /* 宽高信息 */
        pChnAttr->attr.corner_rect_chn.corner_rect.rect.height = pHandle->unHeight;
        pChnAttr->attr.corner_rect_chn.corner_rect.hor_len = pHandle->uHorLen; /* 角框水平线长 */
        pChnAttr->attr.corner_rect_chn.corner_rect.ver_len = pHandle->uVerLen; /* 角框竖直线长 */
        pChnAttr->attr.corner_rect_chn.corner_rect.thick = pHandle->uThick;    /* 角框线宽 */
        pChnAttr->attr.corner_rect_chn.corner_rect_attr.corner_rect_type = OT_CORNER_RECT_TYPE_FULL_LINE; /* 角框形状 */
        pChnAttr->attr.corner_rect_chn.corner_rect_attr.color = pHandle->unFgColor; /* 角框颜色 */
        pChnAttr->attr.corner_rect_chn.layer = pHandle->unLayer;                    /* 区域层级 */
    }

    return TD_SUCCESS;
}

/* 填充mpp通道属性 */
static int mpp_load_chnParam(HiRgn_S *pHandle, ot_mpp_chn *stChn)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("mpp_load_chnParam error");
        return TD_FAILURE;
    }

    stChn->mod_id = pHandle->unModId;
    stChn->dev_id = pHandle->unDevId;
    stChn->chn_id = pHandle->unChnId;

    return TD_SUCCESS;
}

/* 填充mpp通道属性 */
td_s32 mppRgn_get_minHandle(ot_rgn_type enType)
{
    td_s32 unMinHandle;

    switch (enType)
    {
    case OT_RGN_OVERLAY:
        unMinHandle = OVERLAY_MIN_HANDLE;
        break;
    case OT_RGN_COVER:
        unMinHandle = COVER_MIN_HANDLE;
        break;
    case OT_RGN_COVEREX:
        unMinHandle = COVEREX_MIN_HANDLE;
        break;
    case OT_RGN_LINEEX:
        unMinHandle = LINEEX_MIN_HANDLE;
        break;
    case OT_RGN_CORNER_RECT:
        unMinHandle = CORNER_RECT_MIN_HANDLE;
        break;
    case OT_RGN_CORNER_RECTEX:
        unMinHandle = CORNER_RECTEX_MIN_HANDLE;
        break;
    default:
        unMinHandle = -1;
        break;
    }

    return unMinHandle;
}

/* 创建区域 */
static int mppRgn_create(HiRgn_S *pHandle)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("mppRgn_create error");
        return TD_FAILURE;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 区域属性 */
    ot_rgn_attr stRgnAttr;
    memset(&stRgnAttr, 0, sizeof(ot_rgn_attr));
    rgn_load_param(pHandle, &stRgnAttr);

    /* 创建区域 */
    CHECK_API_RETURN(ss_mpi_rgn_create(unHandle, &stRgnAttr));

    mpi_rgn_log("句柄: %d, 创建成功", unHandle);

    return TD_SUCCESS;
}

/* 销毁区域 */
static int mppRgn_destroy(HiRgn_S *pHandle)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("mppRgn_destroy error");
        return TD_FAILURE;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    CHECK_API_RETURN(ss_mpi_rgn_destroy(unHandle));

    /*销毁区域 */
    mpi_rgn_log("句柄: %d, 销毁成功", unHandle);

    return TD_SUCCESS;
}

/* 将区域叠加到通道上 */
static int mppRgn_attachToChn(HiRgn_S *pHandle)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("mppRgn_attachToChn error");
        return TD_FAILURE;
    }

    /* 已经绑定到通道时不再重复绑定，避免重复调用底层接口 */
    if (pHandle->bIsAttached)
    {
        return TD_SUCCESS;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 区域通道属性 */
    ot_rgn_chn_attr stRgnChnAttr;
    memset(&stRgnChnAttr, 0, sizeof(ot_rgn_chn_attr));
    rgn_load_chnParam(pHandle, &stRgnChnAttr);

    /* mpp通道属性 */
    ot_mpp_chn stMppChn;
    memset(&stMppChn, 0, sizeof(ot_mpp_chn));
    mpp_load_chnParam(pHandle, &stMppChn);

    if (pHandle->unOpFlag & REGION_OP_CHN)
    {
        CHECK_API_RETURN(ss_mpi_rgn_attach_to_chn(unHandle, &stMppChn, &stRgnChnAttr));
    }
    else if (pHandle->unOpFlag & REGION_OP_DEV)
    {
        CHECK_API_RETURN(ss_mpi_rgn_attach_to_dev(unHandle, &stMppChn, &stRgnChnAttr));
    }

    pHandle->bIsAttached = TD_TRUE;

    return TD_SUCCESS;
}

/* 将区域从通道上撤出 */
static int mppRgn_detachFromChn(HiRgn_S *pHandle)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("mppRgn_detachFromChn error");
        return TD_FAILURE;
    }

    /* 未绑定到通道时直接返回，避免无意义的底层解绑定 */
    if (!pHandle->bIsAttached)
    {
        return TD_SUCCESS;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* mpp通道属性 */
    ot_mpp_chn stMppChn;
    memset(&stMppChn, 0, sizeof(ot_mpp_chn));
    mpp_load_chnParam(pHandle, &stMppChn);

    if (pHandle->unOpFlag & REGION_OP_CHN)
    {
        CHECK_API_RETURN(ss_mpi_rgn_detach_from_chn(unHandle, &stMppChn));
    }
    else if (pHandle->unOpFlag & REGION_OP_DEV)
    {
        CHECK_API_RETURN(ss_mpi_rgn_detach_from_dev(unHandle, &stMppChn));
    }

    pHandle->bIsAttached = TD_FALSE;

    return TD_SUCCESS;
}

/* 加载图片数据到区域上 */
static int mppRgn_loadPicture(HiRgn_S *pHandle, void *pImage, int nSize)
{
    if (NULL == pHandle || NULL == pImage || 0 >= nSize)
    {
        mpi_rgn_log("传入参数错误");
        return TD_FAILURE;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 区域画布信息 */
    ot_rgn_canvas_info stCanvasInfo;
    memset(&stCanvasInfo, 0, sizeof(ot_rgn_canvas_info));

    /* 获取区域的显示画布信息 */
    CHECK_API_RETURN(ss_mpi_rgn_get_canvas_info(unHandle, &stCanvasInfo));

    /* 复制图片数据 */
    memcpy(stCanvasInfo.virt_addr, pImage, nSize);

    /*更新显示画布*/
    CHECK_API_RETURN(ss_mpi_rgn_update_canvas(unHandle));

    return TD_SUCCESS;
}

/* 清空区域上的贴图 */
static int mppRgn_clearPicture(HiRgn_S *pHandle)
{
    if (NULL == pHandle)
    {
        mpi_rgn_log("传入参数错误");
        return TD_FAILURE;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 区域画布信息 */
    ot_rgn_canvas_info stCanvasInfo;
    memset(&stCanvasInfo, 0, sizeof(ot_rgn_canvas_info));

    /* 获取区域的显示画布信息 */
    CHECK_API_RETURN(ss_mpi_rgn_get_canvas_info(unHandle, &stCanvasInfo));

    /* 计算画布字节大小 */
    uint32_t unCanvasSize = stCanvasInfo.size.height * stCanvasInfo.stride;

    /* 将画布内容清零 - 实现透明背景 */
    if (stCanvasInfo.virt_addr)
    {
        memset(stCanvasInfo.virt_addr, 0, unCanvasSize);
    }

    /* 更新显示画布 */
    CHECK_API_RETURN(ss_mpi_rgn_update_canvas(unHandle));

    return TD_SUCCESS;
}

/* 改变区域的位置 */
static int mppRgn_changePos(HiRgn_S *pHandle, int nStartX, int nStartY)
{

    if (NULL == pHandle)
    {
        mpi_rgn_log("mppRgn_detachFromChn error");
        return TD_FAILURE;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 区域通道属性 */
    ot_rgn_chn_attr stRgnChnAttr;
    memset(&stRgnChnAttr, 0, sizeof(ot_rgn_chn_attr));
    rgn_load_chnParam(pHandle, &stRgnChnAttr);

    /* mpp通道属性 */
    ot_mpp_chn stMppChn;
    memset(&stMppChn, 0, sizeof(ot_mpp_chn));
    mpp_load_chnParam(pHandle, &stMppChn);

    /* 获取区域通道显示属性 */
    CHECK_API_RETURN(ss_mpi_rgn_get_chn_display_attr(unHandle, &stMppChn, &stRgnChnAttr));

    /* 修改位置 */
    if (OT_RGN_OVERLAY == pHandle->unType)
    {
        stRgnChnAttr.attr.overlay_chn.point.x = MPI_ALIGN_UP(nStartX, OT_RGN_ALIGN);
        stRgnChnAttr.attr.overlay_chn.point.y = MPI_ALIGN_UP(nStartY, OT_RGN_ALIGN);
    }
    else if (OT_RGN_COVER == pHandle->unType)
    {
        if (OT_COVER_RECT == stRgnChnAttr.attr.cover_chn.cover.type)
        {
            stRgnChnAttr.attr.cover_chn.cover.rect_attr.rect.x = MPI_ALIGN_UP(nStartX, OT_RGN_ALIGN);
            stRgnChnAttr.attr.cover_chn.cover.rect_attr.rect.y = MPI_ALIGN_UP(nStartY, OT_RGN_ALIGN);
        }
        else if (OT_COVER_QUAD == stRgnChnAttr.attr.cover_chn.cover.type)
        {
            for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
            {
                stRgnChnAttr.attr.cover_chn.cover.quad_attr.point[i].x = pHandle->stuPoints[i].x;
                stRgnChnAttr.attr.cover_chn.cover.quad_attr.point[i].y = pHandle->stuPoints[i].y;
            }
        }
    }
    else if (OT_RGN_COVEREX == pHandle->unType)
    {
        if (OT_COVER_RECT == stRgnChnAttr.attr.coverex_chn.coverex.type)
        {
            stRgnChnAttr.attr.coverex_chn.coverex.rect_attr.rect.x = MPI_ALIGN_UP(nStartX, OT_RGN_ALIGN);
            stRgnChnAttr.attr.coverex_chn.coverex.rect_attr.rect.y = MPI_ALIGN_UP(nStartY, OT_RGN_ALIGN);
        }
        else if (OT_COVER_QUAD == stRgnChnAttr.attr.coverex_chn.coverex.type)
        {
            for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
            {
                stRgnChnAttr.attr.coverex_chn.coverex.quad_attr.point[i].x = pHandle->stuPoints[i].x;
                stRgnChnAttr.attr.coverex_chn.coverex.quad_attr.point[i].y = pHandle->stuPoints[i].y;
            }
        }
    }
    else if (OT_RGN_CORNER_RECT == pHandle->unType || OT_RGN_CORNER_RECTEX == pHandle->unType)
    {
        stRgnChnAttr.attr.corner_rect_chn.corner_rect.rect.x = MPI_ALIGN_UP(nStartX, OT_RGN_ALIGN);
        stRgnChnAttr.attr.corner_rect_chn.corner_rect.rect.y = MPI_ALIGN_UP(nStartY, OT_RGN_ALIGN);
    }

    /* 设置区域通道显示属性 */
    CHECK_API_RETURN(ss_mpi_rgn_set_chn_display_attr(unHandle, &stMppChn, &stRgnChnAttr));

    return TD_SUCCESS;
}

/* 改变区域的尺寸 */
static int mppRgn_changeRect(HiRgn_S *pHandle, int nWidth, int nHeight)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 修改尺寸 */
    if (OT_RGN_OVERLAY == pHandle->unType)
    {
        /* 区域属性 */
        ot_rgn_attr stRgnAttr;
        memset(&stRgnAttr, 0, sizeof(ot_rgn_attr));
        rgn_load_param(pHandle, &stRgnAttr);

        /*解绑定区域*/
        mppRgn_detachFromChn(pHandle);

        /* 获取区域属性 */
        CHECK_API_RETURN(ss_mpi_rgn_get_attr(unHandle, &stRgnAttr));
    
        stRgnAttr.attr.overlay.size.width = MPI_ALIGN_UP(nWidth, OT_RGN_ALIGN);
        stRgnAttr.attr.overlay.size.height = MPI_ALIGN_UP(nHeight, OT_RGN_ALIGN);

        /* 设置区域属性 */
        CHECK_API_RETURN(ss_mpi_rgn_set_attr(unHandle, &stRgnAttr));

        /*绑定区域*/
        mppRgn_attachToChn(pHandle);
    }
    else if (OT_RGN_COVER == pHandle->unType || OT_RGN_COVEREX == pHandle->unType
             || OT_RGN_CORNER_RECT == pHandle->unType || OT_RGN_CORNER_RECTEX == pHandle->unType)
    {
        /* 区域通道属性 */
        ot_rgn_chn_attr stRgnChnAttr;
        memset(&stRgnChnAttr, 0, sizeof(ot_rgn_chn_attr));
        rgn_load_chnParam(pHandle, &stRgnChnAttr);

        /* mpp通道属性 */
        ot_mpp_chn stMppChn;
        memset(&stMppChn, 0, sizeof(ot_mpp_chn));
        mpp_load_chnParam(pHandle, &stMppChn);

        /* 获取区域通道显示属性 */
        CHECK_API_RETURN(ss_mpi_rgn_get_chn_display_attr(unHandle, &stMppChn, &stRgnChnAttr));

        if (OT_RGN_COVER == pHandle->unType && pHandle->bIsRectangle)
        {
            stRgnChnAttr.attr.cover_chn.cover.rect_attr.rect.width = MPI_ALIGN_UP(pHandle->unWidth, OT_RGN_ALIGN);
            stRgnChnAttr.attr.cover_chn.cover.rect_attr.rect.height = MPI_ALIGN_UP(pHandle->unHeight, OT_RGN_ALIGN);
        }
        else if (OT_RGN_COVEREX == pHandle->unType && pHandle->bIsRectangle)
        {
            stRgnChnAttr.attr.coverex_chn.coverex.rect_attr.rect.width = MPI_ALIGN_UP(pHandle->unWidth, OT_RGN_ALIGN);
            stRgnChnAttr.attr.coverex_chn.coverex.rect_attr.rect.height = MPI_ALIGN_UP(pHandle->unHeight, OT_RGN_ALIGN);
        }
        else if (OT_RGN_CORNER_RECT == pHandle->unType || OT_RGN_CORNER_RECTEX == pHandle->unType)
        {
            stRgnChnAttr.attr.corner_rect_chn.corner_rect.rect.width = MPI_ALIGN_UP(pHandle->unWidth, OT_RGN_ALIGN);
            stRgnChnAttr.attr.corner_rect_chn.corner_rect.rect.height = MPI_ALIGN_UP(pHandle->unHeight, OT_RGN_ALIGN);
        }

        /* 设置区域通道显示属性 */
        CHECK_API_RETURN(ss_mpi_rgn_set_chn_display_attr(unHandle, &stMppChn, &stRgnChnAttr));
    }

    return TD_SUCCESS;
}

/* 显示或者隐藏区域 */
static int mppRgn_showOrHide(HiRgn_S *pHandle, td_bool bIsShow)
{
    if (NULL == pHandle || !pHandle->bIsFlicker)
    {
        return TD_FAILURE;
    }
    /* 区域句柄号 */
    uint32_t unHandle = mppRgn_get_minHandle(pHandle->unType) + pHandle->unHandle;

    /* 区域通道属性 */
    ot_rgn_chn_attr stRgnChnAttr;
    memset(&stRgnChnAttr, 0, sizeof(ot_rgn_chn_attr));
    rgn_load_chnParam(pHandle, &stRgnChnAttr);

    /* mpp通道属性 */
    ot_mpp_chn stMppChn;
    memset(&stMppChn, 0, sizeof(ot_mpp_chn));
    mpp_load_chnParam(pHandle, &stMppChn);

    /* 获取区域通道显示属性 */
    CHECK_API_RETURN(ss_mpi_rgn_get_chn_display_attr(unHandle, &stMppChn, &stRgnChnAttr));

    /* 显示或者隐藏区域 */
    stRgnChnAttr.is_show = bIsShow;

    /* 设置区域通道显示属性 */
    CHECK_API_RETURN(ss_mpi_rgn_set_chn_display_attr(unHandle, &stMppChn, &stRgnChnAttr));

    return TD_SUCCESS;
}

/* 更新区域 */
static int mppRgn_update(HiRgn_S *pHandle, HiRgnNeedParam_S stParam)
{
    pHandle->bIsShow = stParam.bIsShow;
    pHandle->unHandle = stParam.unHandle;
    pHandle->unOpFlag = stParam.unOpFlag;
    pHandle->unModId = stParam.unModId;
    pHandle->unDevId = stParam.unDevId;
    pHandle->unChnId = stParam.unChnId;
    pHandle->unType = stParam.unType;
    pHandle->unLayer = stParam.unLayer;
    pHandle->unBgColor = stParam.unBgColor;

    if (OT_RGN_OVERLAY == pHandle->unType)
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
        
        pHandle->enFormat = OT_PIXEL_FORMAT_ARGB_4444;
    }
    else if (OT_RGN_COVER == pHandle->unType || OT_RGN_COVEREX  == pHandle->unType)
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
            for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
            {
                pHandle->stuPoints[i].x = stParam.stuPoints[i].x;
                pHandle->stuPoints[i].y = stParam.stuPoints[i].y;
            }
        }

        pHandle->bIsSolid = stParam.bIsSolid;

        pHandle->enFormat = OT_PIXEL_FORMAT_RGB_888;
    }
    else if (OT_RGN_CORNER_RECT == pHandle->unType || OT_RGN_CORNER_RECTEX == pHandle->unType)
    {
        pHandle->unStartX = stParam.unStartX;
        pHandle->unStartY = stParam.unStartY;
        pHandle->unWidth = stParam.unWidth;
        pHandle->unHeight = stParam.unHeight;
        pHandle->unFgColor = stParam.unFgColor;
        pHandle->uHorLen = stParam.uHorLen;
        pHandle->uVerLen = stParam.uVerLen;
        pHandle->uThick = stParam.uThick;
        pHandle->bIsFlicker = stParam.bIsFlicker;
    }

    return TD_SUCCESS;
}

/* 分配区域句柄 */
HiRgn_S *mppRgn_alloc(HiRgnNeedParam_S stParam)
{
    HiRgn_S *pHandle = (HiRgn_S *)malloc(sizeof(HiRgn_S));
    memset(pHandle, 0, sizeof(HiRgn_S));

    mppRgn_update(pHandle, stParam);

    pHandle->mppRgn_create = mppRgn_create;
    pHandle->mppRgn_destroy = mppRgn_destroy;
    pHandle->mppRgn_attachToChn = mppRgn_attachToChn;
    pHandle->mppRgn_detachFromChn = mppRgn_detachFromChn;
    pHandle->mppRgn_loadPicture = mppRgn_loadPicture;
    pHandle->mppRgn_clearPicture = mppRgn_clearPicture;
    pHandle->mppRgn_changePos = mppRgn_changePos;
    pHandle->mppRgn_changeRect = mppRgn_changeRect;
    pHandle->mppRgn_showOrHide = mppRgn_showOrHide;
    pHandle->mppRgn_update = mppRgn_update;

    return pHandle;
}

/* 释放区域句柄 */
void mppRgn_release(HiRgn_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}
