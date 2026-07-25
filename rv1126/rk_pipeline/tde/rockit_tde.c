/*************************************************************************
	> File Name: rockit_tde.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年05月27日 星期五 14时25分21秒
 ************************************************************************/
#include "rockit_tde.h"
#include "rk_mpi_mmz.h"
/* TDE开启状态*/
RK_BOOL bTde_OpenStatus = RK_FALSE;


/*开启TDE*/
int rkTde_open()
{
    int nRet = 0;

    if ( bTde_OpenStatus )
    {
        return 0;
    }

    nRet = RK_TDE_Open();
    
    if ( nRet != RK_SUCCESS )
    {
        printf( "Open TDE devce fail\n" );
        return -1;
    }
    //bTde_OpenStatus = RK_TRUE;
 
    return 0;
}

/*关闭TDE*/
int rkTde_close()
{
    int nRet = 0;

    if( !bTde_OpenStatus )
    {
        return 0;
    }
    
    RK_TDE_Close();

    bTde_OpenStatus = RK_FALSE;
    return 0;
}

/*分配存储空间*/
int rkTde_alloc_pic( TdePicInfo_S* pstPicInfo )
{
    int nRet = 0;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    stPicBufAttr.u32Width       = pstPicInfo->unWidth;
    stPicBufAttr.u32Height      = pstPicInfo->unHeight;
    stPicBufAttr.enPixelFormat  = pstPicInfo->enPixFormat;
    stPicBufAttr.enCompMode     = pstPicInfo->enCompressMode;
    nRet = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if ( nRet != RK_SUCCESS )
    {
        printf( "TDE get pic size  fail Ox%x\n", nRet );
        return nRet;
    }
    pstPicInfo->nBuffsize = stMbPicCalResult.u32MBSize;
    
    //printf("分配%d\n", pstPicInfo->nBuffsize);
    nRet = RK_MPI_SYS_MmzAllocEx( &pstPicInfo->Blk, RK_NULL, RK_NULL, pstPicInfo->nBuffsize
            , MB_REMAP_MODE_CACHED | MB_ALLOC_TYPE_DMA | MB_DMA_TYPE_NONE );
    if ( nRet != RK_SUCCESS )
    {
        printf( "TDE malloc pic bufer fail Ox%x\n", nRet );
        return nRet;
    }
    pstPicInfo->pData = RK_MPI_MB_Handle2VirAddr( pstPicInfo->Blk );
    return nRet;
}
/*释放空间*/
int rkTde_relese_pic( TdePicInfo_S* pstPicInfo )
{
    if( pstPicInfo->Blk )
    {
        return RK_MPI_MMZ_Free( pstPicInfo->Blk );
    }

    return 0;
}

/*用户空间的数据填充到MBK
 *
 *如果pCallBack为空, 拷贝数据的方式就是memcpy
 *
 * 如果不为空，就是调用pCallBack的方式进行拷贝 pData就是用户传入回调的pParam
 *比如你可以读取文件，这样就能减少一次拷贝
 *
 * */
int rkTde_load_data( TdePicInfo_S* pstPicInfo, void* pData, int nSize, int (*pCallBack)( char* pData, void* pParam), void* pParam )
{
    int nRet = 0;

    /*判断图片大小*/
    if(  pstPicInfo->nBuffsize < nSize )
    {
        printf(" TDE PIC size error PIC size= %d, data size =%d\n", pstPicInfo->nBuffsize, nSize);
        return -1;
    }
    
    /*拷贝数据*/
    if( !pCallBack )
    {
        memcpy( pstPicInfo->pData, pData, nSize);
    }
    else
    {
        pCallBack( (void*)pstPicInfo->pData, pData );
    }
    return 0;
}

static int rkTde_load_param( TDE_SURFACE_S *pSur, TDE_RECT_S* pRet, TdePicInfo_S* pPic)
{
    pSur->pMbBlk            = pPic->Blk;
    pSur->enColorFmt        = pPic->enPixFormat;
    pSur->u32Height         = pPic->unHeight;
    pSur->u32Width          = pPic->unWidth;
    pSur->enComprocessMode  = pPic->enCompressMode;
    
    pRet->s32Xpos =pPic->nX;
    pRet->s32Ypos =pPic->nY;
    if( pPic->nW * pPic->nH < 64 * 64 )
    {
        return -1;
    }
    pRet->u32Width = pPic->nW;
    pRet->u32Height = pPic->nH;
    
    return 0;
}
TDE_HANDLE rkTde_createJob()
{
    TDE_HANDLE handle = RK_TDE_BeginJob( );
    if( handle == RK_ERR_TDE_INVALID_HANDLE)
    {
        printf(" tde create job nRet fail 0x%x\n", handle);
    }
    return handle;
}
int rkTde_endJob( TDE_HANDLE tdeHandle )
{
    int nRet = RK_TDE_EndJob( tdeHandle, RK_FALSE, RK_TRUE, 30 );
    if(nRet != RK_SUCCESS)
    {
        RK_TDE_CancelJob( tdeHandle );
        printf(" tde endjob nRet fail 0x%x\n",nRet);
    }
    return nRet;
}

