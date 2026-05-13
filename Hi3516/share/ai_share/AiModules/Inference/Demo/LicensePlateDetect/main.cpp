/*
 * @FilePath     : main.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-08 16:29:52
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-10-08 15:51:28
 * @Description  :
 */

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <chrono>

#include "LicensePlateDetect.hpp"


using namespace Inference_NS;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <rk_model_path> <image_path>" << std::endl;
        return -1;
    }

    std::string rkModelPath = argv[1];
    std::string imagePath = argv[2];

    /* 初始化模型 */
    CLicensePlateDetect* demo = new CLicensePlateDetect(rkModelPath);
        bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

    cv::Mat image = cv::imread(imagePath);
	cv::resize(image,image,cv::Size(640,640));

    cv::Mat rgbImage;
    cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);

    std::vector<float> vOutData;
    bool result = demo->inference(rgbImage,vOutData);

    for(int i=0;i<vOutData.size()/6;i++)
    {
    	printf("%f ,%f ,%f ,%f ,%f ,%f\n",vOutData[i*14+0],vOutData[i*14+1],vOutData[i*14+2],vOutData[i*14+3],vOutData[i*14+4],vOutData[i*14+5]);
    	cv::rectangle(image, cv::Point(vOutData[i*14+0],vOutData[i*14+1]), cv::Point(vOutData[i*14+2],vOutData[i*14+3]), cv::Scalar(255,0,0), 2);
    }
    cv::imwrite("./LicensePlateDetectDemo.jpg", image);

    delete demo;

    return 0;
}
