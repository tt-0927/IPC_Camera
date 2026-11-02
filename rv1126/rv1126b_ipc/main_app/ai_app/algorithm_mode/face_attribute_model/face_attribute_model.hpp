/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_attribute_model/face_attribute_model.hpp
 * @Date         : 2026-10-20
 * @Description  : 人脸属性分析纯组件（不继承 CAlgorithm，不取流，由调用方驱动）
 */

#pragma once

#include <string>

#include <opencv2/opencv.hpp>

namespace Inference_NS
{
    class CAttribute;
}

/* 人脸属性结果：单张人脸图对应一个结果（字段标签约定与旧 FaceAttributeV1_0 一致） */
typedef struct _FaceAttributeTarget_S_
{
    bool bIsMale       = false; /* 是否男性（标签 0） */
    int  nAgeLabel     = 0;     /* 年龄标签 1~4：小孩/青年/中年/老年 */
    bool bIsGlasses    = false; /* 是否戴眼镜（标签 5） */
    bool bIsBeard      = false; /* 是否有胡子（标签 6） */
    bool bIsMask       = false; /* 是否戴口罩（标签 7） */
    int  nEmotionLabel = 0;     /* 表情标签 8~14：中性/愤怒/快乐/悲伤/惊讶/恐惧/厌恶 */
} FaceAttributeTarget_S;

/**
 * @brief  : 人脸属性分析纯组件
 * @note   : 仅做分类（推理 + 阈值判定），不含事件业务；输入 CV_8UC3 RGB 人脸图，
 *           尺寸须等于模型输入尺寸（组件不做内部缩放，调用方负责裁剪缩放，
 *           与旧 FaceAttributeV1_0 一致），输出性别/年龄/眼镜/胡子/口罩/表情六类属性。
 *           推理复用 Inference 层 CAttribute，不依赖 AiModules/Modules。
 *           移植自 share/ai_share/AiModules/Modules/FaceAttribute（CFaceAttributeV1_0）。
 */
class CFaceAttributeModel
{
public:
    /* 分析参数（预留灵敏度映射入口） */
    typedef struct _AnalyseParam_
    {
    } AnalyseParam_S;

    /* process 输入：人脸 RGB 图 + 参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat; /* CV_8UC3 RGB 人脸图，尺寸须等于模型输入尺寸 */
        AnalyseParam_S stParam;
    } InData_S;

public:
    explicit CFaceAttributeModel(const std::string &strJsonConfigPath);
    ~CFaceAttributeModel();

    /**
     * @brief   : 加载模型
     * @return   {bool} true：成功 false：失败（可修复后重新调用）
     * @note    : 只读取 json 配置加载推理句柄；失败时内部会清理半初始化状态
     */
    bool init();

    /**
     * @brief   : 释放模型
     * @return   {bool} true：成功
     * @note    : 重复调用安全；调用方需保证 unInit 后不再调用 process
     */
    bool unInit();

    /**
     * @brief   : 人脸属性分析同步接口
     * @param    {InData_S} &stInData：输入人脸 RGB 图
     * @param    {FaceAttributeTarget_S} &stResult：输出属性结果
     * @return   {int} 0：成功 -1：模型未初始化 -2：输入非法 -4：推理失败
     * @note    : 非线程安全：并发调用需调用方自行串行化
     */
    int process(const InData_S &stInData, FaceAttributeTarget_S &stResult);

private:
    /* 推理句柄 */
    Inference_NS::CAttribute *m_pInferHandle = nullptr;

    /* 模型配置文件路径 */
    std::string m_strJsonConfigPath;

    /* 模型输入尺寸（init 后以 getSizeLimit 实测为准，不写死） */
    int m_nLimitWidth  = 192;
    int m_nLimitHeight = 192;
    int m_nLimitChannel = 0;

    /* 属性判定阈值（与旧 FaceAttributeV1_0 一致） */
    static constexpr float CONF_THRESHOLD = 0.5F;        /* 通用：性别/口罩 */
    static constexpr float CONF_GLASS_THRESHOLD = 0.02F; /* 眼镜 */
    static constexpr float CONF_BEARD_THRESHOLD = 0.2F;  /* 胡子 */
};
