/*
 * @FilePath     : main.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-09 16:32:26
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-10-09 10:22:18
 * @Description  :
 */

#include <chrono>

#include "NumberOcrV1_0.hpp"

using namespace NumberOcr_NS;

int main(int argc, char** argv)
{
    InParam_S stInParam;
    stInParam.strModelPath = "./111.rknn";

    /* 初始化车牌识别检测 */
    CNumberOcrV1_0* demo = new CNumberOcrV1_0(stInParam);

    delete demo;

    return 0;
}
