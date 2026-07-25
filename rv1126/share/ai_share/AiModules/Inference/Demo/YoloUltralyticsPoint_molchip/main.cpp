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

#include "YoloUltralyticsPoint.hpp"


using namespace Inference_NS;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ai_config_path> <image_path>" << std::endl;
        return -1;
    }

    std::string aiConfigPath = argv[1];
    std::string imagePath = argv[2];

    /* 初始化模型 */
    CYoloUltralyticsPoint* demo = new CYoloUltralyticsPoint(aiConfigPath);
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
    
    std::vector<Inference_NS::PointData_S> vPointDatas;
    demo->inference(stInputData, vPointDatas);

    std::cout << "Detected objects: " << vPointDatas.size() << "\n";
    for(int i=0;i<vPointDatas.size();i++)
    {
    	printf("[%d ,%d] ,[%d ,%d] ,%f, %d\n",
            vPointDatas[i].stBoxs.nX1,
            vPointDatas[i].stBoxs.nY1,
            vPointDatas[i].stBoxs.nX2,
            vPointDatas[i].stBoxs.nY2,
            vPointDatas[i].fConfidence,
            vPointDatas[i].nLabel
        );
    	cv::rectangle(image, 
            cv::Point(vPointDatas[i].stBoxs.nX1, vPointDatas[i].stBoxs.nY1), 
            cv::Point(vPointDatas[i].stBoxs.nX2, vPointDatas[i].stBoxs.nY2), 
            cv::Scalar(255,0,0), 2);

        for(int jj=0; jj< vPointDatas[i].vPoints.size(); jj++)
        {
            cv::circle(image, 
                cv::Point(vPointDatas[i].vPoints[jj].nX, vPointDatas[i].vPoints[jj].nY), 
                    3, cv::Scalar(0,125,125), -1);
        }
    }
    cv::imwrite("./YoloUltralyticsPointDemo.jpg", image);

    delete demo;

    return 0;
}
