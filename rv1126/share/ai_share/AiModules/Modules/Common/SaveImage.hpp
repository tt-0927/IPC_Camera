/*
 * @FilePath     : SaveImage.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 14:48:51
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 14:49:58
 * @Description  :
 */
#pragma once

#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace Modules_NS
{
    /**
     * @brief 保存图片
     * @param [Mat&] image: 图片
     * @param [string&] strOutputPath: 保存的路径
     * @return [*]
     * @note
     */
    bool saveImage(const cv::Mat& image, const std::string& strOutputPath);
    bool saveImage(const cv::Mat& image, const std::string& strOutputPath, int nChnId, int eventType, std::string& outFileName);
}
// namespace Modules_NS
