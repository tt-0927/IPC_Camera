/*
 * @FilePath     : NumberOcrV1_0.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-30 08:53:40
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:48:04
 * @Description  : 黑底白数字识别
 */

#include "NumberOcrV1_0.hpp"

#include "dlog.h"
#include "JsonInterfase.h"
#include "NumberOcr.hpp"


using namespace Scenario_NS;

Scenario_NS::CNumberOcrV1_0::CNumberOcrV1_0(AiScenario_NS::InParam_S stInParam)
    : CScenarioBase(stInParam)
{
}

Scenario_NS::CNumberOcrV1_0::~CNumberOcrV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CNumberOcrV1_0::init()
{
    bool bRet = false;

    if (m_stInParam.stNeedParam.vstrModelPath.size() > 0)
    {
        bRet = false;

        m_pInference = new InferenceV1_0_NS::CNumberOcr(
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
bool Scenario_NS::CNumberOcrV1_0::unInit()
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
bool Scenario_NS::CNumberOcrV1_0::process(
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
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        try
        {
            cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            return false;
        }
    }

    if (stInData.inMat.channels() != m_nLimitChannel)
    {
        if (stInData.inMat.channels() == 3)
        {
            cv::cvtColor(stInData.inMat, stInData.inMat, cv::COLOR_RGB2GRAY);
        }
        else
        {
            dlog(LOG_ERROR, "模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]", stInData.inMat.channels(), m_nLimitChannel);
            goto EXIT;
        }
    }

    /* 二值化，这里将128以下的变为0，128以上的变为255 */
    cv::threshold(stInData.inMat, stInData.inMat, 128, 255, cv::THRESH_BINARY);

    /* 推理+后处理 */
    bRet = m_pInference->inference(stInData, vecPos);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        goto EXIT;
    }

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.stExParam.strAnalyzeDataPath.empty())
        {
            if (vecPos.size() > 0)
            {
                int nNumber = (int)vecPos[0];

                cv::putText(
                    stInData.inMat,
                    std::to_string(nNumber),
                    cv::Point(32, 32),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.9,
                    cv::Scalar(128, 128, 128),
                    4);

                if (!saveImage(stInData.inMat, m_stInParam.stExParam.strAnalyzeDataPath))
                {
                    dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.stExParam.strAnalyzeDataPath.c_str());
                }
            }
        }
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
            dlog(LOG_ERROR, "该算法场景未实现 XML格式返回");
            break;
        }
        case AiScenario_NS::PIC:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景未实现 PIC格式返回");
            break;
        }
        case AiScenario_NS::STRING:
        {
            bRet = convertToString(vecPos, &pchOutData, nDataSize);
            break;
        }
        default:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景未实现 [%d]格式返回", m_stInParam.stNeedParam.enResultType);

            break;
        }
    }

EXIT:

    return bRet;
}

/* 处理数据 */
bool Scenario_NS::CNumberOcrV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CNumberOcrV1_0::releaseData(char*& pchOutData)
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
        case AiScenario_NS::STRING:
        {
            delete[] pchOutData;
            pchOutData = nullptr;
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

bool Scenario_NS::CNumberOcrV1_0::convertToJson(
    std::vector<float> vPointsXY,
    char**             pchOutData,
    int&               nDataSize)
{
    auto pRootJson      = Json::init();
    auto pArrayDataJson = Json::Array::init();

    for (int nIndex = 0; nIndex < vPointsXY.size(); nIndex++)
    {
        if (vPointsXY.size() >= (nIndex + 1))
        {
            auto pItem = Json::init();
            Json::add(pItem, "Number", (int)vPointsXY[nIndex]);
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

bool Scenario_NS::CNumberOcrV1_0::convertToString(
    std::vector<float> vPointsXY,
    char**             pchOutData,
    int&               nDataSize)
{
    if (!pchOutData || vPointsXY.size() <= 0)
    {
        return false;
    }

    int nNumber = (int)vPointsXY[0];

    /* 计算整数值转换为字符串所需的空间 */
    int nLength = snprintf(nullptr, 0, "%d", nNumber);
    if (nLength < 0)
    {
        return false;
    }

    /* 为字符串分配内存 */
    *pchOutData = new char[nLength + 1];
    if (*pchOutData == nullptr)
    {
        /* 内存分配失败 */
        return false;
    }

    /* 将整数值转换为字符串并存储在分配的内存中 */
    snprintf(*pchOutData, nLength + 1, "%d", nNumber);

    nDataSize = nLength;

    return true;
}
