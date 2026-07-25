/**
 * @file YoloUltralytics.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 * 
 * @brief Ultralytics的yolo框架
 */
#pragma once

#include "InputDataEXT.hpp"
#include "CVInferenceYT.hpp"
#include "YOLOV8PostProcess.hpp"

namespace Inference_NS
{
    class CYoloUltralytics : public CCVInferenceYT
    {
    public:

        CYoloUltralytics(std::string strConfigPath);
        ~CYoloUltralytics();

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
         * @param [bool] bDCLResize: 是否启动了硬件缩放，硬件缩放直接将数据缩放到模型内部，不需要再赋值
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::BoxData_S>& vBoxDatas,bool bDCLResize=false);

        /**
         * @brief 设置参数
         * @param [float] fBoxThreshold: yolo的阈值
         * @param [float] fNmsThreshold: yolo的阈值
         * @return [*]
         * @note
         */
        bool setParam(float fBoxThreshold, float fNmsThreshold = -1);

    private:

        /* 后处理 */
        PostProcess_NS::cYOLOV8PostProcess* m_postProcess;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.25; /* 目标框的置信度 */
        float m_fNmsThreshold = 0.25; /* 需要过滤的重叠框程度 */
        /* 算法识别的种类 */
        int   m_nCLASS_NUM    = 1;
    };


}    // namespace Inference_NS
