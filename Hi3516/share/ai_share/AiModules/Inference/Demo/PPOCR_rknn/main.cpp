/*
 * @FilePath     : main.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-08 16:29:52
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-10-08 15:51:28
 * @Description  :
 */

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <chrono>

#include "PPOcrPoint.hpp"
#include "PPOCRCls.hpp"

using namespace Inference_NS;

std::vector<std::string> readTokenFile(
    const std::string& filePath, 
    bool skipEmptyLines = false,
    bool trimWhitespace = false)
{
    std::vector<std::string> tokens;
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件 " << filePath << std::endl;
        return tokens; // 返回空vector
    }

    std::string line;
    while (std::getline(file, line)) {
        // 去除首尾空白（如果启用）
        if (trimWhitespace) {
            size_t start = line.find_first_not_of(" \t");
            size_t end = line.find_last_not_of(" \t");
            
            if (start == std::string::npos) { // 全空行
                line.clear();
            } else {
                line = line.substr(start, end - start + 1);
            }
        }
        
        // 检查是否添加当前行
        if (!(skipEmptyLines && line.empty())) {
            tokens.push_back(line);
        }
    }
    return tokens;
}

cv::Mat preprocessImage(const cv::Mat& src_img, int model_width, int model_height) {
    // 计算宽高比
    float ratio = src_img.cols / float(src_img.rows);
    int resized_w;

    // 决定缩放后的宽度
    if (std::ceil(model_height * ratio) > model_width) {
        resized_w = model_width;  // 超过模型宽度时限制最大值
    } else {
        resized_w = std::ceil(model_height * ratio);  // 按比例计算宽度
    }

    // 等比缩放
    cv::Mat processed_img;
    cv::resize(src_img, processed_img, cv::Size(resized_w, model_height));

    // 右侧填充0（归一化后的0对应原图的127.5）
    if (resized_w < model_width) {
        int pad_right = model_width - resized_w;
        cv::copyMakeBorder(processed_img, processed_img, 
                          0, 0, 0, pad_right, 
                          cv::BORDER_CONSTANT, 
                          cv::Scalar(0, 0, 0));
    }

    return processed_img;
}

// int main(int argc, char** argv)
// {
//     if (argc < 4)
//     {
//         std::cerr << "Usage: " << argv[0] << " <ai_cls_config_path>  <token_path> <image_path>" << std::endl;
//         return -1;
//     }
//     std::string aiClsConfigPath = argv[1];
//     std::string tokenPath = argv[2];
//     std::string imagePath = argv[3];

//     std::vector<std::string> vToken =  readTokenFile(tokenPath);
//     /* 初始化模型 */
//     CPPOCRCls* demoCls = new CPPOCRCls(aiClsConfigPath);
//     bool bT = demoCls->init();
// 	if(!bT)
// 	{
// 		printf("分类初始化参数识别\n");
// 		exit(0);
// 	}

//     int nWidth; 
//     int nHeight;
//     int nChannel;
//     demoCls->getSizeLimit(0, nWidth, nHeight, nChannel);

//     cv::Mat image = cv::imread(imagePath);
//     image = preprocessImage(image,nWidth,nHeight);
//     // cv::resize(image, image, cv::Size(nWidth,nHeight));

//     Inference_NS::InputData_S stInputClsData;
//     stInputClsData.pData = (float*)image.data;
//     stInputClsData.nDataSize = static_cast<int>(image.total() * image.elemSize()*sizeof(float));

//     std::vector<Inference_NS::ClsData_S> vClsDatas;
//     demoCls->inference(stInputClsData, vClsDatas);

//     std::string res = "";
//     for(int j=0;j<vClsDatas[0].vCls.size(); j++)
//     {
//         printf("[%d] [%f]\n", vClsDatas[0].vCls[j].nLabel, vClsDatas[0].vCls[j].fConfidence);
//         res += vToken[vClsDatas[0].vCls[j].nLabel];
//     }
//     printf("结果为:[%s]\n", res.c_str());
//     delete demoCls;

//     return 0;
// }


