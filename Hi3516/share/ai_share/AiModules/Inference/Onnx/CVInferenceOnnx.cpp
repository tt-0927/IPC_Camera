/**
 * @file CVInferenceOnnx.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#include "CVInferenceOnnx.hpp"

#include <cstring>
#include <fstream>

Inference_NS::CCVInferenceOnnx::CCVInferenceOnnx(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CCVInferenceOnnx::~CCVInferenceOnnx()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CCVInferenceOnnx::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (!checkModelConfig())
        {
            return false;
        };
        
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

            getSizeLimit(0, m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);
            return true;
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CCVInferenceOnnx::unInit()
{
    if (m_pModel)
    {
        delete m_pModel;
        m_pModel = nullptr;
    }
    /* 数据清空 */
    m_vInputAttrs.clear();
    m_vOutputAttrs.clear();
    m_nInputNum = 0;
    m_nOutputNum = 0;

    return true;
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CCVInferenceOnnx::checkModelConfig()
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
bool Inference_NS::CCVInferenceOnnx::checkModelPreConfig()
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
    int nSizeItem;
    double fMean, fStd;

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

        bRet = Json::Value::get(pItemObject, fMean);
        if (!bRet)
        {
            printf("解析[mean]字段失败\n");
            goto EXIT;
        }
        m_vMean.push_back(fMean);
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

        bRet = Json::Value::get(pItemObject, fStd);
        if (!bRet)
        {
            printf("解析[std]字段失败\n");
            goto EXIT;
        }
        m_vStd.push_back(fStd);
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
bool Inference_NS::CCVInferenceOnnx::checkModelInferConfig()
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
        bRet = false;
        goto EXIT;
    }

    /* 获取CPU推理的线程数 */
    bRet = Json::get(pJsonData, "intra_op_num_threads", m_nCpuInferThread);
    if (!bRet || m_nCpuInferThread < 0)
    {
        printf("解析intra_op_num_threads字段失败\n");
        goto EXIT;
    }

    /* 获取GPU设备号 */
    bRet = Json::get(pJsonData, "device_id", m_nDeviceId);
    if (!bRet | m_nDeviceId < 0)
    {
        printf("解析device_id字段失败\n");
        goto EXIT;
    }

    /* 获取模型限制最大显存 */
    bRet = Json::get(pJsonData, "gpu_mem_limit", m_nGpuMemLimit);
    if (!bRet | m_nGpuMemLimit < 0)
    {
        printf("解析gpu_mem_limit字段失败\n");
        goto EXIT;
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

/* 校验模型配置文件中的后处理信息 */
bool Inference_NS::CCVInferenceOnnx::checkModelProConfig()
{
    return true;
}

/* 获取配置文件的均值和方差 */ 
bool Inference_NS::CCVInferenceOnnx::getMeanStd(std::vector<float>& vMean, std::vector<float>& vStd)
{
    if (m_vMean.size() < 0 || m_vStd.size() < 0)
    {
        printf("config.json配置文件 未设置均值和方差\n");
        return false;
    }
    vMean = m_vMean;
    vStd = m_vStd;

    return true;
}

/* 获取输入图片限制 */
bool Inference_NS::CCVInferenceOnnx::getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel)
{
    if (m_pModel &&
        m_vInputAttrs.size() > nIndex &&
        m_vInputAttrs[nIndex].size() > 3)
    {
        if (m_vInputAttrs[nIndex][1] <= 4)
        {
            nChannel = m_vInputAttrs[nIndex][1];
            nHeight = m_vInputAttrs[nIndex][2];
            nWidth = m_vInputAttrs[nIndex][3];
        }
        else
        {
            nHeight = m_vInputAttrs[nIndex][1];
            nWidth = m_vInputAttrs[nIndex][2];
            nChannel = m_vInputAttrs[nIndex][3];
        }

        return true;
    }

    return false;
}

/* 能否推理的使用前判断 */
bool Inference_NS::CCVInferenceOnnx::inferenceInfe(int nImgSize)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (m_nLimitHeight <= 0 ||
        m_nLimitWidth <= 0 ||
        m_nLimitChannel <= 0)
    {
        printf("推理失败-模型限制数据异常 [%d]x[%d]x[%d]\n",
               m_nLimitWidth,
               m_nLimitHeight,
               m_nLimitChannel);
        return false;
    }

    if (nImgSize <= 0)
    {
        printf("推理失败-传入的数据大小nImgSize[%d]小于0\n", nImgSize);
        return false;
    }

    int nDstSize = m_nLimitHeight * m_nLimitWidth * m_nLimitChannel * sizeof(float);

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
