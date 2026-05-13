/*
 * @Author       : EasonLu
 * @Date         : 2023-10-11 11:29:06
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-10-11 16:53:16
 * @FilePath     : ai_stu_behavior.h
 * @Description  : AI学生行为分析功能统筹类
 */
#ifndef _AI_STU_BEHAVIOR_H_
#define _AI_STU_BEHAVIOR_H_
#include "os_que.h"
#include "os_thr.h"
#include "os_mutex.h"

/* 设置分析的数据结构 */
typedef struct _AiStuBehaviorInputData_
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
} AiStuBehaviorInputData_S;

/* 每组数据的结构 */
typedef struct _AiStuBehaviorPos_
{
    float fX1;       /* 左上角x坐标 */
    float fY1;       /* 左上角y坐标 */
    float fX2;       /* 右下角x坐标 */
    float fY2;       /* 右下角y坐标 */
    float fIsObject; /* 是否为物体概率 */
    int nClass;      /* 物体类别枚举 */
} AiStuBehaviorPos_S;

/* 获取分析的数据结构 */
typedef struct _AiStuBehaviorOutputData_S_
{
    /* 识别出总的方框数 */
    int nTotal;
    /* 坐标数组指针，长度为识别总数 */
    AiStuBehaviorPos_S *pstPos;
    /* 分析时间（单位：毫秒） */
    int nAnalyseTime;
    /* 用户自定义参数，从输入数据中传入 */
    void *pUsrParam;
} AiStuBehaviorOutputData_S;

typedef struct _AiStuBehaviorNeedParam_S_
{
    /* 图片宽度 */
    int nWidth;
    /* 图片高度 */
    int nHeight;
    /* 使用的人脸分析模型绝对路径 */
    char *pModelPath;
    /* 分析间隔 */
    int nInterval;
} AiStuBehaviorNeedParam_S;

typedef struct _AiStuBehaviorExParam_S_
{
    /* 分析图片的队列大小，默认为30 */
    int nQueSize;
} AiStuBehaviorExParam_S;

typedef struct AI_Stu_Behavior AiStuBehavior_S;
struct AI_Stu_Behavior
{
    /* 初始化 */
    int (*init)(AiStuBehavior_S *pHandle);
    /* 反初始化 */
    int (*uninit)(AiStuBehavior_S *pHandle);
    /* 发送需要分析的图片 */
    int (*send_image)(AiStuBehavior_S *pHandle, AiStuBehaviorInputData_S *pstData, int nTimeout);
    /* 获取已经分析完毕的数据 */
    int (*get_data)(AiStuBehavior_S *pHandle, AiStuBehaviorOutputData_S *pstData, int nTimeout);
    /* 清空队列数据 */
    int (*clear_data)(AiStuBehavior_S *pHandle);
    /* 释放已经分析完毕的数据 */
    int (*release_data)(AiStuBehaviorOutputData_S *pstData);
    /* 必须参数 */
    AiStuBehaviorNeedParam_S stNeedParam;
    /* 额外参数 */
    AiStuBehaviorExParam_S stExParam;

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
 * @param  [AiStuBehaviorNeedParam_S] stNeedParam
 * @return [*]
 * @author EasonLu
 * @note
 */
AiStuBehavior_S *ai_stu_behavior_alloc(AiStuBehaviorNeedParam_S stNeedParam);

/**
 * @brief  释放人脸分析句柄
 * @param  [AiStuBehavior_S] *pHandle
 * @return [*]
 * @author EasonLu
 * @note
 */
int ai_stu_behavior_release(AiStuBehavior_S *pHandle);

#endif /* _AI_STU_BEHAVIOR_H_ */ 
