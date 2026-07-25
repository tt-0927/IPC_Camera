/*
 * @FilePath     : CVInferenceRK.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-22 13:54:11
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 19:44:39
 * @Description  :
 */
#include "CVInferenceRK.hpp"
#include <algorithm>
#include <cstring>

Inference_NS::CCVInferenceRK::CCVInferenceRK(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CCVInferenceRK::~CCVInferenceRK()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CCVInferenceRK::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (!checkModelConfig())
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

            /* 初始化输入参数 */
            if (m_nInputNum > 0)
            {
                m_pInputs = new rknn_tensor_mem *[m_nInputNum];
            }

            /* 初始化输出参数 */
            if (m_nOutputNum > 0)
            {
                m_pOutputs = new rknn_tensor_mem *[m_nOutputNum];
            }

            /* 初始化参数 */
            return initParams();
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CCVInferenceRK::unInit()
{
    if (m_pInputs)
    {
        for (int i = 0; i < m_vInputAttrs.size(); i++)
        {
            rknn_destroy_mem(m_pModel->m_hCtx, m_pInputs[i]);
        }
        m_vInputAttrs.clear();
        m_pInputs = nullptr;
        m_nInputNum = 0;
    }

    if (m_pOutputs)
    {
        for (int i = 0; i < m_vOutputAttrs.size(); i++)
        {
            rknn_destroy_mem(m_pModel->m_hCtx, m_pOutputs[i]);
        }
        m_vOutputAttrs.clear();
        m_pOutputs = nullptr;
        m_nOutputNum = 0;
    }

    if (m_pModel)
    {
        delete m_pModel;
        m_pModel = nullptr;
    }

    return true;
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CCVInferenceRK::checkModelConfig()
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
    bool bRet = true;

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
        bRet = false;
        goto EXIT;
    }

    if (!checkModelInferConfig())
    {
        printf("json配置文件[%s], 推理部分解析异常\n", m_strConfigPath.c_str());
        bRet = false;
        goto EXIT;
    }

    if (!checkModelProConfig())
    {
        printf("json配置文件[%s], 后处理部分解析异常\n", m_strConfigPath.c_str());
        bRet = false;
        goto EXIT;
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 校验模型配置文件中的预处理信息 */
bool Inference_NS::CCVInferenceRK::checkModelPreConfig()
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
    bool bRet = true;
    int nSize = 0;
    int i;
    int nSizeItem, nMean, nStd;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "pre_process");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        bRet = false;
        goto EXIT;
    }

    /* 1、获取模型输入大小限制 */
    pJsonDataItem = Json::get(pJsonData, "size");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
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
            bRet = false;
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
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nMean);
        if (!bRet)
        {
            printf("解析[mean]字段失败\n");
            bRet = false;
            goto EXIT;
        }
        m_vMean.push_back(nMean);
    }
    /* 5、归一化-方差 */
    pJsonDataItem = Json::get(pJsonData, "std");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;
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

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 校验模型配置文件中的模型推理信息 */
bool Inference_NS::CCVInferenceRK::checkModelInferConfig()
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
    bool bRet = true;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "inference");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        bRet = false;
        goto EXIT;
    }

    /* 获取芯片号 */
    bRet = Json::get(pJsonData, "framework", m_strFramework);
    if (!bRet)
    {
        printf("解析framework字段失败\n");
        goto EXIT;
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 校验模型配置文件中的后处理信息 */
bool Inference_NS::CCVInferenceRK::checkModelProConfig()
{
    return true;
}

/* 获取输入图片限制 */
bool Inference_NS::CCVInferenceRK::getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel)
{
    if (m_pModel &&
        m_vInputAttrs.size() > nIndex &&
        m_vInputAttrs[nIndex].n_dims > 3)
    {
        if (m_vInputAttrs[nIndex].fmt == RKNN_TENSOR_NCHW)
        {
            nChannel = m_vInputAttrs[nIndex].dims[1];
            nHeight = m_vInputAttrs[nIndex].dims[2];
            nWidth = m_vInputAttrs[nIndex].dims[3];
        }
        else
        {
            nHeight = m_vInputAttrs[nIndex].dims[1];
            nWidth = m_vInputAttrs[nIndex].dims[2];
            nChannel = m_vInputAttrs[nIndex].dims[3];
        }

        return true;
    }

    return false;
}

