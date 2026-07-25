#pragma once

#include "ClipImageV2_0.hpp"
#include "SaveImage.hpp"

ClipImage_NS::CClipImageV2_0::CClipImageV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

ClipImage_NS::CClipImageV2_0::~CClipImageV2_0()
{
    unInit();
}

/* 初始化 */
bool ClipImage_NS::CClipImageV2_0::init()
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
            if (bRet)
            {
                m_pClipImage->getMeanStd(m_vMean, m_vStd);
            }
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strModelPath.c_str());
        goto FAIL;
    }
    else
    {
        printf("模型初始化成功 [%d * %d * %d]\n",
               m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool ClipImage_NS::CClipImageV2_0::unInit()
{
    if (m_pClipImage)
    {
        delete m_pClipImage;
        m_pClipImage = nullptr;
    }
    return true;
}

/* 处理数据 */
bool ClipImage_NS::CClipImageV2_0::process(
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

    /* 推理+后处理 */
    cv::Mat processedMat;
    Inference_NS::InputData_S stInputData;
    
    /* 预处理 */
    if (access("/ClibImage", F_OK) == 0)
    {
        cv::cvtColor(stInData.inMat, stInData.inMat, cv::COLOR_BGR2RGB);
        if (!Modules_NS::saveImage(stInData.inMat, "/opt/bl/bin/ClibImage/"))
        {
            printf("Debug-保存图片失败[ClipImageV2_0.cpp]\n");
        }
    }

    PreProcess(
        stInData.inMat,
        processedMat,
        m_nLimitWidth,
        m_nLimitHeight,
        m_vMean,
        m_vStd,
        false);

    stInputData.pData = (float*)processedMat.data;
    stInputData.nDataSize = static_cast<int>(processedMat.total() * processedMat.elemSize());

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

bool ClipImage_NS::CClipImageV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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

bool ClipImage_NS::CClipImageV2_0::PreProcess(
    cv::Mat aInput,
    cv::Mat& aOutput,
    int nTargetWidth,
    int nTargetHeight,
    std::vector<float> vMean,
    std::vector<float> vStd,
    bool bRgb)
{
    try
    {
        CV_Assert(!aInput.empty());
        CV_Assert(vMean.size() == 3 && vStd.size() == 3);

        // ===== 1. resize =====
        cv::Mat img;
        cv::resize(aInput, img, cv::Size(nTargetWidth, nTargetHeight), 0, 0, cv::INTER_LINEAR);

        // ===== 2. BGR -> RGB（如果需要）=====
        if (bRgb)
            cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

        // ===== 3. 转 float（只做一次）=====
        img.convertTo(img, CV_32F);

        const int H = img.rows;
        const int W = img.cols;
        const int plane = H * W;

        // ===== 4. 输出 NCHW =====
        aOutput.create(1, 3 * plane, CV_32F);

        const float* src = img.ptr<float>();
        float* dst = aOutput.ptr<float>();

        // plane 分通道指针
        float* dst0 = dst;
        float* dst1 = dst + plane;
        float* dst2 = dst + 2 * plane;

        // ===== 5. 预计算（关键优化）=====
        const float m0 = vMean[0], m1 = vMean[1], m2 = vMean[2];
        const float s0 = 1.0f / vStd[0];
        const float s1 = 1.0f / vStd[1];
        const float s2 = 1.0f / vStd[2];

        // ===== 6. 单遍 HWC → NCHW（极限核心）=====
        for (int i = 0, j = 0; i < plane; ++i, j += 3)
        {
            float b = src[j + 0];
            float g = src[j + 1];
            float r = src[j + 2];

            dst0[i] = (b - m0) * s0;
            dst1[i] = (g - m1) * s1;
            dst2[i] = (r - m2) * s2;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        printf("PreProcess error: %s\n", e.what());
        return false;
    }
}
