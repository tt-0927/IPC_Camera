/**
 * @file YoloUltralyticsPoint.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-28
 *
 * @brief YoloUltralytics关键的检测框架
 */
#include "YoloUltralyticsPoint.hpp"
#include <memory>
#include <cstring>
#include <algorithm>

Inference_NS::CYoloUltralyticsPoint::CYoloUltralyticsPoint(std::string strConfigPath)
    : CCVInferenceCix(strConfigPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcess_NS::cYOLOV8PostProcess;
}

Inference_NS::CYoloUltralyticsPoint::~CYoloUltralyticsPoint()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool Inference_NS::CYoloUltralyticsPoint::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::PointData_S> &vPointDatas)
{
    /* 输出数据清空 */
    if (!vPointDatas.empty())
    {
        vPointDatas.clear();
    }

    if (!m_pModel || !m_postProcess)
    {
        printf("CYoloUltralyticsPoint 未初始化\n");
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
    if (!stInputData.pData)
    {
        printf("CYoloUltralyticsPoint 输入的pData数据为空\n");
        return false;
    }

    /* 使用模型推理前的相关变量判断 */
    stInputData.nDataSize /= sizeof(float); /* 计算元素个数 */
    bool bIsInfe = inferenceInfe(0, stInputData.nDataSize);
    if (!bIsInfe)
    {
        printf("CYoloUltralyticsPoint 使用模型推理前的相关变量判断失败\n");
        return false;
    }

    /* 填充数据 */
    std::vector<void *> vInputData;
    vInputData.push_back((void *)stInputData.pData);
    std::vector<std::vector<float>> vOutputs(m_vOutputAttrs.size());
    /* 运行 */
    if (!m_pModel->run(vInputData, vOutputs))
    {
        printf("CYoloUltralyticsPoint 推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    for (int i = 0; i < m_vOutputAttrs.size(); ++i)
    {
        vInput.push_back(vOutputs[i].data());
    }

    /* 后处理 */
    if (m_vOutputAttrs.size() != m_vOutSizes.size())
    {
        printf("CYoloUltralyticsPoint json文件的 output_shape 数组长度[%ld] 不等于 模型输入个数[%ld]\n", m_vOutSizes.size(), m_vOutputAttrs.size());
        return false;
    }
    bool bPointShow = true;
    int nSPointNum = bPointShow ? 3 : 2;
    int nFlLen = m_vOutSizes[0][1] / 4;
    m_nCLASS_NUM = m_vOutSizes[1][1];
    m_nKeyPoint_NUM = m_vOutSizes[9][1] / nSPointNum;
    m_postProcess->postProcessKeyPoint(
        vInput,
        m_nLimitHeight,
        m_nLimitWidth,
        fBoxThreshold,
        fNmsThreshold,
        m_nCLASS_NUM,
        m_nKeyPoint_NUM,
        vPointDatas,
        nFlLen,
        bPointShow);

    return true;
}

/* 设置参数 */
bool Inference_NS::CYoloUltralyticsPoint::setParam(float fBoxThreshold, float fNmsThreshold)
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
bool Inference_NS::CYoloUltralyticsPoint::checkModelProConfig()
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
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    bool bRet = true;

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