/**
 * @file CVInferenceOnnx.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#pragma once

#include <fstream>
#include "JsonInterfase.h"
#include "OnnxModelOpt.hpp"

namespace Inference_NS
{
    class CCVInferenceOnnx
    {
    public:
        CCVInferenceOnnx(std::string strConfigPath);

        ~CCVInferenceOnnx();

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
        bool getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel);

        /**
         * @brief 获取配置文件的均值和方差
         * @param [std::vector<int>] vMean: 均值
         * @param [std::vector<int>&] vStd: 方差
         * @return [*]
         * @note
         */
        bool getMeanStd(std::vector<float>& vMean, std::vector<float>& vStd);


        /**
         * @brief 推理的使用前判断
         * @param [int] nImgSize: 传入的图片数据
         * @return [*]
         * @note
         */
        bool inferenceInfe(int nImgSize);

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
        std::string m_strConfigPath;   /* 模型json配置路径 */
        std::string m_strModelPath;    /* 模型路径 */
        COnnxModelOpt *m_pModel = nullptr; /* 模型操作句柄 */
        int m_nInputNum = 0;           /* 模型输入参数数量 */
        int m_nOutputNum = 0;          /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<std::vector<int64_t>> m_vInputAttrs;
        std::vector<std::vector<int64_t>> m_vOutputAttrs;

        /* 模型处理数据限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;

        /* 模型预测处理相关的信息 */
        int m_nChannel;             /* 模型输入的通道数 */
        std::string strType;        /* 模型需要输入的数据类型 ["rgb", "bgr", ...]等 */
        std::vector<int> m_vModelInputSize; /* 模型的输入形状 */
        std::vector<float> m_vMean;   /* 图片归一化的均值 */
        std::vector<float> m_vStd;    /* 图片归一化的方差 */ 
        int m_nPadding = 0;         /* 图片缩放是否填充， 0表示不跳充，1表示填充 */

        /* 模型推理的相关信息 */
        std::string strFramework;  /* 推理架构 */
        int m_nCpuInferThread = 1; /* CPU推理的线程数 */
        int m_nDeviceId = 0;    /* 选择 GPU 设备的序列号 */
        long long m_nGpuMemLimit = 2 * 1024 * 1024 * 1024;  /* 模型限制最大显存 */
    };

} // namespace Inference_NS
