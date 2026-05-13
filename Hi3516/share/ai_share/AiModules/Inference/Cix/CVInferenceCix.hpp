/**
 * @file CVInferenceCix.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-11-06
 *
 * @brief
 */
#pragma once

#include <fstream>

#include "CixModelOpt.hpp"
#include "InputDataEXT.hpp"
#include "JsonInterfase.h"
#include "OutputDataEXT.hpp"


namespace Inference_NS
{
    class CCVInferenceCix
    {
    public:

        CCVInferenceCix(std::string strConfigPath);

        ~CCVInferenceCix();

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
         * @brief 获取Mean Std对象
         * @param vfMean 均值
         * @param vfStd 方差
         * @return * void
         */
        void getMeanStd(std::vector<float>& vfMean, std::vector<float>& vfStd);

        /**
         * @brief 获取Scale Zero Point对象
         * @brief 获取预处理的缩放大小和偏移量
         * @param nIndex  模型输入下标
         * @param fScale 预处理的缩放大小
         * @param nZeroPoint 偏移量
         * @param eDataType 输入类型
         * @return true
         * @return false
         */
        bool getScaleZeroPoint(int nIndex, float& fScale, int& nZeroPoint, noe_data_type_t& eDataType);

        /**
         * @brief 推理的使用前判断
         * @param [int] nIndex: 下标
         * @param [int] nImgSize: 传入的图片数据
         * @return [*]
         * @note
         */
        bool inferenceInfe(int nIndex, int nImgSize);

        /**
         * @brief 校验模型配置文件的公共信息
         * @return [*]
         */
        bool checkModelConfig();

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        virtual bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::BoxData_S>& vBoxDatas)
        {
            printf("未定义推理函数\n");
            return false;
        }

        virtual bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas)
        {
            printf("未定义推理函数\n");
            return false;
        }

        virtual bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::PointData_S>& vPointDatas)
        {
            printf("未定义推理函数\n");
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

    private:

        /* 初始化输入输出参数 */
        bool initParams();

    protected:

        std::string   m_strConfigPath;        /* 模型json配置路径 */
        std::string   m_strModelPath;         /* 模型路径 */
        CCixModelOpt* m_pModel     = nullptr; /* 模型操作句柄 */
        int           m_nInputNum  = 0;       /* 模型输入参数数量 */
        int           m_nOutputNum = 0;       /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<tensor_desc_t> m_vInputAttrs;
        std::vector<tensor_desc_t> m_vOutputAttrs;

        /* 模型处理数据限制 */
        int m_nLimitWidth   = 0;
        int m_nLimitHeight  = 0;
        int m_nLimitChannel = 3;

        /* 模型预测处理相关的信息 */
        int                m_nChannel;        /* 模型输入的通道数 */
        std::string        strType;           /* 模型需要输入的数据类型 ["rgb", "bgr", ...]等 */
        std::vector<int>   m_vModelInputSize; /* 模型的输入形状 */
        std::vector<float> m_vMean;           /* 图片归一化的均值 */
        std::vector<float> m_vStd;            /* 图片归一化的方差 */
        int                m_nPadding = 0;    /* 图片缩放是否填充， 0表示不跳充，1表示填充 */

        /* 模型推理的相关信息 */
        std::string                   strFramework; /* 推理架构 */
        /* 模型的输出形状 */
        std::vector<std::vector<int>> m_vOutSizes;
    };

}    // namespace Inference_NS
