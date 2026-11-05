/*
 * @Author: zhengxh
 * @Date: 2026-09-24 08:45:07
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-11-02
 * @Description: 人脸识别主检测器（取流 + 人脸检测/质量评分基础管线）
 */


#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "face_detect_model.hpp"
#include "face_quality_model.hpp"
#include "face_recognition_define.hpp"

/* 子检测器槽位 */
enum SubDetectorIndex_E
{
    SUB_DETECTOR_FACE = 0, /* 人脸识别事件 */
    SUB_DETECTOR_SOCIAL,   /* 社交关系分析事件 */
    SUB_DETECTOR_KITCHEN,  /* 后厨无关人员进入事件 */
    SUB_DETECTOR_COUNT
};

/* 人脸类事件子检测器契约（sub_detectors/），仅持有指针，前置声明 */
class IFaceSubDetector;

/**
 * @brief  : 人脸识别主检测器（CAlgorithm 子类）
 * @note   : 取流框架 + 基础人脸管线（检测/质量评分），无事件业务感知；
 *           事件消费由子检测器完成（sub_detectors/，布防时创建并自拉配置）
 */
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
     * @brief 更新算法运行开关
     * @param stAlgoConfig 当前布防调度生成的算法开关
     * @note 布防的事件创建子检测器并通知其自拉配置，任一事件使能即拉起取流
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

private:
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
     * @brief 填充检测上下文的帧源信息并准备算法输入图像
     */
    bool prepareFrameContext(const MediaData_S &stMediaData,
                             FrameContext_S &stContext);

    /**
     * @brief 执行人脸检测模型并绑定质量评估，产出目标结果
     */
    void detectFaces(DetectContext_S &stDetectContext);

    /**
     * @brief 对单个人脸执行质量评估
     */
    bool evaluateFaceQuality(const cv::Mat &stRgbImage,
                             const FaceDetectedTarget_S &stResult,
                             float &fQualityScore);

    /**
     * @brief 检测线程入口
     */
    void run();

private:
    static constexpr int FRAME_QUEUE_MAX = 2;        /* 帧队列最大长度 */
    static constexpr int DETECT_INTERVAL_MS = 1000;  /* 帧入口固定检测间隔，单位毫秒 */

    BQ_NS::CBlockingQueue<MediaData_S> m_frameQueue; /* 待检测帧队列 */

    /* 检测线程相关定义 */
    std::atomic<bool> m_bRunning{false};
    std::atomic<bool> m_bEnabled{false};
    std::mutex m_threadMutex;
    std::condition_variable m_threadCondition;
    std::thread m_thread;

    EventManager m_recvManager{DETECT_INTERVAL_MS}; /* 检测帧接收频率控制 */

    CFaceDetectModel *m_pFaceDetectHandle = nullptr;  /* 人脸检测模型 */
    CFaceQualityModel *m_pFaceQualityHandle = nullptr; /* 人脸质量模型 */

    cv::Mat m_stPanoramaRgbImage;                    /* 复用的AI全景RGB缓冲 */

    /* 子检测器：人脸/社交/后厨；布防时创建并自拉配置，撤防保留实例 */
    std::array<std::unique_ptr<IFaceSubDetector>, SUB_DETECTOR_COUNT> m_arrSubDetectors;

    /* 布防标志：驱动时跳过未布防槽位 */
    std::atomic<bool> m_bFaceArmed{false};
    std::atomic<bool> m_bSocialArmed{false};
    std::atomic<bool> m_bKitchenArmed{false};
};
