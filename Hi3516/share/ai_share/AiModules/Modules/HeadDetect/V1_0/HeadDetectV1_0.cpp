/**
 * @file HeadDetectV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-22
 *
 * @brief人脸检测
 */

#include "HeadDetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#define nResultNum 6

HeadDetect_NS::CHeadDetectV1_0::CHeadDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

HeadDetect_NS::CHeadDetectV1_0::~CHeadDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool HeadDetect_NS::CHeadDetectV1_0::init()
{
    CStatisticsTimer runTime("CHeadDetectV1_0 初始化耗时");
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

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool HeadDetect_NS::CHeadDetectV1_0::unInit()
{
    CStatisticsTimer runTime("CHeadDetectV1_0 反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }
    return true;
}

/* 处理数据 */
bool HeadDetect_NS::CHeadDetectV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vResults)
{
    vResults.clear();
    std::vector<float> vOutData;

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralytics)
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

        const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
        const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;
        for (const auto& box : vBoxDatas)
        {
            Result_S stResult;
            stResult.nID = box.nLabel;
            stResult.fX1 = (float)box.stBoxs.nX1 * scaleX;
            stResult.fY1 = (float)box.stBoxs.nY1 * scaleY;
            stResult.fX2 = (float)box.stBoxs.nX2 * scaleX;
            stResult.fY2 = (float)box.stBoxs.nY2 * scaleY;
            stResult.fBoxConfidence = (float)box.fConfidence;
            vResults.push_back(stResult);
        }
    }

    return true;
}

/* 处理数据 */
bool HeadDetect_NS::CHeadDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);


    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}
