/**
 * @file ShapeDetectV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 *
 * @brief 形状检测
 */
#include "ShapeDetectV1_0.hpp"

#include "ShapeDetect.hpp"
#include "SaveImage.hpp"

ShapeDetect_NS::CShapeDetectV1_0::CShapeDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

ShapeDetect_NS::CShapeDetectV1_0::~CShapeDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool ShapeDetect_NS::CShapeDetectV1_0::init()
{
    bool bRet = false;

    m_pShapeDetect = new Inference_NS::CShapeDetect();
    return true;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool ShapeDetect_NS::CShapeDetectV1_0::unInit()
{
    if (m_pShapeDetect)
    {
        delete m_pShapeDetect;
        m_pShapeDetect = nullptr;
    }
    return true;
}

/* 处理数据 */
bool ShapeDetect_NS::CShapeDetectV1_0::process(
    InData_S stInData,
    Inference_NS::InferRelust_S &stRelusts)
{
    std::vector<std::vector<cv::Point>> vApproxPolygons;
    vApproxPolygons.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pShapeDetect)
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

    /* 推理 */
    Inference_NS::InferParam_S stInferParam;
    stInferParam.dRCircularity = stInData.stParam.dRCircularity;
    stInferParam.dPCircularity = stInData.stParam.dPCircularity;
    stInferParam.fEpsilonNum = stInData.stParam.fEpsilonNum;

    bRet = m_pShapeDetect->shapeDetect(stInData.inMat, stInferParam, stRelusts);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    return true;
}
