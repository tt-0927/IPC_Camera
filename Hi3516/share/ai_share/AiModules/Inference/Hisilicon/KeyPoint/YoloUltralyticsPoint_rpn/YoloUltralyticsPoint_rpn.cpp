/**
 * @file YoloUltralyticsPoint.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-14
 *
 * @brief
 */
#include "YoloUltralyticsPoint_rpn.hpp"
#include <memory>
#include <cstring>

Inference_NS::CYoloUltralyticsPoint::CYoloUltralyticsPoint(std::string strConfigPath)
    : CCVInferenceHISI(strConfigPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcess_NS::cYOLOV8PostProcessHisi;
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
    std::vector<Inference_NS::PointData_S>& vPointDatas)
{
    /* 输出数据清空 */
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

    // 获取检测框数量（第3个输出索引）
    const uint8_t outNumIdx = 3;
    svp_acl_data_buffer* outNumBuffer = svp_acl_mdl_get_dataset_buffer(m_pOutputs, outNumIdx);
    const int nOutNum = static_cast<int>(reinterpret_cast<float*>(svp_acl_get_data_buffer_addr(outNumBuffer))[0]); // 检测框数量
    if (nOutNum <= 0)
    {
        return true;
    }
    // 获取置信度分数（第2个输出索引）
    const uint8_t scoreValIdx = 2;
    svp_acl_data_buffer* scoreBuffer = svp_acl_mdl_get_dataset_buffer(m_pOutputs, scoreValIdx);
    float* scoreData = reinterpret_cast<float*>(svp_acl_get_data_buffer_addr(scoreBuffer));
    // 获取得分最高框的索引（第4个输出索引）
    const uint8_t scoreIndexIdx = 4;
    svp_acl_data_buffer* maxScoreIdxBuffer = svp_acl_mdl_get_dataset_buffer(m_pOutputs, scoreIndexIdx);
    uint16_t* maxScoreIdx = reinterpret_cast<uint16_t*>(svp_acl_get_data_buffer_addr(maxScoreIdxBuffer));
    // 获取边框坐标信息（第1个输出索引）
    const uint8_t coordIdx = 1;
    svp_acl_data_buffer* coordBuffer = svp_acl_mdl_get_dataset_buffer(m_pOutputs, coordIdx);
    float* coord = reinterpret_cast<float*>(svp_acl_get_data_buffer_addr(coordBuffer));
    // 获取类别ID信息（第5个输出索引）
    const uint8_t maxClassIdx = 5;
    svp_acl_data_buffer* maxClassBuffer = svp_acl_mdl_get_dataset_buffer(m_pOutputs, maxClassIdx);
    uint16_t* maxClass = reinterpret_cast<uint16_t*>(svp_acl_get_data_buffer_addr(maxClassBuffer));
    // 获取类别ID信息（第0个输出索引）
    const uint8_t kptIdx = 0;
    svp_acl_data_buffer* kptBuffer = svp_acl_mdl_get_dataset_buffer(m_pOutputs, kptIdx);
    float* kptData = reinterpret_cast<float*>(svp_acl_get_data_buffer_addr(kptBuffer));

    int nKptNumPerBox = m_vOutputDims[0][2] / 3;
    uint32_t coordNum = m_vOutputDims[1][3];

    // printf("关键点的维度为 [%d], 目标框的coordNum为 [%zu] \n", nKptNumPerBox, coordNum);
 
    m_postProcess->postProcessKeyPoint(
            nOutNum,
            scoreData,
            maxScoreIdx,
            coord,
            maxClass,
            kptData,
            m_nLimitHeight,
            m_nLimitWidth,
            nKptNumPerBox,
            coordNum,
            fBoxThreshold,
            fNmsThreshold,
            vPointDatas);
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