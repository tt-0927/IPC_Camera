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

#include "YoloUltralyticsPoint_rpn.hpp"
#include "ImageFeature.hpp"
#include "ot_common_video.h"
#include "alarm_define.h"

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

    using DetectResult = std::vector<Inference_NS::PointData_S>;

    using DetectCallback = std::function<void(const DetectResult &)>;

    struct DetectTask
    {
        std::string taskId;

        TaskType type;

        ot_video_frame_info *pFrame;

        int width;
        int height;

        DetectCallback callback;

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
    bool start();
    bool init();

    void deinit();
    bool isRunning() const;
    void releaseHandle();

public:
    /*
     * 视频流异步接口
     */
    void submitVideoFrame(ot_video_frame_info *frame, int width, int height, DetectCallback callback);

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

    Inference_NS::CYoloUltralyticsPoint *m_pFaceDetHandle = nullptr;

    Inference_NS::CImageFeature *m_pFaceFeatureHandle = nullptr;
};