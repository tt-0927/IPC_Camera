/**
 * @file OnnxModelOpt.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief Onnx模型操作
 */
#include "OnnxModelOpt.hpp"

#include <cstring>
#include <fstream>

Inference_NS::COnnxModelOpt::COnnxModelOpt(std::string strModelPath)
    : m_strModelPath(strModelPath), m_stEnv(ORT_LOGGING_LEVEL_WARNING, "InferenceEngine"), m_stAllocator(), m_stModelMetadata(nullptr)
{
}

Inference_NS::COnnxModelOpt::~COnnxModelOpt()
{
    unInit();
}

/* 设置模型路径 */
void Inference_NS::COnnxModelOpt::setModelPath(std::string strModelPath)
{
    m_strModelPath = strModelPath;
}

/* 设置GPU的相关配置 */
void Inference_NS::COnnxModelOpt::setInferData(
    int nCpuInferThread,
    int nDeviceId,
    long long nGpuMemLimit)
{
    m_nCpuInferThread = nCpuInferThread;
    m_nDeviceId = nDeviceId;
    m_nGpuMemLimit = nGpuMemLimit;
}

/* 初始化模型 */
bool Inference_NS::COnnxModelOpt::init()
{
    if (m_bInitialized)
    {
        /* 模型已被初始化 */
        return true;
    }
    bool bRet = false;
    int nRet = 0;

    /* 配置会话线程池大小 */
    Ort::SessionOptions stSessionOptions;
    stSessionOptions.SetIntraOpNumThreads(m_nCpuInferThread);
    /* 开启图优化 */
    stSessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    /* 启用内存优化（默认已开启） */
    stSessionOptions.EnableCpuMemArena();

#ifdef USE_CUDA
    /* 采用 GPU 推理 */
    OrtCUDAProviderOptions cuda_options{};
    /* 选择 GPU 设备 0 */
    cuda_options.device_id = m_nDeviceId;
    cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
    /* 限制 2GB 显存（可调） */
    cuda_options.gpu_mem_limit = m_nGpuMemLimit;
    cuda_options.arena_extend_strategy = m_nArenaExtendStrategy;
    cuda_options.do_copy_in_default_stream = m_nDoCopyInDefaultStream;
    /* 不同版本的 ONNX Runtime 可能对 CUDA 参数支持不同 */
    stSessionOptions.AppendExecutionProvider_CUDA(cuda_options);
    std::cout << "使用 GPU 进行推理..." << std::endl;
#else
    std::cout << "使用 CPU 进行推理..." << std::endl;
#endif

    /* 加载onnx，初始化会话 */
    m_stSession = Ort::Session(m_stEnv, m_strModelPath.c_str(), stSessionOptions);

    /* 初始化成功 */
    m_bInitialized = true;

    /* 获取版本号 */
    std::string strVersion = Ort::GetVersionString();
    printf("获取Onnxrumtime版本信息 [%s]\n", strVersion.c_str());

    /* 获取模型的输入和输出的数量 */
    m_nInputNum = m_stSession.GetInputCount();
    m_nOutputNum = m_stSession.GetOutputCount();
    /* 获取元组信息对象 */
    m_stModelMetadata = m_stSession.GetModelMetadata();

    /* 获取输入tensor的属性信息 */
    m_vInputNamesStr.clear();
    m_vInputAttrs.clear();
    for (size_t i = 0; i < m_nInputNum; ++i)
    {
        /* 获取i输入的名字 */
        std::string strInputName = m_stSession.GetInputNameAllocated(i, m_stAllocator).get();
        m_vInputNamesStr.push_back(strInputName);
        /* 获取i输入的形状 */
        auto InputTypeInfo = m_stSession.GetInputTypeInfo(i);
        auto InputTensorInfo = InputTypeInfo.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> vInputShape = InputTensorInfo.GetShape();
        m_vInputAttrs.push_back(vInputShape);
        /* 打印输入信息 */
        std::cout << "[" << i << "] 名称：" << strInputName << ", 形状: [";
        for (size_t j = 0; j < vInputShape.size(); j++)
        {
            std::cout << vInputShape[j];
            if (j < vInputShape.size() - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
    for (const auto &name : m_vInputNamesStr)
    {
        m_vInputNames.push_back(name.c_str());
    }

    /* 获取输出tensor的属性信息 */
    m_vOutputNamesStr.clear();
    m_vOutputAttrs.clear();
    for (size_t i = 0; i < m_nOutputNum; ++i)
    {
        /* 获取i输出的名字 */
        std::string strOutputName = m_stSession.GetOutputNameAllocated(i, m_stAllocator).get();
        m_vOutputNamesStr.push_back(strOutputName);
        /* 获取i输出的形状 */
        auto OutputTypeInfo = m_stSession.GetOutputTypeInfo(i);
        auto OutputTensorInfo = OutputTypeInfo.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> vOutputShape = OutputTensorInfo.GetShape();
        m_vOutputAttrs.push_back(vOutputShape);
        /* 打印输出信息 */
        std::cout << "[" << i << "] 名称：" << strOutputName << ", 形状: [";
        for (size_t j = 0; j < vOutputShape.size(); j++)
        {
            std::cout << vOutputShape[j];
            if (j < vOutputShape.size() - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
    for (const auto &name : m_vOutputNamesStr)
    {
        m_vOutputNames.push_back(name.c_str());
    }

    bRet = true;
EXIT:
    if (!bRet)
    {
        if (m_bInitialized)
        {
            m_bInitialized = false;
        }
    }

    return bRet;
}

/* 反初始化模型 */
bool Inference_NS::COnnxModelOpt::unInit()
{
    if (m_bInitialized)
    {
        m_bInitialized = false;
        return true;
    }

    return false;
}

bool Inference_NS::COnnxModelOpt::getStringMetadata(std::string strName, std::string &strOutData)
{
    try
    {
        /* 使用 AllocatedStringPtr 接收返回值 */
        Ort::AllocatedStringPtr valuePtr = m_stModelMetadata.LookupCustomMetadataMapAllocated(strName.c_str(), m_stAllocator);

        /* 直接获取字符串指针 */
        char *strData = valuePtr.get();
        if (strData != nullptr)
        {
            strOutData = std::string(strData);
            return true;
        }
        else
        {
            std::cout << "Custom Metadata [" << strName << "] 不存在或者不是字符串" << std::endl;
            return false;
        }
    }
    catch (const Ort::Exception &e)
    {
        return false;
    }
}

/* 获取模型输入参数 */
bool Inference_NS::COnnxModelOpt::getInputAttrs(std::vector<std::vector<int64_t>> &vInputAttrs)
{
    if (m_bInitialized)
    {
        vInputAttrs = m_vInputAttrs;
        return true;
    }
    return false;
}

/* 获取模型输出参数 */
bool Inference_NS::COnnxModelOpt::getOutputAttrs(std::vector<std::vector<int64_t>> &vOutputAttrs)
{
    if (m_bInitialized)
    {
        vOutputAttrs = m_vOutputAttrs;
        return true;
    }
    return false;
}

/* 运行模型 */
bool Inference_NS::COnnxModelOpt::run(
    std::vector<float *> vInputs,
    std::vector<int64_t> nInputDataSizes,
    std::vector<float *> &vOutputs)
{
    int nRet = 0;

    if (m_bInitialized &&
        m_nInputNum == vInputs.size())
    {

        try
        {
            Ort::MemoryInfo stMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            /* 准备张量数组 */
            std::vector<Ort::Value> vInputTensors;
            for (int nI = 0; nI < m_nInputNum; nI++)
            {
                Ort::Value stInputTensor = Ort::Value::CreateTensor<float>(
                    stMemoryInfo,
                    vInputs[nI],
                    nInputDataSizes[nI],
                    m_vInputAttrs[nI].data(),
                    m_vInputAttrs[nI].size());
                vInputTensors.emplace_back(std::move(stInputTensor));
            }

            /* 模型推理 */
            auto OutputTensors = m_stSession.Run(
                Ort::RunOptions{nullptr},
                m_vInputNames.data(),
                vInputTensors.data(),
                vInputTensors.size(),
                m_vOutputNames.data(),
                m_vOutputNames.size());
            /* 将模型输出存放到容器 */
            for (auto &tensor : OutputTensors)
            {
                float *data = tensor.GetTensorMutableData<float>();
                vOutputs.push_back(data);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "模型推理异常: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    return false;
}

/* 运行模型 */
bool Inference_NS::COnnxModelOpt::run(
    std::vector<Ort::Value> &vInputs,
    std::vector<Ort::Value> &vOutputs)
{
    int nRet = 0;

    if (m_bInitialized &&
        m_nInputNum == vInputs.size())
    {

        try
        {
            /* 模型推理 */
            vOutputs = m_stSession.Run(
                Ort::RunOptions{nullptr},
                m_vInputNames.data(),
                vInputs.data(),
                vInputs.size(),
                m_vOutputNames.data(),
                m_vOutputNames.size());
        }
        catch (const std::exception &e)
        {
            std::cerr << "模型推理异常: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    printf("未初始化模型，或者输入数据个数[%ld]!+模型个数[%ld]\n",vInputs.size(),m_nInputNum);
    return false;
}