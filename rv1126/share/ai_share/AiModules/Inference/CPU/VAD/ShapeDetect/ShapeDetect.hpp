/**
 * @file ShapeDetect.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 *
 * @brief 形状检测（只支持多边形检测）
 */
#pragma once

#include <cmath>
#include <iostream>
#include <sndfile.h>
#include <vector>
#include <algorithm>
/* 自定义的预处理头文件 */
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

namespace Inference_NS
{
    typedef struct _INFERPARAM_
    {
        float dRCircularity = 0.85; /* 圆形的圆度阈值 */
        float dPCircularity = 0.5;  /* 多边形的圆度阈值 */

        float fEpsilonNum = 0.09; /* 原始轮廓与近似多边形之间的最大距离,越小会保留更多的细节 */
    } InferParam_S;

    typedef enum
    {
        NullShape = 0, /* 未识别到图像 */
        Triangle,      /* 三角形 */
        Rectangle,     /* 矩形 */
        Rotundity,     /* 圆形 */
        Ellipse,       /* 椭圆 */
    } ShapeType_N;

    typedef struct _INFERRESULT_
    {
        ShapeType_N nShapeType;           /* 检测图形的类型 */
        std::vector<cv::Point> vBoxPoints; /* 多边形的各个点 */
        cv::RotatedRect aEllipse;         /* 圆与椭圆的结果 */
    } InferRelust_S;

    class CShapeDetect
    {
    public:
        CShapeDetect();
        ~CShapeDetect();

    public:
        bool inference(cv::Mat &aImage, std::vector<std::vector<cv::Point>> &vApproxPolygons);
        
        bool shapeDetect(cv::Mat &aImage, InferParam_S stInferParam, InferRelust_S &stInferRelust);
    };
} // namespace ShapeDetect_NS
