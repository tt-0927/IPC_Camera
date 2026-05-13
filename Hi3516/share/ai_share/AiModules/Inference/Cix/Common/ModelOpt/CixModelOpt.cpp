/**
 * @file CixModelOpt.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-11-11
 *
 * @brief
 */
#include "CixModelOpt.hpp"
#include "OutDataProcess.hpp"
#include <cstring>
#include <fstream>

Inference_NS::CCixModelOpt::CCixModelOpt(std::string strModelPath)
    : m_strModelPath(strModelPath)
{
}

Inference_NS::CCixModelOpt::~CCixModelOpt()
{
    unInit();
}

/* 设置模型路径 */
void Inference_NS::CCixModelOpt::setModelPath(std::string strModelPath)
{
    m_strModelPath = strModelPath;
}

/* 初始化模型 */
bool Inference_NS::CCixModelOpt::init()
{
    if (m_bInitialized)
    {
        /* 模型已被初始化 */
        return true;
    }
    bool bRet = false;
    noe_status_t nRet;
    const char *stMsg = nullptr;

    uint32_t nInputNum, nOutputNum;

    /* 初始化推理上下文 */
    nRet = noe_init_context(&m_pCtx);
    if (nRet != NOE_STATUS_SUCCESS)
    {
        noe_get_error_message(m_pCtx, nRet, &stMsg);
        std::cout << "noe_init_context fail：" << stMsg << std::endl;
    }

    /* 加载模型 */
    nRet = noe_load_graph(m_pCtx, m_strModelPath.c_str(), &m_nGraphId);
    if (nRet != NOE_STATUS_SUCCESS)
    {
        noe_get_error_message(m_pCtx, nRet, &stMsg);
        std::cout << "noe_load_graph fail：" << stMsg << std::endl;
    }

    /* 初始化成功 */
    m_bInitialized = true;

    /* 获取模型的输入属性 */
    nRet = noe_get_tensor_count(m_pCtx, m_nGraphId, NOE_TENSOR_TYPE_INPUT, &nInputNum);
    if (nRet != NOE_STATUS_SUCCESS)
    {
        noe_get_error_message(m_pCtx, nRet, &stMsg);
        std::cout << "noe_get_tensor_count: " << stMsg << std::endl;
    }
    m_vInputAttrs.clear();
    for (uint32_t i = 0; i < nInputNum; i++)
    {
        tensor_desc_t strDesc;
        nRet = noe_get_tensor_descriptor(m_pCtx, m_nGraphId, NOE_TENSOR_TYPE_INPUT, i, &strDesc);
        if (nRet != NOE_STATUS_SUCCESS)
        {
            noe_get_error_message(m_pCtx, nRet, &stMsg);
            std::cout << "noe_get_tensor_descriptor: " << stMsg << std::endl;
            goto EXIT;
        }
        m_vInputAttrs.push_back(strDesc);
        std::cout << " 输入ID为 " << strDesc.id << "   大小为: " << strDesc.size
                  << "   缩放比例为: " << strDesc.scale << "    零点值为:" << strDesc.zero_point
                  << "   数据类型为: " << dataTypeString(strDesc.data_type) << std::endl;
    }

    /* 获取模型的输出属性 */
    nRet = noe_get_tensor_count(m_pCtx, m_nGraphId, NOE_TENSOR_TYPE_OUTPUT, &nOutputNum);
    if (nRet != NOE_STATUS_SUCCESS)
    {
        noe_get_error_message(m_pCtx, nRet, &stMsg);
        std::cout << "noe_get_tensor_count: " << stMsg << std::endl;
    }
    m_vOutputAttrs.clear();
    for (uint32_t i = 0; i < nOutputNum; i++)
    {
        tensor_desc_t strDesc;
        nRet = noe_get_tensor_descriptor(m_pCtx, m_nGraphId, NOE_TENSOR_TYPE_OUTPUT, i, &strDesc);
        if (nRet != NOE_STATUS_SUCCESS)
        {
            noe_get_error_message(m_pCtx, nRet, &stMsg);
            std::cout << "noe_get_tensor_descriptor: " << stMsg << std::endl;
            goto EXIT;
        }
        m_vOutputAttrs.push_back(strDesc);
        std::cout << " 输出ID为 " << strDesc.id << "   大小为: " << strDesc.size
                  << "   缩放比例为: " << strDesc.scale << "    零点值为:" << strDesc.zero_point
                  << "   数据类型为: " << dataTypeString(strDesc.data_type) << std::endl;

        void *pData = new char[strDesc.size];
        m_vOutputs.push_back(pData);
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
bool Inference_NS::CCixModelOpt::unInit()
{

    noe_status_t nRet;
    const char *stMsg = nullptr;

    for (int i = 0; i < m_vOutputs.size(); i++)
    {
        delete[] m_vOutputs[i];
    }
    m_vOutputs.clear();

    if (m_bInitialized)
    {
        nRet = noe_unload_graph(m_pCtx, m_nGraphId);
        if (nRet != NOE_STATUS_SUCCESS)
        {
            noe_get_error_message(m_pCtx, nRet, &stMsg);
            std::cout << "noe_unload_graph: " << stMsg << std::endl;
        }
    }

    if (m_pCtx)
    {
        nRet = noe_deinit_context(m_pCtx);
        if (nRet != NOE_STATUS_SUCCESS)
        {
            noe_get_error_message(m_pCtx, nRet, &stMsg);
            std::cout << "m_pCtx上下文释放失败: " << stMsg << std::endl;
        }
    }

    m_bInitialized = false;
    return false;
}

/* 获取模型输入参数 */
bool Inference_NS::CCixModelOpt::getInputAttrs(std::vector<tensor_desc_t> &vInputAttrs)
{
    if (m_bInitialized)
    {
        vInputAttrs = m_vInputAttrs;
        return true;
    }
    return false;
}

/* 获取模型输出参数 */
bool Inference_NS::CCixModelOpt::getOutputAttrs(std::vector<tensor_desc_t> &vOutputAttrs)
{
    if (m_bInitialized)
    {
        vOutputAttrs = m_vOutputAttrs;
        return true;
    }
    return false;
}

/* 运行模型 */
bool Inference_NS::CCixModelOpt::run(
    std::vector<void *> vInputs,
    std::vector<std::vector<float>> &vOutputs)
{
    if (!m_pCtx || !m_bInitialized || m_vInputAttrs.size() != vInputs.size())
    {
        return false;
    }
    int i;
    float fInvScale;
    noe_status_t nRet;
    const char *stMsg = nullptr;
    bool bRet = true;

    int8_t *psInput = nullptr;
    uint8_t *puInput = nullptr;
    int16_t *ps16Input = nullptr;
    uint16_t *pu16Input = nullptr;

    /* 作业配置 */
    uint64_t nJobID;
    job_config_npu_t stNpuConfig = {0};
    stNpuConfig.partition_id = 0;
    stNpuConfig.qos_level = 0;
    stNpuConfig.fm_mem_region = NOE_MEM_REGION_DEFAULT;
    job_config_t stCreateJobCfg = {0};
    stCreateJobCfg.conf_j_npu = &stNpuConfig;
    /* 创建作业 */
    nRet = noe_create_job(m_pCtx, m_nGraphId, &nJobID, &stCreateJobCfg);
    if (nRet != NOE_STATUS_SUCCESS)
    {
        noe_get_error_message(m_pCtx, nRet, &stMsg);
        std::cout << "创建推理作业失败: " << stMsg << std::endl;
        return false;
    }
    try
    {
        /* 拷贝输入数据 */
        for (i = 0; i < m_vInputAttrs.size(); i++)
        {
            if (vInputs[i] == nullptr)
            {
                printf("第[%d]个输入数据为空\n", i);
                bRet = false;
                goto EXIT;
            }
            nRet = noe_load_tensor(m_pCtx, nJobID, i, (void *)vInputs[i]);
            if (nRet != NOE_STATUS_SUCCESS)
            {
                noe_get_error_message(m_pCtx, nRet, &stMsg);
                std::cout << "模型输入[" << i << "]赋值失败: " << stMsg << std::endl;
                bRet = false;
                goto EXIT;
            }
        }

        /* 推理 */
        nRet = noe_job_infer_sync(m_pCtx, nJobID, -1);
        if (nRet != NOE_STATUS_SUCCESS)
        {
            noe_get_error_message(m_pCtx, nRet, &stMsg);
            std::cout << "模型推理失败: " << stMsg << std::endl;
            bRet = false;
            goto EXIT;
        }

        /* 模型输出 */
        for (i = 0; i < m_vOutputAttrs.size(); i++)
        {
            if (m_vOutputs[i] == nullptr)
            {
                printf("第[%d]个输出数据为空\n", i);
                goto EXIT;
            }

            nRet = noe_get_tensor(m_pCtx, nJobID, NOE_TENSOR_TYPE_OUTPUT, i, (void *)m_vOutputs[i]);
            if (nRet != NOE_STATUS_SUCCESS)
            {
                noe_get_error_message(m_pCtx, nRet, &stMsg);
                std::cout << "模型输出[" << i << "]获取失败: " << stMsg << std::endl;
                goto EXIT;
            }

            vOutputs[i].resize(m_vOutputAttrs[i].size);
            fInvScale = 1.0f / m_vOutputAttrs[i].scale;
            if (m_vOutputAttrs[i].data_type == NOE_DATA_TYPE_S8)
            {
                psInput = static_cast<int8_t *>(m_vOutputs[i]);
                dequantize_s8_to_f32_simd(psInput, vOutputs[i].data(), m_vOutputAttrs[i].size, fInvScale, m_vOutputAttrs[i].zero_point);
            }
            else if (m_vOutputAttrs[i].data_type == NOE_DATA_TYPE_U8)
            {
                puInput = static_cast<uint8_t *>(m_vOutputs[i]);
                dequantize_u8_to_f32_simd(puInput, vOutputs[i].data(), m_vOutputAttrs[i].size, fInvScale, m_vOutputAttrs[i].zero_point);
            }
            else if (m_vOutputAttrs[i].data_type == NOE_DATA_TYPE_S16)
            {
                ps16Input = static_cast<int16_t *>(m_vOutputs[i]);
                dequantize_s16_to_f32_simd(ps16Input, vOutputs[i].data(), m_vOutputAttrs[i].size, fInvScale, m_vOutputAttrs[i].zero_point);
            }
            else if (m_vOutputAttrs[i].data_type == NOE_DATA_TYPE_S16)
            {
                pu16Input = static_cast<uint16_t *>(m_vOutputs[i]);
                dequantize_u16_to_f32_simd(pu16Input, vOutputs[i].data(), m_vOutputAttrs[i].size, fInvScale, m_vOutputAttrs[i].zero_point);
            }
            else if (m_vOutputAttrs[i].data_type == NOE_DATA_TYPE_F16)
            {
                pu16Input = static_cast<uint16_t *>(m_vOutputs[i]);
                dequantize_f16_to_f32_simd(pu16Input, vOutputs[i].data(), m_vOutputAttrs[i].size, fInvScale, m_vOutputAttrs[i].zero_point);
                // cv::Mat mat(1, m_vOutputAttrs[i].size, CV_16F, m_vOutputs[i]);
                // mat.convertTo(mat, CV_32F);
                // vOutputs[i].assign(mat.begin<float>(), mat.end<float>());
            }
            else
            {
                printf("输出的数据格式，暂未支持该\n");
                goto EXIT;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "模型推理异常: " << e.what() << std::endl;
        bRet = false;
        goto EXIT;
    }

    bRet = true;
EXIT:
    nRet = noe_clean_job(m_pCtx, nJobID);
    if (nRet != NOE_STATUS_SUCCESS)
    {
        noe_get_error_message(m_pCtx, nRet, &stMsg);
        std::cout << "作业销毁失败\n: " << stMsg << std::endl;
    }
    return bRet;
}

/* 格式打印 */
const char *Inference_NS::CCixModelOpt::dataTypeString(noe_data_type_t stDataType)
{
    switch (stDataType)
    {
    case NOE_DATA_TYPE_NONE:
        return "NONE";
    case NOE_DATA_TYPE_BOOL:
        return "BOOL";
    case NOE_DATA_TYPE_U8:
        return "U8";
    case NOE_DATA_TYPE_S8:
        return "S8";
    case NOE_DATA_TYPE_U16:
        return "U16";
    case NOE_DATA_TYPE_S16:
        return "S16";
    case NOE_DATA_TYPE_U32:
        return "U32";
    case NOE_DATA_TYPE_S32:
        return "S32";
    case NOE_DATA_TYPE_U64:
        return "U64";
    case NOE_DATA_TYPE_S64:
        return "S64";
    case NOE_DATA_TYPE_F16:
        return "F16";
    case NOE_DATA_TYPE_F32:
        return "F32";
    case NOE_DATA_TYPE_F64:
        return "F64";
    case NOE_DATA_TYPE_BF16:
        return "BF16";
    case NOE_DATA_TYPE_ALIGNED_U4:
        return "ALIGNED_U4";
    case NOE_DATA_TYPE_ALIGNED_S4:
        return "ALIGNED_S4";
    case NOE_DATA_TYPE_ALIGNED_U12:
        return "ALIGNED_U12";
    case NOE_DATA_TYPE_ALIGNED_S12:
        return "ALIGNED_S12";
    case NOE_DATA_TYPE_COMPACT_U4:
        return "COMPACT_U4";
    case NOE_DATA_TYPE_COMPACT_S4:
        return "COMPACT_S4";
    case NOE_DATA_TYPE_COMPACT_U12:
        return "COMPACT_U12";
    case NOE_DATA_TYPE_COMPACT_S12:
        return "COMPACT_S12";
    default:
        return "UNKNOWN";
    }
}