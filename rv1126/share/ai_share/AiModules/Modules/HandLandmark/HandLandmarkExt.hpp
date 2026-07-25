/**
 * @file HandLandmark.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-05-21
 *
 * @brief
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace HandLandmark_NS
{
    /* 分析参数 */
    typedef struct _AnalyseParam_
    {
        float fBoxThreshold = 0.8; /* 框的置信度阈值 */
        float fNmsThreshold = -1;  /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strDetectModelPath;   /* 人脸检测模型路径 */
        std::string strLandmarkModelPath; /* 人脸关键点检测模型路径 */

        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;

    /* 输入数据 */
    typedef struct _InData_
    {
        int            nChnId;  /* 通道 */
        cv::Mat        inMat;   /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* 检测结果 */
    typedef struct _OutData_
    {
        float                           fX1         = 0; /* 左上角x坐标 */
        float                           fY1         = 0; /* 左上角y坐标 */
        float                           fX2         = 0; /* 右下角x坐标 */
        float                           fY2         = 0; /* 右下角y坐标 */
        float                           fConfidence = 0; /* 置信度 */
        int                             nLabel      = 0; /* 标签 */
        std::vector<std::vector<float>> vvLandmarks;

        // 打印函数
        void print() const
        {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "Box: (" << fX1 << ", " << fY1
                      << ") - (" << fX2 << ", " << fY2 << ")\n";
            std::cout << "Confidence: " << fConfidence << "\n";
            std::cout << "Label: " << nLabel << "\n";

            if (!vvLandmarks.empty())
            {
                std::cout << "Landmarks: \n";
                for (size_t i = 0; i < vvLandmarks.size(); ++i)
                {
                    std::cout << "  [" << i << "]: ";
                    for (size_t j = 0; j < vvLandmarks[i].size(); ++j)
                    {
                        std::cout << vvLandmarks[i][j];
                        if (j + 1 < vvLandmarks[i].size())
                        {
                            std::cout << ", ";
                        }
                    }
                    std::cout << "\n";
                }
            }
            else
            {
                std::cout << "Landmarks: (none)\n";
            }
            std::cout << "-----------------------------\n";
        }

    } OutData_S;

    /* =========================================================================== */

}    // namespace HandLandmark_NS