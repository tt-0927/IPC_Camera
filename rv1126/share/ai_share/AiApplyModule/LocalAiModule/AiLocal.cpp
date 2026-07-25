/*
 * @FilePath     : AiLocal.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-22 09:06:53
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-10 17:14:55
 * @Description  : 本地AI分析
 */
#include "AiLocal.hpp"

AiLocal_NS::CAiLocal::CAiLocal(returnDataFunc pHeadCount, returnDataFunc pBehavior, returnDataFunc pDiscipline)
    : m_pHeadCount(pHeadCount),
      m_pBehavior(pBehavior),
      m_pDiscipline(pDiscipline)
{
}

AiLocal_NS::CAiLocal::~CAiLocal()
{
    uninitHeadCount();
    uninitBehavior();
    uninitDiscipline();
}

/* 初始化分析功能 */
BlError_E AiLocal_NS::CAiLocal::initHeadCount()
{
    std::unique_lock<std::mutex> lock(m_headCountMutex);
    if (m_pLocalHeadCount == nullptr)
    {
        m_pLocalHeadCount = new CLocalHeadCount(m_pHeadCount);
        if (m_pLocalHeadCount && m_pLocalHeadCount->init() < OK)
        {
            delete m_pLocalHeadCount;
            m_pLocalHeadCount = nullptr;
            dlog(LOG_ERROR, "本地AI分析-人数统计-初始化失败！");
            return NOK;
        }
    }
    dlog(LOG_INFO, "本地AI分析-人数统计-初始化成功！");
    return OK;
}

/* 初始化分析功能 */
BlError_E AiLocal_NS::CAiLocal::initBehavior()
{
    std::unique_lock<std::mutex> lock(m_behaviorMutex);
    if (m_pLocalBehavior == nullptr)
    {
        m_pLocalBehavior = new CLocalBehavior(m_pBehavior);
        if (m_pLocalBehavior && m_pLocalBehavior->init() < OK)
        {
            delete m_pLocalBehavior;
            m_pLocalBehavior = nullptr;
            dlog(LOG_ERROR, "本地AI分析-行为分析-初始化失败！");
            return NOK;
        }
    }
    dlog(LOG_INFO, "本地AI分析-行为分析-初始化成功！");
    return OK;
}

/* 初始化分析功能 */
BlError_E AiLocal_NS::CAiLocal::initDiscipline()
{
    std::unique_lock<std::mutex> lock(m_disciplineMutex);
    if (m_pLocalDiscipline == nullptr)
    {
        m_pLocalDiscipline = new CLocalDisciplineGather(m_pDiscipline);
        if (m_pLocalDiscipline && m_pLocalDiscipline->init() < OK)
        {
            delete m_pLocalDiscipline;
            m_pLocalDiscipline = nullptr;
            dlog(LOG_ERROR, "本地AI分析-活跃度-初始化失败！");
            return NOK;
        }
    }
    dlog(LOG_INFO, "本地AI分析-活跃度-初始化成功！");
    return OK;
}

/* 反初始化分析功能 */
BlError_E AiLocal_NS::CAiLocal::uninitHeadCount()
{
    std::unique_lock<std::mutex> lock(m_headCountMutex);
    if (m_pLocalHeadCount)
    {
        delete m_pLocalHeadCount;
        m_pLocalHeadCount = nullptr;

        dlog(LOG_INFO, "本地AI分析-人数统计-反初始化成功！");
    }
    return OK;
}

/* 反初始化分析功能 */
BlError_E AiLocal_NS::CAiLocal::uninitBehavior()
{
    std::unique_lock<std::mutex> lock(m_behaviorMutex);
    if (m_pLocalBehavior)
    {
        delete m_pLocalBehavior;
        m_pLocalBehavior = nullptr;

        dlog(LOG_INFO, "本地AI分析-行为分析-反初始化成功！");
    }
    return OK;
}

/* 反初始化分析功能 */
BlError_E AiLocal_NS::CAiLocal::uninitDiscipline()
{
    std::unique_lock<std::mutex> lock(m_disciplineMutex);
    if (m_pLocalDiscipline)
    {
        delete m_pLocalDiscipline;
        m_pLocalDiscipline = nullptr;
        dlog(LOG_INFO, "本地AI分析-活跃度-反初始化成功！");
    }
    return OK;
}

/* 添加分析人数统计数据 */
BlError_E AiLocal_NS::CAiLocal::analyseCountStudent(AiManage_NS::CommDataInfo_S* pstData)
{
    if (!pstData)
    {
        return ERR_IN_PARAM_NULL;
    }

    std::unique_lock<std::mutex> lock(m_headCountMutex);
    if (!m_pLocalHeadCount)
    {
        if (pstData)
        {
            delete[] pstData;
            pstData = nullptr;
        }
        return ERR_IN_PARAM_NULL;
    }
    return m_pLocalHeadCount->addData(pstData);
}

/* 添加分析数据 */
BlError_E AiLocal_NS::CAiLocal::analyseStudentBehavior(AiManage_NS::CommDataInfo_S* pstData)
{
    if (!pstData)
    {
        return ERR_IN_PARAM_NULL;
    }

    std::unique_lock<std::mutex> lock(m_behaviorMutex);
    if (!m_pLocalBehavior)
    {
        if (pstData)
        {
            delete[] pstData;
            pstData = nullptr;
        }
        return ERR_IN_PARAM_NULL;
    }
    return m_pLocalBehavior->addData(pstData);
}

/* 添加课堂纪律分析数据 */
BlError_E AiLocal_NS::CAiLocal::analyseDiscipline(AiManage_NS::CommDataInfo_S* pstData)
{
    if (!pstData)
    {
        return ERR_IN_PARAM_NULL;
    }

    std::unique_lock<std::mutex> lock(m_disciplineMutex);
    if (!m_pLocalDiscipline)
    {
        if (pstData)
        {
            delete[] pstData;
            pstData = nullptr;
        }
        return ERR_IN_PARAM_NULL;
    }
    return m_pLocalDiscipline->addData(pstData);
}

/* 清空数据 */
BlError_E AiLocal_NS::CAiLocal::clearData()
{
    std::unique_lock<std::mutex> lock1(m_headCountMutex);
    if (m_pLocalHeadCount)
    {
        m_pLocalHeadCount->clearData();
    }

    std::unique_lock<std::mutex> lock2(m_behaviorMutex);
    if (m_pLocalBehavior)
    {
        m_pLocalBehavior->clearData();
    }

    std::unique_lock<std::mutex> lock3(m_disciplineMutex);
    if (m_pLocalDiscipline)
    {
        m_pLocalDiscipline->clearData();
    }

    return OK;
}