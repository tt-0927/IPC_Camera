#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

// #include "YoloUltralyticsPoint_rpn.hpp"
#include "ImageFeature.hpp"
#include "ot_common_video.h"
#include "alarm_define.h"
#include "svp_ai_detect.h"
#include "stream_ai_detect.h"
#include "share_data.h"
#include "internal/processors/face/hvf_face_processor.hpp"
#include "internal/processors/boundary/hvf_boundary_processor.hpp"
#include "internal/processors/region/hvf_intrusion_processor.hpp"
#include "internal/processors/region/hvf_loitering_processor.hpp"
#include "internal/processors/region/hvf_parking_processor.hpp"
#include "internal/processors/region/hvf_enter_exit_processor.hpp"

#include "hvf_detect_context.hpp"
class CFaceDetectWorker
{
public:
    enum class TaskType
    {
        FEATURE_EXTRACT = 0,
        FACE_IMAGE = 1,
        VIDEO_FRAME = 2,
    };

    enum class TaskState
    {
        PENDING,
        RUNNING,
        SUCCESS,
        FAILED,
        TIMEOUT
    };

    // using DetectResult = std::vector<Inference_NS::PointData_S>;
    using DetectResult = std::vector<Inference_NS::BoxData_S>;

    using DetectCallback = std::function<void(const DetectResult &)>;

    struct DetectTask
    {
        std::string taskId;

        TaskType type;

        ot_video_frame_info *pFrame;

        int width;
        int height;

        DetectCallback callback;

        /* 视频任务结束或被丢弃时执行的资源归还函数。 */
        std::function<void()> cleanup;
        
        uint64_t seq = 0;

        std::chrono::steady_clock::time_point submitTime;
        ot_video_frame_info featureInputFP16;
    };

    struct TaskResult
    {
        TaskState state = TaskState::PENDING;

        DetectResult result;

        std::chrono::steady_clock::time_point updateTime;

        std::vector<float> feature;
    };

public:
    CFaceDetectWorker();

    ~CFaceDetectWorker();
    bool start(bool bExclusiveModelResidency = false);
    bool init();

    void deinit();
    bool isRunning() const;
    void releaseHandle();

public:
    /*
     * 视频流异步接口
     */
    void submitVideoFrame(ot_video_frame_info *frame,
                          int width,
                          int height,
                          DetectCallback callback,
                          std::function<void()> cleanup);

    /*
     * 人脸库异步接口
     */
    std::string submitFaceImage(ot_video_frame_info *frame, int width, int height);

    /*
     * 查询任务结果
     */
    bool queryTaskResult(const std::string &taskId, TaskResult &result);

    /*
     * 删除任务
     */
    void removeTask(const std::string &taskId);

    std::string submitFeatureTask(const ot_video_frame_info &inputFP16, int width, int height);

    Inference_NS::CImageFeature *getFeatureHandle();

private:
    void workerLoop();

    std::string generateTaskId();

    bool ensureDetectionModel();
    void releaseDetectionModel();
#if CAP_AI_FACE_COMPARE
    bool ensureFeatureModel();
    void releaseFeatureModel();
#endif

private:
    struct TaskCompare
    {
        bool operator()(const std::shared_ptr<DetectTask> &a, const std::shared_ptr<DetectTask> &b)
        {
            /*
             * FACE_IMAGE优先级最高
             */
            if (a->type != b->type)
            {
                return static_cast<int>(a->type) < static_cast<int>(b->type);
            }

            return a->seq > b->seq;
        }
    };

private:
    std::atomic<bool> m_running{ false };

    std::thread m_thread;

    std::mutex m_mutex;

    std::condition_variable m_cond;

    std::priority_queue<std::shared_ptr<DetectTask>, std::vector<std::shared_ptr<DetectTask>>, TaskCompare> m_queue;

private:
    std::mutex m_resultMutex;

    std::unordered_map<std::string, TaskResult> m_taskResults;

private:
    uint64_t m_seq = 0;

private:
    std::mutex m_npuMutex;

    /* 低内存临时任务中，YOLO 与 ArcFace 不允许同时驻留。 */
    bool m_bExclusiveModelResidency = false;

    // Inference_NS::CYoloUltralyticsPoint *m_pFaceDetHandle = nullptr;
    Inference_NS::CYoloUltralytics *m_pFaceDetHandle = nullptr;
    /* 人脸侦测句柄 */
    // HiAiDetect_S *m_pFaceDetHandle = nullptr;
    #if CAP_AI_FACE_COMPARE
    Inference_NS::CImageFeature *m_pFaceFeatureHandle = nullptr;
    // int m_nWidth = PIXEL_WIDTH_1024;
    // /* 算法默认分辨率高度 */
    // int m_nHeight = PIXEL_HEIGHT_576;
    int m_nWidth = PIXEL_WIDTH_640;
    /* 算法默认分辨率高度 */
    int m_nHeight = PIXEL_HEIGHT_640;
    /* 人脸侦测处理器 */
    // HVFDetectInternal::CHVFFaceProcessor m_faceProcessor;
    #endif
};
