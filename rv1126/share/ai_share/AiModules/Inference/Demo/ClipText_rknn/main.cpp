/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-29
 * 
 * @brief 
 */


#include "NLPInferenceRK.hpp"

#include <iostream>
#include <chrono>

#include "TextFeature.hpp"

using namespace Inference_NS;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <rk_model_path> <prompt>" << std::endl;
        return -1;
    }

    std::string rkModelPath = argv[1];
    std::string strText = argv[2];
    
    /* 初始化模型 */
    CTextFeature* demo = new CTextFeature(rkModelPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

    Inference_NS::InputData_S stInputData;
    stInputData.strText = strText;

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bool result = demo->inference(stInputData,vClsDatas);

    printf("文本特征向量微为:");
    for(int i=0;i<vClsDatas[0].vFeature.size();i++)
    {
        printf("%f ",vClsDatas[0].vFeature[i]);
    }
    printf("\n");

    delete demo;

    return 0;
}
