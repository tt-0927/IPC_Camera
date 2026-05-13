/*
 * @FilePath     : HeadCountPPV2_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 20:03:29
 * @Description  :
 */
#pragma once
#include <vector>
#include <iostream>
#include "OutputDataEXT.hpp"
using namespace std;

namespace PostProcess_NS
{
    class cYOLOV8PostProcessHisi
    {
    public:
        /**
         * @brief 目标检测数据后处理
         * @param [int] nOutNum: 输出框个数
         * @param [float*] scoreData: 置信度输出头
         * @param [uint16_t*] maxScoreIdx: 置信度最高的几个id
         * @param [float*] coord: 目标框的坐标信息
         * @param [uint16_t*] maxClass: 目标框的类别
         * @param [int] nHeight: 图片高
         * @param [int] nWidth: 图片宽
         * @param [uint32_t] coordNum: 每一列坐标数量
         * @param [float] nConfThreshold: 阈值
         * @param [float] nNmsThreshold: 阈值
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 处理后的数据
         * @return [*]
         * @note
         */
        bool postProcessDetect(
            int nOutNum,
            float* scoreData,
            uint16_t* maxScoreIdx,
            float* coord,
            uint16_t* maxClass,
            int nHeight,
            int nWidth,
            uint32_t coordNum,
            float nConfThreshold,
            float nNmsThreshold,
            std::vector<Inference_NS::BoxData_S> &vBoxDatas);
        
        /**
         * @brief 目标检测数据后处理
         * @param [int] nOutNum: 输出框个数
         * @param [float*] scoreData: 置信度输出头
         * @param [uint16_t*] maxScoreIdx: 置信度最高的几个id
         * @param [float*] coord: 目标框的坐标信息
         * @param [uint16_t*] maxClass: 目标框的类别
         * @param [float*] kptData: 关键点坐标信息
         * @param [int] nHeight: 图片高
         * @param [int] nWidth: 图片宽
         * @param [uint32_t] coordNum: 每一列坐标数量
         * @param [float] nConfThreshold: 阈值
         * @param [float] nNmsThreshold: 阈值
         * @param [std::vector<Inference_NS::PointData_S>&] vPointDatas: 处理后的数据
         * @return [*]
         * @note
         */
        bool postProcessKeyPoint(
            int nOutNum,
            float* scoreData,
            uint16_t* maxScoreIdx,
            float* coord,
            uint16_t* maxClass,
            float* kptData,
            int nHeight,
            int nWidth,
            int nKptNumPerBox,
            uint32_t coordNum,
            float nConfThreshold,
            float nNmsThreshold,
            std::vector<Inference_NS::PointData_S>& vPointDatas);

    private:
        inline static int clamp(float fVal, int nMin, int nMax)
        {
            return fVal > nMin ? (fVal < nMax ? fVal : nMax) : nMin;
        }

        static void MulticlassNms(
            vector<vector<float>>& bboxes, 
            const vector<vector<float>>& vaildBox, 
            float nmsThr);
        
        static void MulticlassNmsPoint(
            vector<vector<float>>& bboxes, 
            const vector<vector<float>>& vaildBox, 
            float nmsThr);

        static float CalcIou(
            const vector<float> &box1, 
            const vector<float> &box2);
        
        static void quick_sort_indices_desc(
            const float* inputData,
            int nLeft,
            int nRight,
            std::vector<int>& indices);
    };

} // namespace PostProcess_NS
