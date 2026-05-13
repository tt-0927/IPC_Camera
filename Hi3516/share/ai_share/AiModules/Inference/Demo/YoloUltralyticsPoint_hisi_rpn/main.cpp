/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 * 
 * @brief 
 */
#include <chrono>
#include <iostream>
#include <fstream>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "YoloUltralyticsPoint_rpn.hpp"


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


int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <config_path> <image_path>" << std::endl;
        return -1;
    }

    /* 初始化 */
    std::string strConfigPath = argv[1];
    CYoloUltralyticsPoint *demo = new CYoloUltralyticsPoint(strConfigPath);
    /* 模型初始化 */
    demo->init();

    std::vector<float> planar_data;
    std::string image_bin_path = argv[2];

    if (!ReadBinFileToVector(image_bin_path, planar_data)) {
        std::cerr << "无法将 bin 文件读入 planar_data" << std::endl;
        return -1;
    }

    Inference_NS::InputData_S stInputData;
    stInputData.pData = planar_data.data();
    stInputData.nDataSize = static_cast<int>(planar_data.size()) * sizeof(float);

    std::cerr << "开始推理" << std::endl;
    std::vector<Inference_NS::PointData_S> vPointDatas;
    demo->inference(stInputData, vPointDatas);

    /* 打印输出数据 */
    std::cout << "Detected objects: " << vPointDatas.size() << "\n";
    for(int i=0;i<vPointDatas.size();i++)
    {
    	// printf("[%d ,%d] ,[%d ,%d] ,%f, %d\n",
        //     vPointDatas[i].stBoxs.nX1,
        //     vPointDatas[i].stBoxs.nY1,
        //     vPointDatas[i].stBoxs.nX2,
        //     vPointDatas[i].stBoxs.nY2,
        //     vPointDatas[i].fConfidence,
        //     vPointDatas[i].nLabel
        // );

        std::cout << "Object " << i + 1 << ":\n";
        std::cout << "  Class ID: " << vPointDatas[i].nLabel << "\n";
        std::cout << "  Confidence: " << vPointDatas[i].fConfidence << "\n";
        std::cout << "  Bounding Box: (x1=" << vPointDatas[i].stBoxs.nX1 << ", y1=" << vPointDatas[i].stBoxs.nY1
                  << ", x2=" << vPointDatas[i].stBoxs.nX2 << ", y2=" << vPointDatas[i].stBoxs.nY2 << ")\n";

        for(int jj=0; jj< vPointDatas[i].vPoints.size(); jj++)
        {
            printf("第 %d 个点坐标：[%d ,%d]\n",
            jj,
            vPointDatas[i].vPoints[jj].nX,
            vPointDatas[i].vPoints[jj].nY
            );
        }
    }

    /* 显示图像 */ 
    // cv::imwrite("output.jpg", resized_image);


    delete demo;

    return 0;
}
