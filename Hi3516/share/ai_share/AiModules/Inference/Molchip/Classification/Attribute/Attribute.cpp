/**
 * @file AttrbuteRec.cpp
 * @author caishengjie (caisj@kfb.cn)
 * @date 2024-10-09
 *
 * @brief
 */
#include "Attribute.hpp"
#include <cstring>

Inference_NS::CAttribute::CAttribute(std::string strConfigPath)
    : CCVInferenceMOL(strConfigPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcess_NS::cAttributePostProcess;
}

Inference_NS::CAttribute::~CAttribute()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

bool Inference_NS::CAttribute::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::ClsData_S> &vClsDatas)
{
    /* 输出数据清空 */
    if (!vClsDatas.empty())
    {
        vClsDatas.clear();
    }

    if (!m_pModel || !m_postProcess)
    {
        return false;
    }

    /* 解析输入的数据结构体 */
    float fConfThreshold = m_fConfThreshold;
    if (stInputData.stBoxs.fConfidence > 0 && stInputData.stBoxs.fConfidence < 1)
    {
        fConfThreshold = stInputData.stBoxs.fConfidence;
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
    
    /* 进行同属性过滤 */
    m_postProcess->setParam(m_nClassNum, m_vGroupOnce);
#if 0
    /* 模型性能测试，输入原始的输出 */
    if (bPerformanceTest)
    {
        m_postProcess->setParam(m_nClassNum, std::vector<std::vector<int>>());
    }
#endif

    /* 后处理 */
    std::vector<float> vResult;
    bool bFlag = m_postProcess->postProcess(
        vInput[0],
        fConfThreshold,
        vResult);
#if 0
    if(!bFlag)
    {
        printf("属性识别后处理调用失败\n");
    }
    /* 性能测试模式 */
    if(bPerformanceTest)
    {
        vTestResult.clear();
        vTestResult.assign(vResult.begin(), vResult.end());
    }
#endif

    Inference_NS::ClsData_S stOutData;
    for (int nIndex = 0; nIndex < vResult.size(); nIndex++)
    {
        if (vResult[nIndex] > 0)
        {
            Inference_NS::Cls_S stData;
            stData.nLabel = nIndex;
            stData.fConfidence = vResult[nIndex];
            stOutData.vCls.push_back(stData);
        }
    }
    vClsDatas.push_back(stOutData);

    return true;
}

bool Inference_NS::CAttribute::setParam(float fThreshold)
{
    if (0 <= fThreshold && fThreshold <= 1.0)
    {
        m_fConfThreshold = fThreshold;
    }

    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CAttribute::checkModelProConfig()
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
    Json::Object* pObjectItem   = NULL;
    Json::Object* pItemOne   = NULL;
    bool          bRet        = false;
    int i,j, nISize, nJSize, nSizeItem;
    std::vector<int> vGroupOnce;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "post_precess");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        goto EXIT;
    }
    /* 1、置信度 */
    bRet = Json::get(pJsonData, "confidence", m_fConfThreshold);
    if (!bRet)
    {
        printf("解析confidence字段失败\n");
        goto EXIT;
    }
    /* 2、属性别数 */
    bRet = Json::get(pJsonData, "cls_num", m_nClassNum);
    if (!bRet || m_nClassNum<=0)
    {
        printf("解析cls_num字段失败\n");
        goto EXIT;
    }
    /* 3、同属性类别组 */
    pJsonObject = Json::get(pJsonData, "cls_group");
    if (! pJsonObject)
    {
        printf("解析cls_group字段失败\n");
        goto EXIT;
    }
    nISize = Json::Array::size(pJsonObject);
    if (nISize <= 0)
    {
        printf("解析[数组大小异常]\n");
        goto EXIT;
    }
    for (i = 0; i < nISize; i++)
    {
        /* 获取数组的节点 */
        pObjectItem = Json::Array::get(pJsonObject, i);
        if (NULL == pObjectItem)
        {
            printf("获取第一层数组节点失败\n");
            goto EXIT;
        }

        nJSize = Json::Array::size(pObjectItem);
        if (nJSize <= 0)
        {
            printf("解析[数组大小异常]\n");
            goto EXIT;
        }
        vGroupOnce.clear();
        for (j = 0; j < nJSize; j++)
        {
            /* 获取数组的节点 */
            pItemOne = Json::Array::get(pObjectItem, j);
            if (NULL == pItemOne)
            {
                printf("获取第二层数组节点失败\n");
                goto EXIT;
            }
            bRet = Json::get(pItemOne, nSizeItem);
            if (!bRet)
            {
                printf("解析[size]字段失败\n");
                goto EXIT;
            }
            vGroupOnce.push_back(nSizeItem);
        }
        m_vGroupOnce.push_back(vGroupOnce);
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
