/**
 * @file Yolov5.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#include "Yolov5.hpp"
#include <chrono>

#include <cstring>

Inference_NS::CYolov5::CYolov5(std::string strConfigPath)
    : CCVInferenceOnnx(strConfigPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcess_NS::cYOLOV5PostProcess;
}

Inference_NS::CYolov5::~CYolov5()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool Inference_NS::CYolov5::inference(
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

    /* 模型推理 */
    std::vector<float *> vInput = {reinterpret_cast<float *>(stInputData.pData)};
    std::vector<int64_t> nInputDataSizes = {static_cast<int64_t>(stInputData.nDataSize)};
    std::vector<float *> vOutputs;
    if (!m_pModel->run(
            vInput,
            nInputDataSizes,
            vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 后处理 */
    m_nCLASS_NUM = (m_vOutputAttrs[0][1] / 3) - 5;
    m_postProcess->postProcess(
        vOutputs,
        m_nLimitHeight,
        m_nLimitWidth,
        fBoxThreshold,
        fNmsThreshold,
        m_nCLASS_NUM,
        vBoxDatas);

    return true;
}

/* 设置参数 */
bool Inference_NS::CYolov5::setParam(float fBoxThreshold, float fNmsThreshold)
{
    if (m_fBoxThreshold != -1)
    {
        if (m_fBoxThreshold >= 0 && m_fBoxThreshold <= 1)
        {
            m_fBoxThreshold = fBoxThreshold;
        }
        else
        {
            printf("fBoxThreshold参数的值不在指定范围: [%f]\n", fBoxThreshold);
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
            printf("fNmsThreshold参数的值不在指定范围: [%f]\n", fNmsThreshold);
            return false;
        }
    }
    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CYolov5::checkModelProConfig()
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
    bool          bRet        = true;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "post_precess");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        bRet = false;
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

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}