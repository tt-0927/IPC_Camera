#pragma once

#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT

#include <atomic>
#include <chrono>
#include <thread>

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

private:
    bool init();
    void unInit();
    void run();
    void logDiagnostics();
    void processResult(const std::vector<Inference_NS::BoxData_S> &boxes,
                       const SEventProcessContext &context);

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
};

#endif
