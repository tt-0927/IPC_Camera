/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-27 17:24:46
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-09 15:57:18
 * @FilePath: /1126/share/ai_share/AiModules/Modules/VehicleDetect/V2_0/VehicleDetectV2_0.cpp
 * @Description: 车辆检测
 */

#include "VehicleDetectV2_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6
using namespace VehicleDetect_NS;

VehicleDetect_NS::CVehicleDetectV2_0::CVehicleDetectV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

VehicleDetect_NS::CVehicleDetectV2_0::~CVehicleDetectV2_0()
{
    unInit();
}

/* 初始化 */
bool VehicleDetect_NS::CVehicleDetectV2_0::init()
{
    CStatisticsTimer runTime("停车检测初始化耗时");
    bool bRet = false;

    m_pYoloUltralytics = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pYoloUltralytics && m_pYoloUltralytics->init())
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
bool VehicleDetect_NS::CVehicleDetectV2_0::unInit()
{
    CStatisticsTimer runTime("停车检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    if (m_pByteTracker)
    {
        delete m_pByteTracker;
        m_pByteTracker = nullptr;
    }

    return true;
}

/* 处理数据 */
bool VehicleDetect_NS::CVehicleDetectV2_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    // std::vector<Result_S> &vecResult1,
    OutData_S *stOutData)
{
    OutData_S defaultOutData;

    /* 如果传入的指针为空，则使用默认对象 */
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    /* 初始化输出结果 */
    stOutData->validResult = false;
    stOutData->nType = -1;
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralytics || !m_pByteTracker)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = m_pYoloUltralytics->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
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
    
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    /* ================= 拥堵识别检测 ================= */
    int nVehicleCount = 0;
    bool bCongestion = false;
    for(auto &BoxData : vBoxDatas)
    {
        if(BoxData.fConfidence >= stInData.stParam.stCongestionParam.fCongestionBoxThreshold)
        {
            nVehicleCount++;
        }
    }
    if(nVehicleCount >= stInData.stParam.stCongestionParam.nCongestionThreshold)
    {
        bCongestion = true;
    }
    /* ================= 拥堵识别检测 ================= */

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
        // vecResult1.push_back(stResult);
        // printf(" ======== result.fConfidence = %f ======== \n", result.fConfidence);
    }
    /* 跟踪算法+区域分析 */
    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        std::vector<cSTrack> vecStracks = m_pByteTracker->update(vecBoxs);

        for (int nIndex = 0; nIndex < (int)vecStracks.size(); nIndex++)
        {
            int nVehicleId = vecStracks[nIndex].track_id;
            std::vector<float> vectlwh = vecStracks[nIndex].tlwh;
            float fBoxConfidence = vecStracks[nIndex].score;
            // printf(" ========== vecStracks[nIndex].score = %f ========== \n", fBoxConfidence);
            
            if (vectlwh.size() < 4)
            {
                continue;
            }
            cv::Point centerPoint(
                static_cast<int>(vectlwh[0] + (vectlwh[2] / 2)),
                static_cast<int>(vectlwh[1] + (vectlwh[3] / 2))); /* 目标中心点 */
            /* 检查目标是否已经在跟踪列表中 */
            if (m_mapVehicle.count(nVehicleId))
            {
                /* 更新目标的位置信息和状态 */
                m_mapVehicle[nVehicleId].startPoint = m_mapVehicle[nVehicleId].curPoint;
                m_mapVehicle[nVehicleId].curPoint = centerPoint;
                m_mapVehicle[nVehicleId].ndwellTime = 0;
                m_mapVehicle[nVehicleId].isUsed = true; /* 标记为当前帧检测到 */
            }
            else
            {
                /* 新目标，添加到跟踪列表 */
                Vehicle_S newVehicle;
                newVehicle.nId = nVehicleId;
                newVehicle.startPoint = centerPoint;
                newVehicle.curPoint = centerPoint;
                newVehicle.ndwellTime = 0;
                newVehicle.isUsed = true;  /* 标记为当前帧检测到 */
                m_mapVehicle[nVehicleId] = newVehicle;
            }
            /* 区域分析 */
            Result_S stResult;

            /* 违规变道检测区域 */
            // int i = 0;
            for (auto &Param : stInData.stParam.vstIllegalLaneChangeParam)
            {
                // i++;
                if (Param.bEnable)
                {
                    if(fBoxConfidence < Param.fIllegalLaneChangeBoxThreshold)
                    {
                        // printf(" ======= 框的置信度 = %f < 置信度阈值 = %f ======= %d \n", fBoxConfidence, Param.fIllegalLaneChangeBoxThreshold, __LINE__);
                        continue;
                    }
                    stResult.bIllegalLaneChangeFlag = doLinesIntersect(
                        m_mapVehicle[nVehicleId].startPoint ,centerPoint,
                        Param.alertLine1, Param.alertLine2);

                    if (stResult.bIllegalLaneChangeFlag)
                    {
                        // printf("区域 %d 触发违规变道\n", i);
                        break;
                    }
                    
                }
            }

            // i = 0;
            /* 逆行检测区域 */
            for (auto &Param : stInData.stParam.vstDrivingAgainstTrafficParam)
            {
                // i++;
                if (Param.bEnable)
                {
                    if(fBoxConfidence < Param.fDrivingAgainstTrafficBoxThreshold)
                    {
                        continue;
                    }
                    TripLineType_E enTripLineType = OVERFLOW_NONE;
                    enTripLineType = tripLineDetection(
                            m_mapVehicle[nVehicleId].startPoint ,centerPoint,
                            Param.alertLine1,
                            Param.alertLine2);
                            

                    /* 检测结果不为空，且不符合预设方向则判定为逆行 */
                    if(enTripLineType != OVERFLOW_NONE && enTripLineType != Param.eTripLineType)
                    {
                        stResult.bDrivingAgainstTrafficFlag = true;
                        // printf(" 区域 %d 触发逆行 当前帧车辆框中点(%d, %d)\n", i, m_mapVehicle[nVehicleId].curPoint.x, m_mapVehicle[nVehicleId].curPoint.y);
                        if(m_stInParam.bDebug)
                        {
                            /* 框 */
                            cv::rectangle(
                                stInData.inMat,
                                cv::Rect(vectlwh[0], vectlwh[1], vectlwh[2], vectlwh[3]),
                                cv::Scalar(0, 0, 255),
                                4);
                            
                            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strAnalyzeDataPath))
                            {
                                printf("Debug-保存图片失败[%s]\n", m_stInParam.strAnalyzeDataPath.c_str());
                            }
                        }

                        break;
                    }
                }
            }

            /* 停车检测区域 */
            // i = 0;
            for (auto &Param : stInData.stParam.vstParkingParam)
            {
                // i++;
                if (Param.bEnable)
                {
                    if(fBoxConfidence < Param.fParkingBoxThreshold)
                    {
                        // printf(" ===== 框的置信度 = %f < 置信度阈值 = %f===== %d \n", fBoxConfidence, Param.fParkingBoxThreshold, __LINE__);
                        continue;
                    }
                    bool nRet = intrusionZoneDetection(m_mapVehicle[nVehicleId].curPoint, Param.vecPoints);
                    if(nRet) /* 进入检测区域 */
                    {
                        if(!m_mapVehicle[nVehicleId].stParking.bParking) /* 记录第一次进入检测区域 */
                        {
                            // printf(" =====区域 %d 出现违停 =======%d \n", i, Param.nParkingTimeThreshold);
                            m_mapVehicle[nVehicleId].stParking.bParking = true;
                            m_mapVehicle[nVehicleId].stParking.nParkingTimeStamp = getSteadyTimeStampMs();
                        }
                        else
                        {
                            /* 判断从进入检测停车区域到当前持续时间是否超过设定阈值 */
                            if(getSteadyTimeStampMs() - m_mapVehicle[nVehicleId].stParking.nParkingTimeStamp >= Param.nParkingTimeThreshold)
                            {
                                stResult.bParkingFlag = true;
                                // printf(" =====区域 %d 达到违停的阈值 =======%d \n", i, Param.nParkingTimeThreshold);
                            }   
                        }
                    }
                    else /* 目标离开检测区域，重置 */
                    {
                        m_mapVehicle[nVehicleId].stParking.bParking = false;
                        m_mapVehicle[nVehicleId].stParking.nParkingTimeStamp = getSteadyTimeStampMs();
                    }
                    
                    if(stResult.bParkingFlag == true)
                    {
                        break;
                    }
                    
                }
            }

            stResult.nId = nVehicleId;
            stResult.fX = vectlwh[0];
            stResult.fY = vectlwh[1];
            stResult.fWidth = vectlwh[2];
            stResult.fHeight = vectlwh[3];

            vecResult.push_back(stResult);
        }

        /* 清除丢失的目标 */
        for (auto it = m_mapVehicle.begin(); it != m_mapVehicle.end();)
        {
            int nVehicleId = it->first;
            if (!m_mapVehicle[nVehicleId].isUsed)
            {
                m_mapVehicle[nVehicleId].ndwellTime++; /* 增加丢失时间 */
                if (m_mapVehicle[nVehicleId].ndwellTime > m_nMaxTimeLost)
                {
                    it = m_mapVehicle.erase(it); /* 从跟踪列表中移除 */
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                m_mapVehicle[nVehicleId].isUsed = false; /* 重置使用状态 */
                ++it;
            }
        }

        int enType = 0;
        if (vecStracks.size() > 0)
        {
            
            const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
            const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;

            for (auto &result : vecResult)
            {
                if (result.bDrivingAgainstTrafficFlag != 0)
                {
                    enType |= 0x01; // Type_E::REVERSE_DIRECTION;
                }

                if (result.bParkingFlag)
                {
                    enType |= 0x04; // Type_E::ILLEGAL_PARKING;
                }

                if (result.bIllegalLaneChangeFlag)
                {
                    enType |= 0x08; // Type_E::ILLEGAL_LANE_CHANGE;
                }

                result.fX *= scaleX;
                result.fY *= scaleY;
                result.fWidth *= scaleX;
                result.fHeight *= scaleY;
            }

            stOutData->nChnId = stInData.nChnId;
        }

        /* 是否触发拥堵 */
        if(bCongestion)
        {
            enType |= 0x02; // Type_E::CONGESTION;
        }

        if (enType != 0)
        {
            stOutData->validResult = true;
        }
        stOutData->nType = enType;  
    }

    return true;
}