bool containsRv1103(const std::string &str)
{
    std::string target = "rv1103";
    std::string lowerStr;
    std::transform(str.begin(), str.end(), std::back_inserter(lowerStr),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return lowerStr.find(target) != std::string::npos;
}

/* 初始化输入输出参数 */
bool Inference_NS::CCVInferenceRK::initParams()
{
    if (m_pInputs)
    {
        /* 初始化每个 rknn_input 实例 */
        for (int i = 0; i < m_vInputAttrs.size(); ++i)
        {
            if (i == 0 && m_vInputAttrs[0].n_dims > 3)
            {
                if (m_vInputAttrs[0].fmt == RKNN_TENSOR_NCHW)
                {
                    m_nLimitChannel = m_vInputAttrs[0].dims[1];
                    m_nLimitHeight = m_vInputAttrs[0].dims[2];
                    m_nLimitWidth = m_vInputAttrs[0].dims[3];
                }
                else
                {
                    m_nLimitHeight = m_vInputAttrs[0].dims[1];
                    m_nLimitWidth = m_vInputAttrs[0].dims[2];
                    m_nLimitChannel = m_vInputAttrs[0].dims[3];
                }

                printf("输入图片限制 [%d]x[%d]x[%d]\n",
                       m_nLimitWidth,
                       m_nLimitHeight,
                       m_nLimitChannel);
            }

            /* 输出数据的相关配置 */
            /* 如果设置为 uint8，将在 NPU 中进行归一化和量化处理 */
            m_vInputAttrs[i].type = RKNN_TENSOR_UINT8;
            /*  默认格式为 NHWC，NPU 仅支持在零拷贝模式下使用 NHWC 格式 */
            m_vInputAttrs[i].fmt = RKNN_TENSOR_NHWC;
            /* 申请输入数据内存 */
            m_pInputs[i] = rknn_create_mem(m_pModel->m_hCtx, m_vInputAttrs[i].size_with_stride);
            printf("Input %u: size=%u, size_with_stride=%u\n",
                   i,
                   m_vInputAttrs[i].size,
                   m_vInputAttrs[i].size_with_stride);
            /* 调用rknn_set_io_mem 让NPU使用上面申请到的内存 */
            rknn_set_io_mem(m_pModel->m_hCtx, m_pInputs[i], &m_vInputAttrs[i]);
        }
    }

    if (m_pOutputs)
    {
        std::string strLowerStr;
        std::transform(
            m_strFramework.begin(),
            m_strFramework.end(),
            std::back_inserter(strLowerStr),
            [](unsigned char c)
            { return std::tolower(c); });
        bool bNeedInt8 = (strLowerStr.find("rv1106") != std::string::npos) || (strLowerStr.find("rv1103") != std::string::npos);
        if(bNeedInt8)
        {
            printf("=======================================================\n");
            printf("** 开启 {rv1103/rv1106} 芯片推理 **\n");
            printf("=======================================================\n");
        }
        /* 初始化每个 rknn_output 实例 */
        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            if (bNeedInt8)
            {
                /* 申请输入数据内存 */
                m_pOutputs[i] = rknn_create_mem(m_pModel->m_hCtx, m_vOutputAttrs[i].n_elems);
            }
            else
            {
                /* 输出数据的相关配置 */
                m_vOutputAttrs[i].type = RKNN_TENSOR_FLOAT32;
                /* 申请输入数据内存 */
                m_pOutputs[i] = rknn_create_mem(m_pModel->m_hCtx, m_vOutputAttrs[i].n_elems * sizeof(float));
            }
            /* 调用rknn_set_io_mem 让NPU使用上面申请到的内存 */
            rknn_set_io_mem(m_pModel->m_hCtx, m_pOutputs[i], &m_vOutputAttrs[i]);
        }
    }

    return true;
}

/* 定义一个数据输入存储函数 */
void Inference_NS::CCVInferenceRK::setInputDatas(unsigned char *pDataBuffer, int nInputIndex)
{
    /* 将输入数据拷贝到输入张量内存 */
    int nInputWidth = m_vInputAttrs[nInputIndex].dims[2];
    int nInputStride = m_vInputAttrs[nInputIndex].w_stride;
    if (nInputWidth == nInputStride)
    {
        memcpy(m_pInputs[nInputIndex]->virt_addr, pDataBuffer, nInputWidth * m_vInputAttrs[nInputIndex].dims[1] * m_vInputAttrs[nInputIndex].dims[3]);
    }
    else
    {
        int nInputHeight = m_vInputAttrs[nInputIndex].dims[1];
        int nInputChannel = m_vInputAttrs[nInputIndex].dims[3];
        /* 按照跨度从源地址到目标地址进行拷贝 */
        uint8_t *pSrcPtr = pDataBuffer;
        uint8_t *pDstPtr = (uint8_t *)m_pInputs[nInputIndex]->virt_addr;
        /* 宽度乘以通道的元素数量 */
        int nSrcWcElems = nInputWidth * nInputChannel;
        int nDstWcElems = nInputStride * nInputChannel;
        for (int h = 0; h < nInputHeight; ++h)
        {
            memcpy(pDstPtr, pSrcPtr, nSrcWcElems);
            pSrcPtr += nSrcWcElems;
            pDstPtr += nDstWcElems;
        }
    }
}

/* 能否推理的使用前判断 */
bool Inference_NS::CCVInferenceRK::inferenceInfe(int nImgSize, int nInputIndex)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (nImgSize <= 0)
    {
        printf("推理失败-传入的数据大小nImgSize[%d]小于0\n", nImgSize);
        return false;
    }

    int nDstSize = 1;
    for (int nIndex = 0; nIndex < m_vInputAttrs[nInputIndex].n_dims; nIndex++)
    {
        nDstSize *= m_vInputAttrs[nInputIndex].dims[nIndex];
    }
    nDstSize *= sizeof(float);

    if (nImgSize != nDstSize)
    {
        printf("模型[%s]需要的大小与输入图片大小不一致 nImgSize[%d] nDstSize[%d]\n",
               m_strModelPath.c_str(),
               nImgSize,
               nDstSize);
        return false;
    }
    return true;
}
