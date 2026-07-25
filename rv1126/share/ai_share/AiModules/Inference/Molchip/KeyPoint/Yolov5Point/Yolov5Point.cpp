/**
 * @file Yolov5Point.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-28
 * 
 * @brief Yolov5关键点检测
 */
#include "Yolov5Point.hpp"

#include <cstring>

Inference_NS::CYolov5Point::CYolov5Point(std::string strConfigPath)
    : CCVInferenceMOL(strConfigPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcess_NS::cYOLOV5PostProcess;
}

Inference_NS::CYolov5Point::~CYolov5Point()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool Inference_NS::CYolov5Point::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::PointData_S>& vPointDatas)
{
    /* 输出数据清空 */
    if (!vPointDatas.empty())
    {
        vPointDatas.clear();
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
    std::memcpy(reinterpret_cast<void *>(m_vInputs[0].dataIn.virAddr), stInputData.pData, m_vInputs[0].dataIn.size);

    /* 运行 */
    if (!m_pModel->run(m_vInputs,
                        m_vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    for (int i = 0; i < m_stModelDesc.ioDesc.outputNum; ++i)
    {
        float *pOut = reinterpret_cast<float *>(m_vOutputs[i].dataOut.virAddr);
        vInput.push_back(pOut);
    }
    
    /* 后处理 */
    m_nClassNum = (m_stModelDesc.ioDesc.out[0].tensor.dims[1] / 3) - 5 - m_nPointNum * 2;
    m_postProcess->postProcessPoint(
        vInput,
        m_nLimitHeight,
        m_nLimitWidth,
        m_fBoxThreshold,
        m_fNmsThreshold,
        m_nClassNum,
        m_nPointNum,
        vPointDatas);

    return true;
}

/* 设置参数 */
bool Inference_NS::CYolov5Point::setParam(float fBoxThreshold, float fNmsThreshold)
{
    if (m_fBoxThreshold != -1)
    {
        if (m_fBoxThreshold >= 0 && m_fBoxThreshold <= 1)
        {
            m_fBoxThreshold = fBoxThreshold;
        } else{
            printf("fBoxThreshold参数的值不在指定范围：[%f]\n", fBoxThreshold);
            return false;
        }
    }
    if (fNmsThreshold != -1)
    {
        if (fNmsThreshold >= 0 && fNmsThreshold <= 1)
        {
            m_fNmsThreshold = fNmsThreshold;
        } else{
            printf("fNmsThreshold参数的值不在指定范围：[%f]\n", fNmsThreshold);
            return false;
        }
    }
    return true;
}


/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CYolov5Point::checkModelProConfig()
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
    Json::Object* pJsonObject   = NULL;
    Json::Object* pItemObject   = NULL;
    int i,nSizeItem;
    int nSize = 0;
    bool          bRet        = true;
    std::vector<int> vAnchors;

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
    /* 3、关键点的数量 */
    bRet = Json::get(pJsonData, "point_num", m_nPointNum);
    if (!bRet || m_nPointNum<=0)
    {
        printf("解析point_num字段失败\n");
        goto EXIT;
    }
    /* 4、解析anchors（可选） */
    pJsonObject = Json::get(pJsonData, "anchors");
    if (!pJsonObject)
    {
        printf("解析anchors字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonObject);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }

    for (i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonObject, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nSizeItem);
        if (!bRet)
        {
            printf("解析[size]字段失败\n");
            goto EXIT;
        }
        vAnchors.push_back(nSizeItem);
    }
    
    /* 将anchor赋值给后处理 */
    if(vAnchors.size() / 6 == 3)
    {
        std::copy(vAnchors.begin(), vAnchors.begin() + 6, std::begin(m_postProcess->m_nAnchor0));
        std::copy(vAnchors.begin() + 6, vAnchors.begin() + 12, std::begin(m_postProcess->m_nAnchor1));
        std::copy(vAnchors.begin() + 12, vAnchors.begin() + 18, std::begin(m_postProcess->m_nAnchor2)); 
    }
    else
    {
        printf("anchors赋值给模型失败\n");
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}