/* 处理数据 */
bool VehicleDetect_NS::CVehicleDetectV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}

bool VehicleDetect_NS::CVehicleDetectV2_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
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

/* 判断两个多边形是否有交点 */
bool VehicleDetect_NS::CVehicleDetectV2_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    int result = cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    if (intersectionPoints.empty())
    {
        return false;
    }
    else
    {
        return true;
    }
}

int VehicleDetect_NS::CVehicleDetectV2_0::crossProduct(cv::Point alertLineStart,
                 cv::Point alertLineEnd,
                 cv::Point testPoint)
{
    return (alertLineEnd.x - alertLineStart.x) * (testPoint.y - alertLineStart.y) -
              (alertLineEnd.y - alertLineStart.y) * (testPoint.x - alertLineStart.x);
}

bool VehicleDetect_NS::CVehicleDetectV2_0::isBoundingBoxIntersecting(
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

/* 判断是否跨越线段 */
bool VehicleDetect_NS::CVehicleDetectV2_0::doLinesIntersect(const cv::Point &p1,
                      const cv::Point &q1,
                      const cv::Point &p2,
                      const cv::Point &q2)
{
    int d1 = crossProduct(p2, q2, p1);
    int d2 = crossProduct(p2, q2, q1);
    int d3 = crossProduct(p1, q1, p2);
    int d4 = crossProduct(p1, q1, q2);
	
	/* 如果两条线段相交 */
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;

    /* 处理共线的特殊情况 */
    if (d1 == 0 && d2 == 0 && d3 == 0 && d4 == 0)
    {
        return isBoundingBoxIntersecting(p1, q1, p2, q2);
    }

    return false;
}

/* 拌线检测：判断两条线段是否有交点 */
TripLineType_E VehicleDetect_NS::CVehicleDetectV2_0::tripLineDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond)
{
    TripLineType_E enType = OVERFLOW_NONE;
    
    if (startPoint.x == lastPoint.x && startPoint.y == lastPoint.y) return enType;

    if (!isBoundingBoxIntersecting(startPoint, lastPoint, alertLineFirst, alertLineSecond))
    {
        return enType;
    }

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

    // printf(" ===== (%d, %d) (%d, %d) =======%d \n", startPoint.x, startPoint.y, lastPoint.x, lastPoint.y, __LINE__); 

    /* 判断A -> B */
    if (crossStartPoint < 0 && crossLastPoint > 0)
    {
        /* 拌线方向是A -> B */
        enType = OVERFLOW_A_TO_B;

        // std::cout << "拌线方向是A -> B :" << crossStartPoint << " " << crossLastPoint << std::endl;
    }

    /* 判断B -> A */
    if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        /* 拌线方向是B -> A */
        enType = OVERFLOW_B_TO_A;
        // std::cout << "拌线方向是B -> A " << crossStartPoint << " " << crossLastPoint << std::endl;
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

/* 入侵检测：判断禁止入侵的区域有无点 */
bool VehicleDetect_NS::CVehicleDetectV2_0::intrusionZoneDetection(
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double intrusionResult = cv::pointPolygonTest(polygons, lastPoint, false);
    return intrusionResult >= 0;
}