/*** 
 * @FilePath     : rockit_com.c
 * @Author       : luoyk 
 * @Date         : 2022-12-08 08:48:16
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-14 17:11:43
 * @Description  : 
 */

#include"rockit_com.h"
#include "rk_mpi_vo.h"

/*分配帧缓冲*/
VIDEO_FRAME_INFO_S * alloc_nocache_tdeFrame( int nWidth, int nHeight, PIXEL_FORMAT_E enPixFormat, COMPRESS_MODE_E enCompressMode  )
{
    MB_BLK Blk = NULL;
    unsigned char* pData = NULL;
    int nRet = 0;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    VIDEO_FRAME_INFO_S* pstVideFrame = NULL;

    stPicBufAttr.u32Width       = nWidth;
    stPicBufAttr.u32Height      = nHeight;
    stPicBufAttr.enPixelFormat  = enPixFormat;
    stPicBufAttr.enCompMode     = enCompressMode;
    nRet = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if ( nRet != RK_SUCCESS )
    {
        printf( "TDE get pic size  fail Ox%x\n", nRet );
        return NULL;
    }
    
    nRet = RK_MPI_SYS_MmzAllocEx( &Blk, RK_NULL, RK_NULL, stMbPicCalResult.u32MBSize
            , MB_REMAP_MODE_NOCACHE | MB_ALLOC_TYPE_DMA | MB_DMA_TYPE_NONE );
    if ( nRet != RK_SUCCESS )
    {
        printf( "alloc tdeFrame fail Ox%x\n", nRet );
        return NULL;
    }
    pData = RK_MPI_MB_Handle2VirAddr( Blk );
    pstVideFrame = (VIDEO_FRAME_INFO_S*)malloc(sizeof(VIDEO_FRAME_INFO_S));
    memset(pstVideFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
    pstVideFrame->stVFrame.u32Width         = nWidth;
    pstVideFrame->stVFrame.u32Height        = nHeight;
    pstVideFrame->stVFrame.u32VirWidth      = nWidth;
    pstVideFrame->stVFrame.u32VirHeight     = nHeight;
    pstVideFrame->stVFrame.pMbBlk           = Blk;
    pstVideFrame->stVFrame.enPixelFormat    = enPixFormat;
    pstVideFrame->stVFrame.enCompressMode   = enCompressMode;
    pstVideFrame->stVFrame.pVirAddr[0]      = pData;
    return pstVideFrame;
}





/*分配帧缓冲*/
VIDEO_FRAME_INFO_S * alloc_tdeFrame( int nWidth, int nHeight, PIXEL_FORMAT_E enPixFormat, COMPRESS_MODE_E enCompressMode  )
{
    MB_BLK Blk = NULL;
    unsigned char* pData = NULL;
    int nRet = 0;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    VIDEO_FRAME_INFO_S* pstVideFrame = NULL;

    stPicBufAttr.u32Width       = nWidth;
    stPicBufAttr.u32Height      = nHeight;
    stPicBufAttr.enPixelFormat  = enPixFormat;
    stPicBufAttr.enCompMode     = enCompressMode;
    nRet = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if ( nRet != RK_SUCCESS )
    {
        printf( "TDE get pic size  fail Ox%x\n", nRet );
        return NULL;
    }
    
    nRet = RK_MPI_SYS_MmzAllocEx( &Blk, RK_NULL, RK_NULL, stMbPicCalResult.u32MBSize
            , MB_REMAP_MODE_CACHED | MB_ALLOC_TYPE_DMA | MB_DMA_TYPE_NONE );
    if ( nRet != RK_SUCCESS )
    {
        printf( "alloc tdeFrame fail Ox%x\n", nRet );
        return NULL;
    }
    pData = RK_MPI_MB_Handle2VirAddr( Blk );
    pstVideFrame = (VIDEO_FRAME_INFO_S*)malloc(sizeof(VIDEO_FRAME_INFO_S));
    memset(pstVideFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
    pstVideFrame->stVFrame.u32Width         = nWidth;
    pstVideFrame->stVFrame.u32Height        = nHeight;
    pstVideFrame->stVFrame.u32VirWidth      = nWidth;
    pstVideFrame->stVFrame.u32VirHeight     = nHeight;
    pstVideFrame->stVFrame.pMbBlk           = Blk;
    pstVideFrame->stVFrame.enPixelFormat    = enPixFormat;
    pstVideFrame->stVFrame.enCompressMode   = enCompressMode;
    pstVideFrame->stVFrame.pVirAddr[0]      = pData;
    return pstVideFrame;
}



/* VGS-分配帧缓冲 */
VIDEO_FRAME_INFO_S* alloc_vgsFrame(int nWidth, int nHeight, PIXEL_FORMAT_E enPixFormat, COMPRESS_MODE_E enCompressMode)
{
    MB_BLK Blk = NULL;
    unsigned char* pData = NULL;
    int nRet = 0;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    VIDEO_FRAME_INFO_S* pstVideFrame = NULL;

    stPicBufAttr.u32Width       = nWidth;
    stPicBufAttr.u32Height      = nHeight;
    stPicBufAttr.enPixelFormat  = enPixFormat;
    stPicBufAttr.enCompMode     = enCompressMode;
    
    nRet = RK_MPI_CAL_VGS_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if ( nRet != RK_SUCCESS )
    {
        printf( "VGS get pic size  fail Ox%x\n", nRet );
        return NULL;
    }
    
    nRet = RK_MPI_SYS_MmzAllocEx( &Blk, RK_NULL, RK_NULL, stMbPicCalResult.u32MBSize
            , MB_REMAP_MODE_NONE | MB_ALLOC_TYPE_DMA | MB_DMA_TYPE_NONE );
    if ( nRet != RK_SUCCESS )
    {
        printf( "VGS malloc pic bufer fail Ox%x %d %d\n", nRet, stPicBufAttr.u32Width, stPicBufAttr.u32Height );
        return NULL;
    }
    pData = RK_MPI_MB_Handle2VirAddr( Blk );
    pstVideFrame = (VIDEO_FRAME_INFO_S*)malloc(sizeof(VIDEO_FRAME_INFO_S));
    memset(pstVideFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
    pstVideFrame->stVFrame.u32Width         = nWidth;
    pstVideFrame->stVFrame.u32Height        = nHeight;
    pstVideFrame->stVFrame.u32VirWidth      = nWidth;
    pstVideFrame->stVFrame.u32VirHeight     = nHeight;
    pstVideFrame->stVFrame.pMbBlk           = Blk;
    pstVideFrame->stVFrame.enPixelFormat    = enPixFormat;
    pstVideFrame->stVFrame.enCompressMode   = enCompressMode;
    pstVideFrame->stVFrame.pVirAddr[0]      = pData;
    return pstVideFrame;
}


/*释放帧缓冲*/
void free_tdeFrame( VIDEO_FRAME_INFO_S* pFrame )
{
    if (pFrame != NULL)
    {
        if(pFrame->stVFrame.pMbBlk)
        {
            RK_MPI_MMZ_Free( pFrame->stVFrame.pMbBlk );
        }
        free(pFrame);
        pFrame = NULL;
    }
    return;
}

/* vgs-释放缓存帧 */
void free_vgsFrame( VIDEO_FRAME_INFO_S* pFrame )
{
    if( pFrame )
    {
        RK_MPI_SYS_Free( pFrame->stVFrame.pMbBlk );
        free( pFrame );
        pFrame = NULL;
    }
    return;
}

/*缩放*/
int resize_frame( VIDEO_FRAME_INFO_S* pSrcFrame, VIDEO_FRAME_INFO_S* pDstFrame )
{
    int nRet = 0;
    TdePicInfo_S stTdeSrcPic;
    TdePicInfo_S stTdeDstPic;
    memset( &stTdeSrcPic, 0, sizeof(TdePicInfo_S) );
    memset( &stTdeDstPic, 0, sizeof(TdePicInfo_S) );
    stTdeSrcPic.nX              = 0;
    stTdeSrcPic.nY              = 0;
    stTdeSrcPic.nW              = pSrcFrame->stVFrame.u32Width;
    stTdeSrcPic.nH              = pSrcFrame->stVFrame.u32Height;
    stTdeSrcPic.unWidth         = pSrcFrame->stVFrame.u32VirWidth;
    stTdeSrcPic.unHeight        = pSrcFrame->stVFrame.u32VirHeight;
    stTdeSrcPic.enPixFormat      = pSrcFrame->stVFrame.enPixelFormat;
    stTdeSrcPic.enCompressMode  = pSrcFrame->stVFrame.enCompressMode;
    stTdeSrcPic.Blk             =  pSrcFrame->stVFrame.pMbBlk;
    
    stTdeDstPic.nX              = 0;
    stTdeDstPic.nY              = 0;
    stTdeDstPic.nW              = pDstFrame->stVFrame.u32Width;
    stTdeDstPic.nH              = pDstFrame->stVFrame.u32Height;
    stTdeDstPic.unWidth         = pDstFrame->stVFrame.u32VirWidth;
    stTdeDstPic.unHeight        = pDstFrame->stVFrame.u32VirHeight;
    stTdeDstPic.enPixFormat      = pDstFrame->stVFrame.enPixelFormat;
    stTdeDstPic.enCompressMode  = pDstFrame->stVFrame.enCompressMode;
    
    if( pDstFrame->stVFrame.pMbBlk == NULL )
    {
        nRet = rkTde_alloc_pic( &stTdeDstPic );
        if ( nRet != RK_SUCCESS )
        {
            return -1;
        }
        pDstFrame->stVFrame.pMbBlk= stTdeDstPic.Blk;
        pDstFrame->stVFrame.pVirAddr[0]= stTdeDstPic.pData;
    }
    else
    {
        stTdeDstPic.Blk =  pDstFrame->stVFrame.pMbBlk;
        
    }
    if( stTdeDstPic.unWidth == stTdeSrcPic.unWidth && stTdeDstPic.unHeight == stTdeSrcPic.unHeight )
    {
        rkTde_copy( &stTdeSrcPic, &stTdeDstPic );
    }
    else
    {
        rkTde_resize( &stTdeSrcPic, &stTdeDstPic );
    }
    return 0;
}

/*缩放或拷贝到指定的位置*/
int resize_rect_frame( VIDEO_FRAME_INFO_S* pSrcFrame, VIDEO_FRAME_INFO_S* pDstFrame, int nX, int nY, int nW, int nH )
{
    int nRet = 0;
    TdePicInfo_S stTdeSrcPic;
    TdePicInfo_S stTdeDstPic;
    stTdeSrcPic.nX              = 0;
    stTdeSrcPic.nY              = 0;
    stTdeSrcPic.nW              = pSrcFrame->stVFrame.u32Width;
    stTdeSrcPic.nH              = pSrcFrame->stVFrame.u32Height;
    stTdeSrcPic.unWidth         = pSrcFrame->stVFrame.u32VirWidth;
    stTdeSrcPic.unHeight        = pSrcFrame->stVFrame.u32VirHeight;
    stTdeSrcPic.enPixFormat      = pSrcFrame->stVFrame.enPixelFormat;
    stTdeSrcPic.enCompressMode  = pSrcFrame->stVFrame.enCompressMode;
    stTdeSrcPic.Blk             =  pSrcFrame->stVFrame.pMbBlk;
    
    stTdeDstPic.nX              = nX;
    stTdeDstPic.nY              = nY;
    stTdeDstPic.nW              = nW;
    stTdeDstPic.nH              = nH;
    stTdeDstPic.unWidth         = pDstFrame->stVFrame.u32Width;
    stTdeDstPic.unHeight        = pDstFrame->stVFrame.u32Height;
    stTdeDstPic.enPixFormat      = pDstFrame->stVFrame.enPixelFormat;
    stTdeDstPic.enCompressMode  = pDstFrame->stVFrame.enCompressMode;
    
    if( pDstFrame->stVFrame.pMbBlk == NULL )
    {
        nRet = rkTde_alloc_pic( &stTdeDstPic );
        if ( nRet != RK_SUCCESS )
        {
            return -1;
        }
        pDstFrame->stVFrame.pMbBlk= stTdeDstPic.Blk;
        pDstFrame->stVFrame.pVirAddr[0]= stTdeDstPic.pData;
    }
    else
    {
        stTdeDstPic.Blk =  pDstFrame->stVFrame.pMbBlk;
        
    }
    if( nW == (int)stTdeSrcPic.unWidth && nH == (int)stTdeSrcPic.unHeight )
    {
        rkTde_copy( &stTdeSrcPic, &stTdeDstPic );
    }
    else
    {
        rkTde_resize( &stTdeSrcPic, &stTdeDstPic );
    }
    return 0;
}

int quickRect_frame( VIDEO_FRAME_INFO_S* pDstFrame)
{
    TdePicInfo_S stTdeDstPic;
    
    stTdeDstPic.nX              = 0;
    stTdeDstPic.nY              = 0;
    stTdeDstPic.nW              = pDstFrame->stVFrame.u32Width;
    stTdeDstPic.nH              = pDstFrame->stVFrame.u32Height;
    stTdeDstPic.unWidth         = pDstFrame->stVFrame.u32VirWidth;
    stTdeDstPic.unHeight        = pDstFrame->stVFrame.u32VirHeight;
    stTdeDstPic.enPixFormat      = pDstFrame->stVFrame.enPixelFormat;
    stTdeDstPic.enCompressMode  = pDstFrame->stVFrame.enCompressMode;
    stTdeDstPic.Blk             = pDstFrame->stVFrame.pMbBlk;

    rkTde_full(&stTdeDstPic, 0x1);
    return 0;
}

/*虚拟地址转视频帧*/
VIDEO_FRAME_INFO_S* dataToFrame( char* pData, int nSize, PIXEL_FORMAT_E enPixForMat, int nWidth, int nHeight)
{
    char* pVirData = NULL;
    MB_BLK pSrcBlk = MB_INVALID_HANDLE;
    VIDEO_FRAME_INFO_S* pFrame = (VIDEO_FRAME_INFO_S*)malloc(sizeof(VIDEO_FRAME_INFO_S));
    memset( pFrame, 0, sizeof(VIDEO_FRAME_INFO_S) );
    RK_MPI_MMZ_Alloc( &pSrcBlk, nSize, RK_MMZ_ALLOC_CACHEABLE);
    if( pSrcBlk == MB_INVALID_HANDLE )
    {
        return NULL;
    }
    pVirData = RK_MPI_MB_Handle2VirAddr( pSrcBlk );
    if( !pVirData )
    {
        return NULL;
    }
    memcpy( pVirData, pData, nSize );
    RK_MPI_SYS_MmzFlushCache( pSrcBlk, RK_FALSE);
    pFrame->stVFrame.pMbBlk         = pSrcBlk;
    pFrame->stVFrame.u32Width       = nWidth;
    pFrame->stVFrame.u32Height      = nHeight;
    pFrame->stVFrame.u32VirWidth    = nWidth;
    pFrame->stVFrame.u32VirHeight   = nHeight;
    pFrame->stVFrame.enPixelFormat  = enPixForMat;
    pFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    return pFrame;
}
/*虚拟地址编码成MJPEG*/
int dataToMjpeg(int nChn, int nWidth, int nHeight, PIXEL_FORMAT_E eFormat, 
                char* pInData, int nInSize,char **pOutData, int* pOutSize )
{
    int nRet = 0;
    RkVencNeedParam_S stParam;
    VENC_STREAM_S stFrame;
    VENC_PACK_S stPacks[10]; 
    memset( &stParam, 0, sizeof( RkVencNeedParam_S ) );
    memset( &stFrame, 0, sizeof( VENC_STREAM_S ) );
    /*默认赋值*/
    stParam.unWidth         = nWidth;
    stParam.unHeight        = nHeight;
    stParam.unVirWidth      = nWidth;
    stParam.unVirHeight     = nHeight;
    stParam.enPixFormat      = eFormat;
    stParam.enCodec          = RK_VIDEO_ID_MJPEG;
    stParam.nGop            = 1;
    stParam.nChn      = nChn;
    stParam.nInFrameRate    = 1; 
    stParam.nOutFrameRate   = 1;
    stParam.enCompressMode = COMPRESS_MODE_NONE;
    RkVenc_S* pHandle    = rockitVenc_alloc(stParam);
    /*编码帧数*/
    pHandle->stExParam.nSnapPicCount = 1;
    pHandle->stExParam.unMbCnt = 1; 
    nRet = pHandle->rockitVenc_init( pHandle);
    if( nRet != RK_SUCCESS )
    {
        rockitVenc_release( pHandle );
        return -1;   
    }
    /*编码*/
    nRet = pHandle->rockitVenc_send_frame( pHandle, pInData, nInSize, NULL );
    if( nRet != RK_SUCCESS )
    {
        pHandle->rockitVenc_unInit( pHandle );
        rockitVenc_release( pHandle );
        return -1;   
    }
    nRet = pHandle->rockitVenc_get_stream( pHandle, &stFrame, stPacks, 10 ,200);
    if( nRet != RK_SUCCESS )
    {
        pHandle->rockitVenc_unInit( pHandle );
        rockitVenc_release( pHandle );
        return -1;   
    }

    /* 计算全帧总长度 (SPS + PPS + IDR)*/
    uint32_t nTotalLen = 0;
    for (uint32_t i = 0; i < stFrame.u32PackCount; i++) 
    {
        nTotalLen += stFrame.pstPack[i].u32Len;
    }
    
    /*拿编码后的虚拟地址*/
    char* pEncData = (char*)pHandle->rockitVenc_get_streamVirdata( &stFrame.pstPack[0]);
    if( nRet != RK_SUCCESS )
    {
        pHandle->rockitVenc_unInit( pHandle );
        rockitVenc_release( pHandle );
        return -1;   
    }
    *pOutData = (char*)malloc( nTotalLen );
    memcpy( *pOutData, pEncData, nTotalLen);
    pHandle->rockitVenc_release_stream( pHandle, &stFrame );
    *pOutSize = nTotalLen;
    pHandle->rockitVenc_unInit( pHandle );
    rockitVenc_release( pHandle );
    return 0;
}
/*将frame编码成MJPEG*/
int frameToMjpeg(int nChn, VIDEO_FRAME_INFO_S* pVFrame, char **pOutData, int* pOutSize )
{
    int nRet = 0;
    RkVencNeedParam_S stParam;
    VENC_STREAM_S stFrame;
    VENC_PACK_S stPacks[10]; 
    memset( &stParam, 0, sizeof( RkVencNeedParam_S ) );
    memset( &stFrame, 0, sizeof( VENC_STREAM_S ) );
    /*默认赋值*/
    stParam.unWidth         = pVFrame->stVFrame.u32Width;
    stParam.unHeight        = pVFrame->stVFrame.u32Height;
    stParam.unVirWidth      = pVFrame->stVFrame.u32VirWidth;
    stParam.unVirHeight     = pVFrame->stVFrame.u32VirHeight;
    stParam.enPixFormat      = pVFrame->stVFrame.enPixelFormat;
    stParam.enCodec          = RK_VIDEO_ID_MJPEG;
    stParam.nGop            = 10;
    stParam.nChn      = nChn;
    stParam.nInFrameRate    = 60; 
    stParam.nOutFrameRate   = 60;
    stParam.enCompressMode  = pVFrame->stVFrame.enCompressMode;//COMPRESS_MODE_NONE;
    RkVenc_S* pHandle = NULL;
        
        pHandle    = rockitVenc_alloc(stParam);
        /*编码帧数*/
        pHandle->stExParam.nSnapPicCount = -1;
        pHandle->stExParam.unMbCnt = 3; 
        nRet = pHandle->rockitVenc_init( pHandle);
        if( nRet != RK_SUCCESS )
        {
            rockitVenc_release( pHandle );
            pHandle = NULL;
            return -1; 
        }
    /*编码*/
    nRet = pHandle->rockitVenc_send_VFrame( pHandle, pVFrame, 5 );
    if( nRet != RK_SUCCESS )
    {
        printf("发送编码失败jpeg%x\n", nRet);
        return -1;   
    }
    nRet = pHandle->rockitVenc_get_stream( pHandle, &stFrame, stPacks, 10 ,200);
    if( nRet != RK_SUCCESS )
    {
        printf("get编码失败jpeg%x\n", nRet);
        return -1;   
    }

    /* 计算全帧总长度 (SPS + PPS + IDR)*/
    uint32_t nTotalLen = 0;
    for (uint32_t i = 0; i < stFrame.u32PackCount; i++) 
    {
        nTotalLen += stFrame.pstPack[i].u32Len;
    }

    /*拿编码后的虚拟地址*/
    char* pEncData = (char*)pHandle->rockitVenc_get_streamVirdata( &stFrame.pstPack[0] );
    if( nRet != RK_SUCCESS )
    {
        return -1;   
    }
    *pOutData = (char*)malloc( nTotalLen );
    memcpy( *pOutData, pEncData, nTotalLen);
    pHandle->rockitVenc_release_stream(pHandle, &stFrame);
    *pOutSize = nTotalLen;
    pHandle->rockitVenc_unInit( pHandle );
    rockitVenc_release( pHandle );
    return 0;
}

void copyVideoFrame(VIDEO_FRAME_INFO_S *pstDst, const VIDEO_FRAME_INFO_S *pstSrc)
{
    // 拷贝基础元数据
    memcpy(pstDst, pstSrc, sizeof(VIDEO_FRAME_INFO_S));

    // 处理MB内存块拷贝
    if (pstSrc->stVFrame.pMbBlk)
    {
        // 获取源内存块信息
        RK_U8 *u8SrcVir = (RK_U8 *)RK_MPI_MB_Handle2VirAddr(pstSrc->stVFrame.pMbBlk);
        RK_U64 u64SrcSize = RK_MPI_MB_GetSize(pstSrc->stVFrame.pMbBlk);

        // 创建新内存块（关键步骤）
        MB_BLK newMb;
        RK_MPI_MMZ_Alloc(&newMb, u64SrcSize, RK_MMZ_ALLOC_CACHEABLE);
        RK_U8 *u8DstVir = (RK_U8 *)RK_MPI_MB_Handle2VirAddr(newMb);

        // 拷贝实际数据
        memcpy(u8DstVir, u8SrcVir, u64SrcSize);

        // 更新目标结构体指针
        pstDst->stVFrame.pMbBlk = newMb;
    }
}

void freeVideoFrame(VIDEO_FRAME_INFO_S *pstFrame)
{
    if (pstFrame->stVFrame.pMbBlk)
    {
        RK_MPI_MMZ_Free(pstFrame->stVFrame.pMbBlk);
    }
}
