/**
 * @FilePath     : ImageEnoderInferenceRK.hpp
 * @Author       : leiyy
 * @Date         : 2025-09-12
 * @LastEditors  : leyy
 * @LastEditTime : 2025-09-22
 * @Description  : 图像编码器类
 */

#pragma once

#include "rknn_api.h"
#include <string>
#include <memory>
#ifdef JSON_ENABLE
#include "JsonInterfase.h"
#else
#include "Json.h"
#endif
#include <fstream>

class CImageEnoderInferenceRK
{
public:
    // 图像编码器上下文结构体
    typedef struct {
        rknn_context rknn_ctx;
        rknn_input_output_num io_num;
        rknn_tensor_attr* input_attrs;
        rknn_tensor_attr* output_attrs;
        int model_channel;
        int model_width;
        int model_height;
    } ImageEncoderContext;

    CImageEnoderInferenceRK(std::string &strConfigPath);
    ~CImageEnoderInferenceRK();

    /**
     * @brief 初始化图像编码器
     * @return 成功返回0，失败返回非0
     */
    int init();

    /**
     * @brief 释放图像编码器资源
     * @return 成功返回0，失败返回非0
     */
    int release();

    /**
    * @brief 校验模型配置文件的公共信息
    * @return [*]
    */
     bool checkModelConfig();

    /**
     * @brief 运行图像编码
     * @param img_data 图像数据
     * @param out_result 输出结果
     * @return 成功返回0，失败返回非0
     */
    int run(void* img_data, float* out_result);

    /**
     * @brief 获取模型输入宽度
     * @return 模型输入宽度
     */
    int getModelWidth() const { return m_ctx.model_width; }

    /**
     * @brief 获取模型输入高度
     * @return 模型输入高度
     */
    int getModelHeight() const { return m_ctx.model_height; }

    /**
     * @brief 获取模型输入通道数
     * @return 模型输入通道数
     */
    int getModelChannel() const { return m_ctx.model_channel; }

    /**
     * @brief 检查编码器是否已初始化
     * @return 已初始化返回true，否则返回false
     */
    bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 从文件读取数据
     * @param path 文件路径
     * @param out_data 输出数据指针
     * @return 文件大小，失败返回-1
     */
    int readDataFromFile(const char* path, char** out_data);

    /**
     * @brief 打印张量属性
     * @param attr 张量属性
     */
    void dumpTensorAttr(rknn_tensor_attr* attr);

private:
    std::string m_strConfigPath;             /* 模型json配置路径 */

    std::string m_strModelPath;              /* 模型路径 */

    int m_CoreNum;        

    ImageEncoderContext m_ctx;

    bool m_initialized;
};