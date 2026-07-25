/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-05-08
 *
 * @brief
 */

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/dnn.hpp>

#include <iostream>
#include <chrono>
#include <sys/time.h>

#include "CixPreprocess.hpp"
#include "Yolov5.hpp"


#include <vector>
#include <cmath>

using namespace Inference_NS;

class TVTimer
{
public:
    /* 开始计时 */
    void start()
    {
        gettimeofday(&tv_start, nullptr);
        running = true;
    }

    /* 结束计时 */
    void stop()
    {
        gettimeofday(&tv_end, nullptr);
        running = false;
    }

    /* 打印耗时（毫秒），如果未调用 stop() 则以当前时间为终点 */
    void print(const char *tag = "") const
    {
        timeval tv_now;
        if (running)
        {
            gettimeofday(&tv_now, nullptr);
        }
        else
        {
            tv_now = tv_end;
        }

        long seconds = tv_now.tv_sec - tv_start.tv_sec;
        long useconds = tv_now.tv_usec - tv_start.tv_usec;
        long ms = seconds * 1000 + useconds / 1000;

        if (*tag)
        {
            std::cout << "[" << tag << "] ";
        }
        std::cout << "Elapsed: " << ms << " ms\n";
    }

private:
    struct timeval tv_start
    {
    };
    struct timeval tv_end
    {
    };
    bool running = false;
};

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ai_config_path> <image_path>" << std::endl;
        return -1;
    }

    std::string aiConfigPath = argv[1];
    std::string imagePath = argv[2];

    TVTimer cTime;
    /* 初始化模型 */
    CYolov5 *demo = new CYolov5(aiConfigPath);
    bool bT = demo->init();
    if (!bT)
    {
        printf("初始化参数失败\n");
        exit(0);
    }

    /* 模型初始化 */
    int nWidth = 0;
    int nHeight = 0;
    int nChannel = 0;
    demo->getSizeLimit(0, nWidth, nHeight, nChannel);
    /* 获取Mean Std对象 */
    std::vector<float> vfMean;
    std::vector<float> vfStd;
    demo->getMeanStd(vfMean, vfStd);
    /* 获取预处理的缩放大小和偏移量 */
    float fScale;
    int nZeroPoint;
    noe_data_type_t eDataType;
    demo->getScaleZeroPoint(0, fScale, nZeroPoint, eDataType);
    cv::Mat image = cv::imread(imagePath);
    cv::resize(image, image, cv::Size(nWidth, nHeight));
    /* 预处理 */
    cTime.start();
    cv::Mat aOutput;
    PreProcess_NS::CixPreprocess(
        image,
        aOutput,
        nWidth,
        nHeight,
        vfMean,
        vfStd,
        false,
        fScale,
        nZeroPoint,
        eDataType);
    cTime.print("图片预处理");

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float *)aOutput.data;
    stInputData.nDataSize = static_cast<int>(aOutput.total() * aOutput.elemSize())*sizeof(float);
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    for (int Ti = 0; Ti < 10; Ti++)
    {
        cTime.start();
        vBoxDatas.clear();
        demo->inference(stInputData, vBoxDatas);
        cTime.stop();
        cTime.print("模型推理");
    }

    /* 打印输出数据 */
    std::cout << "Detected objects: " << vBoxDatas.size() << "\n";
    for (int i = 0; i < vBoxDatas.size(); i++)
    {
        std::cout << "Object " << i + 1 << ":\n";
        std::cout << "  Class ID: " << vBoxDatas[i].nLabel << "\n";
        std::cout << "  Confidence: " << vBoxDatas[i].fConfidence << "\n";
        std::cout << "  Bounding Box: (x1=" << vBoxDatas[i].stBoxs.nX1 << ", y1=" << vBoxDatas[i].stBoxs.nY1
                  << ", x2=" << vBoxDatas[i].stBoxs.nX2 << ", y2=" << vBoxDatas[i].stBoxs.nY2 << ")\n";

        cv::rectangle(image, cv::Point(vBoxDatas[i].stBoxs.nX1, vBoxDatas[i].stBoxs.nY1), cv::Point(vBoxDatas[i].stBoxs.nX2, vBoxDatas[i].stBoxs.nY2), cv::Scalar(255, 0, 0), 2);
    }

    cv::imwrite("Yolov5.jpg", image);
    delete demo;

    return 0;
}
