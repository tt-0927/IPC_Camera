/**
 * @file ScrfdPostProcess.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-01
 * 
 * @brief 
 */

#include "ScrfdPostProcess.hpp"
#define nPointNum 5

/* 数据后处理 */
bool PostProcess_NS::cScrfdPostProcess::postProcess(std::vector<float *> vInput,
                                        int nHeight,
                                        int nWidth,
                                        float fConfThreshold,
                                        float fNmsThreshold,
                                        std::vector<Inference_NS::PointData_S> &vPointDatas)
{
    /* 存储容器 */
    std::vector<float> vfBoxes;
    std::vector<float> vfPoints;
    std::vector<float> vfObjProbs;

    /* 生成中心点信息 */
    generatePoints(nHeight, nWidth);

    /* stride 8 */
    int nStride1 = 8;
    int nGridH1 = nHeight / nStride1;
    int nGridW1 = nWidth / nStride1;
    int nNumPoints = nGridH1*nGridW1;
    processPoint(
        nNumPoints, 
        vInput[0], 
        vInput[3], 
        vInput[6], 
        8, 
        fConfThreshold,
        nHeight, 
        nWidth, 
        vfBoxes,
        vfPoints,
        vfObjProbs
    );

    /* stride 16 */
    int nStride2 = 16;
    int nGridH2 = nHeight / nStride2;
    int nGridW2 = nWidth / nStride2;
    nNumPoints = nGridH2*nGridW2;
    processPoint(
        nNumPoints, 
        vInput[1], 
        vInput[4], 
        vInput[7], 
        16, 
        fConfThreshold,
        nHeight, 
        nWidth, 
        vfBoxes,
        vfPoints,
        vfObjProbs
    );

    /* stride 32 */
    int nStride3 = 32;
    int nGridH3 = nHeight / nStride3;
    int nGridW3 = nWidth / nStride3;
    nNumPoints = nGridH3*nGridW3;
    processPoint(
        nNumPoints, 
        vInput[2],
         vInput[5], 
         vInput[8], 
         32, 
         fConfThreshold,nHeight, 
         nWidth, 
         vfBoxes,
         vfPoints,
         vfObjProbs
    );

    int nValidCount = vfObjProbs.size();
    /* 无框，直接返回 */
    if (nValidCount <= 0)
    {
        return true;
    }

    /* nms非极大值抑制 */
    std::vector<int> vnIndexArray;
    for (int ii = 0; ii < nValidCount; ii++)
    {
        vnIndexArray.push_back(ii);
    }
    quick_sort_indice_inverse(vfObjProbs, 0, nValidCount - 1, vnIndexArray);
    nms(nValidCount, vfBoxes, vnIndexArray, fNmsThreshold);

    vPointDatas.clear();
    /* box valid detect target */
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnIndexArray[i] == -1)
        {
            continue;
        }
        int n = vnIndexArray[i];

        float fX1 = vfBoxes[n * 4 + 0];
        float fY1 = vfBoxes[n * 4 + 1];
        float fX2 = vfBoxes[n * 4 + 2];
        float fY2 = vfBoxes[n * 4 + 3];
        float fObjConf = vfObjProbs[i];

        /* 存储结果 */
        Inference_NS::PointData_S stPointData;
        stPointData.stBoxs.nX1 =  fX1;
        stPointData.stBoxs.nY1 =  fY1;
        stPointData.stBoxs.nX2 =  fX2;
        stPointData.stBoxs.nY2 =  fY2;
        stPointData.fConfidence =  fObjConf;
        stPointData.nLabel = 0;

        for (int nP = 0; nP < nPointNum; nP++)
        {
            Inference_NS::Point_S stPoint;
            stPoint.nX = vfPoints[n * nPointNum * 2 + nP * 2 + 0];
            stPoint.nY = vfPoints[n * nPointNum * 2 + nP * 2 + 1];
            stPointData.vPoints.push_back(stPoint);
        }
        vPointDatas.push_back(stPointData);
    }

    return true;
}

/* 用于生成目标高度和宽度范围内的点 */
void PostProcess_NS::cScrfdPostProcess::generatePoints(const int nTargetHeight, const int nTargetWidth)
{
    /* 是否获取过网格信息 */
    if (bCenterPointsUpdate) return;

    /* 8, 16, 32 */
    for (auto nStride : vFeatStrideFpn)
    {
        unsigned int num_grid_w = nTargetWidth / nStride;
        unsigned int num_grid_h = nTargetHeight / nStride;
        /* u */
        for (unsigned int i = 0; i < num_grid_h; ++i)
        {
            /* x */
            for (unsigned int j = 0; j < num_grid_w; ++j)
            {
                /* nNumAnchors, col major */
                for (unsigned int k = 0; k < nNumAnchors; ++k)
                {
                    ScrfdPoint_S stPoint;
                    stPoint.fCx = (float)j;
                    stPoint.fCy = (float)i;
                    stPoint.fStride = (float)nStride;
                    mCenterPoints[nStride].push_back(stPoint);
                }
            }
        }
    }
    bCenterPointsUpdate = true;
}

