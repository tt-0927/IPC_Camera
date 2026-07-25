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
#include "Retinaface.hpp"
#include <sys/time.h>

using namespace Inference_NS;


class TVTimer {
    public:
        /* 开始计时 */
        void start() {
            gettimeofday(&tv_start, nullptr);
            running = true;
        }
    
        /* 结束计时 */
        void stop() {
            gettimeofday(&tv_end, nullptr);
            running = false;
        }
    
        /* 打印耗时（毫秒），如果未调用 stop() 则以当前时间为终点 */
        void print(const char* tag = "") const {
            timeval tv_now;
            if (running) {
                gettimeofday(&tv_now, nullptr);
            } else {
                tv_now = tv_end;
            }
    
            long seconds  = tv_now.tv_sec  - tv_start.tv_sec;
            long useconds = tv_now.tv_usec - tv_start.tv_usec;
            long ms = seconds * 1000 + useconds / 1000;
    
            if (*tag) {
                std::cout << "[" << tag << "] ";
            }
            std::cout << "Elapsed: " << ms << " ms\n";
        }
    
    private:
        struct timeval tv_start{};
        struct timeval tv_end{};
        bool running = false;
};

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
        std::cerr << "Usage: " << argv[0] << " <model_path> <image_path>" << std::endl;
        return -1;
    }

    TVTimer cTime;
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
    cv::resize(image, image, cv::Size(640, 640));

     /* 预处理 */
    cv::Mat aOutput;
    std::vector<float> vfMean = {104, 117, 123};
    std::vector<float> vfStd = {1.0, 1.0, 1.0};
    PreProcess(
        image,
        aOutput,
        vfMean,
        vfStd,
        true
    );

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)aOutput.data;
    stInputData.nDataSize = static_cast<int>(aOutput.total() * aOutput.elemSize());

    std::vector<Inference_NS::PointData_S> vPointDatas;
    for(int Ti=0; Ti<10; Ti++)
    {
        cTime.start();
        vPointDatas.clear();
        demo->inference(stInputData, vPointDatas);
        cTime.stop();
        cTime.print("模型推理");
    }
    printf("===========检测到[%d]个目标============\n",vPointDatas.size());
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
