/*
*  File Name:main.cpp
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :AI场景调用例子 	
*  Modify date: 
*/

#include "AIScenario.hpp"
#include "HumanCutout.hpp"
#include "Factory.hpp"
#include <opencv2/opencv.hpp>
int main() {

    /* 读取图像文件 */
    cv::Mat image = cv::imread("/home/user/liaoel/code/c_demo/工厂模式/model_methods/demo/2.jpg");
    /* 选择AI场景 */
    AIScenario* scenario = Factory<HumanCutout>::create();
    /* 放入图片进行检测 */
    scenario->detection(image);
    /* 停止这个AI场景 */
    delete scenario;
    
    return 0;
}
