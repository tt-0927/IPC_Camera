/*************************************************************************
	> File Name: opencv_vda.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2023年07月17日 星期一 15时25分03秒
 ************************************************************************/

#include"opencv_vda.h"

/*发送数据*/
static float opencvVda_send_fram(CvVda_S* pHandle, char* pData, int nSize)
{
    float fDiff = 0;
    int nImageSize = pHandle->stNeedParam.nWidth * pHandle->stNeedParam.nHeight * 3;
    if( nSize <  nImageSize )
    {
        return -1;
    }
    CompareImageParam_S stCompareImageParam;
    memset(&stCompareImageParam, 0, sizeof(CompareImageParam_S));
    stCompareImageParam.pImage1Buffer = pHandle->pData;
    stCompareImageParam.pImage2Buffer = pData;
    stCompareImageParam.nLength1      = nSize;
    stCompareImageParam.nLength2      = nSize;
    stCompareImageParam.nWidth        = pHandle->stNeedParam.nWidth; 
    stCompareImageParam.nHeight       = pHandle->stNeedParam.nHeight;
    /*比较*/
    if( pHandle->nFrameNumm % pHandle->stExParam.nInterVal == 0 )
    {
        fDiff = compareImage_Image_Threshold(stCompareImageParam);
    }
        /*保存数据*/
        if( pHandle->stNeedParam.enDetectType == SHARE_DETECT )
        {
            memcpy( pHandle->pData, pData, nImageSize );
        }
        else if( pHandle->stNeedParam.enDetectType == STATIC_DETECT && pHandle->nFrameNumm == 0 )
        {
            memcpy( pHandle->pData, pData, nImageSize );
        }

    pHandle->nFrameNumm++;
    return fDiff;
}

/*初始化*/
static int opencvVda_init( CvVda_S* pHandle)
{
    /*分配图像内存*/
    int nSize = pHandle->stNeedParam.nWidth * pHandle->stNeedParam.nHeight * 3;
    pHandle->pData = (char*)malloc( nSize );
    memset( pHandle->pData, 0,  nSize );
    return 0;
}

/*反初始化*/
static int opencvVda_uninit( CvVda_S* pHandle )
{
    if( pHandle && pHandle->pData )
    {
        free( pHandle->pData );
        pHandle->pData = NULL;
    }
    return 0;
}

/*分配句柄*/
CvVda_S* opencvVda_alloc(  CvVdaNeedParam_S stNeedParam )
{
    CvVda_S* pHandle = ( CvVda_S* )malloc( sizeof(CvVda_S) );
    memset( pHandle, 0, sizeof(CvVda_S) );
    memcpy( &pHandle->stNeedParam, &stNeedParam, sizeof( CvVdaNeedParam_S ) );

    pHandle->stExParam.nInterVal    = 5; 
    pHandle->opencvVda_init         = opencvVda_init;
    pHandle->opencvVda_uninit       = opencvVda_uninit;
    pHandle->opencvVda_send_fram    = opencvVda_send_fram;
    return pHandle;
}

/*释放句柄*/
int opencvVda_release( CvVda_S*pHandle )
{
    if( pHandle )
    {
        free( pHandle );
        pHandle = NULL;
    }
    return 0;
}
