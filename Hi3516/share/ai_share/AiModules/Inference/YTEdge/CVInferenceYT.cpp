/**
 * @file CVInferenceYT.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-13
 *
 * @brief
 */
#include "CVInferenceYT.hpp"
#include <algorithm>
#include <cstring>

#include "dcl_ive.h"

Inference_NS::CCVInferenceYT::CCVInferenceYT(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CCVInferenceYT::~CCVInferenceYT()
{
    unInit();
}

/* 初始化HCP系统 */
static bool deviceInit(std::string &strConfigPath)
{
    static bool bRun = false;
    if(bRun)
    {
        return true;
    }
    bRun = true;
    // dclError nRet = dclInit(strConfigPath.c_str());
    // if (DCL_SUCCESS != nRet)
    // {
    //     printf("dcl init failed, errorCode = %d\n", static_cast<int32_t>(nRet));
    //     return false;
    // }

    // // create mpi channel
    // // dclError dclRet;
    // // uint32_t chId = 0;
    // // dclIveChnAttr attr;
    // // dclRet = dcliveCreateChn(chId, &attr);
    // // if (dclRet != DCL_SUCCESS)
    // // {
    // //     DCL_APP_LOG(DCL_ERROR, "create VPC channel:%d fail", chId);
    // //     return false;
    // // }
    // // DCL_APP_LOG(DCL_INFO, "create VPC channel:%d done", chId);

    // /* 获取版本号 */
    // int32_t nMajorVersion = 0, nMinorVersion = 0, nPatchVersion = 0;
    // nRet = dclrtGetVersion(&nMajorVersion, &nMinorVersion, &nPatchVersion);
    // if (DCL_SUCCESS != nRet)
    // {
    //     printf("dcl get version failed, errorCode = %d\n", static_cast<int32_t>(nRet));
    //     return false;
    // }
    // printf("TyHcp系统版本为: v%d.%d.%d\n", nMajorVersion, nMinorVersion, nPatchVersion);

    return true;
}

/* 初始化 */
bool Inference_NS::CCVInferenceYT::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (!checkModelConfig())
        {
            return false;
        };

        /* 初始化TYHCP系统，必须在模型初始化之前调用 */
        if (!deviceInit(m_strTyhcpConfigPath))
        {
            return false;
        };

        /* 创建模型操作类 */
        m_pModel = new CModelOpt(m_strModelPath);
        if (m_pModel)
        {
            if (!m_pModel->init())
            {
                return false;
            }

            /* 初始化值 */
            m_pModel->getInputAttrs(m_vInputAttrs);
            m_pModel->getOutputAttrs(m_vOutputAttrs);

            m_nInputNum = m_vInputAttrs.size();
            m_nOutputNum = m_vOutputAttrs.size();

            /* 创建输入数据对象 */
            m_pInputDataset = dclmdlCreateDataset();
            if (!m_pInputDataset)
            {
                printf("创建输入数据对象失败\n");
                return false;
            }
            /* 创建输出数据对象 */
            m_pOutputDataset = dclmdlCreateDataset();
            if (!m_pOutputDataset)
            {
                printf("创建输出数据对象失败\n");
                return false;
            }

            /* 打印模型信息 */
            if (false)
            {
                /* NPU利用率 */
                m_pModel->getUtilizationRate();
                /* 查询模型的网络结构 */
                m_pModel->getNetGrap();
            }

            /* 初始化参数 */
            return initParams();
        }
    }

    return false;
}

