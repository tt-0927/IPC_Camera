/*
 * @FilePath     : BridgeAlgorithmBase.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 10:08:07
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 16:10:44
 * @Description  :
 */
#include "BridgeAlgorithmBase.hpp"

#include "dlog.h"

using namespace BDGA_NS;
using namespace BQ_NS;

static void freeMediaData(MediaDataInfo_S& stMediaDataInfo)
{
    stMediaDataInfo.free();
}

CBridgeAlgorithmBase::CBridgeAlgorithmBase(InParam_S stInParam)
    : m_stInParam(stInParam)
{
    m_pPendingQueue = new CBlockingQueue<MediaDataInfo_S>(stInParam.stExParam.nMaxQueue, freeMediaData);
    m_pResultQueue  = new CBlockingQueue<AnalyzerResult_S>(stInParam.stExParam.nMaxQueue);

    /* 创建线程 */
    m_threadObj = std::thread(&CBridgeAlgorithmBase::run, this);
}

CBridgeAlgorithmBase::~CBridgeAlgorithmBase()
{
    /* 结束线程 */
    m_bRunning.store(false);
    m_threadObj.join();

    if (m_pPendingQueue)
    {
        delete m_pPendingQueue;
        m_pPendingQueue = nullptr;
    }

    if (m_pResultQueue)
    {
        delete m_pResultQueue;
        m_pResultQueue = nullptr;
    }
}

/* 发送分析数据 */
BlError_E CBridgeAlgorithmBase::send_dataAnalysis(MediaDataInfo_S stMediaDataInfo, int nTimeOutMs)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    if (nullptr == m_pPendingQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    if (!m_pPendingQueue->push(stMediaDataInfo, nTimeOutMs))
    {
        dlog(LOG_ERROR, "入队失败");
        enRetCode = NOK;
    }
    return enRetCode;
}

/* 读取分析数据 */
BlError_E CBridgeAlgorithmBase::read_analysisResult(AnalyzerResult_S& stOutInfo, int nTimeOutMs)
{
    if (nullptr == m_pResultQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    if (!m_pResultQueue->pop(stOutInfo, nTimeOutMs))
    {
        dlog(LOG_ERROR, "出队失败");
        enRetCode = NOK;
    }
    return enRetCode;
}

/* 实时分析数据 */
BlError_E CBridgeAlgorithmBase::realTime_dataAnalysis(
    MediaDataInfo_S   stMediaDataInfo,
    AnalyzerResult_S& stOutInfo)
{
    return dataAnalysis(stMediaDataInfo, stOutInfo);
}

/* 设置桥梁基准线 */
BlError_E BDGA_NS::CBridgeAlgorithmBase::setDatumLine(std::list<PosInfo_S> listPos)
{
    dlog(LOG_ERROR, "未定义设置桥梁基准线函数");
    return NOK;
}

/* 坍塌检测线程函数 */
void CBridgeAlgorithmBase::run()
{
    BlError_E enRetCode = OK;

    std::chrono::milliseconds sleepDuration(100);

    MediaDataInfo_S stPendingInfo;

    AnalyzerResult_S stResultInfo;

    while (m_bRunning.load())
    {
        if (nullptr == m_pPendingQueue || nullptr == m_pResultQueue)
        {
            /* 没有数据，等待ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        // if (m_pPendingQueue->size() > 0)
        // {
        //     dlog(LOG_INFO, "断桥分析待分析队列[%d]", m_pPendingQueue->size());
        // }

        /* 取数据 */
        if (!m_pPendingQueue->pop(stPendingInfo, -1))
        {
            /* 没有数据，等待ms */
            // std::this_thread::sleep_for(sleepDuration);
            continue;
        }


        /* 进行分析 */
        enRetCode = dataAnalysis(stPendingInfo, stResultInfo);
        if (enRetCode < OK)
        {
            continue;
        }

        /* 保存分析后的数据 */
        m_pResultQueue->pushOrReplace(stResultInfo);
    }
}
