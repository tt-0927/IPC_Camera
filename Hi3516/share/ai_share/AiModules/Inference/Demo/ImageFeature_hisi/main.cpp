/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-29
 * 
 * @brief 
 */

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceHISI.hpp"

#include <iostream>
#include <chrono>

#include "ImageFeature.hpp"

#include <cstring>
#include <sys/stat.h>

using namespace Inference_NS;

bool ReadBinFileToVector(const std::string& fileName, std::vector<float>& planar_data) {
    struct stat sBuf;
    if (stat(fileName.c_str(), &sBuf) != 0) {
        std::cerr << "Failed to get file status: " << fileName << std::endl;
        return false;
    }

    if (!S_ISREG(sBuf.st_mode)) {
        std::cerr << "Provided path is not a regular file: " << fileName << std::endl;
        return false;
    }

    std::ifstream binFile(fileName, std::ios::binary);
    if (!binFile.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return false;
    }

    // 获取文件大小
    binFile.seekg(0, std::ios::end);
    size_t fileSize = binFile.tellg();
    binFile.seekg(0, std::ios::beg);

    if (fileSize != 3 * 160 * 160) {
        std::cerr << "Unexpected bin file size: " << fileSize << " bytes (expected: " << 3 * 160 * 160 << " bytes)" << std::endl;
        return false;
    }

    // 分配 planar_data 容器大小
    planar_data.resize(fileSize);

    // 一次性读取
    binFile.read(reinterpret_cast<char*>(planar_data.data()), fileSize);
    if (!binFile) {
        std::cerr << "Failed to read expected amount of data from bin file" << std::endl;
        return false;
    }

    binFile.close();
    return true;
}


int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <config_path> <image_path>" << std::endl;
        return -1;
    }

    std::string rkModelPath = argv[1];
    std::string imagePath = argv[2];
    
    /* 初始化模型 */
    CImageFeature* demo = new CImageFeature(rkModelPath);
    bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}

	std::vector<float> planar_data;
    std::string image_bin_path = argv[2];

    if (!ReadBinFileToVector(image_bin_path, planar_data)) {
        std::cerr << "无法将 bin 文件读入 planar_data" << std::endl;
        return -1;
    }

    Inference_NS::InputData_S stInputData;
    stInputData.pData = planar_data.data();
    stInputData.nDataSize = static_cast<int>(planar_data.size()) * sizeof(float);

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bool result = demo->inference(stInputData,vClsDatas);

    printf("图片特征向量为:");
    for(int i=0;i<vClsDatas[0].vFeature.size();i++)
    {
        printf("%f ",vClsDatas[0].vFeature[i]);
    }
    printf("\n");

    delete demo;

    return 0;
}