/* 去初始化HCP系统 */
static bool deviceFinalize()
{
    dclError nRet = dclFinalize();
    if (DCL_SUCCESS != nRet)
    {
        printf("finalize dcl failed, errorCode = %d\n", static_cast<int32_t>(nRet));
        return false;
    }

    return true;
}
/* 反初始化 */
bool Inference_NS::CCVInferenceYT::unInit()
{
    if (m_pInputDataset)
    {
        for (int i = 0; i < dclmdlGetDatasetNumBuffers(m_pInputDataset); ++i)
        {
            auto *pBuffer = dclmdlGetDatasetBuffer(m_pInputDataset, i);
            auto *pData = dclGetDataBufferAddr(pBuffer);
            (void)dclrtFree(pData);
            pData = nullptr;
            (void)dclDestroyDataBuffer(pBuffer);
            pBuffer = nullptr;
        }
        (void)dclmdlDestroyDataset(m_pInputDataset);
        m_pInputDataset = nullptr;
    }
    if (m_pOutputDataset)
    {
        for (int i = 0; i < dclmdlGetDatasetNumBuffers(m_pOutputDataset); ++i)
        {
            auto *pBuffer = dclmdlGetDatasetBuffer(m_pOutputDataset, i);
            auto *pData = dclGetDataBufferAddr(pBuffer);
            (void)dclrtFree(pData);
            pData = nullptr;
            (void)dclDestroyDataBuffer(pBuffer);
            pBuffer = nullptr;
        }
        (void)dclmdlDestroyDataset(m_pOutputDataset);
        m_pOutputDataset = nullptr;
    }

    for (auto &stInput : m_vInputAttrs)
    {
        if (stInput.stTensor.host_data)
        {
            delete stInput.stTensor.host_data;
            stInput.stTensor.host_data = nullptr;
        }
    }
    m_vInputAttrs.clear();
    m_nInputNum = 0;
    for (auto &stOutput : m_vOutputAttrs)
    {
        if (stOutput.stTensor.host_data)
        {
            delete stOutput.stTensor.host_data;
            stOutput.stTensor.host_data = nullptr;
        }
    }
    m_vOutputAttrs.clear();
    m_nOutputNum = 0;

    if (m_pModel)
    {
        delete m_pModel;
        m_pModel = nullptr;
    }
    /* 反初始化Tyhcp */
    deviceFinalize();
    return true;
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CCVInferenceYT::checkModelConfig()
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

    Json::Object *pJsonHandle = NULL;
    pJsonHandle = Json::init(pchJson);
    bool bRet;

    /* Device 配置文件路径 */
    bRet = Json::get(pJsonHandle, "sdk_path", m_strTyhcpConfigPath);
    if (!bRet)
    {
        printf("解析sdk_path字段失败\n");
        goto EXIT;
    }
    /* 获取模型地址 */
    bRet = Json::get(pJsonHandle, "model_path", m_strModelPath);
    if (!bRet)
    {
        printf("解析model_path字段失败\n");
        goto EXIT;
    }

    if (!checkModelPreConfig())
    {
        printf("json配置文件[%s], 预处理部分解析异常\n", m_strConfigPath.c_str());
        goto EXIT;
    }

    if (!checkModelInferConfig())
    {
        printf("json配置文件[%s], 推理部分解析异常\n", m_strConfigPath.c_str());
        goto EXIT;
    }

    if (!checkModelProConfig())
    {
        printf("json配置文件[%s], 后处理部分解析异常\n", m_strConfigPath.c_str());
        goto EXIT;
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

/* 校验模型配置文件中的预处理信息 */
bool Inference_NS::CCVInferenceYT::checkModelPreConfig()
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

    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    Json::Object *pJsonDataItem = NULL;
    Json::Object *pItemObject = NULL;
    bool bRet = false;
    int nSize = 0;
    int i;
    int nSizeItem, nMean, nStd;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "pre_process");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        goto EXIT;
    }

    /* 1、获取模型输入大小限制 */
    pJsonDataItem = Json::get(pJsonData, "size");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        goto EXIT;
    }
    m_vModelInputSize.clear();
    for (i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nSizeItem);
        if (!bRet)
        {
            printf("解析[size]字段失败\n");
            goto EXIT;
        }
        m_vModelInputSize.push_back(nSizeItem);
    }
    /* 2、图片通道 */
    bRet = Json::get(pJsonData, "channel", m_nChannel);
    if (!bRet)
    {
        printf("解析channel字段失败\n");
        goto EXIT;
    }
    /* 3、输入是数据的格式 */
    bRet = Json::get(pJsonData, "type", strType);
    if (!bRet)
    {
        printf("解析type字段失败\n");
        goto EXIT;
    }
    /* 4、归一化-均值 */
    pJsonDataItem = Json::get(pJsonData, "mean");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
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

        bRet = Json::Value::get(pItemObject, nMean);
        if (!bRet)
        {
            printf("解析[mean]字段失败\n");
            goto EXIT;
        }
        m_vMean.push_back(nMean);
    }
    /* 5、归一化-方差 */
    pJsonDataItem = Json::get(pJsonData, "std");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
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

        bRet = Json::Value::get(pItemObject, nStd);
        if (!bRet)
        {
            printf("解析[std]字段失败\n");
            goto EXIT;
        }
        m_vStd.push_back(nStd);
    }
    /* 6、图片缩放时，是否填充 */
    bRet = Json::get(pJsonData, "padding", m_nPadding);
    if (!bRet)
    {
        printf("解析padding字段失败\n");
        goto EXIT;
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

/* 校验模型配置文件中的模型推理信息 */
bool Inference_NS::CCVInferenceYT::checkModelInferConfig()
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

    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    bool bRet = false;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "inference");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        goto EXIT;
    }

    /* 获取芯片号 */
    bRet = Json::get(pJsonData, "framework", m_strFramework);
    if (!bRet)
    {
        printf("解析framework字段失败\n");
        goto EXIT;
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

/* 校验模型配置文件中的后处理信息 */
bool Inference_NS::CCVInferenceYT::checkModelProConfig()
{
    return true;
}

/* 获取输入图片限制 */
bool Inference_NS::CCVInferenceYT::getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel)
{
    if (m_pModel &&
        m_vInputAttrs.size() > nIndex &&
        m_vInputAttrs[nIndex].stTensor.dimCount > 3)
    {
        if (m_vInputAttrs[0].stTensor.dims[1] <= 4)
        {
            nChannel = m_vInputAttrs[nIndex].stTensor.dims[1];
            nHeight = m_vInputAttrs[nIndex].stTensor.dims[2];
            nWidth = m_vInputAttrs[nIndex].stTensor.dims[3];
        }
        else
        {
            nHeight = m_vInputAttrs[nIndex].stTensor.dims[1];
            nWidth = m_vInputAttrs[nIndex].stTensor.dims[2];
            nChannel = m_vInputAttrs[nIndex].stTensor.dims[3];
        }

        return true;
    }

    return false;
}

