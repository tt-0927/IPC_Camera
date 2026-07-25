/*************************************************************************
	> File Name: rockit_tde.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月27日 星期五 14时25分21秒
 ************************************************************************/
#include "rockit_vgs.h"

/*分配存储空间*/
int rockitVgs_alloc_blk( VgsFrame_S* pstPicInfo )
{
    int nRet = 0;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    stPicBufAttr.u32Width       = pstPicInfo->unWidth;
    stPicBufAttr.u32Height      = pstPicInfo->unHeight;
    stPicBufAttr.enPixelFormat  = pstPicInfo->ePixFormat;
    stPicBufAttr.enCompMode     = pstPicInfo->enCompressMode;
    
    nRet = RK_MPI_CAL_VGS_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    //nRet = RK_MPI_CAL_COMM_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if ( nRet != RK_SUCCESS )
    {
        printf( "VGS get pic size  fail Ox%x\n", nRet );
        return nRet;
    }
    pstPicInfo->nBuffsize = stMbPicCalResult.u32MBSize;
    
    nRet = RK_MPI_SYS_MmzAllocEx( &pstPicInfo->Blk, RK_NULL, RK_NULL, pstPicInfo->nBuffsize
            , MB_REMAP_MODE_NONE | MB_ALLOC_TYPE_DMA | MB_DMA_TYPE_NONE );
    if ( nRet != RK_SUCCESS )
    {
        printf( "VGS malloc pic bufer fail Ox%x %d %d\n", nRet, stPicBufAttr.u32Width, stPicBufAttr.u32Height );
        return nRet;
    }
    pstPicInfo->pData = RK_MPI_MB_Handle2VirAddr( pstPicInfo->Blk );
    return nRet;
}
/*释放空间*/
int rockitVgs_relese_blk( VgsFrame_S* pstPicInfo )
{
    if( pstPicInfo->Blk )
    {
        return RK_MPI_SYS_Free( pstPicInfo->Blk );
    }
    return 0;
}

static int rockitVgs_load_param( VGS_TASK_ATTR_S *pTaskAttr, VgsFrame_S* pSrcFrame, VgsFrame_S* pDstFrame)
{
    VIDEO_FRAME_S stSrcFrame;
    VIDEO_FRAME_S stDstFrame;
    stDstFrame.u32Width       = pDstFrame->unWidth;
    stDstFrame.u32Height      = pDstFrame->unHeight;
    stDstFrame.u32VirWidth    = pDstFrame->unVirWidth;
    stDstFrame.u32VirHeight   = pDstFrame->unVirHeight;
    stDstFrame.enPixelFormat  = pDstFrame->ePixFormat;
    stDstFrame.enCompressMode  = pDstFrame->enCompressMode;
    stDstFrame.u32TimeRef     = 0;
	stDstFrame.u64PTS         = 0; 
    stDstFrame.pMbBlk         = pDstFrame->Blk;
    
    stSrcFrame.u32Width       = pSrcFrame->unWidth;
    stSrcFrame.u32Height      = pSrcFrame->unHeight;
    stSrcFrame.u32VirWidth    = pSrcFrame->unVirWidth;
    stSrcFrame.u32VirHeight   = pSrcFrame->unVirHeight;
    stSrcFrame.enPixelFormat  = pSrcFrame->ePixFormat;
    stSrcFrame.enCompressMode = pSrcFrame->enCompressMode;
    stSrcFrame.u32TimeRef     = 0;
	stSrcFrame.u64PTS         = 0; 
    stSrcFrame.pMbBlk         = pSrcFrame->Blk;
    
    pTaskAttr->stImgIn.stVFrame  = stSrcFrame;
    pTaskAttr->stImgOut.stVFrame = stDstFrame;
    return 0;
}

