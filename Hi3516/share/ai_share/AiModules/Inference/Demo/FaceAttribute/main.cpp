/*
 * @FilePath     : main.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-21 16:32:26
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-10-21 20:09:05
 * @Description  :
 */

// #include "opencv2/core/core.hpp"
// #include "opencv2/imgcodecs.hpp"
// #include "opencv2/imgproc.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK.hpp"

#include <iostream>
#include <chrono>

#include "FaceAttribute.hpp"

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
    // CFaceAttribute* demo = new CFaceAttribute("./FaceAttribute.rknn");
    CFaceAttribute* demo = new CFaceAttribute(rkModelPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

	cv::Mat image = cv::imread(imagePath);
	cv::resize(image,image,cv::Size(192,192));
    std::vector<Attrbute_S> vOutDatas;
    bool result = demo->inference(image,vOutDatas);

    for (int i = 0; i < vOutDatas.size(); i++)
    {
        std::cerr << "类别名字: " << vOutDatas[i].strName << "，概率：" << vOutDatas[i].fConfidence << std::endl;
    }

    delete demo;

    return 0;
}
