/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_quality_model/face_quality_model.cpp
 * @Date         : 2026-10-20
 * @Description  : 人脸质量评估纯组件实现
 */

#include "face_quality_model.hpp"

#include <algorithm>
#include <vector>
#include "ImageFeature.hpp"
#include "dlog.h"

CFaceQualityModel::CFaceQualityModel(const std::string &strJsonConfigPath)
    : m_strJsonConfigPath(strJsonConfigPath)
{
}

CFaceQualityModel::~CFaceQualityModel()
{
    unInit();
}

bool CFaceQualityModel::init()
{
    /* 重复 init 先释放旧句柄，保证可重试 */
    if (m_pInferHandle != nullptr)
    {
        unInit();
    }

    m_pInferHandle = new Inference_NS::CImageFeature(m_strJsonConfigPath);
    if (m_pInferHandle == nullptr || !m_pInferHandle->init())
    {
        dlog_error("FACE_QUALITY: 模型初始化失败 [%s]", m_strJsonConfigPath.c_str());
        unInit();
        return false;
    }

    /* 取模型实际输入尺寸，换模型不改代码 */
    m_pInferHandle->getSizeLimit(0, m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);

    dlog_info("FACE_QUALITY: 模型初始化成功 [%s] 输入尺寸[%dx%d]",
              m_strJsonConfigPath.c_str(), m_nLimitWidth, m_nLimitHeight);
    return true;
}

bool CFaceQualityModel::unInit()
{
    if (m_pInferHandle != nullptr)
    {
        delete m_pInferHandle;
        m_pInferHandle = nullptr;
    }
    return true;
}

int CFaceQualityModel::process(const InData_S &stInData, float &fScore)
{
    fScore = 0.0f;

    if (m_pInferHandle == nullptr)
    {
        dlog_error("FACE_QUALITY: 模型未初始化，无法评估");
        return -1;
    }

    if (stInData.inMat.empty() || stInData.inMat.type() != CV_8UC3)
    {
        dlog_error("FACE_QUALITY: 输入图片为空或非 CV_8UC3 RGB 格式");
        return -2;
    }

    /* 尺寸不一致时等比缩放贴黑边（对齐旧 FaceQualityAssessmentV1_0，无条件缩放） */
    cv::Mat matInput;
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        if (!resizeAndPad(stInData.inMat, matInput))
        {
            return -3;
        }
    }
    else
    {
        matInput = stInData.inMat;
    }

    /* 组装推理输入：
       pData 实为 uint8 RGB 数据，强转 float* 与 nDataSize 乘 sizeof(float)
       是推理库内部校验的历史约定（对照 face_expression.cpp），照抄勿改 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData     = (float *)matInput.data;
    stInputData.nDataSize = static_cast<int>(matInput.total() * matInput.elemSize() * sizeof(float));

    /* 推理（内部完成 rknn 推理 + 特征解码） */
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    if (!m_pInferHandle->inference(stInputData, vClsDatas) || vClsDatas.empty())
    {
        dlog_error("FACE_QUALITY: 推理失败");
        return -4;
    }

    /* 评分 = 特征向量前 10 维均值（对齐旧 FaceQualityAssessmentV1_0） */
    const std::vector<float> &vFeature = vClsDatas[0].vFeature;
    const size_t nCount = std::min<size_t>(10, vFeature.size());
    float fSum = 0.0f;
    for (size_t i = 0; i < nCount; ++i)
    {
        fSum += vFeature[i];
    }
    if (nCount > 0)
    {
        fScore = fSum / static_cast<float>(nCount);
    }
    return 0;
}

bool CFaceQualityModel::resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage)
{
    if (inputImage.empty())
    {
        dlog_error("FACE_QUALITY: 输入图片为空，无法贴边");
        return false;
    }

    /* 长边贴模型输入尺寸，等比缩放 */
    const float fScale = static_cast<float>(m_nLimitWidth) /
                         static_cast<float>(std::max(inputImage.cols, inputImage.rows));
    const int nNewWidth = static_cast<int>(inputImage.cols * fScale);
    const int nNewHeight = static_cast<int>(inputImage.rows * fScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(nNewWidth, nNewHeight));

    /* 居中贴黑边 */
    const int nPadX = (m_nLimitWidth - nNewWidth) / 2;
    const int nPadY = (m_nLimitHeight - nNewHeight) / 2;
    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());
    resizedImage.copyTo(output(cv::Rect(nPadX, nPadY, nNewWidth, nNewHeight)));

    outputImage = output;
    return true;
}