/*缩放*/
int rockitVgs_scale( VgsFrame_S* pSrcPic, VgsFrame_S* pDstPic)
{
    int nRet = 0;
    VGS_HANDLE vgsHandle;
    VGS_TASK_ATTR_S stTask;

    memset( &stTask, 0, sizeof(VGS_TASK_ATTR_S) );

    nRet = RK_MPI_VGS_BeginJob(&vgsHandle);
    if ( nRet != RK_SUCCESS )
    {
        printf("vgs begin scale fail\n");
        return -1;
    }
    
    rockitVgs_load_param( &stTask, pSrcPic, pDstPic );

    nRet = RK_MPI_VGS_AddScaleTask( vgsHandle, &stTask, VGS_SCLCOEF_NORMAL);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf("  vgs scale add  fail 0x%x\n",nRet);
        return -1;
    }
    nRet = RK_MPI_VGS_EndJob(vgsHandle);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf(" vgs scale end fail 0x%x\n",nRet);
        return -1;
    }
    return 0;
}

/*打OSD*/
int rockitVgs_osd( VgsFrame_S* pSrcPic, RECT_S* pstSrcRect, VgsFrame_S* pDstPic)
{
    int nRet = 0;
    VGS_HANDLE vgsHandle;
    VGS_TASK_ATTR_S stTask;
    VGS_ADD_OSD_S stOsd;

    memset( &stTask, 0, sizeof(VGS_TASK_ATTR_S) );
    memset( &stOsd, 0, sizeof(VGS_ADD_OSD_S) );

    nRet = RK_MPI_VGS_BeginJob(&vgsHandle);
    if ( nRet != RK_SUCCESS )
    {
        printf("vgs begin osd fail\n");
        return -1;
    }
    rockitVgs_load_param( &stTask, pDstPic, pDstPic );

    stOsd.stRect.s32X = pstSrcRect->s32X;
    stOsd.stRect.s32Y = pstSrcRect->s32Y;
    stOsd.stRect.u32Height = pstSrcRect->u32Height;
    stOsd.stRect.u32Width = pstSrcRect->u32Width;
    stOsd.pMbBlk = pSrcPic->Blk;
    stOsd.enPixelFmt = pSrcPic->ePixFormat;
    nRet = RK_MPI_VGS_AddOsdTask( vgsHandle, &stTask, &stOsd);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf("  vgs osd add  fail 0x%x\n",nRet);
        return -1;
    }
    nRet = RK_MPI_VGS_EndJob(vgsHandle);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf(" vgs osd end fail 0x%x\n",nRet);
        return -1;
    }
    return 0;
}

/* 批量添加双OSD叠加（使用RK_MPI_VGS_AddOsdTaskArray函数） */
int rockitVgs_osd_double(VgsFrame_S* pSrcPic1, RECT_S* pstRect1,
                           VgsFrame_S* pSrcPic2, RECT_S* pstRect2,
                           VgsFrame_S* pDstPic)
{
    int nRet = 0;
    VGS_HANDLE vgsHandle;
    VGS_TASK_ATTR_S stTask;
    VGS_ADD_OSD_S astOsd[2];

    /* 清零任务及OSD参数数组 */
    memset(&stTask, 0, sizeof(VGS_TASK_ATTR_S));
    memset(astOsd, 0, sizeof(astOsd));

    /* 开始VGS任务 */
    nRet = RK_MPI_VGS_BeginJob(&vgsHandle);
    if(nRet != RK_SUCCESS)
    {
        printf("vgs begin job fail, error: 0x%x\n", nRet);
        return -1;
    }

    /* 加载任务参数（目标图像为pDstPic） */
    rockitVgs_load_param(&stTask, pDstPic, pDstPic);

    /* 配置第一个OSD叠加参数 */
    astOsd[0].stRect.s32X     = pstRect1->s32X;
    astOsd[0].stRect.s32Y     = pstRect1->s32Y;
    astOsd[0].stRect.u32Width  = pstRect1->u32Width;
    astOsd[0].stRect.u32Height = pstRect1->u32Height;
    astOsd[0].pMbBlk          = pSrcPic1->Blk;
    astOsd[0].enPixelFmt      = pSrcPic1->ePixFormat;

    /* 配置第二个OSD叠加参数 */
    astOsd[1].stRect.s32X     = pstRect2->s32X;
    astOsd[1].stRect.s32Y     = pstRect2->s32Y;
    astOsd[1].stRect.u32Width  = pstRect2->u32Width;
    astOsd[1].stRect.u32Height = pstRect2->u32Height;
    astOsd[1].pMbBlk          = pSrcPic2->Blk;
    astOsd[1].enPixelFmt      = pSrcPic2->ePixFormat;

    /* 批量添加2个OSD叠加任务 */
    nRet = RK_MPI_VGS_AddOsdTaskArray(vgsHandle, &stTask, astOsd, 2);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf("vgs osd add task array fail, error: 0x%x\n", nRet);
        return -1;
    }

    /* 提交VGS任务 */
    nRet = RK_MPI_VGS_EndJob(vgsHandle);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf("vgs osd end job fail, error: 0x%x\n", nRet);
        return -1;
    }

    return 0;
}

