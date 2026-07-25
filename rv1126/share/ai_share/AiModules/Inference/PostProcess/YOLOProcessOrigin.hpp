/**
 * @file YOLOProcessOrigin.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-17
 *
 * @brief
 */
#pragma once
#include <stdio.h>
#include <cmath>
#include <vector>

#include "OutputDataEXT.hpp"

namespace PostProcess_NS
{
    class cYOLOProcessOrigin
    {
    public:
        /**
         * @brief 数据后处理
         * @param [std::vector<float *>] vInput: 神经网络的输出
         * @param [int] nHeight: 模型的高
         * @param [int] nWidth: 模型的款
         * @param [int] nClsNum: 类别数量
         * @param [int] nAnchorsNum: 输出的Anchor数量
         * @param [float] fConfThreshold: 阈值
         * @param [float] FNmsThreshold: 阈值
         * @param [std::vector<Inference_NS::BoxData_S>&] vBoxDatas: 处理后的数据
         * @return [*]
         * @note
         */
        bool postProcess(
            std::vector<float *> vInput,
            int nWidth,
            int nHeight,
            int nClsNum,
            int nAnchorsNum,
            float fConfThreshold,
            float FNmsThreshold,
            std::vector<Inference_NS::BoxData_S> &vBoxDatas);

        /**
         * @brief 数据后处理
         * @param [std::vector<float *>] vInput: 神经网络的输出
         * @param nHeight 输入数据高
         * @param nWidth 输入数据宽
         * @param fConfThreshold 阈值
         * @param FNmsThreshold 阈值
         * @param nClassNUM 种类
         * @param nPointNum 关键点个数
         * @param vPointDatas 返回的结果
         * @return true
         * @return false
         */
        bool postProcessPoint(std::vector<float *> vInput,
                              int nHeight,
                              int nWidth,
                              float fConfThreshold,
                              float FNmsThreshold,
                              int nClassNUM,
                              int nPointNum,
                              std::vector<Inference_NS::PointData_S> &vPointDatas);

    private:
        /* 计算IOU */
        float bboxOverlap(const Inference_NS::Box_S &stVi, const Inference_NS::Box_S &stVo);
        /* 非极大值抑制 */
        bool nonMaxSuppression(std::vector<Inference_NS::BoxData_S> &vBoxDatas, float FNmsThreshold);

    private:
        /* 返回预测框最大和最小边界 */
        float m_fMinWH = 2;
        float m_fMaxWH = 7680;

    };
} // namespace PostProcess_NS