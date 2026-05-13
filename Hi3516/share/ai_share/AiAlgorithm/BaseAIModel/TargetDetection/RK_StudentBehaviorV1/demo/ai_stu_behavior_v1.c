/*
 * @Author       : EasonLu
 * @Date         : 2023-10-11 11:29:15
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 13:51:03
 * @FilePath     : ai_stu_behavior_v1.c
 * @Description  : AI学生行为分析功能统筹类
 */
#include "ai_stu_behavior_v1.h"
#include "ai_stu_behavior_interface_v1.h"
#include "share_os.h"


/**
 * @brief  释放输入数据结构体
 * @param  [AiStuBehaviorInputData_S] *pInputData 输入数据结构体指针
 * @return [*]
 * @author EasonLu
 * @note
 */
int ai_stu_behavior_release_inputData(AiStuBehaviorInputData_S *pInputData)
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
 * @brief  学生行为分析线程
 * @param  [void] *pInparam - 输入参数
 * @return [*]
 * @author EasonLu
 * @note
 */
void *ai_stu_behavior_thr_analyse(void *pInparam)
{
    AiStuBehavior_S *pHandle = (AiStuBehavior_S *)pInparam;
    if (pHandle == NULL)
    {
        printf("ai_stu_behavior_thr_analyse pInparam is NULL\n");
        return NULL;
    }
    printf("学生行为分析线程启动\n");
    long long ldStartTime, ldEndTime;
    AiStuBehaviorInputData_S *pInputData = NULL;
    Int64 nAddr = 0;
    while (!pHandle->bThrExit)
    {
        int nStatus = OS_queGet(&(pHandle->stInputQue), &nAddr, 50);
        if (0 == nStatus)
        {
            pInputData = (AiStuBehaviorInputData_S *)nAddr;
            if (pInputData != NULL)
            {
                /* 申请输出数据结构体 */
                AiStuBehaviorOutputData_S *pOutputData = (AiStuBehaviorOutputData_S *)malloc(sizeof(AiStuBehaviorOutputData_S));
                if (pOutputData == NULL)
                {
                    printf("ai_stu_behavior_thr_analyse pOutputData is NULL\n");
                    continue;
                }

                memset(pOutputData, 0, sizeof(AiStuBehaviorOutputData_S));
                /* 同步更新传入数据的用户自定义参数 */
                pOutputData->pUsrParam = pInputData->pUsrParam;
                /* 记录开始分析时间点 */
                ldStartTime = get_run_time();
                stu_behavior_send_image(pInputData->pImageData);
                /* 分析完毕，释放输入分析数据 */
                OS_mutexLock(&(pHandle->stMutexInput));
                ai_stu_behavior_release_inputData(pInputData);
                OS_mutexUnlock(&(pHandle->stMutexInput));
                /* TODO:获取其他分析数据 */
                pOutputData->pstPos = NULL;
                int nRet = stu_behavior_get_pos(&pOutputData->pstPos,
                                                &pOutputData->nTotal);

                /* 记录结束分析时间点 */
                ldEndTime = get_run_time();
                /* 计算并记录分析时间 */
                pOutputData->nAnalyseTime = ldEndTime - ldStartTime;
                /* 将分析完毕的图片存放到输出队列 */
                nStatus = OS_quePut(&(pHandle->stOutputQue), (Int64)pOutputData, 50);
                if (nStatus != 0)
                {
                    /* 无法送输出队列，则释放当前已完成分析的数据 */
                    if (pOutputData->pstPos)
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
    printf("学生行为分析线程退出\n");
    return NULL;
}

/**
 * @brief  初始化AI学生行为分析句柄
 * @param  [AiStuBehavior_S] *pHandle - AI学生行为分析句柄
 * @return [*]
 * @author EasonLu
 * @note
 */
static int ai_stu_behavior_init(AiStuBehavior_S *pHandle)
{
    if (pHandle->stNeedParam.pModelPath == NULL)
    {
        printf("pModlePath is NULL");
        return -2;
    }
    stu_behavior_init(pHandle->stNeedParam.pModelPath);

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
                 ai_stu_behavior_thr_analyse,
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
 * @brief  反初始化AI学生行为分析句柄
 * @param  [AiStuBehavior_S] *pHandle - AI学生行为分析句柄
 * @return [*]
 * @author EasonLu
 * @note
 */
static int ai_stu_behavior_uninit(AiStuBehavior_S *pHandle)
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

        stu_behavior_uninit();
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}

/**
 * @brief  送图像数据进行分析
 * @param  [AiStuBehavior_S] *pHandle - AI学生行为分析句柄
 * @param  [AiStuBehaviorInputData_S] *pstData - 输入数据结构体
 * @param  [int] nTimeout - 超时时间
 * @return [*]
 * @note
 */
static int ai_stu_behavior_send_image(
    AiStuBehavior_S *pHandle,
    AiStuBehaviorInputData_S *pstData,
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

    AiStuBehaviorInputData_S *pQueData = (AiStuBehaviorInputData_S *)malloc(sizeof(AiStuBehaviorInputData_S));
    if (pQueData == NULL)
    {
        printf("ai_stu_behavior_send_image pQueData is NULL\n");
        return -3;
    }

    memset(pQueData, 0, sizeof(AiStuBehaviorInputData_S));
    /* 拷贝一份结构体数据，用于保存在队列中 */
    memcpy(pQueData, pstData, sizeof(AiStuBehaviorInputData_S));

    /* 直通/拷贝的模式，根据是否传入释放函数进行判定 */
    char *pAnalyseData = NULL;
    if (pQueData->fnFree != NULL)
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
    pQueData->pImageData = pAnalyseData;

    /* 送分析队列 */
    int nRet = OS_quePut(&(pHandle->stInputQue), (Int64)pQueData, nTimeout);
    if (nRet != 0)
    {
        /* 送不进去队列，拷贝模式则需要释放申请的内存数据 */
        if (pQueData->fnFree == NULL)
        {
            free(pAnalyseData);
            pAnalyseData = NULL;
        }
        free(pQueData);
        pQueData = NULL;
        return nRet;
    }
    return 0;
}

/**
 * @brief  获取分析结果
 * @param  [AiStuBehavior_S] *pHandle - AI学生行为分析句柄
 * @param  [AiStuBehaviorOutputData_S] *pstData - 输出数据结构体
 * @param  [int] nTimeout - 超时时间
 * @return [*]
 * @author EasonLu
 * @note   
 */
static int ai_stu_behavior_get_data(AiStuBehavior_S *pHandle,AiStuBehaviorOutputData_S *pstData,int nTimeout)
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
        AiStuBehaviorOutputData_S *pQueData = (AiStuBehaviorOutputData_S *)nQueData;
        if (pQueData)
        {
            /* 将对应的数据拷贝至输出的指针 */
            memcpy(pstData, pQueData, sizeof(AiStuBehaviorOutputData_S));
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
 * @brief  释放输出数据结构体
 * @param  [AiStuBehaviorOutputData_S] *pstData - 输出数据结构体
 * @return [*]
 * @author EasonLu
 * @note   
 */
static int ai_stu_behavior_release_data(AiStuBehaviorOutputData_S *pstData)
{
    if (pstData == NULL)
    {
        return -1;
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
 * @param  [AiStuBehavior_S] *pHandle
 * @return [*]
 * @author EasonLu
 * @note   
 */
static int ai_stu_behavior_clear_data(AiStuBehavior_S *pHandle)
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
        AiStuBehaviorInputData_S *pQueData = (AiStuBehaviorInputData_S *)nQueData;
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
        AiStuBehaviorOutputData_S *pQueData = (AiStuBehaviorOutputData_S *)nQueData;
        if (pQueData)
        {
            /* 释放数据 */
            ai_stu_behavior_release_data(pQueData);
            nCount++;
        }
        OS_mutexUnlock(&(pHandle->stMutexOutput));
    }
    printf("清空输出队列数据：%d\n", nCount);

    return 0;
}

AiStuBehavior_S *ai_stu_behavior_alloc(AiStuBehaviorNeedParam_S stNeedParam)
{
    AiStuBehavior_S *pHandle = (AiStuBehavior_S *)malloc(sizeof(AiStuBehavior_S));
    if (pHandle == NULL)
    {
        printf("ai_stu_behavior_alloc pHandle is NULL\n");
        return NULL;
    }
    memset(pHandle, 0, sizeof(AiStuBehavior_S));
    memcpy(&pHandle->stNeedParam, &stNeedParam, sizeof(AiStuBehaviorNeedParam_S));

    /* 设置默认的额外参数 */
    pHandle->stExParam.nQueSize = 30;

    pHandle->init = ai_stu_behavior_init;
    pHandle->uninit = ai_stu_behavior_uninit;
    pHandle->send_image = ai_stu_behavior_send_image;
    pHandle->get_data = ai_stu_behavior_get_data;
    pHandle->release_data = ai_stu_behavior_release_data;
    pHandle->clear_data = ai_stu_behavior_clear_data;
    return pHandle;
}

int ai_stu_behavior_release(AiStuBehavior_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}