/* 数据处理 */
void PostProcess_NS::cScrfdPostProcess::processPoint(
    int nNumPoints,
    float *pScore, 
    float *pBox,
    float *pKps, 
    int nStride, 
    float fConfThreshold, 
    float nHeight,
    float nWidth, 
    std::vector<float> &vfBoxes,
    std::vector<float> &vfPoints,
    std::vector<float> &vfObjProbs
)
{
    unsigned int nCount = 0;
    std::vector<ScrfdPoint_S> vStridePoints = mCenterPoints[nStride];

    for (unsigned int i = 0; i < nNumPoints; ++i)
    {
        const float fclsConf = pScore[i];
        if (fclsConf < fConfThreshold)
            continue; 
        /* 获取网格的信息 */
        const float fCx = vStridePoints.at(i).fCx;    // fCx
        const float fCy = vStridePoints.at(i).fCy;    // fCy
        const float fS = vStridePoints.at(i).fStride; // fStride

        /* 目标框 */
        const float *offsets = pBox + i * 4;
        float fLeft = offsets[0]; // left
        float fTop = offsets[1]; // top
        float fRight = offsets[2]; // right
        float fBottom = offsets[3]; // bottom
        /* 模型尺寸变换 */
        float fX1 = (fCx - fLeft) * fS; // fCx - l x1
        float fY1 = (fCy - fTop) * fS; // fCy - t y1
        float fX2 = (fCx + fRight) * fS; // fCx + r x2
        float fY2 = (fCy + fBottom) * fS; // fCy + b y2
        vfBoxes.push_back(fX1);
        vfBoxes.push_back(fY1);
        vfBoxes.push_back(fX2);
        vfBoxes.push_back(fY2);
        /* 置信度 */
        vfObjProbs.push_back(fclsConf);
        /* 关键点 */
        const float *kps_offsets = pKps + i * nPointNum*2;
        for (unsigned int j = 0; j < nPointNum*2; j += 2)
        {
            float fKpsLeft = kps_offsets[j];
            float fKpsTop = kps_offsets[j + 1];
            float fKpsX = (fCx + fKpsLeft) * fS; // fCx + l x
            float fKpsY = (fCy + fKpsTop) * fS; // fCy + t y
            float fPX = std::min(std::max(0.f, fKpsX), nWidth - 1.f);
            float fPy = std::min(std::max(0.f, fKpsY), nHeight - 1.f);
            vfPoints.push_back(fPX);
            vfPoints.push_back(fPy);
        }
        /* 限制非极大值抑制的个数 */
        nCount += 1; 
        if (nCount > nMaxNms)
        {
            break;
        }
    }
}

/* 快排算法 */
int PostProcess_NS::cScrfdPostProcess::quick_sort_indice_inverse(
    std::vector<float> &vfInput,
    int nLeft,
    int nRight,
    std::vector<int> &vnIndices)
{
    float fKey;
    int nKeyIndex;
    int nLow = nLeft;
    int nHigh = nRight;
    if (nLeft < nRight)
    {
        nKeyIndex = vnIndices[nLeft];
        fKey = vfInput[nLeft];
        while (nLow < nHigh)
        {
            while (nLow < nHigh && vfInput[nHigh] <= fKey)
            {
                nHigh--;
            }
            vfInput[nLow] = vfInput[nHigh];
            vnIndices[nLow] = vnIndices[nHigh];
            while (nLow < nHigh && vfInput[nLow] >= fKey)
            {
                nLow++;
            }
            vfInput[nHigh] = vfInput[nLow];
            vnIndices[nHigh] = vnIndices[nLow];
        }
        vfInput[nLow] = fKey;
        vnIndices[nLow] = nKeyIndex;
        quick_sort_indice_inverse(vfInput, nLeft, nLow - 1, vnIndices);
        quick_sort_indice_inverse(vfInput, nLow + 1, nRight, vnIndices);
    }
    return nLow;
}

/* 非极大值抑制 */
int PostProcess_NS::cScrfdPostProcess::nms(
    int nValidCount,
    std::vector<float> &nfOutputLocations,
    std::vector<int> &vnOrder,
    float fThreshold)
{
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnOrder[i] == -1)
        {
            continue;
        }
        int n = vnOrder[i];
        for (int j = i + 1; j < nValidCount; ++j)
        {
            int m = vnOrder[j];
            if (m == -1)
            {
                continue;
            }

            float fXmin0 = nfOutputLocations[n * 4 + 0];
            float fYmin0 = nfOutputLocations[n * 4 + 1];
            float fXmax0 = nfOutputLocations[n * 4 + 0] + nfOutputLocations[n * 4 + 2];
            float fYmax0 = nfOutputLocations[n * 4 + 1] + nfOutputLocations[n * 4 + 3];

            float fXmin1 = nfOutputLocations[m * 4 + 0];
            float fYmin1 = nfOutputLocations[m * 4 + 1];
            float fXmax1 = nfOutputLocations[m * 4 + 0] + nfOutputLocations[m * 4 + 2];
            float fYmax1 = nfOutputLocations[m * 4 + 1] + nfOutputLocations[m * 4 + 3];

            float fIou = CalculateOverlap(fXmin0, fYmin0, fXmax0, fYmax0, fXmin1, fYmin1, fXmax1, fYmax1);

            if (fIou > fThreshold)
            {
                vnOrder[j] = -1;
            }
        }
    }
    return 0;
}

/* IOU计算 */
float PostProcess_NS::cScrfdPostProcess::CalculateOverlap(
    float fXmin0,
    float fYmin0,
    float fXmax0,
    float fYmax0,
    float fXmin1,
    float fYmin1,
    float fXmax1,
    float fYmax1)
{
    float fW = fmax(0.f, fmin(fXmax0, fXmax1) - fmax(fXmin0, fXmin1) + 1.0);
    float fH = fmax(0.f, fmin(fYmax0, fYmax1) - fmax(fYmin0, fYmin1) + 1.0);
    float i = fW * fH;
    float u = (fXmax0 - fXmin0 + 1.0) * (fYmax0 - fYmin0 + 1.0) + (fXmax1 - fXmin1 + 1.0) * (fYmax1 - fYmin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}
