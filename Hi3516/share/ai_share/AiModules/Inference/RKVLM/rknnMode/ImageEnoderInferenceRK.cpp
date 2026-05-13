/**
 * @FilePath     : ImageEnoderInferenceRK.hpp
 * @Author       : leiyy
 * @Date         : 2025-09-12
 * @LastEditors  : leyy
 * @LastEditTime : 2025-09-22
 * @Description  : 图像编码器
 */

#include "ImageEnoderInferenceRK.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

CImageEnoderInferenceRK::CImageEnoderInferenceRK(std::string &strConfigPath)
    : m_initialized(false),m_strConfigPath(strConfigPath)
{
    memset(&m_ctx, 0, sizeof(ImageEncoderContext));
}

CImageEnoderInferenceRK::~CImageEnoderInferenceRK()
{
    release();
}

int CImageEnoderInferenceRK::readDataFromFile(const char* path, char** out_data)
{
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        std::cerr << "fopen " << path << " fail!" << std::endl;
        return -1;
    }
    
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    char* data = (char*)malloc(file_size + 1);
    data[file_size] = 0;
    fseek(fp, 0, SEEK_SET);
    
    if (file_size != fread(data, 1, file_size, fp)) {
        std::cerr << "fread " << path << " fail!" << std::endl;
        free(data);
        fclose(fp);
        return -1;
    }
    
    if (fp) {
        fclose(fp);
    }
    
    *out_data = data;
    return file_size;
}

void CImageEnoderInferenceRK::dumpTensorAttr(rknn_tensor_attr* attr)
{
    std::cout << "  index=" << attr->index 
              << ", name=" << attr->name 
              << ", n_dims=" << attr->n_dims 
              << ", dims=[" << attr->dims[0] << ", " << attr->dims[1] 
              << ", " << attr->dims[2] << ", " << attr->dims[3] 
              << "], n_elems=" << attr->n_elems 
              << ", size=" << attr->size 
              << ", fmt=" << get_format_string(attr->fmt)
              << ", type=" << get_type_string(attr->type)
              << ", qnt_type=" << get_qnt_type_string(attr->qnt_type)
              << ", zp=" << attr->zp 
              << ", scale=" << attr->scale 
              << std::endl;
}

int CImageEnoderInferenceRK::init()
{
    if (m_initialized) {
        std::cout << "RKNN模型已初始化" << std::endl;
        return true;
    }

    /* 解析json模型参数 */
    if (checkModelConfig())
    {
        return true;
    }

    int ret;
    int model_len = 0;
    char* model = nullptr;
    rknn_context ctx = 0;
    memset(&ctx, 0, sizeof(rknn_context));

    // 加载RKNN模型
    //  model_len = readDataFromFile(m_strModelPath.c_str(), &model);
    // if (model == NULL) {
    //     std::cerr << "load_model fail!" << std::endl;
    //     return false;
    // }


    ret = rknn_init(&ctx, (void*)m_strModelPath.c_str(), 0, 0, NULL);

    if (ret < 0) {
        std::cerr << "rknn_init fail! ret=" << ret << std::endl;
        return ret;
    }

    std::cout << "===the core num is " << m_CoreNum << "===" << std::endl;
    
    // 设置多核推理
    if (m_CoreNum == 2) {
        int ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_0_1);
    } else if (m_CoreNum == 3) {
        int ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_0_1_2);
    } else {
        int ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_AUTO);
    }
    
    if (ret < 0) {
        std::cerr << "rknn_set_core_mask fail! ret=" << ret << std::endl;
        return ret;
    }

    // 获取模型输入输出数量
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        std::cerr << "rknn_query fail! ret=" << ret << std::endl;
        return ret;
    }
    
    std::cout << "model input num: " << io_num.n_input << ", output num: " << io_num.n_output << std::endl;

    // 获取模型输入信息
    std::cout << "input tensors:" << std::endl;
    rknn_tensor_attr* input_attrs = new rknn_tensor_attr[io_num.n_input];
    memset(input_attrs, 0, sizeof(rknn_tensor_attr) * io_num.n_input);
    
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            std::cerr << "rknn_query fail! ret=" << ret << std::endl;
            delete[] input_attrs;
            return ret;
        }
        dumpTensorAttr(&(input_attrs[i]));
    }

    // 获取模型输出信息
    std::cout << "output tensors:" << std::endl;
    rknn_tensor_attr* output_attrs = new rknn_tensor_attr[io_num.n_output];
    memset(output_attrs, 0, sizeof(rknn_tensor_attr) * io_num.n_output);
    
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            std::cerr << "rknn_query fail! ret=" << ret << std::endl;
            delete[] input_attrs;
            delete[] output_attrs;
            return ret;
        }
        dumpTensorAttr(&(output_attrs[i]));
    }

    // 设置到上下文
    m_ctx.rknn_ctx = ctx;
    m_ctx.io_num = io_num;
    m_ctx.input_attrs = input_attrs;
    m_ctx.output_attrs = output_attrs;

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        m_ctx.model_channel = input_attrs[0].dims[1];
        m_ctx.model_height = input_attrs[0].dims[2];
        m_ctx.model_width = input_attrs[0].dims[3];
    } else {
        m_ctx.model_height = input_attrs[0].dims[1];
        m_ctx.model_width = input_attrs[0].dims[2];
        m_ctx.model_channel = input_attrs[0].dims[3];
    }
    
    std::cout << "model input height=" << m_ctx.model_height 
              << ", width=" << m_ctx.model_width 
              << ", channel=" << m_ctx.model_channel 
              << std::endl;

    m_initialized = true;
    return 0;
}

