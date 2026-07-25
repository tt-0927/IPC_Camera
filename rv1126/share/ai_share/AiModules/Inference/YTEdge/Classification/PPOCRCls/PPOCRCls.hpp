/**
 * @file PPOCRCls.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-22
 * 
 * @brief 
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "iostream"

namespace Inference_NS
{
    class CPPOCRCls : public CCVInferenceYT
    {
    public:

        CPPOCRCls(std::string strConfigPath);
        ~CPPOCRCls();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true 
         * @return false 
         */
        bool checkModelProConfig() override;

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ClsData_S>&] vClsDatas: 推理出来的数据
         * @param [bool] bDCLResize: 是否启动了硬件缩放，硬件缩放直接将数据缩放到模型内部，不需要再赋值
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas,bool bDCLResize=false);

        /**
         * @brief 后处理
         * @param [float*] pOutData: 模型输入
         * @param [int nOutChannel] 输入通道
         * @param [int nOutSeqLen:] 文字的类别总数
         * @param [Inference_NS::ClsData_S&] stClsData 识别的结果结构体: 
         * @return [*]
         * @note
         */
        bool recPostprocess(float* pOutData, int nOutChannel, int nOutSeqLen, Inference_NS::ClsData_S& stClsData);
    };


}    // namespace Inference_NS
