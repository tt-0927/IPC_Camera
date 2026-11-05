/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 人脸识别事件子检测器声明
 */

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "algorithm.hpp"                /* CAlarmStateMachine, EventManager */
#include "face_attribute_model.hpp"     /* CFaceAttributeModel */
#include "face_expression.hpp"          /* CFaceExpression */
#include "face_recognition_define.hpp"  /* DetectContext_S, cropFaceForModel */
#include "face_recognition_processor.hpp"
#include "face_sub_detector.hpp"        /* IFaceSubDetector */

/* 人脸识别事件富目标：基础结果 + 特化分析（属性/表情），仅人脸子内部流转 */
struct FaceTargetResult_S
{
    explicit FaceTargetResult_S(const FaceBaseTarget_S &stBase)
        : stDetectResult(stBase.stDetectResult),
          fQualityScore(stBase.fQualityScore)
    {
    }

    FaceDetectedTarget_S stDetectResult;       /* 人脸检测结果 */
    float fQualityScore = 0.0F;                /* 人脸质量分数 */
    bool bAttributeReady = false;              /* 属性分析是否成功 */
    FaceAttributeTarget_S stAttributeResult{}; /* 人脸属性结果 */
    bool bExpressionReady = false;             /* 表情分析是否成功 */
    int nExpressionLabel = -1;                 /* 表情类别 0~6，-1=未分析 */
};

/**
 * @brief  : 人脸识别事件子检测器
 * @note   : 特化分析（属性/表情）模型自持 lazy 加载（业务上与后厨无关人员
 *           进入不同时布防，属性模型各自持有实例）；规则过滤后驱动
 *           OSD/抓拍落盘/属性推送/GAT1400/事件组图等业务输出；
 *           配置自订阅，无线程，由算法线程串行驱动
 */
class CFaceRecognitionSubDetector : public IFaceSubDetector
{
public:
    CFaceRecognitionSubDetector();
    ~CFaceRecognitionSubDetector() override;

    void updateConfig() override;
    int process(const DetectContext_S &stDetectContext) override;
    bool isEventEnabled() const override;
    void reset() override;

private:
    /**
     * @brief 单张图片的落盘与数据库记录信息（业务输出链路内部流转）
     */
    struct CaptureRecord_S
    {
        std::string strImagePath;    /* 本地保存路径，空=未保存 */
        std::string strImageDate;    /* 图片日期 */
        std::string strImageTime;    /* 图片时间 */
        std::string strCaptureTime;  /* 数据库抓拍时间 */
    };

    /**
     * @brief 一帧图片保存结果，仅业务输出阶段局部使用
     */
    struct FrameCaptureResult_S
    {
        CaptureRecord_S stPanorama;                  /* 全景图记录 */
        std::vector<CaptureRecord_S> vstTargets;     /* 目标图记录，下标对齐 vstTargets */
    };

    /* lazy 加载属性模型，失败返回 false */
    bool initAttributeModel();

    /* lazy 加载表情模型，失败返回 false */
    bool initExpressionModel();

    /* 对目标执行人脸属性分析 */
    bool analyzeFaceAttribute(const cv::Mat &stRgbImage,
                              const FaceDetectedTarget_S &stResult,
                              FaceAttributeTarget_S &stAttributeResult);

    /* 对目标执行人脸表情分析 */
    bool analyzeFaceExpression(const cv::Mat &stRgbImage,
                               const FaceDetectedTarget_S &stResult,
                               int &nExpressionLabel);

    /* 对全部有效目标执行属性分析（配置开关驱动），结果写入富目标 */
    void processAttributeAnalysis(const cv::Mat &stRgbImage,
                                  std::vector<FaceTargetResult_S> &vstTargets,
                                  const Alarm::FaceRecognition_S &stConfig);

    /* 对全部有效目标执行表情分析（配置开关驱动），结果写入富目标 */
    void processExpressionAnalysis(const cv::Mat &stRgbImage,
                                   std::vector<FaceTargetResult_S> &vstTargets,
                                   const Alarm::FaceRecognition_S &stConfig);

    /* 处理人脸识别事件业务输出：规则过滤后组织OSD、抓拍、属性推送与事件输出
       （富目标与基础目标同序，候选下标互通） */
    void processBusinessOutput(const FrameContext_S &stFrame,
                               const std::vector<FaceBaseTarget_S> &vstBaseTargets,
                               const std::vector<FaceTargetResult_S> &vstTargets,
                               const Alarm::FaceRecognition_S &stConfig);

    /* 按需落盘并写数据库：抓拍联动配置要求或属性推送需要图片时执行 */
    void saveAlarmImages(const FrameContext_S &stContext,
                         const std::vector<FaceTargetResult_S> &vstTargets,
                         const Alarm::FaceRecognition_S &stConfig,
                         const std::vector<bool> &vbPushFlags,
                         FrameCaptureResult_S &stResult);

