/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 * 
 * @brief 
 */
#include <chrono>
#include <iostream>
#include <fstream>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "Retinaface.hpp"

using namespace Inference_NS;

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <model_path> <image_path>" << std::endl;
        return -1;
    }

    /* 初始化 */
    std::string strModelPath = argv[1];
    CRetinaface *demo = new CRetinaface(strModelPath);
    /* 模型初始化 */
    demo->init();

    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);

    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

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
