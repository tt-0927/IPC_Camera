#include "face_detect_worker.hpp"
#include "dlog.h"
#include "path_define.h"

#include <unistd.h>
namespace
{
constexpr int VIDEO_QUEUE_MAX = 2;

constexpr int FACE_TASK_TIMEOUT_MS = 5000;
}

CFaceDetectWorker::CFaceDetectWorker()
{
    m_running.store(false);
}

CFaceDetectWorker::~CFaceDetectWorker()
{
    deinit();
}

bool CFaceDetectWorker::start()
{
    /*
     * 已启动
     */
    if (m_running.load())
    {
        return true;
    }

    m_running.store(true);

    m_thread = std::thread(&CFaceDetectWorker::workerLoop, this);

    return true;
}
bool CFaceDetectWorker::isRunning() const
{
    return m_running.load();
}

bool CFaceDetectWorker::init()
{

    if (!m_pFaceDetHandle)
    {
        dlog_info("开始显式初始化 YOLO 和 ArcFace 模型...");

        // 1. YOLO 检测模型初始化
        std::string detectModel = AI_FACE_DETECTION_CONFIG_FILE;

        m_pFaceDetHandle = new Inference_NS ::CYoloUltralyticsPoint(detectModel);

        if (!m_pFaceDetHandle || !m_pFaceDetHandle->init())
        {
            dlog_error("YOLO模型初始化失败");
            m_running.store(false);
            return false;
        }

        dlog_info("YOLO模型初始化成功");
    }

    if (!m_pFaceFeatureHandle)
    {
        // 2. ArcFace 特征提取模型初始化
        std::string featureModel = AI_FACE_FEATURE_CONFIG_FILE;

        m_pFaceFeatureHandle = new Inference_NS ::CImageFeature(featureModel);

        if (!m_pFaceFeatureHandle || !m_pFaceFeatureHandle->init())
        {
            dlog_error("ArcFace模型初始化失败");
            m_running.store(false);
            return false;
            ;
        }

        dlog_info("ArcFace模型初始化成功");
    }
    return true;
}

void CFaceDetectWorker::deinit()
{
    m_running.store(false);

    m_cond.notify_all();

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    if (m_pFaceDetHandle)
    {
        delete m_pFaceDetHandle;

        m_pFaceDetHandle = nullptr;
    }
    if (m_pFaceFeatureHandle)
    {
        delete m_pFaceFeatureHandle;

        m_pFaceFeatureHandle = nullptr;
    }
}

void CFaceDetectWorker::releaseHandle()
{
    if (m_pFaceDetHandle)
    {
        delete m_pFaceDetHandle;

        m_pFaceDetHandle = nullptr;
    }
    if (m_pFaceFeatureHandle)
    {
        delete m_pFaceFeatureHandle;

        m_pFaceFeatureHandle = nullptr;
    }
}

void CFaceDetectWorker::submitVideoFrame(ot_video_frame_info *frame, int width, int height, DetectCallback callback)
{
    auto task = std::make_shared<DetectTask>();

    task->type = TaskType::VIDEO_FRAME;
    task->pFrame = frame;
    task->width = width;
    task->height = height;
    task->callback = callback;
    task->submitTime = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        /*
         * 丢弃旧视频帧
         */
        while (m_queue.size() >= VIDEO_QUEUE_MAX)
        {
            auto top = m_queue.top();

            if (top->type == TaskType::VIDEO_FRAME)
            {
                m_queue.pop();
            }
            else
            {
                break;
            }
        }

        task->seq = ++m_seq;

        m_queue.push(task);
    }

    m_cond.notify_one();
}

std::string CFaceDetectWorker::submitFaceImage(ot_video_frame_info *frame, int width, int height)
{
    auto task = std::make_shared<DetectTask>();

    task->taskId = generateTaskId();
    task->type = TaskType::FACE_IMAGE;
    task->pFrame = frame;
    task->width = width;
    task->height = height;
    task->submitTime = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_resultMutex);

        TaskResult result;

        result.state = TaskState::PENDING;

        result.updateTime = std::chrono::steady_clock::now();

        m_taskResults[task->taskId] = result;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        task->seq = ++m_seq;

        m_queue.push(task);
    }

    m_cond.notify_one();

    return task->taskId;
}


