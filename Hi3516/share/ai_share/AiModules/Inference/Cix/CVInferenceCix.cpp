/**
 * @file CVInferenceCix.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-11-06
 *
 * @brief
 */
#include "CVInferenceCix.hpp"

#include <cstring>
#include <fstream>

Inference_NS::CCVInferenceCix::CCVInferenceCix(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CCVInferenceCix::~CCVInferenceCix()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CCVInferenceCix::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (!checkModelConfig())
        {
            return false;
        };

        /* 创建模型操作类 */
        m_pModel = new CCixModelOpt(m_strModelPath);
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

            /* 从json文件中获取大小 */
            m_nLimitWidth = m_vModelInputSize[0];
            m_nLimitHeight = m_vModelInputSize[1];
            return initParams();
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CCVInferenceCix::unInit()
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
bool Inference_NS::CCVInferenceCix::checkModelConfig()
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
bool Inference_NS::CCVInferenceCix::checkModelPreConfig()
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
    int nSizeItem;
    double fMean, fStd;

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
bool Inference_NS::CCVInferenceCix::checkModelInferConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return false;
    }
    /* 读取文件内容到 std::string */
    std::string strJson;
    File.seekg(0, std::ios::end);
    size_t size = File.tellg();
    if (size > 0)
    {
        strJson.resize(size);
        File.seekg(0, std::ios::beg);
        File.read(&strJson[0], size);
    }
    File.close();
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    Json::Object *pItemObject = NULL;
    Json::Object *pJsonDataItem = NULL;
    Json::Object *pJsonDataItemObject = NULL;
    bool bRet = false;
    int nSize, nSizeO;
    int nStep;
    std::vector<int> vOutSizes;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "inference");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        goto EXIT;
    }

    /* 获取芯片号 */
    bRet = Json::get(pJsonData, "framework", strFramework);
    if (!bRet)
    {
        printf("解析framework字段失败\n");
        goto EXIT;
    }

    /* 模型的输出形状 */
    pJsonDataItem = Json::get(pJsonData, "output_shape");
    if (!pJsonDataItem)
    {
        printf("解析[output_shape]字段失败\n");
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[output_shape 数组大小异常]\n");
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
        vOutSizes.clear();
        for (int j = 0; j < nSizeO; j++)
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
                printf("解析[mean]字段失败\n");
                goto EXIT;
            }
            vOutSizes.push_back(nStep);
        }
        m_vOutSizes.push_back(vOutSizes);
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
bool Inference_NS::CCVInferenceCix::checkModelProConfig()
{
    return true;
}

/* 获取输入图片限制 */
bool Inference_NS::CCVInferenceCix::getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }
    if (nIndex != 0)
    {
        printf("只支持获取输入下标为0的大小\n");
        return false;
    }
    nWidth = m_nLimitWidth;
    nHeight = m_nLimitHeight;
    nChannel = m_nLimitChannel;

    return true;
}

/* 获取预处理的缩放大小和偏移量 */
bool Inference_NS::CCVInferenceCix::getScaleZeroPoint(int nIndex, float &fScale, int &nZeroPoint, noe_data_type_t &eDataType)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (nIndex >= m_vInputAttrs.size())
    {
        printf("nIndex[%d] 超出模型输入大小[%ld]\n", nIndex, m_vInputAttrs.size());
        return false;
    }

    fScale = m_vInputAttrs[nIndex].scale;
    nZeroPoint = m_vInputAttrs[nIndex].zero_point;
    eDataType = m_vInputAttrs[nIndex].data_type;

    return true;
}

/* 获取均值和方差 */
void Inference_NS::CCVInferenceCix::getMeanStd(std::vector<float> &vfMean, std::vector<float> &vfStd)
{
    vfMean = m_vMean;
    vfStd = m_vStd;
}

/* 初始化输入输出参数 */
bool Inference_NS::CCVInferenceCix::initParams()
{
    return true;
}

/* 能否推理的使用前判断 */
bool Inference_NS::CCVInferenceCix::inferenceInfe(int nIndex, int nImgSize)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    int nDstSize = m_vInputAttrs[nIndex].size;
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
