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

#include "Yolov5Point.hpp"

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
    CYolov5Point* demo = new CYolov5Point(aiConfigPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

    cv::Mat image = cv::imread(imagePath);
	cv::resize(image,image,cv::Size(640,640));

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)image.data;
    stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

    std::vector<Inference_NS::PointData_S> vPointDatas;
    demo->inference(stInputData, vPointDatas);

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
    cv::imwrite("./Yolov5PointDemo.jpg", image);

    delete demo;

    return 0;
}
