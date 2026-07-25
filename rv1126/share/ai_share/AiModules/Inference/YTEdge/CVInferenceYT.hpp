/**
 * @file CVInferenceYT.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-13
 * 
 * @brief 
 */
#pragma once

#include <fstream>
#include "ModelOpt.hpp"
#include "JsonInterfase.h"
#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"

#include "dcl_memory.h"

namespace Inference_NS
{
    class CCVInferenceYT
    {
    public:
        CCVInferenceYT(std::string strConfigPath);

        ~CCVInferenceYT();

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
         * @brief 获取模型的Tensor信息
         * @param [int] nIndex: 第几个输入图片, 0开始
         * @param [TensorAttr_S&] stTensor: 模型的相关tensor信息
         * @return [*]
         * @note
         */
        bool getModelTensor(int nIndex, TensorAttr_S& stTensor);


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
         * @note [*]
         */
        bool setInputDatas(unsigned char *pDataBuffer, int nDataSize, int nInputIndex);

        /**
         * @brief 校验模型配置文件的公共信息
         * @return [*]
         */
        bool checkModelConfig();

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 推理出来的数据
         * @param [bool] bDCLResize: 是否启动了硬件缩放，硬件缩放直接将数据缩放到模型内部，不需要再赋值
         * @return [*]
         * @note
         */
        virtual bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::BoxData_S>& vBoxDatas,bool bDCLResize=false)
        {
            return false;
        }
        virtual bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas,bool bDCLResize=false)
        {
            return false;
        }
        virtual bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::PointData_S>& vPointDatas,bool bDCLResize=false)
        {
            return false;
        }

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
        std::string m_strConfigPath;               /* 模型json配置路径 */
        std::string m_strModelPath;                /* 模型路径 */
        std::string m_strTyhcpConfigPath;          /* tyhcp系统配置文件路径 */
        CModelOpt *m_pModel = nullptr;             /* 模型操作句柄 */
        dclmdlDataset *m_pInputDataset = nullptr;  /* 模型输入参数 */
        int m_nInputNum = 0;                       /* 模型输入参数数量 */
        dclmdlDataset *m_pOutputDataset = nullptr; /* 模型输出参数 */
        int m_nOutputNum = 0;                      /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<TensorAttr_S> m_vInputAttrs;
        std::vector<TensorAttr_S> m_vOutputAttrs;

        /* 模型处理数据限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;

        /* 模型预测处理相关的信息 */
        int m_nChannel;                     /* 模型输入的通道数 */
        std::string strType;                /* 模型需要输入的数据类型 ["rgb", "bgr", ...]等 */
        std::vector<int> m_vModelInputSize; /* 模型的输入形状 */
        std::vector<int> m_vMean;           /* 图片归一化的均值 */
        std::vector<int> m_vStd;            /* 图片归一化的方差 */
        int m_nPadding = 0;                 /* 图片缩放是否填充， 0表示不跳充，1表示填充 */

        /* 模型推理的相关信息 */
        std::string m_strFramework; /* 推理架构 */
    };

} // namespace Inference_NS