/* 获取模型的Tensor信息 */
bool Inference_NS::CCVInferenceYT::getModelTensor(int nIndex, TensorAttr_S &stTensor)
{
    if (nIndex > m_vInputAttrs.size())
    {
        printf("nIndex[%d]超出m_vInputAttrs[%ld]的范围\n", nIndex, m_vInputAttrs.size());
        return false;
    }
    stTensor = m_vInputAttrs[nIndex];
    return true;
}

/* 初始化输入输出参数 */
bool Inference_NS::CCVInferenceYT::initParams()
{
    dclError nRet;
    if (m_pInputDataset)
    {
        /* 输入信息初始化 */
        for (int i = 0; i < m_vInputAttrs.size(); ++i)
        {
            if (i == 0 && m_vInputAttrs[0].stTensor.dimCount > 3)
            {
                if (m_vInputAttrs[0].stTensor.dims[1] <= 4)
                {
                    m_nLimitChannel = m_vInputAttrs[0].stTensor.dims[1];
                    m_nLimitHeight = m_vInputAttrs[0].stTensor.dims[2];
                    m_nLimitWidth = m_vInputAttrs[0].stTensor.dims[3];
                }
                else
                {
                    m_nLimitHeight = m_vInputAttrs[0].stTensor.dims[1];
                    m_nLimitWidth = m_vInputAttrs[0].stTensor.dims[2];
                    m_nLimitChannel = m_vInputAttrs[0].stTensor.dims[3];
                }
                printf("输入图片限制 [%d]x[%d]x[%d]\n",
                       m_nLimitWidth,
                       m_nLimitHeight,
                       m_nLimitChannel);
            }

            nRet = dclrtMallocCachedEx(&(m_vInputAttrs[i].stTensor.data), &(m_vInputAttrs[i].stTensor.phyAddr), m_vInputAttrs[i].nDataSize, 16, DCL_MEM_MALLOC_NORMAL_ONLY);
            // nRet = dclrtMallocEx(&(m_vInputAttrs[i].stTensor.data), &(m_vInputAttrs[i].stTensor.phyAddr), m_vInputAttrs[i].nDataSize, 16, DCL_MEM_MALLOC_NORMAL_ONLY);
            if (DCL_SUCCESS != nRet)
            {
                printf("输入分配内存失败，错误码[%d]\n", nRet);
                return false;
            }
            dclDataBuffer *buffer = dclCreateDataBuffer(m_vInputAttrs[i].stTensor.data, m_vInputAttrs[i].nDataSize);
            nRet = dclmdlAddDatasetBuffer(m_pInputDataset, buffer);
            if (DCL_SUCCESS != nRet)
            {
                printf("输入dclmdlAddDatasetBuffer 失败，错误码[%d]\n", nRet);
                return false;
            }
            /* host的CPU内存 */
            m_vInputAttrs[i].stTensor.host_data = new char[m_vInputAttrs[i].nDataSize];
        }
    }

    if (m_pOutputDataset)
    {
        /* 输出信息初始化 */
        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            nRet = dclrtMallocEx(&(m_vOutputAttrs[i].stTensor.data), &(m_vOutputAttrs[i].stTensor.phyAddr), m_vOutputAttrs[i].nDataSize, 16, DCL_MEM_MALLOC_NORMAL_ONLY);
            // nRet = dclrtMallocEx(&(m_vOutputAttrs[i].stTensor.data), &(m_vOutputAttrs[i].stTensor.phyAddr), m_vOutputAttrs[i].nDataSize, 16, DCL_MEM_MALLOC_NORMAL_ONLY);
            if (DCL_SUCCESS != nRet)
            {
                printf("输入分配内存失败，错误码[%d]\n", nRet);
                return false;
            }
            dclDataBuffer *buffer = dclCreateDataBuffer(m_vOutputAttrs[i].stTensor.data, m_vOutputAttrs[i].nDataSize);
            nRet = dclmdlAddDatasetBuffer(m_pOutputDataset, buffer);
            if (DCL_SUCCESS != nRet)
            {
                printf("输出dclmdlAddDatasetBuffer 失败，错误码[%d]\n", nRet);
                return false;
            }
            /* host的CPU内存 */
            m_vOutputAttrs[i].stTensor.host_data = new char[m_vOutputAttrs[i].nDataSize];
        }
    }

    return true;
}

