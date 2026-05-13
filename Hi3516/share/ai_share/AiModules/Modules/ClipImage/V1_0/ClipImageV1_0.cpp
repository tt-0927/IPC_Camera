#pragma once

#include "ClipImageV1_0.hpp"
#include "SaveImage.hpp"

ClipImage_NS::CClipImageV1_0::CClipImageV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

ClipImage_NS::CClipImageV1_0::~CClipImageV1_0()
{
    unInit();
}

/* 初始化 */
bool ClipImage_NS::CClipImageV1_0::init()
{
    bool bRet = false;

    m_pClipImage = new Inference_NS::CImageFeature(m_stInParam.strModelPath);
    if (m_pClipImage)
    {
        if (m_pClipImage->init())
        {
            bRet = m_pClipImage->getSizeLimit(
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
bool ClipImage_NS::CClipImageV1_0::unInit()
{
    if (m_pClipImage)
    {
        delete m_pClipImage;
        m_pClipImage = nullptr;
    }
    return true;
}

/* 处理数据 */
bool ClipImage_NS::CClipImageV1_0::process(
    InData_S stInData,
    std::vector<float> &vResult)
{
    vResult.clear();
    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pClipImage)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

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
    // if (stInData.inMat.channels() != m_nLimitChannel)
    // {
    //     printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
    //            stInData.inMat.channels(),
    //            m_nLimitChannel);
    //     return false;
    // }

    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        // cv::cvtColor(stInData.inMat, stInData.inMat, cv::COLOR_BGR2RGB);
        // resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<int>(stInData.inMat.total() * stInData.inMat.elemSize())* sizeof(float);

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bRet = m_pClipImage->inference(stInputData, vClsDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    printf("图片特征向量为:");
    for(int i=0;i<10;i++)
    {
        printf("%f ",vClsDatas[0].vFeature[i]);
    }
    printf("\n");

    vResult = vClsDatas[0].vFeature;
    return true;
}

bool ClipImage_NS::CClipImageV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    float m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    if (newWidth == newHeight)
    {
        outputImage = resizedImage;
        return true;
    }
    else
    {
        cv::Mat output = cv::Mat::ones(m_nLimitWidth, m_nLimitHeight, inputImage.type()) * 128;

        int m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
        int m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

        resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
        outputImage = output;
        return true;
    }
}
