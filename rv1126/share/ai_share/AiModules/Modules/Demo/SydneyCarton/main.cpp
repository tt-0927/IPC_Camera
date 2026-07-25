/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 * 
 * @brief 形状检测（只支持多边形检测）
 */
#include <chrono>

#include "SydneyCartonV1_0.hpp"

using namespace SydneyCarton_NS;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image_path>" << std::endl;
        return -1;
    }

    std::string imagePath = argv[1];

    SydneyCarton_NS::InParam_S stInParam;
    /* 初始化车牌识别检测 */
    CSydneyCartonV1_0 *demo = new CSydneyCartonV1_0(stInParam);
    demo->init();

    InData_S stInData;
    stInData.inMat = cv::imread(imagePath);

    /* 推理 */
    int nResult;
    demo->process(stInData, nResult);

    if(nResult==-1)
    {
        printf("出现异常!!\n");
    }
    else
    {   
        printf("检测的结果: [%d]\n", nResult);
    }

    delete demo;
    return 0;
}
