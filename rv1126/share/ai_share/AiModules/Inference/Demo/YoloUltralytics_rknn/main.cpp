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
#include "YoloUltralytics.hpp"

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
    CYoloUltralytics *demo = new CYoloUltralytics(strModelPath);
    /* 模型初始化 */
    demo->init();

    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);

    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)image.data;
    stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    demo->inference(stInputData, vBoxDatas);

    /* 打印输出数据 */
    std::cout << "Detected objects: " << vBoxDatas.size() << "\n";
    for (int i = 0; i < vBoxDatas.size(); i++)
    {
        std::cout << "Object " << i + 1 << ":\n";
        std::cout << "  Class ID: " << vBoxDatas[i].nLabel << "\n";
        std::cout << "  Confidence: " << vBoxDatas[i].fConfidence << "\n";
        std::cout << "  Bounding Box: (x1=" << vBoxDatas[i].stBoxs.nX1 << ", y1=" << vBoxDatas[i].stBoxs.nY1
                  << ", x2=" << vBoxDatas[i].stBoxs.nX2 << ", y2=" << vBoxDatas[i].stBoxs.nY2 << ")\n";
        
        cv::rectangle(image, cv::Point(vBoxDatas[i].stBoxs.nX1, vBoxDatas[i].stBoxs.nY1), cv::Point(vBoxDatas[i].stBoxs.nX2, vBoxDatas[i].stBoxs.nY2), cv::Scalar(255,0,0), 2);
    }

    /* 显示图像 */ 
    cv::imwrite("output.jpg", image);


    delete demo;

    return 0;
}
