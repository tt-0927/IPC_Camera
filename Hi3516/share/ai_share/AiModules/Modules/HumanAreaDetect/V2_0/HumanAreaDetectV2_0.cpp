/*
 * @FilePath     : HumanAreaDetectV2_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-28 11:48:50
 * @Description  : 人少场景
 */
#include "HumanAreaDetectV2_0.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"


using namespace HumanAreaDetect_NS;

HumanAreaDetect_NS::CHumanAreaDetectV2_0::CHumanAreaDetectV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

HumanAreaDetect_NS::CHumanAreaDetectV2_0::~CHumanAreaDetectV2_0()
{
    unInit();
}

/* 初始化 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::init()
{
    CStatisticsTimer runTime("边界检测初始化耗时");
    bool bRet = false;

    m_pYolov5 = new Inference_NS::CYolov5(m_stInParam.strModelPath);
    if (m_pYolov5 && m_pYolov5->init())
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n", m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    m_pByteTracker = new Inference_NS::cBYTETracker();
    if (m_pByteTracker)
    {
        m_pByteTracker->setValue(m_fTrackThresh, m_fHighThresh, m_fMatchThresh, m_nFrameId, m_nMaxTimeLost);
        bRet = true;
    }

    if (!bRet)
    {
        printf("跟踪算法初始化失败\n");
        goto FAIL;
    }

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::unInit()
{
    CStatisticsTimer runTime("边界检测反初始化耗时");
    if (m_pYolov5)
    {
        delete m_pYolov5;
        m_pYolov5 = nullptr;
    }

    if (m_pByteTracker)
    {
        delete m_pByteTracker;
        m_pByteTracker = nullptr;
    }

    return true;
}

/* 处理数据 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    OutData_S *stOutData)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYolov5 || !m_pByteTracker)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = m_pYolov5->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
        return false;
    }
    
    if (m_stInParam.bDebug && !stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
    {
        if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
        {
            printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
        }
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

    /* 推理+后处理 */
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bRet = m_pYolov5->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    /* 跟踪算法 */
    std::vector<DetectResult_S> vecBoxs;
    for (const auto& box : vBoxDatas)
    {
        DetectResult_S result;
        result.fConfidence = box.fConfidence;
        result.nClassId = box.nLabel;
        result.vfBox = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1, 
                                       box.stBoxs.nX2 - box.stBoxs.nX1, 
                                       box.stBoxs.nY2 - box.stBoxs.nY1);
        vecBoxs.push_back(result);

        Result_S stResult;
        stResult.nId = 0;
        stResult.fX = (float)box.stBoxs.nX1;
        stResult.fY = (float)box.stBoxs.nY1;
        stResult.fWidth = (float)box.stBoxs.nX2 - box.stBoxs.nX1;
        stResult.fHeight = (float)box.stBoxs.nY2 - box.stBoxs.nY1;
        // vecResult.push_back(stResult);
    }

    /* 跟踪算法+区域分析 */
    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        std::vector<cSTrack> vecStracks = m_pByteTracker->update(vecBoxs);

        /* 分析结果 */
        /* 进行区域判断 */
        for (int nIndex = 0; nIndex < (int)vecStracks.size(); nIndex++)
        {
            int nPersonId = vecStracks[nIndex].track_id;
            std::vector<float> vectlwh = vecStracks[nIndex].tlwh;
            if (vectlwh.size() < 4)
            {
                continue;
            }

            cv::Point centerPoint(static_cast<int>(vectlwh[0] + (vectlwh[2] / 2)),
                                static_cast<int>(vectlwh[1] + (vectlwh[3] / 2)));

            /* 判断id在不在表里 */
            if (m_mapPenson.count(nPersonId))
            {
                m_mapPenson[nPersonId].startPoint = m_mapPenson[nPersonId].curPoint;
                m_mapPenson[nPersonId].curPoint = centerPoint;
                m_mapPenson[nPersonId].ndwellTime = 0;
                m_mapPenson[nPersonId].isUsed = true;
            }
            else
            {
                Penson_S newPenson;
                newPenson.nId = nPersonId;
                newPenson.startPoint = centerPoint;
                newPenson.curPoint = centerPoint;
                newPenson.ndwellTime = 0;
                newPenson.isUsed = true;

                m_mapPenson[nPersonId] = newPenson;
            }

            /* 区域分析 */
            Result_S stResult;
            
            stResult.fX = vectlwh[0];
            stResult.fY = vectlwh[1];
            stResult.fWidth = vectlwh[2];
            stResult.fHeight = vectlwh[3];

                /* 越界检测 */
            if (stInData.stParam.stTripLineParam.bEnable)
            {
                stResult.enTripLineType = tripLineDetection(
                    stResult,
                    stInData.stParam.stTripLineParam.alertLine1,
                    stInData.stParam.stTripLineParam.alertLine2);
            }

            /* 入侵检测 */
            if (stInData.stParam.stIntrusionParam.bEnable)
            {
                stResult.bIntrusionFlag = intrusionZoneDetection(
                    centerPoint,
                    stInData.stParam.stIntrusionParam.vecPoints);
            }

            /* 进入检测 */
            if (stInData.stParam.stEntryParam.bEnable)
            {
                stResult.bEntryFlag = entryZoneDetection(
                    m_mapPenson[nPersonId].startPoint,
                    centerPoint,
                    stInData.stParam.stEntryParam.vecPoints);
            }

            /* 离开检测 */
            if (stInData.stParam.stLeaveParam.bEnable)
            {
                stResult.bLeaveFlag = leaveZoneDetection(
                    m_mapPenson[nPersonId].startPoint,
                    centerPoint,
                    stInData.stParam.stLeaveParam.vecPoints);
            }

            stResult.nId = nPersonId;
            

            vecResult.push_back(stResult);
        }

        for (auto pair = m_mapPenson.begin(); pair != m_mapPenson.end();)
        {
            int id = pair->first;
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
                pair = m_mapPenson.erase(pair);
            }
            else
            {
                ++pair;
            }
        }

        if (vecStracks.size() > 0)
        {
            int enType = 0;
            const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
            const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;

            for (auto &result : vecResult)
            {
                if (result.enTripLineType != 0)
                {
                    enType |= 0x01; // Type_E::OVERSHOOT;
                }

                if (result.bIntrusionFlag)
                {
                    enType |= 0x02; // Type_E::INTRUSION;
                }

                if (result.bEntryFlag)
                {
                    enType |= 0x04; // Type_E::ENTRY;
                }

                if (result.bLeaveFlag)
                {
                    enType |= 0x08; // Type_E::EXIT;
                }

                result.fX *= scaleX;
                result.fY *= scaleY;
                result.fWidth *= scaleX;
                result.fHeight *= scaleY;
            }

            stOutData->nChnId = stInData.nChnId;

            if (enType != 0)
            {
                stOutData->validResult = true;
                stOutData->nType = enType;
            }
        }
    }
    return true;
}

