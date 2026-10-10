/**
 * @file face_recognition.hpp
 * @brief 人脸识别算法运行模块
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
#include "FaceAttributeV1_0.hpp"
#include "FaceDetectV1_0.hpp"
#include "FaceQualityAssessmentV1_0.hpp"

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
     * @brief 更新算法运行开关及人脸识别配置
     * @param stAlgoConfig 当前布防调度生成的算法开关
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

private:
    /**
     * @brief 单张业务图片及其保存信息
     */
    struct ImageSaveInfo_S
    {
        cv::Mat stImage;                    /* 图片数据，颜色格式由所属字段约定 */
        std::string strImagePath;           /* 本地保存路径 */
        std::string strImageDate;           /* 图片日期 */
        std::string strImageTime;           /* 图片时间 */
        std::string strCaptureTime;         /* 数据库抓拍时间 */
    };

    /**
     * @brief 单个人脸目标的处理结果
     */
    struct TargetResult_S
    {
        explicit TargetResult_S(FaceDetect_NS::Result_S stResult)
            : stDetectResult(std::move(stResult))
        {
        }

        FaceDetect_NS::Result_S stDetectResult;              /* 人脸检测结果 */
        float fQualityScore = 0.0F;                          /* 人脸质量分数 */
        bool bCaptureTriggered = false;                      /* 是否触发抓拍 */
        bool bAttributeReady = false;                        /* 属性分析是否成功 */
        bool bAttributePushTriggered = false;                /* 是否触发属性推送 */
        FaceAttribute_NS::Result_S stAttributeResult{};      /* 人脸属性结果 */
        ImageSaveInfo_S stTargetImage;                       /* BGR目标小图 */
    };

    /**
     * @brief 当前检测帧对应的高分辨率特写源
     */
    struct CloseupSource_S
    {
        bool isValid() const
        {
            return pNv12 &&
                   nDataSize > 0 &&
                   nWidth > 0 &&
                   nHeight > 0 &&
                   nVirWidth > 0 &&
                   nVirHeight > 0;
        }

        std::shared_ptr<char[]> pNv12;    /* NV12帧数据 */
        std::size_t nDataSize = 0;        /* 数据长度 */
        int nWidth = 0;                   /* 可见宽 */
        int nHeight = 0;                  /* 可见高 */
        int nVirWidth = 0;                /* 虚拟宽 */
        int nVirHeight = 0;               /* 虚拟高 */
    };

    /**
     * @brief 单帧人脸识别上下文，封装算法输入、中间数据和输出结果
     */
    struct FrameContext_S
    {
        int nChannelId = 0;                              /* 通道号 */
        long long llTimestamp = 0;                       /* 毫秒时间戳 */
        Alarm::FaceRecognition_S stConfig;               /* 当前帧配置 */
        ImageSaveInfo_S stPanoramaImage;                 /* RGB全景大图 */
        CloseupSource_S stCloseupSource;                 /* 同步高分辨率特写源 */
        std::vector<TargetResult_S> vstTargets;          /* 有效人脸目标 */
        int nBestTargetIndex = -1;                       /* 事件代表目标下标 */
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
     * @brief 更新算法使用的配置副本
     * @param stConfig 持久化的人脸识别配置
     */
    void updateRuntimeConfig(const Alarm::FaceRecognition_S &stConfig);

    /**
     * @brief 准备当前帧的配置、元数据和算法输入图像
     */
    bool prepareFrameContext(const MediaData_S &stMediaData,
                             FrameContext_S &stContext);

    /**
     * @brief 依次执行人脸检测、过滤、质量评估和属性分析
     */
    bool processAlgorithms(FrameContext_S &stContext);

    /**
     * @brief 执行人脸检测模型
     */
    bool detectFaces(FrameContext_S &stContext);

    /**
     * @brief 执行置信度和检测区域过滤
     */
    void filterDetectionResults(FrameContext_S &stContext) const;

    /**
     * @brief 执行质量评估并形成有效目标结果
     */
    void processQualityAssessment(FrameContext_S &stContext);

    /**
     * @brief 对全部有效目标执行属性分析
     */
    void processAttributeAnalysis(FrameContext_S &stContext);

    /**
     * @brief 按业务组织人脸识别输出
     */
    void processBusinessOutput(FrameContext_S &stContext);

    /**
     * @brief 输出人脸抓拍结果
     */
    void processCaptureResult(FrameContext_S &stContext);

    /**
     * @brief 输出人脸属性分析结果
     */
    void processAttributeResult(FrameContext_S &stContext);

#ifdef ENABLE_TVSDK_SRC
    /**
     * @brief 在算法工作线程同步发送本帧触发的人脸抓拍，每个目标独立携带属性。
     * @param [in,out] stContext 当前帧配置与结果，按需生成目标小图，不要求图片落盘。
     * @param [out] 无。
     * @return 无，编码及推送失败记录日志，不影响其他输出链路。
     */
    void processTvSdkCaptureResult(FrameContext_S &stContext);
#endif

    /**
     * @brief 输出 GAT1400 人脸结果
     */
    void processGat1400Result(FrameContext_S &stContext);

    /**
     * @brief 对全部有效目标执行抓拍规则和帧级间隔判断
     */
    void processCapturePolicy(FrameContext_S &stContext);

    /**
     * @brief 判断属性结果是否需要作为新目标推送
     */
    void processAttributePushPolicy(FrameContext_S &stContext);

    /**
     * @brief 按原图坐标准备单个目标小图
     */
    bool ensureTargetImage(FrameContext_S &stContext,
                           TargetResult_S &stTarget);

    /**
     * @brief 使用RGA从紧凑NV12特写源裁剪并转换BGR目标图
     */
    bool cropCloseupImageByRga(const CloseupSource_S &stCloseupSource,
                               const cv::Rect2f &stDetectRect,
                               const cv::Size &stDetectCoordinateSize,
                               cv::Mat &stTargetBgrImage) const;

    /**
     * @brief 确保本帧全景图已保存并记录数据库
     */
    bool ensurePanoramaImageSaved(FrameContext_S &stContext);

    /**
     * @brief 确保指定目标图已保存并记录数据库
     */
    bool ensureTargetImageSaved(FrameContext_S &stContext,
                                TargetResult_S &stTarget,
                                int nTargetIndex);

    /**
     * @brief 准备本帧图片文件使用的统一时间信息
     */
    void ensureImageTimeInfo(FrameContext_S &stContext,
                             ImageSaveInfo_S &stImageInfo) const;

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
                             const TargetResult_S &stTarget) const;

    /**
     * @brief 输出动态分析框
     */
    void outputOsd(const FrameContext_S &stContext);

    /**
     * @brief 获取最佳目标结果
     */
    TargetResult_S *getBestTarget(FrameContext_S &stContext);
    const TargetResult_S *getBestTarget(const FrameContext_S &stContext) const;

    /**
     * @brief 判断人脸框中心点是否位于规则区域
     */
    bool isInRegion(const Alarm::Region_S &stRegion,
                    const FaceDetect_NS::Result_S &stResult) const;

    /**
     * @brief 判断区域是否包含可用于过滤的有效多边形
     */
    bool isRegionConfigured(const Alarm::Region_S &stRegion) const;

    /**
     * @brief 对单个人脸执行质量评估
     */
    bool evaluateFaceQuality(const cv::Mat &stRgbImage,
                             const FaceDetect_NS::Result_S &stResult,
                             float &fQualityScore);

    /**
     * @brief 裁剪人脸并等比填充到指定尺寸
     */
    bool cropFaceForModel(const cv::Mat &stRgbImage,
                          const FaceDetect_NS::Result_S &stResult,
                          int nWidth,
                          int nHeight,
                          cv::Mat &stFaceImage) const;

    /**
     * @brief 判断目标是否满足抓拍专有规则
     */
    bool isCaptureCandidate(const FaceDetect_NS::Result_S &stResult,
                            const Alarm::FaceRecognitionCaptureRule_S &stRule) const;

    /**
     * @brief 对目标执行人脸属性分析
     */
    bool analyzeFaceAttribute(const cv::Mat &stRgbImage,
                              const FaceDetect_NS::Result_S &stResult,
                              FaceAttribute_NS::Result_S &stAttributeResult);

    /**
     * @brief 处理统一人脸识别事件，按联动选项附加全景图和同一目标的小图。
     * @param [in,out] stContext 当前帧配置和检测结果，按需准备目标小图。
     * @param [out] 无。
     * @return 无，图片处理失败不阻断事件状态处理。
     */
    void processFaceRecognitionEvent(FrameContext_S &stContext);

    /**
     * @brief 检测线程入口
     */
    void run();

