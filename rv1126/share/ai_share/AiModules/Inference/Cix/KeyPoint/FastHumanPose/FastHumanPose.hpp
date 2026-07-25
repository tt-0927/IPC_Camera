/**
 * @file FastPose.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-28
 *
 * @brief Yolov5关键点检测
 */
#pragma once

#include <iostream>

#include "CVInferenceCix.hpp"
#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"


namespace Inference_NS
{
    class CFastHumanPose : public CCVInferenceCix
    {
    public:

        CFastHumanPose(std::string strConfigPath);
        ~CFastHumanPose();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true
         * @return false
         */
        bool checkModelProConfig() override;

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::PointData_S>&] vPointDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::PointData_S>& vPointDatas);
    };


}    // namespace Inference_NS
