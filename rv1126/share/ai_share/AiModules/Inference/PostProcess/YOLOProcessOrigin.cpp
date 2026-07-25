/**
 * @file YOLOProcessOrigin.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-17
 *
 * @brief
 */

#include "YOLOProcessOrigin.hpp"

#include <set>

#include <vector>
#include <algorithm>


/* 目标检测数据后处理 */
bool PostProcess_NS::cYOLOProcessOrigin::postProcess(
    std::vector<float *> vInput,
    int nWidth,
    int nHeight,
    int nClsNum,
    int nAnchorsNum,
    float fConfThreshold,
    float FNmsThreshold,
    std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    if (vInput.size() == 0 ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        printf("传入参数为空\n");
        return false;
    }

    vBoxDatas.clear();
    float *pPred = vInput[0]; /* 第0个输入数据 [1,84,8400] */
    for (int dn = 0; dn < nAnchorsNum; ++dn)
    {
        int nCls = -1;
        float fMaxConf = 0.0;
        for (int dc = 0; dc < nClsNum; ++dc)
        {
            float conf = pPred[(4 + dc) * nAnchorsNum + dn];
            if (fMaxConf < conf)
            {
                nCls = dc;
                fMaxConf = conf;
            }
        }

        if (fMaxConf < fConfThreshold)
        {
            continue;
        }

        float fCx = pPred[0 * nAnchorsNum + dn];
        float fCy = pPred[1 * nAnchorsNum + dn];
        float fW =  pPred[2 * nAnchorsNum + dn];
        float fH =  pPred[3 * nAnchorsNum + dn];
        /* 边框大小过滤 */
        if (fW < m_fMinWH || fH < m_fMinWH || fW > m_fMaxWH || fH > m_fMaxWH)
        {
            continue;
        }
        /* scale_coords */
        int nX1 = int(fCx - fW * 0.5f);
        int nY1 = int(fCy - fH * 0.5f);
        int nX2 = int(fCx + fW * 0.5f);
        int nY2 = int(fCy + fH * 0.5f);
        /* clip */
        nX1 = nX1 < 0 ? 0 : nX1;
        nY1 = nY1 < 0 ? 0 : nY1;
        nX2 = nX2 >= nWidth ? nWidth - 1 : nX2;
        nY2 = nY2 >= nHeight ? nHeight - 1 : nY2;

        /* 存于容器 */
        Inference_NS::BoxData_S stOneBoxData;
        stOneBoxData.stBoxs.nX1 = nX1;
        stOneBoxData.stBoxs.nY1 = nY1;
        stOneBoxData.stBoxs.nX2 = nX2;
        stOneBoxData.stBoxs.nY2 = nY2;

        stOneBoxData.nLabel = nCls;
        stOneBoxData.fConfidence = fMaxConf;
        vBoxDatas.emplace_back(stOneBoxData);
    }

    if (!vBoxDatas.empty())
    {
        /* nms */
        nonMaxSuppression(vBoxDatas, FNmsThreshold);
    }

    return true;
}

/* 关键点检测数据后处理 */
bool PostProcess_NS::cYOLOProcessOrigin::postProcessPoint(
    std::vector<float *> vInput,
    int nHeight,
    int nWidth,
    float fConfThreshold,
    float FNmsThreshold,
    int nClassNUM,
    int nPointNum,
    std::vector<Inference_NS::PointData_S> &vPointDatas)
{
    return true;
}

/* 计算IOU */
float PostProcess_NS::cYOLOProcessOrigin::bboxOverlap(const Inference_NS::Box_S &stVi, const Inference_NS::Box_S &stVo)
{
    int nBX1 = std::max(stVi.nX1, stVo.nX1);
    int nBY1 = std::max(stVi.nY1, stVo.nY1);
    int nBX2 = std::min(stVi.nX2, stVo.nX2);
    int nBY2 = std::min(stVi.nY2, stVo.nY2);

    int nW = std::max(0, nBX2 - nBX1);
    int nH = std::max(0, nBY2 - nBY1);

    int nArea = nW * nH;
    if (nArea <= 0.0)
    {
        return 0.0;
    }
    float fDist = float(nArea) / float((stVi.nX2 - stVi.nX1) * (stVi.nY2 - stVi.nY1) +
                                       (stVo.nY2 - stVo.nY1) * (stVo.nX2 - stVo.nX1) - nArea);
    return fDist;
}
/* 非极大值抑制 */
bool PostProcess_NS::cYOLOProcessOrigin::nonMaxSuppression(std::vector<Inference_NS::BoxData_S> &vBoxDatas, float FNmsThreshold)
{
    /* 置信度排序 */
    std::sort(vBoxDatas.begin(), vBoxDatas.end(),
              [](const Inference_NS::BoxData_S &d1, const Inference_NS::BoxData_S &d2)
              { return d1.fConfidence > d2.fConfidence; });

    /* 非极大值抑制 */
    std::vector<Inference_NS::BoxData_S> vKeeDetections;
    std::vector<char> vSuppressed(vBoxDatas.size(), 0);
    int nNumDetections = vBoxDatas.size();
    for (int i = 0; i < nNumDetections; ++i)
    {
        if (vSuppressed[i])
        {
            continue;
        }
        vKeeDetections.emplace_back(vBoxDatas[i]);
        for (int j = i + 1; j < nNumDetections; ++j)
        {
            if (vSuppressed[j])
            {
                continue;
            }
            float fIou = bboxOverlap(vBoxDatas[i].stBoxs, vBoxDatas[j].stBoxs);
            if (fIou > FNmsThreshold)
            {
                vSuppressed[j] = 1;
            }
        }
    }
    vKeeDetections.swap(vBoxDatas);

    return true;
} 