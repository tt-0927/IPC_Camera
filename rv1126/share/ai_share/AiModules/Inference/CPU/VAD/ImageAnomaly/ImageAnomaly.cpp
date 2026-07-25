/*
 *  File Name: ImageAnomaly.cpp
 *  Created on: 2024年9月5日
 *  Author: wcp
 *  description : 视频异常分析算法
 *  Modify date: 2024年9月5日
 */

#include "ImageAnomaly.hpp"

#include <algorithm>
#include <chrono>

namespace Inference_NS
{
    /* 构造函数 -- 初始化变量 */
    cImageAnomaly::cImageAnomaly()
    {
    }

    /* 销毁创建的模型 */
    cImageAnomaly::~cImageAnomaly()
    {
    }

    /* 过滤掉检测底背景 */
    void cImageAnomaly::detectRedAnomaly(cv::Mat aFrame, cv::Scalar aLowerColor, cv::Scalar aUpperColor, cv::Mat &aColorMask, cv::Mat &aAnomalyMask, cv::Mat &aAnomalyFrame)
    {
        /* 将图像从BGR转换到HSV颜色空间 */
        cv::Mat hsv;
        cv::cvtColor(aFrame, hsv, cv::COLOR_BGR2HSV);
        /* 创建一个掩码，用于标识红色区域 */
        cv::inRange(hsv, aLowerColor, aUpperColor, aColorMask);
        /* 取反掩码，以标识非红色区域 */
        cv::bitwise_not(aColorMask, aAnomalyMask);
        /* 将掩码应用到帧上，突出显示异常区域 */
        cv::bitwise_and(aFrame, aFrame, aAnomalyFrame, aAnomalyMask);
    }

    /* 前后帧图片做差 */
    void cImageAnomaly::detectRedAnomaly(cv::Mat aFrame, cv::Mat &aColorMask, cv::Mat &aAnomalyMask, cv::Mat &aAnomalyFrame)
    {
        if(aLastFrame.empty())
        {
            aLastFrame = aFrame.clone();
        }
        /* 将图像从BGR转换到HSV颜色空间 */
        cv::Mat frameDiff;
        cv::absdiff(aLastFrame, aFrame, frameDiff);
        cv::cvtColor(frameDiff, frameDiff, cv::COLOR_BGR2GRAY);
        /* 设置阈值以获取变化区域的掩码 */ 
        cv::threshold(frameDiff, aAnomalyMask, 30, 255, cv::THRESH_BINARY); 
        /* 取反掩码，以标识非红色区域 */
        cv::bitwise_not(aAnomalyMask, aColorMask);

        /* 给上一帧赋值 */
        aLastFrame = aFrame.clone();

        /* 将掩码应用到帧上，突出显示异常区域 */
        cv::bitwise_and(aFrame, aFrame, aAnomalyFrame, aAnomalyMask);

    }

    /* 条纹检测算法 */
    void cImageAnomaly::detectStripes(cv::Mat &aAnomalyFrame, std::vector<cv::Vec4i> &vLines)
    {
        /* 使用Canny算法检测边缘 */
        cv::Mat aEdges;
        cv::Canny(aAnomalyFrame, aEdges, 50, 150);
        // cv::cvtColor(aAnomalyFrame, aEdges, cv::COLOR_BGR2GRAY);
        // cv::threshold(aEdges, aEdges, 20, 255, cv::THRESH_BINARY);
        /* 使用霍夫线变换检测直线 */
        cv::HoughLinesP(aEdges, vLines, 1, CV_PI / 180, 80, 100, 10);
    }

    /* 亮暗检测算法 */
    double cImageAnomaly::getLight(cv::Mat &aFrame, cv::Mat &aColorMask)
    {
        /* 计算红色区域的平均亮度 */
        double dAreaBrightness = 0;
        if (cv::countNonZero(aColorMask) > 0)
        {
            cv::Scalar meanValue = cv::mean(aFrame, aColorMask);
            dAreaBrightness = meanValue[0];
        }
        double dRes = dAreaBrightness - dLastLight;
        dLastLight = dAreaBrightness;
        return dRes;
    }

    /* 噪点检测算法 */
    int cImageAnomaly::countNoise(cv::Mat &aAnomalyMask)
    {
        /* 计算非零像素的数量，即噪点的数量 */
        return cv::countNonZero(aAnomalyMask);
    }

    /* 图片模糊度检测 */
    double cImageAnomaly::assessImageSharpness(cv::Mat &aImage)
    {
        /* 将图像从BGR转换为灰度 */
        cv::Mat gray;
        cv::cvtColor(aImage, gray, cv::COLOR_BGR2GRAY);
        /* 计算拉普拉斯方差 */
        cv::Mat laplacian;
        cv::Laplacian(gray, laplacian, CV_64F);

        cv::Scalar mean, stdDev;
        cv::meanStdDev(laplacian, mean, stdDev);
        // 方差是标准差的平方
        double dLaplacianVar = stdDev[0] * stdDev[0];
        return dLaplacianVar;
    }

}; // namespace ImageAnomaly_NS
