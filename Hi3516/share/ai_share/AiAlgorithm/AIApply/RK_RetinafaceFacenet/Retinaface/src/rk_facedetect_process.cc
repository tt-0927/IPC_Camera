/*
 *  File Name: rk_facedetect_process.cc
 *  Created on: 2023年7月20日
 *  Author: wcp
 *  description : 对网络输出进行后处理，得到人脸框和5个关键点
 *  Modify date: 2023年7月21日
 */

#include "rk_facedetect_process.h"

/* 辅助解码容器的生产 */
void vPriorBoxVector(std::vector<float>& vPriors, std::vector<int>& vNeedVector, int width, int height)
{
    /*i,j:[width/8 ],[height/8]   min_size:[16], [32]  {min_size/height  min_size/width  [8]*[img1/i]*height  [8]*[img0/i]*width}   -> [width/8 ]*[height/8]*2
  i,j:[width/16],height/16]   min_size:[64], [128] {min_size/height  min_size/width  [16]*[img1/i]*height  [16]*[img0/i]*width}  -> [width/16 ]*[height/16]*2
  i,j:[width/32],height/32]   min_size:[256],[512] {min_size/height  min_size/width  [32]*[img1/i]*height  [32]*[img0/i]*width}  -> [width/32 ]*[height/32]*2
   */
    // 使用基于范围的 for 循环获取原始值
    for (const auto index_copy : vNeedVector)
    {
        if (index_copy < std::ceil(height * 1.0 / 8) * int(std::ceil(width * 1.0 / 8)) * 2)
        {
            int index = index_copy;
            vPriors.push_back((((index / 2) % int(std::ceil(width * 1.0 / 8))) + 0.5) * 8.0 / width);
            vPriors.push_back(((index / 2) / int(std::ceil(width * 1.0 / 8)) + 0.5) * 8.0 / height);
            if (index % 2 == 0)
            {
                vPriors.push_back(16.0 / width);
                vPriors.push_back(16.0 / height);
            }
            else
            {
                vPriors.push_back(32.0 / width);
                vPriors.push_back(32.0 / height);
            }
        }
        else if (index_copy < std::ceil(height * 1.0 / 8) * int(std::ceil(width * 1.0 / 8)) * 2 + ceil(height * 1.0 / 16) * ceil(width * 1.0 / 16) * 2)
        {
            int index  = index_copy;
            index     -= std::ceil(height * 1.0 / 8) * int(std::ceil(width * 1.0 / 8)) * 2;
            vPriors.push_back(((index / 2) % int(std::ceil(width * 1.0 / 16)) + 0.5) * 16.0 / width);
            vPriors.push_back(((index / 2) / int(std::ceil(width * 1.0 / 16)) + 0.5) * 16.0 / height);
            if (index % 2 == 0)
            {
                vPriors.push_back(64.0 / width);
                vPriors.push_back(64.0 / height);
            }
            else
            {
                vPriors.push_back(128.0 / width);
                vPriors.push_back(128.0 / height);
            }
        }
        else
        {
            int index = index_copy;
            index     = index - (std::ceil(height * 1.0 / 8) * int(std::ceil(width * 1.0 / 8)) * 2 + ceil(height * 1.0 / 16) * ceil(width * 1.0 / 16) * 2);
            /*vNeedVector.erase(std::remove(vNeedVector.begin(), vNeedVector.end(), index), vNeedVector.end());*/
            vPriors.push_back(((index / 2) % int(std::ceil(width * 1.0 / 32)) + 0.5) * 32.0 / width);
            vPriors.push_back(((index / 2) / int(std::ceil(width * 1.0 / 32)) + 0.5) * 32.0 / height);
            if (index % 2 == 0)
            {
                vPriors.push_back(256.0 / width);
                vPriors.push_back(256.0 / height);
            }
            else
            {
                vPriors.push_back(512.0 / width);
                vPriors.push_back(512.0 / height);
            }
        }
    }
}

