/*
 * @FilePath     : PlayPhone.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-08-01 16:29:52
 * @Description  : 玩手机检测算法
 */
#pragma once

#include "CVInferenceRK_V1_0.hpp"
#include "YOLOV5PostProcessV1_0.hpp"

namespace InferenceV1_0_NS
{
    class CPlayPhone : public CCVInferenceRK
    {
    public:

        CPlayPhone(std::string strModelPath);
        ~CPlayPhone();

    private:
        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<float>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData);


    private:
        
        /* 后处理 */
        PostProcessV1_0_NS::cYOLOV5PostProcess* m_postProcess;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.25;
        float m_fNmsThreshold = 0.25;
        /* 算法识别的种类 */
        int m_nCLASS_NUM = 1;
    };


}    // namespace InferenceV1_0_NS
