/*
 * @FilePath     : main.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-29 16:32:26
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-09-29 20:09:05
 * @Description  :
 */

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceMOL.hpp"

#include <iostream>
#include <chrono>

#include "LicensePlateRec.hpp"

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
    CLicensePlateRec* demo = new CLicensePlateRec(rkModelPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
    
    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    /* bgr 转为 yuv420 */
    cv::Mat yuvImage;
    cv::cvtColor(image, yuvImage, cv::COLOR_BGR2YUV_I420);

    /* 维度转换 */
    Inference_NS::InputData_S stInputData;
    /* 获取图像大小 */
    stInputData.nDataSize = static_cast<size_t>(yuvImage.total() * yuvImage.elemSize());

    /* 分配内存并复制图像数据 */
    stInputData.pData = (float*)yuvImage.data;

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bool result = demo->inference(stInputData,vClsDatas);

    printf("识别的车牌号的token下标为:");
    for(int i=0;i<vClsDatas[0].vCls.size();i++)
    {
        printf("%d ",vClsDatas[0].vCls[i].nLabel);
    }
    printf("\n");

    printf("识别到车牌的颜色下标nLabel[%d]-fConfidence[%f]\n", vClsDatas[0].stCls.nLabel, vClsDatas[0].stCls.fConfidence);

    delete demo;

    return 0;
}
