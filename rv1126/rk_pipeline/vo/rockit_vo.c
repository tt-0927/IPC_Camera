/*************************************************************************
	> File Name: rockit_vo.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2023年05月22日 星期一 14时55分28秒
 ************************************************************************/
#include "rockit_vo.h"
int rockitVo_WbcInit(RkVo_S *pHandle)
{
    VO_WBC_SOURCE_S stWbcSource;
    VO_WBC_ATTR_S stWbcAttr;
    VO_WBC_MODE_E enWbcMode;
    RK_U32 VoWbc;
    RK_S32 s32Ret = RK_SUCCESS;
    MPP_CHN_S stSrcChn, stDestChn;
    VoWbc = 0;
    stWbcSource.enSourceType = VO_WBC_SOURCE_DEV;
    stWbcSource.u32SourceId = 1;
    RK_MPI_VO_SetWbcSource(VoWbc, &stWbcSource);
    stWbcAttr.stTargetSize.u32Width = 1920;
    stWbcAttr.stTargetSize.u32Height = 1080;
    stWbcAttr.enPixelFormat = RK_FMT_YUV422_YUYV;
    stWbcAttr.u32FrameRate = 60;
    stWbcAttr.enCompressMode = COMPRESS_MODE_NONE;
    s32Ret = RK_MPI_VO_SetWbcAttr(VoWbc, &stWbcAttr);
    if (s32Ret != RK_SUCCESS)
    {
        return s32Ret;
    }
    s32Ret = RK_MPI_VO_EnableWbc(VoWbc);
    if (s32Ret != RK_SUCCESS)
    {
        return s32Ret;
    }

    RK_MPI_VO_SetWbcDepth(VoWbc,4);
    
    return 0;
}

int rockitVo_WbcUninit(RkVo_S *pHandle)
{
    int nRet = 0;
    RK_U32 VoWbc = 0;
    /* disable wbc */
    nRet = RK_MPI_VO_DisableWbc(VoWbc);
    if (nRet != RK_SUCCESS)
    {
        return nRet;
    }
    return 0;
}

/*送图像帧到图层通道 
*@inparam nChn -1 直接送图层， 不为-1 表示送图层通道
*@return 0 成功  -1 表示图层或者通道或者参数有误  其它失败
* */
int rockitVo_send_frame(RkVo_S *pHandle, VIDEO_FRAME_INFO_S *pFrame,  int nLayer, int nChn, int nMilliSec)
{
    int nRet = RK_FAILURE;
    if(NULL == pHandle || NULL == pFrame)
    {
        return RK_FAILURE;
    }
    for( int i=0; i<pHandle->stNeedParam.nLayerNum; i++ )
    {
        int nVoLayer = pHandle->stNeedParam.aLayer[i];
        if( nVoLayer == nLayer )
        {
            if( -1 == nChn  )
            {
                nRet = RK_MPI_VO_SendLayerFrame( nVoLayer, pFrame);
            }
            else if( pHandle->stNeedParam.aChnSum[i] > nChn )
            {
                nRet = RK_MPI_VO_SendFrame( nVoLayer, nChn, pFrame, nMilliSec);
            }
        }
    }
    return  nRet;
}
/*隐藏某个图层的通道显示*/
int rockitVo_isShowChn( RkVo_S *pHandle, int nLayer, int nChn, RK_BOOL bShow)
{
    if( NULL == pHandle )
    {
        return -1;
    }
    RK_S32 (*funtion)( int, int ) = NULL;
    if( bShow )
    {
        funtion = RK_MPI_VO_ShowChn;
    }
    else
    {
        funtion = RK_MPI_VO_HideChn;
    }

    for( int i=0; i<pHandle->stNeedParam.nLayerNum; i++ )
    {
        if( pHandle->stNeedParam.aLayer[i] == nLayer )
        {
            if( pHandle->stNeedParam.aChnSum[i] > nChn )
            {
                funtion( nLayer, nChn );
                return 0;
            }
        }
    }
    return -1;
}

