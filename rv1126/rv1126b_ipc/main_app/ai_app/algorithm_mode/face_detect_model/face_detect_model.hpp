/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_detect_model/face_detect_model.hpp
 * @Date         : 2026-10-20
 * @Description  : 人脸检测纯组件（不继承 CAlgorithm，不取流，由调用方驱动）
 */

#pragma once

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

namespace Inference_NS
{
    class CYoloUltralyticsPoint;
}

/* 人脸关键点数量（左右眼、鼻尖、左右嘴角） */
const int FACE_DETECT_POINT_NUM = 5;

/* 人脸检测目标：单一类别，不设类别字段 */
typedef struct _FaceDetectedTarget_S_
{
    float fX1, fY1, fX2, fY2;                /* 左上/右下坐标 */
    float fConfidence;                       /* 置信度 */
    float vPoint[FACE_DETECT_POINT_NUM * 2]; /* 关键点 x,y 依次展开，与框同坐标系 */
} FaceDetectedTarget_S;

/**
 * @brief  : 人脸检测纯组件
 * @note   : 仅做检测（贴边缩放 + 推理 + 坐标还原），输出人脸框与 5 关键点，
 *           瞳距过滤、区域过滤等业务由调用方处理；输入 CV_8UC3 RGB，
 *           NV12 等格式由调用方先转换；贴边行为对齐旧 FaceDetectV1_0：
 *           仅超限时缩小，小图原尺寸居中贴黑边。
 *           推理复用 Inference 层 CYoloUltralyticsPoint，不依赖 AiModules/Modules。
 */
class CFaceDetectModel
{
public:
    /* 检测参数：阈值 <=0 时由推理库回落到 json 配置的默认值 */
    typedef struct _AnalyseParam_
    {
        float fBoxThreshold = -1.0f;
        float fNmsThreshold = -1.0f;
    } AnalyseParam_S;

    /* process 输入：RGB 图 + 参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat; /* CV_8UC3 RGB，任意分辨率 */
        AnalyseParam_S stParam;
    } InData_S;

public:
    explicit CFaceDetectModel(const std::string &strJsonConfigPath);
    ~CFaceDetectModel();

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
     * @brief   : 人脸检测同步接口
     * @param    {InData_S} &stInData：输入 RGB 图 + 阈值参数
     * @param    {std::vector<FaceDetectedTarget_S>} &vecResults：输出人脸目标（框与关键点坐标为输入图分辨率），无目标时为空
     * @return   {int} 0：成功 -1：模型未初始化 -2：输入非法 -3：贴边失败 -4：推理失败
     * @note    : 非线程安全：并发调用需调用方自行串行化
     */
    int process(const InData_S &stInData, std::vector<FaceDetectedTarget_S> &vecResults);

private:
    /**
     * @brief   : 贴边到模型输入尺寸（仅超限缩小，小图原尺寸居中）
     * @return   {bool} true：成功 false：输入为空
     */
    bool resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage,
                      float &fScale, int &nPadX, int &nPadY);

private:
    /* 推理句柄 */
    Inference_NS::CYoloUltralyticsPoint *m_pInferHandle = nullptr;

    /* 模型配置文件路径 */
    std::string m_strJsonConfigPath;

    /* 模型输入尺寸（init 后以 getSizeLimit 实测为准，不写死） */
    int m_nLimitWidth  = 640;
    int m_nLimitHeight = 640;
    int m_nLimitChannel = 3;
};
