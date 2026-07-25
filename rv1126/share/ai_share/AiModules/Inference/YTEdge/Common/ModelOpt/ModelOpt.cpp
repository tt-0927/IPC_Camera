/**
 * @file ModelOpt.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-11
 *
 * @brief RK模型操作
 */
#include "ModelOpt.hpp"
#include <iostream>
#include <cstring>
#include <fstream>

Inference_NS::CModelOpt::CModelOpt(std::string strModelPath)
    : m_strModelPath(strModelPath)
{
}

Inference_NS::CModelOpt::~CModelOpt()
{
    unInit();
}

/* 设置模型路径 */
void Inference_NS::CModelOpt::setModelPath(std::string strModelPath)
{
    m_strModelPath = strModelPath;
}

/* 初始化模型 */
bool Inference_NS::CModelOpt::init()
{
    if (m_bInitialized)
    {
        /* 模型已被初始化 */
        return true;
    }
    bool bRet = false;
    dclError nRet = 0;
    int nIONum = 0;

    /* 加载模型 */
    nRet = dclmdlLoadFromFile(m_strModelPath.c_str(), &m_nModelId);
    if (DCL_SUCCESS != nRet)
    {
        printf("加载模型识别 [%s], 错误码[%d]\n", m_strModelPath.c_str(), nRet);
        goto EXIT;
    }

    /* 初始化成功 */
    m_bInitialized = true;

    /* 创建 dclmdlDesc 类型的数据 */
    m_pModelDesc = dclmdlCreateDesc();
    if (!m_pModelDesc)
    {
        printf("创建模型描述对象失败\n");
        goto EXIT;
    }
    nRet = dclmdlGetDesc(m_pModelDesc, m_nModelId);
    if (nRet != DCL_ERROR_NONE)
    {
        printf("获取模型描述对象失败，模型ID[%d]，错误码[%d]\n", m_nModelId, nRet);
        goto EXIT;
    }

    /* 获取输入tensor的属性信息 */
    m_vInputAttrs.clear();
    /* 获取输入条个数 */
    nIONum = dclmdlGetNumInputs(m_pModelDesc);
    for (int n = 0; n < nIONum; n++)
    {
        /* 获取属性信息 */
        dclmdlIODims dims;
        memset(&dims, 0, sizeof(dims));
        nRet = dclmdlGetInputDims(m_pModelDesc, n, &dims);
        if (DCL_SUCCESS != nRet)
        {
            printf("获取输入数据属性失败,错误码[%d]\n", nRet);
            goto EXIT;
        }
        /* 获取输入信息 */
        TensorAttr_S stTensorAttr;
        stTensorAttr.stTensor.idx = n;
        stTensorAttr.stTensor.copyDims(dims);
        stTensorAttr.stTensor.name = dclmdlGetInputNameByIndex(m_pModelDesc, n);
        stTensorAttr.stTensor.dtype = dclmdlGetInputDataType(m_pModelDesc, n);
        stTensorAttr.stTensor.layout = dclmdlGetInputFormat(m_pModelDesc, n);
        stTensorAttr.nDataSize = dclmdlGetInputSizeByIndex(m_pModelDesc, n);
        m_vInputAttrs.push_back(stTensorAttr);
        /* 打印输入信息 */
        printf("input[%d], name: [%s], nDim: [%ld], shape: [%s], dtype: [%s]\n",
               n, stTensorAttr.stTensor.name.c_str(), stTensorAttr.stTensor.dimCount,
               stTensorAttr.stTensor.dimsToString().c_str(), dclDataTypeToString(stTensorAttr.stTensor.dtype).c_str());
    }

    /* 获取输出tensor的属性信息 */
    m_vOutputAttrs.clear();
    /* 获取输入条个数 */
    nIONum = dclmdlGetNumOutputs(m_pModelDesc);
    for (int n = 0; n < nIONum; n++)
    {
        /* 获取属性信息 */
        dclmdlIODims dims;
        nRet = dclmdlGetOutputDims(m_pModelDesc, n, &dims);
        if (DCL_SUCCESS != nRet)
        {
            printf("获取输入数据属性失败,错误码[%d]\n", nRet);
            goto EXIT;
        }
        /* 获取输出信息 */
        TensorAttr_S stTensorAttr;
        stTensorAttr.stTensor.idx = n;
        stTensorAttr.stTensor.copyDims(dims);
        stTensorAttr.stTensor.name = dclmdlGetOutputNameByIndex(m_pModelDesc, n);
        stTensorAttr.stTensor.dtype = dclmdlGetOutputDataType(m_pModelDesc, n);
        stTensorAttr.stTensor.layout = dclmdlGetOutputFormat(m_pModelDesc, n);
        stTensorAttr.nDataSize = dclmdlGetOutputSizeByIndex(m_pModelDesc, n);
        m_vOutputAttrs.push_back(stTensorAttr);
        /* 打印输出信息 */
        printf("output[%d], name: [%s], nDim: [%ld], shape: [%s], dtype: [%s]\n",
               n, stTensorAttr.stTensor.name.c_str(), stTensorAttr.stTensor.dimCount,
               stTensorAttr.stTensor.dimsToString().c_str(), dclDataTypeToString(stTensorAttr.stTensor.dtype).c_str());
    }

    return true;
EXIT:
    if (m_pModelDesc)
    {
        dclmdlDestroyDesc(m_pModelDesc);
    }
    return bRet;
}

