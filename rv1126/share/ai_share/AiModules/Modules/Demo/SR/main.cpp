/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-01-09
 * 
 * @brief 
 */
#include <chrono>

#include "SRV1_0.hpp"


using namespace SR_NS;

int main()
{
    SR_NS::InParam_S stInParam;
    stInParam.strModelPath = "./SR.rknn";
    /* 初始化攀爬检测 */
    CSRV1_0* demo = new CSRV1_0(stInParam);

    /* 初始化 */
    demo->init();

    SR_NS::InData_S stInData;
    stInData.inMat = cv::imread("test.jpg");

    /* 推理 */
    cv::Mat aOutput;
    demo->process(stInData, aOutput);
    cv::imwrite("SRDemo.jpg", aOutput);
    
    /* 反初始化 */
    demo->unInit();

    delete demo;

    return 0;
}
