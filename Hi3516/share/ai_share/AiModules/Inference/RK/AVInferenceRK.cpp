/**
 * @file AVInferenceRK.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-05
 *
 * @brief
 */
#include "AVInferenceRK.hpp"

#include <cstring>

Inference_NS::CAVInferenceRK::CAVInferenceRK(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CAVInferenceRK::~CAVInferenceRK()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CAVInferenceRK::init()
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

            m_nInputNum  = m_vInputAttrs.size();
            m_nOutputNum = m_vOutputAttrs.size();

            /* 初始化输入参数 */
            if (m_nInputNum > 0)
            {
                m_pInputs = new rknn_input[m_nInputNum];
            }

            /* 初始化输出参数 */
            if (m_nOutputNum > 0)
            {
                m_pOutputs = new rknn_output[m_nOutputNum];
            }

            /* 初始化参数 */
            return initParams();
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CAVInferenceRK::unInit()
{
    if (m_pInputs)
    {
        m_vInputAttrs.clear();
        delete[] m_pInputs;
        m_pInputs   = nullptr;
        m_nInputNum = 0;
    }

    if (m_pOutputs)
    {
        m_vOutputAttrs.clear();
        delete[] m_pOutputs;
        m_pOutputs   = nullptr;
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
bool Inference_NS::CAVInferenceRK::checkModelConfig()
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
bool Inference_NS::CAVInferenceRK::checkModelPreConfig()
{
    return true;
}

/* 校验模型配置文件中的模型推理信息 */
bool Inference_NS::CAVInferenceRK::checkModelInferConfig()
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
bool Inference_NS::CAVInferenceRK::checkModelProConfig()
{
    return true;
}


/* 获取输入音频限制 */
bool Inference_NS::CAVInferenceRK::getSizeLimit(int nIndex, std::vector<int> &vModelSize)
{
    if (m_pModel &&
        m_vInputAttrs.size() > nIndex)
    {
        for (int i = 0; i < m_vInputAttrs[nIndex].n_dims; i++)
        {
            vModelSize.push_back(m_vInputAttrs[nIndex].dims[i]);
        }
        return true;
    }

    return false;
}


/* 获取输出音频限制 */
bool Inference_NS::CAVInferenceRK::getOutputSizeLimit(int nIndex, std::vector<int> &vModelSize)
{
    if (m_pModel &&
        m_vOutputAttrs.size() > nIndex)
    {
        for (int i = 0; i < m_vOutputAttrs[nIndex].n_dims; i++)
        {
            vModelSize.push_back(m_vOutputAttrs[nIndex].dims[i]);
        }
        return true;
    }

    return false;
}

/* 初始化输入输出参数 */
bool Inference_NS::CAVInferenceRK::initParams()
{
    if (m_pInputs)
    {
        /* 初始化每个 rknn_input 实例 */
        for (int i = 0; i < m_vInputAttrs.size(); ++i)
        {
            printf("输入文本限制 [%d] ", i);
            for (int nIndex = 0; nIndex < m_vInputAttrs[i].n_dims; nIndex++)
            {
                printf("x[%d]",
                       m_vInputAttrs[i].dims[nIndex]);
            }
            printf("\n");

            m_pInputs[i].index = i;
            /* 输出数据的相关配置 */
            if (m_vInputAttrs[i].type == RKNN_TENSOR_FLOAT16)
            {
                m_pInputs[i].size = m_vInputAttrs[i].n_elems * sizeof(float);
                m_pInputs[i].type = RKNN_TENSOR_FLOAT32; /* 根据需要设置数据类型 */
                m_pInputs[i].fmt = m_vInputAttrs[i].fmt;
                m_pInputs[i].buf = (float *)malloc(m_pInputs[i].size);
                memset(m_pInputs[i].buf, 0, m_pInputs[i].size);
            }
            else if (m_vInputAttrs[i].type == RKNN_TENSOR_INT64)
            {
                m_pInputs[i].size = m_vInputAttrs[i].n_elems * sizeof(int64_t);
                m_pInputs[i].type = RKNN_TENSOR_INT64; /* 根据需要设置数据类型 */
                m_pInputs[i].fmt = m_vInputAttrs[i].fmt;
                m_pInputs[i].buf = (int64_t *)malloc(m_pInputs[i].size);
                memset(m_pInputs[i].buf, 0, m_pInputs[i].size);
            }
            else if(m_vInputAttrs[i].type == RKNN_TENSOR_INT32)
            {
                m_pInputs[i].size = m_vInputAttrs[i].n_elems * sizeof(int32_t);
                m_pInputs[i].type = RKNN_TENSOR_INT32;
                m_pInputs[i].fmt = m_vInputAttrs[i].fmt;
                m_pInputs[1].buf = (int32_t *)malloc(m_pInputs[i].size);      
                memset(m_pInputs[i].buf, 0, m_pInputs[i].size);
            }
        }
    }

    if (m_pOutputs)
    {
        /* 初始化每个 rknn_output 实例 */
        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            m_pOutputs[i].index = i;

            if (m_vOutputAttrs[i].type == RKNN_TENSOR_FLOAT16)
            {
                m_pOutputs[i].size = m_vOutputAttrs[i].n_elems * sizeof(float);
                m_pOutputs[i].is_prealloc = true;
                m_pOutputs[i].want_float = 1;
                m_pOutputs[i].buf = (float *)malloc(m_pOutputs[i].size);
                memset(m_pOutputs[i].buf, 0, m_pOutputs[i].size);
            }
            else if (m_vOutputAttrs[i].type == RKNN_TENSOR_INT64)
            {
                m_pOutputs[i].size = m_vOutputAttrs[i].n_elems * sizeof(int64_t);
                m_pOutputs[i].is_prealloc = true;
                m_pOutputs[i].want_float = 0;
                m_pOutputs[i].buf = (int64_t *)malloc(m_pOutputs[i].size);
                memset(m_pOutputs[i].buf, 0, m_pOutputs[i].size);
            }
            else
            {
                m_pOutputs[i].size = m_vOutputAttrs[i].n_elems * sizeof(float);
                m_pOutputs[i].is_prealloc = true;
                m_pOutputs[i].want_float = 1;
                m_pOutputs[i].buf = (float *)malloc(m_pOutputs[i].size);
                memset(m_pOutputs[i].buf, 0, m_pOutputs[i].size);
            }
        }
    }
    return true;
}

/* 能否推理的使用前判断 */
bool Inference_NS::CAVInferenceRK::inferenceInfe(int nTextSize, int nInputIndex)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (nTextSize <= 0)
    {
        printf("推理失败-传入的数据大小nTextSize[%d]小于0\n", nTextSize);
        return false;
    }

    int nDstSize = 1;
    for (int nIndex = 0; nIndex < m_vInputAttrs[nInputIndex].n_dims; nIndex++)
    {
        nDstSize *= m_vInputAttrs[nInputIndex].dims[nIndex];
    }
    if (nTextSize != nDstSize)
    {
        printf("模型[%s]需要的大小与输入图片大小不一致 nTextSize[%d] nDstSize[%d]\n",
               m_strModelPath.c_str(),
               nTextSize,
               nDstSize);
        return false;
    }
    return true;
}

