/**
 * @file PMNMDetectV3_0.hpp
 * @author songww
 * @date 2026-04-17
 *
 * @brief 行人、机动车、非机动车检测
 */

#include "PMNMDetectV3_0.hpp"
#include "SaveImage.hpp"

#define nResultNum 6

PMNMDetect_NS::CPMNMDetectV3_0::CPMNMDetectV3_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

PMNMDetect_NS::CPMNMDetectV3_0::~CPMNMDetectV3_0()
{
    unInit();
}

/* 初始化 */
bool PMNMDetect_NS::CPMNMDetectV3_0::init()
{
    bool bRet = false;

    m_pPMNMDetect = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pPMNMDetect)
    {
        if (m_pPMNMDetect->init())
        {
            bRet = m_pPMNMDetect->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool PMNMDetect_NS::CPMNMDetectV3_0::unInit()
{
    if (m_pPMNMDetect)
    {
        delete m_pPMNMDetect;
        m_pPMNMDetect = nullptr;
    }
    return true;
}

/* 处理数据 */
bool PMNMDetect_NS::CPMNMDetectV3_0::process(
    InData_S stInData,
    std::vector<Result_S> &vResults)
{
    vResults.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pPMNMDetect)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    bRet = m_pPMNMDetect->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
        return false;
    }

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

    Inference_NS::InputData_S stInputData;
    if(stInData.inMat.type() == CV_8UC3)
    {
        if(stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
        {
            cv::Mat reMat;
            resizeAndPadImage(stInData.inMat,reMat);
            stInputData.pData              = (float *)reMat.data;
            stInputData.nDataSize          = static_cast<size_t>(reMat.total() * reMat.elemSize() * sizeof(float));
        }
        else
        {
            stInputData.pData              = (float *)stInData.inMat.data;
            stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
        }
    }
    else
    {
        stInputData.pData              = (float *)stInData.inMat.data;
        stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    }
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

    std::vector<Inference_NS::BoxData_S> vBoxDatas;

    /* 推理+后处理 */
    bRet = m_pPMNMDetect->inference(stInputData, vBoxDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
    float scaleY = 0.0;
    if(stInData.inMat.type() == CV_8UC3)
        scaleY = static_cast<float>(stInData.inMat.rows) / m_nLimitHeight;
    else
        scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;

    for (int nIndex = 0; nIndex < vBoxDatas.size(); nIndex++)
    {
        Inference_NS::BoxData_S box = vBoxDatas.at(nIndex);
        if(box.nLabel != 0) continue;
        Result_S stResult;
        stResult.fX1 = (float)(box.stBoxs.nX1) * scaleX;
        stResult.fY1 = (float)(box.stBoxs.nY1) * scaleY;
        stResult.fX2 = (float)(box.stBoxs.nX2) * scaleX;
        stResult.fY2 = (float)(box.stBoxs.nY2) * scaleY;
        stResult.fBoxConfidence = box.fConfidence;
        stResult.nID = box.nLabel;
        vResults.push_back(stResult);
    }

    return true;
}

/* 处理数据 */
bool PMNMDetect_NS::CPMNMDetectV3_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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
