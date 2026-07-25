/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 16:32:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 20:09:05
 * @Description  :
 */
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK.hpp"

#include <iostream>
#include <chrono>
#include "NumberOcr.hpp"

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
    CNumberOcr* demo = new CNumberOcr(rkModelPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

	cv::Mat image = cv::imread(imagePath);
	cv::resize(image,image,cv::Size(128,64));
    cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
    cv::threshold(image, image, 128, 255, cv::THRESH_BINARY);

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)image.data;
    stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bool result = demo->inference(stInputData,vClsDatas);

    printf("识别到数字微[%d]\n", vClsDatas[0].stCls.nLabel);

    delete demo;

    return 0;
}
