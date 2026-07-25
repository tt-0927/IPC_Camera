/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 16:32:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 10:22:18
 * @Description  :
 */

#include <chrono>

#include "VideoAnomalyDetectV1_0.hpp"


using namespace VideoAnomalyDetect_NS;

int main()
{
    InParam_S stInParam;

    /* 初始化攀爬检测 */
    CVideoAnomalyDetectV1_0* demo = new CVideoAnomalyDetectV1_0(stInParam);

    /* 初始化 */
    demo->init();

    VideoAnomalyDetect_NS::Result_S vecResult;
    VideoAnomalyDetect_NS::InData_S stInData;
    stInData.inMat = cv::imread("test.jpg");
    stInData.stParam.stBlurrinessParam.bEnable = true;
    stInData.stParam.stLightDarkParam.bEnable = true;
    stInData.stParam.stNoiseParam.bEnable = true;
    stInData.stParam.stStripesParam.bEnable = true;

    /* 推理 */
    demo->process(stInData, vecResult);

    /* 反初始化 */
    demo->unInit();

    delete demo;

    return 0;
}
