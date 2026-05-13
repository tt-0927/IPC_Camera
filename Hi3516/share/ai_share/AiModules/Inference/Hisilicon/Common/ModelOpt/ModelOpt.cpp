/*
 * @FilePath     : ModelOpt.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 20:06:21
 * @Description  : HISI模型操作
 */
#include "ModelOpt.hpp"

#include <cstring>
#include <fstream>
#include <sys/stat.h>

static const int BYTE_BIT_NUM = 8; // 1 byte = 8 bit

Inference_NS::CModelOpt::CModelOpt(std::string strModelPath)
    : m_strModelPath(strModelPath)
{
}

Inference_NS::CModelOpt::~CModelOpt()
{
    unInit();
}

/* 设置模型路径 */
void Inference_NS::CModelOpt::setModelPath(std::string strModelPath)
{
    m_strModelPath = strModelPath;
}

/* 初始化模型 */
bool Inference_NS::CModelOpt::init()
{
    if (m_bInitialized)
    {
        /* 模型已被初始化 */
        return true;
    }

    svp_acl_error ret = SVP_ACL_SUCCESS;
    svp_acl_rt_run_mode runMode;
    uint32_t fileSize = 0;     // 用于存储模型文件的字节大小
    struct stat sBuf;          // 用于存储文件的状态信息
    std::ifstream binFile;
    int modelSize;

    /* 1. 初始化 ACL 环境 */
    ret = svp_acl_init(NULL);
    if (ret != SVP_ACL_SUCCESS) {
        printf("ACL 初始化失败。 \n");
        goto EXIT;
    }

    /* 2. 设置设备 */
    ret = svp_acl_rt_set_device(deviceId);
    if (ret != SVP_ACL_SUCCESS) {
        printf("设置设备 ID=[%d] 失败。 \n", deviceId);
        goto EXIT;
    }

    /* 3. 设置操作超时时间 */
    ret = svp_acl_rt_set_op_wait_timeout(nWaitTimeOut);
    if (ret != SVP_ACL_SUCCESS) {
        printf("设置操作超时时间为 [%d] 失败 \n", nWaitTimeOut);
        goto EXIT;
    }

    /* 4. 创建上下文 */
    ret = svp_acl_rt_create_context(&context_, deviceId);
    if (ret != SVP_ACL_SUCCESS) {
        printf("创建上下文失败。 \n");
        goto EXIT;
    }

    /* 5. 创建 Stream */
    ret = svp_acl_rt_create_stream(&stream_);
    if (ret != SVP_ACL_SUCCESS) {
        printf("创建 stream 失败。 \n");
        goto EXIT;
    }

    /* 6. 检查运行模式 */
    ret = svp_acl_rt_get_run_mode(&runMode);
    if (ret != SVP_ACL_SUCCESS || runMode != SVP_ACL_DEVICE) {
        printf("ACL 运行模式异常。\n");
        goto EXIT;
    }

    /* 7. 加载模型文件 */
    binFile.open(m_strModelPath, std::ifstream::binary);
    /* 获取文件长度 */
    binFile.seekg(0, binFile.end);
    modelSize = binFile.tellg();
    if (modelSize == 0) {
        binFile.close();
        printf("读取模型 [%s] 为空，请确认模型文件是否正确！ \n", m_strModelPath.c_str());
        goto EXIT;
    }
    binFile.seekg(0, binFile.beg);  // 将文件指针重置到文件开头
    /* 在设备内存中分配 buffer */
    ret = svp_acl_rt_malloc(&modelMemPtr_ , modelSize, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != SVP_ACL_SUCCESS) {
        binFile.close();
        printf("分配设备内存缓冲区失败. 大小为 [%u] \n", modelSize);
        goto EXIT;
    }
    /* 初始化设备内存（通常清零），避免残留脏数据 */
    memset(static_cast<int8_t*>(modelMemPtr_ ), 0, modelSize);
    /* 将文件内容读入设备内存 buffer 中 */
    binFile.read(static_cast<char *>(modelMemPtr_ ), modelSize);
    binFile.close();

    /* 8. 加载模型到设备 */
    ret = svp_acl_mdl_load_from_mem(static_cast<uint8_t*>(modelMemPtr_ ), modelSize, &modelId_);
    if (ret != SVP_ACL_SUCCESS) {
        printf("模型加载失败，模型路径为 [%s]。 \n", m_strModelPath.c_str());
        goto EXIT;
    }

    printf("模型加载成功！！ \n");

    /* 9. 创建模型描述 */
    modelDesc_ = svp_acl_mdl_create_desc();
    if (modelDesc_ == nullptr) {
        printf("创建模型描述符失败。 \n");
        goto EXIT;
    }
    ret = svp_acl_mdl_get_desc(modelDesc_, modelId_);
    if (ret != SVP_ACL_SUCCESS) {
        printf("获取模型描述失败. \n");
        goto EXIT;
    }

    m_bInitialized = true;
    return true;

EXIT:
    // 释放资源
    // for (auto buf : outputBuffers_) {
    //     if (buf) svp_acl_rt_free(buf);
    // }
    // outputBuffers_.clear();

    Unload();
    DestroyResource();

    m_bInitialized = false;

    return false;

}

