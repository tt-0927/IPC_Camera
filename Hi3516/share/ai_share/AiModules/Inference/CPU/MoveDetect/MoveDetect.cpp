#include "MoveDetect.hpp"

Inference_NS::CMoveDetect::CMoveDetect()
{
    erode_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(erode_size * 2 + 1, erode_size * 2 + 1));
    dilate_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(dilate_size * 2 + 1, dilate_size * 2 + 1));
}

Inference_NS::CMoveDetect::~CMoveDetect()
{
}

bool Inference_NS::CMoveDetect::inference(cv::Mat &frontMat, cv::Mat &afterMat, std::vector<std::vector<int>> &output)
{
    cv::Mat frontGray, afterGray, diffGray;

    // 灰度处理
    cv::cvtColor(frontMat, frontGray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(afterMat, afterGray, cv::COLOR_BGR2GRAY);

    // 帧差处理 找到帧与帧之间运动的物体差异
    cv::absdiff(frontGray, afterGray, diffGray);

    // 二值化：黑白分明 会产生大量白色噪点
    cv::threshold(diffGray, diffGray, 25, 255, cv::THRESH_BINARY);

    // 腐蚀处理：去除白色噪点 噪点不能完全去除，反而主要物体会被腐蚀的图案都变得不明显
    cv::erode(diffGray, diffGray, erode_kernel);

    // 膨胀处理：将白色区域变“胖”
    cv::dilate(diffGray, diffGray, dilate_kernel);

    // 动态物体标记
    std::vector<std::vector<cv::Point>> contours; // 保存关键点
    findContours(diffGray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

    // 提取关键点
    std::vector<std::vector<cv::Point>> contours_poly(contours.size());
    std::vector<cv::Rect> boundRect(contours.size());

    int num = contours.size();
    // printf("=================这帧有%d个框=================",num);
    for (int i = 0; i < num; i++)
    {
        approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));

        std::vector<int> box;
        box.push_back(boundRect[i].x);
        box.push_back(boundRect[i].y);
        box.push_back(boundRect[i].width);
        box.push_back(boundRect[i].height);
        output.push_back(box);
        
    }
    return true;
}

void Inference_NS::CMoveDetect::setParam(int change_erode_size=1, int change_dilate_size=1)
{
    if (change_erode_size<0)
    {
        printf("参数设置失败：erode_szie[%d]不能为小于0\n",change_erode_size);
        return;
    }
    if (change_dilate_size<0)
    {
        printf("参数设置失败：dilate_size[%d]不能为小于0\n",change_dilate_size);
        return;
    }
    if (change_erode_size !=  erode_size)
    {
        erode_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(change_erode_size * 2 + 1, change_erode_size * 2 + 1));
        erode_size = change_erode_size;
    }
    if (change_dilate_size !=  dilate_size)
    {
        dilate_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(change_dilate_size * 2 + 1, change_dilate_size * 2 + 1));
        dilate_size = change_dilate_size;
    }
}