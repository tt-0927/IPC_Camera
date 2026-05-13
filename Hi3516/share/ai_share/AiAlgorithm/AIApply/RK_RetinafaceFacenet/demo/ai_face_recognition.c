/*
 * @Author       : chenchl
 * @Date         : 2023-11-15 09:26:13
 * @LastEditors  : 
 * @LastEditTime : 2023-11-15 09:59:19
 * @FilePath     : ai_face_recognition.c
 * @Description  : 人脸识别跟踪分析
 */
#include "ai_face_recognition.h"
#include "ai_face_recognition_interface.h"
#include <stdio.h>
#include <time.h>
#include "share_os.h"


/**
 * @brief  释放输入数据结构体
 * @param  [AiFaceFeatureseInputData_S] *pInputData 输入数据结构体指针
 * @return [*]
 * @author chenchl
 * @note
 */
int ai_face_recognition_release_inputData(AiFaceFeatureseInputData_S *pInputData)
{
    if (pInputData == NULL)
    {
        return -1;
    }

    if (pInputData->fnFree == NULL)
    {
        /* 拷贝模式需要内部释放申请的内存 */
        free(pInputData->pImageData);
        pInputData->pImageData = NULL;
    }
    else
    {
        /* 直通模式调用释放函数进行释放 */
        pInputData->fnFree(NULL, pInputData->pFreeParam);
    }

    free(pInputData);
    pInputData = NULL;
    return 0;
}

/**
 * @brief  人脸识别分析线程
 * @param  [void] *Inparam 输入参数
 * @return [*]
 * @author chenchl
 * @note
 */
void *ai_face_recognition_thr_analyse(void *Inparam)
{

    AiFaceFeaturese_S *pHandle = (AiFaceFeaturese_S *)Inparam;
    if (pHandle == NULL)
    {
        return NULL;
    }
    printf("人脸识别分析线程启动\n");
    long long ldStartTime, ldEndTime;
    AiFaceFeatureseInputData_S *pInputData = NULL;
    Int64 nAddr = 0;

    AISaveFeature();

    GetAllFeature();
    while (!pHandle->bThrExit)
    {
        int nStatus = OS_queGet(&(pHandle->stInputQue), &nAddr, 50);

        if (0 == nStatus)
        {
            pInputData = (AiFaceFeatureseInputData_S *)nAddr;

            if (pInputData != NULL)
            {
                /* 申请输出数据结构体 */
                AiFaceFeatureseOutputData_S *pOutputData = (AiFaceFeatureseOutputData_S *)malloc(sizeof(AiFaceFeatureseOutputData_S));
                if (pOutputData == NULL)
                {
                    /* NOTE:无法输出数据，则释放当前已完成分析的数据 */
                    printf("申请输出数据结构体失败\n");
                    continue;
                }

                memset(pOutputData, 0, sizeof(AiFaceFeatureseOutputData_S));
                /* 同步更新传入数据的用户自定义参数 */
                pOutputData->pUsrParam = pInputData->pUsrParam;
 

                /* 记录开始分析时间点 */
                ldStartTime = get_run_time();
                /* 送分析，将分析结果直接更新到分配的新地址 */
                face_recognition_send_image(pInputData->pImageData);

                /* NOTE:分析完毕 */
                OS_mutexLock(&(pHandle->stMutexInput));
                /* 释放输入数据 */
                ai_face_recognition_release_inputData(pInputData);
                OS_mutexUnlock(&(pHandle->stMutexInput));

                /* TODO:获取其他分析数据 */
                pOutputData->pstPos = NULL;
                /* 获取坐标数据 */
                face_recognition_get_pos(&(pOutputData->pstPos),
                                     &(pOutputData->nDataLen));

                /* 记录分析完毕时间点 */
                ldEndTime = get_run_time();
                /* 计算并记录分析时间 */
                pOutputData->nAnalyseTime = ldEndTime - ldStartTime;

                /* 将分析完毕的图片存放到输出队列 */
                nStatus = OS_quePut(&(pHandle->stOutputQue), (Int64)pOutputData, 50);
                if (nStatus != 0)
                {
                    /* 无法送输出队列，则释放当前已完成分析的数据 */
    
                    if(pOutputData->pstPos)
                    {
                        free(pOutputData->pstPos);
                        pOutputData->pstPos = NULL;
                    }
                    free(pOutputData);
                    pOutputData = NULL;
                    printf("送输出队列失败\n");
                }
            }
        }
    }
    printf("人脸识别分析线程退出\n");
    return NULL;
}


/**
 * @brief  初始化人脸识别分析的句柄
 * @param  [AiFaceFeaturese_S] *pHandle - 人脸识别分析的句柄
 * @return [*]
 * @author chenchl
 * @note
 */
