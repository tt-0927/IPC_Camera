/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-29
 * 
 * @brief 
 */

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK.hpp"

#include <iostream>
#include <chrono>

#include "ImageFeature.hpp"

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
    CImageFeature* demo = new CImageFeature(rkModelPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

	cv::Mat image = cv::imread(imagePath);
	cv::resize(image,image,cv::Size(224,224));

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)image.data;
    stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bool result = demo->inference(stInputData,vClsDatas);

    printf("图片特征向量微为:");
    for(int i=0;i<vClsDatas[0].vFeature.size();i++)
    {
        printf("%f ",vClsDatas[0].vFeature[i]);
    }
    printf("\n");

    delete demo;

    return 0;
}
