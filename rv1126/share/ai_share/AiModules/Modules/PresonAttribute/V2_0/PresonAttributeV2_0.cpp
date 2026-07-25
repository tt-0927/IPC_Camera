/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-02-02 15:33:26
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:38:47
 * @FilePath: /1126/share/ai_share/AiModules/Modules/PresonAttribute/V2_0/PresonAttributeV2_0.cpp
 * @Description: 行人属性分析
 */

 #pragma once

#include "PresonAttributeV2_0.hpp"
#include "SaveImage.hpp"
#include <unistd.h>

PresonAttribute_NS::CPresonAttributeV2_0::CPresonAttributeV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

PresonAttribute_NS::CPresonAttributeV2_0::~CPresonAttributeV2_0()
{
    unInit();
}

/* 初始化 */
bool PresonAttribute_NS::CPresonAttributeV2_0::init()
{
    bool bRet = false;

    m_pPedestrianAttribute = new Inference_NS::CAttribute(m_stInParam.strModelPath);
    if (m_pPedestrianAttribute)
    {
        if (m_pPedestrianAttribute->init())
        {
            bRet = m_pPedestrianAttribute->getSizeLimit(
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
bool PresonAttribute_NS::CPresonAttributeV2_0::unInit()
{
    if (m_pPedestrianAttribute)
    {
        delete m_pPedestrianAttribute;
        m_pPedestrianAttribute = nullptr;
    }
    return true;
}

void printResult(const PresonAttribute_NS::Result_S &result, const std::string &title = "行人属性检测结果")
{
#if 1
    std::cout << "==================== " << title << " ====================" << std::endl;

    std::cout << "0. 年龄 (nAgeLabel): ";
    switch (result.nAgeLabel)
    {
    case 0:
        std::cout << "婴幼儿";
        break;
    case 1:
        std::cout << "青少年";
        break;
    case 2:
        std::cout << "中青年";
        break;
    case 3:
        std::cout << "中老年";
        break;
    case 4:
        std::cout << "老年";
        break;
    default:
        std::cout << "未知 (" << result.nAgeLabel << ")";
        break;
    }
    std::cout << std::endl;

    std::cout << "1. 是否是男性 (bIsMale): " << (result.bIsMale ? "是" : "否") << std::endl;

    std::cout << "2. 是否有双肩包 (bBackPack): " << (result.bBackPack ? "是" : "否") << std::endl;

    std::cout << "3. 下身穿着 (nBottomTypeLabel): ";
    switch (result.nBottomTypeLabel)
    {
    case 0:
        std::cout << "短裤";
        break;
    case 1:
        std::cout << "长裤";
        break;
    case 2:
        std::cout << "长裙";
        break;
    default:
        std::cout << "未知 (" << result.nBottomTypeLabel << ")";
        break;
    }
    std::cout << std::endl;

    std::cout << "4. 上身穿着颜色 (nTopColorLabel): ";
    switch (result.nTopColorLabel)
    {
    case 0:
        std::cout << "上身黑色";
        break;
    case 1:
        std::cout << "上身灰色";
        break;
    case 2:
        std::cout << "上身橙色";
        break;
    case 3:
        std::cout << "上身粉色";
        break;
    case 4:
        std::cout << "上身红色";
        break;
    case 5:
        std::cout << "上身白色";
        break;
    case 6:
        std::cout << "上身黄色";
        break;
    default:
        std::cout << "未知 (" << result.nTopColorLabel << ")";
        break;
    }
    std::cout << std::endl;

    std::cout << "5. 下身穿着颜色 (nBottomColorLabel): ";
    switch (result.nBottomColorLabel)
    {
    case 0:
        std::cout << "下身黑色";
        break;
    case 1:
        std::cout << "下身灰色";
        break;
    case 2:
        std::cout << "下身橙色";
        break;
    case 3:
        std::cout << "下身粉色";
        break;
    case 4:
        std::cout << "下身红色";
        break;
    case 5:
        std::cout << "下身白色";
        break;
    case 6:
        std::cout << "下身黄色";
        break;
    default:
        std::cout << "未知 (" << result.nBottomColorLabel << ")";
        break;
    }
    std::cout << std::endl;

    std::cout << "6. 上身穿着 (nTopTypeLabel): ";
    switch (result.nTopTypeLabel)
    {
    case 0:
        std::cout << "短袖";
        break;
    case 1:
        std::cout << "长袖";
        break;
    default:
        std::cout << "未知 (" << result.nTopTypeLabel << ")";
        break;
    }
    std::cout << std::endl;

    std::cout << "7. 是否有帽子 (bHat): " << (result.bHat ? "是" : "否") << std::endl;

    std::cout << "8. 是否有邮差包 (bPostManBag): " << (result.bPostManBag ? "是" : "否") << std::endl;

    std::cout << "9. 是否有手提袋 (bHandBag): " << (result.bHandBag ? "是" : "否") << std::endl;

    std::cout << "10. 是否有雨伞 (bUmbrella): " << (result.bUmbrella ? "是" : "否") << std::endl;

#endif
    std::cout << "========================================================" << std::endl;
}

/* 处理数据 */
bool PresonAttribute_NS::CPresonAttributeV2_0::process(InData_S stInData, std::vector<Result_S> &vecResult)
{
    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pPedestrianAttribute)
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
    Inference_NS::InputData_S            stInputData;
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    stInputData.pData     = (float *)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));

    bRet = m_pPedestrianAttribute->inference(stInputData, vClsDatas);

    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    for (int i = 0; i < vClsDatas.size(); i++)
    {
        const std::vector<Inference_NS::Cls_S> &vCls = vClsDatas[i].vCls;

        Result_S stResult;
        for (int j = 0; j < vCls.size(); j++)
        {
            const Inference_NS::Cls_S &cls        = vCls[j];
            // float                      confidence = cls.fConfidence;
            // printf("vClsDatas[%d]  组别[%d] nLabel[%d] - fConfidence[%f]\n", i, j, cls.nLabel, cls.fConfidence);

            // 年龄标签
            switch (cls.nLabel)
            {
            case (int)AgeLabel_E::AGE_0_14:
                stResult.nAgeLabel = 0;
                break;
            case (int)AgeLabel_E::AGE_15_29:
                stResult.nAgeLabel = 1;
                break;
            case (int)AgeLabel_E::AGE_30_44:
                stResult.nAgeLabel = 2;
                break;
            case (int)AgeLabel_E::AGE_45_59:
                stResult.nAgeLabel = 3;
                break;
            case (int)AgeLabel_E::AGE_60_PLUS:
                stResult.nAgeLabel = 4;
                break;
            // 性别标签
            case (int)GenderLabel_E::MALE:
                stResult.bIsMale = true;
                break;
            case (int)GenderLabel_E::FEMALE:
                stResult.bIsMale = false;
                break;
            // 物品标签
            case (int)ItemLabel_E::BACKPACK:
                stResult.bBackPack = true;
                break;
            case (int)ItemLabel_E::HAT:
                stResult.bHat = true;
                break;
            case (int)ItemLabel_E::POSTMAN_BAG:
                stResult.bPostManBag = 2;
                break;
            case (int)ItemLabel_E::HAND_BAG:
                stResult.bHandBag = true;
                break;
            case (int)ItemLabel_E::UMBRELLA:
                stResult.bUmbrella = true;
                break;
            /* 下装类型标签 */
            case (int)BottomTypeLabel_E::SHORT_PANTS:
                stResult.nBottomTypeLabel = 0;
                break;
            case (int)BottomTypeLabel_E::LONG_PANTS:
                stResult.nBottomTypeLabel = 1;
                break;
            case (int)BottomTypeLabel_E::LONG_SKIRT:
                stResult.nBottomTypeLabel = 2;
                break;
            // 上身颜色标签
            case (int)TopColorLabel_E::BLACK:
                stResult.nTopColorLabel = 0;
                break;
            case (int)TopColorLabel_E::GRAY:
                stResult.nTopColorLabel = 1;
                break;
            case (int)TopColorLabel_E::ORANGE:
                stResult.nTopColorLabel = 2;
                break;
            case (int)TopColorLabel_E::PINK:
                stResult.nTopColorLabel = 3;
                break;
            case (int)TopColorLabel_E::RED:
                stResult.nTopColorLabel = 4;
                break;
            case (int)TopColorLabel_E::WHITE:
                stResult.nTopColorLabel = 5;
                break;
            case (int)TopColorLabel_E::YELLOW:
                stResult.nTopColorLabel = 6;
                break;
            // 下身颜色标签
            case (int)BottomColorLabel_E::BLACK:
                stResult.nBottomColorLabel = 0;
                break;
            case (int)BottomColorLabel_E::GRAY:
                stResult.nBottomColorLabel = 1;
                break;
            case (int)BottomColorLabel_E::ORANGE:
                stResult.nBottomColorLabel = 2;
                break;
            case (int)BottomColorLabel_E::PINK:
                stResult.nBottomColorLabel = 3;
                break;
            case (int)BottomColorLabel_E::RED:
                stResult.nBottomColorLabel = 4;
                break;
            case (int)BottomColorLabel_E::WHITE:
                stResult.nBottomColorLabel = 5;
                break;
            case (int)BottomColorLabel_E::YELLOW:
                stResult.nBottomColorLabel = 6;
                break;
            // 上装类型标签
            case (int)TopTypeLabel_E::SHORT_SLEEVE:
                stResult.nTopTypeLabel = 0;
                break;
            case (int)TopTypeLabel_E::LONG_SLEEVE:
                stResult.nTopTypeLabel = 1;
                break;
            case (int)TopTypeLabel_E::VEST:
                stResult.nTopTypeLabel = 2;
                break;
            default:
                std::cout << "警告：j=" << j << " 超出预期范围（0~31）" << std::endl;
                break;
            }
        }
        vecResult.push_back(stResult);

        if (access("/print_personAttribute", F_OK) == 0)
        {
            printResult(stResult);
        }
        
    }

    return true;
}

bool PresonAttribute_NS::CPresonAttributeV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int   imageWidth     = inputImage.cols;
    int   imageHeight    = inputImage.rows;
    float m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
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
