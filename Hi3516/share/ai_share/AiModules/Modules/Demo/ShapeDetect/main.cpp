/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 *
 * @brief 形状检测
 */
#include <chrono>

#include "ShapeDetectV1_0.hpp"

using namespace ShapeDetect_NS;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image_path>" << std::endl;
        return -1;
    }

    std::string imagePath = argv[1];

    ShapeDetect_NS::InParam_S stInParam;
    /* 初始化车牌识别检测 */
    CShapeDetectV1_0 *demo = new CShapeDetectV1_0(stInParam);
    demo->init();

    InData_S stInData;
    stInData.inMat = cv::imread(imagePath);
    cv::Mat imShow = stInData.inMat.clone();
    /* 推理 */
    Inference_NS::InferRelust_S stRelusts;
    demo->process(stInData, stRelusts);

    switch (stRelusts.nShapeType)
    {
    case Inference_NS::NullShape:
        printf("未识别到相关的图形\n");
        break;
    case Inference_NS::Triangle:
        printf("识别到三角形\n");
        cv::polylines(imShow, {stRelusts.vBoxPoints}, true, cv::Scalar(255, 0, 0), 2);
        break;
    case Inference_NS::Rectangle:
        printf("识别到矩形\n");
        cv::polylines(imShow, {stRelusts.vBoxPoints}, true, cv::Scalar(255, 0, 0), 2);
        break;
    case Inference_NS::Rotundity:
        printf("识别到圆形\n");
        cv::circle(imShow, stRelusts.aEllipse.center, static_cast<int>(stRelusts.aEllipse.size.width / 2), cv::Scalar(255, 0, 0), 2);
        break;
    case Inference_NS::Ellipse:
        printf("识别到椭圆\n");
        cv::ellipse(imShow, stRelusts.aEllipse, cv::Scalar(0, 255, 0), 2);
        break;
    default:
        break;
    }

    cv::imwrite("result.jpg", imShow);
    delete demo;
    return 0;
}
