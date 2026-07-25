/*
 * @FilePath     : CountingAlgorithmBase.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 13:59:20
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-02-21 16:34:48
 * @Description  :
 */
#include "CountingAlgorithmBase.hpp"

#include "dlog.h"

using namespace CA_NS;

static void freeMediaData(MediaDataInfo_S stMediaDataInfo)
{
    stMediaDataInfo.free();
}

CA_NS::CCountingAlgorithmBase::CCountingAlgorithmBase(CountingAnalyzerInParam_S stInParam)
    : m_stInParam(stInParam)
{
    if (nullptr == m_pDataQueue)
    {
        m_pDataQueue = new CDataQueue<MediaDataInfo_S, CountingAnalyzerResult_S>(freeMediaData, nullptr, stInParam.stExParam.nMaxQueue);
    }

    /* 创建线程 */
    m_threadObj = std::thread(&CCountingAlgorithmBase::run, this);
}

CA_NS::CCountingAlgorithmBase::~CCountingAlgorithmBase()
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
BlError_E CA_NS::CCountingAlgorithmBase::send_dataAnalysis(MediaDataInfo_S stMediaDataInfo)
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

    enRetCode = m_pDataQueue->push_pendingQueue(stMediaDataInfo);
    return enRetCode;
}

/* 读取分析数据 */
BlError_E CA_NS::CCountingAlgorithmBase::read_analysisResult(CountingAnalyzerResult_S& stOutInfo)
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
BlError_E CA_NS::CCountingAlgorithmBase::realTime_dataAnalysis(
    MediaDataInfo_S           stMediaDataInfo,
    CountingAnalyzerResult_S& stOutInfo)
{
    return dataAnalysis(stMediaDataInfo, stOutInfo);
}

/* 线程函数 */
void CA_NS::CCountingAlgorithmBase::run()
{
    BlError_E enRetCode = OK;

    std::chrono::milliseconds sleepDuration(10);

    MediaDataInfo_S stPendingInfo;

    CountingAnalyzerResult_S stResultInfo;

    while (m_bRunning.load())
    {
        if (nullptr == m_pDataQueue)
        {
            /* 没有数据，等ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        /* 取数据 */
        enRetCode = m_pDataQueue->pop_pendingQueue(stPendingInfo);
        if (enRetCode < OK)
        {
            /* 没有数据，等待ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        if (m_pDataQueue->getSize_pendingQueue() > 0)
        {
            dlog(LOG_INFO, "人数统计待分析队列[%d]", m_pDataQueue->getSize_pendingQueue());
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
