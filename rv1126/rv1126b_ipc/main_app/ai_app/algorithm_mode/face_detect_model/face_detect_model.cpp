/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_detect_model/face_detect_model.cpp
 * @Date         : 2026-10-20
 * @Description  : 人脸检测纯组件实现
 */

#include "face_detect_model.hpp"

#include <algorithm>
#include "YoloUltralyticsPoint.hpp"
#include "dlog.h"

CFaceDetectModel::CFaceDetectModel(const std::string &strJsonConfigPath)
    : m_strJsonConfigPath(strJsonConfigPath)
{
}

CFaceDetectModel::~CFaceDetectModel()
{
    unInit();
}

bool CFaceDetectModel::init()
{
    /* 重复 init 先释放旧句柄，保证可重试 */
    if (m_pInferHandle != nullptr)
    {
        unInit();
    }

    m_pInferHandle = new Inference_NS::CYoloUltralyticsPoint(m_strJsonConfigPath);
    if (m_pInferHandle == nullptr || !m_pInferHandle->init())
    {
        dlog_error("FACE_DETECT: 模型初始化失败 [%s]", m_strJsonConfigPath.c_str());
        unInit();
        return false;
    }

    /* 取模型实际输入尺寸，换模型不改代码 */
    m_pInferHandle->getSizeLimit(0, m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);

    dlog_info("FACE_DETECT: 模型初始化成功 [%s] 输入尺寸[%dx%d]",
              m_strJsonConfigPath.c_str(), m_nLimitWidth, m_nLimitHeight);
    return true;
}

bool CFaceDetectModel::unInit()
{
    if (m_pInferHandle != nullptr)
    {
        delete m_pInferHandle;
        m_pInferHandle = nullptr;
    }
    return true;
}

int CFaceDetectModel::process(const InData_S &stInData, std::vector<FaceDetectedTarget_S> &vecResults)
{
    if (!vecResults.empty())
    {
        vecResults.clear();
    }

    if (m_pInferHandle == nullptr)
    {
        dlog_error("FACE_DETECT: 模型未初始化，无法检测");
        return -1;
    }

    if (stInData.inMat.empty() || stInData.inMat.type() != CV_8UC3)
    {
        dlog_error("FACE_DETECT: 输入图片为空或非 CV_8UC3 RGB 格式");
        return -2;
    }

    /* 贴边到模型输入尺寸 */
    float fScale = 1.0f;
    int nPadX = 0;
    int nPadY = 0;
    cv::Mat matInput;
    if (!resizeAndPad(stInData.inMat, matInput, fScale, nPadX, nPadY))
    {
        return -3;
    }

    /* 组装推理输入：
       pData 实为 uint8 RGB 数据，强转 float* 与 nDataSize 乘 sizeof(float)
       是推理库内部校验的历史约定（对照 fire_person_detect.cpp），照抄勿改 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData              = (float *)matInput.data;
    stInputData.nDataSize          = static_cast<int>(matInput.total() * matInput.elemSize() * sizeof(float));
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms        = stInData.stParam.fNmsThreshold;

    /* 推理（内部完成 rknn 推理 + 解码 + NMS）；
       阈值 <=0 时推理库内部回落到 json 配置的默认值 */
    std::vector<Inference_NS::PointData_S> vPointDatas;
    if (!m_pInferHandle->inference(stInputData, vPointDatas))
    {
        dlog_error("FACE_DETECT: 推理失败");
        return -4;
    }

    /* 结果转换 + 坐标还原（推理输出坐标在贴边坐标系，框与关键点同一套还原公式，
       与旧 FaceDetectV1_0 的 RGB 分支逐行对应） */
    vecResults.reserve(vPointDatas.size());
    for (const auto &stPointData : vPointDatas)
    {
        FaceDetectedTarget_S stResult = {};
        const Inference_NS::Box_S &stBox = stPointData.stBoxs;
        stResult.fX1         = static_cast<float>(std::max(0, stBox.nX1 - nPadX)) / fScale;
        stResult.fY1         = static_cast<float>(std::max(0, stBox.nY1 - nPadY)) / fScale;
        stResult.fX2         = static_cast<float>(std::max(0, stBox.nX2 - nPadX)) / fScale;
        stResult.fY2         = static_cast<float>(std::max(0, stBox.nY2 - nPadY)) / fScale;
        stResult.fConfidence = stPointData.fConfidence;

        const size_t nPointCount = std::min(stPointData.vPoints.size(),
                                            static_cast<size_t>(FACE_DETECT_POINT_NUM));
        for (size_t i = 0; i < nPointCount; ++i)
        {
            const Inference_NS::Point_S &stPoint = stPointData.vPoints[i];
            stResult.vPoint[i * 2]     = static_cast<float>(std::max(0, stPoint.nX - nPadX)) / fScale;
            stResult.vPoint[i * 2 + 1] = static_cast<float>(std::max(0, stPoint.nY - nPadY)) / fScale;
        }
        vecResults.push_back(stResult);
    }
    return 0;
}

bool CFaceDetectModel::resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage,
                                    float &fScale, int &nPadX, int &nPadY)
{
    if (inputImage.empty())
    {
        dlog_error("FACE_DETECT: 输入图片为空，无法贴边");
        return false;
    }

    /* 仅超限时按长边缩小，小图保持原尺寸（对齐旧 FaceDetectV1_0） */
    int nNewWidth = inputImage.cols;
    int nNewHeight = inputImage.rows;
    cv::Mat resizedImage = inputImage;
    if (inputImage.cols > m_nLimitWidth || inputImage.rows > m_nLimitHeight)
    {
        fScale = static_cast<float>(m_nLimitWidth) /
                 static_cast<float>(std::max(inputImage.cols, inputImage.rows));
        nNewWidth = static_cast<int>(inputImage.cols * fScale);
        nNewHeight = static_cast<int>(inputImage.rows * fScale);
        cv::resize(inputImage, resizedImage, cv::Size(nNewWidth, nNewHeight));
    }
    else
    {
        fScale = 1.0f;
    }

    /* 居中偏移量（黑边补在两侧，与缩放比例一并用于坐标还原） */
    nPadX = (m_nLimitWidth - nNewWidth) / 2;
    nPadY = (m_nLimitHeight - nNewHeight) / 2;

    /* 固定尺寸黑色背景，图像居中放置 */
    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());
    resizedImage.copyTo(output(cv::Rect(nPadX, nPadY, nNewWidth, nNewHeight)));

    outputImage = output;
    return true;
}
