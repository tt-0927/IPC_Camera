/**
 * @file FaceDetect.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-30
 *
 * @brief
 */
#pragma once

#include "InputDataEXT.hpp"
#include "CVInferenceYT.hpp"
#include "RetinafacePostProcess.hpp"
namespace Inference_NS
{
    class CRetinaface : public CCVInferenceYT
    {
    public:
        CRetinaface(std::string strConfigPath);
        ~CRetinaface();

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
         * @param [bool] bDCLResize: 是否启动了硬件缩放，硬件缩放直接将数据缩放到模型内部，不需要再赋值
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::PointData_S>& vPointDatas,bool bDCLResize=false);

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
        PostProcess_NS::cRetinafacePostProcess *m_postProcess;

        /* 人脸检测的阈值 */
        float m_fBoxThreshold = 0.75;
        float m_fNmsThreshold = 0.25;
        /* 模型后处理相关参数 */
        std::vector<std::vector<int>> m_vMinSizes = {{16, 32},{64, 128},{256, 512}};
        std::vector<int> m_vSteps = {8, 16, 32};
    };

} // namespace Inference_NS
