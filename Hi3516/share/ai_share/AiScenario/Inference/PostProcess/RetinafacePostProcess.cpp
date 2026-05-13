/*
 * @FilePath     : RetinafacePostProcess.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-31 14:09:50
 * @Description  :
 */

#include "RetinafacePostProcess.hpp"

#include "dlog.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

/* 辅助解码容器的生产 */
void PostProcessV1_0_NS::cRetinafacePostProcess::vPriorBoxVector(std::vector<float> &vPriors,std::vector<int> &vNeedVector,int nWidth, int nHeight)
{
    // 使用基于范围的 for 循环获取原始值
    for (const auto index_copy : vNeedVector) 
    {
        if(index_copy< (std::ceil(nHeight * 1.0 / 8) * int(std::ceil(nWidth * 1.0/8)) * 2))
        {
            int nIndex = index_copy;
            vPriors.push_back((((nIndex / 2) % int(std::ceil(nWidth * 1.0 / 8))) + 0.5) * 8.0 / nWidth);
            vPriors.push_back(((nIndex / 2)/int(std::ceil(nWidth * 1.0 / 8)) + 0.5) * 8.0 / nHeight);
            if(nIndex % 2 == 0)
            {
                vPriors.push_back(16.0 / nWidth);
                vPriors.push_back(16.0 / nHeight);
            }
            else
            {
                vPriors.push_back(32.0 / nWidth);
                vPriors.push_back(32.0 / nHeight);
            }
        }
        else if(index_copy< std::ceil(nHeight * 1.0 / 8) * int(std::ceil(nWidth * 1.0/8)) * 2+ceil(nHeight * 1.0/16) * ceil(nWidth * 1.0 / 16) * 2)
        {
            int nIndex = index_copy;
            nIndex -= std::ceil(nHeight * 1.0 / 8) * int(std::ceil(nWidth * 1.0 / 8)) * 2;
            vPriors.push_back(((nIndex / 2) % int(std::ceil(nWidth * 1.0 / 16)) + 0.5) * 16.0 / nWidth);
            vPriors.push_back(((nIndex / 2) / int(std::ceil(nWidth * 1.0 / 16)) + 0.5) * 16.0 / nHeight);
            if(nIndex % 2 == 0)
            {
                vPriors.push_back(64.0 / nWidth);
                vPriors.push_back(64.0 / nHeight);
            }
            else
            {
                vPriors.push_back(128.0 / nWidth);
                vPriors.push_back(128.0 / nHeight);
	    }
	}
        else
        {
            int nIndex = index_copy;
            nIndex = nIndex- (std::ceil(nHeight * 1.0 / 8) * int(std::ceil(nWidth * 1.0 / 8)) * 2 + ceil(nHeight * 1.0 / 16) * ceil(nWidth * 1.0 / 16) * 2);
            vPriors.push_back(((nIndex / 2) % int(std::ceil(nWidth * 1.0 / 32)) + 0.5) * 32.0 / nWidth);
            vPriors.push_back(((nIndex / 2) / int(std::ceil(nWidth * 1.0 / 32)) + 0.5) * 32.0 / nHeight);
            if(nIndex % 2 == 0)
            {
            	vPriors.push_back(256.0 / nWidth);
                vPriors.push_back(256.0 / nHeight);
            }  
            else
            {
                vPriors.push_back(512.0 / nWidth);
                vPriors.push_back(512.0 / nHeight);            
            }
        }

      }
}

/* 预测框的解码 */
void PostProcessV1_0_NS::cRetinafacePostProcess::decodeBoxs(std::vector<int> vNeedVector,std::vector<float> vPriors,float *pInput0,int nWidth,int nHeight)
{
    // 使用传统的 for 循环获取原始值
    for (size_t i = 0; i < vNeedVector.size(); ++i) 
    {
        pInput0[vNeedVector[i] * 4 + 0] =vPriors[i * 4 + 0] + pInput0[vNeedVector[i] * 4 + 0] * 0.1 * vPriors[i * 4 + 2];
        pInput0[vNeedVector[i] * 4 + 1] =vPriors[i * 4 + 1] + pInput0[vNeedVector[i] * 4 + 1] * 0.1 * vPriors[i * 4 + 3];
        pInput0[vNeedVector[i] * 4 + 2] =vPriors[i * 4 + 2] * std::exp(pInput0[vNeedVector[i] * 4 + 2] * 0.2);
        pInput0[vNeedVector[i] * 4 + 3] =vPriors[i * 4 + 3] * std::exp(pInput0[vNeedVector[i] * 4 + 3] * 0.2);
        
        pInput0[vNeedVector[i] * 4 + 0] -= pInput0[vNeedVector[i] * 4 + 2] * 1.0 / 2;
        pInput0[vNeedVector[i] * 4 + 1] -= pInput0[vNeedVector[i] * 4 + 3] * 1.0 / 2;
        pInput0[vNeedVector[i] * 4 + 2] += pInput0[vNeedVector[i] * 4 + 0];
        pInput0[vNeedVector[i] * 4 + 3] += pInput0[vNeedVector[i] * 4 + 1];
        
        pInput0[vNeedVector[i] * 4 + 0] *= nWidth;
        pInput0[vNeedVector[i] * 4 + 1] *= nHeight;
        pInput0[vNeedVector[i] * 4 + 2] *= nWidth;
        pInput0[vNeedVector[i] * 4 + 3] *= nHeight;
        //std::cout<<"解码后的坐标："<<pInput0[vNeedVector[i]*4+0]<<";"<<pInput0[vNeedVector[i]*4+1]<<";"<<pInput0[vNeedVector[i]*4+2]<<";"<<pInput0[vNeedVector[i]*4+3]<<std::endl;

    }
}

