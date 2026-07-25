/*
 * @FilePath     : CAInferenceRK.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-06-05 14:52:54
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-05 15:12:03
 * @Description  :
 */

#pragma once

#include "CAInferenceBase.hpp"
#include "ModelOptV1_0.hpp"

namespace InferenceV1_0_NS
{
    class CCAInferenceRK : public CCAInferenceBase
    {
    public:

        CCAInferenceRK(std::string strModelPath)
            : m_strModelPath(strModelPath)
        {
        }

        virtual ~CCAInferenceRK()
        {
            unInit();
        }

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init() override
        {
            if (!m_pModel)
            {
                /* 创建模型操作类 */
                m_pModel = new CModelOpt(m_strModelPath);
                if (m_pModel)
                {
                    m_pModel->init();

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

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit() override
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

        /**
         * @brief 获取输入图片限制
         * @param [int] nIndex: 第几个输入图片, 0开始
         * @param [int&] nWidth: 需要的图像宽度
         * @param [int&] nHeight: 需要的图像高度
         * @param [int&] nChannel: 需要的图像通道号
         * @return [*]
         * @note
         */
        bool getSizeLimit(int nIndex, int& nWidth, int& nHeight, int& nChannel) override
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

        /**
         * @brief 初始化输入输出参数
         * @return [*]
         * @note
         */
        virtual bool initParams() = 0;

        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<void*>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        virtual bool inference(AiScenario_NS::CVData_S stInData, std::vector<void*>& vOutData) = 0;

    protected:

        std::string  m_strModelPath;         /* 模型路径 */
        CModelOpt*   m_pModel     = nullptr; /* 模型操作句柄 */
        rknn_input*  m_pInputs    = nullptr; /* 模型输入参数 */
        int          m_nInputNum  = 0;       /* 模型输入参数数量 */
        rknn_output* m_pOutputs   = nullptr; /* 模型输出参数 */
        int          m_nOutputNum = 0;       /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<rknn_tensor_attr> m_vInputAttrs;
        std::vector<rknn_tensor_attr> m_vOutputAttrs;
    };

}    // namespace InferenceV1_0_NS