static int ai_face_recognition_init(AiFaceFeaturese_S *pHandle)
{
    /* 初始化人脸识别分析模型 */

    face_recognition_init();

    /* 创建分析图片队列 */
    int nRet = OS_queCreate(&(pHandle->stInputQue),
                            pHandle->stExParam.nQueSize);
    if (nRet < 0)
    {
        printf("创建分析图片队列失败\n");
        return -3;
    }

    /* 创建分析图片线程 */
    OS_thrCreate(&pHandle->stAnalyseThr,
                 ai_face_recognition_thr_analyse,
                 OS_JOINABLE,
                 OS_THR_STACK_SIZE_DEFAULT,
                 pHandle);

    /* 创建上抛数据队列 */
    nRet = OS_queCreate(&(pHandle->stOutputQue),
                        pHandle->stExParam.nQueSize);
    if (nRet < 0)
    {
        printf("创建上抛数据队列失败\n");
        return -4;
    }


    /* 创建互斥锁 */
    OS_mutexCreate(&(pHandle->stMutexInput));
    OS_mutexCreate(&(pHandle->stMutexOutput));

    return 0;
}

/**
 * @brief  反初始化人脸识别分析的句柄
 * @param  [AiFaceFeaturese_S] *pHandle
 * @return [*]
 * @author chenchl
 * @note
 */
