/**
 * @file FaceDetectV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-22
 *
 * @brief人脸检测
 */

#include "FaceDetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#define nResultNum 16

FaceDetect_NS::CFaceDetectV1_0::CFaceDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

FaceDetect_NS::CFaceDetectV1_0::~CFaceDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool FaceDetect_NS::CFaceDetectV1_0::init()
{
    bool bRet = false;
    
    m_pYoloUltralyticsPoint = new Inference_NS::CYoloUltralyticsPoint(m_stInParam.strModelPath);
    if (m_pYoloUltralyticsPoint && m_pYoloUltralyticsPoint->init())
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n", m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool FaceDetect_NS::CFaceDetectV1_0::unInit()
{
    if (m_pYoloUltralyticsPoint)
    {
        delete m_pYoloUltralyticsPoint;
        m_pYoloUltralyticsPoint = nullptr;
    }
    return true;
}

/* 处理数据 */
bool FaceDetect_NS::CFaceDetectV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vResults,
    OutData_S*             stOutData)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    vResults.clear();
    std::vector<float> vOutData;

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralyticsPoint)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = m_pYoloUltralyticsPoint->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
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

    /* 推理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;
    
    std::vector<Inference_NS::PointData_S> vPointDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bRet = m_pYoloUltralyticsPoint->inference(stInputData, vPointDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        int enType = -1; //int::Type_COUNT;
        const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
        const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;
        
        /* 遍历vPointDatas并转换到vResults */
        for (const auto& pointData : vPointDatas)
        {
            Result_S stResult;
            stResult.fX1 = static_cast<float>(pointData.stBoxs.nX1 * scaleX);
            stResult.fY1 = static_cast<float>(pointData.stBoxs.nY1 * scaleY);
            stResult.fX2 = static_cast<float>(pointData.stBoxs.nX2 * scaleX);
            stResult.fY2 = static_cast<float>(pointData.stBoxs.nY2 * scaleY);
            stResult.fBoxConfidence = pointData.fConfidence;
            
            for (const auto& pt : pointData.vPoints)
            {
                stResult.vPoint.push_back(static_cast<float>(pt.nX * scaleX));
                stResult.vPoint.push_back(static_cast<float>(pt.nY * scaleY));
            }

            stResult.nID = pointData.nLabel;
            stResult.sClassName = "人脸";
            
            vResults.push_back(stResult);
            enType = 4; //Type_E::TARGET_CAPTURE;
        }

        stOutData->nChnId = stInData.nChnId;
        stOutData->nType = enType;
    }

    return true;
}

/* 处理数据 */
bool FaceDetect_NS::CFaceDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());
    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    
    outputImage = output;

    return true;
}
