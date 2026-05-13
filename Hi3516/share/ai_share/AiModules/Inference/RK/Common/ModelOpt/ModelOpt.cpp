/*
 * @FilePath     : ModelOpt.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 20:06:21
 * @Description  : RK模型操作
 */
#include "ModelOpt.hpp"

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
    int  nRet = 0;

    int            nModelDataSize = 0;
    unsigned char* pModelData     = nullptr;

    rknn_tensor_attr stInputAttrs;
    rknn_tensor_attr stOutputAttrs;

#if 0
    /* 载入模型，并转为二进制格式 */
    pModelData = loadModel(m_strModelPath, nModelDataSize);
    if (pModelData == nullptr)
    {
        printf("载入模型，并转为二进制格式失败\n");
        bRet = false;
        goto EXIT;
    }

    /* 初始化句柄 */
    nRet = rknn_init(&m_hCtx, pModelData, nModelDataSize, 0, NULL);
    if (nRet != RKNN_SUCC)
    {
        printf("rknn 初始化失败 nRet=[%d] nModelDataSize=[%d]\n", nRet, nModelDataSize);
        bRet = false;
        goto EXIT;
    }
#else
    /* 初始化句柄 */
    nRet = rknn_init(&m_hCtx, const_cast<char*>(m_strModelPath.data()), 0, 0, NULL);
    if (nRet != RKNN_SUCC)
    {
        printf("rknn 初始化失败 nRet=[%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }
#endif

    /* 初始化成功 */
    m_bInitialized = true;

    /* 获取版本号 */
    rknn_sdk_version version;
    nRet = rknn_query(m_hCtx, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
    if (nRet != RKNN_SUCC)
    {
        printf("获取RKNN版本信息失败 [%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }

    /* SDK 的版本信息。SDK 所基于的驱动版本信息 */
    printf("RKNN sdk version: [%s]  driver version: [%s]\n",
           version.api_version,
           version.drv_version);

    /* 获取模型的输入和输出的数量 */
    nRet = rknn_query(m_hCtx, RKNN_QUERY_IN_OUT_NUM, &m_stIoNumInfo, sizeof(m_stIoNumInfo));
    if (nRet != RKNN_SUCC)
    {
        printf("获取模型的输入和输出的数量失败 [%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }

    printf("模型的输入数量[%d], 输出的数量[%d]\n",
           m_stIoNumInfo.n_input,
           m_stIoNumInfo.n_output);

    /* 获取输入tensor的属性信息 */
    m_vInputAttrs.clear();
    for (int i = 0; i < m_stIoNumInfo.n_input; i++)
    {
        memset(&stInputAttrs, 0, sizeof(stInputAttrs));

        /* 需要在调用rknn_query之前设置。 */
        stInputAttrs.index = i;

        nRet = rknn_query(m_hCtx,
                          RKNN_QUERY_INPUT_ATTR,
                          &stInputAttrs,
                          sizeof(rknn_tensor_attr));
        if (nRet != RKNN_SUCC)
        {
            printf("获取输入tensor的属性信息-失败[%d]\n", nRet);
            bRet = false;
            goto EXIT;
        }

        m_vInputAttrs.push_back(stInputAttrs);
    }

    /* 获取输出tensor的属性信息 */
    m_vOutputAttrs.clear();
    for (int i = 0; i < m_stIoNumInfo.n_output; i++)
    {
        memset(&stOutputAttrs, 0, sizeof(stOutputAttrs));

        /* 需要在调用rknn_query之前设置。 */
        stOutputAttrs.index = i;

        nRet = rknn_query(m_hCtx,
                          RKNN_QUERY_OUTPUT_ATTR,
                          &stOutputAttrs,
                          sizeof(rknn_tensor_attr));
        if (nRet != RKNN_SUCC)
        {
            printf("获取输出tensor的属性信息-失败[%d]\n", nRet);
            bRet = false;
            goto EXIT;
        }

        m_vOutputAttrs.push_back(stOutputAttrs);
    }

    bRet = true;
EXIT:

    if (pModelData)
    {
        free(pModelData);
        pModelData = nullptr;
    }

    if (!bRet)
    {
        if (m_bInitialized)
        {
            rknn_destroy(m_hCtx);
            m_bInitialized = false;
        }
    }

    return bRet;
}

/* 反初始化模型 */
bool Inference_NS::CModelOpt::unInit()
{
    if (m_bInitialized)
    {
        rknn_destroy(m_hCtx);
        m_bInitialized = false;
        return true;
    }

    return false;
}

/* 获取模型输入参数 */
bool Inference_NS::CModelOpt::getInputAttrs(std::vector<rknn_tensor_attr>& vInputAttrs)
{
    if (m_bInitialized)
    {
        vInputAttrs = m_vInputAttrs;
        return true;
    }
    return false;
}

/* 获取模型输出参数 */
bool Inference_NS::CModelOpt::getOutputAttrs(std::vector<rknn_tensor_attr>& vOutputAttrs)
{
    if (m_bInitialized)
    {
        vOutputAttrs = m_vOutputAttrs;
        return true;
    }
    return false;
}

/* 释放输出参数 */
bool Inference_NS::CModelOpt::releaseOutputs(rknn_output*& pOutputs, int nNum)
{
    if (m_bInitialized)
    {
        int nRet = 0;
        nRet     = rknn_outputs_release(m_hCtx, nNum, pOutputs);
        if (nRet != RKNN_SUCC)
        {
            return false;
        }
        return true;
    }
    return false;
}


/* 运行模型 */
bool Inference_NS::CModelOpt::run()
{
    int nRet = 0;

    if (m_bInitialized)
    {
        /* 运行 */
        nRet = rknn_run(m_hCtx, nullptr);
        if (nRet != RKNN_SUCC)
        {
            return false;
        }
        return true;
    }

    return false;
}

/* 载入模型 */
unsigned char* Inference_NS::CModelOpt::loadModel(std::string strFileName, int& nModelSize)
{
    int nRet = 0;

    FILE*          pFp   = nullptr;
    unsigned char* pData = nullptr;
    int            nSize = 0;

    pFp = fopen(strFileName.c_str(), "rb");
    if (nullptr == pFp)
    {
        printf("打开模型失败 [%s]\n", strFileName.c_str());
        goto EXIT;
    }

    fseek(pFp, 0, SEEK_END);
    nSize = ftell(pFp);

    nRet = fseek(pFp, 0, SEEK_SET);
    if (nRet != 0)
    {
        printf("将文件句柄设置到文件头失败\n");
        goto EXIT;
    }

    pData = (unsigned char*)malloc(nSize);
    if (pData == nullptr)
    {
        printf("创建空间失败\n");
        goto EXIT;
    }

    nRet = fread(pData, 1, nSize, pFp);
    if (nRet != nSize)
    {
        if (pData)
        {
            free(pData);
            pData = nullptr;
        }
        printf("读取失败\n");
        goto EXIT;
    }

EXIT:
    if (pFp)
    {
        fclose(pFp);
        pFp = nullptr;
    }

    nModelSize = nSize;

    return pData;
}