std::string CFaceDetectWorker::submitFeatureTask(const ot_video_frame_info &inputFP16, int width, int height)
{
    auto task = std::make_shared<DetectTask>();

    task->taskId = generateTaskId();

    task->type = TaskType::FEATURE_EXTRACT;

    task->featureInputFP16 = inputFP16;
    task->width = width;
    task->height = height;
    task->submitTime = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_resultMutex);

        TaskResult result;

        result.state = TaskState::PENDING;

        result.updateTime = std::chrono::steady_clock::now();

        m_taskResults[task->taskId] = result;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        task->seq = ++m_seq;

        m_queue.push(task);
    }

    m_cond.notify_one();

    return task->taskId;
}

bool CFaceDetectWorker::queryTaskResult(const std::string &taskId, TaskResult &result)
{
    std::lock_guard<std::mutex> lock(m_resultMutex);

    auto iter = m_taskResults.find(taskId);

    if (iter == m_taskResults.end())
    {
        return false;
    }

    result = iter->second;

    return true;
}

void CFaceDetectWorker::removeTask(const std::string &taskId)
{
    std::lock_guard<std::mutex> lock(m_resultMutex);

    m_taskResults.erase(taskId);
}

std::string CFaceDetectWorker::generateTaskId()
{
    static std::atomic<uint64_t> counter{ 0 };

    return std::to_string(++counter);
}

void CFaceDetectWorker::workerLoop()
{
    pthread_setname_np(pthread_self(), "FaceDetW");

    dlog_info("啓動綫程.");
    while (m_running.load())
    {

        if (!init())
        {
            dlog_error("等待人脸检测初始化.");

            std::this_thread ::sleep_for(std::chrono ::seconds(1));

            continue;
        }

        std::shared_ptr<DetectTask> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_cond.wait(lock,
                        [&]
                        {
                            return !m_running.load() || !m_queue.empty();
                        });

            if (!m_running.load())
            {
                break;
            }

            task = m_queue.top();

            m_queue.pop();
        }

        if (!task)
        {
            continue;
        }

        /*
         * 更新RUNNING状态
         */
        if (!task->taskId.empty())
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);

            auto &result = m_taskResults[task->taskId];

            result.state = TaskState::RUNNING;

            result.updateTime = std::chrono::steady_clock::now();
        }

        DetectResult vPointDatas;

        auto startTime = std::chrono::steady_clock::now();

        TaskResult finalResult;

        finalResult.state = TaskState::FAILED;

        if (task->type == TaskType::VIDEO_FRAME || task->type == TaskType::FACE_IMAGE)
        {
            /*
             * YOLO检测
             */
            DetectResult vPointDatas;

            Inference_NS::InputData_S stInputData;

            stInputData.pData = reinterpret_cast<float *>(task->pFrame->video_frame.virt_addr[0]);

            stInputData.nDataSize = static_cast<int>(task->width * task->height * 1.5) * sizeof(float);

            m_pFaceDetHandle->inference(stInputData, vPointDatas);

            finalResult.result = vPointDatas;

            finalResult.state = TaskState::SUCCESS;

            /*
             * 视频流回调
             */
            if (task->callback)
            {
                task->callback(vPointDatas);
            }
        }
        else if (task->type == TaskType ::FEATURE_EXTRACT)
        {
            /*
             * ArcFace特征提取
             */
            Inference_NS::InputData_S stInputData;

            stInputData.pData = reinterpret_cast<float *>(task->featureInputFP16.video_frame.virt_addr[0]);

            stInputData.nDataSize = static_cast<int>(task->width * task->height * 1.5) * sizeof(float);

            std::vector<Inference_NS::ClsData_S> vClsDatas;

            m_pFaceFeatureHandle->inference(stInputData, vClsDatas);

            if (!vClsDatas.empty())
            {
                finalResult.feature = vClsDatas[0].vFeature;

                finalResult.state = TaskState::SUCCESS;
            }
            else
            {
                finalResult.state = TaskState::FAILED;
            }
        }

        auto endTime = std::chrono::steady_clock::now();

        auto costMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        /*
         * 更新任务结果
         */
        if (!task->taskId.empty())
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);

            auto &result = m_taskResults[task->taskId];

            if (costMs > FACE_TASK_TIMEOUT_MS)
            {
                result.state = TaskState::TIMEOUT;
            }
            else
            {
                result = finalResult;
            }

            result.updateTime = std::chrono::steady_clock::now();
        }
    }
}

Inference_NS::CImageFeature *CFaceDetectWorker::getFeatureHandle()
{
    return m_pFaceFeatureHandle;
}
