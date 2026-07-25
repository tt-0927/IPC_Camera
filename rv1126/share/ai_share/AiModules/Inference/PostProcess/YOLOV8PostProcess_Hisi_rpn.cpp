/*
 * @FilePath     : YOLOV5PostProcess.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 20:07:54
 * @Description  :
 */

#include "YOLOV8PostProcess_Hisi_rpn.hpp"
#include <cmath>
#include <set>

bool PostProcess_NS::cYOLOV8PostProcessHisi::postProcessDetect(
    int nOutNum,
    float* scoreData,
    uint16_t* maxScoreIdx,
    float* coord,
    uint16_t* maxClass,
    int nHeight,
    int nWidth,
    uint32_t coordNum,
    float nConfThreshold,
    float nNmsThreshold,
    std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    // === 构造原始边框结果（包括置信度与坐标） ===
    std::vector<std::vector<float>> bboxes;
    for (uint32_t idx = 0; idx < nOutNum; idx++) {
        uint32_t coordIdx = maxScoreIdx[idx];  // 对应的坐标索引
        // 每个检测框包含：score, xmin, ymin, xmax, ymax, classId
        std::vector<float> bbox{
            scoreData[idx],                            // 置信度
            coord[coordIdx],                           // xmin
            coord[coordIdx + 1 * coordNum],            // ymin
            coord[coordIdx + 2 * coordNum],            // xmax
            coord[coordIdx + 3 * coordNum],            // ymax
            static_cast<float>(maxClass[coordIdx])     // 类别 ID
        };
        bboxes.push_back(bbox);
    }

    std::vector<std::vector<float>> finalBoxes;
    MulticlassNms(finalBoxes, bboxes, nNmsThreshold);  // 多类别 NMS 处理

    vBoxDatas.clear();
    /* box valid detect target */
    for (int i = 0; i < finalBoxes.size(); ++i)
    {
        float fX1 = finalBoxes[i][0];
        float fY1 = finalBoxes[i][1];
        float fX2 = finalBoxes[i][2];
        float fY2 = finalBoxes[i][3];
        float fObjConf = finalBoxes[i][4];
        int nId = static_cast<int>(finalBoxes[i][5]);
        
        // printf("(%f,%f),(%f,%f),%d\n",fX1,fY1,fX2,fY2,nId);
        if (fObjConf > nConfThreshold)
        {
            Inference_NS::BoxData_S stBoxData;
            stBoxData.stBoxs.nX1 = clamp(fX1, 0, nWidth);
            stBoxData.stBoxs.nY1 = clamp(fY1, 0, nHeight);
            stBoxData.stBoxs.nX2 = clamp(fX2, 0, nWidth);
            stBoxData.stBoxs.nY2 = clamp(fY2, 0, nHeight);
            stBoxData.fConfidence = fObjConf;
            stBoxData.nLabel = nId;
            vBoxDatas.push_back(stBoxData);
        }
    }
    return true;
}

