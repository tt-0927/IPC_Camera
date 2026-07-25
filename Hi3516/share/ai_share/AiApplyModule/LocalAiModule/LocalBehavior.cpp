/*
 * @FilePath     : LocalBehavior.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-17 16:16:35
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-10 19:56:07
 * @Description  : 学生行为分析
 */
#include "LocalBehavior.hpp"

#include <unistd.h>

#include "JsonInterfase.h"
#include "share_device.h"

AiLocal_NS::CLocalBehavior::CLocalBehavior(returnDataFunc pReturn)
    : m_pReturn(pReturn)
{
}

AiLocal_NS::CLocalBehavior::~CLocalBehavior()
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
BlError_E AiLocal_NS::CLocalBehavior::addData(AiManage_NS::CommDataInfo_S* pstData)
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
        dlog(LOG_TRACE, "本地学生行为分析，添加数据队列[%ld]", m_pendingQueue.size());
        m_condition.notify_one();
        return OK;
    }
    else
    {
        if (!pstData)
        {
            dlog(LOG_ERROR, "添加学生行为分析数据-失败, 传入的数据为空");
        }

        if (!m_pAlgorithmHndl)
        {
            dlog(LOG_ERROR, "添加学生行为分析数据-失败, 本类未初始化");
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
BlError_E AiLocal_NS::CLocalBehavior::clearData()
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
BlError_E AiLocal_NS::CLocalBehavior::init()
{
    if (m_pAlgorithmHndl)
    {
        dlog(LOG_ERROR, "学生行为分析功能已初始化, 不用再次初始化");
        return ERR_INI_ERR;
    }

    AiScenario_NS::InParam_S stInParam;
    stInParam.clear();
    stInParam.stNeedParam.enType       = AiScenario_NS::STUDENT_BEHAVIOR;
    stInParam.stNeedParam.enVersions   = AiScenario_NS::V2_0;
    stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;
    stInParam.stNeedParam.vstrModelPath.clear();
    DeviceID_E enDevType = share_get_currDeviceID();
    if (TS_0663L == enDevType ||
        TS_0663PH == enDevType)
    {
        if (access("/opt/bl/model/RK3576_model/RK3576_Head_Detect_V1.0.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/bl/model/RK3576_model/RK3576_Head_Detect_V1.0.rknn");
        }
        else if (access("/opt/rk/model/RK3576_model/RK3576_Head_Detect_V1.0.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/rk/model/RK3576_model/RK3576_Head_Detect_V1.0.rknn");
        }
        else
        {
            return ERR_INI_ERR;
        }

        if (access("/opt/bl/model/RK3576_model/RK3576_Human_Point_V1.0.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/bl/model/RK3576_model/RK3576_Human_Point_V1.0.rknn");
        }
        else if (access("/opt/rk/model/RK3576_model/RK3576_Human_Point_V1.0.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/rk/model/RK3576_model/RK3576_Human_Point_V1.0.rknn");
        }
        else
        {
            return ERR_INI_ERR;
        }
    }
    else
    {
        if (access("/opt/bl/model/RK3588_model/RK3588_HeadDetect.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/bl/model/RK3588_model/RK3588_HeadDetect.rknn");
        }
        else if (access("/opt/rk/model/RK3588_model/RK3588_HeadDetect.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/rk/model/RK3588_model/RK3588_HeadDetect.rknn");
        }
        else
        {
            return ERR_INI_ERR;
        }

        if (access("/opt/bl/model/RK3588_model/RK3588_Fastpose.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/bl/model/RK3588_model/RK3588_Fastpose.rknn");
        }
        else if (access("/opt/rk/model/RK3588_model/RK3588_Fastpose.rknn", F_OK) == 0)
        {
            stInParam.stNeedParam.vstrModelPath.push_back("/opt/rk/model/RK3588_model/RK3588_Fastpose.rknn");
        }
        else
        {
            return ERR_INI_ERR;
        }
    }

    stInParam.stExParam.fBoxThreshold = 0.7;

    stInParam.stExParam.bDebug              = false;
    stInParam.stExParam.strOriginalDataPath = "/opt/course/O_stBehavior";
    stInParam.stExParam.strAnalyzeDataPath  = "/opt/course/stBehavior";
    if (stInParam.stExParam.bDebug)
    {
        system("rm /opt/course/O_stBehavior -r;rm /opt/course/stBehavior -r");
    }

    m_pAlgorithmHndl = new Scenario_NS::CStudentBehaviorV2_0(stInParam);
    if (!m_pAlgorithmHndl)
    {
        dlog(LOG_ERROR, "创建学生行为分析句柄-失败");
        return ERR_INI_ERR;
    }

    if (!m_pAlgorithmHndl->init())
    {
        dlog(LOG_ERROR, "初始化学生行为分析句柄-失败");
        return ERR_INI_ERR;
    }

    /* 创建线程 */
    m_bRunning.store(true);
    m_threadObj = std::thread(&CLocalBehavior::run, this);

    dlog(LOG_TRACE, "学生行为分析功能初始化成功");

    return OK;
}

/* 线程函数 */
void AiLocal_NS::CLocalBehavior::run()
{
    BlError_E enRetCode = OK;

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
            /* 赋值varParam持有BehaviorParam_S类型 */
            stInData.varParam = AiScenario_NS::BehaviorParam_S {};
            if (std::holds_alternative<AiScenario_NS::BehaviorParam_S>(stInData.varParam))
            {
                AiScenario_NS::BehaviorParam_S& behaviorParam = std::get<AiScenario_NS::BehaviorParam_S>(stInData.varParam);
                memcpy(&behaviorParam, pstDataInfo->data, pstDataInfo->nExSize);
            }

            char* pData = pstDataInfo->data + pstDataInfo->nExSize;

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
                // dlog(LOG_TRACE, "本地学生行为分析数据:\n%s",
                //      pchOutJson);

                Json::release(pchOutJson);
                pchOutJson = nullptr;
            }

            if (pchProcessData)
            {
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