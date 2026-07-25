#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include "rk_compare_image.h"

float compareImage_Path(char* strImagePath1, char* strImagePath2) {
    // 加载图像
    cv::Mat aImage1 = cv::imread(strImagePath1);
    cv::Mat aImage2 = cv::imread(strImagePath2);
    // 检查图像是否成功加载
    if (aImage1.empty() || aImage2.empty()) {
        std::cout << "无法加载图像" << std::endl;
        exit(0);
    }
    // 将图像转换为灰度图像
    cv::Mat aGrayImage1, aGrayImage2;
    cv::cvtColor(aImage1, aGrayImage1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(aImage2, aGrayImage2, cv::COLOR_BGR2GRAY);
    // 比较图像
    cv::Mat aDifference;
    cv::absdiff(aGrayImage1, aGrayImage2, aDifference);
    // 统计非零像素的数量
    int nDiffPixels = cv::countNonZero(aDifference);
    int nTotalPixels = aImage1.rows * aImage1.cols;
    // 判断结果
    float fResult = static_cast<float>(nDiffPixels) / nTotalPixels;
    return fResult;
}


float compareImage_Image_Threshold(CompareImageParam_S stCompareParam) {
    int nLength1=stCompareParam.nLength1;
    int nLength2=stCompareParam.nLength2;
    int nWidth = stCompareParam.nWidth;
    int nHeight= stCompareParam.nHeight;
    int nThreshold =stCompareParam.nThreshold;
    
    // 检查图像是否符合输入
    if (nLength1!=nWidth*nHeight*3 || nLength2!=nWidth*nHeight*3) {
        std::cout << "视频流的长度与输入的尺寸不符" << std::endl;
        exit(0);
    }
    /* 图像预处理  BGR转为RGB */
    cv::Mat aImage1(nHeight, nWidth, CV_8UC3, (void*)stCompareParam.pImage1Buffer);
    cv::Mat aImage2(nHeight, nWidth, CV_8UC3, (void*)stCompareParam.pImage2Buffer);
    
    // 将图像转换为灰度图像
    cv::Mat aGrayImage1, aGrayImage2;
    cv::cvtColor(aImage1, aGrayImage1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(aImage2, aGrayImage2, cv::COLOR_BGR2GRAY);
    // 比较图像
    cv::Mat aDifference;
    cv::absdiff(aGrayImage1, aGrayImage2, aDifference);
    // 设置差异大于阈值的元素为0
    cv::threshold(aDifference, aDifference, nThreshold, 0, cv::THRESH_TOZERO);
    // 统计非零像素的数量
    int nDiffPixels = cv::countNonZero(aDifference);
    int nTotalPixels = aImage1.rows * aImage1.cols;
    // 判断结果
    float fResult = static_cast<float>(nDiffPixels) / nTotalPixels;
    return fResult;
}