int rkTde_copy_task( TDE_HANDLE tdeHandle, TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic  )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stSrcSur, &stSrcRect, pSrcPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = RK_TDE_QuickCopy( tdeHandle, &stSrcSur, &stSrcRect, &stDstSur, &stDstRect);
    if(nRet != RK_SUCCESS)
    {
        //RK_TDE_CancelJob( tdeHandle );
        printf(" tde copy task nRet fail 0x%x\n",nRet);
    }
    return nRet;

}
/*拷贝*/
int rkTde_copy( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stSrcSur, &stSrcRect, pSrcPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    
    TDE_HANDLE tdeHandle = RK_TDE_BeginJob( );
 
    if ( RK_ERR_TDE_INVALID_HANDLE == tdeHandle )
    {
        printf("tde begin copy fail\n");
        return -1;
    }
    
    nRet = RK_TDE_QuickCopy( tdeHandle, &stSrcSur, &stSrcRect, &stDstSur, &stDstRect);
    if(nRet != RK_SUCCESS)
    {
        printf(" tde copy nRet fail 0x%x\n",nRet);
    }
    nRet = RK_TDE_EndJob( tdeHandle, RK_FALSE, RK_TRUE, 30 );
    if(nRet != RK_SUCCESS)
    {
        RK_TDE_CancelJob( tdeHandle );
        printf(" tde copy nRet fail 0x%x\n",nRet);
    }
        //RK_TDE_WaitForDone( tdeHandle );
    
    return 0;
}

/*缩放*/
int rkTde_resize_task( TDE_HANDLE tdeHandle, TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic  )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stSrcSur, &stSrcRect, pSrcPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = RK_TDE_QuickResize( tdeHandle, &stSrcSur, &stSrcRect, &stDstSur, &stDstRect);
    if(nRet != RK_SUCCESS)
    {
        //RK_TDE_CancelJob( tdeHandle );
        printf(" tde resize task nRet fail 0x%x\n",nRet);
    }
    return nRet;

}
int rkTde_resize( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stSrcSur, &stSrcRect, pSrcPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    
    TDE_HANDLE tdeHandle = RK_TDE_BeginJob( );
 
    if ( RK_ERR_TDE_INVALID_HANDLE == tdeHandle )
    {
        printf("tde begin resize fail\n");
        return -1;
    }
    
    nRet = RK_TDE_QuickResize( tdeHandle, &stSrcSur, &stSrcRect, &stDstSur, &stDstRect);
    if(nRet != RK_SUCCESS)
    {
        printf(" tde resize nRet fail 0x%x\n",nRet);
    }
    nRet = RK_TDE_EndJob( tdeHandle, RK_FALSE, RK_TRUE, 10 );
    if(nRet != RK_SUCCESS)
    {
        RK_TDE_CancelJob( tdeHandle );
        printf(" tde resize endjob nRet fail 0x%x\n",nRet);
    }
    else
    {
        RK_TDE_WaitForDone( tdeHandle );
        
    }
    
    return nRet;
}

/*填充*/
int rkTde_full_task( TDE_HANDLE tdeHandle, TdePicInfo_S* pDstPic, unsigned int unFillData  )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    RK_TDE_QuickFill( tdeHandle, &stDstSur, &stDstRect, unFillData);;
    if(nRet != RK_SUCCESS)
    {
        //RK_TDE_CancelJob( tdeHandle );
        printf(" tde resize task nRet fail 0x%x\n",nRet);
    }
    return nRet;

}
int rkTde_full( TdePicInfo_S* pDstPic , unsigned int unFillData)
{
    int nRet = 0;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    
    TDE_HANDLE tdeHandle = RK_TDE_BeginJob( );
 
    if ( RK_ERR_TDE_INVALID_HANDLE == tdeHandle )
    {
        printf("tde begin copy fail\n");
        return -1;
    }

    RK_TDE_QuickFill( tdeHandle, &stDstSur, &stDstRect, unFillData);;
    RK_TDE_EndJob( tdeHandle, RK_FALSE, RK_TRUE, 10 );
    RK_TDE_WaitForDone( tdeHandle );
    
    return 0;
}


/*旋转*/
int rkTde_rotate( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic, ROTATION_E enRotateAngle )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stSrcSur, &stSrcRect, pSrcPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    
    TDE_HANDLE tdeHandle = RK_TDE_BeginJob( );
 
    if ( RK_ERR_TDE_INVALID_HANDLE == tdeHandle )
    {
        printf("tde begin copy fail\n");
        return -1;
    }
    RK_TDE_Rotate( tdeHandle, &stSrcSur, &stSrcRect, &stDstSur, &stDstRect, enRotateAngle);
    RK_TDE_EndJob( tdeHandle, RK_FALSE, RK_TRUE, 10 );
    RK_TDE_WaitForDone( tdeHandle );
    
    return 0;
}
/*镜像, 抠图，融合*/
int rkTde_bitblit( TdePicInfo_S* pSrcPic, TdePicInfo_S* pDstPic, TDE_OPT_S* pstOpt )
{
    int nRet = 0;
    TDE_SURFACE_S stSrcSur;
    TDE_SURFACE_S stDstSur;
    TDE_RECT_S stSrcRect;
    TDE_RECT_S stDstRect;
    
    nRet = rkTde_load_param( &stSrcSur, &stSrcRect, pSrcPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    nRet = rkTde_load_param( &stDstSur, &stDstRect, pDstPic );
    if( nRet != 0 )
    {
        return nRet;
    }
    
    TDE_HANDLE tdeHandle = RK_TDE_BeginJob( );
 
    if ( RK_ERR_TDE_INVALID_HANDLE == tdeHandle )
    {
        printf("tde begin copy fail\n");
        return -1;
    }
    
    RK_TDE_Bitblit( tdeHandle, &stDstSur, &stDstRect, &stSrcSur, &stSrcRect,
                    &stDstSur, &stDstRect, pstOpt);
    RK_TDE_EndJob( tdeHandle, RK_FALSE, RK_TRUE, 10 );
    RK_TDE_WaitForDone( tdeHandle );
    
    return 0;
}

