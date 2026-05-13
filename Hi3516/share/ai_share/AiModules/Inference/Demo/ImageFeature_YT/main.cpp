/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 *
 * @brief
 */
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "ImageFeature.hpp"

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
    std::string strModelPath = argv[1];
    CImageFeature *demo = new CImageFeature(strModelPath);
    /* 模型初始化 */
    demo->init();
    int nWidth = 0;
    int nHeight = 0;
    int nChannel = 0;
    demo->getSizeLimit(0, nWidth, nHeight, nChannel);

    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);
    if (image.empty())
    {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

    cv::resize(image, image, cv::Size(nWidth, nHeight));
    cv::Mat vInputData = whc2chw(image);

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    for (int Ti = 0; Ti < 10; Ti++)
    {
        cTime.start();
        Inference_NS::InputData_S stInputData;
        stInputData.pData = (float *)vInputData.data;
        stInputData.nDataSize = static_cast<int>(vInputData.total() * vInputData.elemSize()) * sizeof(float);

        vClsDatas.clear();
        demo->inference(stInputData, vClsDatas);
        cTime.stop();
        cTime.print("模型推理");
    }

    /* 打印输出数据 */
    std::cout << "Detected objects: " << vClsDatas.size() << "\n";
    for (int i = 0; i < vClsDatas.size(); i++)
    {
        printf("[%d]: 元素个数[%d]\n", i, vClsDatas[i].vFeature.size());
        for(int j=0;j<vClsDatas[i].vFeature.size();j++)
        {
            printf("%f ", vClsDatas[i].vFeature[j]);
        }
        printf("\n");
    }
    /* 显示图像 */
    delete demo;
    return 0;
}
