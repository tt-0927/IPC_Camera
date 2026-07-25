/*
 * @FilePath     : HumanAreaDetect.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-18 19:14:00
 * @Description  : 人少场景
 */

#include "HumanAreaDetect.hpp"

#include "BYTETracker.h"
#include "dlog.h"
#include "HumanDetect.hpp"
#include "JsonInterfase.h"


/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6


using namespace Scenario_NS;

Scenario_NS::CHumanAreaDetect::CHumanAreaDetect(AiScenario_NS::InParam_S stInParam)
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

Scenario_NS::CHumanAreaDetect::~CHumanAreaDetect()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CHumanAreaDetect::init()
{
    bool bRet = false;

    if (m_stInParam.stNeedParam.vstrModelPath.size() > 0)
    {
        bRet = false;

        m_pInference = new InferenceV1_0_NS::CHumanDetect(
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

        ByteTracker = new cBYTETracker();
        ByteTracker->setValue(m_fTrackThresh, m_fHighThresh, m_fMatchThresh, m_nFrameId, m_nMaxTimeLost);
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool Scenario_NS::CHumanAreaDetect::unInit()
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
bool Scenario_NS::CHumanAreaDetect::process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize)
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
        // try
        // {
        //     cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        // }
        // catch (const cv::Exception& e)
        // {
        //     dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
        //     return false;
        // }
        resizeAndPadImage(stInData.inMat, stInData.inMat);

        std::cout << "图片等比例缩放后：" << stInData.inMat.cols << "x" << stInData.inMat.rows << std::endl;
        std::cout << "图片等比例缩放完成！！" << std::endl;
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

    /* 坐标转换 */
    std::vector<DetectResult_S> vecBoxs;
    vecBoxs.clear();
    for (int nIndex = 0; nIndex < vecPos.size(); nIndex++)
    {
        if (vecPos.size() >= ((nIndex + 1) * DATA_GROUP_SIZE))
        {
            DetectResult_S Box;
            float          x1 = (vecPos[nIndex * DATA_GROUP_SIZE + 0] - m_nXOffset) / m_fResizeScale;
            float          y1 = (vecPos[nIndex * DATA_GROUP_SIZE + 1] - m_nYOffset) / m_fResizeScale;
            float          x2 = (vecPos[nIndex * DATA_GROUP_SIZE + 2] - m_nXOffset) / m_fResizeScale;
            float          y2 = (vecPos[nIndex * DATA_GROUP_SIZE + 3] - m_nYOffset) / m_fResizeScale;
            Box.fConfidence   = vecPos[nIndex * DATA_GROUP_SIZE + 4];
            Box.nClassId      = (int)vecPos[nIndex * DATA_GROUP_SIZE + 4];
            Box.vfBox         = cv::Rect_<float> { x1, y1, x2 - x1, y2 - y1 };
            vecBoxs.push_back(Box);
        }
    }

    std::cout << "推理结果的数据长度：" << vecBoxs.size() << std::endl;

    /* 跟踪算法 */
    std::vector<cSTrack> vecStracks = ByteTracker->update(vecBoxs);

    std::cout << "跟踪结果的数据长度：" << vecStracks.size() << std::endl;

    /* 进行区域判断 */
    for (int nIndex = 0; nIndex < vecStracks.size(); nIndex++)
    {
        int                n_personId = vecStracks[nIndex].track_id;
        std::vector<float> vectlwh    = vecStracks[nIndex].tlwh;
        cv::Point          centerPoint(static_cast<int>(vectlwh[0] + (vectlwh[2] / 2)),
                                       static_cast<int>(vectlwh[1] + (vectlwh[3] / 2)));
        /* 判断id在不在表里 */
        if (m_mapPenson.count(n_personId))
        {
            m_mapPenson[n_personId].ndwellTime = 0;
            m_mapPenson[n_personId].isUsed     = true;
        }
        else
        {
            Penson_S newPenson;
            newPenson.nId        = n_personId;
            newPenson.startPoint = centerPoint;
            newPenson.ndwellTime = 0;
            newPenson.isUsed     = true;

            m_mapPenson[n_personId] = newPenson;
        }
    }


    for (const auto& pair : m_mapPenson)
    {
        int id = pair.first;
        if (m_mapPenson[id].isUsed)
        {
            m_mapPenson[id].isUsed = false;
        }
        else
        {
            m_mapPenson[id].ndwellTime++;
        }

        if (m_mapPenson[id].ndwellTime >= m_nMaxTimeLost)
        {
            size_t erasedCount = m_mapPenson.erase(id);
            if (erasedCount > 0)
            {
                std::cout << "成功删除 id: " << id << std::endl;
            }
        }
    }

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存分析后的图片 */
        if (!stInData.inMat.empty() && !m_stInParam.stExParam.strAnalyzeDataPath.empty())
        {

            for (int nIndex = 0; nIndex < vecStracks.size(); nIndex++)
            {
                std::vector<float> vectlwh = vecStracks[nIndex].tlwh;
                /* 框 */
                cv::rectangle(
                    stInData.inMat,
                    cv::Rect(vectlwh[0], vectlwh[1], vectlwh[2], vectlwh[3]),
                    cv::Scalar(0, 0, 255),
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
            bRet = convertToJson(vecStracks, &pchOutData, nDataSize);
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
bool Scenario_NS::CHumanAreaDetect::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CHumanAreaDetect::releaseData(char*& pchOutData)
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

bool Scenario_NS::CHumanAreaDetect::convertToJson(
    std::vector<cSTrack> vPointsXY,
    char**               pchOutData,
    int&                 nDataSize)
{
    auto pRootJson      = Json::init();
    auto pArrayDataJson = Json::Array::init();

    for (int nIndex = 0; nIndex < vPointsXY.size(); nIndex++)
    {
        auto               pArrayBox = Json::Array::init();
        auto               pItem     = Json::init();
        std::vector<float> vPtlwh    = vPointsXY[nIndex].tlwh;
        Json::Array::add(pArrayBox, (int)vPtlwh[0]);
        Json::Array::add(pArrayBox, (int)vPtlwh[1]);
        Json::Array::add(pArrayBox, static_cast<int>(vPtlwh[0] + vPtlwh[2]));
        Json::Array::add(pArrayBox, static_cast<int>(vPtlwh[1] + vPtlwh[3]));
        Json::add(pItem, "Box", pArrayBox);
        Json::add(pItem, "ID", vPointsXY[nIndex].track_id);
        Json::Array::add(pArrayDataJson, pItem);
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

/* 处理数据 */
bool Scenario_NS::CHumanAreaDetect::resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage)
{
    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale  = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // std::cout << "imageWidth：" << imageWidth << std::endl;
    // std::cout << "imageHeight：" << imageHeight << std::endl;
    // std::cout << "m_nLimitWidth：" << m_nLimitWidth << std::endl;
    // std::cout << "m_nLimitWidth：" << m_fResizeScale << std::endl;

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    try
    {
        cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));
    }
    catch (const cv::Exception& e)
    {
        dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
        return false;
    }

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}

bool Scenario_NS::CHumanAreaDetect::tripLineDetection(
    const cv::Point& lineA1,
    const cv::Point& lineA2,
    const cv::Point& lineB1,
    const cv::Point& lineB2)
{
    if (!isBoundingBoxIntersecting(lineA1, lineA2, lineB1, lineB2))
    {
        return false;
    }
    /* 计算叉积 */
    int d1 = (lineA2.x - lineA1.x) * (lineB1.y - lineA1.y) - (lineA2.y - lineA1.y) * (lineB1.x - lineA1.x);
    int d2 = (lineA2.x - lineA1.x) * (lineB2.y - lineA1.y) - (lineA2.y - lineA1.y) * (lineB2.x - lineA1.x);
    int d3 = (lineB2.x - lineB1.x) * (lineA1.y - lineB1.y) - (lineB2.y - lineB1.y) * (lineA1.x - lineB1.x);
    int d4 = (lineB2.x - lineB1.x) * (lineA2.y - lineB1.y) - (lineB2.y - lineB1.y) * (lineA2.x - lineB1.x);

    /* 如果叉积符号相反，说明线段相交 */
    if ((d1 * d2 < 0) && (d3 * d4 < 0))
    {
        return true;
    }
    /* 如果叉积为 0，进一步判断端点是否在线段上 */
    if (d1 == 0 && isPointOnSegment(lineA1, lineA2, lineB1))
    {
        return true;    // lineB1 在 lineA 上
    }
    if (d2 == 0 && isPointOnSegment(lineA1, lineA2, lineB2))
    {
        return true;    // lineB2 在 lineA 上
    }
    if (d3 == 0 && isPointOnSegment(lineB1, lineB2, lineA1))
    {
        return true;    // lineA1 在 lineB 上
    }
    if (d4 == 0 && isPointOnSegment(lineB1, lineB2, lineA2))
    {
        return true;    // lineA2 在 lineB 上
    }

    return false;    // 无交点
}

bool Scenario_NS::CHumanAreaDetect::isBoundingBoxIntersecting(
    const cv::Point& lineA1,
    const cv::Point& lineA2,
    const cv::Point& lineB1,
    const cv::Point& lineB2)
{
    return std::max(lineA1.x, lineA2.x) >= std::min(lineB1.x, lineB2.x) &&
        std::max(lineB1.x, lineB2.x) >= std::min(lineA1.x, lineA2.x) &&
        std::max(lineA1.y, lineA2.y) >= std::min(lineB1.y, lineB2.y) &&
        std::max(lineB1.y, lineB2.y) >= std::min(lineA1.y, lineA2.y);
}

bool Scenario_NS::CHumanAreaDetect::isPointOnSegment(
    const cv::Point& line1,
    const cv::Point& line2,
    const cv::Point& testPoint)
{
    return std::min(line1.x, line2.x) <= testPoint.x &&
        testPoint.x <= std::max(line1.x, line2.x) &&
        std::min(line1.y, line2.y) <= testPoint.y &&
        testPoint.y <= std::max(line1.y, line2.y);
}

bool Scenario_NS::CHumanAreaDetect::intrusionZoneDetection(
    const cv::Point&       LastPoint,
    std::vector<cv::Point> Polygons)
{
    double intrusionResult = cv::pointPolygonTest(Polygons, LastPoint, false);
    if (intrusionResult >= 0)
    {
        return true;    // 入侵
    }
    else
    {
        return false;    // 未入侵
    }
}

bool Scenario_NS::CHumanAreaDetect::entryZoneDetection(
    const cv::Point&       StartPoint,
    const cv::Point&       LastPoint,
    std::vector<cv::Point> Polygons)
{
    double StartResult = cv::pointPolygonTest(Polygons, StartPoint, false);
    if (StartResult >= 0)
    {
        return false;
    }
    else
    {
        double LastResult = cv::pointPolygonTest(Polygons, LastPoint, false);
        if (LastResult >= 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

bool Scenario_NS::CHumanAreaDetect::leaveZoneDetection(
    const cv::Point&       StartPoint,
    const cv::Point&       LastPoint,
    std::vector<cv::Point> Polygons)
{
    double StartResult = cv::pointPolygonTest(Polygons, StartPoint, false);
    if (StartResult >= 0)
    {
        double LastResult = cv::pointPolygonTest(Polygons, LastPoint, false);
        if (LastResult >= 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}