/* 预测框的解码 */
void DecodeBoxs(std::vector<int> vNeedVector, std::vector<float> vPriors, float* pInput0, int nWidth, int nHeight)
{
    // 使用传统的 for 循环获取原始值
    for (size_t i = 0; i < vNeedVector.size(); ++i)
    {
        pInput0[vNeedVector[i] * 4 + 0] = vPriors[i * 4 + 0] + pInput0[vNeedVector[i] * 4 + 0] * 0.1 * vPriors[i * 4 + 2];
        pInput0[vNeedVector[i] * 4 + 1] = vPriors[i * 4 + 1] + pInput0[vNeedVector[i] * 4 + 1] * 0.1 * vPriors[i * 4 + 3];
        pInput0[vNeedVector[i] * 4 + 2] = vPriors[i * 4 + 2] * std::exp(pInput0[vNeedVector[i] * 4 + 2] * 0.2);
        pInput0[vNeedVector[i] * 4 + 3] = vPriors[i * 4 + 3] * std::exp(pInput0[vNeedVector[i] * 4 + 3] * 0.2);

        pInput0[vNeedVector[i] * 4 + 0] -= pInput0[vNeedVector[i] * 4 + 2] * 1.0 / 2;
        pInput0[vNeedVector[i] * 4 + 1] -= pInput0[vNeedVector[i] * 4 + 3] * 1.0 / 2;
        pInput0[vNeedVector[i] * 4 + 2] += pInput0[vNeedVector[i] * 4 + 0];
        pInput0[vNeedVector[i] * 4 + 3] += pInput0[vNeedVector[i] * 4 + 1];

        pInput0[vNeedVector[i] * 4 + 0] *= nWidth;
        pInput0[vNeedVector[i] * 4 + 1] *= nHeight;
        pInput0[vNeedVector[i] * 4 + 2] *= nWidth;
        pInput0[vNeedVector[i] * 4 + 3] *= nHeight;
        // std::cout<<"解码后的坐标："<<pInput0[vNeedVector[i]*4+0]<<";"<<pInput0[vNeedVector[i]*4+1]<<";"<<pInput0[vNeedVector[i]*4+2]<<";"<<pInput0[vNeedVector[i]*4+3]<<std::endl;
    }
}

/* 五个关键点的解码 */
void DecodeLandm(std::vector<int> vNeedVector, std::vector<float> vPriors, float* pInput2, int nWidth, int nHeight)
{
    // 使用传统的 for 循环获取原始值
    for (size_t i = 0; i < vNeedVector.size(); ++i)
    {
        pInput2[vNeedVector[i] * 10 + 0] = (vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 0] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 1] = (vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 1] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 2] = (vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 2] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 3] = (vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 3] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 4] = (vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 4] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 5] = (vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 5] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 6] = (vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 6] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 7] = (vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 7] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 8] = (vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 8] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 9] = (vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 9] * 0.1 * vPriors[i * 4 + 3]) * nHeight;

        /*std::cout<<"解码后的坐标："<<pInput2[vNeedVector[i]*10+0]<<";"<<pInput2[vNeedVector[i]*10+1]<<";"<<pInput2[vNeedVector[i]*10+2]<<";"<<pInput2[vNeedVector[i]*10+3]<<";"<<pInput2[vNeedVector[i]*10+4]<<";"<<pInput2[vNeedVector[i]*10+5]<<
        ";"<<pInput2[vNeedVector[i]*10+5]<<";"<<pInput2[vNeedVector[i]*10+6]<<";"<<pInput2[vNeedVector[i]*10+7]<<";"<<pInput2[vNeedVector[i]*10+8]<<";"<<pInput2[vNeedVector[i]*10+9]<<std::endl;*/
    }
}

