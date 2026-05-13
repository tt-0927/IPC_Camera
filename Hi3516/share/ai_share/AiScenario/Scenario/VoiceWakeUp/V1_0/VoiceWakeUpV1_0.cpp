/*
 * @FilePath     : VoiceWakeUpV1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 17:39:27
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:39:31
 * @Description  :
 */

#include "VoiceWakeUpV1_0.hpp"

#include "dlog.h"
#include "JsonInterfase.h"
#include "KWS.hpp"


using namespace Scenario_NS;

Scenario_NS::CVoiceWakeUpV1_0::CVoiceWakeUpV1_0(AiScenario_NS::InParam_S stInParam)
    : CScenarioBase(stInParam)
{
}

Scenario_NS::CVoiceWakeUpV1_0::~CVoiceWakeUpV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CVoiceWakeUpV1_0::init()
{
    bool bRet = false;
    if (m_stInParam.stNeedParam.vstrModelPath.size() >= 5)
    {
        bRet = false;

        m_pInference = new InferenceV1_0_NS::CKWS(
            m_stInParam.stNeedParam.vstrModelPath.at(0),
            m_stInParam.stNeedParam.vstrModelPath.at(1),
            m_stInParam.stNeedParam.vstrModelPath.at(2),
            m_stInParam.stNeedParam.vstrModelPath.at(3),
            m_stInParam.stNeedParam.vstrModelPath.at(4));
        if (m_pInference)
        {
            bRet = m_pInference->init();
        }

        if (!bRet)
        {
            dlog(LOG_ERROR, "模型初始化失败 0=[%s] 1=[%s] 2=[%s] 3=[%s] 4=[%s] ",
                 m_stInParam.stNeedParam.vstrModelPath.at(0).c_str(),
                 m_stInParam.stNeedParam.vstrModelPath.at(1).c_str(),
                 m_stInParam.stNeedParam.vstrModelPath.at(2).c_str(),
                 m_stInParam.stNeedParam.vstrModelPath.at(3).c_str(),
                 m_stInParam.stNeedParam.vstrModelPath.at(4).c_str());
            goto FAIL;
        }
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool Scenario_NS::CVoiceWakeUpV1_0::unInit()
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
bool Scenario_NS::CVoiceWakeUpV1_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为音频场景, 调用失败");
    return false;
}

/* 处理数据 */
bool Scenario_NS::CVoiceWakeUpV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (!stInData.pData || stInData.nDataSize <= 0 || stInData.nSample <= 0)
    {
        dlog(LOG_ERROR, "传入数据为空");
        return false;
    }

    if (!m_pInference)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return false;
    }

    bool        bRet = true;
    std::string strInferenceData;

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存图片 */
        if (stInData.pData && !m_stInParam.stExParam.strAnalyzeDataPath.empty())
        {
            static FILE* fp = nullptr;
            if (!fp)
            {
                /* 以只读方式打开二进制文件 */
                fp = fopen(m_stInParam.stExParam.strAnalyzeDataPath.c_str(), "wb+");
            }
            if (fp)
            {
                fwrite(stInData.pData, sizeof(int8_t), stInData.nDataSize, fp);
            }
        }
    }

    /* 推理 */
    bRet = m_pInference->inference(stInData, strInferenceData);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        goto EXIT;
    }

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = true;
            if (!strInferenceData.empty())
            {
                bRet = convertToJson(strInferenceData, &pchOutData, nDataSize);
            }
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
            nDataSize  = static_cast<int>(strInferenceData.size()); /* 长度 */
            pchOutData = new char[nDataSize + 1];                   /* 多一个字节存放 '\0' */
            memcpy(pchOutData, strInferenceData.c_str(), nDataSize + 1);
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

/* 释放处理结果 */
bool Scenario_NS::CVoiceWakeUpV1_0::releaseData(char*& pchOutData)
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
            delete pchOutData;
            pchOutData = NULL;
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

bool Scenario_NS::CVoiceWakeUpV1_0::convertToJson(
    std::string strData,
    char**      pchOutData,
    int&        nDataSize)
{
    auto pRootJson      = Json::init();
    auto pArrayDataJson = Json::Array::init();

    auto pItem = Json::init();
    Json::add(pItem, "Voice", strData);
    Json::Array::add(pArrayDataJson, pItem);

    Json::add(pRootJson, "BaseData", pArrayDataJson);

    *pchOutData = Json::print(pRootJson);

    nDataSize = strlen(Json::to_string(pRootJson).c_str());

    if (pRootJson)
    {
        /* 释放数据 */
        Json::deinit(pRootJson);
        pRootJson = nullptr;
    }

    return true;
}
