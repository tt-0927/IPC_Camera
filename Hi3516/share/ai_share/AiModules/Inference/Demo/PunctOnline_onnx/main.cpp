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

#include "CTTransformerOnlinePunct.hpp"

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
    CCTTransformerOnlinePunct *PunctDemo = new CCTTransformerOnlinePunct(vadPath);
    bool bT = PunctDemo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }

    Inference_NS::InputData_S stInputData;
    Inference_NS::ASRData_S stASRData;
    std::vector<std::string> strIn= {"星期二后","天是星期三","ai是什么","明天是什么后","天是什么"};
    printf("标点后的结果:");
    for (auto in : strIn)
    {
        stInputData.strText = std::string(in);
        if (!PunctDemo->inference(stInputData, stASRData))
        {
            printf("PunctDemo 推理失败\n");
        }
        printf("%s", stASRData.strText.c_str());
    }
    std::string endStr = PunctDemo->clearArrCache();
    printf("%s\n", endStr.c_str());

    std::vector<std::string> strIn1= {"星期二后","天是星期三","ai是什么","明天是什么后","天是什么"};
    printf("标点后的结果:");
    for (auto in : strIn1)
    {
        stInputData.strText = std::string(in);
        if (!PunctDemo->inference(stInputData, stASRData))
        {
            printf("PunctDemo 推理失败\n");
        }
        printf("%s", stASRData.strText.c_str());
    }
    std::string endStr1 = PunctDemo->clearArrCache();
    printf("%s\n", endStr1.c_str());

    delete PunctDemo;
    return 0;
}
