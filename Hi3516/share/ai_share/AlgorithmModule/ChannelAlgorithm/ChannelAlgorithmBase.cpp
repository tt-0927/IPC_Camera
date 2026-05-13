/*
 * @FilePath     : ChannelAlgorithmBase.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 10:08:07
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-08 10:53:48
 * @Description  :
 */
#include "ChannelAlgorithmBase.hpp"

#include "dlog.h"

using namespace ChannelAlgorithm_NS;

static void freeMediaData(MediaDataInfo_S stMediaDataInfo)
{
    stMediaDataInfo.free();
}

CChannelAlgorithmBase::CChannelAlgorithmBase(InParam_S stInParam)
    : m_stInParam(stInParam)
{
    if (nullptr == m_pDataQueue)
    {
        m_pDataQueue = new CDataQueue<MediaDataInfo_S, std::list<AnalyzerResult_S>>(freeMediaData, nullptr, stInParam.stExParam.nMaxQueue);
    }

    /* 创建线程 */
    m_threadObj = std::thread(&CChannelAlgorithmBase::run, this);
}

CChannelAlgorithmBase::~CChannelAlgorithmBase()
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
BlError_E CChannelAlgorithmBase::send_dataAnalysis(MediaDataInfo_S stMediaDataInfo)
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
BlError_E CChannelAlgorithmBase::read_analysisResult(std::list<AnalyzerResult_S>& listOutInfo)
{
    if (nullptr == m_pDataQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    enRetCode = m_pDataQueue->pop_resultQueue(listOutInfo);
    return enRetCode;
}

/* 实时分析数据 */
BlError_E CChannelAlgorithmBase::realTime_dataAnalysis(
    MediaDataInfo_S              stMediaDataInfo,
    std::list<AnalyzerResult_S>& listOutInfo)
{
    return dataAnalysis(stMediaDataInfo, listOutInfo);
}

/* 线程函数 */
void CChannelAlgorithmBase::run()
{
    BlError_E enRetCode = OK;

    std::chrono::milliseconds sleepDuration(10);

    MediaDataInfo_S stPendingInfo;

    std::list<AnalyzerResult_S> listResultInfo;

    while (m_bRunning.load())
    {
        if (nullptr == m_pDataQueue)
        {
            /* 没有数据，等待ms */
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
            dlog(LOG_INFO, "断桥分析待分析队列[%d]", m_pDataQueue->getSize_pendingQueue());
        }

        /* 进行分析 */
        enRetCode = dataAnalysis(stPendingInfo, listResultInfo);
        if (enRetCode < OK)
        {
            continue;
        }

        /* 保存分析后的数据 */
        m_pDataQueue->push_resultQueue(listResultInfo);
    }
}
