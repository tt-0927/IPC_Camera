/*
 * @Author: zhengxh
 * @Date: 2026-10-22 08:56:48
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-22 09:07:51
 * @Description: 厨房14类别检测纯组件
 */


#include "kitchen_detect.hpp"

#include <algorithm>
#include <unistd.h>
#include "YoloUltralytics.hpp"
#include "dlog.h"

/* 类别名称表（labelToString 专用，下标与类别 ID 一致） */
static const std::string KITCHEN_CLASS_NAME[] = {
    "蟑螂", "发网", "未戴手套", "未戴厨师帽", "老鼠", "戴口罩", "未戴口罩",
    "抽烟", "手机", "满溢", "垃圾", "垃圾桶", "厨师工作服", "厨师帽"
};

/* 当系统中存在 /printAlgoResults 文件时打印检测结果 */
static void printDetectResults(const std::vector<DetectedTarget_S> &vecResults)
{
    if (access("/printAlgoResults", F_OK))
    {
        return;
    }

    dlog_info("KITCHEN_DETECT: 检测到 %d 个目标", (int)vecResults.size());
    for (size_t i = 0; i < vecResults.size(); i++)
    {
        const DetectedTarget_S &stResult = vecResults[i];
        dlog_info("  [%zu] 类别=%s(%d) conf=%.4f bbox=(%.1f, %.1f, %.1f, %.1f)",
                  i, CKitchenDetector::labelToString(stResult.nLabel).c_str(), stResult.nLabel,
                  stResult.fConfidence, stResult.fX1, stResult.fY1, stResult.fX2, stResult.fY2);
    }
}

CKitchenDetector::CKitchenDetector(const std::string &strJsonConfigPath)
    : m_strJsonConfigPath(strJsonConfigPath)
{
}

CKitchenDetector::~CKitchenDetector()
{
    unInit();
}

bool CKitchenDetector::init()
{
    /* 重复 init 先释放旧句柄，保证可重试 */
    if (m_pYoloHandle != nullptr)
    {
        unInit();
    }

    m_pYoloHandle = new Inference_NS::CYoloUltralytics(m_strJsonConfigPath);
    if (m_pYoloHandle == nullptr || !m_pYoloHandle->init())
    {
        dlog_error("KITCHEN_DETECT: 模型初始化失败 [%s]", m_strJsonConfigPath.c_str());
        unInit();
        return false;
    }

    /* 取模型实际输入尺寸，换模型不改代码 */
    m_pYoloHandle->getSizeLimit(0, m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);

    dlog_info("KITCHEN_DETECT: 模型初始化成功 [%s] 输入尺寸[%dx%d]",
              m_strJsonConfigPath.c_str(), m_nLimitWidth, m_nLimitHeight);
    return true;
}

bool CKitchenDetector::unInit()
{
    if (m_pYoloHandle != nullptr)
    {
        delete m_pYoloHandle;
        m_pYoloHandle = nullptr;
    }
    return true;
}

/* 类别 ID 转名称 */
std::string CKitchenDetector::labelToString(int label)
{
    if (label < 0 || label >= (int)(sizeof(KITCHEN_CLASS_NAME) / sizeof(KITCHEN_CLASS_NAME[0])))
    {
        return "未知";
    }
    return KITCHEN_CLASS_NAME[label];
}

