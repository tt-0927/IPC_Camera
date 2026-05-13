/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-22
 *
 * @brief
 */
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "YoloUltralyticsPoint.hpp"

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

cv::Mat whc2chw(const cv::Mat &src_mat)
{
    std::vector<cv::Mat> bgr_channels(3);
    cv::split(src_mat, bgr_channels);
    for (size_t i = 0; i < bgr_channels.size(); i++)
    {
        bgr_channels[i] = bgr_channels[i].reshape(1, 1);
    }
    cv::Mat dst_mat;
    cv::hconcat(bgr_channels, dst_mat);
    return dst_mat;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ai_config_path> <image_path>" << std::endl;
        return -1;
    }

    TVTimer cTime;
    /* 初始化 */
    std::string aiConfigPath = argv[1];
    CYoloUltralyticsPoint *demo = new CYoloUltralyticsPoint(aiConfigPath);
    /* 模型初始化 */
    demo->init();

    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);
    if (image.empty())
    {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

    cv::resize(image, image, cv::Size(640, 640));
    cv::Mat vInputData = whc2chw(image);

    std::vector<Inference_NS::PointData_S> vPointDatas;
    for (int Ti = 0; Ti < 10; Ti++)
    {
        cTime.start();
        Inference_NS::InputData_S stInputData;
        stInputData.pData = (float *)vInputData.data;
        stInputData.nDataSize = static_cast<int>(vInputData.total() * vInputData.elemSize()) * sizeof(float);

        vPointDatas.clear();
        demo->inference(stInputData, vPointDatas);
        cTime.stop();
        cTime.print("模型推理");
    }

    /* 打印输出数据 */
    std::cout << "Detected objects: " << vPointDatas.size() << "\n";
    for (int i = 0; i < vPointDatas.size(); i++)
    {
        printf("[%d ,%d] ,[%d ,%d] ,%f, %d\n",
               vPointDatas[i].stBoxs.nX1,
               vPointDatas[i].stBoxs.nY1,
               vPointDatas[i].stBoxs.nX2,
               vPointDatas[i].stBoxs.nY2,
               vPointDatas[i].fConfidence,
               vPointDatas[i].nLabel);
        cv::rectangle(image,
                      cv::Point(vPointDatas[i].stBoxs.nX1, vPointDatas[i].stBoxs.nY1),
                      cv::Point(vPointDatas[i].stBoxs.nX2, vPointDatas[i].stBoxs.nY2),
                      cv::Scalar(255, 0, 0), 2);

        for (int jj = 0; jj < vPointDatas[i].vPoints.size(); jj++)
        {
            cv::circle(image,
                       cv::Point(vPointDatas[i].vPoints[jj].nX, vPointDatas[i].vPoints[jj].nY),
                       3, cv::Scalar(0, 125, 125), -1);
        }
    }
    cv::imwrite("./Yolov5PointDemo.jpg", image);
    delete demo;
    return 0;
}