int CImageEnoderInferenceRK::release()
{
    if (!m_initialized) {
        return 0;
    }

    if (m_ctx.input_attrs != NULL) {
        delete[] m_ctx.input_attrs;
        m_ctx.input_attrs = NULL;
    }
    
    if (m_ctx.output_attrs != NULL) {
        delete[] m_ctx.output_attrs;
        m_ctx.output_attrs = NULL;
    }
    
    if (m_ctx.rknn_ctx != 0) {
        rknn_destroy(m_ctx.rknn_ctx);
        m_ctx.rknn_ctx = 0;
    }
    
    m_initialized = false;
    return 0;
}

/* 校验模型配置文件的公共信息 */
bool CImageEnoderInferenceRK::checkModelConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return true;
    }

    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return true;
    }

    Json::Object *pJsonHandle = NULL;
    pJsonHandle = Json::init(pchJson);
    bool bRet;

    /* 获取模型地址 */
    bRet = Json::get(pJsonHandle, "model_path", m_strModelPath);
    if (!bRet)
    {
        printf("解析model_path字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*core_num参数*/
    bRet = Json::get(pJsonHandle, "core_num", m_CoreNum);
    if (!bRet)
    {
        printf("解析core_num字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    return false;
}

int CImageEnoderInferenceRK::run(void* img_data, float* out_result)
{
    if (!m_initialized) {
        std::cerr << "图像编码器未初始化" << std::endl;
        return -1;
    }

    int ret;
    rknn_input inputs[1];
    rknn_output outputs[1];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // 设置输入数据
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = m_ctx.model_width * m_ctx.model_height * m_ctx.model_channel;
    inputs[0].buf = img_data;

    ret = rknn_inputs_set(m_ctx.rknn_ctx, 1, inputs);
    if (ret < 0) {
        std::cerr << "rknn_input_set fail! ret=" << ret << std::endl;
        return ret;
    }

    // 运行
    ret = rknn_run(m_ctx.rknn_ctx, nullptr);
    if (ret < 0) {
        std::cerr << "rknn_run fail! ret=" << ret << std::endl;
        return ret;
    }

    // 获取输出
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(m_ctx.rknn_ctx, 1, outputs, NULL);
    if (ret < 0) {
        std::cerr << "rknn_outputs_get fail! ret=" << ret << std::endl;
        return ret;
    }

    // 后处理
    memcpy(out_result, outputs[0].buf, outputs[0].size);

    // 释放RKNN输出
    rknn_outputs_release(m_ctx.rknn_ctx, 1, outputs);

    return ret;
}