/*************************************************************************
	> File Name: opencv_vda.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2023年07月17日 星期一 15时05分34秒
 ************************************************************************/

#ifndef _opencv_VDA_H
#define _opencv_VDA_H
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "rk_compare_image.h"


typedef enum _DETECTTYPE{
    
    /*动态检测  与上一帧传入的图像进行比较*/
    SHARE_DETECT,
    /*静态检测  与第一次传入的图像进行比较*/
    STATIC_DETECT,
    DETECT_BUTT

}DetectType;


typedef struct _CVVDANEEDPARAM_{
 
    /*图像宽*/
    int nWidth;
    /*图像高*/
    int nHeight;
    /*检测类型*/
    DetectType enDetectType;
    /*图像类型 目前未使用，只支持BGR888格式数据*/
    int nType;

}CvVdaNeedParam_S;

typedef struct _CVVDAEXPARAM_{
    
    /*检测间隔 默认是5*/
    int nInterVal;

}CvVdaExParam_S;



typedef struct CvVDA_ CvVda_S;
struct CvVDA_{
    
    /*初始化*/
    int (*opencvVda_init)( CvVda_S* pHandle );
    /*反初始化*/
    int (*opencvVda_uninit)( CvVda_S* pHandle );
    /*发送数据
     *@RETURN 返回图像差异比值
     * */
    float (*opencvVda_send_fram)(CvVda_S* pHandle, char* pData, int nSize);
    /*必须参数*/
    CvVdaNeedParam_S stNeedParam;
    CvVdaExParam_S stExParam;
    int64_t nFrameNumm;
    char* pData;
};

/*分配句柄*/
CvVda_S* opencvVda_alloc( CvVdaNeedParam_S stNeedParam );
/*释放句柄*/
int opencvVda_release( CvVda_S* pHandle );
#endif
