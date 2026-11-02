/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_expression/face_expression.cpp
 * @Date         : 2026-10-19
 * @Description  : 人脸表情识别纯组件实现
 */

#include "face_expression.hpp"

#include <unistd.h>
#include <vector>
#include "ImageFeature.hpp"
#include "dlog.h"

/* 类别名称表（labelToString 专用，下标与类别 ID 一致） */
static const std::string FACE_EXPRESSION_CLASS_NAME[] = {
    "生气", "厌恶", "恐惧", "高兴", "难过", "惊讶", "中立"
};

/* 当系统中存在 /printAlgoResults 文件时打印识别结果 */
static void printDetectResult(int nLabel)
{
    if (access("/printAlgoResults", F_OK))
    {
        return;
    }

    dlog_info("FACE_EXPRESSION: 表情=%s(%d)",
              CFaceExpression::labelToString(nLabel).c_str(), nLabel);
}

CFaceExpression::CFaceExpression(const std::string &strJsonConfigPath)
    : m_strJsonConfigPath(strJsonConfigPath)
{
}

CFaceExpression::~CFaceExpression()
{
    unInit();
}

bool CFaceExpression::init()
{
    /* 重复 init 先释放旧句柄，保证可重试 */
    if (m_pImageFeature != nullptr)
    {
        unInit();
    }

    m_pImageFeature = new Inference_NS::CImageFeature(m_strJsonConfigPath);
    if (m_pImageFeature == nullptr || !m_pImageFeature->init())
    {
        dlog_error("FACE_EXPRESSION: 模型初始化失败 [%s]", m_strJsonConfigPath.c_str());
        unInit();
        return false;
    }

    /* 取模型实际输入尺寸，换模型不改代码 */
    m_pImageFeature->getSizeLimit(0, m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);

    dlog_info("FACE_EXPRESSION: 模型初始化成功 [%s] 输入尺寸[%dx%d]",
              m_strJsonConfigPath.c_str(), m_nLimitWidth, m_nLimitHeight);
    return true;
}

bool CFaceExpression::unInit()
{
    if (m_pImageFeature != nullptr)
    {
        delete m_pImageFeature;
        m_pImageFeature = nullptr;
    }
    return true;
}

/* 类别 ID 转名称 */
std::string CFaceExpression::labelToString(int label)
{
    if (label < 0 || label >= (int)(sizeof(FACE_EXPRESSION_CLASS_NAME) / sizeof(FACE_EXPRESSION_CLASS_NAME[0])))
    {
        return "未知";
    }
    return FACE_EXPRESSION_CLASS_NAME[label];
}

int CFaceExpression::process(const InData_S &stInData)
{
    if (m_pImageFeature == nullptr)
    {
        dlog_error("FACE_EXPRESSION: 模型未初始化，无法识别");
        return -1;
    }

    if (stInData.inMat.empty() || stInData.inMat.type() != CV_8UC3)
    {
        dlog_error("FACE_EXPRESSION: 输入图片为空或非 CV_8UC3 RGB 格式");
        return -1;
    }

    /* 前处理 - 直接缩放到模型输入尺寸，不保比例（与原 FaceExpressionV1_0 行为一致）；
       尺寸变化时 resize 会重新分配内存，不影响调用方原图 */
    cv::Mat matInput = stInData.inMat;
    if (matInput.cols != m_nLimitWidth || matInput.rows != m_nLimitHeight)
    {
        cv::resize(matInput, matInput, cv::Size(m_nLimitWidth, m_nLimitHeight));
    }

    /* 组装推理输入：
       pData 实为 uint8 RGB 数据，强转 float* 与 nDataSize 乘 sizeof(float)
       是推理库内部校验的历史约定（对照 FaceExpressionV1_0.cpp），照抄勿改 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData     = (float *)matInput.data;
    stInputData.nDataSize = static_cast<int>(matInput.total() * matInput.elemSize() * sizeof(float));

    /* 推理（内部完成 rknn 推理 + 分类结果解码） */
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    if (!m_pImageFeature->inference(stInputData, vClsDatas) || vClsDatas.empty())
    {
        dlog_error("FACE_EXPRESSION: 推理失败");
        return -1;
    }

    /* 后处理 - 取分值最大的表情类别（softmax 为单调变换，不影响 argmax） */
    const std::vector<float> &vFeature = vClsDatas[0].vFeature;
    int nMaxIndex = 0;
    for (size_t i = 1; i < vFeature.size(); i++)
    {
        if (vFeature[i] > vFeature[static_cast<size_t>(nMaxIndex)])
        {
            nMaxIndex = static_cast<int>(i);
        }
    }

    printDetectResult(nMaxIndex);
    return nMaxIndex; /* 表情类别: 0~6 */
}
