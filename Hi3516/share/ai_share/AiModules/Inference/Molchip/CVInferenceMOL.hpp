/**
 * @file CVInferenceMOL.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-06
 *
 * @brief
 */
#pragma once

#include <fstream>
#include "JsonInterfase.h"
#include "ModelOpt.hpp"

namespace Inference_NS
{
    /* 模型输入配置结构体 */
    typedef struct _InputInfo_
    {
        E_FY_PixelFormat enPicFormat; /* 图片输入的格式(rgb/yuv/../) */
        int nPicWidth = 0;            /* 图片的宽 */
        int nPicHeight = 0;           /* 图片的高 */
        int nRoiX = 0;                /* 图片裁剪的左上角x */
        int nRoiY = 0;                /* 图片裁剪的左上角y */
        int nRoiWidth = 0;            /* 图片裁剪的宽 */
        int nRoiHeigh = 0;            /* 图片裁剪的高 */
    } InputInfo_S;

    /* 模型初始化配置结构体 */
    typedef struct _ModelInitInfo_
    {
        std::string strModelPath; /* 模型路径 */

        /* 仅在模型开启aipp，需要设置模型输入配置结构体
        ** aipp不开启为false，模型需要传入chw的bgr数据
        */
        bool bAipp = false;
        std::vector<InputInfo_S> vInputInfo; /* 每个输入的宽高等信息 */

        /* 非必要设置参数 */
        bool showNpuInfo = true; /* 打印npu信息 */
        bool showOpInfo = false;  /* 打印模型算子信息 */

    } ModelInitInfo_S;

    // /* 图片数据结构体 */
    // typedef struct _ImageData_
    // {
    //     int nWidth = 0;            /* 输入图片的宽 */
    //     int nHeight = 0;           /* 输入图片的高 */
    //     uint64_t *pData = nullptr; /* 输入图片的数据 */
    //     int nSize = 0;             /* 图片数据对应的大小 */
    // } ImageData_S;

    class CCVInferenceMOL
    {
    public:
        CCVInferenceMOL(std::string strConfigPath);

        ~CCVInferenceMOL();

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
         * @brief 初始化输入输出参数
         * @return [*]
         * @note
         */
        bool initParams();

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
        std::string  m_strConfigPath;            /* 模型json配置路径 */
        ModelInitInfo_S m_stModelInitInfo;       /* 模型初始化信息，包含输入尺寸、模型地址 */
        CModelOpt *m_pModel = nullptr;           /* 模型操作句柄 */
        std::vector<T_FY_TaskInput> m_vInputs;   /* 模型输入参数 */
        int m_nInputNum = 0;                     /* 模型输入参数数量 */
        std::vector<T_FY_TaskOutput> m_vOutputs; /* 模型输出参数 */
        int m_nOutputNum = 0;                    /* 模型输出参数数量 */

        /* 模型输入输出相关信息 */
        T_FY_ModelDesc m_stModelDesc;

        /* 模型处理数据限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;

        /* 模型预测处理相关的信息 */
        int m_nChannel;             /* 模型输入的通道数 */
        int m_nAIPP;                  /* aipp加速 */
        std::string strType;        /* 模型需要输入的数据类型 ["rgb", "bgr", ...]等 */
        std::vector<int> m_vModelInputSize; /* 模型的输入形状 */
        std::vector<int> m_vImageInputSize; /* 输入图片形状 */
        std::vector<int> m_vMean;   /* 图片归一化的均值 */
        std::vector<int> m_vStd;    /* 图片归一化的方差 */ 
        int m_nPadding = 0;         /* 图片缩放是否填充， 0表示不跳充，1表示填充 */

    };

} // namespace Inference_NS
