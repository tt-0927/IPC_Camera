/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 16:32:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 21:01:29
 * @Description  :
 */

// #include "opencv2/core/core.hpp"
// #include "opencv2/imgcodecs.hpp"
// #include "opencv2/imgproc.hpp"
#include <chrono>

#include "BYTETracker.h"


using namespace Inference_NS;

int main()
{
    /* 初始化攀爬检测 */
    cBYTETracker* demo = new cBYTETracker();

    delete demo;

    return 0;
}
