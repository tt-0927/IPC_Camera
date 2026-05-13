/**
 * @file FaceQualityAssessmentV1_1.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-12-05
 * 
 * @brief 
 */
#include "FaceQualityAssessmentV1_1.hpp"
#include "SaveImage.hpp"

FaceQualityAssessment_NS::CFaceQualityAssessmentV1_1::CFaceQualityAssessmentV1_1(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

FaceQualityAssessment_NS::CFaceQualityAssessmentV1_1::~CFaceQualityAssessmentV1_1()
{
    unInit();
}

/* 初始化 */
bool FaceQualityAssessment_NS::CFaceQualityAssessmentV1_1::init()
{
    bool bRet = false;
    
    m_pImageFeature = new Inference_NS::CImageFeature(m_stInParam.strModelPath);
    if (m_pImageFeature && m_pImageFeature->init())
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
bool FaceQualityAssessment_NS::CFaceQualityAssessmentV1_1::unInit()
{
    if (m_pImageFeature)
    {
        delete m_pImageFeature;
        m_pImageFeature = nullptr;
    }
    return true;
}

/* 处理数据 */
bool FaceQualityAssessment_NS::CFaceQualityAssessmentV1_1::process(
    InData_S stInData,
    float &fResult)
{
    fResult = 0.0;
    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pImageFeature)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;
    
    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bRet = m_pImageFeature->inference(stInputData, vClsDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }
    
    /* 得到人脸评价分数 */
    // float fSum = std::accumulate(fResults.begin(), fResults.end(), 0.0f);
    // fResult = fSum*1.0/fResults.size() ;

    return true;
}

/* 处理数据 */
bool FaceQualityAssessment_NS::CFaceQualityAssessmentV1_1::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    float fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * fResizeScale);
    int newHeight = static_cast<int>(imageHeight * fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    int nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    int nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(nXOffset, nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}