/* 定义一个数据输入存储函数 */
bool Inference_NS::CCVInferenceYT::setInputDatas(unsigned char *pDataBuffer, int nDataSize, int nInputIndex)
{
    if (nInputIndex > m_nInputNum)
    {
        printf("下标nInputIndex[%d]超出了模型输入分支[%d]\n", nInputIndex, m_nInputNum);
        return false;
    }
    if (nDataSize != m_vInputAttrs[nInputIndex].nDataSize)
    {
        printf("输入长度nDataSize[%d]不等于模型需要输出长度[%d]\n", nDataSize, m_vInputAttrs[nInputIndex].nDataSize);
        return false;
    }

    dclError nRet;
    // nRet = dclrtMemcpy(
    //     m_vInputAttrs[nInputIndex].stTensor.data, // dst
    //     m_vInputAttrs[nInputIndex].nDataSize,     // dstSize
    //     pDataBuffer,                              // src
    //     m_vInputAttrs[nInputIndex].nDataSize,
    //     DCL_MEMCPY_HOST_TO_DEVICE);
    nRet = dclrtMemcpyEx(
        m_vInputAttrs[nInputIndex].stTensor.data,
        m_vInputAttrs[nInputIndex].stTensor.phyAddr,
        m_vInputAttrs[nInputIndex].nDataSize,
        pDataBuffer,
        0,
        nDataSize,
        DCL_MEMCPY_HOST_TO_DEVICE);
    if (DCL_SUCCESS != nRet)
    {
        printf("dclrtMemcpy 拷贝失败\n");
        return false;
    }
    return true;
}
