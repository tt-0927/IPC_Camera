/**
 * @file HandLandmark.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-28
 * 
 * @brief 首部321个关键点+左右手分类
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "CVInferenceRK.hpp"
#include <iostream>

namespace Inference_NS
{
    class CHandLandmark : public CCVInferenceRK
    {
    public:

        CHandLandmark(std::string strConfigPath);
        ~CHandLandmark();

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

        /**
         * @brief 设置参数
         * @param [float] fBoxThreshold: yolo的阈值
         * @param [float] fNmsThreshold: yolo的阈值
         * @return [*]
         * @note
         */
        bool setParam(float fBoxThreshold, float fNmsThreshold = -1);

    private:
        /* 算法识别的关键点数量 */
        int   m_nKeyPointNUM = 21;
        /* 是否为手的分类阈值 */
        float m_fConfThreshold = 0.5;
    };


}    // namespace Inference_NS
