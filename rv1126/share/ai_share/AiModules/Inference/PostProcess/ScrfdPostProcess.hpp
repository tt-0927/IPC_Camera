/**
 * @file ScrfdPostProcess.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-01
 * 
 * @brief 
 */
#pragma once
#include <stdio.h>
#include <cmath>
#include <vector>
#include <unordered_map>

#include "OutputDataEXT.hpp"

namespace PostProcess_NS
{
    /* nested classes */
    typedef struct _ScrfdPoint_
    {
      float fCx;
      float fCy;
      float fStride;
    } ScrfdPoint_S;

    class cScrfdPostProcess
    {
    public:
        /**
         * @brief 数据后处理
         * @param [std::vector<float *>] vInput: 神经网络的输出
         * @param nHeight 模型输入的高
         * @param nWidth 模型输入的宽
         * @param nConfThreshold 阈值
         * @param nNmsThreshold 阈值
         * @param vPointDatas 返回的结果
         * @return true
         * @return false
         */
        bool postProcess(std::vector<float *> vInput,
                              int nHeight,
                              int nWidth,
                              float nConfThreshold,
                              float nNmsThreshold,
                              std::vector<Inference_NS::PointData_S> &vPointDatas);

    private:
        /* 用于生成目标高度和宽度范围内的点 */
        void generatePoints(
            const int nTargetHeight, 
            const int nTargetWidth
        );
        /* 快排算法 */
        static int quick_sort_indice_inverse(
            std::vector<float> &vfInput,
            int nLeft,
            int nRight,
            std::vector<int> &vnIndices);
        /* 非极大值抑制 */
        static int nms(
            int nValidCount,
            std::vector<float> &nfOutputLocations,
            std::vector<int> &vnOrder,
            float fThreshold);
        /* IOU计算 */
        static float CalculateOverlap(
            float fXmin0,
            float fYmin0,
            float fXmax0,
            float fYmax0,
            float fXmin1,
            float fYmin1,
            float fXmax1,
            float fYmax1);
        /* 数据处理 */
        void processPoint(
            int nNumPoints,
            float *pScore, 
            float *pBox,
            float *pKps, 
            int nStride, 
            float fConfThreshold, 
            float nHeight,
            float nWidth, 
            std::vector<float> &vfBoxes,
            std::vector<float> &vfPoints,
            std::vector<float> &vfObjProbs);

    public:
        /* 存放目标高度和宽度范围内的点 */
        bool bCenterPointsUpdate = false;
        std::unordered_map<int, std::vector<ScrfdPoint_S>> mCenterPoints;
        /* 是否包含关键点 */
        bool bUseKps = true;
        /* 取非极大值抑制个数上限 */
        int nMaxNms = 30000;
        /* anchors的个数 */
        int nNumAnchors = 2;
        /* 特征采样倍数 */
        std::vector<int> vFeatStrideFpn = {8, 16, 32};
    };

} // namespace PostProcess_NS
