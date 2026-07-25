/*
 * @FilePath     : TrackerAlgorithmBase.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 18:59:05
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-02-21 16:31:12
 * @Description  : 跟踪模块基类
 */
#include "TrackerAlgorithmBase.hpp"

#include "dlog.h"

using namespace TA_NS;

static void freeMediaData(MediaDataInfo_S stMediaDataInfo)
{
    stMediaDataInfo.free();
}

CTrackerAlgorithmBase::CTrackerAlgorithmBase(TrackerAlgorithmInParam_S stInParam)
    : m_stInParam(stInParam)
{
    if (nullptr == m_pDataQueue)
    {
        m_pDataQueue = new CDataQueue<MediaDataInfo_S, TrackerAlgorithmResult_S>(freeMediaData, nullptr, stInParam.stExParam.nMaxQueue);
    }

    /* 创建线程 */
    m_threadObj = std::thread(&CTrackerAlgorithmBase::run, this);
}

CTrackerAlgorithmBase::~CTrackerAlgorithmBase()
{
    /* 结束线程 */
    m_bRunning.store(false);
    m_threadObj.join();

    if (m_pDataQueue)
    {
        delete m_pDataQueue;
        m_pDataQueue = nullptr;
    }
}

/* 发送分析数据 */
BlError_E CTrackerAlgorithmBase::send_dataAnalysis(MediaDataInfo_S stMediaDataInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    if (nullptr == m_pDataQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    /* 插入队列 */
    enRetCode = m_pDataQueue->push_pendingQueue(stMediaDataInfo);

    return enRetCode;
}

/* 读取分析数据 */
BlError_E CTrackerAlgorithmBase::read_analysisResult(TrackerAlgorithmResult_S& stOutInfo)
{
    if (nullptr == m_pDataQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    enRetCode = m_pDataQueue->pop_resultQueue(stOutInfo);
    return enRetCode;
}

/* 实时分析数据 */
BlError_E CTrackerAlgorithmBase::realTime_dataAnalysis(
    MediaDataInfo_S           stMediaDataInfo,
    TrackerAlgorithmResult_S& stOutInfo)
{
    return dataAnalysis(stMediaDataInfo, stOutInfo);
}

/* 线程函数 */
void CTrackerAlgorithmBase::run()
{
    BlError_E enRetCode = OK;

    std::chrono::milliseconds sleepDuration(1);

    MediaDataInfo_S stPendingInfo;

    TrackerAlgorithmResult_S stResultInfo;


    while (m_bRunning.load())
    {
        if (nullptr == m_pDataQueue)
        {
            /* 没有数据，等待200ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        /* 取数据 */
        enRetCode = m_pDataQueue->pop_pendingQueue(stPendingInfo);
        if (enRetCode < OK)
        {
            /* 没有数据，等待200ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        if (m_pDataQueue->getSize_pendingQueue() > 0)
        {
            dlog(LOG_INFO, "跟踪待分析队列[%d]", m_pDataQueue->getSize_pendingQueue());
        }

        /* 进行分析 */
        enRetCode = dataAnalysis(stPendingInfo, stResultInfo);
        if (enRetCode < OK)
        {
            continue;
        }

        /* 保存分析后的数据 */
        m_pDataQueue->push_resultQueue(stResultInfo);
    }
}