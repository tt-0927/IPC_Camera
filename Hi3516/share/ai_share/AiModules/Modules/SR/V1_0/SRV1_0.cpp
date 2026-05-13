/**
 * @file SRV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-01
 *
 * @brief
 */
#include "SRV1_0.hpp"

#include "RLFN.hpp"
#include "SaveImage.hpp"

#define SRLen 128

SR_NS::CSRV1_0::CSRV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

SR_NS::CSRV1_0::~CSRV1_0()
{
    unInit();
}

/* 初始化 */
bool SR_NS::CSRV1_0::init()
{
    bool bRet = false;

    m_pSR = new Inference_NS::CRLFN(m_stInParam.strModelPath);
    if (m_pSR)
    {
        if (m_pSR->init())
        {
            bRet = m_pSR->getSizeLimit(
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
bool SR_NS::CSRV1_0::unInit()
{
    if (m_pSR)
    {
        delete m_pSR;
        m_pSR = nullptr;
    }
    return true;
}

/* 处理数据 */
bool SR_NS::CSRV1_0::process(
    InData_S            stInData,
    cv::Mat             &aResult)
{
    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pSR)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

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
        // cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth,m_nLimitHeight));
        resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    /* 推理+后处理 */
    aResult.create(m_nLimitHeight*4, m_nLimitWidth*4, CV_32FC3);
    bRet = m_pSR->inference(stInData.inMat, aResult);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    return true;
}

/* 处理数据 */
bool SR_NS::CSRV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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