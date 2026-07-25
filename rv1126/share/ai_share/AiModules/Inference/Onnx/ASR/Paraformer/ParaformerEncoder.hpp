/**
 * @file ParaformerEncoder.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#pragma once

#include "InputDataEXT.hpp"
#include "AVInferenceOnnx.hpp"
#include "onnxruntime_cxx_api.h"

namespace Inference_NS
{
    class CParaformerEncoder : public CAVInferenceOnnx
    {
    public:
        CParaformerEncoder(std::string strConfigPath);
        ~CParaformerEncoder();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true
         * @return false
         */
        bool checkModelProConfig() override;

        /**
         * @brief 推理数据
         * @param [std::vector<Ort::Value>&>] vInputs: 传入的结构体
         * @param [std::vector<Ort::Value>&] vOutputs: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(std::vector<Ort::Value> &vInputs,
                       std::vector<Ort::Value> &vOutputs);

        /**
         * @brief 设置参数
         * @param [float] fBoxThreshold: yolo的阈值
         * @param [float] fNmsThreshold: yolo的阈值
         * @return [*]
         * @note
         */
        bool setParam(float fBoxThreshold, float fNmsThreshold = -1);

        /* 获取模型内部数据 */
        std::string getMetadata(std::string strName);
    };

} // namespace Inference_NS
