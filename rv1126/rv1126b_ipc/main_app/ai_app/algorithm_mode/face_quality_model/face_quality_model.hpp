/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_quality_model/face_quality_model.hpp
 * @Date         : 2026-10-20
 * @Description  : 人脸质量评估纯组件（不继承 CAlgorithm，不取流，由调用方驱动）
 */

#pragma once

#include <string>

#include <opencv2/opencv.hpp>

namespace Inference_NS
{
    class CImageFeature;
}

/**
 * @brief  : 人脸质量评估纯组件
 * @note   : 仅做评估（等比贴边 + 推理 + 评分），评分阈值过滤等业务由调用方处理；
 *           输入 CV_8UC3 RGB 人脸图，输出质量分（特征向量前 10 维均值，对齐旧实现）。
 *           推理复用 Inference 层 CImageFeature，不依赖 AiModules/Modules。
 *           移植自 share/ai_share/AiModules/Modules/FaceQualityAssessment（CFaceQualityAssessmentV1_0）。
 */
class CFaceQualityModel
{
public:
    /* 分析参数（预留灵敏度映射入口） */
    typedef struct _AnalyseParam_
    {
    } AnalyseParam_S;

    /* process 输入：人脸 RGB 图 + 参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat; /* CV_8UC3 RGB 人脸图，任意分辨率 */
        AnalyseParam_S stParam;
    } InData_S;

public:
    explicit CFaceQualityModel(const std::string &strJsonConfigPath);
    ~CFaceQualityModel();

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
     * @brief   : 人脸质量评估同步接口
     * @param    {InData_S} &stInData：输入人脸 RGB 图
     * @param    {float} &fScore：输出质量分
     * @return   {int} 0：成功 -1：模型未初始化 -2：输入非法 -3：贴边失败 -4：推理失败
     * @note    : 非线程安全：并发调用需调用方自行串行化
     */
    int process(const InData_S &stInData, float &fScore);

private:
    /**
     * @brief   : 等比缩放并居中贴黑边到模型输入尺寸（无条件缩放，对齐旧实现）
     * @return   {bool} true：成功 false：输入为空
     */
    bool resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage);

private:
    /* 推理句柄 */
    Inference_NS::CImageFeature *m_pInferHandle = nullptr;

    /* 模型配置文件路径 */
    std::string m_strJsonConfigPath;

    /* 模型输入尺寸（init 后以 getSizeLimit 实测为准，不写死） */
    int m_nLimitWidth  = 112;
    int m_nLimitHeight = 112;
    int m_nLimitChannel = 0;
};
