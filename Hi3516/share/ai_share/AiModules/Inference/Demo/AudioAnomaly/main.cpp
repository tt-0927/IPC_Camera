/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 16:32:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 21:01:29
 * @Description  :
 */
#include <chrono>

#include "AudioAnomaly.hpp"

using namespace Inference_NS;

int main()
{
    /* 初始化攀爬检测 */
    cAudioAnomaly* demo = new cAudioAnomaly();

    delete demo;

    return 0;
}
