/*
 * @FilePath     : CVInferenceRK_V1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-31 14:58:38
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-02 16:32:48
 * @Description  :
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ModelOptV1_0.hpp"

namespace InferenceV1_0_NS
{
    class CCVInferenceRK : public CCVInferenceBase
    {
    public:

        CCVInferenceRK(std::string strModelPath);

        virtual ~CCVInferenceRK();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init() override;

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit() override;

        /**
         * @brief 获取输入图片限制
         * @param [int] nIndex: 第几个输入图片, 0开始
         * @param [int&] nWidth: 需要的图像宽度
         * @param [int&] nHeight: 需要的图像高度
         * @param [int&] nChannel: 需要的图像通道号
         * @return [*]
         * @note
         */
        bool getSizeLimit(int nIndex, int& nWidth, int& nHeight, int& nChannel) override;

        /**
         * @brief 初始化输入输出参数
         * @return [*]
         * @note
         */
        virtual bool initParams();

        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<void*>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        virtual bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData) = 0;

    protected:

        /**
         * @brief 推理的使用前判断
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @return [*]
         * @note
         */
        bool inferenceInfe(AiScenario_NS::CVData_S stInData);

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

        /* 模型处理数据限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;
    };

}    // namespace InferenceV1_0_NS
