/*
 * @FilePath     : ModelOptV1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 11:21:29
 * @Description  : RK模型操作
 */
#include "ModelOptV1_0.hpp"

#include <cstring>
#include <fstream>

#include "dlog.h"

InferenceV1_0_NS::CModelOpt::CModelOpt(std::string strModelPath)
    : m_strModelPath(strModelPath)
{
}

InferenceV1_0_NS::CModelOpt::~CModelOpt()
{
    unInit();
}

/* 设置模型路径 */
void InferenceV1_0_NS::CModelOpt::setModelPath(std::string strModelPath)
{
    m_strModelPath = strModelPath;
}

/* 初始化模型 */
bool InferenceV1_0_NS::CModelOpt::init()
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
        dlog(LOG_ERROR, "载入模型，并转为二进制格式失败");
        bRet = false;
        goto EXIT;
    }

    /* 初始化句柄 */
    nRet = rknn_init(&m_hCtx, pModelData, nModelDataSize, 0, NULL);
    if (nRet != RKNN_SUCC)
    {
        dlog(LOG_ERROR, "rknn 初始化失败 nRet=[%d] nModelDataSize=[%d]", nRet, nModelDataSize);
        bRet = false;
        goto EXIT;
    }
#else
    /* 初始化句柄 */
    nRet = rknn_init(&m_hCtx, const_cast<char*>(m_strModelPath.data()), 0, 0, NULL);
    if (nRet != RKNN_SUCC)
    {
        dlog(LOG_ERROR, "rknn 初始化失败 nRet=[%d]", nRet);
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
        dlog(LOG_ERROR, "获取RKNN版本信息失败 [%d]", nRet);
        bRet = false;
        goto EXIT;
    }

    /* SDK 的版本信息。SDK 所基于的驱动版本信息 */
    dlog(LOG_TRACE, "RKNN sdk version: [%s]  driver version: [%s]",
         version.api_version,
         version.drv_version);

    /* 获取模型的输入和输出的数量 */
    nRet = rknn_query(m_hCtx, RKNN_QUERY_IN_OUT_NUM, &m_stIoNumInfo, sizeof(m_stIoNumInfo));
    if (nRet != RKNN_SUCC)
    {
        dlog(LOG_ERROR, "获取模型的输入和输出的数量失败 [%d]", nRet);
        bRet = false;
        goto EXIT;
    }

    dlog(LOG_TRACE, "模型的输入数量[%d], 输出的数量[%d]",
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
            dlog(LOG_ERROR, "获取输入tensor的属性信息-失败[%d]", nRet);
            bRet = false;
            goto EXIT;
        }

        m_vInputAttrs.push_back(stInputAttrs);
    }

    /* 获取输入tensor的属性信息 */
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
            dlog(LOG_ERROR, "获取输出tensor的属性信息-失败[%d]", nRet);
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
bool InferenceV1_0_NS::CModelOpt::unInit()
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
bool InferenceV1_0_NS::CModelOpt::getInputAttrs(std::vector<rknn_tensor_attr>& vInputAttrs)
{
    if (m_bInitialized)
    {
        vInputAttrs = m_vInputAttrs;
        return true;
    }
    return false;
}

/* 获取模型输出参数 */
bool InferenceV1_0_NS::CModelOpt::getOutputAttrs(std::vector<rknn_tensor_attr>& vOutputAttrs)
{
    if (m_bInitialized)
    {
        vOutputAttrs = m_vOutputAttrs;
        return true;
    }
    return false;
}

/* 释放输出参数 */
bool InferenceV1_0_NS::CModelOpt::releaseOutputs(rknn_output*& pOutputs, int nNum)
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
bool InferenceV1_0_NS::CModelOpt::run(
    rknn_input*   pInputs,
    int           nInNum,
    rknn_output*& pOutputs,
    int           nOutNum)
{
    int nRet = 0;

    if (m_bInitialized &&
        nInNum == m_stIoNumInfo.n_input &&
        nOutNum == m_stIoNumInfo.n_output)
    {
        /* 设置输入数据 */
        nRet = rknn_inputs_set(m_hCtx, nInNum, pInputs);
        if (nRet != RKNN_SUCC)
        {
            return false;
        }

        /* 运行 */
        nRet = rknn_run(m_hCtx, nullptr);
        if (nRet != RKNN_SUCC)
        {
            return false;
        }

        /* 获取输出数据 */
        nRet = rknn_outputs_get(m_hCtx, nOutNum, pOutputs, nullptr);
        if (nRet != RKNN_SUCC)
        {
            return false;
        }

        return true;
    }

    return false;
}

/* 载入模型 */
unsigned char* InferenceV1_0_NS::CModelOpt::loadModel(std::string strFileName, int& nModelSize)
{
    int nRet = 0;

    FILE*          pFp   = nullptr;
    unsigned char* pData = nullptr;
    int            nSize = 0;

    pFp = fopen(strFileName.c_str(), "rb");
    if (nullptr == pFp)
    {
        dlog(LOG_ERROR, "打开模型失败 [%s]", strFileName.c_str());
        goto EXIT;
    }

    fseek(pFp, 0, SEEK_END);
    nSize = ftell(pFp);

    nRet = fseek(pFp, 0, SEEK_SET);
    if (nRet != 0)
    {
        dlog(LOG_ERROR, "将文件句柄设置到文件头失败");
        goto EXIT;
    }

    pData = (unsigned char*)malloc(nSize);
    if (pData == nullptr)
    {
        dlog(LOG_ERROR, "创建空间失败");
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
        dlog(LOG_ERROR, "读取失败");
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
