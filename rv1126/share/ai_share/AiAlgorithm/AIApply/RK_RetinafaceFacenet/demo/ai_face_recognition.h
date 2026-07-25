/*
 * @Author       : chenchl
 * @Date         : 2023-11-15 09:26:01
 * @LastEditors  : chenchl
 * @LastEditTime : 2023-11-15 09:12:26
 * @FilePath     : ai_face_recognition.h
 * @Description  : 人脸识别跟踪分析
 */
#ifndef _AI_SLIDEWINDOW_TRACK_H_
#define _AI_SLIDEWINDOW_TRACK_H_
#include "os_que.h"
#include "os_thr.h"
#include "os_mutex.h"


/* 设置需要进行人脸识别跟踪的数据 */
typedef struct _AiFaceFeatureseInputData_S_
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
} AiFaceFeatureseInputData_S;

/* 坐标数据结构 */
typedef struct _AiFaceFeaturesePos_S_
{
    float fPos[4];   /*人脸坐标 */
    float fFeatures[128];   /* 人脸128特征点 */
    int nResult;            /*人脸识别匹配结果*/
    char achName[256];         /*人脸*/

} AiFaceFeaturesePos_S;

/* 获取人脸识别跟踪的数据结构 */
typedef struct _AiFaceFeatureseOutputData_S_
{
    /* 分析的图片数据地址 */
    unsigned char *pImageData;
    /* 分析的图片数据大小 */
    int nDataLen;
    /* 人数的坐标数组指针，长度为识别人数 */
    AiFaceFeaturesePos_S *pstPos;
    /* 分析时间（单位：毫秒） */
    int nAnalyseTime;
    /* 用户自定义参数，从输入数据中传入 */
    void *pUsrParam;
} AiFaceFeatureseOutputData_S;

typedef struct _AiFaceFeatureseNeedParam_S_
{
    /* 图片宽度 */
    int nWidth;
    /* 图片高度 */
    int nHeight;
    /* 使用的人脸识别跟踪模型绝对路径 */
    char *pModelPath;
    /* 分析间隔 */
    int nInterval;
} AiFaceFeatureseNeedParam_S;

typedef struct _AiFaceFeatureseExParam_S_
{
    /* 分析图片的队列大小，默认为30 */
    int nQueSize;
} AiFaceFeatureseExParam_S;

typedef struct ai_face_recognition AiFaceFeaturese_S;
struct ai_face_recognition
{
    /* 初始化 */
    int (*init)(AiFaceFeaturese_S *pHandle);
    /* 反初始化 */
    int (*uninit)(AiFaceFeaturese_S *pHandle);
    /* 发送需要分析的图片 */
    int (*send_image)(AiFaceFeaturese_S *pHandle, AiFaceFeatureseInputData_S *pstData, int nTimeout);
    /* 获取已经分析完毕的数据 */
    int (*get_data)(AiFaceFeaturese_S *pHandle, AiFaceFeatureseOutputData_S *pstData, int nTimeout);
    /* 清空队列数据 */
    int (*clear_data)(AiFaceFeaturese_S *pHandle);
    /* 释放已经分析完毕的数据 */
    int (*release_data)(AiFaceFeatureseOutputData_S *pstData);
    /* 必须参数 */
    AiFaceFeatureseNeedParam_S stNeedParam;
    /* 额外参数 */
    AiFaceFeatureseExParam_S stExParam;

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
 * @brief  分配人脸识别跟踪句柄
 * @param  [AiFaceFeatureseNeedParam_S] stNeedParam
 * @return [*]
 * @author chenchl
 * @note
 */
AiFaceFeaturese_S *ai_face_recognition_alloc(AiFaceFeatureseNeedParam_S stNeedParam);

/**
 * @brief  释放人脸识别跟踪句柄
 * @param  [AiFaceFeaturese_S] *pHandle
 * @return [*]
 * @author chenchl
 * @note
 */
int ai_face_recognition_release(AiFaceFeaturese_S *pHandle);

/*外部调用人脸特征库初始化接口*/
int externDBInit();

/*外部调用人脸特征库保存设置接口*/
int cmd_SetDb();

#endif /* _AI_SLIDEWINDOW_TRACK_H_ */