/* 反初始化模型 */
bool Inference_NS::CModelOpt::unInit()
{
    if (m_bInitialized)
    {
        Unload();
        DestroyResource();

        m_bInitialized = false;
        return true;
    }

    return false;
}


void Inference_NS::CModelOpt::Unload()
{
    svp_acl_error ret;
    ret = svp_acl_mdl_unload(modelId_);
    if (ret != SVP_ACL_SUCCESS) {
        printf("卸载模型失败, modelId = [%u] \n", modelId_);
    }
    if (modelDesc_ != nullptr) {
        (void)svp_acl_mdl_destroy_desc(modelDesc_);
        modelDesc_ = nullptr;
    }
    if (modelMemPtr_ != nullptr) {
        svp_acl_rt_free(modelMemPtr_);
        modelMemPtr_ = nullptr;
    }
}

void Inference_NS::CModelOpt::DestroyInput(svp_acl_mdl_dataset*& pInputs)
{
    if (pInputs == nullptr) {
        return;
    }
    for (size_t i = 0; i < svp_acl_mdl_get_dataset_num_buffers(pInputs); ++i) {
        svp_acl_data_buffer* dataBuffer = svp_acl_mdl_get_dataset_buffer(pInputs, i);
        void* tmp = svp_acl_get_data_buffer_addr(dataBuffer);
        svp_acl_rt_free(tmp);
        svp_acl_destroy_data_buffer(dataBuffer);
    }
    svp_acl_mdl_destroy_dataset(pInputs);
    pInputs = nullptr;
}

void Inference_NS::CModelOpt::DestroyOutput(svp_acl_mdl_dataset*& pOutputs)
{
    if (pOutputs == nullptr) {
        return;
    }
    for (size_t i = 0; i < svp_acl_mdl_get_dataset_num_buffers(pOutputs); ++i) {
        svp_acl_data_buffer* dataBuffer = svp_acl_mdl_get_dataset_buffer(pOutputs, i);
        void* data = svp_acl_get_data_buffer_addr(dataBuffer);
        (void)svp_acl_rt_free(data);
        (void)svp_acl_destroy_data_buffer(dataBuffer);
    }
    (void)svp_acl_mdl_destroy_dataset(pOutputs);
    pOutputs = nullptr;
}

void Inference_NS::CModelOpt::DestroyResource()
{
    svp_acl_error ret;
    ret = svp_acl_rt_set_current_context(context_);
    if (ret != SVP_ACL_SUCCESS) {
        printf("设置当前 context 失败，stream 无法正确销毁。\n");
    }

    if (stream_ != nullptr) {
        ret = svp_acl_rt_destroy_stream(stream_);
        if (ret != SVP_ACL_SUCCESS) {
            printf("释放 stream 失败。 \n");
        }
        stream_ = nullptr;
    }
    if (context_ != nullptr) {
        ret = svp_acl_rt_destroy_context(context_);
        if (ret != SVP_ACL_SUCCESS) {
            printf("销毁上下文失败。 \n");
        }
        context_ = nullptr;
    }
    ret = svp_acl_rt_reset_device(deviceId);
    if (ret != SVP_ACL_SUCCESS) {
        printf("设备复位失败。 \n");
    }
    ret = svp_acl_finalize();
    if (ret != SVP_ACL_SUCCESS) {
        printf("调用 ACL 反初始化函数失败。 \n");
    }
}

