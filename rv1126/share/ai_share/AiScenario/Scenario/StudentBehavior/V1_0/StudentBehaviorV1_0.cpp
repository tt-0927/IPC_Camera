/*
 * @FilePath     : StudentBehaviorV1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 17:39:27
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:38:44
 * @Description  :
 */

#include "StudentBehaviorV1_0.hpp"

#include "dlog.h"
#include "JsonInterfase.h"
#include "StudentBehavior.hpp"

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6

using namespace Scenario_NS;

Scenario_NS::CStudentBehaviorV1_0::CStudentBehaviorV1_0(AiScenario_NS::InParam_S stInParam)
    : CScenarioBase(stInParam)
{
    if (stInParam.stExParam.fBoxThreshold != 0.0f)
    {
        m_fBoxThreshold = stInParam.stExParam.fBoxThreshold;
    }

    if (stInParam.stExParam.fNmsThreshold != 0.0f)
    {
        m_fNmsThreshold = stInParam.stExParam.fNmsThreshold;
    }
}

Scenario_NS::CStudentBehaviorV1_0::~CStudentBehaviorV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CStudentBehaviorV1_0::init()
{
    bool bRet = false;

    if (m_stInParam.stNeedParam.vstrModelPath.size() > 0)
    {
        bRet = false;

        m_pInference = new InferenceV1_0_NS::CStudentBehavior(
            m_stInParam.stNeedParam.vstrModelPath.at(0));
        if (m_pInference)
        {
            if (m_pInference->init())
            {
                bRet = m_pInference->getSizeLimit(
                    0,
                    m_nLimitWidth,
                    m_nLimitHeight,
                    m_nLimitChannel);

                if (m_nLimitWidth <= 0 || m_nLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nLimitWidth,
                         m_nLimitHeight);
                    bRet = false;
                }
            }
        }

        if (!bRet)
        {
            dlog(LOG_ERROR, "模型初始化失败 [%s]",
                 m_stInParam.stNeedParam.vstrModelPath.at(0).c_str());
            goto FAIL;
        }
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool Scenario_NS::CStudentBehaviorV1_0::unInit()
{
    if (m_pInference)
    {
        delete m_pInference;
        m_pInference = nullptr;
        return true;
    }
    return false;
}

/* 处理数据 */
bool Scenario_NS::CStudentBehaviorV1_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (stInData.inMat.empty())
    {
        dlog(LOG_ERROR, "传入图片为空");
        return false;
    }

    if (!m_pInference)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return false;
    }

    bool bRet = true;

    std::vector<float> vecPos;
    vecPos.clear();

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.stExParam.strOriginalDataPath.empty())
        {
            if (!saveImage(stInData.inMat, m_stInParam.stExParam.strOriginalDataPath))
            {
                dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.stExParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* 前处理 */
    if (stInData.inMat.channels() != m_nLimitChannel)
    {
        dlog(LOG_ERROR, "模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]", stInData.inMat.channels(), m_nLimitChannel);
        bRet = false;
        goto EXIT;
    }
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        try
        {
            cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            bRet = false;
            goto EXIT;
        }
    }

    /* 推理+后处理 */
    bRet = m_pInference->inference(stInData, vecPos);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        goto EXIT;
    }

    if ((vecPos.size() % DATA_GROUP_SIZE) != 0)
    {
        dlog(LOG_ERROR, "算法推理结果异常");
        goto EXIT;
    }

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = convertToJson(vecPos, &pchOutData, nDataSize);
            break;
        }
        case AiScenario_NS::XML:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 XML格式返回");
            break;
        }
        case AiScenario_NS::PIC:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 PIC格式返回");
            break;
        }
        default:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 [%d]格式返回", m_stInParam.stNeedParam.enResultType);

            break;
        }
    }

EXIT:

    return bRet;
}

/* 处理数据 */
bool Scenario_NS::CStudentBehaviorV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CStudentBehaviorV1_0::releaseData(char*& pchOutData)
{
    if (!pchOutData)
    {
        return false;
    }

    int bRet = true;

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            Json::release(pchOutData);
            break;
        }
        case AiScenario_NS::XML:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 XML格式返回");
            break;
        }
        case AiScenario_NS::PIC:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 PIC格式返回");
            break;
        }
        default:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 [%d]格式返回", m_stInParam.stNeedParam.enResultType);

            break;
        }
    }

    return bRet;
}

bool Scenario_NS::CStudentBehaviorV1_0::convertToJson(
    std::vector<float> vPointsXY,
    char**             pchOutData,
    int&               nDataSize)
{
    auto pRootJson      = Json::init();
    auto pArrayDataJson = Json::Array::init();

    for (int nIndex = 0; nIndex < vPointsXY.size(); nIndex++)
    {
        if (vPointsXY.size() >= ((nIndex + 1) * DATA_GROUP_SIZE))
        {
            auto pArrayBox = Json::Array::init();
            auto pItem     = Json::init();
            Json::Array::add(pArrayBox, (int)vPointsXY[nIndex * DATA_GROUP_SIZE]);
            Json::Array::add(pArrayBox, (int)vPointsXY[(nIndex * DATA_GROUP_SIZE) + 1]);
            Json::Array::add(pArrayBox, (int)vPointsXY[(nIndex * DATA_GROUP_SIZE) + 2]);
            Json::Array::add(pArrayBox, (int)vPointsXY[(nIndex * DATA_GROUP_SIZE) + 3]);
            Json::add(pItem, "Box", pArrayBox);
            Json::add(pItem, "Confidence", vPointsXY[(nIndex * DATA_GROUP_SIZE) + 4]);
            Json::add(pItem, "Class", int(vPointsXY[(nIndex * DATA_GROUP_SIZE) + 5] + 0.5));
            Json::Array::add(pArrayDataJson, pItem);
        }
    }
    Json::add(pRootJson, "BaseData", pArrayDataJson);

    *pchOutData = Json::print(pRootJson);

    nDataSize = strlen(Json::to_string(pRootJson).c_str());

    if (pRootJson)
    {
        /* 释放数据 */
        Json::deinit(pRootJson);
    }

    return true;
}
