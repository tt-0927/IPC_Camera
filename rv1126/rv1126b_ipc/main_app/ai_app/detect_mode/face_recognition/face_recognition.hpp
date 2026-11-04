/*
 * @Author: zhengxh
 * @Date: 2026-09-24 08:45:07
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-20 19:51:01
 * @Description: 人脸相关事件处理
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "face_attribute_model.hpp"
#include "face_detect_model.hpp"
#include "face_expression.hpp"
#include "face_quality_model.hpp"
#include "face_recognition_define.hpp"
#include "face_recognition_processor.hpp"
#include "kitchen_unrelated_person_processor.hpp"
#include "social_relationship_processor.hpp"

class CFaceRecognition : public CAlgorithm
{
public:
    CFaceRecognition();
    ~CFaceRecognition() override;

    /**
     * @brief 接收媒体数据
     * @param stMediaData 媒体帧
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief 更新算法运行开关及人脸识别/社交关系分析配置
     * @param stAlgoConfig 当前布防调度生成的算法开关
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新社交关系分析参数
     * @param    {SocialRelationshipAnalysis_S} &stAlgoCfg：社交关系分析
     */
    void setAlgoParamCfg(const Alarm::SocialRelationshipAnalysis_S &stAlgoCfg,
        Event::Type_E enType = Event::Type_E::SOCIAL_RELATIONSHIP_ANALYSIS);

    /**
     * @brief   : 更新后厨无关人员进入参数
     * @param    {KitchenUnrelatedPersonEntry_S} &stAlgoCfg：后厨无关人员进入配置
     */
    void setAlgoParamCfg(const Alarm::KitchenUnrelatedPersonEntry_S &stAlgoCfg,
        Event::Type_E enType = Event::Type_E::KITCHEN_UNRELATED_PERSON_ENTRY);

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

    /**
     * @brief 初始化人脸检测和质量评估模型
     * @return true 初始化成功
     */
    bool init();

    /**
     * @brief 释放算法模型
     */
    void unInit();

    /**
     * @brief 按需初始化人脸属性模型
     */
    bool initAttributeModel();

    /**
     * @brief 按需初始化人脸表情模型
     */
    bool initExpressionModel();

    /**
     * @brief 更新人脸识别运行配置及处理器规则、抓拍节拍
     * @param stConfig 持久化的人脸识别配置
     */
    void updateFaceConfig(const Alarm::FaceRecognition_S &stConfig);

    /**
     * @brief 填充检测上下文的帧源信息并准备算法输入图像
     */
    bool prepareFrameContext(const MediaData_S &stMediaData,
                             FrameContext_S &stContext);

    /**
     * @brief 执行人脸检测模型并绑定质量评估，产出目标结果
     */
    void detectFaces(DetectContext_S &stDetectContext);

    /**
     * @brief 人脸检测与质量评估后，按需继续属性/表情分析
     */
    void processAlgorithms(DetectContext_S &stDetectContext);

    /**
     * @brief 对全部有效目标执行属性分析
     */
    void processAttributeAnalysis(DetectContext_S &stDetectContext);

    /**
     * @brief 对全部有效目标执行表情分析
     */
    void processExpressionAnalysis(DetectContext_S &stDetectContext);

    /**
     * @brief 处理人脸识别事件：规则过滤目标后组织OSD、抓拍、属性推送与事件输出
     */
    void processBusinessOutput(const DetectContext_S &stDetectContext);

    /**
     * @brief 按需落盘并写数据库：抓拍联动配置要求或属性推送需要图片时执行
     */
    void saveAlarmImages(const FrameContext_S &stContext,
                         const std::vector<FaceTargetResult_S> &vstTargets,
                         const Alarm::FaceRecognition_S &stConfig,
                         const std::vector<bool> &vbPushFlags,
                         FrameCaptureResult_S &stResult);

    /**
     * @brief 输出人脸属性分析结果（推送消息，图片路径取自保存记录）
     */
    void processAttributeResult(const FrameContext_S &stContext,
                                const std::vector<FaceTargetResult_S> &vstTargets,
                                const Alarm::FaceRecognition_S &stConfig,
                                const std::vector<bool> &vbPushFlags,
                                const FrameCaptureResult_S &stSaveResult);

