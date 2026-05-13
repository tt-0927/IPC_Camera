/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-02-04 15:43:27
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-05 15:08:19
 * @FilePath: /1126/share/ai_share/AiModules/Modules/NonMotorizedAttribute/V2_0/NonMotorizedAttributeV2_0.hpp
 * @Description: 非机动车属性检测
 */

#pragma once

#include "NonMotorizedAttributeV2_0.hpp"
#include "SaveImage.hpp"

const char *NonMotorTypeToString(
    NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType type)
{
    using NonMotorType = NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType;

    switch (type)
    {
    case NonMotorType::BICYCLE:
        return "自行车";
    case NonMotorType::TWO_WHEELER:
        return "二轮车";
    case NonMotorType::THREE_WHEELER:
        return "三轮车";
    default:
        return "未知车辆类型";
    }
}

const char *NonMotorColorToString(
    NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor color)
{
    using NonMotorColor = NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor;

    switch (color)
    {
    case NonMotorColor::WHITE:
        return "白色";
    case NonMotorColor::ORANGE:
        return "橙色";
    case NonMotorColor::PINK:
        return "粉色";
    case NonMotorColor::BLACK:
        return "黑色";
    case NonMotorColor::RED:
        return "红色";
    case NonMotorColor::YELLOW:
        return "黄色";
    case NonMotorColor::GRAY:
        return "灰色";
    case NonMotorColor::BLUE:
        return "蓝色";
    case NonMotorColor::GREEN:
        return "绿色";
    case NonMotorColor::BROWN:
        return "棕色";
    case NonMotorColor::PURPLE:
        return "紫色";
    default:
        return "未知颜色";
    }
}

void NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::PrintNonVehicleAttribute(const Result_S &stResult)
{
    auto type = static_cast<NonMotorType>(stResult.nNonMotorizedVehicleType);
    printf("非机动车类型: %s\n", NonMotorTypeToString(type));

    auto color = static_cast<NonMotorColor>(stResult.nNonMotorizedVehicleColor);
    printf("非机动车颜色: %s\n", NonMotorColorToString(color));

    return;
}

NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::CNonMotorizedAttributeV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::~CNonMotorizedAttributeV2_0()
{
    unInit();
}

/* 初始化 */
bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::init()
{
    bool bRet = false;

    m_pNonMotorizedAttribute = new Inference_NS::CAttribute(m_stInParam.strModelPath);
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
bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::unInit()
{
    if (m_pNonMotorizedAttribute)
    {
        delete m_pNonMotorizedAttribute;
        m_pNonMotorizedAttribute = nullptr;
    }
    return true;
}

/* 处理数据 */
bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::process(
    InData_S               stInData,
    std::vector<Result_S> &vstResult)
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
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    Inference_NS::InputData_S            stInputData;

    stInputData.pData     = (float *)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());

    bRet = m_pNonMotorizedAttribute->inference(stInputData, vClsDatas);

    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    for (int i = 0; i < vClsDatas.size(); i++)
    {
        const std::vector<Inference_NS::Cls_S> &vCls = vClsDatas[i].vCls;
        Result_S                                stResult;

        for (int j = 0; j < vCls.size(); j++)
        {
            const Inference_NS::Cls_S &cls = vCls[j];
            
            if (cls.nLabel >= (int)NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType::BICYCLE && cls.nLabel <= (int)NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType::THREE_WHEELER)
            {
                stResult.nNonMotorizedVehicleType = cls.nLabel;
                printf("类型：%d\n", cls.nLabel);
            }
            else if (cls.nLabel >= (int)NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::WHITE && cls.nLabel <= (int)NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::PURPLE)
            {
                stResult.nNonMotorizedVehicleColor = cls.nLabel;
                printf("车身颜色：%d\n", cls.nLabel);
            }
        }

        // PrintNonVehicleAttribute(stResult);

        // stRes.strName = vOutDatas[i].strName;
        // stRes.fConfidence = vOutDatas[i].fConfidence;
        vstResult.push_back(stResult);
    }

    return true;
}

bool NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int   imageWidth     = inputImage.cols;
    int   imageHeight    = inputImage.rows;
    float m_fResizeScale = std::min(static_cast<float>(m_nLimitWidth) / imageWidth, static_cast<float>(m_nLimitHeight) / imageHeight);

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    int Xoffset = m_nLimitWidth - newWidth;
    int Yoffset = m_nLimitHeight - newHeight;
    int left    = static_cast<int>(Xoffset / 2);
    int top     = static_cast<int>(Yoffset / 2);
    int right   = Xoffset - left;
    int bottom  = Yoffset - top;

    cv::copyMakeBorder(resizedImage, resizedImage, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(127, 127, 127));
    outputImage = resizedImage;
    return true;
}