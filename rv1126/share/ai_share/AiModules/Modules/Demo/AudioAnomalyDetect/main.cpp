/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 16:32:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 10:22:18
 * @Description  :
 */

#include <chrono>

#include "AudioAnomalyDetectV1_0.hpp"


using namespace AudioAnomalyDetect_NS;

int main()
{
    InParam_S stInParam;

    /* 初始化攀爬检测 */
    CAudioAnomalyDetectV1_0* demo = new CAudioAnomalyDetectV1_0(stInParam);

    /* 初始化 */
    demo->init();
    /* 反初始化 */
    demo->unInit();

    delete demo;

    return 0;
}
