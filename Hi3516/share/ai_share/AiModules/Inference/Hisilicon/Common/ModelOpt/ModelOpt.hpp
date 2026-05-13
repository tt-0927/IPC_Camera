/*
 * @FilePath     : ModelOpt.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 08:40:26
 * @Description  : HISI模型操作
 */
#pragma once

#include <string>
#include <vector>

#include "svp_acl.h"
#include "svp_acl_mdl.h"

namespace Inference_NS
{
    class CModelOpt
    {
    public:

        CModelOpt(std::string strModelPath);
        ~CModelOpt();

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
         * @brief 获取模型输入的参数量
         * @param [int&] inputSize: 模型输入的参数量
         * @return [*]
         * @note
         */
        bool getInputSize(int& inputSize);

        /**
         * @brief 获取模型输出的参数量
         * @param [int&] outputSize: 模型输出的参数量
         * @return [*]
         * @note
         */
        bool getOutputSize(int& outputSize);

        /**
         * @brief 获取模型输入参数
         * @param [int] nIndex: 第几个输入头
         * @param [size_t&] bufSize: 设备端图像 buffer 的大小
         * @param [size_t&] stride: 图像的 stride
         * @param [svp_acl_mdl_io_dims&] inputDims: 模型输入张量的维度信息
         * @param [size_t&] dataSize: 总数据大小
         * @return [*]
         * @note
         */
        bool getInputAttrs(int nIndex, size_t& bufSize, size_t& stride, svp_acl_mdl_io_dims& inputDims, size_t& dataSize);

        /**
         * @brief 获取模型输出参数
         * @param [int] nIndex: 第几个输出头
         * @param [size_t&] stride: 输出的 stride
         * @param [size_t&] bufSize: 输出 buffer 的大小
         * @param [svp_acl_mdl_io_desc&] outputDims: 获取输出属性信息
         * @return [*]
         * @note
         */
        bool getOutputAttrs(int nIndex, size_t& stride, size_t& bufSize, svp_acl_mdl_io_dims& outputDims);

        /**
         * @brief 运行模型
         * @param [svp_acl_mdl_dataset *&] pInputs: 输入数据
         * @param [svp_acl_mdl_dataset *&] pOutputs: 输出数据
         * @return [*]
         * @note
         */
        bool run(svp_acl_mdl_dataset*& pInputs, svp_acl_mdl_dataset*& pOutputs);

        /**
         * @brief 卸载模型
         * @return [*]
         * @note
         */
        void Unload();

        /**
         * @brief 输入释放
         * @param [svp_acl_mdl_dataset *&] pInputs: 输入数据
         * @return [*]
         * @note
         */
        void DestroyInput(svp_acl_mdl_dataset*& pInputs);

        /**
         * @brief 输出释放
         * @param [svp_acl_mdl_dataset *&] pOutputs: 输出数据
         * @return [*]
         * @note
         */
        void DestroyOutput(svp_acl_mdl_dataset*& pOutputs);

        /**
         * @brief 资源释放函数
         * @return [*]
         * @note
         */
        void DestroyResource();
    
    public:
        int32_t deviceId = 0;                      /* 设置当前使用的设备 ID */
        int nWaitTimeOut = 0;                      /* 设置操作超时时间, 0（即：无限等待，不超时） */
        /* ACL 资源 */
        svp_acl_rt_context context_ = nullptr;
        svp_acl_rt_stream stream_ = nullptr;
        uint32_t modelId_ = 0;
        void* modelMemPtr_ = nullptr;

    private:
        bool m_bInitialized = false;
        std::string m_strModelPath;
        /* 模型信息 */
        svp_acl_mdl_desc* modelDesc_ = nullptr;
    };
}    // namespace Inference_NS