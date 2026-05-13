/*
 * @FilePath     : NumberOcrV1_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 17:38:56
 * @Description  : 
 */
#include "NumberOcrV1_0.hpp"

#include "NumberOcr.hpp"
#include "SaveImage.hpp"


NumberOcr_NS::CNumberOcrV1_0::CNumberOcrV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

NumberOcr_NS::CNumberOcrV1_0::~CNumberOcrV1_0()
{
    unInit();
}

/* 初始化 */
bool NumberOcr_NS::CNumberOcrV1_0::init()
{
    bool bRet = false;

    m_pNumberOcr = new Inference_NS::CNumberOcr(m_stInParam.strModelPath);
    if (m_pNumberOcr)
    {
        if (m_pNumberOcr->init())
        {
            bRet = m_pNumberOcr->getSizeLimit(
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
bool NumberOcr_NS::CNumberOcrV1_0::unInit()
{
    if (m_pNumberOcr)
    {
        delete m_pNumberOcr;
        m_pNumberOcr = nullptr;
    }
    return true;
}

/* 处理数据 */
bool NumberOcr_NS::CNumberOcrV1_0::process(
    InData_S               stInData,
    int&                   nResult)
{
    std::vector<float> vecResult;
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pNumberOcr)
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
        // printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
        //        stInData.inMat.channels(),
        //        m_nLimitChannel);
        cv::cvtColor(stInData.inMat, stInData.inMat, cv::COLOR_RGB2GRAY);
    }
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth,m_nLimitHeight));
    }

    cv::threshold(stInData.inMat, stInData.inMat, 128, 255, cv::THRESH_BINARY);
    /* 推理+后处理 */
    bRet = m_pNumberOcr->inference(stInData.inMat, vecResult);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }


    if (vecResult.size() != 1)
    {
        bRet = false;
        printf("算法推理结果异常\n");
        return false;
    }
    nResult = (int) vecResult[0];

    return true;
}