/**
 * @file HumanAreaDetectV4_0.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2026-06-09
 * 
 * @brief 区域检测 ONNX 简化版，只有检测逻辑
 */
#pragma once

#include "HumanAreaDetectV4_0.hpp"
#include "SaveImage.hpp"

HumanAreaDetect_NS::CHumanAreaDetectV4_0::CHumanAreaDetectV4_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

HumanAreaDetect_NS::CHumanAreaDetectV4_0::~CHumanAreaDetectV4_0()
{
    unInit();
}

/* 初始化 */
bool HumanAreaDetect_NS::CHumanAreaDetectV4_0::init()
{
    bool bRet = false;

    m_pYoloUltralytics = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pYoloUltralytics)
    {
        if (m_pYoloUltralytics->init())
        {
            bRet = m_pYoloUltralytics->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
            if (bRet)
            {
                m_pYoloUltralytics->getMeanStd(m_vMean, m_vStd);
            }
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
bool HumanAreaDetect_NS::CHumanAreaDetectV4_0::unInit()
{
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }
    return true;
}

/* 处理数据 - 简化版本 */
bool HumanAreaDetect_NS::CHumanAreaDetectV4_0::process(
    InData_S stInData,
    std::vector<Result_S>& vecResult)
{
    vecResult.clear();
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
    cv::Mat resizedPaddedMat;
    cv::Mat processedMat;
    Inference_NS::InputData_S stInputData;

    /* 缩放和填充图像 */
    if (!resizeAndPadImage(stInData.inMat, resizedPaddedMat))
    {
        printf("缩放和填充图像失败\n");
        return false;
    }

    /* 预处理 */
    PreProcess(
        resizedPaddedMat,
        processedMat,
        m_nLimitWidth,
        m_nLimitHeight,
        m_vMean,
        m_vStd,
        true);

    stInputData.pData = (float*)processedMat.data;
    stInputData.nDataSize = static_cast<int>(processedMat.total() * processedMat.elemSize());

    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    bool bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    /* 整理结果，将坐标缩放回原图尺寸 */
    for (const auto& box : vBoxDatas)
    {
        Result_S stResult;
        stResult.nId = static_cast<int>(box.fConfidence * 100);
        
        /* 减去填充偏移，然后缩放回原图 */
        float x1 = (static_cast<float>(box.stBoxs.nX1) - m_nXOffset) / m_fResizeScale;
        float y1 = (static_cast<float>(box.stBoxs.nY1) - m_nYOffset) / m_fResizeScale;
        float x2 = (static_cast<float>(box.stBoxs.nX2) - m_nXOffset) / m_fResizeScale;
        float y2 = (static_cast<float>(box.stBoxs.nY2) - m_nYOffset) / m_fResizeScale;
        
        /* 确保坐标在图像范围内 */
        stResult.fX = std::max(0.0f, x1);
        stResult.fY = std::max(0.0f, y1);
        stResult.fWidth = std::max(0.0f, x2 - x1);
        stResult.fHeight = std::max(0.0f, y2 - y1);
        
        /* 限制在图像范围内 */
        stResult.fX = std::min(stResult.fX, static_cast<float>(stInData.inMat.cols - 1));
        stResult.fY = std::min(stResult.fY, static_cast<float>(stInData.inMat.rows - 1));
        stResult.fWidth = std::min(stResult.fWidth, stInData.inMat.cols - stResult.fX);
        stResult.fHeight = std::min(stResult.fHeight, stInData.inMat.rows - stResult.fY);
        
        vecResult.push_back(stResult);
    }

    return true;
}

bool HumanAreaDetect_NS::CHumanAreaDetectV4_0::PreProcess(
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
        /* 各通道的归一化倍数是否一样 */
        bool bNormal = std::adjacent_find(vStd.begin(), vStd.end(), std::not_equal_to<>()) == vStd.end();
        /* 归一化 */
        if(!bNormal)
        {
            /* 1. 分离三个通道 */
            std::vector<cv::Mat> aChannels;
            cv::split(aInput, aChannels); 
            /* 2. 对每个通道分别进行缩放 */ 
            for(int nC=0; nC<aChannels.size(); nC++)
            {
                cv::multiply(aChannels[nC], 1.0 / vStd[nC], aChannels[nC]); 
            }

            /* 3. 合并通道 */ 
            cv::merge(aChannels, aInput);
        }

        /* whc转为chw */
        /* 设置目标尺寸 */
        cv::Size stTargetSize = cv::Size(nTargetWidth, nTargetHeight);

        /* 设置方差 */
        float fSC = bNormal? (1.0/vStd[0]) : 1.0;
        /* 设置均值 */
        cv::Scalar stvfMean = cv::Scalar(0, 0, 0);
        if(vMean.size()==1)
        {
            stvfMean = cv::Scalar(vMean[0]);
        }
        else if(vMean.size()==3)
        {
            stvfMean = cv::Scalar(vMean[0],vMean[1],vMean[2]);
        }

        /* 创建 4D blob，适用于神经网络输入 */
        aOutput = cv::dnn::blobFromImage(
            aInput,           /* 输入图像 */
            fSC,             /* 缩放因子 */
            stTargetSize,    /* 目标尺寸 */
            stvfMean,        /* 均值（减去） */
            bRgb,            /* 是否交换 BGR 和 RGB 通道 */
            false,           /* 是否裁剪图像 */
            CV_32F           /* 输出数据类型 */
        );
    }
    catch (const std::exception& e) 
    {
        printf("OpencvPreProcess处理报错: %s\n", e.what());
        return false;
    }

    return true;
}

bool HumanAreaDetect_NS::CHumanAreaDetectV4_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    
    // 计算缩放比例
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // 计算新的宽高
    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // 缩放图像
    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    // 创建目标大小的空白图像
    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());

    // 计算居中位置
    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    // 将缩放后的图像复制到居中位置
    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}
