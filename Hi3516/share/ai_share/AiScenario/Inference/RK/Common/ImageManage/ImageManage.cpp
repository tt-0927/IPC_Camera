/*
 * @FilePath     : ImageManage.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-22 09:07:36
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-07-22 09:13:16
 * @Description  :
 */
#include "ImageManage.hpp"

InferenceV1_0_NS::CImageManage::CImageManage()
{
}

InferenceV1_0_NS::CImageManage::~CImageManage()
{
}

/* 裁剪 */
bool InferenceV1_0_NS::CImageManage::cropping(cv::Mat inMat, cv::Rect rect, cv::Mat& outMat)
{
    return false;
}
