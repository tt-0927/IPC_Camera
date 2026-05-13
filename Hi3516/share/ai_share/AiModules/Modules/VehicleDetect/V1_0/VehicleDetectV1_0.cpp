/**
 * @file VehicleDetectV1_0.cpp
 * @author songww
 * @date 2025-10-29
 * 
 * @brief 车辆检测
 */
#include "VehicleDetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>
/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6
using namespace VehicleDetect_NS;

VehicleDetect_NS::CVehicleDetectV1_0::CVehicleDetectV1_0(InParam_S stInParam1,InParam_S stInParam2)
    : m_stInParamPlate(stInParam1),m_stInParamVehicle(stInParam2)
{
}

VehicleDetect_NS::CVehicleDetectV1_0::~CVehicleDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool VehicleDetect_NS::CVehicleDetectV1_0::init()
{
    CStatisticsTimer runTime("车牌检测初始化耗时");
    bool bRet = false;

    m_pYoloUltralytics_plate = new Inference_NS::CYoloUltralytics(m_stInParamPlate.strModelPath);
    if (m_pYoloUltralytics_plate && m_pYoloUltralytics_plate->init())
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n", m_stInParamPlate.strModelPath.c_str());
        goto FAIL;
    }

    m_pYoloUltralytics_vehicle = new Inference_NS::CYoloUltralytics(m_stInParamVehicle.strModelPath);
    bRet = false;
    
    if (m_pYoloUltralytics_vehicle && m_pYoloUltralytics_vehicle->init())
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("车辆算法初始化失败\n");
        goto FAIL;
    }

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool VehicleDetect_NS::CVehicleDetectV1_0::unInit()
{
    CStatisticsTimer runTime("停车检测反初始化耗时");
    if (m_pYoloUltralytics_plate)
    {
        delete m_pYoloUltralytics_plate;
        m_pYoloUltralytics_plate = nullptr;
    }

    if (m_pYoloUltralytics_vehicle)
    {
        delete m_pYoloUltralytics_vehicle;
        m_pYoloUltralytics_vehicle = nullptr;
    }

    return true;
}


/* 处理数据 */
bool VehicleDetect_NS::CVehicleDetectV1_0::processPlate(
    InData_S stInData,
    std::vector<Result_S> &vecResult)
{
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralytics_plate)
    {
        printf("未初始化算法类\n");
        return false;
    }
    
    if (m_stInParamPlate.bDebug && !stInData.inMat.empty() && !m_stInParamPlate.strOriginalDataPath.empty())
    {
        if (!Modules_NS::saveImage(stInData.inMat, m_stInParamPlate.strOriginalDataPath))
        {
            printf("Debug-保存图片失败[%s]\n", m_stInParamPlate.strOriginalDataPath.c_str());
        }
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    
    #ifdef RK_3588
    cv::Mat reMat;
    resizeAndPadImage(stInData.inMat,reMat);
    stInputData.pData = (float*)reMat.data;
    stInputData.nDataSize = static_cast<size_t>(reMat.total() * reMat.elemSize()) * sizeof(float);
    #else
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    #endif
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;
    
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        int bRet = m_pYoloUltralytics_plate->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }
    const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
    #ifdef RK_3588
    const float scaleY = static_cast<float>(stInData.inMat.rows) / m_nLimitHeight;
    #else
    const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;
    #endif

    for (const auto& box : vBoxDatas)
    {
        Result_S stResult;
        stResult.nId = 0;
        stResult.fX = (float)box.stBoxs.nX1 * scaleX;
        stResult.fY = (float)box.stBoxs.nY1 * scaleY;
        stResult.fWidth = (float)(box.stBoxs.nX2 - box.stBoxs.nX1) * scaleX;
        stResult.fHeight = (float)(box.stBoxs.nY2 - box.stBoxs.nY1) * scaleY;

        vecResult.push_back(stResult);
    }

    return true;
}

