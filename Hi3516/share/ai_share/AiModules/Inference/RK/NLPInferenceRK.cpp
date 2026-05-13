/**
 * @file NLPInferenceRK.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-13
 * 
 * @brief 
 */
#include "NLPInferenceRK.hpp"

#include <cstring>

Inference_NS::CNLPInferenceRK::CNLPInferenceRK(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CNLPInferenceRK::~CNLPInferenceRK()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CNLPInferenceRK::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (!checkModelConfig())
        {
            return false;
        };

        // m_pTokenize = new CLIm_pTokenizer(); /* 英文接口 */ 
        m_pTokenize = new TokenizerClipChinese(m_strVocabPath);
        if(! m_pTokenize)
        {
            printf("分词函数TokenizerClipChinese初始化失败\n");
            return false;
        }

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
                m_pInputs = new rknn_tensor_mem*[m_nInputNum];
            }

            /* 初始化输出参数 */
            if (m_nOutputNum > 0)
            {
                m_pOutputs = new rknn_tensor_mem*[m_nOutputNum];
            }

            /* 初始化参数 */
            return initParams();
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CNLPInferenceRK::unInit()
{
    if (m_pInputs)
    {
        for (int i = 0; i < m_vInputAttrs.size(); i++)
        {
            rknn_destroy_mem(m_pModel->m_hCtx, m_pInputs[i]);
        }
        m_vInputAttrs.clear();
        m_pInputs   = nullptr;
        m_nInputNum = 0;
    }

    if (m_pOutputs)
    {
        for (int i = 0; i < m_vOutputAttrs.size(); i++)
        {
            rknn_destroy_mem(m_pModel->m_hCtx, m_pOutputs[i]);
        }
        m_vOutputAttrs.clear();
        m_pOutputs   = nullptr;
        m_nOutputNum = 0;
    }

    if (m_pModel)
    {
        delete m_pModel;
        m_pModel = nullptr;
    }
    if(m_pTokenize)
    {
        delete m_pTokenize;
    }

    return true;
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CNLPInferenceRK::checkModelConfig()
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

    /* 获取分词文本地址 */
    bRet = Json::get(pJsonHandle, "vocab_path", m_strVocabPath);
    if (!bRet)
    {
        printf("解析model_path字段失败\n");
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

/* 获取输入图片限制 */
bool Inference_NS::CNLPInferenceRK::getSizeLimit(int nIndex, std::vector<int> &vModelSize)
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

/* 初始化输入输出参数 */
bool Inference_NS::CNLPInferenceRK::initParams()
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

            /* 输出数据的相关配置 */
            /* 如果设置为 uint8，将在 NPU 中进行归一化和量化处理 */
            m_vInputAttrs[i].type = RKNN_TENSOR_INT32;
            /*  默认格式为 NHWC，NPU 仅支持在零拷贝模式下使用 NHWC 格式 */
            m_vInputAttrs[i].fmt = RKNN_TENSOR_UNDEFINED;
		    /* 申请输入数据内存 */
            m_pInputs[i] = rknn_create_mem(m_pModel->m_hCtx, m_vInputAttrs[i].size_with_stride);
            /* 调用rknn_set_io_mem 让NPU使用上面申请到的内存 */
            rknn_set_io_mem(m_pModel->m_hCtx, m_pInputs[i], &m_vInputAttrs[i]);
        }
    }

    if (m_pOutputs)
    {
        /* 初始化每个 rknn_output 实例 */
        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            /* 输出数据的相关配置 */
            m_vOutputAttrs[i].type = RKNN_TENSOR_FLOAT32;
            /* 申请输入数据内存 */
            m_pOutputs[i] = rknn_create_mem(m_pModel->m_hCtx, m_vOutputAttrs[i].n_elems*sizeof(float));
            /* 调用rknn_set_io_mem 让NPU使用上面申请到的内存 */
            rknn_set_io_mem(m_pModel->m_hCtx, m_pOutputs[i], &m_vOutputAttrs[i]);
        }
    }

    return true;
}

/* 能否推理的使用前判断 */
bool Inference_NS::CNLPInferenceRK::inferenceInfe(int nTextSize, int nInputIndex)
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
