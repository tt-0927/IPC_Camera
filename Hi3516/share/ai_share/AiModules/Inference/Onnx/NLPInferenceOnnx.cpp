/**
 * @file NLPInferenceRK.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-13
 * 
 * @brief 
 */
#include "NLPInferenceOnnx.hpp"

#include <cstring>

Inference_NS::CNLPInferenceOnnx::CNLPInferenceOnnx(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CNLPInferenceOnnx::~CNLPInferenceOnnx()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CNLPInferenceOnnx::init()
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
        m_pModel = new COnnxModelOpt(m_strModelPath);
        if (m_pModel)
        {
            if (!checkModelInferConfig())
            {
                printf("json配置文件[%s], 推理部分解析异常\n", m_strConfigPath.c_str());
                return false;
            }

            if (!m_pModel->init())
            {
                return false;
            }

            /* 初始化值 */
            m_pModel->getInputAttrs(m_vInputAttrs);
            m_pModel->getOutputAttrs(m_vOutputAttrs);

            m_nInputNum = m_vInputAttrs.size();
            m_nOutputNum = m_vOutputAttrs.size();

            getSizeLimit(0, m_vModelSize);
            return true;
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CNLPInferenceOnnx::unInit()
{
    if (m_pModel)
    {
        delete m_pModel;
        m_pModel = nullptr;
    }
    if(m_pTokenize)
    {
        delete m_pTokenize;
    }

    /* 数据清空 */
    m_vInputAttrs.clear();
    m_vOutputAttrs.clear();
    m_nInputNum = 0;
    m_nOutputNum = 0;

    return true;
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CNLPInferenceOnnx::checkModelConfig()
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

    /* 获取分词文本地址 */
    bRet = Json::get(pJsonHandle, "vocab_path", m_strVocabPath);
    if (!bRet)
    {
        printf("解析model_path字段失败\n");
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
bool Inference_NS::CNLPInferenceOnnx::checkModelInferConfig()
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

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "inference");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        bRet = false;
        goto EXIT;
    }

    /* 获取芯片号 */
    bRet = Json::get(pJsonData, "framework", strFramework);
    if (!bRet)
    {
        printf("解析framework字段失败\n");
        goto EXIT;
    }

    /* 获取CPU推理的线程数 */
    bRet = Json::get(pJsonData, "intra_op_num_threads", m_nCpuInferThread);
    if (!bRet || m_nCpuInferThread < 0)
    {
        printf("解析intra_op_num_threads字段失败\n");
        // goto EXIT;
    }

    /* 获取GPU设备号 */
    bRet = Json::get(pJsonData, "device_id", m_nDeviceId);
    if (!bRet | m_nDeviceId < 0)
    {
        printf("解析device_id字段失败\n");
        // goto EXIT;
    }

    /* 获取模型限制最大显存 */
    bRet = Json::get(pJsonData, "gpu_mem_limit", m_nGpuMemLimit);
    if (!bRet | m_nGpuMemLimit < 0)
    {
        printf("解析gpu_mem_limit字段失败\n");
        // goto EXIT;
    }

    /* 传送相关的CUDA信息到模型初始化 */
    m_pModel->setInferData(
        m_nCpuInferThread,
        m_nDeviceId,
        m_nGpuMemLimit
    );

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 获取输入文本限制 */
bool Inference_NS::CNLPInferenceOnnx::getSizeLimit(int nIndex, std::vector<int> &vModelSize)
{
    if (m_pModel &&
        m_vInputAttrs.size() > nIndex)
    {
        for (int i = 0; i < m_vInputAttrs[nIndex].size(); i++)
        {
            vModelSize.push_back(m_vInputAttrs[nIndex][i]);
        }
        return true;
    }

    return false;
}


/* 能否推理的使用前判断 */
bool Inference_NS::CNLPInferenceOnnx::inferenceInfe(int nTextSize, int nInputIndex)
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
    for (int nIndex = 0; nIndex < m_vInputAttrs[nInputIndex].size(); nIndex++)
    {
        nDstSize *= m_vInputAttrs[nInputIndex][nIndex];
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
