/*
 * @FilePath     : main.cpp
 * @Author       : leiyy
 * @Date         : 2025-9-22 14:32:26
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-09-22 17:35:40
 * @Description  :
 */


#include <iostream>
#include <chrono>

#include "LLMInferenceRK.hpp"
#include <opencv2/opencv.hpp>
#include "ImageEnoderInferenceRK.hpp"

using namespace Inference_NS;

#define IMAGE_HEIGHT 448
#define IMAGE_WIDTH 448
#define IMAGE_TOKEN_NUM 256
#define EMBED_SIZE 896

int main(int argc, char** argv)
{

    if (argc < 4)
    {
            std::cerr << "Usage: " << argv[0] << "<rknn model_config_path> <rkllm model_config_path> <image_path>" << std::endl;
            return -1;
    }

     /* LLM推理引擎 */
    Inference_NS::CLLMInferenceRK* m_pLlmEngine;
    /* RKNN视觉模型 */
    CImageEnoderInferenceRK *m_pImageEncoder;

    /*RKNN视觉模型配置*/
    std::string strRknnConfigPath = argv[1];
    /*LLM推理模型配置*/
    std::string strRkllmConfigPath = argv[2];

    size_t n_image_tokens = IMAGE_TOKEN_NUM;
    size_t image_embed_len = EMBED_SIZE;
    int rkllm_image_embed_len = n_image_tokens * image_embed_len;
    float img_vec[rkllm_image_embed_len];
   
    
    std::cout << "初始化RKNN"<< std::endl;
    /* 初始化RKNN */ 
    m_pImageEncoder = new CImageEnoderInferenceRK(strRknnConfigPath);
    if (!m_pImageEncoder) {
        std::cerr << "创建LLM推理引擎失败"<< std::endl;
        return -1;
    }

    if (m_pImageEncoder->init()) 
    {
        std::cerr << "RKNN初始化失败" << std::endl;
        delete m_pImageEncoder;
        m_pImageEncoder = nullptr;
        return -1;
    }

    std::cout << "初始化RKLLM"<< std::endl;
    // 创建LLM推理引擎
    m_pLlmEngine = new Inference_NS::CLLMInferenceRK(strRkllmConfigPath);
    if (!m_pLlmEngine) {
        std::cerr << "创建LLM推理引擎失败"<< std::endl;
        return -1;
    }

    // 初始化引擎
    if (m_pLlmEngine->init()) {
        std::cerr << "LLM推理引擎初始化失败"<< std::endl;
        delete m_pLlmEngine;
        m_pLlmEngine = nullptr;
        return -1;
    }



    while(1)
    {
        std::string result;
        std::string InputText; 
        std::cout << "请输入提问的问题(分析图片加上，提问词加上<image>):" ; 
        if (!std::getline(std::cin, InputText)) {
            std::cin.clear();
            continue;
        }

        bool isText = (InputText.find("<image>") == std::string::npos) ? true : false;


        if(!isText)
        {

            cv::Mat processed_img = cv::imread(argv[3]);
            if (processed_img .empty()) {
                std::cerr << "无法加载图像: %s" << argv[3] << std::endl;
                return false;
            }

            // 图像预处理
            if (processed_img.channels() == 3) {
                cv::cvtColor(processed_img, processed_img, cv::COLOR_BGR2RGB);
            }
            
            // 扩展为正方形并调整大小
            cv::Scalar background_color(127.0, 127.0, 127.0);
            
            // 计算扩展尺寸
            int width = processed_img.cols;
            int height = processed_img.rows;
            int size = std::max(width, height);
            
            cv::Mat square_img(size, size, processed_img.type(), background_color);
            
            int x_offset = (size - width) / 2;
            int y_offset = (size - height) / 2;
            
            cv::Rect roi(x_offset, y_offset, width, height);
            processed_img.copyTo(square_img(roi));
            
            // 调整到模型输入尺寸
            cv::Mat resized_img;
            cv::Size new_size(IMAGE_WIDTH, IMAGE_HEIGHT);
            cv::resize(square_img, resized_img, new_size, 0, 0, cv::INTER_LINEAR);

            // 调用视觉引擎进行推理
            int ret=m_pImageEncoder->run(resized_img.data, img_vec);
            if (ret != 0) {
                std::cerr << "run_imgenc fail! ret=%d\n"<< std::endl;
                continue;  
            }

        }

        //推理
        if (!m_pLlmEngine->run(isText, InputText, img_vec,result)) {
                std::cout << result << std::endl;
        } else {
               std::cout << "失败" << std::endl;
        }


    }


    
    m_pImageEncoder->release(); 
    delete m_pImageEncoder;
    m_pImageEncoder = nullptr;

    m_pLlmEngine->unInit();
    delete m_pLlmEngine;
    m_pLlmEngine = nullptr;


    return 0;

}
