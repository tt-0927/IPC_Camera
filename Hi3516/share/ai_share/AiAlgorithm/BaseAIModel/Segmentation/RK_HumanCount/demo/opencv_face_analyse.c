/*
 * @Author       : EasonLu
 * @Date         : 2023-07-27 09:26:13
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-10-11 16:35:19
 * @FilePath     : opencv_face_analyse.c
 * @Description  : 人脸分析
 */
#include "opencv_face_analyse.h"
#include "opencv_face_analyse_interface.h"
#include <stdio.h>
#include "share_os.h"

/**
 * @brief  释放输入数据结构体
 * @param  [CvFaceAnlyseInputData_S] *pInputData 输入数据结构体指针
 * @return [*]
 * @author EasonLu
 * @note
 */
int opencv_face_analyse_release_inputData(CvFaceAnlyseInputData_S *pInputData)
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
 * @brief  人脸分析线程
 * @param  [void] *Inparam 输入参数
 * @return [*]
 * @author EasonLu
 * @note
 */
void *opencv_face_analyse_thr_analyse(void *Inparam)
{
    CvFaceAnalyse_S *pHandle = (CvFaceAnalyse_S *)Inparam;
    if (pHandle == NULL)
    {
        return NULL;
    }
    long long ldStartTime, ldEndTime;
    CvFaceAnlyseInputData_S *pInputData = NULL;
    Int64 nAddr = 0;
    while (!pHandle->bThrExit)
    {
        int nStatus = OS_queGet(&(pHandle->stInputQue), &nAddr, 50);
        if (0 == nStatus)
        {
            pInputData = (CvFaceAnlyseInputData_S *)nAddr;
            if (pInputData != NULL)
            {
                /* 申请输出数据结构体 */
                CvFaceAnlyseOutputData_S *pOutputData = (CvFaceAnlyseOutputData_S *)malloc(sizeof(CvFaceAnlyseOutputData_S));
                if (pOutputData == NULL)
                {
                    /* NOTE:无法输出数据，则释放当前已完成分析的数据 */
                    printf("申请输出数据结构体失败\n");
                    opencv_face_analyse_release_inputData(pInputData);
                    continue;
                }

                memset(pOutputData, 0, sizeof(CvFaceAnlyseOutputData_S));
                /* 同步更新传入数据的用户自定义参数 */
                pOutputData->pUsrParam = pInputData->pUsrParam;
                if (pInputData->bAnalyseOnly)
                {
                    pOutputData->pImageData = NULL;
                    pOutputData->nDataLen = 0;
                }
                else
                {
                    /* 申请接收分析数据的地址 */
                    pOutputData->pImageData = (unsigned char *)malloc(pInputData->nDataLen);
                    memset(pOutputData->pImageData, 0, pInputData->nDataLen);
                    /* 设置输出数据结构体数据 */
                    pOutputData->nDataLen = pInputData->nDataLen;
                }

                /* 记录开始分析时间点 */
                ldStartTime = get_run_time();
                /* 送分析，将分析结果直接更新到分配的新地址 */
                face_analyse_bgr(pInputData->pImageData,
                                 pInputData->nDataLen,
                                 pOutputData->pImageData);


                /* NOTE:分析完毕 */
                OS_mutexLock(&(pHandle->stMutexInput));
                /* 释放输入数据 */
                opencv_face_analyse_release_inputData(pInputData);
                OS_mutexUnlock(&(pHandle->stMutexInput));

                /* TODO:获取其他分析数据 */
                pOutputData->pstPos = NULL;
                /* 获取坐标数据 */
                face_analyse_get_pos(&(pOutputData->pstPos),
                                     &(pOutputData->nPeopleNum));

                /* 记录分析完毕时间点 */
                ldEndTime = get_run_time();
                /* 计算并记录分析时间 */
                pOutputData->nAnalyseTime = ldEndTime - ldStartTime;

                /* 将分析完毕的图片存放到输出队列 */
                nStatus = OS_quePut(&(pHandle->stOutputQue), (Int64)pOutputData, 50);
                if (nStatus != 0)
                {
                    /* 无法送输出队列，则释放当前已完成分析的数据 */
                    if(pOutputData->pImageData)
                    {
                        free(pOutputData->pImageData);
                        pOutputData->pImageData = NULL;
                    }
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
    printf("人脸分析线程退出\n");
    return NULL;
}

/**
 * @brief  内部计算图片大小
 * @param  [CvFaceAnalyse_S] *pHandle - 人脸分析的句柄
 * @return [*]
 * @author EasonLu
 * @note   增加其他图片格式时需要修改此函数
 */
int opencv_face_analyse_calculate_image_size(CvFaceAnalyse_S *pHandle)
{
    if (pHandle == NULL)
    {
        return -1;
    }

    switch (pHandle->stNeedParam.enImageFormat)
    {
    case CV_FACE_ANALYSE_IMAGE_FORMAT_RGB888:
        pHandle->nImageSize = pHandle->stNeedParam.nWidth * pHandle->stNeedParam.nHeight * 3;
        break;

    default:
        /* 其他未知格式则为0，拷贝不会造成程序崩溃 */
        pHandle->nImageSize = 0;
        break;
    }

    return 0;
}

/**
 * @brief  初始化人脸分析的句柄
 * @param  [CvFaceAnalyse_S] *pHandle - 人脸分析的句柄
 * @return [*]
 * @author EasonLu
 * @note
 */
static int opencv_face_analyse_init(CvFaceAnalyse_S *pHandle)
{
    /* 初始化人脸分析模型 */
    if (pHandle->stNeedParam.pModlePath)
    {
        face_analyse_init(pHandle->stNeedParam.pModlePath);
    }
    else
    {
        return -2;
    }

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
                 opencv_face_analyse_thr_analyse,
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

    /* 内部计算需要分析的图片大小 */
    nRet = opencv_face_analyse_calculate_image_size(pHandle);
    if (nRet != 0)
    {
        printf("计算图片大小失败\n");
        return -5;
    }

    /* 创建互斥锁 */
    OS_mutexCreate(&(pHandle->stMutexInput));
    OS_mutexCreate(&(pHandle->stMutexOutput));

    return 0;
}

/**
 * @brief  反初始化人脸分析的句柄
 * @param  [CvFaceAnalyse_S] *pHandle
 * @return [*]
 * @author EasonLu
 * @note
 */
static int opencv_face_analyse_uninit(CvFaceAnalyse_S *pHandle)
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

        face_analyse_uninit();
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}

/**
 * @brief  送分析
 * @param  [CvFaceAnalyse_S] *pHandle - 人脸分析的句柄
 * @param  [CvFaceAnlyseInputData_S] *pstData - 送分析的数据
 * @param  [int] nTimeout - 超时时间
 * @return [*]
 * @author EasonLu
 * @note
 */
static int opencv_face_analyse_send_image(
    CvFaceAnalyse_S *pHandle,
    CvFaceAnlyseInputData_S *pstData,
    int nTimeout)
{
    if (pHandle == NULL)
    {
        return -1;
    }

    if (pstData == NULL)
    {
        return -2;
    }

    if (pstData->nDataLen != pHandle->nImageSize)
    {
        printf("分析图片大小与模型不匹配1111\n");
        return -3;
    }

    CvFaceAnlyseInputData_S *pQueDate = (CvFaceAnlyseInputData_S *)malloc(sizeof(CvFaceAnlyseInputData_S));
    if (pQueDate == NULL)
    {
        return -4;
    }
    memset(pQueDate, 0, sizeof(CvFaceAnlyseInputData_S));
    /* 拷贝一份结构体数据，用于保存在队列中 */
    memcpy(pQueDate, pstData, sizeof(CvFaceAnlyseInputData_S));

    /* 直通/拷贝的模式，根据是否传入释放函数进行判定 */
    char *pAnalyseData = NULL;
    if (pQueDate->fnFree != NULL)
    {
        /* 直通模式 */
        pAnalyseData = (char *)pstData->pImageData;
    }
    else
    {
        /* 拷贝模式 */
        pAnalyseData = (char *)malloc(pstData->nDataLen);
        memcpy(pAnalyseData, pstData->pImageData, pstData->nDataLen);
        pstData->fnFree(pstData->pImageData, pstData->pFreeParam);
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
 * @param  [CvFaceAnalyse_S] *pHandle - 人脸分析的句柄
 * @param  [CvFaceAnlyseData_S] *pstData - 分析数据结构体
 * @param  [int] nTimeout - 超时时间
 * @return [*]
 * @author EasonLu
 * @note
 */
static int opencv_face_analyse_get_data(
    CvFaceAnalyse_S *pHandle,
    CvFaceAnlyseOutputData_S *pstData,
    int nTimeout)
{
    if (pHandle == NULL)
    {
        return -1;
    }

    if (pstData == NULL)
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
        CvFaceAnlyseOutputData_S *pQueData = (CvFaceAnlyseOutputData_S *)nQueData;
        if (pQueData)
        {
            /* 将对应的数据拷贝至输出的指针 */
            memcpy(pstData, pQueData, sizeof(CvFaceAnlyseOutputData_S));
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
 * @param  [CvFaceAnlyseOutputData_S] *pstData - 分析数据结构体
 * @return [*]
 * @author EasonLu
 * @note
 */
static int opencv_face_analyse_release_data(CvFaceAnlyseOutputData_S *pstData)
{
    if (pstData == NULL)
    {
        return -1;
    }
    if (pstData->pImageData != NULL)
    {
        free(pstData->pImageData);
        pstData->pImageData = NULL;
    }
    if (pstData->pstPos != NULL)
    {
        free(pstData->pstPos);
        pstData->pstPos = NULL;
    }
    free(pstData);
    pstData = NULL;
    return 0;
}

/**
 * @brief  清空分析数据
 * @param  [CvFaceAnalyse_S] *pHandle - 人脸分析的句柄
 * @return [*]
 * @author EasonLu
 * @note
 */
static int opencv_face_analyse_clear_data(CvFaceAnalyse_S *pHandle)
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
        CvFaceAnlyseInputData_S *pQueData = (CvFaceAnlyseInputData_S *)nQueData;
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
        CvFaceAnlyseOutputData_S *pQueData = (CvFaceAnlyseOutputData_S *)nQueData;
        if (pQueData)
        {
            /* 释放数据 */
            opencv_face_analyse_release_data(pQueData);
            nCount++;
        }
        OS_mutexUnlock(&(pHandle->stMutexOutput));
    }
    printf("清空输出队列数据：%d\n", nCount);

    return 0;
}

CvFaceAnalyse_S *opencv_face_analyse_alloc(CvFaceAnalyseNeedParam_S stNeedParam)
{
    CvFaceAnalyse_S *pHandle = (CvFaceAnalyse_S *)malloc(sizeof(CvFaceAnalyse_S));
    memset(pHandle, 0, sizeof(CvFaceAnalyse_S));
    memcpy(&pHandle->stNeedParam, &stNeedParam, sizeof(CvFaceAnalyseNeedParam_S));

    /* 设置默认的额外参数 */
    pHandle->stExParam.nQueSize = 30;

    pHandle->init = opencv_face_analyse_init;
    pHandle->uninit = opencv_face_analyse_uninit;
    pHandle->send_image = opencv_face_analyse_send_image;
    pHandle->get_data = opencv_face_analyse_get_data;
    pHandle->release_data = opencv_face_analyse_release_data;
    pHandle->clear_data = opencv_face_analyse_clear_data;
    return pHandle;
}

int opencv_face_analyse_release(CvFaceAnalyse_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}
