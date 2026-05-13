/*
 * @FilePath     : CVInferenceHISI.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-22 13:54:11
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 19:44:39
 * @Description  :
 */
#include "CVInferenceHISI.hpp"
#include <algorithm>
#include <cstring>

Inference_NS::CCVInferenceHISI::CCVInferenceHISI(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CCVInferenceHISI::~CCVInferenceHISI()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CCVInferenceHISI::init()
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
            /* 获取输入头和输出头个数 */
            m_pModel->getInputSize(m_nInputNum);
            m_pModel->getOutputSize(m_nOutputNum);
            /* 初始化输入参数 */
            if (m_nInputNum > 0)
            {
                m_pInputs = svp_acl_mdl_create_dataset();
            }
            /* 初始化输出参数 */
            if (m_nOutputNum > 0)
            {
                m_pOutputs = svp_acl_mdl_create_dataset();
            }
            /* 初始化参数 */
            return initParams();
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CCVInferenceHISI::unInit()
{
    if (m_pInputs)
    {
        m_pModel->DestroyInput(m_pInputs);

        m_nInputNum = 0;

        m_vInputDevSize.clear();
        m_vInputStride.clear();
        m_vInputDims.clear();
        m_veviceBuffers.clear();
    }

    if (m_pOutputs)
    {
        m_pModel->DestroyOutput(m_pOutputs);

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
bool Inference_NS::CCVInferenceHISI::checkModelConfig()
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
bool Inference_NS::CCVInferenceHISI::checkModelPreConfig()
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
bool Inference_NS::CCVInferenceHISI::checkModelInferConfig()
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
bool Inference_NS::CCVInferenceHISI::checkModelProConfig()
{
    return true;
}

/* 获取输入图片限制 */
bool Inference_NS::CCVInferenceHISI::getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel)
{
    if (m_pModel &&
        m_nInputNum > nIndex &&
        m_vInputDims[nIndex].dim_count > 3)
    {
        nChannel = m_vInputDims[nIndex].dims[1];
        nHeight = m_vInputDims[nIndex].dims[2];
        nWidth = m_vInputDims[nIndex].dims[3];

        return true;
    }

    return false;
}

/* 初始化输入输出参数 */
bool Inference_NS::CCVInferenceHISI::initParams()
{   
    if (m_pInputs)
    {   
        /* 初始化每个 输入 实例 */
        for (int i = 0; i < m_nInputNum; ++i)
        {
            size_t inputDevSize = 0; 
            size_t inputStride = 0;  
            size_t inputDataSize = 0;   
            svp_acl_mdl_io_dims inputDims; 
            void* deviceBuffer = nullptr;
            int64_t loopTimes = 1;
            size_t totalSize = 0;

            m_pModel->getInputAttrs(i, inputDevSize, inputStride, inputDims, inputDataSize);

            if (i == 0 && inputDims.dim_count > 3)
            {
                int64_t dimValue = inputDims.dims[inputDims.dim_count - 1];
                m_lineSize = dimValue * inputDataSize;

                // printf("第 [%d] 个输入数据的每行有效数据大小 [%zu]！ \n", i, m_lineSize);
                // printf("dimValue: [%zu], inputDataSize: [%zu]！ \n", dimValue, inputDataSize);

                for (size_t loop = 0; loop < inputDims.dim_count - 1; loop++) {
                    m_loopTimes *= inputDims.dims[loop];  // 维度前 dim_count - 1 项相乘，计算循环次数（例如 N×C×H）
                }

                m_nLimitChannel = inputDims.dims[1];
                m_nLimitHeight = inputDims.dims[2];
                m_nLimitWidth = inputDims.dims[3];

                printf("输入图片限制 [%d]x[%d]x[%d]\n",
                       m_nLimitWidth,
                       m_nLimitHeight,
                       m_nLimitChannel);
            }

            svp_acl_error ret = svp_acl_rt_malloc(&deviceBuffer, inputDevSize, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
            if (ret != SVP_ACL_SUCCESS) {
                return false;
            }
            memset(deviceBuffer, 0, inputDevSize);

            svp_acl_data_buffer* inputData = svp_acl_create_data_buffer(deviceBuffer, inputDevSize, inputStride);
            if (inputData == nullptr) {
                printf("创建第 [%d] 个输入数据的缓冲区失败！ \n", i);
            }
            ret = svp_acl_mdl_add_dataset_buffer(m_pInputs, inputData);
            if (ret != SVP_ACL_SUCCESS) {
                printf("将第 [%d] 个输入数据放到缓冲区失败！ \n", i);
                /* 释放数据缓冲区 */
                svp_acl_destroy_data_buffer(inputData);
                inputData = nullptr;
            }
            
            m_vInputDevSize.push_back(inputDevSize);
            m_vInputStride.push_back(inputStride);
            m_vInputDims.push_back(inputDims);
            m_veviceBuffers.push_back(deviceBuffer);
            m_InputData.push_back(inputData);
        }
    }
    
    if (m_pOutputs)
    {
        for (int i = 0; i < m_nOutputNum; ++i)
        {
            svp_acl_error ret;
            size_t outStride = 0;
            size_t outputDevSize = 0;
            void *outBuf  = nullptr;    /* 定义指向输出设备内存的指针 */
            svp_acl_mdl_io_dims outputDims;
            std::vector<int> vDimsVec;
            
            m_pModel->getOutputAttrs(i, outStride, outputDevSize, outputDims);

            for (int j = 0; j < outputDims.dim_count; ++j)
            {
                vDimsVec.push_back(outputDims.dims[j]);
            }

            m_vOutputDims.push_back(vDimsVec);

            printf("第 %d 个输出头的大小 [%d]x[%d]x[%d]x[%d]\n",
                       i,
                       m_vOutputDims[i][0],
                       m_vOutputDims[i][1],
                       m_vOutputDims[i][2],
                       m_vOutputDims[i][3]);

            /* 在 NPU 的设备内存中分配 buffer，用于保存模型推理结果 */
            ret = svp_acl_rt_malloc(&outBuf , outputDevSize, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
            if (ret != SVP_ACL_SUCCESS) {
                printf("分配输出缓冲失败 i=[%zu] size=[%zu]\n", i, outputDevSize);
                return false;
            }
            memset(outBuf, 0, outputDevSize);
            /* 创建用于管理该输出内存的 DataBuffer 对象（包含指针和大小信息） */
            svp_acl_data_buffer* outData  = svp_acl_create_data_buffer(outBuf , outputDevSize, outStride);
            if (!outData ) {
                printf("创建数据缓冲区失败。 \n");
                return false;
            }
            /* 将这个输出 buffer 添加到模型的输出数据集中 */
            ret = svp_acl_mdl_add_dataset_buffer(m_pOutputs, outData);
            if (ret != SVP_ACL_SUCCESS) {
                svp_acl_destroy_data_buffer(outData);
                printf("添加输出缓冲区到数据集失败 \n");
                return false;
            }
            m_vOutputBuffers.push_back(outData);
        }
    }
    return true;
}

/* 定义一个数据输入存储函数 */
void Inference_NS::CCVInferenceHISI::setInputDatas(unsigned char *pDataBuffer, int nInputIndex)
{
    /* 按行将数据从 pDataBuffer 拷贝到 device 内存，考虑 stride 对齐 */
    uint8_t *pSrcPtr = pDataBuffer;
    uint8_t *pDstPtr = static_cast<uint8_t*>(m_veviceBuffers[nInputIndex]);
    if (strType == "YVU420SP")
    {
        // 1. 拷贝 Y 分量
        for (int i = 0; i < m_nLimitHeight; ++i)
        {
            memcpy(pDstPtr, pSrcPtr, m_lineSize);
            pSrcPtr += m_lineSize;
            pDstPtr += m_vInputStride[nInputIndex];
        }
        // 2. 拷贝 VU 分量
        for (int i = 0, limit = m_nLimitHeight / 2; i < limit; ++i)
        {
            memcpy(pDstPtr, pSrcPtr, m_lineSize);
            pSrcPtr += m_lineSize;
            pDstPtr += m_vInputStride[nInputIndex];
        }
    } else
    {
        for (int64_t i = 0; i < m_loopTimes; ++i)
        {
            memcpy(pDstPtr, pSrcPtr, m_lineSize);
            pSrcPtr += m_lineSize;
            pDstPtr += m_vInputStride[nInputIndex];
        }
    }

    svp_acl_error ret = svp_acl_update_data_buffer(m_InputData[nInputIndex], 
                                                   m_veviceBuffers[nInputIndex], 
                                                   m_vInputDevSize[nInputIndex], 
                                                   m_vInputStride[nInputIndex]
                                                   );
    if (ret != SVP_ACL_SUCCESS) {
        printf("添加数据失败！\n");
    }

}

bool Inference_NS::CCVInferenceHISI::setYolov8PostProcessParameters(float confThresh, float nmsThresh, int nInputIndex)
{
    std::vector<float> detPara = {nmsThresh, confThresh, 1.0f, 1.0f};

    if (m_vInputDevSize[nInputIndex] < detPara.size() * sizeof(float)) {
        printf("Input device buffer too small: expected >= %zu bytes, but got %zu",
                detPara.size() * sizeof(float), m_vInputDevSize[nInputIndex]);
        
        return false;
    }

    float* hostBuf = const_cast<float*>(detPara.data());
    memcpy(m_veviceBuffers[nInputIndex], hostBuf, m_vInputDevSize[nInputIndex]);

    svp_acl_error ret = svp_acl_update_data_buffer(
        m_InputData[nInputIndex],
        m_veviceBuffers[nInputIndex],
        m_vInputDevSize[nInputIndex],
        m_vInputStride[nInputIndex]
    );
    if (ret != SVP_ACL_SUCCESS) {
        printf("svp_acl_update_data_buffer for detParas failed, code=%d", ret);
        return false;
    }
    return true;
}

/* 能否推理的使用前判断 */
bool Inference_NS::CCVInferenceHISI::inferenceInfe(int nImgSize, int nInputIndex)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (nImgSize <= 0)
    {
        printf("推理失败-传入的数据大小nImgSize [%d] 小于0\n", nImgSize);
        return false;
    }

    int nDstSize = m_vInputDevSize[nInputIndex];
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
