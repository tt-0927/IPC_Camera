/*
 * @FilePath     : CVInferenceRK_V1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-22 13:54:11
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-02 17:26:28
 * @Description  :
 */
#include "CVInferenceRK_V1_0.hpp"

#include "dlog.h"

InferenceV1_0_NS::CCVInferenceRK::CCVInferenceRK(std::string strModelPath)
    : m_strModelPath(strModelPath)
{
}

InferenceV1_0_NS::CCVInferenceRK::~CCVInferenceRK()
{
    unInit();
}

/* 初始化 */
bool InferenceV1_0_NS::CCVInferenceRK::init()
{
    if (!m_pModel)
    {
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
bool InferenceV1_0_NS::CCVInferenceRK::unInit()
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

/* 获取输入图片限制 */
bool InferenceV1_0_NS::CCVInferenceRK::getSizeLimit(int nIndex, int& nWidth, int& nHeight, int& nChannel)
{
    if (m_pModel &&
        m_vInputAttrs.size() > nIndex &&
        m_vInputAttrs[nIndex].n_dims > 3)
    {
        if (m_vInputAttrs[nIndex].fmt == RKNN_TENSOR_NCHW)
        {
            nChannel = m_vInputAttrs[nIndex].dims[1];
            nHeight  = m_vInputAttrs[nIndex].dims[2];
            nWidth   = m_vInputAttrs[nIndex].dims[3];
        }
        else
        {
            nHeight  = m_vInputAttrs[nIndex].dims[1];
            nWidth   = m_vInputAttrs[nIndex].dims[2];
            nChannel = m_vInputAttrs[nIndex].dims[3];
        }

        return true;
    }

    return false;
}

/* 初始化输入输出参数 */
bool InferenceV1_0_NS::CCVInferenceRK::initParams()
{
    if (m_pInputs)
    {
        /* 初始化每个 rknn_input 实例 */
        for (int i = 0; i < m_vInputAttrs.size(); ++i)
        {
            memset(&m_pInputs[i], 0, sizeof(rknn_input));

            int nTmpSize = 1;
            for (int j = 0; j < m_vInputAttrs[i].n_dims; j++)
            {
                nTmpSize *= m_vInputAttrs[i].dims[j];
            }

            if (i == 0 && m_vInputAttrs[0].n_dims > 3)
            {
                if (m_vInputAttrs[0].fmt == RKNN_TENSOR_NCHW)
                {
                    m_nLimitChannel = m_vInputAttrs[0].dims[1];
                    m_nLimitHeight  = m_vInputAttrs[0].dims[2];
                    m_nLimitWidth   = m_vInputAttrs[0].dims[3];
                }
                else
                {
                    m_nLimitHeight  = m_vInputAttrs[0].dims[1];
                    m_nLimitWidth   = m_vInputAttrs[0].dims[2];
                    m_nLimitChannel = m_vInputAttrs[0].dims[3];
                }
                dlog(LOG_TRACE, "输入图片限制 [%d]x[%d]x[%d]",
                     m_nLimitWidth,
                     m_nLimitHeight,
                     m_nLimitChannel);
            }

            m_pInputs[i].index = i;
            m_pInputs[i].type  = RKNN_TENSOR_UINT8; /* 根据需要设置数据类型 */
            m_pInputs[i].size  = nTmpSize;
            m_pInputs[i].fmt   = RKNN_TENSOR_NHWC;  /* 根据需要设置格式 */

            /* 设置为 1 时会将 buf 存放的输入数据直接设置给模型的输入节点，不做任何预处理。 */
            /* 注意，变换过程在 rknn api 内部自动处理 */
            m_pInputs[i].pass_through = 0;
        }
    }

    if (m_pOutputs)
    {
        /* 初始化每个 rknn_output 实例 */
        for (int i = 0; i < m_vOutputAttrs.size(); ++i)
        {
            memset(&m_pOutputs[i], 0, sizeof(rknn_output));
            /* uint8_t 标识是否需要将输出数据转为 float 类型输出 */
            m_pOutputs[i].want_float = 1;
        }
    }

    return true;
}

/* 能否推理的使用前判断 */
bool InferenceV1_0_NS::CCVInferenceRK::inferenceInfe(AiScenario_NS::CVData_S stInData)
{
    if (!m_pModel || !m_pInputs || !m_pOutputs)
    {
        dlog(LOG_ERROR, "推理失败-模型未初始化或者初始化失败");
        return false;
    }

    if (m_nLimitHeight <= 0 ||
        m_nLimitWidth <= 0 ||
        m_nLimitChannel <= 0)
    {
        dlog(LOG_ERROR, "推理失败-模型限制数据异常 [%d]x[%d]x[%d]",
             m_nLimitWidth,
             m_nLimitHeight,
             m_nLimitChannel);
        return false;
    }

    if (stInData.inMat.empty())
    {
        dlog(LOG_ERROR, "推理失败-传入的图片为空");
        return false;
    }


    /* 获取源图像的尺寸 */
    cv::Size srcSize = stInData.inMat.size();

    /* 设置目标图像的大小 */
    cv::Size dstSize(m_nLimitWidth, m_nLimitHeight);

    if (m_nLimitChannel != stInData.inMat.channels())
    {
        dlog(LOG_ERROR, "模型[%s]需要的数据的通道数未定义处理 [%d]",
             m_strModelPath.c_str(),
             m_nLimitChannel);
        return false;
    }

    if (srcSize != dstSize)
    {
        dlog(LOG_ERROR, "模型[%s]需要的大小与输入图片大小不一致 srcSize[%dx%d] dstSize[%dx%d]",
             m_strModelPath.c_str(),
             srcSize.width,
             srcSize.height,
             dstSize.width,
             dstSize.height);
        return false;
    }
    return true;
}