/* 处理数据 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // std::cout << "imageWidth：" << imageWidth << std::endl;
    // std::cout << "imageHeight：" << imageHeight << std::endl;
    // std::cout << "m_nLimitWidth：" << m_nLimitWidth << std::endl;
    // std::cout << "m_fResizeScale" << m_fResizeScale << std::endl;

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}

bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    int nBottom = m_nLimitHeight - newHeight - m_nYOffset;
    int mRight = m_nLimitWidth - newWidth - m_nXOffset;

    cv::copyMakeBorder(resizedImage, outputImage,
                       m_nYOffset, nBottom, m_nXOffset, mRight, 
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;
}

/* 拌线检测：判断两条线段是否有交点 */
TripLineType_E HumanAreaDetect_NS::CHumanAreaDetectV2_0::tripLineDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond)
{
    TripLineType_E enType = OVERFLOW_NONE;
    
    if (startPoint.x == lastPoint.x && startPoint.y == lastPoint.y) return enType;

    // if (!isBoundingBoxIntersecting(startPoint, lastPoint, alertLineFirst, alertLineSecond))
    // {
    //     return enType;
    // }

    cv::Point lineStartPoint;
    cv::Point lineEndPoint;

    /* 判断线段的情况 */
    if (alertLineFirst.x == alertLineSecond.x)
    {
        if (alertLineFirst.y < alertLineSecond.y)
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint = alertLineFirst;
        }
        else
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint = alertLineSecond;
        }
    }
    else
    {
        if (alertLineFirst.x < alertLineSecond.x)
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint = alertLineSecond;
        }
        else
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint = alertLineFirst;
        }
    }

    /* 计算叉积 */
    int crossStartPoint = crossProduct(lineStartPoint, lineEndPoint, startPoint);
    int crossLastPoint = crossProduct(lineStartPoint, lineEndPoint, lastPoint);

    /* 判断A -> B */
    if (crossStartPoint < 0 && crossLastPoint > 0)
    {
        /* 拌线方向是A -> B */
        enType = OVERFLOW_A_TO_B;
        //std::cout << "拌线方向是A -> B" << std::endl;
    }

    /* 判断B -> A */
    if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        /* 拌线方向是B -> A */
        enType = OVERFLOW_B_TO_A;
        //std::cout << "拌线方向是B -> A" << std::endl;
    }

    /* 判断A <-> B */
    // if ((crossStartPoint * crossLastPoint) < 0)
    // {
    //     /* 拌线方向是A <-> B的其中一种 */
    //     enType = OVERFLOW_A_B_BOTH;
    //     //std::cout << "拌线方向是A <-> B的其中一种" << std::endl;
    // }

    return enType;
}
/* 判断人形区域与拌线是否有交集 */
TripLineType_E HumanAreaDetect_NS::CHumanAreaDetectV2_0::tripLineDetection(
    Result_S& stResult,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond)
{
    cv::Rect rect(stResult.fX,stResult.fY,stResult.fWidth,stResult.fHeight);
    if (rect.contains(alertLineFirst) || rect.contains(alertLineSecond)) 
    {
        return OVERFLOW_A_B_BOTH;
    }

    cv::Point clippedStart = alertLineFirst;
    cv::Point clippedEnd = alertLineSecond;
    bool bRes = cv::clipLine(rect, clippedStart, clippedEnd);
    return bRes?OVERFLOW_A_B_BOTH : OVERFLOW_NONE;
}

bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::isBoundingBoxIntersecting(
    const cv::Point &lineA1,
    const cv::Point &lineA2,
    const cv::Point &lineB1,
    const cv::Point &lineB2)
{
    return std::max(lineA1.x, lineA2.x) >= std::min(lineB1.x, lineB2.x) &&
           std::max(lineB1.x, lineB2.x) >= std::min(lineA1.x, lineA2.x) &&
           std::max(lineA1.y, lineA2.y) >= std::min(lineB1.y, lineB2.y) &&
           std::max(lineB1.y, lineB2.y) >= std::min(lineA1.y, lineA2.y);
}

int HumanAreaDetect_NS::CHumanAreaDetectV2_0::crossProduct(
    const cv::Point &alertLineStart,
    const cv::Point &alertLineEnd,
    const cv::Point &testPoint)
{
    if (alertLineStart.y == alertLineEnd.y) return testPoint.y - alertLineStart.y;
    if (alertLineStart.x == alertLineEnd.x) return alertLineStart.x - testPoint.x;
    return (alertLineEnd.x - alertLineStart.x) * (testPoint.y - alertLineStart.y) - 
           (alertLineEnd.y - alertLineStart.y) * (testPoint.x - alertLineStart.x);
}

/* 入侵检测：判断禁止入侵的区域有无点 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::intrusionZoneDetection(
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double intrusionResult = cv::pointPolygonTest(polygons, lastPoint, false);
    return intrusionResult >= 0;
}

/* 进入检测：根据起始点和当前点的关系，判断是否进入 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::entryZoneDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double StartResult = cv::pointPolygonTest(polygons, startPoint, false);
    if (StartResult >= 0) return false;
    double LastResult = cv::pointPolygonTest(polygons, lastPoint, false);

    return LastResult >= 0;
}

/* 离开检测：根据起始点和当前点的关系，判断是否离开 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::leaveZoneDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double StartResult = cv::pointPolygonTest(polygons, startPoint, false);
    if (StartResult < 0) return false;
    double LastResult = cv::pointPolygonTest(polygons, lastPoint, false);

    return LastResult < 0;
}

/* 判断两个多边形是否有交点 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    return !intersectionPoints.empty();
}

/* 判断目标框与线段是否有交点 */
bool HumanAreaDetect_NS::CHumanAreaDetectV2_0::isLineIntersectingRect(
    const cv::Point &topLeft,
    const cv::Point &bottomRight, 
    const cv::Point &alertLineFirst, 
    const cv::Point &alertLineSecond)
{
    cv::Rect cBoxRect(topLeft, bottomRight);
    cv::Point clippedStart = alertLineFirst;
    cv::Point clippedEnd = alertLineSecond;
    return cv::clipLine(cBoxRect, clippedStart, clippedEnd);
}
