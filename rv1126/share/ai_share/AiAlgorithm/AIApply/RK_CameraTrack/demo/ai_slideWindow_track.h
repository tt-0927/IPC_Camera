/*
 * @Author       : chenchl
 * @Date         : 2023-10-27 09:26:01
 * @LastEditors  : chenchl
 * @LastEditTime : 2023-10-28 09:12:26
 * @FilePath     : ai_slideWindow_track.h
 * @Description  : 滑动窗口跟踪分析
 */
#ifndef _AI_SLIDEWINDOW_TRACK_H_
#define _AI_SLIDEWINDOW_TRACK_H_
#include "os_que.h"
#include "os_thr.h"
#include "os_mutex.h"


/* 设置需要进行滑动窗口跟踪的数据 */
typedef struct _AiSlideTackseInputData_S_
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
} AiSlideTackseInputData_S;

/* 坐标数据结构 */
typedef struct _AiSlideTacksePos_S_
{
    float fX;   /* X坐标 */
    float fY;   /* Y坐标 */
    float fW;   /* W长度 */
    float fH;   /* H高度 */
} AiSlideTacksePos_S;

/* 获取滑动窗口跟踪的数据结构 */
typedef struct _AiSlideTackseOutputData_S_
{
    /* 分析的图片数据地址 */
    unsigned char *pImageData;
    /* 分析的图片数据大小 */
    int nDataLen;
    /* 人数的坐标数组指针，长度为识别人数 */
    AiSlideTacksePos_S *pstPos;
    /* 分析时间（单位：毫秒） */
    int nAnalyseTime;
    /* 用户自定义参数，从输入数据中传入 */
    void *pUsrParam;
} AiSlideTackseOutputData_S;

typedef struct _AiSlideTackseNeedParam_S_
{
    /* 图片宽度 */
    int nWidth;
    /* 图片高度 */
    int nHeight;
    /* 使用的滑动窗口跟踪模型绝对路径 */
    char *pModelPath;
    /* 分析间隔 */
    int nInterval;
} AiSlideTackseNeedParam_S;

typedef struct _AiSlideTackseExParam_S_
{
    /* 分析图片的队列大小，默认为30 */
    int nQueSize;
} AiSlideTackseExParam_S;

typedef struct ai_slideWindow_track AiSlideTackse_S;
struct ai_slideWindow_track
{
    /* 初始化 */
    int (*init)(AiSlideTackse_S *pHandle);
    /* 反初始化 */
    int (*uninit)(AiSlideTackse_S *pHandle);
    /* 发送需要分析的图片 */
    int (*send_image)(AiSlideTackse_S *pHandle, AiSlideTackseInputData_S *pstData, int nTimeout);
    /* 获取已经分析完毕的数据 */
    int (*get_data)(AiSlideTackse_S *pHandle, AiSlideTackseOutputData_S *pstData, int nTimeout);
    /* 清空队列数据 */
    int (*clear_data)(AiSlideTackse_S *pHandle);
    /* 释放已经分析完毕的数据 */
    int (*release_data)(AiSlideTackseOutputData_S *pstData);
    /* 必须参数 */
    AiSlideTackseNeedParam_S stNeedParam;
    /* 额外参数 */
    AiSlideTackseExParam_S stExParam;

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
 * @brief  分配滑动窗口跟踪句柄
 * @param  [AiSlideTackseNeedParam_S] stNeedParam
 * @return [*]
 * @author chenchl
 * @note
 */
AiSlideTackse_S *ai_slideWindow_track_alloc(AiSlideTackseNeedParam_S stNeedParam);

/**
 * @brief  释放滑动窗口跟踪句柄
 * @param  [AiSlideTackse_S] *pHandle
 * @return [*]
 * @author chenchl
 * @note
 */
int ai_slideWindow_track_release(AiSlideTackse_S *pHandle);

#endif /* _AI_SLIDEWINDOW_TRACK_H_ */