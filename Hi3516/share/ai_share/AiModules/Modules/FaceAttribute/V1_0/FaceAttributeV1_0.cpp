/**
 * @file FaceAttributeV2_0.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-20
 * 
 * @brief 人脸属性获取
 */
#pragma once

#include "FaceAttributeV1_0.hpp"
#include "SaveImage.hpp"

FaceAttribute_NS::CFaceAttributeV1_0::CFaceAttributeV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

FaceAttribute_NS::CFaceAttributeV1_0::~CFaceAttributeV1_0()
{
    unInit();
}

/* 初始化 */
bool FaceAttribute_NS::CFaceAttributeV1_0::init()
{
    bool bRet = false;

    m_pFaceAttribute = new Inference_NS::CAttribute(m_stInParam.strModelPath);
    if (m_pFaceAttribute)
    {
        if (m_pFaceAttribute->init())
        {
            bRet = m_pFaceAttribute->getSizeLimit(
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
bool FaceAttribute_NS::CFaceAttributeV1_0::unInit()
{
    if (m_pFaceAttribute)
    {
        delete m_pFaceAttribute;
        m_pFaceAttribute = nullptr;
    }
    return true;
}

void printResult(const FaceAttribute_NS::Result_S& result, const std::string& title = "人脸属性检测结果")
{
    std::cout << "==================== " << title << " ====================" << std::endl;

    // 打印 bool 类型
    std::cout << "1. 是否是男性 (bIsMale): " << (result.bIsMale ? "是" : "否") << std::endl;
    std::cout << "2. 是否戴眼镜 (bIsGlasses): " << (result.bIsGlasses ? "是" : "否") << std::endl;
    std::cout << "3. 是否有胡子 (bIsBeard): " << (result.bIsBeard ? "是" : "否") << std::endl;
    std::cout << "4. 是否戴口罩 (bIsMask): " << (result.bIsMask ? "是" : "否") << std::endl;

    // 打印年龄标签，并转换为文字描述
    std::cout << "5. 年龄 (nAgeLabel): ";
    switch(result.nAgeLabel) {
        case 1: std::cout << "小孩"; break;
        case 2: std::cout << "青年"; break;
        case 3: std::cout << "中年"; break;
        case 4: std::cout << "老年"; break;
        default: std::cout << "未知 (" << result.nAgeLabel << ")";
    }
    std::cout << std::endl;

    // 打印表情标签，并转换为文字描述
    std::cout << "6. 表情 (nEmotionLabel): ";
    switch(result.nEmotionLabel) 
    {
        case 8:  std::cout << "中性"; break;
        case 9:  std::cout << "愤怒"; break;
        case 10: std::cout << "快乐"; break;
        case 11: std::cout << "悲伤"; break;
        case 12: std::cout << "惊讶"; break;
        case 13: std::cout << "恐惧"; break;
        case 14: std::cout << "厌恶"; break;
        default: std::cout << "未知 (" << result.nEmotionLabel << ")";
    }
    std::cout << std::endl;
    
    std::cout << "========================================================" << std::endl;
}

/* 处理数据 */
bool FaceAttribute_NS::CFaceAttributeV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pFaceAttribute)
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

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));

    bRet = m_pFaceAttribute->inference(stInputData, vClsDatas);

    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }
    
    for (int i = 0; i < vClsDatas.size(); i++)
    {
        const std::vector<Inference_NS::Cls_S>& vCls = vClsDatas[i].vCls;
        Result_S stResult;
        for (int j = 0; j < vCls.size(); j++) 
        {
            const Inference_NS::Cls_S& cls = vCls[j];
            float confidence = cls.fConfidence;
            //printf("vClsDatas[%d]  nLabel[%d] - fConfidence[%f]\n",i,cls.nLabel,cls.fConfidence);
            switch (j) 
            {
                case 0: // j=0：是否是男性（标签 0）
                    stResult.bIsMale = (cls.nLabel == 0 && confidence > CONF_THRESHOLD);
                    break;
                case 1: // j=1：年龄标签（1~4）
                    if (cls.nLabel >= 1 && cls.nLabel <= 4) 
                    {
                        stResult.nAgeLabel = cls.nLabel;
                    }
                    break;
                case 2: // j=2：是否戴眼镜（标签 5）
                    stResult.bIsGlasses = (cls.nLabel == 5 && confidence > CONF_GLASS_THRESHOLD);
                    break;
                case 3: // j=3：是否有胡子（标签 6）
                    stResult.bIsBeard = (cls.nLabel == 6 && confidence >= CONF_BEARD_THRESHOLD);
                    break;
                case 4: // j=4：是否戴口罩（标签 7）
                    stResult.bIsMask = (cls.nLabel == 7 && confidence > CONF_THRESHOLD);
                    break;
                case 5: // j=5：表情标签（8~14）
                    if (cls.nLabel >= 8 && cls.nLabel <= 14) 
                    {
                        stResult.nEmotionLabel = cls.nLabel;
                    }
                    break;
                default:
                    std::cout << "警告：j=" << j << " 超出预期范围（0~5）" << std::endl;
                    break;
            }
        }
        vecResult.push_back(stResult);
        //printResult(stResult);
    }

    return true;
}

bool FaceAttribute_NS::CFaceAttributeV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage)
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
