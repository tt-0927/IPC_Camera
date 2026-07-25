/*
 * @Author       : EasonLu
 * @Date         : 2023-07-27 09:26:01
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-08-05 09:12:26
 * @FilePath     : opencv_face_analyse.h
 * @Description  : 人脸分析
 */
#ifndef _OPENCV_FACE_ANALYSE_H_
#define _OPENCV_FACE_ANALYSE_H_
#include "os_que.h"
#include "os_thr.h"
#include "os_mutex.h"


/* 设置需要进行人脸分析的数据 */
typedef struct _CvFaceAnlyseInputData_S_
{
    /* 分析的图片数据地址 */
    char *pImageData;
    /* 分析的图片数据大小 */
    int nDataLen;
    /* 用户自定义参数，会从输出数据中带回 */
    void *pUsrParam;
    /* 用户自定义需要释放的参数 */
    void *pFreeParam;
    /* 释放函数指针 */
    void (*fnFree)(void *pAddr, void *pFreeParam);
    /* 只送分析，不取回图像数据，标记位置1时OutputData为图像指针为NULL */
    int bAnalyseOnly;
} CvFaceAnlyseInputData_S;

/* 坐标数据结构 */
typedef struct _CvFaceAnalysePos_S_
{
    float fX;   /* X坐标 */
    float fY;   /* Y坐标 */
} CvFaceAnalysePos_S;

/* 获取人脸分析的数据结构 */
typedef struct _CvFaceAnlyseOutputData_S_
{
    /* 分析的图片数据地址 */
    unsigned char *pImageData;
    /* 分析的图片数据大小 */
    int nDataLen;
    /* 识别图片中的人数 */
    int nPeopleNum;
    /* 人数的坐标数组指针，长度为识别人数 */
    CvFaceAnalysePos_S *pstPos;
    /* 分析时间（单位：毫秒） */
    int nAnalyseTime;
    /* 用户自定义参数，从输入数据中传入 */
    void *pUsrParam;
} CvFaceAnlyseOutputData_S;

/* 分析图片的像素格式 */
typedef enum _CvFaceAnalyseImageFormat_E_
{
    CV_FACE_ANALYSE_IMAGE_FORMAT_RGB888 = 0, /* RGB888格式 */
} CvFaceAnalyseImageFormat_E;

typedef struct _CvFaceAnalyseNeedParam_S_
{
    /* 图片宽度 */
    int nWidth;
    /* 图片高度 */
    int nHeight;
    /* 未使用，目前只支持RGB图片格式 */
    CvFaceAnalyseImageFormat_E enImageFormat;
    /* 使用的人脸分析模型绝对路径 */
    char *pModlePath;
    /* 分析间隔 */
    int nInterval;
} CvFaceAnalyseNeedParam_S;

typedef struct _CvFaceAnalyseExParam_S_
{
    /* 分析图片的队列大小，默认为30 */
    int nQueSize;
} CvFaceAnalyseExParam_S;

typedef struct opencv_face_analyse CvFaceAnalyse_S;
struct opencv_face_analyse
{
    /* 初始化 */
    int (*init)(CvFaceAnalyse_S *pHandle);
    /* 反初始化 */
    int (*uninit)(CvFaceAnalyse_S *pHandle);
    /* 发送需要分析的图片 */
    int (*send_image)(CvFaceAnalyse_S *pHandle, CvFaceAnlyseInputData_S *pstData, int nTimeout);
    /* 获取已经分析完毕的数据 */
    int (*get_data)(CvFaceAnalyse_S *pHandle, CvFaceAnlyseOutputData_S *pstData, int nTimeout);
    /* 清空队列数据 */
    int (*clear_data)(CvFaceAnalyse_S *pHandle);
    /* 释放已经分析完毕的数据 */
    int (*release_data)(CvFaceAnlyseOutputData_S *pstData);
    /* 必须参数 */
    CvFaceAnalyseNeedParam_S stNeedParam;
    /* 额外参数 */
    CvFaceAnalyseExParam_S stExParam;

    /* ======================================== */
    int nImageSize;             /* 图片大小，根据设置的宽高及图片格式进行内部计算 */
    int bThrExit;               /* 线程退出标志 */
    OS_MutexHndl stMutexInput;  /* 输入队列操作锁 */
    OS_QueHndl stInputQue;      /* 分析图片的输入数据队列 */
    OS_ThrHndl stAnalyseThr;    /* 分析图片的线程 */
    OS_MutexHndl stMutexOutput; /* 输出队列操作锁 */
    OS_QueHndl stOutputQue;     /* 分析图片完毕的输出数据的队列 */
};

/**
 * @brief  分配人脸分析句柄
 * @param  [CvFaceAnalyseNeedParam_S] stNeedParam
 * @return [*]
 * @author EasonLu
 * @note
 */
CvFaceAnalyse_S *opencv_face_analyse_alloc(CvFaceAnalyseNeedParam_S stNeedParam);

/**
 * @brief  释放人脸分析句柄
 * @param  [CvFaceAnalyse_S] *pHandle
 * @return [*]
 * @author EasonLu
 * @note
 */
int opencv_face_analyse_release(CvFaceAnalyse_S *pHandle);

#endif /* _OPENCV_FACE_ANALYSE_H_ */