/*重新初始化vo*/
int rockit_vo_reset(RkVo_S *pHandle, RkVoNeedParam_S* pNeedParam, RkVoExparam_S *pExParam)
{
    if( NULL == pNeedParam && NULL == pExParam )
    {
        pHandle->rockit_vo_uninit( pHandle );
        pHandle->rockit_vo_init( pHandle );
    }
    return 0;
}

/*初始化vo*/
int rockit_vo_init(RkVo_S *pHandle)
{
    VO_PUB_ATTR_S           stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S   stLayerAttr;
    VO_CHN_ATTR_S           stChnAttr;
    VO_CHN_PARAM_S          stChnParam;
    VO_CSC_S                stVideoCSC;
    VO_DEV                  nVoDev;
    VO_LAYER                nVoVideoLayer;
    RK_S32 nRet   = RK_SUCCESS;
    int nWidth      = 0;
    int nHeight     = 0;
    memset( &stVoPubAttr, 0, sizeof(VO_PUB_ATTR_S) );
    memset( &stLayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S) );
    memset(&stChnAttr, 0 ,sizeof(VO_CHN_ATTR_S));
    memset(&stChnParam, 0 ,sizeof(VO_CHN_PARAM_S));
    memset( &stVideoCSC, 0, sizeof(VO_CSC_S) );
    nVoDev  = pHandle->stNeedParam.nDev;
    nWidth  = pHandle->stNeedParam.nDisWidth;
    nHeight = pHandle->stNeedParam.nDisHeight;
    
    /*图层绑定设备*/
    for( int i=0; i < pHandle->stNeedParam.nLayerNum; i++ )
    {
	    nVoVideoLayer = pHandle->stNeedParam.aLayer[i];
	    nRet = RK_MPI_VO_BindLayer(nVoVideoLayer, nVoDev, pHandle->stExParam.aLayerMode[i]);
	    if ( nRet != RK_SUCCESS )
	    {
	        printf("RK_MPI_VO_BindLayer fail %x\n", nRet);
	        return RK_FAILURE;
	    }
    }
    
    /*开启显示设备*/
    stVoPubAttr.enIntfType = pHandle->stNeedParam.enIntfType;
    stVoPubAttr.enIntfSync = pHandle->stNeedParam.enIntfSync;
    stVoPubAttr.u32BgColor = pHandle->stExParam.nBgColor;
    nRet = RK_MPI_VO_SetPubAttr( nVoDev, &stVoPubAttr );
    if ( nRet != RK_SUCCESS )
    {
        printf("set vodev fail %x\n", nRet);
        return RK_FAILURE;
    }
    nRet = RK_MPI_VO_Enable(nVoDev);
    if (nRet != RK_SUCCESS)
    {
        printf("enable vodev fail %x\n",nRet);
        return RK_FAILURE;
    }
    /*设置图层*/
    for( int i=0; i < pHandle->stNeedParam.nLayerNum; i++ )
    {
        nVoVideoLayer = pHandle->stNeedParam.aLayer[i];
        RK_MPI_VO_SetLayerDispBufLen(nVoVideoLayer, pHandle->stExParam.aVoBufLen[i]);

        stLayerAttr.stDispRect.s32X              = 0;
        stLayerAttr.stDispRect.s32Y              = 0;
        stLayerAttr.stDispRect.u32Width          = nWidth;
        stLayerAttr.stDispRect.u32Height         = nHeight;
        stLayerAttr.enPixFormat                  = pHandle->stExParam.aPixFormat[i];
        stLayerAttr.u32DispFrmRt                 = pHandle->stExParam.aDisFrameRate[i];
        stLayerAttr.stImageSize.u32Width         = pHandle->stNeedParam.nImageWidth;
        stLayerAttr.stImageSize.u32Height        = pHandle->stNeedParam.nImageHeight;
        stLayerAttr.enCompressMode               = pHandle->stExParam.aCompressMode[i];
        stLayerAttr.bBypassFrame = pHandle->stExParam.aBypassFrame[i];
        stLayerAttr.bLowDelay = pHandle->stExParam.aLowDelay[i];

        nRet = RK_MPI_VO_SetLayerSpliceMode(nVoVideoLayer, pHandle->stExParam.aLayerSpliceMode[i]);
        if (RK_SUCCESS != nRet)
        {
            printf("setLayerSpliceMode %d fail %x\n", nVoVideoLayer, nRet);
            return RK_FAILURE;
        }
        nRet = RK_MPI_VO_SetLayerAttr(nVoVideoLayer, &stLayerAttr);
        if (RK_SUCCESS != nRet)
        {
            printf("setLayerAttr %d fail %x\n", nVoVideoLayer, nRet);
            return RK_FAILURE;
        }
        nRet = RK_MPI_VO_EnableLayer(nVoVideoLayer);
        if (RK_SUCCESS != nRet)
        {
            printf("enableLayer %d fail %x\n", nVoVideoLayer, nRet);
            return RK_FAILURE;
        }
        stVideoCSC.enCscMatrix  = VO_CSC_MATRIX_IDENTITY;
        stVideoCSC.u32Contrast  = 50;
        stVideoCSC.u32Hue       = 50;
        stVideoCSC.u32Luma      = 50;
        stVideoCSC.u32Satuature = 50;
        nRet = RK_MPI_VO_SetLayerCSC( nVoVideoLayer, &stVideoCSC );
        if ( nRet != RK_SUCCESS) 
        {
            printf("set Layer csc %d fail %x\n", nVoVideoLayer, nRet);
        }
    }
    /*设置hdmi输出格式*/
    if( pHandle->stExParam.bHdmiFmt )
    {
        VO_HDMI_PARAM_S stHDMIParam;
        memset( &stHDMIParam, 0, sizeof(VO_HDMI_PARAM_S) );
        RK_MPI_VO_GetHdmiParam( pHandle->stNeedParam.enIntfType, nVoDev, &stHDMIParam);
        stHDMIParam.enColorFmt      =  pHandle->stExParam.enColorFmt;
        stHDMIParam.enQuantRange    = pHandle->stExParam.enQuantRange;
        stHDMIParam.enHdmiMode      =  pHandle->stExParam.enHdmiMode;
        RK_MPI_VO_SetHdmiParam( pHandle->stNeedParam.enIntfType, nVoDev, &stHDMIParam);
    }
    /*设置插拔回调*/
    if( pHandle->stExParam.pVoCallBack != NULL )
    {
        RK_VO_CALLBACK_FUNC_S stCallbackFunc;
        stCallbackFunc.pfnEventCallback = pHandle->stExParam.pVoCallBack;
        stCallbackFunc.pPrivateData = pHandle->stExParam.pPrivateData;
        nRet = RK_MPI_VO_RegCallbackFunc( pHandle->stNeedParam.enIntfType, nVoDev,&stCallbackFunc);
        if ( RK_SUCCESS != nRet)
        {
            printf("callback video nVoDev %d fail %d \n", nVoDev, nRet);
            return RK_FAILURE;
        }
    }

    /*设置视频图层的通道*/
    for( int i=0; i < pHandle->stNeedParam.nLayerNum; i++ )
    {
        nVoVideoLayer = pHandle->stNeedParam.aLayer[i];
        nRet = RK_MPI_VO_GetLayerAttr(nVoVideoLayer, &stLayerAttr);//获取通道属性
        if ( RK_SUCCESS != nRet )
        {
            printf("get video Layer fail %d \n", nRet);
            return RK_FAILURE;
        }
        RkVoRect_S* pRect           = pHandle->stNeedParam.pChnRect[ i ];
        for( int nChn=0; nChn < pHandle->stNeedParam.aChnSum[i]; nChn++ )
        {
            stChnAttr.u32Priority       = nChn;
            stChnAttr.stRect.s32X       = pRect[nChn].nPosX;
            stChnAttr.stRect.s32Y       = pRect[nChn].nPosY;
            stChnAttr.stRect.u32Width   = pRect[nChn].nWidth;
            stChnAttr.stRect.u32Height  = pRect[nChn].nHeight;
            stChnAttr.u32FgAlpha        = 255;
            stChnAttr.u32BgAlpha        = 255;
            stChnAttr.enMirror          = MIRROR_NONE;
            stChnAttr.enRotation        = ROTATION_0;
            /*设置通道属性*/
            nRet = RK_MPI_VO_SetChnAttr(nVoVideoLayer, nChn, &stChnAttr);
            if (RK_SUCCESS != nRet)
            {
                printf("set VoVideoLayer %d attr fail  %x %d\n", nVoVideoLayer, nRet, nChn);
                return RK_FAILURE;
            }
            /*设置通道参数*/
            stChnParam.stAspectRatio.enMode                 = ASPECT_RATIO_NONE;
            stChnParam.stAspectRatio.stVideoRect.s32Y       = pRect[nChn].nPosX;
            stChnParam.stAspectRatio.stVideoRect.s32X       = pRect[nChn].nPosX; 
            stChnParam.stAspectRatio.stVideoRect.u32Width   = pRect[nChn].nWidth;
            stChnParam.stAspectRatio.stVideoRect.u32Height  = pRect[nChn].nHeight;
            RK_MPI_VO_SetChnParam(nVoVideoLayer, nChn, &stChnParam);
            RK_MPI_VO_SetChnFrameRate(nVoVideoLayer, nChn, pHandle->stExParam.aChnFrameRate[i]);
            nRet = RK_MPI_VO_EnableChn(nVoVideoLayer, nChn);
            if ( nRet != RK_SUCCESS )
            {
                printf("enable VoVideoLayer chnn fail %x \n", nRet);
                return RK_FAILURE;
            }
            RK_MPI_VO_SetChnRecvThreshold(nVoVideoLayer, nChn, 1);
        }
    }
    return 0;
}

