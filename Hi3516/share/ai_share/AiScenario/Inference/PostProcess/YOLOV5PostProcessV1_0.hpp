/*
 * @FilePath     : YOLOV5PostProcessV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 20:03:29
 * @Description  :
 */
#pragma once

#include <vector>

namespace PostProcessV1_0_NS
{
    class cYOLOV5PostProcess
    {
    public:

        /**
         * @brief 数据后处理
         * @param [float*] pInput0: 神经网络的第一个输出
         * @param [float*] pInput1: 神经网络的第二个输出
         * @param [float*] pInput2: 神经网络的第三个输出
         * @param [int] nHeight: 输出数据高
         * @param [int] nWidth: 输出数据宽
         * @param [float] nConfThreshold: 阈值
         * @param [float] nNmsThreshold: 阈值
         * @param [std::vector<float>&] vPointsXY: 处理后的数据
         * @return [*]
         * @note
         */
        bool postProcess(float*              pfInput0,
                         float*              pfInput1,
                         float*              pfInput2,
                         int                 nHeight,
                         int                 nWidth,
                         float               nConfThreshold,
                         float               nNmsThreshold,
                         int                 nCLASS_NUM,
                         std::vector<float>& vPoints);
                        

    private:

        inline static int clamp(float fVal, int nMin, int nMax)
        {
            return fVal > nMin ? (fVal < nMax ? fVal : nMax) : nMin;
        }

        static float sigmoid(float fX);

        static float unsigmoid(float fY);

        static int quick_sort_indice_inverse(
            std::vector<float>& vfInput,
            int                 nLeft,
            int                 nRight,
            std::vector<int>&   vnIndices);

        static int nms(
            int                 nValidCount,
            std::vector<float>& nfOutputLocations,
            std::vector<int>    vnClassIds,
            std::vector<int>&   vnOrder,
            int                 nFilterId,
            float               fThreshold);

        static float CalculateOverlap(
            float fXmin0,
            float fYmin0,
            float fXmax0,
            float fYmax0,
            float fXmin1,
            float fYmin1,
            float fXmax1,
            float fYmax1);

        int process(
            float*              pfInput,
            int*                pnAnchor,
            int                 nGridH,
            int                 nGridW,
            int                 nHeight,
            int                 nWidth,
            int                 nStride,
            std::vector<float>& vfBoxes,
            std::vector<float>& vfObjProbs,
            std::vector<int>&   vnClassId,
            float               fThreshold);


    public:
        /* 目标检测的种类 */
        int m_nOBJ_CLASS_NUM = 1;
        // coco
        int m_nAnchor0[6] = { 10, 13, 16, 30, 33, 23 };
        int m_nAnchor1[6] = { 30, 61, 62, 45, 59, 119 };
        int m_nAnchor2[6] = { 116, 90, 156, 198, 373, 326 };
    };

}    // namespace PostProcessV1_0_NS