#ifdef ENABLE_TVSDK_SRC
    /**
     * @brief 在算法工作线程同步发送本帧触发的人脸抓拍，每个目标独立携带属性。
     * @param [in] stContext 当前帧信息，提供全景图与通道时间戳。
     * @param [in] vstTargets 当前帧检测目标。
     * @param [in] stConfig 人脸识别运行配置快照。
     * @param [out] 无。
     * @return 无，编码及推送失败记录日志，不影响其他输出链路。
     */
    void processTvSdkCaptureResult(const FrameContext_S &stContext,
                                   const std::vector<FaceTargetResult_S> &vstTargets,
                                   const Alarm::FaceRecognition_S &stConfig);

    /**
     * @brief 在事件开始帧推送社交关系分析抓拍：1 张全景 + 每个候选一张特写。
     * @param [in] stContext 当前帧信息，提供全景图与通道时间戳。
     * @param [in] vstCandidates 社交关系分析候选目标（最多 NET_CAPTURE_CROP_MAX_NUM 个）。
     * @param [out] 无。
     * @return 无，编码或推送失败只记录日志。
     */
    void pushSocialRelationshipCapture(const FrameContext_S &stContext,
                                       const std::vector<FaceTargetResult_S> &vstCandidates);
#endif

    /**
     * @brief 输出 GAT1400 人脸结果
     */
    void processGat1400Result(const FrameContext_S &stContext,
                              const std::vector<FaceTargetResult_S> &vstTargets);

    /**
     * @brief 更新属性推送去重基线并输出各目标推送标志
     */
    void updateAttributePushFlags(const std::vector<FaceTargetResult_S> &vstTargets,
                                  const Alarm::FaceRecognition_S &stConfig,
                                  std::vector<bool> &vbPushFlags);

    /**
     * @brief 按需裁剪单个目标 BGR 小图（不缓存，调用方自行持有）
     */
    cv::Mat cropTargetImage(const FrameContext_S &stContext,
                            const FaceTargetResult_S &stTarget) const;

    /**
     * @brief 使用RGA从紧凑NV12特写源裁剪并转换BGR目标图
     */
    bool cropCloseupImageByRga(const CloseupSource_S &stCloseupSource,
                               const cv::Rect2f &stDetectRect,
                               const cv::Size &stDetectCoordinateSize,
                               cv::Mat &stTargetBgrImage) const;

    /**
     * @brief 保存单张抓拍图片
     */
    std::string saveCaptureImage(const cv::Mat &stBgrImage,
                                 Alarm::LinkageType_E enImageType,
                                 const std::string &strCurrentDate,
                                 const std::string &strCurrentTime,
                                 int nJpegQuality,
                                 int nTargetIndex = -1) const;

    /**
     * @brief 将抓拍图片信息写入数据库
     */
    bool saveCaptureRecord(const std::string &strImagePath,
                           int nChannelId,
                           const std::string &strCaptureTime) const;

    /**
     * @brief 对抓拍全景图叠加人脸抓拍信息
     */
    void addCaptureOverlay(cv::Mat &stBgrImage) const;

    /**
     * @brief 推送单个人脸目标的属性结果
     */
    void pushAttributeResult(const FrameContext_S &stContext,
                             const FaceTargetResult_S &stTarget,
                             const CaptureRecord_S &stPanoramaRecord,
                             const CaptureRecord_S &stTargetRecord) const;

    /**
     * @brief 输出动态分析框
     */
    void outputOsd(const FrameContext_S &stContext,
                   const std::vector<FaceTargetResult_S> &vstTargets);

    /**
     * @brief 获取质量分最高的目标
     */
    const FaceTargetResult_S *getBestTarget(
        const std::vector<FaceTargetResult_S> &vstTargetResults) const;

    /**
     * @brief 对单个人脸执行质量评估
     */
    bool evaluateFaceQuality(const cv::Mat &stRgbImage,
                             const FaceDetectedTarget_S &stResult,
                             float &fQualityScore);

    /**
     * @brief 裁剪人脸并等比填充到指定尺寸
     */
    bool cropFaceForModel(const cv::Mat &stRgbImage,
                          const FaceDetectedTarget_S &stResult,
                          int nWidth,
                          int nHeight,
                          cv::Mat &stFaceImage) const;

    /**
     * @brief 对目标执行人脸属性分析
     */
    bool analyzeFaceAttribute(const cv::Mat &stRgbImage,
                              const FaceDetectedTarget_S &stResult,
                              FaceAttributeTarget_S &stAttributeResult);

    /**
     * @brief 对目标执行人脸表情分析
     */
    bool analyzeFaceExpression(const cv::Mat &stRgbImage,
                               const FaceDetectedTarget_S &stResult,
                               int &nExpressionLabel);

    /**
     * @brief 处理统一人脸识别事件，按联动选项附加全景图和同一目标的小图。
     * @param [in] stContext 当前帧信息。
     * @param [in] vstTargets 当前帧检测目标。
     * @param [in] stConfig 人脸识别运行配置快照。
     * @param [in] stSaveResult 本帧图片保存记录，事件附带路径取自全景记录。
     * @param [out] 无。
     * @return 无，图片处理失败不阻断事件状态处理。
     */
    void processFaceRecognitionEvent(const FrameContext_S &stContext,
                                     const std::vector<FaceTargetResult_S> &vstTargets,
                                     const Alarm::FaceRecognition_S &stConfig,
                                     const FrameCaptureResult_S &stSaveResult);

    /**
     * @brief 处理社交关系分析事件，配置启用且处理器候选非空时上报报警。
     * @param [in] stDetectContext 单帧检测上下文。
     * @param [out] 无。
     * @return 无，报警节奏由独立状态机的冷却与结束阈值控制。
     */
    void processSocialRelationshipEvent(const DetectContext_S &stDetectContext);

    /**
     * @brief 处理后厨无关人员进入事件，配置启用且处理器候选非空时上报报警。
     * @param [in] stDetectContext 单帧检测上下文。
     * @param [out] 无。
     * @return 无，报警节奏由独立状态机的冷却与结束阈值控制。
     */
    void processKitchenUnrelatedEvent(const DetectContext_S &stDetectContext);

    /**
     * @brief 检测线程入口
     */
    void run();

