/*************************************************************************
	> File Name: main.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2023年07月24日 星期一 15时14分33秒
 ************************************************************************/

#include<stdio.h>
#include"opencv_vda.h"
#include "opencv_face_analyse.h"

int main()
{
    CvVdaNeedParam_S stNeedParam;
    stNeedParam.nWidth = 1920;
    stNeedParam.nWidth = 1920;
    stNeedParam.enDetectType = SHARE_DETECT;
    CvVda_S* pHandle = opencvVda_alloc( stNeedParam );
    pHandle->opencvVda_init( pHandle );

    CvFaceAnalyseNeedParam_S stAnalyseNeedParam;
    memset(&stAnalyseNeedParam, 0, sizeof(CvFaceAnalyseNeedParam_S));
    stAnalyseNeedParam.nWidth = 1920;
    stAnalyseNeedParam.nHeight = 1080;
    stAnalyseNeedParam.pModlePath = "/home/luoyk/face_analyse/models";
    CvFaceAnalyse_S *pAnalyseHandle = opencv_face_analyse_alloc(stAnalyseNeedParam);
    pAnalyseHandle->init(pAnalyseHandle);
    while(1)
    {
        sleep(1);
    }
    return 0;    
}