private:
    static constexpr int FRAME_QUEUE_MAX = 2;           /* 帧队列最大长度 */
    static constexpr int DETECT_INTERVAL_MS = 2000;     /* 检测间隔，单位毫秒 */
    static constexpr int CONFIG_PIXEL_WIDTH = 1920;     /* 配置坐标系宽度 */
    static constexpr int CONFIG_PIXEL_HEIGHT = 1080;    /* 配置坐标系高度 */

    BQ_NS::CBlockingQueue<MediaData_S> m_frameQueue;    /* 待检测帧队列 */

    /* 检测线程相关定义 */
    std::atomic<bool> m_bRunning{false};
    std::atomic<bool> m_bEnabled{false};
    std::mutex m_threadMutex;
    std::condition_variable m_threadCondition;
    std::thread m_thread;

    std::mutex m_configMutex;                            /* 运行配置保护锁 */
    Alarm::FaceRecognition_S m_stRuntimeConfig;          /* 当前运行配置 */

    EventManager m_recvManager{DETECT_INTERVAL_MS};      /* 检测帧接收频率控制 */

    FaceDetect_NS::CFaceDetectV1_0 *m_pFaceDetectHandle = nullptr;                 /* 人脸检测模型 */
    FaceQualityAssessment_NS::CFaceQualityAssessmentV1_0 *m_pFaceQualityHandle = nullptr; /* 人脸质量模型 */
    FaceAttribute_NS::CFaceAttributeV1_0 *m_pFaceAttributeHandle = nullptr;        /* 人脸属性模型 */

    cv::Mat m_stPanoramaRgbImage;                      /* 复用的AI全景RGB缓冲 */

    std::mutex m_attributeTargetMutex;                   /* 属性目标历史保护锁 */
    std::vector<FaceDetect_NS::Result_S> m_vstLastAttributeTargets; /* 上一帧属性目标 */

    CAlarmStateMachine m_faceRecognitionStateMachine;    /* 人脸识别事件状态机 */
};
