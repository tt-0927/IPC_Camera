/**
 * @file CameraObstructionV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-04
 * 
 * @brief 摄像头遮挡算法
 */
#include "CameraObstructionExt.hpp"
#include "CameraObstructionV1_0.hpp"
#include "SaveImage.hpp"
#include <cmath>

using namespace CameraObstruction_NS;
using namespace Inference_NS;

CameraObstruction_NS::CCameraObstructionV1_0::CCameraObstructionV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

CameraObstruction_NS::CCameraObstructionV1_0::~CCameraObstructionV1_0()
{
    unInit();
}

/* 初始化 */
bool CameraObstruction_NS::CCameraObstructionV1_0::init()
{
    bool bRet = false;

    m_pComplexityDetect = new Inference_NS::CComplexityDetect;
    if (m_pComplexityDetect)
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("[%s] 算法初始化失败 \n",
               "CCameraObstructionV1_0");
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool CameraObstruction_NS::CCameraObstructionV1_0::unInit()
{
    if (m_pComplexityDetect)
    {
        delete m_pComplexityDetect;
        m_pComplexityDetect = nullptr;
    }
    return true;
}

/* 处理数据 */
bool CameraObstruction_NS::CCameraObstructionV1_0::process(
    InData_S stInData,
    Result_S &vecResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pComplexityDetect)
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

    /* 摄像头遮挡检测算法 */
    double dRes = -1;
    m_pComplexityDetect->inference(stInData.inMat, dRes);

    if (dRes > stInData.stParam.dThres)
    {
        vecResult.bBlockFlag = true;
    }
    else
    {
        vecResult.bBlockFlag = false;
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
