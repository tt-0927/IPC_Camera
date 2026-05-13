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

namespace PostProcess_NS
{
    class cYOLOV8PostProcess
    {
    public:
        /**
         * @brief 目标检测数据后处理
         * @param [std::vector<float *>] vfInput: 神经网络的输出
         * @param [int] nHeight: 输出数据高
         * @param [int] nWidth: 输出数据宽
         * @param [float] nConfThreshold: 阈值
         * @param [float] nNmsThreshold: 阈值
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 处理后的数据
         * @param [int] nFlLen: 默认值为16(YOLOV8框架的参数)，设置该参数，主要为了适配YOLOV6框架的接入
         * @return [*]
         * @note
         */
        bool postProcessDetect(
            std::vector<float *> vfInput,
            int nHeight,
            int nWidth,
            float nConfThreshold,
            float nNmsThreshold,
            int nCLASS_NUM,
            std::vector<Inference_NS::BoxData_S> &vBoxDatas,
            int nFlLen = 16);

        /**
         * @brief lite模型(有4个网络输出)目标检测数据后处理
         * @param [std::vector<float *>] vfInput: 神经网络的输出
         * @param [int] nHeight: 输出数据高
         * @param [int] nWidth: 输出数据宽
         * @param [float] nConfThreshold: 阈值
         * @param [float] nNmsThreshold: 阈值
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 处理后的数据
         * @param [int] nFlLen: 默认值为16(YOLOV8框架的参数)，设置该参数，主要为了适配YOLOV6框架的接入
         * @return [*]
         * @note
         */
        bool postProcessLiteDetect(
            std::vector<float *> vfInput,
            int nHeight,
            int nWidth,
            float nConfThreshold,
            float nNmsThreshold,
            int nCLASS_NUM,
            std::vector<Inference_NS::BoxData_S> &vBoxDatas,
            int nFlLen = 16);

        /**
         * @brief 关键点数据后处理
         * @param [std::vector<float *>] vfInput: 神经网络的输出
         * @param [int] nHeight: 输出数据高
         * @param [int] nWidth: 输出数据宽
         * @param [float] nConfThreshold: 阈值
         * @param [float] nNmsThreshold: 阈值
         * @param [int] nCLASS_NUM: 种类数量
         * @param [int] nKeyPoint_NUM: 关键点数量
         * @param [std::vector<Inference_NS::PointData_S>&]vPointDatas: 处理后的数据
         * @param [int] nFlLen: 默认值为16(YOLOV8框架的参数)，设置该参数主要为了适配YOLOV6框架的接入
         * @return [*]
         * @note
         */
        bool postProcessKeyPoint(
            std::vector<float *> vfInput,
            int nHeight,
            int nWidth,
            float nConfThreshold,
            float nNmsThreshold,
            int nCLASS_NUM,
            int nKeyPoint_NUM,
            std::vector<Inference_NS::PointData_S> &vPointDatas,
            int nFlLen = 16,
            bool bPointShow=false
        );

    private:
        inline static int clamp(float fVal, int nMin, int nMax)
        {
            return fVal > nMin ? (fVal < nMax ? fVal : nMax) : nMin;
        }

        static float sigmoid(float fX);

        static float unsigmoid(float fY);

        static int quick_sort_indice_inverse(
            std::vector<float> &vfInput,
            int nLeft,
            int nRight,
            std::vector<int> &vnIndices);

        static int nms(
            int nValidCount,
            std::vector<float> &nfOutputLocations,
            std::vector<int> vnClassIds,
            std::vector<int> &vnOrder,
            int nFilterId,
            float fThreshold);

        static float CalculateOverlap(
            float fXmin0,
            float fYmin0,
            float fXmax0,
            float fYmax0,
            float fXmin1,
            float fYmin1,
            float fXmax1,
            float fYmax1);

        void compute_dfl(
            float *tensor,
            int dfl_len,
            float *box);

        int processDetect(
            float *box_tensor,
            float *score_tensor,
            float *score_sum_tensor,
            int nGridH,
            int nGridW,
            int nStride,
            int nDfl_len,
            std::vector<float> &vfBoxes,
            std::vector<float> &vfObjProbs,
            std::vector<int> &vnClassId,
            float fThreshold,
            int nObjClassNum);

        int processKeyPoint(
            float *box_tensor,
            float *score_tensor,
            float *score_sum_tensor,
            float *point_tensor,
            int nGridH,
            int nGridW,
            int nStride,
            int nDfl_len,
            std::vector<float> &vfBoxes,
            std::vector<float> &vfObjProbs,
            std::vector<float> &vfPoints,
            std::vector<int> &vnClassId,
            float fThreshold,
            int nObjClassNum,
            bool bPointShow=false
        );

    public:
        /* 目标检测的种类 */
        int m_nOBJ_CLASS_NUM = 1;
        /* 关键点的数量 */
        int nPointNum = 0;
        // coco
        // int m_nAnchor0[6] = { 10, 13, 16, 30, 33, 23 };
        // int m_nAnchor1[6] = { 30, 61, 62, 45, 59, 119 };
        // int m_nAnchor2[6] = { 116, 90, 156, 198, 373, 326 };
    };

} // namespace PostProcess_NS
