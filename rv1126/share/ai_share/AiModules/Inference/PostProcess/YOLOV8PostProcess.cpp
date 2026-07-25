/*
 * @FilePath     : YOLOV5PostProcess.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 20:07:54
 * @Description  :
 */

#include "YOLOV8PostProcess.hpp"
#include <cmath>
#include <set>

/* 检测数据后处理 */
bool PostProcess_NS::cYOLOV8PostProcess::postProcessDetect(
    std::vector<float *> vfInput,
    int nHeight,
    int nWidth,
    float nConfThreshold,
    float nNmsThreshold,
    int nCLASS_NUM,
    std::vector<Inference_NS::BoxData_S> &vBoxDatas,
    int nFlLen)
{

    if (0 == vfInput.size() ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        printf("传入参数为空\n");
        return false;
    }

    /* 识别种类 */
    m_nOBJ_CLASS_NUM = nCLASS_NUM;

    std::vector<float> vfFilterBoxes;
    std::vector<float> vfObjProbs;
    std::vector<int> vnClassId;
    // int nFlLen = 16;

    /* stride 8 -- input(640,640,3) */
    int nStride0 = 8;
    int nGridH0 = nHeight / nStride0; // 80
    int nGridW0 = nWidth / nStride0;  // 80
    int nValidCount0 = 0;

    nValidCount0 = processDetect((float *)vfInput[0],
                                 (float *)vfInput[1],
                                 (float *)vfInput[2],
                                 nGridH0,
                                 nGridW0,
                                 nStride0,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

    /* stride 16 */
    int nStride1 = 16;
    int nGridH1 = nHeight / nStride1;
    int nGridW1 = nWidth / nStride1;
    int nValidCount1 = 0;

    nValidCount1 = processDetect((float *)vfInput[3],
                                 (float *)vfInput[4],
                                 (float *)vfInput[5],
                                 nGridH1,
                                 nGridW1,
                                 nStride1,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

    /* stride 32 */
    int nStride2 = 32;
    int nGridH2 = nHeight / nStride2;
    int nGridW2 = nWidth / nStride2;
    int nValidCount2 = 0;

    nValidCount2 = processDetect((float *)vfInput[6],
                                 (float *)vfInput[7],
                                 (float *)vfInput[8],
                                 nGridH2,
                                 nGridW2,
                                 nStride2,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

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

    vBoxDatas.clear();
    /* box valid detect target */
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnIndexArray[i] == -1)
        {
            continue;
        }
        int n = vnIndexArray[i];

        float fX1 = vfFilterBoxes[n * 4 + 0];
        float fY1 = vfFilterBoxes[n * 4 + 1];
        float fX2 = fX1 + vfFilterBoxes[n * 4 + 2];
        float fY2 = fY1 + vfFilterBoxes[n * 4 + 3];
        int nId = vnClassId[n];
        float fObjConf = vfObjProbs[i];
        //printf("(%f,%f),(%f,%f),%d,%f\n", fX1, fY1, fX2, fY2, nId, fObjConf);

        Inference_NS::BoxData_S stBoxData;
        stBoxData.stBoxs.nX1 = clamp(fX1, 0, nWidth);
        stBoxData.stBoxs.nY1 = clamp(fY1, 0, nHeight);
        stBoxData.stBoxs.nX2 = clamp(fX2, 0, nWidth);
        stBoxData.stBoxs.nY2 = clamp(fY2, 0, nHeight);
        stBoxData.fConfidence = fObjConf;
        stBoxData.nLabel = nId;
        vBoxDatas.push_back(stBoxData);
    }

    return true;
}

/* 检测数据后处理 */
bool PostProcess_NS::cYOLOV8PostProcess::postProcessLiteDetect(
    std::vector<float *> vfInput,
    int nHeight,
    int nWidth,
    float nConfThreshold,
    float nNmsThreshold,
    int nCLASS_NUM,
    std::vector<Inference_NS::BoxData_S> &vBoxDatas,
    int nFlLen)
{

    if (0 == vfInput.size() ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        printf("传入参数为空\n");
        return false;
    }

    /* 识别种类 */
    m_nOBJ_CLASS_NUM = nCLASS_NUM;

    std::vector<float> vfFilterBoxes;
    std::vector<float> vfObjProbs;
    std::vector<int> vnClassId;
    // int nFlLen = 16;

    /* stride 8 -- input(640,640,3) */
    int nStride0 = 8;
    int nGridH0 = nHeight / nStride0; // 80
    int nGridW0 = nWidth / nStride0;  // 80
    int nValidCount0 = 0;

    nValidCount0 = processDetect((float *)vfInput[0],
                                 (float *)vfInput[1],
                                 (float *)vfInput[2],
                                 nGridH0,
                                 nGridW0,
                                 nStride0,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

    /* stride 16 */
    int nStride1 = 16;
    int nGridH1 = nHeight / nStride1;
    int nGridW1 = nWidth / nStride1;
    int nValidCount1 = 0;

    nValidCount1 = processDetect((float *)vfInput[3],
                                 (float *)vfInput[4],
                                 (float *)vfInput[5],
                                 nGridH1,
                                 nGridW1,
                                 nStride1,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

    /* stride 32 */
    int nStride2 = 32;
    int nGridH2 = nHeight / nStride2;
    int nGridW2 = nWidth / nStride2;
    int nValidCount2 = 0;

    nValidCount2 = processDetect((float *)vfInput[6],
                                 (float *)vfInput[7],
                                 (float *)vfInput[8],
                                 nGridH2,
                                 nGridW2,
                                 nStride2,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

    /* stride 64 */
    int nStride3 = 64;
    int nGridH3 = nHeight / nStride3;
    int nGridW3 = nWidth / nStride3;
    int nValidCount3 = 0;

    nValidCount3 = processDetect((float *)vfInput[6],
                                 (float *)vfInput[7],
                                 (float *)vfInput[8],
                                 nGridH3,
                                 nGridW3,
                                 nStride3,
                                 nFlLen,
                                 vfFilterBoxes,
                                 vfObjProbs,
                                 vnClassId,
                                 nConfThreshold,
                                 nCLASS_NUM);

    int nValidCount = nValidCount0 + nValidCount1 + nValidCount2 + nValidCount3;

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

    vBoxDatas.clear();
    /* box valid detect target */
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnIndexArray[i] == -1)
        {
            continue;
        }
        int n = vnIndexArray[i];

        float fX1 = vfFilterBoxes[n * 4 + 0];
        float fY1 = vfFilterBoxes[n * 4 + 1];
        float fX2 = fX1 + vfFilterBoxes[n * 4 + 2];
        float fY2 = fY1 + vfFilterBoxes[n * 4 + 3];
        int nId = vnClassId[n];
        float fObjConf = vfObjProbs[i];
        // printf("(%f,%f),(%f,%f),%d,%f\n", fX1, fY1, fX2, fY2, nId, fObjConf);

        Inference_NS::BoxData_S stBoxData;
        stBoxData.stBoxs.nX1 = clamp(fX1, 0, nWidth);
        stBoxData.stBoxs.nY1 = clamp(fY1, 0, nHeight);
        stBoxData.stBoxs.nX2 = clamp(fX2, 0, nWidth);
        stBoxData.stBoxs.nY2 = clamp(fY2, 0, nHeight);
        stBoxData.fConfidence = fObjConf;
        stBoxData.nLabel = nId;
        vBoxDatas.push_back(stBoxData);
    }

    return true;
}

/* 关键点数据后处理 */
bool PostProcess_NS::cYOLOV8PostProcess::postProcessKeyPoint(
    std::vector<float *> vfInput,
    int nHeight,
    int nWidth,
    float nConfThreshold,
    float nNmsThreshold,
    int nCLASS_NUM,
    int nKeyPoint_NUM,
    std::vector<Inference_NS::PointData_S> &vPointDatas,
    int nFlLen,
    bool bPointShow)
{
    if (0 == vfInput.size() ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        printf("传入参数为空\n");
        return false;
    }

    /* 识别种类 */
    m_nOBJ_CLASS_NUM = nCLASS_NUM;
    /* 关键点数量 */
    nPointNum = nKeyPoint_NUM;

    std::vector<float> vfFilterBoxes;
    std::vector<float> vfObjProbs;
    std::vector<float> vfPoints;
    std::vector<int> vnClassId;
    // int nFlLen = 16;

    /* stride 8 -- input(640,640,3) */
    int nStride0 = 8;
    int nGridH0 = nHeight / nStride0; // 80
    int nGridW0 = nWidth / nStride0;  // 80
    int nValidCount0 = 0;

    nValidCount0 = processKeyPoint((float *)vfInput[0],
                                   (float *)vfInput[1],
                                   (float *)vfInput[2],
                                   (float *)vfInput[9],
                                   nGridH0,
                                   nGridW0,
                                   nStride0,
                                   nFlLen,
                                   vfFilterBoxes,
                                   vfObjProbs,
                                   vfPoints,
                                   vnClassId,
                                   nConfThreshold,
                                   nCLASS_NUM,
                                   bPointShow);

    /* stride 16 */
    int nStride1 = 16;
    int nGridH1 = nHeight / nStride1;
    int nGridW1 = nWidth / nStride1;
    int nValidCount1 = 0;

    nValidCount1 = processKeyPoint((float *)vfInput[3],
                                   (float *)vfInput[4],
                                   (float *)vfInput[5],
                                   (float *)vfInput[10],
                                   nGridH1,
                                   nGridW1,
                                   nStride1,
                                   nFlLen,
                                   vfFilterBoxes,
                                   vfObjProbs,
                                   vfPoints,
                                   vnClassId,
                                   nConfThreshold,
                                   nCLASS_NUM,
                                   bPointShow);

    /* stride 32 */
    int nStride2 = 32;
    int nGridH2 = nHeight / nStride2;
    int nGridW2 = nWidth / nStride2;
    int nValidCount2 = 0;

    nValidCount2 = processKeyPoint((float *)vfInput[6],
                                   (float *)vfInput[7],
                                   (float *)vfInput[8],
                                   (float *)vfInput[11],
                                   nGridH2,
                                   nGridW2,
                                   nStride2,
                                   nFlLen,
                                   vfFilterBoxes,
                                   vfObjProbs,
                                   vfPoints,
                                   vnClassId,
                                   nConfThreshold,
                                   nCLASS_NUM,
                                   bPointShow);

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

    vPointDatas.clear();
    /* box valid detect target */
    int nSPointNum = bPointShow ? 3 : 2;
    for (int i = 0; i < nValidCount; ++i)
    {
        if (vnIndexArray[i] == -1)
        {
            continue;
        }
        int n = vnIndexArray[i];

        float fX1 = vfFilterBoxes[n * 4 + 0];
        float fY1 = vfFilterBoxes[n * 4 + 1];
        float fX2 = fX1 + vfFilterBoxes[n * 4 + 2];
        float fY2 = fY1 + vfFilterBoxes[n * 4 + 3];
        int nId = vnClassId[n];
        float fObjConf = vfObjProbs[i];
        // printf("(%f,%f),(%f,%f),%d\n",fX1,fY1,fX2,fY2,nId);

        /* 存储结果 */
        Inference_NS::PointData_S stPointData;
        stPointData.stBoxs.nX1 = clamp(fX1, 0, nWidth);
        stPointData.stBoxs.nY1 = clamp(fY1, 0, nHeight);
        stPointData.stBoxs.nX2 = clamp(fX2, 0, nWidth);
        stPointData.stBoxs.nY2 = clamp(fY2, 0, nHeight);
        stPointData.fConfidence = fObjConf;
        stPointData.nLabel = nId;

        for (int nP = 0; nP < nPointNum; nP++)
        {
            Inference_NS::Point_S stPoint;
            stPoint.nX = clamp(vfPoints[n * nPointNum * nSPointNum + nP * nSPointNum + 0], 0, nWidth);
            stPoint.nY = clamp(vfPoints[n * nPointNum * nSPointNum + nP * nSPointNum + 1], 0, nHeight);
            if(bPointShow)
            {
                stPoint.nShow = vfPoints[n * nPointNum * nSPointNum + nP * nSPointNum + 2];
            }
            stPointData.vPoints.push_back(stPoint);
        }
        vPointDatas.push_back(stPointData);
    }

    return true;
}

float PostProcess_NS::cYOLOV8PostProcess::sigmoid(float fX)
{
    return 1.0 / (1.0 + expf(-fX));
}

float PostProcess_NS::cYOLOV8PostProcess::unsigmoid(float fY)
{
    return -1.0 * logf((1.0 / fY) - 1.0);
}

int PostProcess_NS::cYOLOV8PostProcess::quick_sort_indice_inverse(
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

int PostProcess_NS::cYOLOV8PostProcess::nms(
    int nValidCount,
    std::vector<float> &nfOutputLocations,
    std::vector<int> vnClassIds,
    std::vector<int> &vnOrder,
    int nFilterId,
    float fThreshold)
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

float PostProcess_NS::cYOLOV8PostProcess::CalculateOverlap(
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

void PostProcess_NS::cYOLOV8PostProcess::compute_dfl(
    float *tensor,
    int dfl_len,
    float *box)
{
    for (int b = 0; b < 4; b++)
    {
        float exp_t[dfl_len];
        float exp_sum = 0;
        float acc_sum = 0;
        for (int i = 0; i < dfl_len; i++)
        {
            exp_t[i] = exp(tensor[i + b * dfl_len]);
            exp_sum += exp_t[i];
        }

        for (int i = 0; i < dfl_len; i++)
        {
            acc_sum += exp_t[i] / exp_sum * i;
        }
        box[b] = acc_sum;
    }
}

int PostProcess_NS::cYOLOV8PostProcess::processDetect(
    float *box_tensor,
    float *score_tensor,
    float *score_sum_tensor,
    int nGridH,
    int nGridW,
    int nStride,
    int nDfl_len,
    std::vector<float> &vfBoxes,
    std::vector<float> &vfObjProbs,
    std::vector<int> &vnClassId,
    float fThreshold,
    int nObjClassNum)
{
    int validCount = 0;
    int grid_len = nGridH * nGridW;
    for (int i = 0; i < nGridH; i++)
    {
        for (int j = 0; j < nGridW; j++)
        {
            int offset = i * nGridW + j;
            int max_class_id = -1;

            /* 通过 score sum 起到快速过滤的作用 */
            if (score_sum_tensor != nullptr)
            {
                if (score_sum_tensor[offset] < fThreshold)
                {
                    continue;
                }
            }

            float max_score = 0;
            for (int c = 0; c < nObjClassNum; c++)
            {
                if ((score_tensor[offset] > fThreshold) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score > fThreshold)
            {
                offset = i * nGridW + j;
                float box[4];
                if (nDfl_len > 1)
                {
                    float before_dfl[nDfl_len * 4];
                    for (int k = 0; k < nDfl_len * 4; k++)
                    {
                        before_dfl[k] = box_tensor[offset];
                        offset += grid_len;
                    }
                    compute_dfl(before_dfl, nDfl_len, box);
                }
                else
                {
                    for (int k = 0; k < 4; k++)
                    {
                        box[k] = box_tensor[offset];
                        offset += grid_len;
                    }
                }

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * nStride;
                y1 = (-box[1] + i + 0.5) * nStride;
                x2 = (box[2] + j + 0.5) * nStride;
                y2 = (box[3] + i + 0.5) * nStride;
                w = x2 - x1;
                h = y2 - y1;
                vfBoxes.push_back(x1);
                vfBoxes.push_back(y1);
                vfBoxes.push_back(w);
                vfBoxes.push_back(h);

                vfObjProbs.push_back(max_score);
                vnClassId.push_back(max_class_id);
                validCount++;
            }
        }
    }
    return validCount;
}

int PostProcess_NS::cYOLOV8PostProcess::processKeyPoint(
    float *box_tensor,
    float *score_tensor,
    float *score_sum_tensor,
    float *point_tensor,
    int nGridH,
    int nGridW,
    int nStride,
    int nDfl_len,
    std::vector<float> &vfBoxes,
    std::vector<float> &vfObjProbs,
    std::vector<float> &vfPoints,
    std::vector<int> &vnClassId,
    float fThreshold,
    int nObjClassNum,
    bool bPointShow
)
{
    int validCount = 0;
    int grid_len = nGridH * nGridW;
    int nSPointNum = bPointShow ? 3 : 2;
    for (int i = 0; i < nGridH; i++)
    {
        for (int j = 0; j < nGridW; j++)
        {
            int offset = i * nGridW + j;
            int max_class_id = -1;

            /* 通过 score sum 起到快速过滤的作用 */
            if (score_sum_tensor != nullptr)
            {
                if (score_sum_tensor[offset] < fThreshold)
                {
                    continue;
                }
            }

            float max_score = 0;
            for (int c = 0; c < nObjClassNum; c++)
            {
                if ((score_tensor[offset] > fThreshold) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score > fThreshold)
            {
                offset = i * nGridW + j;
                float box[4];
                float before_dfl[nDfl_len * 4];
                for (int k = 0; k < nDfl_len * 4; k++)
                {
                    before_dfl[k] = box_tensor[offset];
                    offset += grid_len;
                }
                compute_dfl(before_dfl, nDfl_len, box);

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * nStride;
                y1 = (-box[1] + i + 0.5) * nStride;
                x2 = (box[2] + j + 0.5) * nStride;
                y2 = (box[3] + i + 0.5) * nStride;
                w = x2 - x1;
                h = y2 - y1;
                vfBoxes.push_back(x1);
                vfBoxes.push_back(y1);
                vfBoxes.push_back(w);
                vfBoxes.push_back(h);

                for (int nP = 0; nP < nPointNum; nP++)
                {
                    x1 = (point_tensor[(nP * nSPointNum + 0) * grid_len + i * nGridW + j] * 2.0 + j) * nStride;
                    y1 = (point_tensor[(nP * nSPointNum + 1) * grid_len + i * nGridW + j] * 2.0 + i) * nStride;
                    vfPoints.push_back(x1);
                    vfPoints.push_back(y1);
                    if(bPointShow)
                    {
                        vfPoints.push_back(point_tensor[(nP * nSPointNum + 1) * grid_len + i * nGridW + j]);
                    }
                }

                vfObjProbs.push_back(max_score);
                vnClassId.push_back(max_class_id);
                validCount++;
            }
        }
    }
    return validCount;
}
