/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-05
 *
 * @brief 车辆属性性能测试
 */
#include <chrono>
#include <cmath>
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceMOL.hpp"
#include "Attribute.hpp"

using namespace Inference_NS;

int main(int argc, char *argv[])
{
    // 检查是否有足够的参数
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << "<aiConfigPath>  <LabelPath>" << std::endl;
        return 1;
    }

    /* 初始化 */
    CAttribute *demo = new CAttribute(argv[1]);
    bool bT = demo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }
    
    /* 读取数据 */
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    /* bgr 转为 yuv420 */
    cv::Mat yuvImage;
    cv::cvtColor(image, yuvImage, cv::COLOR_BGR2YUV_I420);

    /* 维度转换 */
    Inference_NS::InputData_S stInputData;
    /* 获取图像大小 */
    stInputData.nDataSize = static_cast<size_t>(yuvImage.total() * yuvImage.elemSize());

    /* 分配内存并复制图像数据 */
    stInputData.pData = (float*)yuvImage.data;

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    demo->inference(stInputData, vClsDatas);
    for(int i=0;i<vClsDatas.size();i++)
    {
        for(int j=0; j<vClsDatas[i].vCls.size();j++)
        {
            printf("nLabel[%d] - fConfidence[%f]\n",vClsDatas[i].vCls[j].nLabel, vClsDatas[i].vCls[j].fConfidence);
        }
    }

    delete demo;
    return 0;
}
