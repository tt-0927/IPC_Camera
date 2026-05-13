/*
 * @FilePath     : HighAltitudeDetectV1_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-28 11:48:50
 * @Description  : 人少场景
 */
#include "HighAltitudeDetectV1_0.hpp"
#include "HighAltitudeTracker.hpp"
#include "SaveImage.hpp"

/* 一组数据的大小 */
#define DATA_GROUP_SIZE1 6
#define DATA_GROUP_SIZE2 4

using namespace HighAltitudeDetect_NS;

HighAltitudeDetect_NS::CHighAltitudeDetectV1_0::CHighAltitudeDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

HighAltitudeDetect_NS::CHighAltitudeDetectV1_0::~CHighAltitudeDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool HighAltitudeDetect_NS::CHighAltitudeDetectV1_0::init()
{
    m_pKNNDetector = cv::createBackgroundSubtractorKNN(m_nHistory, m_fDist2Threshold, m_bDetectShadows);

    bool bRet = false;
    m_HighAltitudeTracker = new Inference_NS::cHighAltitudeTracker();
    if (m_HighAltitudeTracker)
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [跟踪算法]\n");
        goto FAIL;
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool HighAltitudeDetect_NS::CHighAltitudeDetectV1_0::unInit()
{
    if (m_HighAltitudeTracker)
    {
        delete m_HighAltitudeTracker;
        m_HighAltitudeTracker = nullptr;
    }

    return true;
}

/* 处理数据 */
bool HighAltitudeDetect_NS::CHighAltitudeDetectV1_0::process(
    InData_S               stInData,
    std::vector<Result_S>& vecResult)
{
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pKNNDetector || !m_HighAltitudeTracker)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    std::vector<float> vecPos;
    vecPos.clear();

    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* 前处理 */
    cv::Mat copiedImage;
    stInData.inMat.copyTo(copiedImage);
    // cv::Mat resizedImage;
    // cv::resize(copiedImage, resizedImage, cv::Size(copiedImage.cols / 2, copiedImage.rows / 2));

    /* 1、背景减除器 */
    cv::Mat mask;
    m_pKNNDetector->apply(copiedImage, mask);

    /* 2、形态学处理 */
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, m_pKernel);
    cv::morphologyEx(mask, mask, cv::MORPH_DILATE, m_pKernel);

    /* 3、查找轮廓 */
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    /* 4. 过滤轮廓并生成边界框 */
    std::vector<cv::Rect> vecBoxs;
    vecBoxs.clear();
    for (const auto& c : contours)
    {
        if (cv::contourArea(c) < m_nMinArea) {
            continue;
        }
        cv::Rect bbox = cv::boundingRect(c);
        // std::cout << "Bounding box: x=" << bbox.x 
        //         << ", y=" << bbox.y 
        //         << ", width=" << bbox.width 
        //         << ", height=" << bbox.height << std::endl;

        int x1 = bbox.x;
        int y1 = bbox.y;
        int w = bbox.width;
        int h = bbox.height;
        vecBoxs.emplace_back(x1, y1, w, h);

    }

    /* 跟踪算法 */
    std::vector<cv::Rect> vecStracks = m_HighAltitudeTracker->update(vecBoxs);

    
    /* 进行区域判断 */
    for (int nIndex = 0; nIndex < vecStracks.size(); nIndex++)
    {
        
        // Result_S stResult;
        // stResult.fX      = vecStracks[nIndex].x * 2;
        // stResult.fY      = vecStracks[nIndex].y * 2;
        // stResult.fWidth  = vecStracks[nIndex].width * 2;
        // stResult.fHeight = vecStracks[nIndex].height * 2;
        // vecResult.push_back(stResult);

        Result_S stResult;
        stResult.fX      = vecStracks[nIndex].x;
        stResult.fY      = vecStracks[nIndex].y;
        stResult.fWidth  = vecStracks[nIndex].width;
        stResult.fHeight = vecStracks[nIndex].height;
        vecResult.push_back(stResult);

        // std::cout << "predict box : x=" << stResult.fX
        //         << ", y=" << stResult.fY
        //         << ", width=" << stResult.fWidth
        //         << ", height=" << stResult.fHeight << std::endl;

        // if (fDistance > 2 * (vecOrgtlwh[2] + vectlwh[2]) && 
        //     fDistance > (vecOrgtlwh[1] + vectlwh[3]))
        // {
        //     Result_S stResult;
        //     stResult.nId     = nTargetId;
        //     stResult.fX      = vectlwh[0] * 2;
        //     stResult.fY      = vectlwh[1] * 2;
        //     stResult.fWidth  = vectlwh[2] * 2;
        //     stResult.fHeight = vectlwh[3] * 2;
        //     vecResult.push_back(stResult);

        //     std::cout << "original box : x=" << vecOrgtlwh[0] * 2
        //             << ", y=" << vecOrgtlwh[1] * 2
        //             << ", width=" << vecOrgtlwh[2] * 2
        //             << ", height=" << vecOrgtlwh[3] * 2 << std::endl;

        //     std::cout << "predict box : x=" << stResult.fX
        //             << ", y=" << stResult.fY
        //             << ", width=" << stResult.fWidth
        //             << ", height=" << stResult.fHeight << std::endl;
        // }

        
        

        // if (m_mapTarget.count(nTargetId))
        // {
        //     float fDeviationX = t_cx - m_mapTarget[nTargetId].previousX;
        //     float fDeviationY = t_cy - m_mapTarget[nTargetId].previousY;
        //     float fDistance = std::hypot(fDeviationX, fDeviationY);

        //     /* 分析结果 */
        //     Result_S stResult;
        //     stResult.nId     = nTargetId;
        //     stResult.fX      = vectlwh[0] * 2;
        //     stResult.fY      = vectlwh[1] * 2;
        //     stResult.fWidth  = vectlwh[2] * 2;
        //     stResult.fHeight = vectlwh[3] * 2;
        //     vecResult.push_back(stResult);
        //     // std::cout << "Bounding box ID: " << stResult.nId
        //     // << ", x=" << stResult.fX
        //     // << ", y=" << stResult.fY
        //     // << ", width=" << stResult.fWidth
        //     // << ", height=" << stResult.fHeight << std::endl;

        //     // if (fDistance > 2 * (m_mapTarget[nTargetId].previousW + vectlwh[2]) && 
        //     //     fDistance > (m_mapTarget[nTargetId].previousH + vectlwh[3]))
        //     // {
        //     //     /* 分析结果 */
        //     //     Result_S stResult;
        //     //     stResult.nId     = nTargetId;
        //     //     stResult.fX      = vectlwh[0];
        //     //     stResult.fY      = vectlwh[1];
        //     //     stResult.fWidth  = vectlwh[2];
        //     //     stResult.fHeight = vectlwh[3];
        //     //     vecResult.push_back(stResult);
        //     //     std::cout << "Bounding box ID: " << stResult.nId
        //     //     << ", x=" << stResult.fX
        //     //     << ", y=" << stResult.fY
        //     //     << ", width=" << stResult.fWidth
        //     //     << ", height=" << stResult.fHeight << std::endl;
        //     // }

        //     m_mapTarget[nTargetId].previousX  = t_cx;
        //     m_mapTarget[nTargetId].previousY  = t_cy;
        //     m_mapTarget[nTargetId].previousW  = vectlwh[2];
        //     m_mapTarget[nTargetId].previousH  = vectlwh[3];
        //     m_mapTarget[nTargetId].ndwellTime = 0;
        //     m_mapTarget[nTargetId].isUsed     = true;
        // }
        // else
        // {
        //     Target_S newTarge;
        //     newTarge.previousX  = t_cx;
        //     newTarge.previousY  = t_cy;
        //     newTarge.previousW  = vectlwh[2];
        //     newTarge.previousH  = vectlwh[3];
        //     newTarge.ndwellTime = 0;
        //     newTarge.isUsed     = true;
        //     m_mapTarget[nTargetId] = newTarge;
        // }
    
    // for (auto pair = m_mapTarget.begin(); pair != m_mapTarget.end();)
    // {
    //     int id = pair->first;
    //     if (m_mapTarget[id].isUsed)
    //     {
    //         m_mapTarget[id].isUsed = false;
    //     }
    //     else
    //     {
    //         m_mapTarget[id].ndwellTime++;
    //     }

    //     if (m_mapTarget[id].ndwellTime >= m_nMaxTimeLost)
    //     {
    //         pair = m_mapTarget.erase(pair);
    //     }
    //     else
    //     {
    //         ++pair;
    //     }
    // }
        
 

    // if (m_stInParam.bDebug)
    // {
    //     /* 保存分析后的图片 */
    //     if (!stInData.inMat.empty() && !m_stInParam.strAnalyzeDataPath.empty())
    //     {

    //         for (int nIndex = 0; nIndex < vecStracks.size(); nIndex++)
    //         {
    //             std::vector<float> vectlwh = vecStracks[nIndex].tlwh;
    //             /* 框 */
    //             cv::rectangle(
    //                 stInData.inMat,
    //                 cv::Rect(vectlwh[0], vectlwh[1], vectlwh[2], vectlwh[3]),
    //                 cv::Scalar(0, 0, 255),
    //                 4);

    //             if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strAnalyzeDataPath))
    //             {
    //                 printf("Debug-保存图片失败[%s]\n", m_stInParam.strAnalyzeDataPath.c_str());
    //             }
    //         }
    //     }
    }

    return true;
}

/* 处理数据 */
bool HighAltitudeDetect_NS::CHighAltitudeDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage)
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
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}



