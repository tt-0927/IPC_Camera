/**
 * @file AttributePostProcess.cpp
 * @author caishengjie (caisj@kfb.cn)
 * @date 2024-10-09
 *
 * @brief
 */

#include "AttributePostProcess.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <cmath>

/*
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
*/

/* 数据后处理 */
PostProcess_NS::cAttributePostProcess::cAttributePostProcess()
{
}
PostProcess_NS::cAttributePostProcess::~cAttributePostProcess()
{
}

void PostProcess_NS::cAttributePostProcess::setParam(int nClassNum, std::vector<std::vector<int>> vGroupOnce)
{
    m_nClassNum = nClassNum;
    m_vGroupOnce = vGroupOnce;
}

bool PostProcess_NS::cAttributePostProcess::postProcess(
    float *pfInput,
    float nConfThreshold,
    std::vector<float> &vOutput)
{
    if (nullptr == pfInput ||
        nConfThreshold <= 0)
    {
        printf("传入参数为空\n");
        return false;
    }
    if (m_nClassNum == 0)
    {
        printf("属性数量[%d]未设置, 请先设置属性数量\n", m_nClassNum);
        return false;
    }
    for (int nIndex = 0; nIndex < m_nClassNum; nIndex++)
    {
        pfInput[nIndex] = sigmoid(pfInput[nIndex]);
    }

    std::copy(pfInput, pfInput + m_nClassNum, std::back_inserter(vOutput));

    if (m_vGroupOnce.size() == 0)
    {
        // printf("属性分组[%ld]未设置，输出所有属性置信度\n", m_vGroupOnce.size());
        return true;
    }

    bool bRes = getMaxIndex(pfInput, vOutput);
    if (bRes == false)
    {
        printf("获取单属性输出类别失败");
        return false;
    }
    bRes = getThresholdIndex(nConfThreshold, vOutput);
    if (bRes == false)
    {
        printf("获取多属性输出类别失败");
        return false;
    }
    return true;
}

float PostProcess_NS::cAttributePostProcess::sigmoid(float fX)
{
    return 1.0 / (1.0 + expf(-fX));
}

float PostProcess_NS::cAttributePostProcess::unsigmoid(float fY)
{
    return -1.0 * logf((1.0 / fY) - 1.0);
}

bool PostProcess_NS::cAttributePostProcess::getMaxIndex(
    float *vArray,
    std::vector<float> &vResult)
{
    if (vResult.size() != m_nClassNum)
    {
        printf("类别数与模型输出尺寸不符");
        return false;
    }
    for (int i = 0; i < m_vGroupOnce.size(); i++)
    {
        std::vector<int> vIndexArray = m_vGroupOnce[i];
        if (vIndexArray.size() > 0)
        {
            int nMaxIndex = vIndexArray[0];
            for (int j = 1; j < vIndexArray.size(); j++)
            {
                if (vArray[vIndexArray[j]] > vArray[nMaxIndex])
                {
                    vResult[nMaxIndex] = -1;
                    nMaxIndex = vIndexArray[j];
                }
                else
                {
                    vResult[vIndexArray[j]] = -1;
                }
            }
            vResult[nMaxIndex] += 1;
        }
    }
    return true;
}

bool PostProcess_NS::cAttributePostProcess::getThresholdIndex(
    float fThreshold,
    std::vector<float> &vResult)
{
    if (vResult.size() != m_nClassNum)
    {
        printf("类别数与模型输出尺寸不符");
        return false;
    }
    for (int i = 0; i < m_nClassNum; i++)
    {
        if (vResult[i] < fThreshold)
        {
            vResult[i] = -1;
        }
        else if (vResult[i] > 1)
        {
            vResult[i] -= 1;
        }
    }
    return true;
}