/* IOU的计算 */
static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1)
{
    float w = std::fmax(0.f, std::fmin(xmax0, xmax1) - std::fmax(xmin0, xmin1) + 1.0);
    float h = std::fmax(0.f, std::fmin(ymax0, ymax1) - std::fmax(ymin0, ymin1) + 1.0);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

/* 快排算法 */
static int quick_sort_indice_inverse(std::vector<float>& input, int left, int right, std::vector<int>& indices)
{
    float key;
    int   key_index;
    int   low  = left;
    int   high = right;
    if (left < right)
    {
        key_index = indices[left];
        key       = input[left];
        while (low < high)
        {
            while (low < high && input[high] <= key)
            {
                high--;
            }
            input[low]   = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key)
            {
                low++;
            }
            input[high]   = input[low];
            indices[high] = indices[low];
        }
        input[low]   = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
}

/* 非极大值抑制 */
static int nms(int validCount, float* outputLocations, std::vector<int>& order, float threshold, std::vector<int> vNeedVector)
{
    for (int i = 0; i < validCount; ++i)
    {
        if (order[i] == -1)
        {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j)
        {
            int m = order[j];
            if (m == -1)
            {
                continue;
            }
            float xmin0 = outputLocations[vNeedVector[n] * 4 + 0];
            float ymin0 = outputLocations[vNeedVector[n] * 4 + 1];
            float xmax0 = outputLocations[vNeedVector[n] * 4 + 2];
            float ymax0 = outputLocations[vNeedVector[n] * 4 + 3];

            float xmin1 = outputLocations[vNeedVector[m] * 4 + 0];
            float ymin1 = outputLocations[vNeedVector[m] * 4 + 1];
            float xmax1 = outputLocations[vNeedVector[m] * 4 + 2];
            float ymax1 = outputLocations[vNeedVector[m] * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold)
            {
                order[j] = -1;
            }
        }
    }
    return 0;
}

/* 图像预处理方法 */
int ctrlNetoutputProcess(float* pInput0, float* pInput1, float* pInput2, int nHeight, int nWidth, std::vector<int>& vBox, DetectParam_S stDetectParam, int nModelNeed)
{
    /* nTotal代码的是网络一共会得到多少个预测框 */
    // int nTotal = 80640;
    // int nTotal = 16800;
    std::vector<float> vPriors;
    /* 记录预测置信度符合预期置信度的下标 */
    std::vector<int>   vNeedVector;
    /* 记录符合条件的置信度值，与vNeedVector一一对应 */
    std::vector<float> objProbs;
    for (int h = 0; h < nModelNeed; h++)
    {
        if (pInput1[h * 2 + 1] >= stDetectParam.fConfidentThreshold)
        {
            vNeedVector.push_back(h);
            objProbs.push_back(pInput1[h * 2 + 1]);
        }
    }

    /* 辅助的容器vPriors，用于获取解码的数据 */
    vPriorBoxVector(vPriors, vNeedVector, nWidth, nHeight);
    /* 预测框的解码 */
    DecodeBoxs(vNeedVector, vPriors, pInput0, nWidth, nHeight);
    /* 五个人脸关键点的解码 */
    DecodeLandm(vNeedVector, vPriors, pInput2, nWidth, nHeight);

    /* 生成一个辅助的下标容器，来记录置信度排序的位置 */
    std::vector<int> indexArray;
    for (int i = 0; i < vNeedVector.size(); ++i)
    {
        indexArray.push_back(i);
    }
    /* 将置信度的容器进行快排 */
    quick_sort_indice_inverse(objProbs, 0, vNeedVector.size() - 1, indexArray);
    /* 非极大值抑制 */
    nms(objProbs.size(), pInput0, indexArray, stDetectParam.fNmsThreshold, vNeedVector);

    // 使用基于范围的 for 循环获取原始值
    for (const auto& value : indexArray)
    {
        if (value != -1)
        {
            if (pInput0[vNeedVector[value] * 4 + 0] >= nWidth || pInput0[vNeedVector[value] * 4 + 2] >= nWidth || pInput0[vNeedVector[value] * 4 + 1] > nHeight || pInput0[vNeedVector[value] * 4 + 3] > nHeight)
            {
                continue;
            }
            for (int i = 0; i < 4; i++)
            {
                vBox.push_back(int(pInput0[vNeedVector[value] * 4 + i]));
            }
            for (int i = 0; i < 10; i++)
            {
                vBox.push_back(int(pInput2[vNeedVector[value] * 10 + i]));
            }
        }
    }

    return 1;
}
