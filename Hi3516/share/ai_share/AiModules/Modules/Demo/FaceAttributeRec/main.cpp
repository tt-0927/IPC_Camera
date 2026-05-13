/**
 * @file main.cpp
 * @author CaiShengJie (Caisj@kfb.cn)
 * @date 2024-10-10
 *
 * @brief
 */
#include <iostream>
#include <chrono>

#include "FaceAttributeV1_0.hpp"
#include "opencv2/opencv.hpp"

using namespace FaceAttribute_NS;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <rk_model_path> <image_path>" << std::endl;
        return -1;
    }

    std::string rkModelPath = argv[1];
    std::string imagePath = argv[2];

    InParam_S stInParam;
    stInParam.strModelPath = rkModelPath;

    /* 初始化行人属性识别 */
    CFaceAttributeV1_0 *demo = new CFaceAttributeV1_0(stInParam);

    demo->init();
    InData_S stInData;

    stInData.inMat = cv::imread(imagePath);
    std::vector<Result_S> vecResult;

    auto start = std::chrono::high_resolution_clock::now();
    demo->process(stInData, vecResult);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> duration = end - start;
    std::cout << "推理时间: " << duration.count() << " ms" << std::endl;

    for (size_t i = 0; i < vecResult.size(); i++)
    {
        printf("属性：%s, 置信度: %.2f!\n", vecResult[i].strName, vecResult[i].fConfidence);
    }

    delete demo;

    return 0;
}
