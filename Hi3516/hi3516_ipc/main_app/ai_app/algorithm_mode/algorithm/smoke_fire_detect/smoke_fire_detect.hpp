#pragma once

#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "algorithm.hpp"
#include "algo_control_deal.h"
#include "blocking_queue.hpp"
#include "common_process.h"
#include "YoloUltralytics_rpn.hpp"

class CSmokeFireDetect : public CAlgorithm
{
public:
    CSmokeFireDetect();
    ~CSmokeFireDetect();

    void recvMediaData(MediaData_S stMediaData) override;
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 单帧抓拍检测结果
     * @note    : 供平台手动抓拍使用，只复用检测能力，不经过报警状态机
     */
    struct SnapshotResult_S
    {
        std::vector<unsigned char> vecJpeg;           /* 编码后的 JPEG 数据 */
        std::vector<Common::RectInfo_S> vstRectInfo;  /* 命中的目标框 */
        bool bSmokeDetected = false;                  /* 是否命中烟雾 */
        bool bFireDetected = false;                   /* 是否命中火焰 */
    };

    /**
     * @brief   : 触发一次单帧抓拍检测
     * @param    {SnapshotResult_S &} stResult 输出：检测结果与 JPEG 数据
     * @param    {int} nTimeoutMs 等待超时，毫秒
     * @return   {bool} true 成功，false 超时或未就绪
     * @note     : 请求被投递到自身流式线程串行执行，避免与 run() 并发调用推理接口
     */
    bool detectOnce(SnapshotResult_S &stResult, int nTimeoutMs = 3000);

private:
    bool init();
    void unInit();
    void run();
    void logDiagnostics();
    void processResult(const std::vector<Inference_NS::BoxData_S> &boxes,
                       const SEventProcessContext &context);

    /**
     * @brief   : 在流式线程内处理一次快照请求
     * @param    {ot_video_frame_info *} pFrameInfo 当前帧
     * @param    {const std::vector<Inference_NS::BoxData_S> &} boxes 推理结果
     * @return   {void}
     */
    void handleSnapshotRequest(ot_video_frame_info *pFrameInfo, const std::vector<Inference_NS::BoxData_S> &boxes);

    /**
     * @brief   : 快照模式下的目标筛选
     * @param    {const std::vector<Inference_NS::BoxData_S> &} boxes 推理结果
     * @param    {SnapshotResult_S &} stResult 输出：命中结果
     * @return   {void}
     * @note     : 仅按置信度阈值筛选，不判断使能、不走报警状态机
     */
    void processSnapshotDetect(const std::vector<Inference_NS::BoxData_S> &boxes, SnapshotResult_S &stResult);

    struct DiagnosticStats
    {
        unsigned int noFrame = 0;
        unsigned int scaleFailed = 0;
        unsigned int inferenceOk = 0;
        unsigned int inferenceFailed = 0;
        unsigned int rawBoxes = 0;
        unsigned int smokeBoxes = 0;
        unsigned int fireBoxes = 0;
        unsigned int otherBoxes = 0;
        unsigned int passedBoxes = 0;
        unsigned int triggeredFrames = 0;
        unsigned int ongoingFrames = 0;
        unsigned int cooldownFrames = 0;
        float maxTargetConfidence = 0.0f;
        float threshold = 0.0f;
    };

    Inference_NS::CYoloUltralytics *m_pDetectHandle = nullptr;
    BQ_NS::CBlockingQueue<MediaData_S> m_dataQueue{2};
    std::atomic<bool> m_bRunning{true};
    std::atomic<bool> m_bEnabled{false};
    std::atomic<unsigned int> m_receivedFrames{0};
    std::atomic<unsigned int> m_queuedFrames{0};
    DiagnosticStats m_stats;
    std::chrono::steady_clock::time_point m_lastDiagnosticLog = std::chrono::steady_clock::now();
    std::thread m_thread;
    EventManager m_recvManager{500};
    CAlarmStateMachine m_alarmStateMachine;
    bool m_eventStartTimeValid = false;
    std::chrono::steady_clock::time_point m_eventStartTime;
    Alarm::SmokeFireDetection_S m_config;
    static constexpr int kWidth = PIXEL_WIDTH_640;
    static constexpr int kHeight = PIXEL_HEIGHT_640;
    ot_video_frame_info m_dstFrameInfo{};

    /* 手动抓拍：请求标志由调用线程置位，结果由流式线程回填，条件变量同步 */
    std::atomic<bool> m_bSnapshotPending{false};
    std::mutex m_snapshotMutex;
    std::condition_variable m_snapshotCv;
    bool m_bSnapshotDone = false;
    SnapshotResult_S m_stSnapshotResult;
};

#endif