/*
 * *@description vo反初始化
 * *@Author: wxz
 * *@param[in] 
 * *@return 成功返回0,失败返回-1
 * */
int rockit_vo_uninit(RkVo_S *pHandle)
{
    int nRet=RK_SUCCESS;
    if(!pHandle) 
    {
        printf("rockit vo uninit error\n");
        return RK_FAILURE;
    }
    /*关闭图层通道*/
    for( int i=0; i<pHandle->stNeedParam.nLayerNum; i++ )
    {
        for(int nChn=0; nChn < pHandle->stNeedParam.aChnSum[i]; nChn++)
        {
            RK_MPI_VO_PauseChn( pHandle->stNeedParam.aLayer[i], nChn );
            RK_MPI_VO_RefreshChn( pHandle->stNeedParam.aLayer[i], nChn );
            RK_MPI_VO_ClearChnBuffer( pHandle->stNeedParam.aLayer[i], nChn, RK_TRUE );
            RK_MPI_VO_DisableChn( pHandle->stNeedParam.aLayer[i], nChn);
        }
    }
    /*关闭图层*/
    for( int i=0; i<pHandle->stNeedParam.nLayerNum; i++ )
    {
        nRet = RK_MPI_VO_DisableLayer(pHandle->stNeedParam.aLayer[i]);
        if(RK_SUCCESS != nRet)
        {
            printf("disable layer fail %x \n",nRet);
            return RK_FAILURE;
        }
    }
    /*关闭设备*/
    nRet = RK_MPI_VO_Disable(pHandle->stNeedParam.nDev);
    if(RK_SUCCESS != nRet)
    {
        printf("disable VoDev %d fail %x \n",pHandle->stNeedParam.nDev, nRet);
        return RK_FAILURE;
    }
    /*解绑图层绑定*/
    for( int i=0; i<pHandle->stNeedParam.nLayerNum; i++ )
    {
        RK_MPI_VO_UnBindLayer(pHandle->stNeedParam.aLayer[i], pHandle->stNeedParam.nDev);
    }
    return nRet;
}

