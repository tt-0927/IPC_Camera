/*
 * @FilePath     : ClassroomMoveDetectV1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-22 20:19:15
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-12 10:17:24
 * @Description  : 人少场景
 */

#include "ClassroomMoveDetectV1_0.hpp"

#include "dlog.h"
#include "HeadDetect.hpp"
#include "JsonInterfase.h"


/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6


using namespace Scenario_NS;

Scenario_NS::CClassroomMoveDetectV1_0::CClassroomMoveDetectV1_0(AiScenario_NS::InParam_S stInParam)
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

Scenario_NS::CClassroomMoveDetectV1_0::~CClassroomMoveDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CClassroomMoveDetectV1_0::init()
{
    bool bRet = false;

    if (m_stInParam.stNeedParam.vstrModelPath.size() > 0)
    {
        bRet = false;

        m_pInference = new InferenceV1_0_NS::CHeadDetect(
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
bool Scenario_NS::CClassroomMoveDetectV1_0::unInit()
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
bool Scenario_NS::CClassroomMoveDetectV1_0::process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize)
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
        return bRet;
    }
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        try
        {
            cv::resize(stInData.inMat, stInData.inMat,
                       cv::Size(m_nLimitWidth, m_nLimitHeight));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            return false;
        }
    }

    /* 推理+后处理 */
    bRet = m_pInference->inference(stInData, vecPos);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        return bRet;
    }

    if ((vecPos.size() % DATA_GROUP_SIZE) != 0)
    {
        dlog(LOG_ERROR, "算法推理结果异常");
        return bRet;
    }

    /* 混乱分析 */
    std::vector<std::vector<int>> v_nfOutputLocations;
    for (int nIndex = 0; nIndex < vecPos.size(); nIndex++)
    {
        if (vecPos.size() >= ((nIndex + 1) * DATA_GROUP_SIZE))
        {
            int x1 = static_cast<int>(vecPos[nIndex * DATA_GROUP_SIZE + 0]);
            int y1 = static_cast<int>(vecPos[nIndex * DATA_GROUP_SIZE + 1]);
            int x2 = static_cast<int>(vecPos[nIndex * DATA_GROUP_SIZE + 2]);
            int y2 = static_cast<int>(vecPos[nIndex * DATA_GROUP_SIZE + 3]);

            std::vector<int> v_nToAdd = { x1, y1, x2, y2 };
            v_nfOutputLocations.push_back(v_nToAdd);
        }
    }
    /* 判断上一帧目标框是否为空 */
    double fMoveResult = 0;
    if (!v_nForeboxs.empty())
    {
        fMoveResult = iou_filter(v_nfOutputLocations, v_nForeboxs, m_fIouFilterThreshold);
    }
    /* 替换上一帧的目标框 */
    v_nForeboxs = v_nfOutputLocations;


    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存分析后的图片 */
        if (!stInData.inMat.empty() && !m_stInParam.stExParam.strAnalyzeDataPath.empty())
        {

            for (int nIndex = 0; nIndex < vecPos.size(); nIndex++)
            {
                if (vecPos.size() >= ((nIndex + 1) * DATA_GROUP_SIZE))
                {
                    /* 框 */
                    cv::rectangle(
                        stInData.inMat,
                        cv::Point(vecPos[nIndex * DATA_GROUP_SIZE + 0],
                                  vecPos[nIndex * DATA_GROUP_SIZE + 1]),
                        cv::Point(vecPos[nIndex * DATA_GROUP_SIZE + 2],
                                  vecPos[nIndex * DATA_GROUP_SIZE + 3]),
                        cv::Scalar(0, 0, 255),
                        4);

                    if (!saveImage(stInData.inMat, m_stInParam.stExParam.strAnalyzeDataPath))
                    {
                        dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.stExParam.strAnalyzeDataPath.c_str());
                    }
                }
            }
        }
    }


    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = convertToJson(fMoveResult, &pchOutData, nDataSize);
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

/* 处理数据 */
bool Scenario_NS::CClassroomMoveDetectV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CClassroomMoveDetectV1_0::releaseData(char*& pchOutData)
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

bool Scenario_NS::CClassroomMoveDetectV1_0::convertToJson(
    double fResult,
    char** pchOutData,
    int&   nDataSize)
{
    auto pRootJson      = Json::init();
    auto pArrayDataJson = Json::Array::init();

    Json::add(pRootJson, "MoveProbability", fResult);

    *pchOutData = Json::print(pRootJson);

    nDataSize = strlen(Json::to_string(pRootJson).c_str());

    if (pRootJson)
    {
        /* 释放数据 */
        Json::deinit(pRootJson);
    }

    return true;
}

double Scenario_NS::CClassroomMoveDetectV1_0::CalculateOverlap(
    const std::vector<int>& box1,
    const std::vector<int>& box2)
{
    // 计算交集区域的宽和高
    int inter_width  = fmax(0, fmin(box1[2], box2[2]) - fmax(box1[0], box2[0]));
    int inter_height = fmax(0, fmin(box1[3], box2[3]) - fmax(box1[1], box2[1]));
    // 交集面积
    int inter_area   = inter_width * inter_height;
    // 计算两个框的面积
    int box1_area    = (box1[2] - box1[0]) * (box1[3] - box1[1]);
    int box2_area    = (box2[2] - box2[0]) * (box2[3] - box2[1]);
    // 并集面积
    int union_area   = box1_area + box2_area - inter_area;

    return (union_area != 0) ? static_cast<double>(inter_area) / union_area : 0.0;
}

double Scenario_NS::CClassroomMoveDetectV1_0::iou_filter(
    std::vector<std::vector<int>>& boxes1,
    std::vector<std::vector<int>>& boxes2,
    double                         IouFilterThreshold)
{
    std::cout << "====================== " << std::endl;
    int               size1 = boxes1.size();
    int               size2 = boxes2.size();
    // std::cout << "当前帧的人头数: " << size1 << std::endl;
    // std::cout << "上一帧的人头数:: " << size2 << std::endl;
    // 标记数组，用于记录哪些框已经被移除
    std::vector<bool> remove_flag1(size1, false);
    std::vector<bool> remove_flag2(size2, false);
    // 遍历两个列表的矩形框
    for (int i = 0; i < size1; ++i)
    {
        if (remove_flag1[i])
        {
            continue;    // 如果框已经被标记为移除，跳过
        }

        for (int j = 0; j < size2; ++j)
        {
            if (remove_flag2[j])
            {
                continue;    // 如果框已经被标记为移除，跳过
            }

            // 计算IOU
            double iou = CalculateOverlap(boxes1[i], boxes2[j]);
            // std::cout << "iou: " << iou << std::endl;

            if (iou > IouFilterThreshold)
            {
                // 如果IOU大于阈值，标记这两个框为移除
                remove_flag1[i] = true;
                remove_flag2[j] = true;
                break;    // 找到匹配后，跳出内层循环，避免不必要的计算
            }
        }
    }
    // 计算剩余未被移除的框数量
    int    remaining1       = std::count(remove_flag1.begin(), remove_flag1.end(), false);
    int    remaining2       = std::count(remove_flag2.begin(), remove_flag2.end(), false);
    // std::cout << "当前帧未被移除的框数量: " << remaining1 << std::endl;
    // std::cout << "上一帧未被移除的框数量: " << remaining2 << std::endl;
    // 返回过滤后的比例
    double fIouFilterReturn = static_cast<double>(remaining1 + remaining2) / (size1 + size2);
    // std::cout << "过滤后的比例: " << fIouFilterReturn << std::endl;

    return fIouFilterReturn;
}