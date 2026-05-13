/*
 * @FilePath     : VideoAnomalyDetectV1_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 17:38:56
 * @Description  : 人少场景
 */
#include "VideoAnomalyDetectExt.hpp"
#include "VideoAnomalyDetectV1_0.hpp"
#include "SaveImage.hpp"
#include <cmath>

using namespace VideoAnomalyDetect_NS;
using namespace Inference_NS;

VideoAnomalyDetect_NS::CVideoAnomalyDetectV1_0::CVideoAnomalyDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

VideoAnomalyDetect_NS::CVideoAnomalyDetectV1_0::~CVideoAnomalyDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool VideoAnomalyDetect_NS::CVideoAnomalyDetectV1_0::init()
{
    bool bRet = false;

    m_pImageAnomaly = new Inference_NS::cImageAnomaly;
    if (m_pImageAnomaly)
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("[%s] 算法初始化失败 \n",
               "CVideoAnomalyDetectV1_0");
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool VideoAnomalyDetect_NS::CVideoAnomalyDetectV1_0::unInit()
{
    if (m_pImageAnomaly)
    {
        delete m_pImageAnomaly;
        m_pImageAnomaly = nullptr;
    }
    return true;
}

/* 处理数据 */
bool VideoAnomalyDetect_NS::CVideoAnomalyDetectV1_0::process(
    InData_S stInData,
    Result_S &vecResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pImageAnomaly)
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
        cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
    }

    /* 去背景 */
    cv::Mat aColorMask, aAnomalyMask, aAnomalyFrame;
    m_pImageAnomaly->detectRedAnomaly(
        stInData.inMat,
        stInData.stParam.vLowerColor,
        stInData.stParam.vUpperColor,
        aColorMask,
        aAnomalyMask,
        aAnomalyFrame);

    // m_pImageAnomaly->detectRedAnomaly(
    //     stInData.inMat,
    //     aColorMask,
    //     aAnomalyMask,
    //     aAnomalyFrame
    // );

    /* 条纹检测 */
    if (stInData.stParam.stStripesParam.bEnable)
    {
        std::vector<cv::Vec4i> vLines, vNewLines;
        m_pImageAnomaly->detectStripes(aAnomalyFrame, vLines);
        /* 过滤直线 */
        for (int nS = 0; nS < vLines.size(); nS++)
        {
            cv::Vec4i vPointData = vLines[nS];
            float fRes = sqrt(pow(vPointData[2] - vPointData[0], 2) + pow(vPointData[3] - vPointData[1], 2));
            if (fRes > stInData.stParam.stStripesParam.nLineSize)
            {
                vNewLines.push_back(vPointData);
            }
        }

        if (vNewLines.size() > 0)
        {
            vecResult.bStripesFlag = true;
        }
        // std::cout << "出现条纹的个数为：" << vLines.size() << "=========== " << vNewLines.size() << " =========" << std::endl;
    }
    /* 亮暗检测算法 */
    if (stInData.stParam.stLightDarkParam.bEnable)
    {
        double dAreaBrightness = m_pImageAnomaly->getLight(stInData.inMat, aColorMask);
        if (abs(dAreaBrightness) > stInData.stParam.stLightDarkParam.fLDThres)
        {
            vecResult.bLightDarkFlag = true;
        }
        // std::cout << "前后两帧平均亮度差为：=========== " << dAreaBrightness << " =========" << std::endl;
    }
    /* 噪点检测算法 */
    if (stInData.stParam.stNoiseParam.bEnable)
    {
        int nZs = m_pImageAnomaly->countNoise(aAnomalyMask);
        float fZb = nZs * 1.0 / aAnomalyMask.total();
        /*if (fZb > stInData.stParam.stNoiseParam.fProportion)
        {
            vecResult.bNoiseFlag = true;
        }*/
        if (fZb > stInData.stParam.stNoiseParam.fSolidColorThes)
        {
            vecResult.bSolidColorFlag = true;
        }
		else if (fZb > stInData.stParam.stNoiseParam.fProportion)
        {
            vecResult.bNoiseFlag = true;
        }
        // std::cout << "噪点检测算法占比以及占比个数：=========== " << fZb << ";" << nZs << " =========" << std::endl;
    }

    /* 图片模糊度检测 */
    if(stInData.stParam.stBlurrinessParam.bEnable)
    {
        double dMh = m_pImageAnomaly->assessImageSharpness(stInData.inMat);
        if(dMh < stInData.stParam.stBlurrinessParam.dBlurrThres)
        {
            vecResult.bBlurrinessFlag = true;
        }
        // std::cout<<"图片模糊度程度：=========== "<< dMh <<" =========" <<std::endl;
    }

    if (m_stInParam.bDebug)
    {
        /* 保存分析后的图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strAnalyzeDataPath.empty())
        {
        }
    }

    return true;
}