int CKitchenDetector::process(const InData_S &stInData, std::vector<DetectedTarget_S> &vecResults)
{
    if (!vecResults.empty())
    {
        vecResults.clear();
    }

    if (m_pYoloHandle == nullptr)
    {
        dlog_error("KITCHEN_DETECT: 模型未初始化，无法检测");
        return -1;
    }

    if (stInData.inMat.empty() || stInData.inMat.type() != CV_8UC3)
    {
        dlog_error("KITCHEN_DETECT: 输入图片为空或非 CV_8UC3 RGB 格式");
        return -2;
    }

    /* letterbox 到模型输入尺寸（尺寸相同则跳过缩放，直接用原图） */
    float fScale = 1.0f;
    int nPadX = 0;
    int nPadY = 0;
    cv::Mat matInput;
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        if (!resizeAndPad(stInData.inMat, matInput, fScale, nPadX, nPadY))
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
       是推理库内部校验的历史约定（对照 pmnm_detect.cpp），照抄勿改 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData              = (float *)matInput.data;
    stInputData.nDataSize          = static_cast<int>(matInput.total() * matInput.elemSize() * sizeof(float));
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms        = stInData.stParam.fNmsThreshold;

    /* 推理（内部完成 rknn 推理 + YOLOv8 解码 + 按类 NMS）；
       阈值 <=0 时推理库内部回落到 json 配置的默认值 */
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    if (!m_pYoloHandle->inference(stInputData, vBoxDatas))
    {
        dlog_error("KITCHEN_DETECT: 推理失败");
        return -4;
    }

    /* 结果转换 + 坐标还原（推理输出坐标在 letterbox 坐标系） */
    vecResults.reserve(vBoxDatas.size());
    for (const auto &stBox : vBoxDatas)
    {
        DetectedTarget_S stResult;
        stResult.fX1         = static_cast<float>(stBox.stBoxs.nX1);
        stResult.fY1         = static_cast<float>(stBox.stBoxs.nY1);
        stResult.fX2         = static_cast<float>(stBox.stBoxs.nX2);
        stResult.fY2         = static_cast<float>(stBox.stBoxs.nY2);
        stResult.fConfidence = stBox.fConfidence;
        stResult.nLabel      = stBox.nLabel;
        vecResults.push_back(stResult);
    }
    restoreCoords(vecResults, fScale, nPadX, nPadY, stInData.inMat.cols, stInData.inMat.rows);
    printDetectResults(vecResults);
    return 0;
}

bool CKitchenDetector::resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage,
                                            float &fScale, int &nPadX, int &nPadY)
{
    if (inputImage.empty())
    {
        dlog_error("KITCHEN_DETECT: 输入图片为空，无法缩放");
        return false;
    }

    /* 缩放比例取宽高两个方向的较小值，保证缩放后两边都不超出模型输入尺寸 */
    fScale = std::min(static_cast<float>(m_nLimitWidth) / static_cast<float>(inputImage.cols),
                      static_cast<float>(m_nLimitHeight) / static_cast<float>(inputImage.rows));

    int nNewWidth  = static_cast<int>(inputImage.cols * fScale);
    int nNewHeight = static_cast<int>(inputImage.rows * fScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(nNewWidth, nNewHeight));

    /* 居中偏移量（黑边补在两侧，与缩放比例一并用于坐标还原） */
    nPadX = (m_nLimitWidth - nNewWidth) / 2;
    nPadY = (m_nLimitHeight - nNewHeight) / 2;

    /* 固定尺寸黑色背景，缩放后的图像居中放置 */
    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());
    resizedImage.copyTo(output(cv::Rect(nPadX, nPadY, nNewWidth, nNewHeight)));

    outputImage = output;
    return true;
}

void CKitchenDetector::restoreCoords(std::vector<DetectedTarget_S> &vecResults,
                                             float fScale, int nPadX, int nPadY,
                                             int nOriWidth, int nOriHeight)
{
    if (fScale <= 0.0f)
    {
        return;
    }

    float fOriWidth  = static_cast<float>(nOriWidth);
    float fOriHeight = static_cast<float>(nOriHeight);

    /* 先减 letterbox 偏移再除以缩放比例；负值截为 0，并夹回原图边界 */
    for (auto &stResult : vecResults)
    {
        stResult.fX1 = std::min(std::max((stResult.fX1 - static_cast<float>(nPadX)) / fScale, 0.0f), fOriWidth);
        stResult.fY1 = std::min(std::max((stResult.fY1 - static_cast<float>(nPadY)) / fScale, 0.0f), fOriHeight);
        stResult.fX2 = std::min(std::max((stResult.fX2 - static_cast<float>(nPadX)) / fScale, 0.0f), fOriWidth);
        stResult.fY2 = std::min(std::max((stResult.fY2 - static_cast<float>(nPadY)) / fScale, 0.0f), fOriHeight);
    }
}