/*分配vo句柄*/
RkVo_S *rockit_vo_alloc(RkVoNeedParam_S stNeedParam)
{
    int i = 0;
    RkVo_S *pHandle = (RkVo_S*)malloc(sizeof(RkVo_S));
    if(NULL == pHandle)
    {
        printf("rockit voHdle malloc error\n");
        return NULL;
    }
    memset(pHandle, 0, sizeof(RkVo_S));
    memcpy( &pHandle->stNeedParam, &stNeedParam, sizeof(RkVoNeedParam_S));
    /*通道宽高的赋值*/
    for( i=0; i<stNeedParam.nLayerNum; i++ )
    {
        int nChnSum = stNeedParam.aChnSum[i];
        pHandle->stNeedParam.pChnRect[i] = (RkVoRect_S *)malloc( nChnSum * sizeof(RkVoRect_S) );
        RkVoRect_S* pVoAttrPtr  = pHandle->stNeedParam.pChnRect[ i ];
        RkVoRect_S* pVoParamPtr = stNeedParam.pChnRect[ i ];
        for( int nVoChn=0; nVoChn < nChnSum; nVoChn++ )
        {
            
            pVoAttrPtr[nVoChn].nPosX       = pVoParamPtr[nVoChn].nPosX;
            pVoAttrPtr[nVoChn].nPosY       = pVoParamPtr[nVoChn].nPosY;
            pVoAttrPtr[nVoChn].nWidth      = pVoParamPtr[nVoChn].nWidth;
            pVoAttrPtr[nVoChn].nHeight     = pVoParamPtr[nVoChn].nHeight;
        }
    }
    /*非必要参数直接先给默认值*/
    for( i=0; i<stNeedParam.nLayerNum; i++ )
    {
        pHandle->stExParam.aLayerMode[i]       = VO_LAYER_MODE_VIDEO; //VO_LAYER_MODE_GRAPHIC;
        pHandle->stExParam.aBypassFrame[i]     = RK_FALSE;  //RK_TRUE 时，视频直接送往图层
        pHandle->stExParam.aLowDelay[i]        = RK_FALSE;
        pHandle->stExParam.aVoBufLen[i]        = 6;
        pHandle->stExParam.aPixFormat[i]       = RK_FMT_RGB888;
        pHandle->stExParam.aDisFrameRate[i]    = 30;
        pHandle->stExParam.aChnFrameRate[i]    = 30;
        pHandle->stExParam.aLayerSpliceMode[i] = VO_SPLICE_MODE_GPU;
        pHandle->stExParam.aCompressMode[i]    = COMPRESS_MODE_NONE;
    }

    pHandle->stExParam.pPrivateData = NULL;
    pHandle->stExParam.pVoCallBack = NULL;
    pHandle->stExParam.bHdmiFmt = RK_FALSE;
    pHandle->stExParam.enColorFmt = VO_HDMI_COLOR_FORMT_AUTO;
    pHandle->stExParam.enHdmiMode = VO_HDMI_MODE_AUTO;
    pHandle->stExParam.enQuantRange = VO_HDMI_QUANT_RANGE_AUTO;

    pHandle->rockitVo_WbcInit       = rockitVo_WbcInit;
    pHandle->rockitVo_WbcUninit     = rockitVo_WbcUninit;
    pHandle->rockitVo_send_frame = rockitVo_send_frame;
    pHandle->rockitVo_isShowChn     = rockitVo_isShowChn;
    pHandle->rockit_vo_reset        = rockit_vo_reset;
    pHandle->rockit_vo_init         = rockit_vo_init;
    pHandle->rockit_vo_uninit       = rockit_vo_uninit;
    return pHandle;
}

/*释放vo句柄*/
int rockit_vo_release( RkVo_S* pHandle )
{
    for( int i=0; i<pHandle->stNeedParam.nLayerNum; i++ )
    {
        if( pHandle->stNeedParam.pChnRect[i] )
        {
            free( pHandle->stNeedParam.pChnRect[i] );
            pHandle->stNeedParam.pChnRect[i] = NULL;
        }
    }
    free(pHandle);
    pHandle = NULL;
    return 0;
}