bool VehicleDetect_NS::CVehicleDetectV1_0::processVehicle(
    InData_S stInData,
    std::vector<Result_S> &vecResult)
{
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralytics_vehicle)
    {
        printf("未初始化算法类\n");
        return false;
    }
    
    if (m_stInParamPlate.bDebug && !stInData.inMat.empty() && !m_stInParamPlate.strOriginalDataPath.empty())
    {
        if (!Modules_NS::saveImage(stInData.inMat, m_stInParamPlate.strOriginalDataPath))
        {
            printf("Debug-保存图片失败[%s]\n", m_stInParamPlate.strOriginalDataPath.c_str());
        }
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;

    #ifdef RK_3588
    cv::Mat reMat;
    resizeAndPadImage(stInData.inMat,reMat);
    stInputData.pData = (float*)reMat.data;
    stInputData.nDataSize = static_cast<size_t>(reMat.total() * reMat.elemSize()) * sizeof(float);
    #else
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    #endif

    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;
    
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        int bRet = m_pYoloUltralytics_vehicle->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }
    const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
    #ifdef RK_3588
    const float scaleY = static_cast<float>(stInData.inMat.rows) / m_nLimitHeight;
    #else
    const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;
    #endif
    // std::cout << "sX:" << scaleX << " sY:" << scaleY << " pos(" << stInData.inMat.cols << "," << stInData.inMat.rows << ")" << " m_nLimitWidth:" << m_nLimitWidth << " m_nLimitHeight:" << m_nLimitHeight << std::endl;
    /* 跟踪算法 */
    for (const auto& box : vBoxDatas)
    {
        // std::cout << std::endl << "box label:"<< box.nLabel << " detectionType:" << stInData.stParam.detectionType << " 位且结果:" <<!( (1 << box.nLabel) & stInData.stParam.detectionType) << std::endl;
        if (!( (1 << box.nLabel) & stInData.stParam.detectionType)) { continue; };

        bool bAreaRet = false;
        std::vector<cv::Point> vRectPolygon = {cv::Point(box.stBoxs.nX1, box.stBoxs.nY1),
                                                cv::Point(box.stBoxs.nX2, box.stBoxs.nY1),
                                                cv::Point(box.stBoxs.nX2, box.stBoxs.nY2),
                                                cv::Point(box.stBoxs.nX1, box.stBoxs.nY2)};
        #ifdef RK_3588
        
        if(access("/testDrawImage", F_OK) == 0)
        {
            cv::Rect rect = cv::boundingRect(vRectPolygon);
            // 画矩形框
            cv::rectangle(reMat, rect, cv::Scalar(0, 0, 255), 2);
            
            if (!Modules_NS::saveImage(reMat, "/mnt/image"))
            {
                std::cout<< "[车辆算法]保存图片失败" << std::endl;
            }
        }

        for(auto itr : stInData.stParam.vsVehicleParam)
        {
            bAreaRet = isIntersecting(vRectPolygon,itr.vecPoints);
            if(bAreaRet)
            {
                break;
            }
        }
        #else
        if (stInData.stParam.vsVehicleParam.size()>0 && stInData.stParam.vsVehicleParam[0].bEnable)
        {
            bAreaRet = isIntersecting(vRectPolygon,
                                        stInData.stParam.vsVehicleParam[0].vecPoints);
        }
        #endif

        if (bAreaRet)
        {
            Result_S stResult;
            stResult.nId = 0;
            #ifdef RK_3588
            stResult.fX = (float)std::max(0,(box.stBoxs.nX1 - m_nXOffset)) * std::max(scaleX,scaleY);
            stResult.fY = (float)std::max(0,(box.stBoxs.nY1 - m_nYOffset)) * std::max(scaleX,scaleY);
            stResult.fWidth = (float)(box.stBoxs.nX2 - box.stBoxs.nX1) * std::max(scaleX,scaleY);
            stResult.fHeight = (float)(box.stBoxs.nY2 - box.stBoxs.nY1) * std::max(scaleX,scaleY);
            #else
            stResult.fX = (float)box.stBoxs.nX1 * scaleX;
            stResult.fY = (float)box.stBoxs.nY1 * scaleY;
            stResult.fWidth = (float)(box.stBoxs.nX2 - box.stBoxs.nX1) * scaleX;
            stResult.fHeight = (float)(box.stBoxs.nY2 - box.stBoxs.nY1) * scaleY;
            #endif
            // std::cout<< "point1:" << stResult.fX << " " << stResult.fY << " point2:" << stResult.fX + stResult.fWidth << " " << stResult.fY + stResult.fWidth << std::endl;
            // std::cout<< "pos  (" << box.stBoxs.nX1 << "," << box.stBoxs.nY1 << ") " << "(" << box.stBoxs.nX2 << "," << box.stBoxs.nY2 << ")" << std::endl;
            vecResult.push_back(stResult);
        }
    }

    return true;
}

/* 判断两个多边形是否有交点 */
bool VehicleDetect_NS::CVehicleDetectV1_0::isIntersecting(
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

bool VehicleDetect_NS::CVehicleDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;

    int newWidth = 0;
    int newHeight = 0;
    
    cv::Mat resizedImage;
    if(imageWidth > m_nLimitWidth || imageHeight > m_nLimitHeight)
    {
        m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);
        
        newWidth = static_cast<int>(imageWidth * m_fResizeScale);
        newHeight = static_cast<int>(imageHeight * m_fResizeScale);
        
        cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

        m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
        m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    }
    else
    {
        m_nXOffset = 0;
        m_nYOffset = 0;
        m_fResizeScale = 1.0;
        
        newWidth = imageWidth;
        newHeight = imageHeight;
        
        resizedImage = inputImage;
    }

    cv::Mat output = cv::Mat::zeros(cv::Size(m_nLimitWidth, m_nLimitHeight), inputImage.type());
    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    
    outputImage = output;

    return true;
}