/*裁剪*/
int rockitVgs_crop( VgsFrame_S* pSrcPic, RECT_S stRect, VgsFrame_S* pDstPic)
{
    int nRet = 0;
    VGS_HANDLE vgsHandle;
    VGS_TASK_ATTR_S stTask;
    VGS_CROP_INFO_S stCrop;

    memset(&stTask, 0, sizeof(VGS_TASK_ATTR_S));
    memset(&stCrop, 0, sizeof(VGS_CROP_INFO_S));

    nRet = RK_MPI_VGS_BeginJob(&vgsHandle);
    if (nRet != RK_SUCCESS)
    {
        printf("crop begin fail\n");
        return -1;
    }

    rockitVgs_load_param(&stTask, pSrcPic, pDstPic);

    stCrop.enCropCoordinate = VGS_CROP_ABS_COOR;

    stCrop.stCropRect.s32X = stRect.s32X;
    stCrop.stCropRect.s32Y = stRect.s32Y;
    stCrop.stCropRect.u32Height = stRect.u32Height;
    stCrop.stCropRect.u32Width = stRect.u32Width;

    nRet = RK_MPI_VGS_AddCropTask(vgsHandle, &stTask, &stCrop);
    if (nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf("  crop add  fail 0x%x\n", nRet);
        return -1;
    }
    nRet = RK_MPI_VGS_EndJob(vgsHandle);
    if (nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf(" crop end fail 0x%x\n", nRet);
        return -1;
    }
    return 0;
}

/*马赛克*/
int rockitVgs_mosaic( VgsFrame_S* pSrcPic, RECT_S stRect, VgsFrame_S* pDstPic)
{
    int nRet = 0;
    VGS_HANDLE vgsHandle;
    VGS_TASK_ATTR_S stTask;
    VGS_MOSAIC_S stMosaic;

    memset( &stTask, 0, sizeof(VGS_TASK_ATTR_S) );
    memset( &stMosaic, 0, sizeof(VGS_MOSAIC_S) );

    nRet = RK_MPI_VGS_BeginJob(&vgsHandle);
    if ( nRet != RK_SUCCESS )
    {
        printf("crop begin fail\n");
        return -1;
    }
    
    rockitVgs_load_param( &stTask, pSrcPic, pDstPic );

    stMosaic.enBlkSize = RK_MOSAIC_BLK_SIZE_8;

    stMosaic.stDstRect.s32X = stRect.s32X;
    stMosaic.stDstRect.s32Y = stRect.s32Y;
    stMosaic.stDstRect.u32Height = stRect.u32Height;
    stMosaic.stDstRect.u32Width = stRect.u32Width;

    nRet = RK_MPI_VGS_AddMosaicTask(vgsHandle, &stTask, &stMosaic);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf("  crop add  fail 0x%x\n",nRet);
        return -1;
    }
    nRet = RK_MPI_VGS_EndJob(vgsHandle);
    if(nRet != RK_SUCCESS)
    {
        RK_MPI_VGS_CancelJob(vgsHandle);
        printf(" crop end fail 0x%x\n",nRet);
        return -1;
    }
    return 0;
}
