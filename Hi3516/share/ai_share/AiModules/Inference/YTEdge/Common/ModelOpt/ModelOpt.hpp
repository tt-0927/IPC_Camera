/**
 * @file ModelOpt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-11
 *
 * @brief RK模型操作
 */
#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "dcl_mpi.h"
#include "dcl.h"
#include "dcl_memory.h"

namespace Inference_NS
{
    struct Tensor
    {
        int idx{-1};
        std::string name;
        void *data{nullptr}; // device
        void *host_data{nullptr};
        uint64_t phyAddr{0};
        int64_t dimCount{0};
        int64_t dims[DCL_MAX_DIM_CNT]{};
        dclFormat layout{DCL_FORMAT_UNDEFINED};
        dclDataType dtype{DCL_FLOAT};

        void copyDims(dclmdlIODims &d)
        {
            dimCount = d.dimCount;
            memcpy(dims, d.dims, dimCount * sizeof(int64_t));
        }

        std::string dimsToString()
        {
            std::string shape_s;
            for (int d = 0; d < dimCount - 1; ++d)
            {
                shape_s += std::to_string(dims[d]);
                shape_s += "x";
            }
            shape_s += std::to_string(dims[dimCount - 1]);
            return shape_s;
        }

        size_t size() const
        {
            if (0 == dimCount)
                return 0;
            size_t length = 1;
            for (int i = 0; i < dimCount; ++i)
                length *= dims[i];
            return length;
        }

        size_t size_bytes()
        {
            switch (dtype)
            {
            case DCL_INT64:
            case DCL_UINT64:
                return size() * 8;
            case DCL_FLOAT:
            case DCL_INT32:
            case DCL_UINT32:
                return size() * 4;
            case DCL_FLOAT16:
            case DCL_INT16:
            case DCL_UINT16:
                return size() * 2;
            case DCL_INT8:
            case DCL_UINT8:
            case DCL_BOOL:
                return size();
            default:
                break;
            }
            return -1;
        }
    };
    typedef struct _TensorAttr_
    {
        Tensor stTensor; /* 输入/输出信息结构体 */
        int nDataSize;   /* 该输入/输出需要的大小 */
    } TensorAttr_S;

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
         * @brief 获取模型输入参数
         * @param [std::vector<TensorAttr_S>&] vInputAttrs: 输入参数
         * @return [*]
         * @note
         */
        bool getInputAttrs(std::vector<TensorAttr_S> &vInputAttrs);

        /**
         * @brief 获取模型输出参数
         * @param [std::vector<TensorAttr_S>&] vInputAttrs: 输出参数
         * @return [*]
         * @note
         */
        bool getOutputAttrs(std::vector<TensorAttr_S> &vOutputAttrs);

        /**
         * @brief 运行模型
         * @param [dclmdlDataset*] pInputDataset: 模型输入
         * @param [dclmdlDataset*] pOutputDataset: 模型输出
         * @return [*]
         * @note
         */
        bool run(dclmdlDataset *pInputDataset, dclmdlDataset *pOutputDataset);

        /* NPU利用率 */
        bool getUtilizationRate();
        /* 查询模型的网络结构 */
        bool getNetGrap();

    private:
        /* 数据格式转换 */
        std::string dclDataTypeToString(dclDataType &dataType);

    private:
        /* 模型路径 */
        uint32_t m_nModelId = 1000000; /* 模型ID号 */
        std::string m_strModelPath;

        bool m_bInitialized = false;

        /* 模型输入输出IO信息 */
        dclmdlDesc *m_pModelDesc = nullptr;

        /* tensor的属性信息 */
        std::vector<TensorAttr_S> m_vInputAttrs;
        std::vector<TensorAttr_S> m_vOutputAttrs;
    };

} // namespace Inference_NS