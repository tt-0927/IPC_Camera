/*
*  File Name:Model.cpp
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :ONNX模型读取 	
*  Modify date: 
*/

#include "Model.h"

Model::Model(const std::string ModelPath) {
    /* onnxruntime读取模型初始化参数 */
    Ort::SessionOptions sessionOptions = Ort::SessionOptions();
    Ort::AllocatorWithDefaultOptions allocator;
    /* 模型路径 */
    std::cerr << "模型路径：" << ModelPath << std::endl;
    /* 读取模型 */
    std::cerr << "读取模型......" << std::endl;
    sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
    session = new Ort::Session(env, ModelPath.c_str(), sessionOptions);
    std::cerr << "模型读取成功！" << std::endl;
    /* 获取模型输入输出的信息 */
    std::cerr << "获取模型输入输出的信息......" << std::endl;
    const char* input_name = session->GetInputName(0, allocator);
    m_InputNodeNames = { input_name };
    const char* output_name = session->GetOutputName(0, allocator);
    m_OutNodeNames = { output_name };
    std::cerr << "模型输入输出的信息获取成功！" << std::endl;
}


// onnxruntime模型的推理过程
cv::Mat Model::Inference(cv::Mat blob, std::vector<int64_t> input_node_dims) {
    std::vector<Ort::Value> input_tensors;
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(memory_info, blob.ptr<float>(), blob.total(), input_node_dims.data(), input_node_dims.size()));
    /* 放入到模型进行推理 */
    std::cerr << "开始推理......" << std::endl;
    std::vector<Ort::Value> output_tensors = session->Run(
        Ort::RunOptions{nullptr},
        m_InputNodeNames.data(),
        input_tensors.data(),
        m_InputNodeNames.size(),
        m_OutNodeNames.data(),
        m_OutNodeNames.size()
    );
    std::cerr << "推理完成！" << std::endl;
    float* floatarr = output_tensors[0].GetTensorMutableData<float>();
    /* 数据格式转换 */
    cv::Mat mask = cv::Mat::zeros(static_cast<int>(input_node_dims[2]), static_cast<int>(input_node_dims[3]), CV_8UC1);
    for (int i{ 0 }; i < static_cast<int>(input_node_dims[2]); i++) {
        for (int j{ 0 }; j < static_cast<int>(input_node_dims[3]); ++j) {
            mask.at<uchar>(i, j) = static_cast<uchar>(floatarr[i * static_cast<int>(input_node_dims[3]) + j] > 0.5);
        }
    }
    input_tensors.clear();
    return mask;
}
