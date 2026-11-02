/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_attribute_model/face_attribute_model.cpp
 * @Date         : 2026-10-20
 * @Description  : 人脸属性分析纯组件实现
 */

#include "face_attribute_model.hpp"

#include <vector>
#include "Attribute.hpp"
#include "dlog.h"

CFaceAttributeModel::CFaceAttributeModel(const std::string &strJsonConfigPath)
    : m_strJsonConfigPath(strJsonConfigPath)
{
}

CFaceAttributeModel::~CFaceAttributeModel()
{
    unInit();
}

bool CFaceAttributeModel::init()
{
    /* 重复 init 先释放旧句柄，保证可重试 */
    if (m_pInferHandle != nullptr)
    {
        unInit();
    }

    m_pInferHandle = new Inference_NS::CAttribute(m_strJsonConfigPath);
    if (m_pInferHandle == nullptr || !m_pInferHandle->init())
    {
        dlog_error("FACE_ATTRIBUTE: 模型初始化失败 [%s]", m_strJsonConfigPath.c_str());
        unInit();
        return false;
    }

    /* 取模型实际输入尺寸，换模型不改代码 */
    m_pInferHandle->getSizeLimit(0, m_nLimitWidth, m_nLimitHeight, m_nLimitChannel);

    dlog_info("FACE_ATTRIBUTE: 模型初始化成功 [%s] 输入尺寸[%dx%d]",
              m_strJsonConfigPath.c_str(), m_nLimitWidth, m_nLimitHeight);
    return true;
}

bool CFaceAttributeModel::unInit()
{
    if (m_pInferHandle != nullptr)
    {
        delete m_pInferHandle;
        m_pInferHandle = nullptr;
    }
    return true;
}

int CFaceAttributeModel::process(const InData_S &stInData, FaceAttributeTarget_S &stResult)
{
    stResult = FaceAttributeTarget_S();

    if (m_pInferHandle == nullptr)
    {
        dlog_error("FACE_ATTRIBUTE: 模型未初始化，无法分析");
        return -1;
    }

    /* 旧实现不做内部缩放，尺寸不符时依赖推理库数据大小校验；此处提前拦截便于定位 */
    if (stInData.inMat.empty() || stInData.inMat.type() != CV_8UC3 ||
        stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        dlog_error("FACE_ATTRIBUTE: 输入为空/非 CV_8UC3/尺寸不符[%dx%d]，要求[%dx%d]",
                   stInData.inMat.cols, stInData.inMat.rows,
                   m_nLimitWidth, m_nLimitHeight);
        return -2;
    }

    /* 组装推理输入：
       pData 实为 uint8 RGB 数据，强转 float* 与 nDataSize 乘 sizeof(float)
       是推理库内部校验的历史约定（对照 face_expression.cpp），照抄勿改 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData     = (float *)stInData.inMat.data;
    stInputData.nDataSize = static_cast<int>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));

    /* 推理（内部完成 rknn 推理 + 多分类头解码） */
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    if (!m_pInferHandle->inference(stInputData, vClsDatas) || vClsDatas.empty())
    {
        dlog_error("FACE_ATTRIBUTE: 推理失败");
        return -4;
    }

    /* 单输入单输出，取首组分类头，按下标映射属性字段（与旧 FaceAttributeV1_0 一致） */
    const std::vector<Inference_NS::Cls_S> &vCls = vClsDatas[0].vCls;
    for (size_t j = 0; j < vCls.size(); ++j)
    {
        const Inference_NS::Cls_S &stCls = vCls[j];
        switch (j)
        {
        case 0: /* 性别（标签 0 为男性） */
            stResult.bIsMale = (stCls.nLabel == 0 && stCls.fConfidence > CONF_THRESHOLD);
            break;
        case 1: /* 年龄（标签 1~4） */
            if (stCls.nLabel >= 1 && stCls.nLabel <= 4)
            {
                stResult.nAgeLabel = stCls.nLabel;
            }
            break;
        case 2: /* 眼镜（标签 5） */
            stResult.bIsGlasses = (stCls.nLabel == 5 && stCls.fConfidence > CONF_GLASS_THRESHOLD);
            break;
        case 3: /* 胡子（标签 6） */
            stResult.bIsBeard = (stCls.nLabel == 6 && stCls.fConfidence >= CONF_BEARD_THRESHOLD);
            break;
        case 4: /* 口罩（标签 7） */
            stResult.bIsMask = (stCls.nLabel == 7 && stCls.fConfidence > CONF_THRESHOLD);
            break;
        case 5: /* 表情（标签 8~14） */
            if (stCls.nLabel >= 8 && stCls.nLabel <= 14)
            {
                stResult.nEmotionLabel = stCls.nLabel;
            }
            break;
        default:
            break;
        }
    }
    return 0;
}