/* 反初始化模型 */
bool Inference_NS::CModelOpt::unInit()
{
    if (m_pModelDesc)
    {
        dclmdlDestroyDesc(m_pModelDesc);
    }
    if (m_bInitialized)
    {
        dclmdlUnload(m_nModelId);
        m_nModelId = 1000000;
        m_bInitialized = false;
        return true;
    }

    return true;
}

/* 获取模型输入参数 */
bool Inference_NS::CModelOpt::getInputAttrs(std::vector<TensorAttr_S> &vInputAttrs)
{
    if (m_bInitialized)
    {
        vInputAttrs = m_vInputAttrs;
        return true;
    }
    return false;
}

/* 获取模型输出参数 */
bool Inference_NS::CModelOpt::getOutputAttrs(std::vector<TensorAttr_S> &vOutputAttrs)
{
    if (m_bInitialized)
    {
        vOutputAttrs = m_vOutputAttrs;
        return true;
    }
    return false;
}

/* 运行模型 */
bool Inference_NS::CModelOpt::run(dclmdlDataset *pInputDataset, dclmdlDataset *pOutputDataset)
{
    if (pInputDataset == nullptr || pOutputDataset == nullptr)
    {
        printf("pInputDataset 或 pOutputDataset指针 为空\n");
        return false;
    }
    dclError nRet = 0;

    if (m_bInitialized)
    {
        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            dclrtMemFlushEx(m_vOutputAttrs[i].stTensor.phyAddr, m_vOutputAttrs[i].stTensor.data, m_vOutputAttrs[i].stTensor.size_bytes());
        }

        /* 运行 */
        nRet = dclmdlExecute(m_nModelId, pInputDataset, pOutputDataset);
        if (DCL_SUCCESS != nRet)
        {
            return false;
        }

        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            dclrtMemInvalidateEx(m_vOutputAttrs[i].stTensor.phyAddr, m_vOutputAttrs[i].stTensor.data, m_vOutputAttrs[i].stTensor.size_bytes());
        }

        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            /* 实现 Host 内、Host 与 Device 之间、Device 内的同步内存复制 */
            dclrtMemcpyEx(
                (void *)m_vOutputAttrs[i].stTensor.host_data,
                0,
                m_vOutputAttrs[i].stTensor.size_bytes(),
                m_vOutputAttrs[i].stTensor.data,
                m_vOutputAttrs[i].stTensor.phyAddr,
                m_vOutputAttrs[i].stTensor.size_bytes(),
                DCL_MEMCPY_DEVICE_TO_HOST);
        }

        return true;
    }

    return false;
}

/* NPU利用率 */
bool Inference_NS::CModelOpt::getUtilizationRate()
{
    int32_t nDevCount = 0;
    dclNpuUtilizationRate aRates[8];
    dclError err = dclNpuGetUtilizationRate(&nDevCount, aRates);
    if (err != DCL_SUCCESS)
    {
        printf("dclNpuGetUtilizationRate 调用失败\n");
        return false;
    }
    printf("=============== NPU利用率 ===============\n");
    for (int i = 0; i < nDevCount; i++)
    {
        printf("Device[%d] NPU占用[%f]\n", aRates[i].devId, aRates[i].rate * 100);
    }
    printf("========================================\n");
    return true;
}

/* 查询模型的网络结构 */
bool Inference_NS::CModelOpt::getNetGrap()
{
    dclError nRet;

    /* 查询模型的网络结构字符串长度 */
    uint32_t nNetGrapSize;
    nRet = dclmdlGetGraphSize(m_nModelId, &nNetGrapSize);
    if (nRet != DCL_SUCCESS)
    {
        printf("dclmdlGetGraphSize 调用失败\n");
        return false;
    }
    /* 查询模型的网络结构 */
    char *graphBuf = (char *)malloc(nNetGrapSize);
    nRet = dclmdlGetGraph(m_nModelId, graphBuf, nNetGrapSize);
    if (nRet != DCL_SUCCESS)
    {
        printf("dclmdlGetGraph 调用失败\n");
        free(graphBuf);
        return false;
    }
    // printf("========== modelId=%u 模型结构 (JSON) ==========\n", m_nModelId);
    // printf("%s\n", (char *)graphBuf);
    // printf("===============================================\n");
    std::string strFileName = "NetGrap.json";
    FILE *fp = fopen(strFileName.c_str(), "w");
    if (fp == nullptr)
    {
        printf("open %s for write failed\n", strFileName.c_str());
        free(graphBuf);
        return false;
    }
    fputs(graphBuf, fp);
    fclose(fp);
    printf("模型结构已保存到: %s\n", strFileName.c_str());

    free(graphBuf);
    return true;
}

/* 数据格式转换 */
std::string Inference_NS::CModelOpt::dclDataTypeToString(dclDataType &dataType)
{
    switch (dataType)
    {
    case DCL_FLOAT:
        return "float32";
    case DCL_FLOAT16:
        return "float16";
    case DCL_INT8:
        return "int8";
    case DCL_INT32:
        return "int32";
    case DCL_UINT8:
        return "uint8";
    case DCL_INT16:
        return "int16";
    case DCL_UINT16:
        return "uint16";
    case DCL_UINT32:
        return "uint32";
    case DCL_INT64:
        return "int64";
    case DCL_UINT64:
        return "uint64";
    case DCL_DOUBLE:
        return "float64";
    case DCL_BOOL:
        return "bool";
    default:
        printf("Not support dataType: %d\n", dataType);
        std::exit(-1);
    }
}