static int ai_face_recognition_uninit(AiFaceFeaturese_S *pHandle)
{
    if (pHandle)
    {
        /* 退出异步线程 */
        pHandle->bThrExit = 1;
        OS_thrJoin(&(pHandle->stAnalyseThr));

        /* 清空队列数据 */
        pHandle->clear_data(pHandle);

        /* 删除队列 */
        if (pHandle->stInputQue.queue != NULL)
        {
            OS_queDelete(&(pHandle->stInputQue));
        }
        if (pHandle->stOutputQue.queue != NULL)
        {
            OS_queDelete(&(pHandle->stOutputQue));
        }

        /* 删除互斥锁 */
        OS_mutexDelete(&(pHandle->stMutexInput));
        OS_mutexDelete(&(pHandle->stMutexOutput));

        face_recognition_uninit();
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}

/**
 * @brief  送分析
 * @param  [AiFaceFeaturese_S] *pHandle - 人脸识别分析的句柄
 * @param  [AiFaceFeatureseInputData_S] *pTrackData - 送分析的数据
 * @param  [int] nTimeout - 超时时间
 * @return [*]
 * @author chenchl
 * @note
 */
static int ai_face_recognition_send_image(
    AiFaceFeaturese_S *pHandle,
    AiFaceFeatureseInputData_S *pTrackData,
    int nTimeout)
{
    if (pHandle == NULL)
    {
        return -1;
    }

    if (pTrackData == NULL)
    {
        return -2;
    }

    if (pTrackData->nDataLen != pHandle->nImageSize)
    {
        printf("分析图片大小与模型不匹配\n");
        return -3;
    }

    AiFaceFeatureseInputData_S *pQueDate = (AiFaceFeatureseInputData_S *)malloc(sizeof(AiFaceFeatureseInputData_S));
    if (pQueDate == NULL)
    {
        return -4;
    }
    memset(pQueDate, 0, sizeof(AiFaceFeatureseInputData_S));
    /* 拷贝一份结构体数据，用于保存在队列中 */
    memcpy(pQueDate, pTrackData, sizeof(AiFaceFeatureseInputData_S));

    /* 直通/拷贝的模式，根据是否传入释放函数进行判定 */
    char *pAnalyseData = NULL;
    if (pQueDate->fnFree != NULL)
    {
        /* 直通模式 */
        pAnalyseData = (char *)pTrackData->pImageData;
    }
    else
    {
        /* 拷贝模式 */
        pAnalyseData = (char *)malloc(pTrackData->nDataLen);
        memcpy(pAnalyseData, pTrackData->pImageData, pTrackData->nDataLen);
        pTrackData->fnFree(pTrackData->pImageData, pTrackData->pFreeParam);
    }

    /* 更新分析数据地址 */
    pQueDate->pImageData = pAnalyseData;

    /* 送分析队列 */
    int nRet = OS_quePut(&(pHandle->stInputQue), (Int64)pQueDate, nTimeout);
    if (nRet != 0)
    {
        /* 送不进去队列，拷贝模式则需要释放申请的内存数据 */
        if (pQueDate->fnFree == NULL)
        {
            free(pAnalyseData);
            pAnalyseData = NULL;
        }
        free(pQueDate);
        pQueDate = NULL;
        return nRet;
    }
    return 0;
}

/**
 * @brief  获取分析数据
 * @param  [AiFaceFeaturese_S] *pHandle - 人脸识别分析的句柄
 * @param  [CvFaceAnlyseData_S] *pTrackData - 分析数据结构体
 * @param  [int] nTimeout - 超时时间
 * @return [*]
 * @author chenchl
 * @note
 */
static int ai_face_recognition_get_data(
    AiFaceFeaturese_S *pHandle,
    AiFaceFeatureseOutputData_S *pTrackData,
    int nTimeout)
{
    if (pHandle == NULL)
    {
        return -1;
    }

    if (pTrackData == NULL)
    {
        return -2;
    }

    /* 从队列中获取数据 */
    Int64 nQueData = 0;
    int nRet = 0;
    nRet = OS_queGet(&(pHandle->stOutputQue), &nQueData, nTimeout);

    OS_mutexLock(&(pHandle->stMutexOutput));
    if (nRet == 0)
    {
        AiFaceFeatureseOutputData_S *pQueData = (AiFaceFeatureseOutputData_S *)nQueData;
        if (pQueData)
        {
            /* 将对应的数据拷贝至输出的指针 */
            memcpy(pTrackData, pQueData, sizeof(AiFaceFeatureseOutputData_S));
            free(pQueData);
            pQueData = NULL;
        }
        else
        {
            nRet = -3;
        }
    }

    OS_mutexUnlock(&(pHandle->stMutexOutput));
    return nRet;
}

/**
 * @brief  释放分析数据
 * @param  [AiFaceFeatureseOutputData_S] *pTrackData - 分析数据结构体
 * @return [*]
 * @author chenchl
 * @note
 */
static int ai_face_recognition_release_data(AiFaceFeatureseOutputData_S *pTrackData)
{
    if (pTrackData == NULL)
    {
        return -1;
    }
    if (pTrackData->pImageData != NULL)
    {
        free(pTrackData->pImageData);
        pTrackData->pImageData = NULL;
    }
    if (pTrackData->pstPos != NULL)
    {
        free(pTrackData->pstPos);
        pTrackData->pstPos = NULL;
    }
    free(pTrackData);
    pTrackData = NULL;
    return 0;
}

/**
 * @brief  清空分析数据
 * @param  [AiFaceFeaturese_S] *pHandle - 人脸识别分析的句柄
 * @return [*]
 * @author chenchl
 * @note
 */
static int ai_face_recognition_clear_data(AiFaceFeaturese_S *pHandle)
{
    if (pHandle == NULL)
    {
        return -1;
    }
    int nCount = 0;
    /* 清空输入队列 */
    Int64 nQueData = 0;
    while (OS_queGet(&(pHandle->stInputQue), &nQueData, 0) == 0)
    {
        OS_mutexLock(&(pHandle->stMutexInput));
        AiFaceFeatureseInputData_S *pQueData = (AiFaceFeatureseInputData_S *)nQueData;
        if (pQueData)
        {
            /* 释放数据 */
            if (pQueData->fnFree == NULL)
            {
                free(pQueData->pImageData);
                pQueData->pImageData = NULL;
            }
            else
            {
                pQueData->fnFree(pQueData->pImageData, pQueData->pFreeParam);
            }
            free(pQueData);
            pQueData = NULL;
            nCount++;
        }
        OS_mutexUnlock(&(pHandle->stMutexInput));
    }
    printf("清空输入队列数据：%d\n", nCount);
    /* 清空输出队列 */
    nQueData = 0;
    nCount = 0;
    while (OS_queGet(&(pHandle->stOutputQue), &nQueData, 0) == 0)
    {
        OS_mutexLock(&(pHandle->stMutexOutput));
        AiFaceFeatureseOutputData_S *pQueData = (AiFaceFeatureseOutputData_S *)nQueData;
        if (pQueData)
        {
            /* 释放数据 */
            if (pQueData->pImageData != NULL)
            {
                free(pQueData->pImageData);
                pQueData->pImageData = NULL;
            }
            free(pQueData);
            pQueData = NULL;
            nCount++;
        }
        OS_mutexUnlock(&(pHandle->stMutexOutput));
    }
    printf("清空输出队列数据：%d\n", nCount);

    return 0;
}

AiFaceFeaturese_S *ai_face_recognition_alloc(AiFaceFeatureseNeedParam_S stNeedParam)
{
    AiFaceFeaturese_S *pHandle = (AiFaceFeaturese_S *)malloc(sizeof(AiFaceFeaturese_S));
    if (pHandle == NULL)
    {
        printf("ai_face_recognition_alloc pHandle is NULL\n");
        return NULL;
    }

    memset(pHandle, 0, sizeof(AiFaceFeaturese_S));
    memcpy(&pHandle->stNeedParam, &stNeedParam, sizeof(AiFaceFeatureseNeedParam_S));

    /* 设置默认的额外参数 */
    pHandle->stExParam.nQueSize = 30;

    pHandle->init = ai_face_recognition_init;
    pHandle->uninit = ai_face_recognition_uninit;
    pHandle->send_image = ai_face_recognition_send_image;
    pHandle->get_data = ai_face_recognition_get_data;
    pHandle->release_data = ai_face_recognition_release_data;
    pHandle->clear_data = ai_face_recognition_clear_data;
    return pHandle;
}

/*外部调用人脸特征库初始化接口*/
int externDBInit()
{
    shareDBInit();
    return 0;
}

/*外部调用人脸特征库保存设置接口*/
int cmd_SetDb()
{
    AISaveFeature();
    return 0;

}

int ai_face_recognition_release(AiFaceFeaturese_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}