bool Inference_NS::CModelOpt::getInputSize(int& inputSize)
{
    if (m_bInitialized)
    {
        inputSize = static_cast<int>(svp_acl_mdl_get_num_inputs(modelDesc_));
        return true;
    }
    return false;
}

bool Inference_NS::CModelOpt::getOutputSize(int& outputSize)
{
    if (m_bInitialized)
    {
        outputSize = static_cast<int>(svp_acl_mdl_get_num_outputs(modelDesc_));
        return true;
    }
    return false;
}



/* 获取模型输入参数 */
bool Inference_NS::CModelOpt::getInputAttrs(int nIndex, size_t& bufSize, size_t& stride, svp_acl_mdl_io_dims& inputDims, size_t& dataSize)
{
    if (m_bInitialized)
    {
        svp_acl_error ret;
        ret = svp_acl_mdl_get_input_dims(modelDesc_, nIndex, &inputDims);
        if (ret != SVP_ACL_SUCCESS) {
            printf("获取模型描述符中指定输入索引的张量维度信息失败 \n");
            return false;
        }
        /* 获取该输入的默认 stride */
        stride = svp_acl_mdl_get_input_default_stride(modelDesc_, nIndex);
        if (stride == 0) {
            printf("获取该输入的默认 stride 失败 \n");
            return false;
        }
        /* 获取指定输入张量实际所需的内存大小 */
        bufSize = svp_acl_mdl_get_input_size_by_index(modelDesc_, nIndex);

        if (bufSize == 0) {
            printf("获取指定输入张量实际所需的内存大小失败 \n");
            return false;
        }
        /* 获取模型第 0 个输入的总数据大小 */
        svp_acl_data_type dataType = svp_acl_mdl_get_input_data_type(modelDesc_, nIndex);
        dataSize = svp_acl_data_type_size(dataType) / BYTE_BIT_NUM;
        // printf("dataSize: [%zu], svp_acl_data_type_size(dataType): [%zu]！ \n", dataSize, svp_acl_data_type_size(dataType));
        if (dataSize == 0) {
            printf("获取模型第 0 个输入的总数据大小失败 \n");
            return false;
        }

        return true;
    }
    return false;
}

/* 获取模型输出参数 */
bool Inference_NS::CModelOpt::getOutputAttrs(int nIndex, size_t& stride, size_t& bufSize, svp_acl_mdl_io_dims& outputDims)
{
    if (m_bInitialized)
    {
        svp_acl_error ret;
        ret = svp_acl_mdl_get_output_dims(modelDesc_, nIndex, &outputDims);
        if (ret != SVP_ACL_SUCCESS)
        {
            printf("获取模型输出描述失败：index=[%zu]\n", nIndex);
            return false;
        }
        /* 获取该输出的默认步长（即每行数据的对齐大小） */
        stride = svp_acl_mdl_get_output_default_stride(modelDesc_, nIndex);
        if (stride == 0) {
            printf("错误：输出的步长是 %zu. \n", stride);
            return false;
        }
        /* 获取该输出张量所需的总内存大小 */
        bufSize = svp_acl_mdl_get_output_size_by_index(modelDesc_, nIndex);
        if (bufSize == 0) {
            printf("错误：输出大小为 %zu. \n", bufSize);
            return false;
        }
        return true;
    }
    return false;
}

/* 运行模型 */
bool Inference_NS::CModelOpt::run(svp_acl_mdl_dataset*& pInputs, svp_acl_mdl_dataset*& pOutputs)
{
    if (m_bInitialized)
    {
        /* 运行 */
        svp_acl_error ret;
        ret = svp_acl_mdl_execute(modelId_, pInputs, pOutputs);
        if (ret != SVP_ACL_SUCCESS)
        {
            return false;
        }
        return true;
    }

    return false;
}