/* 五个人脸关键点的解码 */
void PostProcessV1_0_NS::cRetinafacePostProcess::decodeLandm(std::vector<int> vNeedVector,std::vector<float> vPriors,float *pInput2,int nWidth, int nHeight)
{
    // 使用传统的 for 循环获取原始值
    for (size_t i = 0; i < vNeedVector.size(); ++i) 
    {
        pInput2[vNeedVector[i] * 10 + 0] =(vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 0] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 1] =(vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 1] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 2] =(vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 2] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 3] =(vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 3] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 4] =(vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 4] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 5] =(vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 5] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 6] =(vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 6] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 7] =(vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 7] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
        pInput2[vNeedVector[i] * 10 + 8] =(vPriors[i * 4 + 0] + pInput2[vNeedVector[i] * 10 + 8] * 0.1 * vPriors[i * 4 + 2]) * nWidth;
        pInput2[vNeedVector[i] * 10 + 9] =(vPriors[i * 4 + 1] + pInput2[vNeedVector[i] * 10 + 9] * 0.1 * vPriors[i * 4 + 3]) * nHeight;
    
	    /*std::cout<<"解码后的坐标："<<pInput2[vNeedVector[i]*10+0]<<";"<<pInput2[vNeedVector[i]*10+1]<<";"<<pInput2[vNeedVector[i]*10+2]<<";"<<pInput2[vNeedVector[i]*10+3]<<";"<<pInput2[vNeedVector[i]*10+4]<<";"<<pInput2[vNeedVector[i]*10+5]<<
	    ";"<<pInput2[vNeedVector[i]*10+5]<<";"<<pInput2[vNeedVector[i]*10+6]<<";"<<pInput2[vNeedVector[i]*10+7]<<";"<<pInput2[vNeedVector[i]*10+8]<<";"<<pInput2[vNeedVector[i]*10+9]<<std::endl;*/
	}
}

/* IOU的计算 */
static float CalculateOverlap(float fXMin0, float fYMin0, float fXMax0, float fYMax0, float fXMin1, float fYMin1, float fXMax1,
                              float fYMax1)
{
  float fW = std::fmax(0.f, std::fmin(fXMax0, fXMax1) - std::fmax(fXMin0, fXMin1) + 1.0);
  float fH = std::fmax(0.f, std::fmin(fYMax0, fYMax1) - std::fmax(fYMin0, fYMin1) + 1.0);
  float fI = fW * fH;
  float fU = (fXMax0 - fXMin0 + 1.0) * (fYMax0 - fYMin0 + 1.0) + (fXMax1 - fXMin1 + 1.0) * (fYMax1 - fYMin1 + 1.0) - fI;
  return fU <= 0.f ? 0.f : (fI / fU);
}

/* 快排算法 */
static int quick_sort_indice_inverse(std::vector<float>& vInput, int nLeft, int nRight, std::vector<int>& vIndices)
{
  float fKey;
  int   nKeyIndex;
  int   nLow  = nLeft;
  int   nHigh = nRight;
  if (nLeft < nRight) {
    nKeyIndex = vIndices[nLeft];
    fKey       = vInput[nLeft];
    while (nLow < nHigh) {
      while (nLow < nHigh && vInput[nHigh] <= fKey) {
        nHigh--;
      }
      vInput[nLow]   = vInput[nHigh];
      vIndices[nLow] = vIndices[nHigh];
      while (nLow < nHigh && vInput[nLow] >= fKey) 
      {
        nLow++;
      }
      vInput[nHigh]   = vInput[nLow];
      vIndices[nHigh] = vIndices[nLow];
    }
    vInput[nLow]   = fKey;
    vIndices[nLow] = nKeyIndex;
    quick_sort_indice_inverse(vInput, nLeft, nLow - 1, vIndices);
    quick_sort_indice_inverse(vInput, nLow + 1, nRight, vIndices);
  }
  return nLow;
}

