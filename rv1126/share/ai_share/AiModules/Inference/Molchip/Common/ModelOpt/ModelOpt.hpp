/**
 * @file ModelOpt.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-06
 *
 * @brief Molchip模型操作
 */
#pragma once

#include <string>
#include <vector>

#include "mpi_sys.h"
#include "npu/fy_npu.h"
#include "npu/fy_sys.h"

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
         * @brief 获取模型输入输出参数
         * @param [T_FY_ModelDesc&] stModelDesc: 模型输入输出的相关信息
         * @return [*]
         * @note
         */
        bool getModelDesc(T_FY_ModelDesc &stModelDesc);

        /**
         * @brief 分配指定大小、对齐方式和分配类型的内存块
         * @param [T_FY_Mem *] pMem: 指向用于存储内存分配信息的内存结构体的指针。
         * @param [uint32_t] nSize: 要分配的内存块大小
         * @param [uint32_t] nAlign: 内存块的对齐要求
         * @param [E_FY_MemAllocType] stType: 内存分配类型，定义了分配方式或使用的内存区域。
         * @return [*]
         * @note
         */
        int allocMmzMemory(T_FY_Mem *pMem, uint32_t nSize, uint32_t nAlign, E_FY_MemAllocType stType);

        /**
         * @brief 分配所有定义的内存分段
         * @param [T_FY_MemSegmentInfo*] self:  指向内存分段信息结构体的指针，用于保存内存分段的分配结果。
         * @return [*]
         * @note
         */
        int allocMemSegment(T_FY_MemSegmentInfo *self);

        /**
         * @brief 打印模型算子信息
         * @return [*]
         * @note
         */
        bool showModelOpt();

        /**
         * @brief 打印模型算子信息
         * @return [*]
         * @note
         */
        bool showModelNpu();

        /**
         * @brief 释放输入参数
         * @param [std::vector<T_FY_TaskInput>&] vTaskInputs: 输入参数
         * @return [*]
         * @note
         */
        bool releaseInputs(std::vector<T_FY_TaskInput> &vTaskInputs);

        /**
         * @brief 释放输出参数
         * @param [std::vector<T_FY_TaskOutput>&] vTaskOutputs: 输出参数
         * @param [int] nNum: 输出参数数量
         * @return [*]
         * @note
         */
        bool releaseOutputs(std::vector<T_FY_TaskOutput> &vTaskOutputs);

        /**
         * @brief 运行模型
         * @param [std::vector<T_FY_TaskInput>&] vTaskInputs: 输入参数
         * @param [std::vector<T_FY_TaskOutput>&] vTaskOutputs: 输出参数
         * @return [*]
         * @note
         */
        bool run(std::vector<T_FY_TaskInput> &vTaskInputs,
                 std::vector<T_FY_TaskOutput> &vTaskOutputs);

    private:
        /**
         * @brief 载入模型
         * @param [std::string] strFileName: 模型路径
         * @param [int&] nModelSize: 模型大小
         * @return [*]
         * @note
         */
        void loadModel(std::string strFileName, int &nModelSize);

    private:
        /* 模型路径 */
        std::string m_strModelPath;

        /* 模型对应的内存存储（MemSegMent）空间 */
        T_FY_MemSegmentInfo m_stModel;
        /* 创建的模型实例；用户利用此实例创建推理task */
        FY_NPU_MODEL_HANDLE m_pModelHandle;
        /* 创建的模型运行实例；用户利用此实例执行推理task */
        FY_NPU_TASK_HANDLE m_pTaskHandle;
        /* 子图网络运行所需内存信息 */
        T_FY_MemSegmentInfo m_stTaskMem;

        /* 模型句柄 */
        bool m_bInitialized = false;

        /* 模型输入输出相关信息 */
        T_FY_ModelDesc m_stModelDesc;
    };

} // namespace Inference_NS