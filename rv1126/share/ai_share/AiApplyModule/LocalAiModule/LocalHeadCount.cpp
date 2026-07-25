/*
 * @FilePath     : LocalHeadCount.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-31 17:17:57
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-10 19:44:04
 * @Description  : 人数统计
 */
#include "LocalHeadCount.hpp"

#include <unistd.h>

#include "JsonInterfase.h"
#include "share_device.h"

AiLocal_NS::CLocalHeadCount::CLocalHeadCount(returnDataFunc pReturn)
    : m_pReturn(pReturn)
{
}

AiLocal_NS::CLocalHeadCount::~CLocalHeadCount()
{
    /* 结束线程 */
    m_bRunning.store(false);
    m_threadObj.join();

    if (m_pAlgorithmHndl)
    {
        delete m_pAlgorithmHndl;
        m_pAlgorithmHndl = nullptr;
    }
}

/* 添加分析数据 */
BlError_E AiLocal_NS::CLocalHeadCount::addData(AiManage_NS::CommDataInfo_S* pstData)
{
    if (pstData && m_pAlgorithmHndl)
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        if (m_pendingQueue.size() >= m_nMaxQueueSize)
        {
            if (!m_pendingQueue.empty())
            {
                AiManage_NS::CommDataInfo_S* pstValue = m_pendingQueue.front();
                m_pendingQueue.pop();
                if (pstValue)
                {
                    delete[] pstValue;
                    pstValue = nullptr;
                }
            }
        }
        m_pendingQueue.push(pstData);
        dlog(LOG_TRACE, "本地人数统计分析，添加数据队列[%ld]", m_pendingQueue.size());
        m_condition.notify_one();
        return OK;
    }
    else
    {
        if (!pstData)
        {
            dlog(LOG_ERROR, "添加人数统计分析数据-失败, 传入的数据为空");
        }

        if (!m_pAlgorithmHndl)
        {
            dlog(LOG_ERROR, "添加人数统计分析数据-失败, 本类未初始化");
        }
    }

    if (pstData)
    {
        delete[] pstData;
        pstData = nullptr;
    }
    return NOK;
}

/* 清空数据 */
BlError_E AiLocal_NS::CLocalHeadCount::clearData()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    while (!m_pendingQueue.empty())
    {
        AiManage_NS::CommDataInfo_S* pstValue = m_pendingQueue.front();
        m_pendingQueue.pop();
        if (pstValue)
        {
            delete[] pstValue;
            pstValue = nullptr;
        }
    }

    return OK;
}

/* 初始化分析功能 */
BlError_E AiLocal_NS::CLocalHeadCount::init()
{
    if (m_pAlgorithmHndl)
    {
        dlog(LOG_ERROR, "人数统计功能已初始化, 不用再次初始化");
        return ERR_INI_ERR;
    }

    AiScenario_NS::InParam_S stInParam;
    stInParam.clear();
    stInParam.stNeedParam.enType       = AiScenario_NS::HUMAN_CUTOUT;
    stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;
    stInParam.stNeedParam.vstrModelPath.clear();
    stInParam.stExParam.bDebug = false;

    stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    DeviceID_E enDevType             = share_get_currDeviceID();
    if (TS_0663L == enDevType ||
        TS_0663PH == enDevType)
    {
        if (access("/opt/bl/model/RK3576_model/RK3576_Head_Seg_V1.0.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/bl/model/RK3576_model/RK3576_Head_Seg_V1.0.rknn");
        }
        else if (access("/opt/rk/model/RK3576_model/RK3576_Head_Seg_V1.0.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/rk/model/RK3576_model/RK3576_Head_Seg_V1.0.rknn");
        }
        else
        {
            return ERR_INI_ERR;
        }
    }
    else
    {
        if (access("/opt/bl/model/RK3588_model/RK3588_HR_human768x640_NWPU.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/bl/model/RK3588_model/RK3588_HR_human768x640_NWPU.rknn");
        }
        else if (access("/opt/rk/model/RK3588_model/RK3588_HR_human768x640_NWPU.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/rk/model/RK3588_model/RK3588_HR_human768x640_NWPU.rknn");
        }
        else
        {
            return ERR_INI_ERR;
        }
    }
    stInParam.stExParam.strAnalyzeDataPath = "/root/111/";
    m_pAlgorithmHndl                       = new Scenario_NS::CHeadCountV1_0(stInParam);
    if (!m_pAlgorithmHndl)
    {
        dlog(LOG_ERROR, "创建人数统计句柄-失败");
        return ERR_INI_ERR;
    }

    if (!m_pAlgorithmHndl->init())
    {
        dlog(LOG_ERROR, "初始化人数统计句柄-失败");
        return ERR_INI_ERR;
    }

    /* 创建线程 */
    m_threadObj = std::thread(&CLocalHeadCount::run, this);

    dlog(LOG_TRACE, "人数统计功能初始化成功");

    return OK;
}

/* 线程函数 */
void AiLocal_NS::CLocalHeadCount::run()
{
    BlError_E      enRetCode    = OK;
    constexpr auto kWaitTimeout = std::chrono::milliseconds(500);

    while (m_bRunning.load())
    {
        AiManage_NS::CommDataInfo_S* pstDataInfo = nullptr;

        AiScenario_NS::CVData_S stInData;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait_for(lock, kWaitTimeout, [this] {
                return !m_bRunning.load() || !m_pendingQueue.empty();
            });

            if (!m_bRunning.load())
            {
                return;
            }

            if (m_pendingQueue.size() > 0)
            {
                pstDataInfo = m_pendingQueue.front();
                m_pendingQueue.pop();
            }
            else
            {
                pstDataInfo = nullptr;
            }
        }

        if (m_pAlgorithmHndl && pstDataInfo)
        {
            char* pData    = pstDataInfo->data + pstDataInfo->nExSize;
            stInData.inMat = cv::imdecode(cv::_InputArray(pData, pstDataInfo->nDataSize), cv::IMREAD_COLOR);

            char* pchProcessData = nullptr;
            char* pchOutJson     = nullptr;
            int   nDateSize      = 0;
            if (m_pAlgorithmHndl->process(stInData, pchProcessData, nDateSize))
            {
                /* 重新组装JSON数据 */
                auto pRootJson = Json::init();
                auto pDataJson = Json::init(pchProcessData);

                Json::add(pRootJson, "Mode", pstDataInfo->nCode);
                Json::add(pRootJson, "CurRecordTime", pstDataInfo->nCurRecordTime);
                Json::add(pRootJson, "Timestamp", pstDataInfo->lTimestamp);
                Json::add(pRootJson, "ClassId", pstDataInfo->nClassId);
                if (pDataJson)
                {
                    Json::add(pRootJson, "Datas", pDataJson);
                }
                else
                {
                    Json::add(pRootJson, "Datas");
                }


                /* 转换成字符串 */
                pchOutJson = Json::print(pRootJson);

                /* 释放数据 */
                Json::deinit(pRootJson);
                pRootJson = nullptr;

                /* 发送外部处理 */
                if (m_pReturn)
                {
                    m_pReturn(pchOutJson);
                }
                // dlog(LOG_TRACE, "本地人数统计分析数据:\n%s",
                //      pchOutJson);

                Json::release(pchOutJson);
                pchOutJson = nullptr;

                m_pAlgorithmHndl->releaseData(pchProcessData);
            }
        }

        if (pstDataInfo)
        {
            delete[] pstDataInfo;
            pstDataInfo = nullptr;
        }
    }
}