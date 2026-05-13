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

#include <opencv2/dnn.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "YoloUltralytics.hpp"

using namespace Inference_NS;

bool PreProcess(
    cv::Mat aInput,
    cv::Mat& aOutput,
    std::vector<float> vfMean,
    std::vector<float> vfStd,
    bool bRgb
)
{
    try 
    {
        /* 各通道的归一化倍数是否一样 */
        bool bNormal = std::adjacent_find(vfStd.begin(), vfStd.end(), std::not_equal_to<>()) == vfStd.end();
        /* 归一化 */
        if(!bNormal)
        {
            /* 1. 分离三个通道 */
            std::vector<cv::Mat> aChannels;
            cv::split(aInput, aChannels); 
            /* 2. 对每个通道分别进行缩放 */ 
            for(int nC=0; nC<aChannels.size(); nC++)
            {
                cv::multiply(aChannels[nC], 1.0 / vfStd[nC], aChannels[nC]); 
            }

            /* 3. 合并通道 */ 
            cv::merge(aChannels, aInput);
        }

        /* whc转为chw */
        /* 设置目标尺寸 */
        cv::Size stTargetSize = cv::Size();

        /* 设置方差 */
        float fSC = bNormal? (1.0/vfStd[0]) : 1.0;
        /* 设置均值 */
        cv::Scalar stvfMean = cv::Scalar(0, 0, 0);
        if(vfMean.size()==1)
        {
            stvfMean = cv::Scalar(vfMean[0]);
        }
        else if(vfMean.size()==3)
        {
            stvfMean = cv::Scalar(vfMean[0],vfMean[1],vfMean[2]);
        }

        /* 创建 4D blob，适用于神经网络输入 */
        aOutput = cv::dnn::blobFromImage(
            aInput,           /* 输入图像 */
            fSC,             /* 缩放因子 */
            stTargetSize,    /* 目标尺寸 */
            stvfMean,        /* 均值（减去） */
            bRgb,            /* 是否交换 BGR 和 RGB 通道 */
            false,           /* 是否裁剪图像 */
            CV_32F           /* 输出数据类型 */
        );
    }
    catch (const std::exception& e) 
    {
        std::cerr << "OpencvPreProcess处理报错： " << e.what() << std::endl;
        return false;
    }

     return true;
 }
 

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <config_path> <image_path>" << std::endl;
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

    
    /* 预处理 */
    cv::Mat aOutput;
    std::vector<float> vfMean = {0, 0, 0};
    std::vector<float> vfStd = {255.0, 255.0, 255.0};
    PreProcess(
        image,
        aOutput,
        vfMean,
        vfStd,
        true);

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)aOutput.data;
    stInputData.nDataSize = static_cast<int>(aOutput.total() * aOutput.elemSize());

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
