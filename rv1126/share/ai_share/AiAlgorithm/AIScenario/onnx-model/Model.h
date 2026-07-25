/*
*  File Name:Model.h
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :ONNX模型读取 	
*  Modify date: 
*/

#ifndef ONNX_MODEL_H
#define ONNX_MODEL_H

#include "onnxruntime_cxx_api.h"
#include <opencv2/opencv.hpp>

class Model {
    private:
        /* 存储模型的参数 */
        Ort::Env env;
        Ort::Session *session;
        /* 存放模型输入和输出信息的参数 */
        std::vector<const char*> m_InputNodeNames;
        std::vector<const char*> m_OutNodeNames;
    
    public:
        Model(const std::string ModelPath);
        /* onnxruntime模型的推理过程 */
        cv::Mat Inference(cv::Mat blob, std::vector<int64_t> input_node_dims);
};


#endif