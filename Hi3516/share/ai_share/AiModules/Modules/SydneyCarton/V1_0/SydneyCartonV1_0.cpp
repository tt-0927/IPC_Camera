/**
 * @file SydneyCartonV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 *
 * @brief 形状检测（只支持多边形检测）
 */
#include "SydneyCartonV1_0.hpp"

#include "ShapeDetect.hpp"
#include "SaveImage.hpp"

SydneyCarton_NS::CSydneyCartonV1_0::CSydneyCartonV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

SydneyCarton_NS::CSydneyCartonV1_0::~CSydneyCartonV1_0()
{
    unInit();
}

/* 初始化 */
bool SydneyCarton_NS::CSydneyCartonV1_0::init()
{
    bool bRet = false;

    m_pSydneyCarton = new Inference_NS::CShapeDetect();
    return true;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool SydneyCarton_NS::CSydneyCartonV1_0::unInit()
{
    if (m_pSydneyCarton)
    {
        delete m_pSydneyCarton;
        m_pSydneyCarton = nullptr;
    }
    return true;
}

/* 用于对轮廓进行排序的比较函数，按左端点的 x 坐标从小到大排序 */
bool compareContours(const std::vector<cv::Point> &a, const std::vector<cv::Point> &b)
{
    /* 获取轮廓的最左端的点的 x 坐标 */
    return cv::boundingRect(a).x < cv::boundingRect(b).x;
}

/* 处理数据 */
bool SydneyCarton_NS::CSydneyCartonV1_0::process(
    InData_S stInData,
    int &nResult)
{
    std::vector<std::vector<cv::Point>> vApproxPolygons;
    vApproxPolygons.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pSydneyCarton)
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

    /* 推理+后处理 */
    bRet = m_pSydneyCarton->inference(stInData.inMat, vApproxPolygons);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

#if 0
    if (vApproxPolygons.size() != 3)
    {
        nResult = -1;
    }
    else
    {
        int nStartNum = 3; /* 起始的多边形（三角形） */
        nResult = 0;
        /* 对多边形按照从左到右的顺序进行排序 */
        sort(vApproxPolygons.begin(), vApproxPolygons.end(), compareContours);
        /* 按顺序绘制逼近的多边形 */
        for (size_t i = 0; i < vApproxPolygons.size(); i++)
        {
            if (vApproxPolygons[i].size() > nStartNum + stInData.stParam.nShapeNum + 1 || vApproxPolygons[i].size() < nStartNum)
            {
                continue;
            }

            // for (int i = 0; i < vApproxPolygons[i].size(); i++)
            // {
            //     vApproxPolygons[i][i].x *= 4;
            //     vApproxPolygons[i][i].y *= 4;
            // }
            // cv::drawContours(aImageShow, std::vector<std::vector<cv::Point>>{vApproxPolygons[i]}, 0, cv::Scalar(0, 255, 0), 3);
            nResult += (vApproxPolygons[i].size() - nStartNum) * pow(5, stInData.stParam.nShapeNum - 1 - i);
        }
    }
#else
    /* 识别图像到四边形的上线 */

    int nStartNum = 4; /* 检测哪种多边形 */

    nResult = 0;       /*多边形的数量*/
    /* 按顺序绘制逼近的多边形 */
    for (size_t i = 0; i < vApproxPolygons.size(); i++)
    {
        if (vApproxPolygons[i].size() != nStartNum)
        {
            continue;
        }
        nResult +=1;
    }
#endif

    return true;
}
