/*
 * @FilePath     : CVInferenceRK.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-23 15:51:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 09:43:52
 * @Description  :
 */
#pragma once

#include <fstream>
#include "ModelOpt.hpp"
#ifndef SHAREJSON_ENABLE//公共库的json文件
#include "JsonInterfase.h"
#else
#include "Json.h"
#endif

namespace Inference_NS
{
    class CCVInferenceRK
    {
    public:

        CCVInferenceRK(std::string strConfigPath);

        ~CCVInferenceRK();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 获取输入图片限制
         * @param [int] nIndex: 第几个输入图片, 0开始
         * @param [int&] nWidth: 需要的图像宽度
         * @param [int&] nHeight: 需要的图像高度
         * @param [int&] nChannel: 需要的图像通道号
         * @return [*]
         * @note
         */
        bool getSizeLimit(int nIndex, int& nWidth, int& nHeight, int& nChannel);

        /**
         * @brief 初始化输入输出参数
         * @return [*]
         * @note
         */
        bool initParams();

        /**
         * @brief 将数据传入推理模型
         * @param [unsigned char*] pDataBuffer: 输入图片数据
         * @param [int] nInputIndex: 第几个模型输入
         * @note
         */
        void setInputDatas(unsigned char* pDataBuffer, int nInputIndex);

        /**
         * @brief 推理的使用前判断
         * @param [int] nImgSize: 传入的图片数据
         * @param [int] nInputIndex: 神经网络输入的下标，默认为0
         * @return [*]
         * @note
         */
        bool inferenceInfe(int nImgSize, int nInputIndex = 0);

        /**
         * @brief 校验模型配置文件的公共信息
         * @return [*]
         */
        bool checkModelConfig();

        /**
         * @brief 校验模型配置文件中的预处理信息
         * @return [*]
         */
        virtual bool checkModelPreConfig();

        /**
         * @brief 校验模型配置文件中的模型推理信息
         * @return [*]
         */
        virtual bool checkModelInferConfig();
        
        /**
         * @brief 校验模型配置文件中的后处理信息
         * @return [*]
         */
        virtual bool checkModelProConfig();

    protected:

        std::string  m_strConfigPath;        /* 模型json配置路径 */
        std::string  m_strModelPath;         /* 模型路径 */
        CModelOpt*   m_pModel     = nullptr; /* 模型操作句柄 */
        rknn_tensor_mem**  m_pInputs    = nullptr; /* 模型输入参数 */
        int          m_nInputNum  = 0;       /* 模型输入参数数量 */
        rknn_tensor_mem** m_pOutputs   = nullptr; /* 模型输出参数 */
        int          m_nOutputNum = 0;       /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<rknn_tensor_attr> m_vInputAttrs;
        std::vector<rknn_tensor_attr> m_vOutputAttrs;

        /* 模型处理数据限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;

        /* 模型预测处理相关的信息 */
        int m_nChannel;             /* 模型输入的通道数 */
        std::string strType;        /* 模型需要输入的数据类型 ["rgb", "bgr", ...]等 */
        std::vector<int> m_vModelInputSize; /* 模型的输入形状 */
        std::vector<int> m_vMean;   /* 图片归一化的均值 */
        std::vector<int> m_vStd;    /* 图片归一化的方差 */ 
        int m_nPadding = 0;         /* 图片缩放是否填充， 0表示不跳充，1表示填充 */

        /* 模型推理的相关信息 */
        std::string m_strFramework;  /* 推理架构 */
    };

}    // namespace Inference_NS
