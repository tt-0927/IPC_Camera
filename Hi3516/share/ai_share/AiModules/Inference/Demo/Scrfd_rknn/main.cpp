/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-01
 * 
 * @brief 
 */
#include <chrono>
#include <iostream>
#include <fstream>
#include <chrono>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "Scrfd.hpp"

using namespace Inference_NS;

std::chrono::high_resolution_clock::time_point startTime;
/* 开始计时函数 */
void startTimer() 
{
    startTime = std::chrono::high_resolution_clock::now();
}

/* 打印程序运行时间 */
void printElapsedTime(const std::string& message) 
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << message << ": " << duration.count() << " ms" << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <model_path> <image_path>" << std::endl;
        return -1;
    }

    /* 初始化 */
    std::string strModelPath = argv[1];
    CScrfd *demo = new CScrfd(strModelPath);
    /* 模型初始化 */
    demo->init();

    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);
    cv::Mat aImg = image.clone();
    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }
    int nImgWidth = image.cols;  
    int nImgHeight = image.rows; 

    int nWidth=1;
    int nHeight=1;
    int nChannel=3;
    demo->getSizeLimit(0,nWidth,nHeight,nChannel);
    cv::resize(image,image,cv::Size(nWidth,nHeight));

    float fW = nImgWidth*1.0/nWidth;
    float fH = nImgHeight*1.0/nHeight;

    startTimer();
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)image.data;
    stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

    std::vector<Inference_NS::PointData_S> vPointDatas;
    demo->inference(stInputData, vPointDatas);

    printElapsedTime("模型推理耗时");
    
    /* 打印输出数据 */
    std::cout << "Detected objects: " << vPointDatas.size() << "\n";
    for (int i = 0; i < vPointDatas.size(); i++)
    {
        int x1 = vPointDatas[i].stBoxs.nX1 * fW;
        int y1 = vPointDatas[i].stBoxs.nY1 * fH;
        int x2 = vPointDatas[i].stBoxs.nX2 * fW;
        int y2 = vPointDatas[i].stBoxs.nY2 * fH;
        std::cout << "Object " << i + 1 << ":\n";
        std::cout << "  Class ID: " << vPointDatas[i].nLabel << "\n";
        std::cout << "  Confidence: " << vPointDatas[i].fConfidence << "\n";
        std::cout << "  Box: (x1=" << x1 << ", y1=" << y1
                << ", x2=" << x2 << ", y2=" << y2 << ")\n";
        cv::rectangle(aImg, cv::Rect(x1, y1, x2-x1, y2-y1), cv::Scalar(0, 255, 0), 2);

        for(int j=0;j<vPointDatas[i].vPoints.size();j++)
        {
            int x = vPointDatas[i].vPoints[j].nX * fW;
            int y = vPointDatas[i].vPoints[j].nY * fH;
            std::cout << "  Points" << j << ": (x=" << x << ", y=" << y << ")\n";
            cv::circle(aImg, cv::Point(x,y), 3, cv::Scalar(0, 0, 255), -1);
        }
    }

    /* 保存结果 */
    cv::imwrite("output.jpg", aImg);

    delete demo;

    return 0;
}
