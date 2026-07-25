/**
 * @file VehicleAttributeV1_0.hpp
 * @author CaiShengJie (Caisj@kfb.cn)
 * @date 2024-10-10
 *
 * @brief
 */
#pragma once

#include <unordered_map>
#include "VehicleAttribute.hpp"
#include "VehicleAttributeExt.hpp"

namespace VehicleAttribute_NS
{
    class CVehicleAttributeV1_0
    {
    public:
        CVehicleAttributeV1_0(InParam_S stInParam);
        ~CVehicleAttributeV1_0();

        /**
         * @brief 初始化
         * @return true
         * @return false
         */
        bool init();

        /**
         * @brief 反初始化
         * @return true
         * @return false
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param stInData 传入的视频数据
         * @param nResult 分析的参数
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<Result_S> &nResult);
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);


    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CVehicleAttribute *m_pVehicleAttribute = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;
    };

} // namespace VehicleAttribute_NS
