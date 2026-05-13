/*
 * @FilePath     : main.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-09 16:32:26
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-10-09 10:22:18
 * @Description  :
 */

#include <chrono>

#include "LicensePlateCognitionV1_0.hpp"

using namespace LicensePlateCognition_NS;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image_path>" << std::endl;
        return -1;
    }

    std::string imagePath = argv[1];

    InParam_S stInParam;
    stInParam.strModelPath1 = "./LicensePlateKeyPoint.rknn";
    stInParam.strModelPath2 = "./LicensePlateRec.rknn";
    /* 初始化车牌识别检测 */
    CLicensePlateCognitionV1_0* demo = new CLicensePlateCognitionV1_0(stInParam);
    demo->init();

    cv::Mat image = cv::imread(imagePath);
	// cv::resize(image,image,cv::Size(640,640));
    cv::Mat rgbimage;
    cv::cvtColor(image, rgbimage, cv::COLOR_BGR2RGB);

    std::vector<Result_S> vOutData;
    LicensePlateCognition_NS::InData_S stInData;
    stInData.inMat = rgbimage;
    bool result = demo->process(stInData,vOutData);

    for(int i=0;i<vOutData.size();i++)
    {
        cv::rectangle(image,
                      cv::Point(vOutData[i].fX,vOutData[i].fY),
                      cv::Point(vOutData[i].fX+vOutData[i].fWidth,vOutData[i].fY+vOutData[i].fHeight),
                      cv::Scalar(255,0,0), 2);

        std::cerr << "车牌号: " << vOutData[i].licensePlateNumber << std::endl;
        std::cerr << "车牌颜色: " << vOutData[i].licensePlateColor << std::endl;
        std::cerr << "车牌类型: " << vOutData[i].licensePlateType << std::endl;

    }
    cv::imwrite("./LicensePlateCognitionDemo.jpg", image);

    delete demo;

    return 0;
}
