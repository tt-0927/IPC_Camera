/**
 * @file YoloUltralyticsPoint.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 * 
 * @brief Ultralytics的yolo框架
 */
#pragma once

#include "InputDataEXT.hpp"
#include "CVInferenceHISI.hpp"
#include "YOLOV8PostProcess_Hisi_rpn.hpp"

namespace Inference_NS
{
    class CYoloUltralyticsPoint final : public CCVInferenceHISI
    {
    public:

        CYoloUltralyticsPoint(std::string strConfigPath);
        ~CYoloUltralyticsPoint();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true 
         * @return false 
         */
        bool checkModelProConfig() override;

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::PointData_S>& vPointDatas);

    private:

        /* 后处理 */
        PostProcess_NS::cYOLOV8PostProcessHisi* m_postProcess;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.25; /* 目标框的置信度 */
        float m_fNmsThreshold = 0.25; /* 需要过滤的重叠框程度 */

        /* 算法识别的种类 */
        int   m_nCLASS_NUM    = 1;
        /* 算法识别的关键点数量 */
        int   m_nKeyPoint_NUM = 0;
    };


}    // namespace Inference_NS
