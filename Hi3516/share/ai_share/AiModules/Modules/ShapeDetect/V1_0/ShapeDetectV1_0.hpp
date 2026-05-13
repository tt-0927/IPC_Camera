/**
 * @file ShapeDetectV1_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 * 
 * @brief 形状检测
 */
#pragma once

#include <unordered_map>
#include "ShapeDetect.hpp"
#include "ShapeDetectExt.hpp"


namespace ShapeDetect_NS
{
    class CShapeDetectV1_0
    {
    public:
        CShapeDetectV1_0(InParam_S stInParam);
        ~CShapeDetectV1_0();

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
         * @param stRelusts 推理结果
         * @return true
         * @return false
         */
        bool process(InData_S stInData, Inference_NS::InferRelust_S& stRelusts);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CShapeDetect *m_pShapeDetect = nullptr;
    };

} // namespace ShapeDetect_NS

