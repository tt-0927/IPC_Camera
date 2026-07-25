/*
 * @FilePath     : VirtualCutoutV1_0.cpp
 * @Author       : 李辉 lihui@kfb.cn
 * @Date         : 2024-09-02 09:13:16
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-26 11:25:41
 * @Description  : 虚拟抠像场景
 */

#include "VirtualCutoutV1_0.hpp"

#include "dlog.h"
#include "ColorCutout.hpp"
#include "JsonInterfase.h"



/* 一组数据的大小 */
#define DATA_GROUP_SIZE 2

using namespace Scenario_NS;

Scenario_NS::CVirtualCutoutV1_0::CVirtualCutoutV1_0(AiScenario_NS::InParam_S stInParam)
    : CScenarioBase(stInParam)
{
}

Scenario_NS::CVirtualCutoutV1_0::~CVirtualCutoutV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CVirtualCutoutV1_0::init()
{
    if (m_pInference == nullptr)
    {
        m_pInference = new ColorCutout_NS::CColorCutout();
        if (m_pInference)
        {
            return true;
        }
    }

    return false;
}

/* 反初始化 */
bool Scenario_NS::CVirtualCutoutV1_0::unInit()
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
bool Scenario_NS::CVirtualCutoutV1_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (stInData.inMat.empty() || !std::holds_alternative<AiScenario_NS::CutoutParam_S>(stInData.varParam))
    {
        dlog(LOG_ERROR, "传入图片或抠像参数为空");
        return false;
    }

    if (!m_pInference)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return false;
    }

    bool bRet = true;

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

    AiScenario_NS::CutoutParam_S& stCutoutParam = std::get<AiScenario_NS::CutoutParam_S>(stInData.varParam);
    stCutoutParam.aInputImage = stInData.inMat;
    /* 推理+后处理 */
    try
    {
        bRet = m_pInference->inference(std::get<AiScenario_NS::CutoutParam_S>(stInData.varParam));
        if (bRet < 0) 
        {
            dlog(LOG_ERROR, "算法分析失败");
            goto EXIT;
        }
    }
    catch(const std::exception& e)
    {
        bRet = -1;
        dlog(LOG_ERROR, "算法分析异常");
        goto EXIT;
    }
    
    
    

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存图片 */
        if (!std::get<AiScenario_NS::CutoutParam_S>(stInData.varParam).aOutputImage.empty() && !m_stInParam.stExParam.strAnalyzeDataPath.empty())
        {
            if (!saveImage(std::get<AiScenario_NS::CutoutParam_S>(stInData.varParam).aOutputImage, m_stInParam.stExParam.strAnalyzeDataPath.c_str()))
            {
                dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.stExParam.strAnalyzeDataPath.c_str());
            }
        }
    }

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 JSON格式返回");
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
            bRet = convertToPic(std::get<AiScenario_NS::CutoutParam_S>(stInData.varParam).aOutputImage, &pchOutData, nDataSize);
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
bool Scenario_NS::CVirtualCutoutV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CVirtualCutoutV1_0::releaseData(char*& pchOutData)
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
            dlog(LOG_ERROR, "该算法场景为实现 JSON格式返回");
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
            free(pchOutData);
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

bool Scenario_NS::CVirtualCutoutV1_0::convertToPic(
    cv::Mat aInputImage,
    char**  pchOutData,
    int&    nDataSize)
{
    if(aInputImage.empty() || pchOutData == nullptr)
    {
        dlog(LOG_ERROR,"参数错误!");
        return false;
    }
    /* 计算图像数据总大小 */
    nDataSize = aInputImage.total() * aInputImage.elemSize();
    /* 分配足够的内存来存储图像数据 */
    *pchOutData = (char*)malloc(nDataSize);
    /* 确保内存分配成功 */
    if(*pchOutData == nullptr)
    {
        dlog(LOG_ERROR,"分配空间失败[%d]!", nDataSize);
        return false;
    }

    /* 拷贝 */
    memcpy(*pchOutData, aInputImage.data, nDataSize);

    return true;
}
