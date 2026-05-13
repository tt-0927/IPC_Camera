/*
 * @FilePath     : CVInferenceHISI.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-23 15:51:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 09:43:52
 * @Description  :
 */
#pragma once

#include <fstream>
#include "ModelOpt.hpp"
#if CAP_AI_USE_SIMPLE_JSON
#include "Json.h"
#else
#include "JsonInterfase.h"
#endif

namespace Inference_NS
{
    class CCVInferenceHISI
    {
    public:

        CCVInferenceHISI(std::string strConfigPath);

        ~CCVInferenceHISI();

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
         * @brief 为模型额外的 TaskBuffer 和 WorkBuffer 输入创建输入缓冲区
         * @param [float] confThresh: 置信度参数
         * @param [float] nmsThresh: nms参数
         * @param [int] nInputIndex: 第几个模型输入
         * @return [*]
         * @note
         */
        bool setYolov8PostProcessParameters(float confThresh, float nmsThresh, int nInputIndex);

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
        /* 模型的输入信息 */
        int          m_nInputNum  = 0;       /* 模型输入参数数量 */
        svp_acl_mdl_dataset *m_pInputs = nullptr;
        int64_t m_loopTimes = 1;               /* 用于计算 NCHW 等维度下的样本行数 */
        size_t m_lineSize = 0;                   /* 每行有效数据的大小 */
        std::vector<svp_acl_data_buffer*> m_InputData;
        std::vector<size_t> m_vInputDevSize;              /* 设备端图像 buffer 的大小 */
        std::vector<size_t> m_vInputStride;               /* 图像的 stride */
        std::vector<svp_acl_mdl_io_dims> m_vInputDims;    /* 存储模型输入张量的维度信息 */
        std::vector<void*> m_veviceBuffers;               /* 存放推理数据 */
        
        /* 模型的输出信息 */
        int          m_nOutputNum = 0;       /* 模型输出参数数量 */
        svp_acl_mdl_dataset *m_pOutputs = nullptr;
        std::vector<std::vector<int>> m_vOutputDims;
        std::vector<svp_acl_data_buffer*> m_vOutputBuffers;

        /* 模型处理数据限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;

        /* 配置文件及模型信息 */
        std::string  m_strConfigPath;        /* 模型json配置路径 */
        std::string  m_strModelPath;         /* 模型路径 */
        CModelOpt*   m_pModel     = nullptr; /* 模型操作句柄 */


        /* 模型预测处理相关的信息 */
        int m_nChannel = 0;             /* 模型输入的通道数 */
        std::string strType;        /* 模型需要输入的数据类型 ["rgb", "bgr", ...]等 */
        std::vector<int> m_vModelInputSize; /* 模型的输入形状 */
        std::vector<int> m_vMean;   /* 图片归一化的均值 */
        std::vector<int> m_vStd;    /* 图片归一化的方差 */ 
        int m_nPadding = 0;         /* 图片缩放是否填充， 0表示不跳充，1表示填充 */

        /* 模型推理的相关信息 */
        std::string m_strFramework;  /* 推理架构 */
    };

}    // namespace Inference_NS