/* 非极大值抑制 */
static int nms(int validCount, float *pOutputLocations, std::vector<int>& vOrder, float fThreshold, std::vector<int> vNeedVector)
{
  for (int i = 0; i < validCount; ++i) {
    if (vOrder[i] == -1) 
    {
      continue;
    }
    int n = vOrder[i];
    for (int j = i + 1; j < validCount; ++j) 
    {
      int m = vOrder[j];
      if (m == -1) {
        continue;
      }
      float xmin0 = pOutputLocations[vNeedVector[n] * 4 + 0];
      float ymin0 = pOutputLocations[vNeedVector[n] * 4 + 1];
      float xmax0 = pOutputLocations[vNeedVector[n] * 4 + 2];
      float ymax0 = pOutputLocations[vNeedVector[n] * 4 + 3];

      float xmin1 = pOutputLocations[vNeedVector[m] * 4 + 0];
      float ymin1 = pOutputLocations[vNeedVector[m] * 4 + 1];
      float xmax1 = pOutputLocations[vNeedVector[m] * 4 + 2];
      float ymax1 = pOutputLocations[vNeedVector[m] * 4 + 3];

      float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

      if (iou > fThreshold) 
      {
        vOrder[j] = -1;
      }
    }
  }
  return 0;
}

/* 数据后处理 */
bool PostProcessV1_0_NS::cRetinafacePostProcess::postProcess(
    float*              pInput0,
    float*              pInput1,
    float*              pInput2,
    int                 nModelNeed,
    int                 nHeight,
    int                 nWidth,
    float               nConfThreshold,
    float               nNmsThreshold,
    std::vector<float>& vPointsXY)
{
    if (nullptr == pInput0 ||
        nullptr == pInput1 ||
        nullptr == pInput2 ||
        nModelNeed <= 0    ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    /* nTotal代码的是网络一共会得到多少个预测框 */
    std::vector<float> vPriors;
    /* 记录预测置信度符合预期置信度的下标 */
    std::vector<int> vNeedVector; 
    /* 记录符合条件的置信度值，与vNeedVector一一对应 */
    std::vector<float> vObjProbs;
    for (int nH = 0; nH < nModelNeed; nH++) 
      {
        if(pInput1[nH * 2 + 1] >= nConfThreshold)
        {
          vNeedVector.push_back(nH);
          vObjProbs.push_back(pInput1[nH * 2 + 1]);
        }
    }
    
    /* 辅助的容器vPriors，用于获取解码的数据 */
    vPriorBoxVector(vPriors, vNeedVector, nWidth, nHeight);
    /* 预测框的解码 */
    decodeBoxs(vNeedVector, vPriors, pInput0, nWidth, nHeight);
    /* 五个人脸关键点的解码 */
    decodeLandm(vNeedVector, vPriors, pInput2, nWidth, nHeight);
    
    /* 生成一个辅助的下标容器，来记录置信度排序的位置 */
    std::vector<int> vIndexArray;
    for (int i = 0; i < vNeedVector.size(); ++i) 
      {
      vIndexArray.push_back(i);
    }
    /* 将置信度的容器进行快排 */
    quick_sort_indice_inverse(vObjProbs, 0, vNeedVector.size() - 1, vIndexArray);
    /* 非极大值抑制 */
    nms(vObjProbs.size(), pInput0, vIndexArray, nNmsThreshold, vNeedVector);
    
    // 使用基于范围的 for 循环获取原始值
    for (const auto& value : vIndexArray) 
    {
        if(value!=-1)
        {
            if(pInput0[vNeedVector[value] * 4 + 0] >= nWidth 
                  || pInput0[vNeedVector[value] * 4 + 2] >= nWidth 
                  || pInput0[vNeedVector[value] * 4 + 1] > nHeight 
                  || pInput0[vNeedVector[value] * 4 + 3] > nHeight)
            {
                continue;
            } 
            for(int i = 0; i < 4; i++)
            {
                vPointsXY.push_back(float(pInput0[vNeedVector[value] * 4 + i]));
            }

            for(int i = 0; i < 10; i++)
            {
                vPointsXY.push_back(float(pInput2[vNeedVector[value] * 10 + i]));
            }
        }
    }

    return true;
}