    /* 输出人脸属性分析结果（推送消息，图片路径取自保存记录） */
    void processAttributeResult(const FrameContext_S &stContext,
                                const std::vector<FaceTargetResult_S> &vstTargets,
                                const Alarm::FaceRecognition_S &stConfig,
                                const std::vector<bool> &vbPushFlags,
                                const FrameCaptureResult_S &stSaveResult);

#ifdef ENABLE_TVSDK_SRC
    /**
     * @brief 在算法工作线程同步发送本帧触发的人脸抓拍，每个目标独立携带属性。
     */
    void processTvSdkCaptureResult(const FrameContext_S &stContext,
                                   const std::vector<FaceTargetResult_S> &vstTargets,
                                   const Alarm::FaceRecognition_S &stConfig);
#endif

    /* 输出 GAT1400 人脸结果 */
    void processGat1400Result(const FrameContext_S &stContext,
                              const std::vector<FaceTargetResult_S> &vstTargets);

    /* 更新属性推送去重基线并输出各目标推送标志 */
    void updateAttributePushFlags(const std::vector<FaceTargetResult_S> &vstTargets,
                                  const Alarm::FaceRecognition_S &stConfig,
                                  std::vector<bool> &vbPushFlags);

    /* 按需裁剪单个目标 BGR 小图（不缓存，调用方自行持有） */
    cv::Mat cropTargetImage(const FrameContext_S &stContext,
                            const FaceTargetResult_S &stTarget) const;

    /* 使用RGA从紧凑NV12特写源裁剪并转换BGR目标图 */
    bool cropCloseupImageByRga(const CloseupSource_S &stCloseupSource,
                               const cv::Rect2f &stDetectRect,
                               const cv::Size &stDetectCoordinateSize,
                               cv::Mat &stTargetBgrImage) const;

    /* 保存单张抓拍图片 */
    std::string saveCaptureImage(const cv::Mat &stBgrImage,
                                 Alarm::LinkageType_E enImageType,
                                 const std::string &strCurrentDate,
                                 const std::string &strCurrentTime,
                                 int nJpegQuality,
                                 int nTargetIndex = -1) const;

    /* 将抓拍图片信息写入数据库 */
    bool saveCaptureRecord(const std::string &strImagePath,
                           int nChannelId,
                           const std::string &strCaptureTime) const;

    /* 对抓拍全景图叠加人脸抓拍信息 */
    void addCaptureOverlay(cv::Mat &stBgrImage) const;

    /* 推送单个人脸目标的属性结果 */
    void pushAttributeResult(const FrameContext_S &stContext,
                             const FaceTargetResult_S &stTarget,
                             const CaptureRecord_S &stPanoramaRecord,
                             const CaptureRecord_S &stTargetRecord) const;

    /* 输出动态分析框 */
    void outputOsd(const FrameContext_S &stContext,
                   const std::vector<FaceTargetResult_S> &vstTargets);

    /* 获取质量分最高的目标 */
    const FaceTargetResult_S *getBestTarget(
        const std::vector<FaceTargetResult_S> &vstTargetResults) const;

    /* 处理统一人脸识别事件，按联动选项附加全景图和同一目标的小图 */
    void processFaceRecognitionEvent(const FrameContext_S &stContext,
                                     const std::vector<FaceTargetResult_S> &vstTargets,
                                     const Alarm::FaceRecognition_S &stConfig,
                                     const FrameCaptureResult_S &stSaveResult);

private:
    static constexpr int CAPTURE_INTERVAL_DEFAULT_MS = 1000; /* 抓拍节拍默认窗口，毫秒 */

    CFaceAttributeModel *m_pFaceAttributeHandle = nullptr; /* 人脸属性模型 */
    CFaceExpression *m_pFaceExpressionHandle = nullptr;    /* 人脸表情模型 */

    EventManager m_faceCaptureManager{CAPTURE_INTERVAL_DEFAULT_MS}; /* 事件消费节拍，窗口为抓拍间隔 */

    std::vector<FaceDetectedTarget_S> m_vstLastAttributeTargets; /* 上一帧属性目标（推送去重基线） */

    /* 跨会话/开关跳变清基线标记（算法线程消费，避免跨线程清向量） */
    std::atomic<bool> m_bResetAttributeTargets{false};

    CFaceRecognitionProcessor m_processor;  /* 规则判断器 */
    CAlarmStateMachine m_stateMachine;      /* 报警状态机 */

    mutable std::mutex m_mutexConfig;       /* updateConfig/reset 与算法线程互斥 */
    Alarm::FaceRecognition_S m_stConfig;    /* 配置快照 */
};
