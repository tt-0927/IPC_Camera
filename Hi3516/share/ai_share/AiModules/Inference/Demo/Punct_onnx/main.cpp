/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-29
 *
 * @brief
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include <cstdint>
#include <fstream>
#include <cstdint>

#include "CTTransformerPunct.hpp"

using namespace Inference_NS;

int main(int argc, char **argv)
{
    if (argc < 1)
    {
        std::cerr << "Usage: " << argv[0] << " <punctx_config_path>" << std::endl;
        return -1;
    }

    std::string vadPath = argv[1];
    /* 初始化模型 */
    CCTTransformerPunct *PunctDemo = new CCTTransformerPunct(vadPath);
    bool bT = PunctDemo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }

    Inference_NS::InputData_S stInputData;
    Inference_NS::ASRData_S stASRData;
    stInputData.strText = "今天是星期一明天是星期二后天是星期三ai是什么";
    if (!PunctDemo->inference(stInputData, stASRData))
    {
        printf("PunctDemo 推理失败\n");
    }
    printf("标点后的结果: %s\n", stASRData.strText.c_str());

    delete PunctDemo;
    return 0;
}
