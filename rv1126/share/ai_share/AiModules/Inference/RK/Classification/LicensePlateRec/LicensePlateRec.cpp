/*
 * @FilePath     : LicensePlateRec.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-29 16:29:52
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-09-29 16:29:52
 * @Description  : 车牌识别算法
 */
#include "LicensePlateRec.hpp"
#include <cmath>
#include <memory>
#include <cstring>
#include <algorithm>

Inference_NS::CLicensePlateRec::CLicensePlateRec(std::string strConfigPath)
    : CCVInferenceRK(strConfigPath)
{
}

Inference_NS::CLicensePlateRec::~CLicensePlateRec()
{
}

/* 推理数据 */
bool Inference_NS::CLicensePlateRec::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::ClsData_S> &vClsDatas)
{
    /* 输出数据清空 */
    if (!vClsDatas.empty())
    {
        vClsDatas.clear();
    }

    if (!m_pModel)
    {
        return false;
    }

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInputData.nDataSize);
    if (!bIsInfe)
    {
        return false;
    }

    /* 填充数据 */
    setInputDatas((unsigned char*)stInputData.pData, 0);
    /* 运行 */
    if (!m_pModel->run())
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }
    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    std::vector<std::unique_ptr<float[]>> vBufKeeper;  /* 引入智能指针-保证内存生命周期 */
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        /* int8输出通过反量化转为float */
        if (m_vOutputAttrs[i].type == RKNN_TENSOR_INT8)
        {
            int8_t *pOut = (int8_t *)m_pOutputs[i]->virt_addr;
            int nOutSize = m_vOutputAttrs[i].size_with_stride;
            auto buf = std::make_unique<float[]>(nOutSize);
            float* pDst = buf.get();
            for (int index = 0; index < m_vOutputAttrs[i].n_elems; ++index)
            {
                pDst[index] = (pOut[index] - m_vOutputAttrs[i].zp) * m_vOutputAttrs[i].scale;
            }
            vInput.push_back(pDst);
            vBufKeeper.push_back(std::move(buf));  /* 保留智能指针，防止悬垂 */
        }
        else
        {
            float* pOut = (float*) m_pOutputs[i]->virt_addr;
            vInput.push_back(pOut);
        }
    }

    Inference_NS::ClsData_S stClsData;

    /* 车牌号输出 */
    std::vector<float> vOutData0;
    int nFeatureHeight = m_vOutputAttrs[0].dims[1];
    int nFeatureWidth = m_vOutputAttrs[0].dims[2];
    vOutData0.reserve(nFeatureHeight*nFeatureWidth);
    float* pOutput0 = (float*)vInput[0];
    std::copy(pOutput0, pOutput0 + (nFeatureHeight*nFeatureWidth), std::back_inserter(vOutData0));

    std::vector<int> vOutIndex;
    std::vector<float> vOutCharConf;
    for (size_t i = 0; i < vOutData0.size(); i += nFeatureWidth)
    {
        std::vector<float> vMiniOutData0(vOutData0.begin() + i, vOutData0.begin() + i + nFeatureWidth);

        auto max_it0 = std::max_element(vMiniOutData0.begin(), vMiniOutData0.end());
        int max_index0 = static_cast<int>(std::distance(vMiniOutData0.begin(), max_it0));
        vOutIndex.push_back(max_index0);
        vOutCharConf.push_back(vMiniOutData0[max_index0]);

        vMiniOutData0.clear();
    }

    /* 过滤重复字符 */
    decodePlate(vOutIndex, stClsData.vCls);

    /* 车牌颜色输出 */
    std::vector<float> vOutData1;
    int nFeatureNum1 = m_vOutputAttrs[1].dims[1];
    vOutData1.reserve(nFeatureNum1);
    float* pOutput1 = (float*)vInput[1];
    std::copy(pOutput1, pOutput1 + nFeatureNum1, std::back_inserter(vOutData1));

    std::vector<float> vSoftmaxOutData1(vOutData1.size());
    softMax(vOutData1, vSoftmaxOutData1);
    auto max_it1 = std::max_element(vSoftmaxOutData1.begin(), vSoftmaxOutData1.end());
    // 计算最大元素的下标
    if (max_it1 != vSoftmaxOutData1.end()) 
    {
        int max_index1 = static_cast<int>(std::distance(vSoftmaxOutData1.begin(), max_it1));
        stClsData.stCls.fConfidence = vSoftmaxOutData1[max_index1];
        stClsData.stCls.nLabel= max_index1;
    } else {
        std::cout << "容器为空，没有最大值" << std::endl;
    }

    vClsDatas.push_back(stClsData);

    return true;
}

bool Inference_NS::CLicensePlateRec::softMax(std::vector<float> vInputData, std::vector<float>& vOutData)
{
    float sumExp = 0.0f;
    /* 计算每个元素的指数并求和 */
    for (size_t i = 0; i < vInputData.size(); ++i) {
        vOutData[i] = expf(vInputData[i]);
        sumExp += vOutData[i];
    }
    /* 计算 softmax 值 */
    for (size_t i = 0; i < vInputData.size(); ++i) {
        vOutData[i] /= sumExp;
    }
    return true;
}

bool Inference_NS::CLicensePlateRec::decodePlate(std::vector<int> vInputPreds, std::vector<Inference_NS::Cls_S>& vOutPreds)
{
    int pre = 0;
    for (size_t i = 0; i < vInputPreds.size(); ++i) {
        if (vInputPreds[i] != 0 && vInputPreds[i] != pre) {
            Inference_NS::Cls_S stData;
            stData.nLabel = vInputPreds[i];
            vOutPreds.push_back(stData);
        }
        pre = vInputPreds[i];
    }

    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CLicensePlateRec::checkModelProConfig()
{
    return true;
}
