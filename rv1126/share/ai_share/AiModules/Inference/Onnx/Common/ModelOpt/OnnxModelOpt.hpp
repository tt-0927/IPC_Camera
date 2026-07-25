/**
 * @file OnnxModelOpt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief Onnx模型操作
 */
#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "onnxruntime_cxx_api.h"

namespace Inference_NS
{
    class COnnxModelOpt
    {
    public:
        COnnxModelOpt(std::string strModelPath);
        ~COnnxModelOpt();

        /**
         * @brief 设置模型路径
         * @return [*]
         * @note 再次初始化的时候生效
         */
        void setModelPath(std::string strModelPath);

        /**
         * @brief 初始化模型
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化模型
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 获取模型输入参数
         * @param [vector<std::vector<int64_t>>&] vInputAttrs: 输入参数
         * @return [*]
         * @note
         */
        bool getInputAttrs(std::vector<std::vector<int64_t>> &vInputAttrs);

        /**
         * @brief 获取模型输出参数
         * @param [std::vector<std::vector<int64_t>>&] vInputAttrs: 输出参数
         * @return [*]
         * @note
         */
        bool getOutputAttrs(std::vector<std::vector<int64_t>> &vOutputAttrs);

        /**
         * @brief 运行模型
         * @param [std::vector<float*>] vInputs: 输入的图片数据
         * @param [std::vector<int64_t>] nInputDataSizes: 输入的数据大小
         * @param [std::vector<float*>&] vOutputs: 模型的输出
         * @return [*]
         * @note
         */
        bool run(
            std::vector<float *> vInputs,
            std::vector<int64_t> nInputDataSizes,
            std::vector<float *> &vOutputs);

        bool run(
            std::vector<Ort::Value> &vInputs,
            std::vector<Ort::Value> &vOutputs);

        /**
         * @brief 设置GPU的相关配置
         * @param nCpuInferThread  CPU推理的线程数
         * @param nDeviceId  GPU的设备序号
         * @param nGpuMemLimit  显存限制
         */
        void setInferData(
            int nCpuInferThread = 1,
            int nDeviceId = 0,
            long long nGpuMemLimit = 2 * 1024 * 1024 * 1024);

        bool getStringMetadata(std::string strName, std::string &strOutData);

    public:
        /* 内存分配管理​​的工具类 */
        Ort::AllocatorWithDefaultOptions m_stAllocator;

    private:
        /* 模型路径 */
        std::string m_strModelPath;

        /* 模型句柄 */
        Ort::Env m_stEnv;
        Ort::Session m_stSession{nullptr};

        /* 创建默认分配器 */
        Ort::ModelMetadata m_stModelMetadata;

        bool m_bInitialized = false;

        /* 获取模型的输入和输出的数量 */
        size_t m_nInputNum = 0;
        size_t m_nOutputNum = 0;

        /* 模型的输入和输出的名字和大小 */
        std::vector<std::string> m_vInputNamesStr;
        std::vector<const char *> m_vInputNames;
        std::vector<std::vector<int64_t>> m_vInputAttrs;
        std::vector<std::string> m_vOutputNamesStr;
        std::vector<const char *> m_vOutputNames;
        std::vector<std::vector<int64_t>> m_vOutputAttrs;

        /* CPU的相关信息 */
        int m_nCpuInferThread = 1; /* CPU推理的线程数 */
        /* GPU相关信息配置 */
        int m_nDeviceId = 0;                               /* 选择 GPU 设备的序列号 */
        long long m_nGpuMemLimit = 2 * 1024 * 1024 * 1024; /* 模型限制最大显存 */
        int m_nArenaExtendStrategy = 0;                    /* 内存分配策略 */
        int m_nDoCopyInDefaultStream = 1;                  /* 数据复制操作策略 */
    };

} // namespace Inference_NS