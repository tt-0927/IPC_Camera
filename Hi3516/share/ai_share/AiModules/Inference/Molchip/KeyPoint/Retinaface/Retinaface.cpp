/**
 * @file FaceDetect.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-30
 * 
 * @brief 
 */
#include "Retinaface.hpp"

#include <cstring>


Inference_NS::CRetinaface::CRetinaface(std::string strConfigPath)
    : CCVInferenceMOL(strConfigPath)
{
    /* 后处理类初始化 */
    m_postProcess = new PostProcess_NS::cRetinafacePostProcess;
}

Inference_NS::CRetinaface::~CRetinaface()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool Inference_NS::CRetinaface::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::PointData_S>& vPointDatas)
{
    /* 输出数据清空 */
    if (!vPointDatas.empty())
    {
        vPointDatas.clear();
    }

    if (!m_pModel || !m_postProcess)
    {
        return false;
    }

    /* 解析输入的数据结构体 */
    float fBoxThreshold = m_fBoxThreshold;
    float fNmsThreshold = m_fNmsThreshold;
    if (stInputData.stBoxs.fConfidence > 0 && stInputData.stBoxs.fConfidence < 1)
    {
        fBoxThreshold = stInputData.stBoxs.fConfidence;
    }
    if (stInputData.stBoxs.fNms > 0 && stInputData.stBoxs.fNms < 1)
    {
        fNmsThreshold = stInputData.stBoxs.fNms;
    }
    if(!stInputData.pData)
    {
        printf("输入的pData数据为空\n");
        return false;
    }

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInputData.nDataSize);
    if (!bIsInfe)
    {
        return false;
    }

    /* 填充数据 */
    std::memcpy(reinterpret_cast<void *>(m_vInputs[0].dataIn.virAddr), stInputData.pData, m_vInputs[0].dataIn.size);

    /* 运行 */
    if (!m_pModel->run(m_vInputs,
                        m_vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    for (int i = 0; i < m_stModelDesc.ioDesc.outputNum; ++i)
    {
        float *pOut = reinterpret_cast<float *>(m_vOutputs[i].dataOut.virAddr);
        vInput.push_back(pOut);
    }

    /* 后处理 */
    m_postProcess->postProcess(
        vInput[0],
        vInput[1],
        vInput[2],
        m_stModelDesc.ioDesc.out[0].tensor.dims[1],
        m_nLimitHeight,
        m_nLimitWidth,
        fBoxThreshold,
        fNmsThreshold,
        m_vSteps,
        m_vMinSizes,
        vPointDatas);

    return true;
}

/* 设置参数 */
bool Inference_NS::CRetinaface::setParam(float fBoxThreshold, float fNmsThreshold)
{
    if (m_fBoxThreshold != -1)
    {
        if (m_fBoxThreshold >= 0 && m_fBoxThreshold <= 1)
        {
            m_fBoxThreshold = fBoxThreshold;
        } else{
            printf("fBoxThreshold参数的值不在指定范围：[%f]\n", fBoxThreshold);
            return false;
        }
    }
    if (fNmsThreshold != -1)
    {
        if (fNmsThreshold >= 0 && fNmsThreshold <= 1)
        {
            m_fNmsThreshold = fNmsThreshold;
        } else{
            printf("fNmsThreshold参数的值不在指定范围：[%f]\n", fNmsThreshold);
            return false;
        }
    }
    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CRetinaface::checkModelProConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return false;
    }
    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    m_vSteps.clear();
    m_vMinSizes.clear();
    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    Json::Object *pJsonDataItem = NULL;
    Json::Object *pJsonDataItemObject = NULL;
    Json::Object *pItemObject = NULL;
    bool bRet = false;
    int nSize, nSizeO;
    int nStep ,nMinSizes;
    std::vector<int> vMinSizes;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "post_precess");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        goto EXIT;
    }
    /* 1、置信度 */
    bRet = Json::get(pJsonData, "confidence", m_fBoxThreshold);
    if (!bRet)
    {
        printf("解析confidence字段失败\n");
        goto EXIT;
    }
    /* 2、非极大值抑制阈值 */
    bRet = Json::get(pJsonData, "nms", m_fNmsThreshold);
    if (!bRet)
    {
        printf("解析nms字段失败\n");
        goto EXIT;
    }
    /* 3、获取步长Step */
    pJsonDataItem = Json::get(pJsonData, "steps");
    if (!pJsonDataItem)
    {
        printf("解析[steps]字段失败\n");
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[steps 数组大小异常]\n");
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            goto EXIT;
        }
        bRet = Json::Value::get(pItemObject, nStep);
        if (!bRet)
        {
            printf("解析[nStep]字段失败\n");
            goto EXIT;
        }
        m_vSteps.push_back(nStep);
    }
    /* 3、获取min_sizes */
    pJsonDataItem = Json::get(pJsonData, "min_sizes");
    if (!pJsonDataItem)
    {
        printf("解析[min_sizes]字段失败\n");
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[steps 数组大小异常]\n");
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pJsonDataItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pJsonDataItemObject)
        {
            printf("pJsonDataItemObject 获取数组节点失败\n");
            goto EXIT;
        }
        nSizeO = Json::Array::size(pJsonDataItemObject);
        if (nSizeO <= 0)
        {
            printf("解析[steps 数组大小异常]\n");
            goto EXIT;
        }
        vMinSizes.clear();
        for(int j=0;j<nSizeO;j++)
        {
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pJsonDataItemObject, j);
            if (NULL == pItemObject)
            {
                printf("pItemObject 获取数组节点失败\n");
                goto EXIT;
            }
            bRet = Json::Value::get(pItemObject, nStep);
            if (!bRet)
            {
                printf("解析[steps]字段失败\n");
                goto EXIT;
            }
            vMinSizes.push_back(nStep);
        }
        m_vMinSizes.push_back(vMinSizes);
    }
    return true;

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return false;
}