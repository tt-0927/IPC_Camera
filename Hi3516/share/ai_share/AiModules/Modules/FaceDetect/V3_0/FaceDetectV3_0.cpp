/**
 * @file FaceDetectV3_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-22
 *
 * @brief人脸检测
 */

#include "FaceDetectV3_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#define nResultNum 16

FaceDetect_NS::CFaceDetectV3_0::CFaceDetectV3_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

FaceDetect_NS::CFaceDetectV3_0::~CFaceDetectV3_0()
{
    unInit();
}

/* 初始化 */
bool FaceDetect_NS::CFaceDetectV3_0::init()
{
    bool bRet = false;
    
    m_pYolov5Point = new Inference_NS::CYolov5Point(m_stInParam.strModelPath);
    if (m_pYolov5Point && m_pYolov5Point->init())
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
bool FaceDetect_NS::CFaceDetectV3_0::unInit()
{
    if (m_pYolov5Point)
    {
        delete m_pYolov5Point;
        m_pYolov5Point = nullptr;
    }
    return true;
}

/* 处理数据 */
bool FaceDetect_NS::CFaceDetectV3_0::process(
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

    if (!m_pYolov5Point)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = m_pYolov5Point->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
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
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;
    
    std::vector<Inference_NS::PointData_S> vPointDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bRet = m_pYolov5Point->inference(stInputData, vPointDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        int enType = -1; //int::Type_COUNT;
        
        /* 遍历vPointDatas并转换到vResults */
        for (const auto& pointData : vPointDatas)
        {
            Result_S stResult;
            stResult.fX1 = pointData.stBoxs.nX1;
            stResult.fY1 = pointData.stBoxs.nY1;
            stResult.fX2 = pointData.stBoxs.nX2;
            stResult.fY2 = pointData.stBoxs.nY2;
            stResult.fBoxConfidence = pointData.fConfidence;
            for (const auto& pt : pointData.vPoints)
            {
                stResult.vPoint.push_back(pt.nX);
                stResult.vPoint.push_back(pt.nY);
            }
            
            stResult.nID = pointData.nLabel;
            stResult.sClassName = "人脸";
            
            vResults.push_back(stResult);
            stOutData->validResult = true;
            enType = 4; //Type_E::TARGET_CAPTURE;
        }

        stOutData->nType = enType;
    }

    return true;
}

/* 处理数据 */
bool FaceDetect_NS::CFaceDetectV3_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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
