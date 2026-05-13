/**
 * @file YoloUltralytics.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 *
 * @brief
 */
#include "YoloUltralytics.hpp"
#include <memory>
#include <cstring>

Inference_NS::CYoloUltralytics::CYoloUltralytics(std::string strConfigPath)
    : CCVInferenceHISI(strConfigPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcess_NS::cYOLOV8PostProcess;
}

Inference_NS::CYoloUltralytics::~CYoloUltralytics()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool Inference_NS::CYoloUltralytics::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    /* 输出数据清空 */
    if (!vBoxDatas.empty())
    {
        vBoxDatas.clear();
    }

    if (!m_pModel || !m_postProcess)
    {
        return false;
    }

    /* 解析输入的数据结构体 */
    float fBoxThreshold = m_fBoxThreshold;
    float fNmsThreshold = m_fNmsThreshold;
    if (stInputData.stBoxs.fConfidence > 0 && stInputData.stBoxs.fConfidence < 1)
    {
        fBoxThreshold = stInputData.stBoxs.fConfidence;
    }
    if (stInputData.stBoxs.fNms > 0 && stInputData.stBoxs.fNms < 1)
    {
        fNmsThreshold = stInputData.stBoxs.fNms;
    }
    if(!stInputData.pData)
    {
        printf("输入的pData数据为空\n");
        return false;
    }

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInputData.nDataSize);
    if (!bIsInfe)
    {
        return false;
    }
    
    /* 填充数据 */
    setInputDatas((unsigned char*)stInputData.pData, 0);
    /* 运行 */
    if (!m_pModel->run(m_pInputs, m_pOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    // /* 将所有的模型输出头，加入容器中 */
    // std::vector<float *> vInput;
    // for (size_t i = 0; i < m_nOutputNum; ++i)
    // {
    //     printf("Output %zu dims: ", i);
    //     for (int j = 0; j < m_vOutputDims[i].size(); ++j) {
    //         printf("%d ", m_vOutputDims[i][j]);
    //     }
    //     printf("\n");

    //     /* 获取数据的虚拟地址 */
    //     void *dataBuf = svp_acl_get_data_buffer_addr(m_vOutputBuffers[i]);
    //     float *pOut = reinterpret_cast<float *>(dataBuf);
    //     vInput.push_back(pOut);
    // }

    std::vector<float *> vInput;
    std::vector<size_t> confIndices = {6, 7, 8, 6, 4, 5, 0, 1, 2};
    for (size_t i : confIndices)
    {
        printf("Output %zu dims: ", i);
        for (int j = 0; j < m_vOutputDims[i].size(); ++j) {
            printf("%d ", m_vOutputDims[i][j]);
        }
        printf("\n");

        /* 获取数据的虚拟地址 */
        void *dataBuf = svp_acl_get_data_buffer_addr(m_vOutputBuffers[i]);
        float *pOut = reinterpret_cast<float *>(dataBuf);
        vInput.push_back(pOut);
    }

    /* 后处理 */
    m_nCLASS_NUM = m_vOutputDims[1][1];
    int nFlLen =  m_vOutputDims[0][1] / 4;


    /* 模型输出头 为9 的后处理 */
    if ((m_nOutputNum / 3) == 3)
    {
        m_postProcess->postProcessDetect(
            vInput,
            m_nLimitHeight,
            m_nLimitWidth,
            m_fBoxThreshold,
            m_fNmsThreshold,
            m_nCLASS_NUM,
            vBoxDatas,
            nFlLen);
    } 
    /* 模型输出头 为12 的后处理 */
    else if ((m_nOutputNum / 3) == 4)
    {
        m_postProcess->postProcessLiteDetect(
            vInput,
            m_nLimitHeight,
            m_nLimitWidth,
            m_fBoxThreshold,
            m_fNmsThreshold,
            m_nCLASS_NUM,
            vBoxDatas,
            nFlLen);
    }
    else
    {
        printf("模型输出数量[%d]的后处理函数不存在\n", m_nOutputNum);
        return false;
    }
    m_pModel->DestroyInput(m_pInputs);

    return true;
}

/* 设置参数 */
bool Inference_NS::CYoloUltralytics::setParam(float fBoxThreshold, float fNmsThreshold)
{
    if (m_fBoxThreshold != -1)
    {
        if (m_fBoxThreshold >= 0 && m_fBoxThreshold <= 1)
        {
            m_fBoxThreshold = fBoxThreshold;
        }
        else
        {
            printf("fBoxThreshold参数的值不在指定范围：[%f]\n", fBoxThreshold);
            return false;
        }
    }
    if (fNmsThreshold != -1)
    {
        if (fNmsThreshold >= 0 && fNmsThreshold <= 1)
        {
            m_fNmsThreshold = fNmsThreshold;
        }
        else
        {
            printf("fNmsThreshold参数的值不在指定范围：[%f]\n", fNmsThreshold);
            return false;
        }
    }
    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CYoloUltralytics::checkModelProConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File) 
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return false;
    }
    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char* pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    Json::Object* pJsonHandle = NULL;
    Json::Object* pJsonData   = NULL;
    bool          bRet        = false;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "post_precess");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        goto EXIT;
    }
    /* 1、置信度 */
    bRet = Json::get(pJsonData, "confidence", m_fBoxThreshold);
    if (!bRet)
    {
        printf("解析confidence字段失败\n");
        goto EXIT;
    }
    /* 2、非极大值抑制阈值 */
    bRet = Json::get(pJsonData, "nms", m_fNmsThreshold);
    if (!bRet)
    {
        printf("解析nms字段失败\n");
        goto EXIT;
    }
    return true;

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return false;
}