int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " <ai_detect_config_path>  <ai_cls_config_path>  <token_path> <image_path>" << std::endl;
        return -1;
    }
    std::string aiDetectConfigPath = argv[1];
    std::string aiClsConfigPath = argv[2];
    std::string tokenPath = argv[3];
    std::string imagePath = argv[4];

    std::vector<std::string> vTokens =  readTokenFile(tokenPath);

    /* 初始化模型 */
    CPPOcrPoint* demo = new CPPOcrPoint(aiDetectConfigPath);
    CPPOCRCls* demoCls = new CPPOCRCls(aiClsConfigPath);
    bool bT = demo->init();
    if(!bT)
	{
		printf("检测初始化参数识别\n");
		exit(0);
	}
    bT = demoCls->init();
	if(!bT)
	{
		printf("分类初始化参数识别\n");
		exit(0);
	}

    cv::Mat image = cv::imread(imagePath);
    cv::Mat imgOld = image.clone();
    cv::Mat imgShow = image.clone();
    int width = imgOld.cols;  
    int height = imgOld.rows;

    int nDetWidth; 
    int nDetHeight;
    int nDetChannel;
    demo->getSizeLimit(0, nDetWidth, nDetHeight, nDetChannel);
    /* 检测框缩放 */
    float fWR = width*1.0 / nDetWidth;
    float fHR = height*1.0 / nDetHeight;

    int nCLSWidth; 
    int nCLSHeight;
    int nCLSChannel;
    demoCls->getSizeLimit(0, nCLSWidth, nCLSHeight, nCLSChannel);

	cv::resize(image,image,cv::Size(nDetWidth,nDetHeight));
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)image.data;
    stInputData.nDataSize = static_cast<int>(image.total() * image.elemSize())* sizeof(float);

    std::vector<Inference_NS::PointData_S> vPointDatas;
    demo->inference(stInputData, vPointDatas);

    for(int i=0;i<vPointDatas.size();i++)
    {
        int nX1 = vPointDatas[i].stBoxs.nX1*fWR;
        int nY1 = vPointDatas[i].stBoxs.nY1*fHR;
        int nX2 = vPointDatas[i].stBoxs.nX2*fWR;
        int nY2 = vPointDatas[i].stBoxs.nY2*fHR;
    	printf("[%d ,%d] ,[%d ,%d] ,%f, %d\n",
            nX1,
            nY1,
            nX2,
            nY2,
            vPointDatas[i].fConfidence,
            vPointDatas[i].nLabel
        );
    	cv::rectangle(imgShow, 
            cv::Point(nX1, nY1), 
            cv::Point(nX2, nY2), 
            cv::Scalar(255,0,0), 2);

        for(int jj=0; jj< vPointDatas[i].vPoints.size(); jj++)
        {
            int nXX = vPointDatas[i].vPoints[jj].nX*fWR;
            int Nyy = vPointDatas[i].vPoints[jj].nY*fHR;
            cv::circle(imgShow, 
                cv::Point(nXX, Nyy), 
                    3, cv::Scalar(0,125,125), -1);
        }

        /* 文字识别 */
        cv::Rect aRoi(nX1, nY1, nX2 - nX1, nY2 - nY1);
        cv::Mat aCroppedImage = imgOld(aRoi).clone();
        aCroppedImage = preprocessImage(aCroppedImage,nCLSWidth,nCLSHeight);

        Inference_NS::InputData_S stInputClsData;
        stInputClsData.pData = (float*)aCroppedImage.data;
        stInputClsData.nDataSize = static_cast<int>(aCroppedImage.total() * aCroppedImage.elemSize()*sizeof(float));

        std::vector<Inference_NS::ClsData_S> vClsDatas;
        demoCls->inference(stInputClsData, vClsDatas);

        std::string strRes = "";
        for(int j=0;j<vClsDatas[0].vCls.size(); j++)
        {
            // printf("[%d] [%f]\n", vClsDatas[0].vCls[j].nLabel, vClsDatas[0].vCls[j].fConfidence);
            strRes += vTokens[vClsDatas[0].vCls[j].nLabel];
        }
        printf("结果为: [%s]\n", strRes.c_str());

    }
    cv::imwrite("./PPOCR.jpg", imgShow);

    delete demo;
    delete demoCls;

    return 0;
}
