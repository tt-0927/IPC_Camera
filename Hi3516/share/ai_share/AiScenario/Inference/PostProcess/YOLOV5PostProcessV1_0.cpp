/*
 * @FilePath     : YOLOV5PostProcessV1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:33
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-05-31 13:47:39
 * @Description  :
 */

#include "YOLOV5PostProcessV1_0.hpp"
#include <set>

#include "dlog.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

/* 数据后处理 */
bool PostProcessV1_0_NS::cYOLOV5PostProcess::postProcess(
	float*              pfInput0,
	float*              pfInput1,
	float*              pfInput2,
	int                 nHeight,
	int                 nWidth,
	float               nConfThreshold,
	float               nNmsThreshold,
	int                 nCLASS_NUM,
	std::vector<float>& vPoints)
{
    if (nullptr == pfInput0 ||
        nullptr == pfInput1 ||
        nullptr == pfInput2 ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    /* 识别种类 */
    m_nOBJ_CLASS_NUM = nCLASS_NUM;

    std::vector<float> vfFilterBoxes;
    std::vector<float> vfObjProbs;
    std::vector<int>   vnClassId;

    /* stride 8 -- input(640,640,3) */
    int nStride0     = 8;
    int nGridH0      = nHeight / nStride0;    // 80
    int nGridW0      = nWidth / nStride0;     // 80
    int nValidCount0 = 0;

    nValidCount0 = process(pfInput0,
                           (int*)m_nAnchor0,
                           nGridH0,
                           nGridW0,
                           nHeight,
                           nWidth,
                           nStride0,
                           vfFilterBoxes,
                           vfObjProbs,
                           vnClassId,
                           nConfThreshold);

    /* stride 16 */
    int nStride1     = 16;
    int nGridH1      = nHeight / nStride1;
    int nGridW1      = nWidth / nStride1;
    int nValidCount1 = 0;

    nValidCount1 = process(pfInput1,
                           (int*)m_nAnchor1,
                           nGridH1,
                           nGridW1,
                           nHeight,
                           nWidth,
                           nStride1,
                           vfFilterBoxes,
                           vfObjProbs,
                           vnClassId,
                           nConfThreshold);

    /* stride 32 */
    int nStride2     = 32;
    int nGridH2      = nHeight / nStride2;
    int nGridW2      = nWidth / nStride2;
    int nValidCount2 = 0;

    nValidCount2 = process(pfInput2,
                           (int*)m_nAnchor2,
                           nGridH2,
                           nGridW2,
                           nHeight,
                           nWidth,
                           nStride2,
                           vfFilterBoxes,
                           vfObjProbs,
                           vnClassId,
                           nConfThreshold);

    int nValidCount = nValidCount0 + nValidCount1 + nValidCount2;

    // no object detect
    if (nValidCount <= 0)
    {
        return 0;
    }

    std::vector<int> vnIndexArray;
    for (int i = 0; i < nValidCount; ++i)
    {
        vnIndexArray.push_back(i);
    }

    quick_sort_indice_inverse(vfObjProbs, 0, nValidCount - 1, vnIndexArray);

    std::set<int> class_set(std::begin(vnClassId), std::end(vnClassId));

    for (auto c : class_set)
    {
        nms(nValidCount, vfFilterBoxes, vnClassId, vnIndexArray, c, nNmsThreshold);
    }

    /* box valid detect target */
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnIndexArray[i] == -1)
        {
            continue;
        }
        int n = vnIndexArray[i];

        float fX1      = vfFilterBoxes[n * 4 + 0];
        float fY1      = vfFilterBoxes[n * 4 + 1];
        float fX2      = fX1 + vfFilterBoxes[n * 4 + 2];
        float fY2      = fY1 + vfFilterBoxes[n * 4 + 3];
        int   nId      = vnClassId[n];
        float fObjConf = vfObjProbs[i];
        // printf("(%f,%f),(%f,%f),%d\n",fX1,fY1,fX2,fY2,nId);

        vPoints.push_back(clamp(fX1, 0, nWidth));
        vPoints.push_back(clamp(fY1, 0, nHeight));
        vPoints.push_back(clamp(fX2, 0, nWidth));
        vPoints.push_back(clamp(fY2, 0, nHeight));
        vPoints.push_back(fObjConf);
        vPoints.push_back(nId);
    }

    return true;
}

float PostProcessV1_0_NS::cYOLOV5PostProcess::sigmoid(float fX)
{
    return 1.0 / (1.0 + expf(-fX));
}

float PostProcessV1_0_NS::cYOLOV5PostProcess::unsigmoid(float fY)
{
    return -1.0 * logf((1.0 / fY) - 1.0);
}

