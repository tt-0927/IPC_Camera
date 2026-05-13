#pragma once

#include "VehicleAttributeV1_0.hpp"
#include "SaveImage.hpp"

VehicleAttribute_NS::CVehicleAttributeV1_0::CVehicleAttributeV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

VehicleAttribute_NS::CVehicleAttributeV1_0::~CVehicleAttributeV1_0()
{
    unInit();
}

/* 初始化 */
bool VehicleAttribute_NS::CVehicleAttributeV1_0::init()
{
    bool bRet = false;

    m_pVehicleAttribute = new Inference_NS::CVehicleAttribute(m_stInParam.strModelPath);
    if (m_pVehicleAttribute)
    {
        if (m_pVehicleAttribute->init())
        {
            bRet = m_pVehicleAttribute->getSizeLimit(
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
bool VehicleAttribute_NS::CVehicleAttributeV1_0::unInit()
{
    if (m_pVehicleAttribute)
    {
        delete m_pVehicleAttribute;
        m_pVehicleAttribute = nullptr;
    }
    return true;
}

/* 处理数据 */
bool VehicleAttribute_NS::CVehicleAttributeV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &nResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pVehicleAttribute)
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

    bRet = m_pVehicleAttribute->inference(stInData.inMat, vOutDatas);

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

bool VehicleAttribute_NS::CVehicleAttributeV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    float m_fResizeScale = std::min(static_cast<float>(m_nLimitWidth) / imageWidth, static_cast<float>(m_nLimitHeight) / imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    int Xoffset = m_nLimitWidth - newWidth;
    int Yoffset = m_nLimitHeight - newHeight;
    int left = static_cast<int>(Xoffset / 2);
    int top = static_cast<int>(Yoffset / 2);
    int right = Xoffset - left;
    int bottom = Yoffset - top;

    cv::copyMakeBorder(resizedImage, resizedImage, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(127, 127, 127));
    outputImage = resizedImage;
    return true;
}