bool PostProcess_NS::cYOLOV8PostProcessHisi::postProcessKeyPoint(
    int nOutNum,
    float* scoreData,
    uint16_t* maxScoreIdx,
    float* coord,
    uint16_t* maxClass,
    float* kptData,
    int nHeight,
    int nWidth,
    int nKptNumPerBox,
    uint32_t coordNum,
    float nConfThreshold,
    float nNmsThreshold,
    std::vector<Inference_NS::PointData_S>& vPointDatas)
{
    // === 内存优化：直接处理原始数据 ===
    // 预计算边界框面积 (避免后续重复计算)
    std::vector<float> areas;
    areas.reserve(nOutNum);
    std::vector<int> sortedIndices(nOutNum);
    for (int i = 0; i < nOutNum; i++) {
        const uint16_t coordIdx = maxScoreIdx[i];
        const float xmin = coord[coordIdx];
        const float ymin = coord[coordIdx + coordNum];
        const float xmax = coord[coordIdx + 2 * coordNum];
        const float ymax = coord[coordIdx + 3 * coordNum];
        areas.push_back((xmax - xmin) * (ymax - ymin));
        sortedIndices[i] = i;
        // printf("第 %d 个目标框的输出 coordIdx 为 [%d] ，置信度：[%f]\n", i, maxScoreIdx[i], scoreData[i]);
    }

    quick_sort_indices_desc(scoreData, 0, nOutNum -1, sortedIndices);

    // === 优化NMS (原地操作，避免额外数据结构) ===
    std::vector<uint8_t> keepFlags(nOutNum, 1);
    
    // 优化NMS：仅比较更高置信度的框
    for (int i = 0; i < nOutNum; i++) {
        if (!keepFlags[sortedIndices[i]]) continue;
        
        const int idx_i = sortedIndices[i];
        const uint16_t coordIdx_i = maxScoreIdx[idx_i];
        const float* box_i = &coord[coordIdx_i];

        for (int j = i + 1; j < nOutNum; j++) {
            if (!keepFlags[sortedIndices[j]]) continue;
            
            const int idx_j = sortedIndices[j];
            const uint16_t coordIdx_j = maxScoreIdx[idx_j];
            const float* box_j = &coord[coordIdx_j];

            // 快速计算IoU
            const float xx1 = std::max(box_i[0], box_j[0]);
            const float yy1 = std::max(box_i[coordNum], box_j[coordNum]);
            const float xx2 = std::min(box_i[2 * coordNum], box_j[2 * coordNum]);
            const float yy2 = std::min(box_i[3 * coordNum], box_j[3 * coordNum]);
            
            const float w = std::max(0.0f, xx2 - xx1);
            const float h = std::max(0.0f, yy2 - yy1);
            const float inter = w * h;
            
            if (inter / (areas[idx_i] + areas[idx_j] - inter) > nNmsThreshold) {
                keepFlags[idx_j] = 0;
            }
        }
    }

    int validCount = 0;
    vPointDatas.clear();

    for (int i = 0; i < nOutNum; i++) {
        if (!keepFlags[i]) continue;
        validCount++;


        const uint16_t coordIdx = maxScoreIdx[i];
        const float score = scoreData[i];
        // printf("第 %d 个目标框的置信度: [%f]\n", i, score);
        // printf("第 %d 个目标框的面积大小: [%f]\n", i, areas[i]);

        if (score > nConfThreshold)
        {
            Inference_NS::PointData_S stPointData;
            // 直接使用坐标指针，避免复制
            const float xmin = coord[coordIdx];
            const float ymin = coord[coordIdx + coordNum];
            const float xmax = coord[coordIdx + 2 * coordNum];
            const float ymax = coord[coordIdx + 3 * coordNum];
            stPointData.stBoxs.nX1 = xmin;
            stPointData.stBoxs.nY1 = ymin;
            stPointData.stBoxs.nX2 = xmax;
            stPointData.stBoxs.nY2 = ymax;
            stPointData.fConfidence = score;
            stPointData.nLabel = 0;
            // 直接处理关键点数据 (避免创建临时容器)
            for (int k = 0; k < nKptNumPerBox; k++) {
                Inference_NS::Point_S stPoint;
                const int baseIdx = k * 3;
                // const int baseIdx = coordIdx + k * 3;
                const float kx = kptData[coordIdx + baseIdx * coordNum];
                const float ky = kptData[coordIdx + (baseIdx + 1) * coordNum];
                const float kv = kptData[coordIdx + (baseIdx + 2) * coordNum];
                
                // printf("第 %d 个目标框的第 %d 个关键点的输出：[%f]x[%f]---[%f]\n", i, k, kx, ky, kv);

                stPoint.nX = clamp(kptData[coordIdx + baseIdx * coordNum], 0, nWidth);
                stPoint.nY = clamp(kptData[coordIdx + (baseIdx + 1) * coordNum], 0, nHeight);
                stPointData.vPoints.push_back(stPoint);
            }

            // for (int i = 0; i < 126000; ++i) {
            //     printf("%.6f ", kptData[i]);  // 保留 6 位小数
            //     if ((i + 1) % 8400 == 0)
            //         printf("\n");
            // }
            // // 若最后一行不足 15 个，补一个换行
            // if (126000 % 8400 != 0)
            //     printf("\n");
            
            vPointDatas.push_back(stPointData);
        }
        
    }


    // MulticlassNmsPoint(finalBoxes, bboxes, nNmsThreshold);  // 多类别 NMS 处理

    return true;
}


void PostProcess_NS::cYOLOV8PostProcessHisi::MulticlassNms(
    vector<vector<float>>& bboxes, 
    const vector<vector<float>>& vaildBox,
    float nmsThr)
{
    const uint8_t scoreIdx = 0;
    const uint8_t xminIdx = 1;
    const uint8_t yminIdx  = 2; // 2 index
    const uint8_t xmaxIdx = 3; // 3: index
    const uint8_t ymaxIdx = 4; // 4: index
    const uint8_t classIdIdx = 5; // 5: index
    for (auto &item : vaildBox) { /* score, xcenter, ycenter, w, h, classId */
        float x1 = item[xminIdx];
        float y1 = item[yminIdx];
        float x2 = item[xmaxIdx];
        float y2 = item[ymaxIdx];
        float area = (x2 - x1 + 1) * (y2 - y1 + 1);
        bool keep = true;
        /* lx, ly, rx, ry, score, class id, area */
        vector<float> bbox {x1, y1, x2, y2, item[scoreIdx], item[classIdIdx], area};
        for (size_t j = 0; j < bboxes.size(); j++) {
            if (CalcIou(bbox, bboxes[j]) > nmsThr) {
                keep = false;
                break;
            }
        }
        if (keep) {
            bboxes.push_back(bbox);
        }
    }
}

float PostProcess_NS::cYOLOV8PostProcessHisi::CalcIou(
    const vector<float> &box1, 
    const vector<float> &box2)
{
    float area1 = box1[6];
    float area2 = box2[6];
    float xx1 = max(box1[0], box2[0]);
    float yy1 = max(box1[1], box2[1]);
    float xx2 = min(box1[2], box2[2]);
    float yy2 = min(box1[3], box2[3]);
    float w = max(0.0f, xx2 - xx1 + 1);
    float h = max(0.0f, yy2 - yy1 + 1);
    float inter = w * h;
    float ovr = inter / (area1 + area2 - inter);
    return ovr;
}


void PostProcess_NS::cYOLOV8PostProcessHisi::quick_sort_indices_desc(
    const float* inputData,
    int nLeft,
    int nRight,
    std::vector<int>& indices)
{
    if (nLeft < nRight) {
        int i = nLeft;
        int j = nRight;
        int pivotIndex = indices[nLeft];
        float pivotValue = inputData[pivotIndex];

        while (i < j) {
            while (i < j && inputData[indices[j]] <= pivotValue) j--;
            indices[i] = indices[j];

            while (i < j && inputData[indices[i]] >= pivotValue) i++;
            indices[j] = indices[i];
        }

        indices[i] = pivotIndex;

        quick_sort_indices_desc(inputData, nLeft, i - 1, indices);
        quick_sort_indices_desc(inputData, i + 1, nRight, indices);
    }
}