private:
    static constexpr int FRAME_QUEUE_MAX = 2;           /* 帧队列最大长度 */
    static constexpr int DETECT_INTERVAL_MS = 1000;     /* 帧入口固定检测间隔，单位毫秒 */

    BQ_NS::CBlockingQueue<MediaData_S> m_frameQueue;    /* 待检测帧队列 */

    /* 检测线程相关定义 */
    std::atomic<bool> m_bRunning{false};
    std::atomic<bool> m_bEnabled{false};
    std::mutex m_threadMutex;
    std::condition_variable m_threadCondition;
    std::thread m_thread;

    Alarm::FaceRecognition_S m_stFaceConfig; /* 人脸识别运行配置 */
    Alarm::SocialRelationshipAnalysis_S m_stSocialConfig; /* 社交关系分析运行配置 */
    Alarm::KitchenUnrelatedPersonEntry_S m_stKitchenConfig; /* 后厨无关人员进入运行配置 */

    EventManager m_recvManager{DETECT_INTERVAL_MS};      /* 检测帧接收频率控制 */
    EventManager m_faceCaptureManager{DETECT_INTERVAL_MS}; /* 人脸识别事件消费节拍，窗口为抓拍间隔 */

    CFaceDetectModel *m_pFaceDetectHandle = nullptr;                 /* 人脸检测模型 */
    CFaceQualityModel *m_pFaceQualityHandle = nullptr;               /* 人脸质量模型 */
    CFaceAttributeModel *m_pFaceAttributeHandle = nullptr;           /* 人脸属性模型 */
    CFaceExpression *m_pFaceExpressionHandle = nullptr;             /* 人脸表情模型 */

    cv::Mat m_stPanoramaRgbImage;                      /* 复用的AI全景RGB缓冲 */

    std::vector<FaceDetectedTarget_S> m_vstLastAttributeTargets; /* 上一帧属性目标 */

    CFaceRecognitionProcessor m_faceRecognitionProcessor;        /* 人脸识别事件处理器 */
    CSocialRelationshipProcessor m_socialRelationshipProcessor;  /* 社交关系分析事件处理器 */
    CKitchenUnrelatedPersonProcessor m_kitchenUnrelatedProcessor; /* 后厨无关人员进入事件处理器 */

    CAlarmStateMachine m_faceRecognitionStateMachine;    /* 人脸识别事件状态机 */
    CAlarmStateMachine m_socialRelationshipStateMachine; /* 社交关系分析事件状态机 */
    CAlarmStateMachine m_kitchenUnrelatedStateMachine;   /* 后厨无关人员进入事件状态机 */
};
