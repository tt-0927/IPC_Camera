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
#include "Yolov5.hpp"

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
    CYolov5 *demo = new CYolov5(strModelPath);
    /* 模型初始化 */
    demo->init();

    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);

    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

    for(int Ti=0; Ti<10; Ti++)
    {
        cTime.start();
        Inference_NS::InputData_S stInputData;
        stInputData.pData = (float*)image.data;
        stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

        std::vector<Inference_NS::BoxData_S> vBoxDatas;
        demo->inference(stInputData, vBoxDatas);
        cTime.stop();
        cTime.print("模型推理");
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
