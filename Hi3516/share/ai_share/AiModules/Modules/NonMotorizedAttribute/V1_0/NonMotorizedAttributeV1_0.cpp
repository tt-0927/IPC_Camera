#pragma once

#include "NonMotorizedAttributeV1_0.hpp"
#include "SaveImage.hpp"

NonMotorizedAttribute_NS::CNonMotorizedAttributeV1_0::CNonMotorizedAttributeV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

NonMotorizedAttribute_NS::CNonMotorizedAttributeV1_0::~CNonMotorizedAttributeV1_0()
{
    unInit();
}

/* 初始化 */
bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV1_0::init()
{
    bool bRet = false;

    m_pNonMotorizedAttribute = new Inference_NS::CNonMotorizedAttribute(m_stInParam.strModelPath);
    if (m_pNonMotorizedAttribute)
    {
        if (m_pNonMotorizedAttribute->init())
        {
            bRet = m_pNonMotorizedAttribute->getSizeLimit(
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
bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV1_0::unInit()
{
    if (m_pNonMotorizedAttribute)
    {
        delete m_pNonMotorizedAttribute;
        m_pNonMotorizedAttribute = nullptr;
    }
    return true;
}

/* 处理数据 */
bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &nResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pNonMotorizedAttribute)
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
    if (stInData.inMat.channels() != m_nLimitChannel)
    {
        printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
               stInData.inMat.channels(),
               m_nLimitChannel);
        return false;
    }

    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        // cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    /* 推理+后处理 */
    std::vector<Inference_NS::Attrbute_S> vOutDatas;

    bRet = m_pNonMotorizedAttribute->inference(stInData.inMat, vOutDatas);

    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    for (int i = 0; i < vOutDatas.size(); i++)
    {
        Result_S stRes;
        stRes.strName = vOutDatas[i].strName;
        stRes.fConfidence = vOutDatas[i].fConfidence;
        nResult.push_back(stRes);
    }

    return true;
}

bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage)
{
    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;
    float m_fResizeScale  = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    if (newWidth == newHeight)
    {
        outputImage = resizedImage;
        return true;
    } else {
        cv::Mat output = cv::Mat::ones(m_nLimitWidth, m_nLimitHeight, inputImage.type()) * 128;

        int m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
        int m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

        resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
        outputImage = output;
        return true;
    }

    
}