int PostProcessV1_0_NS::cYOLOV5PostProcess::quick_sort_indice_inverse(
    std::vector<float>& vfInput,
    int                 nLeft,
    int                 nRight,
    std::vector<int>&   vnIndices)
{
    float fKey;
    int   nKeyIndex;
    int   nLow  = nLeft;
    int   nHigh = nRight;
    if (nLeft < nRight)
    {
        nKeyIndex = vnIndices[nLeft];
        fKey      = vfInput[nLeft];
        while (nLow < nHigh)
        {
            while (nLow < nHigh && vfInput[nHigh] <= fKey)
            {
                nHigh--;
            }
            vfInput[nLow]   = vfInput[nHigh];
            vnIndices[nLow] = vnIndices[nHigh];
            while (nLow < nHigh && vfInput[nLow] >= fKey)
            {
                nLow++;
            }
            vfInput[nHigh]   = vfInput[nLow];
            vnIndices[nHigh] = vnIndices[nLow];
        }
        vfInput[nLow]   = fKey;
        vnIndices[nLow] = nKeyIndex;
        quick_sort_indice_inverse(vfInput, nLeft, nLow - 1, vnIndices);
        quick_sort_indice_inverse(vfInput, nLow + 1, nRight, vnIndices);
    }
    return nLow;
}

int PostProcessV1_0_NS::cYOLOV5PostProcess::nms(
    int                 nValidCount,
    std::vector<float>& nfOutputLocations,
    std::vector<int>    vnClassIds,
    std::vector<int>&   vnOrder,
    int                 nFilterId,
    float               fThreshold)
{
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnOrder[i] == -1 || vnClassIds[i] != nFilterId)
        {
            continue;
        }
        int n = vnOrder[i];
        for (int j = i + 1; j < nValidCount; ++j)
        {
            int m = vnOrder[j];
            if (m == -1 || vnClassIds[i] != nFilterId)
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

float PostProcessV1_0_NS::cYOLOV5PostProcess::CalculateOverlap(
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
    float i  = fW * fH;
    float u  = (fXmax0 - fXmin0 + 1.0) * (fYmax0 - fYmin0 + 1.0) + (fXmax1 - fXmin1 + 1.0) * (fYmax1 - fYmin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

int PostProcessV1_0_NS::cYOLOV5PostProcess::process(
    float*              pfInput,
    int*                pnAnchor,
    int                 nGridH,
    int                 nGridW,
    int                 nHeight,
    int                 nWidth,
    int                 nStride,
    std::vector<float>& vfBoxes,
    std::vector<float>& vfObjProbs,
    std::vector<int>&   vnClassId,
    float               fThreshold)
{
    int nValidCount = 0;
    int nGridLen    = nGridH * nGridW;
    for (int a = 0; a < 3; a++)
    {
        for (int i = 0; i < nGridH; i++)
        {
            for (int j = 0; j < nGridW; j++)
            {
                // (5+m_nOBJ_CLASS_NUM)*(i3)+4
                int nOffset = (a * (5 + m_nOBJ_CLASS_NUM) + 4) * nGridLen + i * nGridW + j;

                float fBoxConfidence = sigmoid(pfInput[nOffset]);

                if (fBoxConfidence >= fThreshold)
                {

                    nOffset     = i * nGridW + j;
                    float fBoxX = sigmoid(pfInput[nOffset + (a * (5 + m_nOBJ_CLASS_NUM) + 0) * nGridLen]) * 2.0 - 0.5;
                    float fBoxY = sigmoid(pfInput[nOffset + (a * (5 + m_nOBJ_CLASS_NUM) + 1) * nGridLen]) * 2.0 - 0.5;
                    float fBoxW = sigmoid(pfInput[nOffset + (a * (5 + m_nOBJ_CLASS_NUM) + 2) * nGridLen]) * 2.0;
                    float fBoxH = sigmoid(pfInput[nOffset + (a * (5 + m_nOBJ_CLASS_NUM) + 3) * nGridLen]) * 2.0;

                    fBoxX  = (fBoxX + j) * (float)nStride;
                    fBoxY  = (fBoxY + i) * (float)nStride;
                    fBoxW  = fBoxW * fBoxW * (float)pnAnchor[a * 2];
                    fBoxH  = fBoxH * fBoxH * (float)pnAnchor[a * 2 + 1];
                    fBoxX -= (fBoxW / 2.0);
                    fBoxY -= (fBoxH / 2.0);
                    vfObjProbs.push_back(fBoxConfidence);

                    float fMaxClassProbs = 0;
                    int   nMaxClassId    = 0;

                    for (int k = 0; k < m_nOBJ_CLASS_NUM; k++)
                    {
                        float fProb = sigmoid(pfInput[nOffset + (a * (5 + m_nOBJ_CLASS_NUM) + 5 + k) * nGridLen]);
                        if (fProb > fMaxClassProbs)
                        {
                            nMaxClassId    = k;
                            fMaxClassProbs = fProb;
                        }
                    }

                    vnClassId.push_back(nMaxClassId);
                    nValidCount++;
                    vfBoxes.push_back(fBoxX);
                    vfBoxes.push_back(fBoxY);
                    vfBoxes.push_back(fBoxW);
                    vfBoxes.push_back(fBoxH);
                }
            }
        }
    }
    return nValidCount;
}
