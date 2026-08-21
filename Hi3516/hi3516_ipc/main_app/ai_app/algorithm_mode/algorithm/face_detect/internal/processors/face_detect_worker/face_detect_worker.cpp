/**
 * @FilePath     : face_detect_worker.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-15 14:16:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 14:29:37
 * @Description  : 人脸检测与特征提取异步工作线程实现
 */

#include "face_detect_worker.hpp"
#include "dlog.h"
#include "path_define.h"
#include "internal/base/hvf_detect_common.hpp"
#include <unistd.h>
#include "YoloUltralytics_rpn.hpp"
#include "garbage_detect.hpp"
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

bool CFaceDetectWorker::start(bool bExclusiveModelResidency)
{
    /*
     * 已启动
     */
    if (m_running.load())
    {
        return true;
    }

    m_bExclusiveModelResidency = bExclusiveModelResidency;
    if (m_bExclusiveModelResidency)
    {
        dlog_info("启用低内存模型互斥模式，YOLO 与 ArcFace 将按任务切换");
    }
    m_running.store(true);
    // m_faceProcessor.setEnabled(true);
    m_thread = std::thread(&CFaceDetectWorker::workerLoop, this);

    return true;
}
bool CFaceDetectWorker::isRunning() const
{
    return m_running.load();
}

bool CFaceDetectWorker::init()
{
    if (m_bExclusiveModelResidency)
    {
        return true;
    }

    if (!m_pFaceDetHandle
#if CAP_AI_FACE_COMPARE
        || !m_pFaceFeatureHandle
#endif
    )
    {
        dlog_info("开始显式初始化 YOLO 和 ArcFace 模型...");
    }
    if (!ensureDetectionModel())
    {
        return false;
    }
#if CAP_AI_FACE_COMPARE
    return ensureFeatureModel();
#else
    return true;
#endif
}

bool CFaceDetectWorker::ensureDetectionModel()
{
#if CAP_AI_FACE_COMPARE
    if (m_bExclusiveModelResidency)
    {
        releaseFeatureModel();
    }
#endif
    if (m_pFaceDetHandle)
    {
        return true;
    }

    std::string detectModel = AI_FACE_DETECTION_CONFIG_FILE;
    m_pFaceDetHandle = new Inference_NS::CYoloUltralytics(detectModel);
    if (!m_pFaceDetHandle || !m_pFaceDetHandle->init())
    {
        dlog_error("YOLO模型初始化失败");
        releaseDetectionModel();
        return false;
    }
    dlog_info("YOLO模型初始化成功");
    return true;
}

void CFaceDetectWorker::releaseDetectionModel()
{
    if (m_pFaceDetHandle)
    {
        delete m_pFaceDetHandle;
        m_pFaceDetHandle = nullptr;
        dlog_info("YOLO模型已释放");
    }
}

#if CAP_AI_FACE_COMPARE
bool CFaceDetectWorker::ensureFeatureModel()
{
    if (m_bExclusiveModelResidency)
    {
        releaseDetectionModel();
    }
    if (m_pFaceFeatureHandle)
    {
        return true;
    }

    std::string featureModel = AI_FACE_FEATURE_CONFIG_FILE;
    m_pFaceFeatureHandle = new Inference_NS::CImageFeature(featureModel);
    if (!m_pFaceFeatureHandle || !m_pFaceFeatureHandle->init())
    {
        dlog_error("ArcFace模型初始化失败");
        releaseFeatureModel();
        return false;
    }
    dlog_info("ArcFace模型初始化成功");
    return true;
}

void CFaceDetectWorker::releaseFeatureModel()
{
    if (m_pFaceFeatureHandle)
    {
        delete m_pFaceFeatureHandle;
        m_pFaceFeatureHandle = nullptr;
        dlog_info("ArcFace模型已释放");
    }
}
#endif

void CFaceDetectWorker::deinit()
{
    m_running.store(false);

    m_cond.notify_all();

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    std::vector<std::function<void()>> pendingCleanups;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty())
        {
            auto task = m_queue.top();
            m_queue.pop();
            if (task && task->cleanup)
            {
                pendingCleanups.push_back(task->cleanup);
            }
        }
    }
    for (auto &cleanup : pendingCleanups)
    {
        cleanup();
    }

    releaseDetectionModel();
    // if (m_pFaceDetHandle)
    // {
    //     streamAiDetect_uninit(m_pFaceDetHandle);
    //     m_pFaceDetHandle = nullptr;
    // }
    // m_faceProcessor.setEnabled(false);
    #if CAP_AI_FACE_COMPARE
    releaseFeatureModel();
    #endif
    m_bExclusiveModelResidency = false;
}

void CFaceDetectWorker::releaseHandle()
{
    releaseDetectionModel();
    // if (m_pFaceDetHandle)
    // {
    //     streamAiDetect_uninit(m_pFaceDetHandle);
    //     m_pFaceDetHandle = nullptr;
    // }
    #if CAP_AI_FACE_COMPARE
    releaseFeatureModel();
    #endif
}

void CFaceDetectWorker::submitVideoFrame(ot_video_frame_info *frame,
                                         int width,
                                         int height,
                                         DetectCallback callback,
                                         std::function<void()> cleanup)
{
    auto task = std::make_shared<DetectTask>();

    task->type = TaskType::VIDEO_FRAME;
    task->pFrame = frame;
    task->width = width;
    task->height = height;
    task->callback = callback;
    task->cleanup = cleanup;
    task->submitTime = std::chrono::steady_clock::now();

    std::vector<std::function<void()>> droppedCleanups;
    bool bAccepted = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_running.load())
        {
            if (task->cleanup)
            {
                droppedCleanups.push_back(task->cleanup);
            }
        }
        else
        {

            /*
             * 丢弃旧视频帧
             */
            while (m_queue.size() >= VIDEO_QUEUE_MAX)
            {
                auto top = m_queue.top();

                if (top->type == TaskType::VIDEO_FRAME)
                {
                    m_queue.pop();
                    if (top->cleanup)
                    {
                        droppedCleanups.push_back(top->cleanup);
                    }
                }
                else
                {
                    break;
                }
            }

            task->seq = ++m_seq;

            m_queue.push(task);
            bAccepted = true;
        }
    }

    for (auto &droppedCleanup : droppedCleanups)
    {
        droppedCleanup();
    }

    if (bAccepted)
    {
        m_cond.notify_one();
    }
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

    dlog_info("启动线程.");
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

        bool bModelReady = false;
        if (task->type == TaskType::VIDEO_FRAME || task->type == TaskType::FACE_IMAGE)
        {
            bModelReady = ensureDetectionModel();
        }
#if CAP_AI_FACE_COMPARE
        else if (task->type == TaskType::FEATURE_EXTRACT)
        {
            bModelReady = ensureFeatureModel();
        }
#endif
        if (!bModelReady)
        {
            dlog_error("任务所需模型初始化失败，任务类型[%d]", static_cast<int>(task->type));
            if (task->cleanup)
            {
                task->cleanup();
                task->cleanup = nullptr;
            }
            if (!task->taskId.empty())
            {
                std::lock_guard<std::mutex> lock(m_resultMutex);
                auto &result = m_taskResults[task->taskId];
                result.state = TaskState::FAILED;
                result.updateTime = std::chrono::steady_clock::now();
            }
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
            // std::vector<float> vConfidenceList;
            // if (m_pFaceDetHandle->svpAiDetect_sendFrame(m_pFaceDetHandle, &task->pFrame->video_frame) != TD_SUCCESS)
            // {
            //     continue;
            // }
            // std::vector<Common::RectInfo_S> vstRectInfo;
            // if (m_faceProcessor.isEnabled())
            // {
            //     const ot_aidetect_object_of_one_class *pstObjectClass = HVFDetectInternal::find_object_class(m_pFaceDetHandle->stResult, OT_AIDETECT_CLASS_FACE);
            //     if (pstObjectClass)
            //     {
            //         /* 当前人脸规则换算后的实际置信度门限 */
            //         const float fSensitivityThreshold = 0.4f;
            //         for (size_t i = 0; i < pstObjectClass->object_num; ++i)
            //         {
            //             /* 当前遍历到的人脸目标 */
            //             const ot_aidetect_object &stObject = pstObjectClass->objects[i];
            //             if (stObject.detect_confidence < fSensitivityThreshold)
            //             {
            //                 continue;
            //             }
            //             {
            //                 add_result_to_vector(stObject, vstRectInfo);
            //                 dlog_info("vstRectInfo ： %d",vstRectInfo.size());
            //                 vConfidenceList.push_back(stObject.detect_confidence);
                            
            //             }
                        
            //         }
            //     }
            //     // m_faceProcessor.process(stContext);
            // }
            // dlog_info("vstRectInfo ： %d",vstRectInfo.size());
            // for (size_t i = 0; i < vstRectInfo.size(); i++)
            // {
            //     Inference_NS::PointData_S pointData;

            //     pointData.stBoxs.nX1 = vstRectInfo[i].nX1;
            //     pointData.stBoxs.nX2 = vstRectInfo[i].nX2;
            //     pointData.stBoxs.nY1 = vstRectInfo[i].nY1;
            //     pointData.stBoxs.nY2 = vstRectInfo[i].nY2;

            //     if (i < vConfidenceList.size())
            //     {
            //         pointData.fConfidence = vConfidenceList[i];
            //     }

            //     vPointDatas.push_back(pointData);
            // }
            finalResult.result = vPointDatas;
            finalResult.state = TaskState::SUCCESS;
            /*
             * 视频流回调
             */
            // #define FACE_DETECT_LABEL_ID (2)
            // if( vPointDatas[0].nLabel == FACE_DETECT_LABEL_ID)
            // {
            //     dlog_info("检测到人脸");
           
            if (task->callback)
            {
                task->callback(vPointDatas);
            }
            if (task->cleanup)
            {
                task->cleanup();
                task->cleanup = nullptr;
            }
            // }
            // else {
            //     dlog_info("检测到垃圾");
            //     CGarbageDetect garbageDetect;
            //     garbageDetect.handleDetectResult(vPointDatas,task->pFrame);
            // }

        }
        #if CAP_AI_FACE_COMPARE
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
        #endif
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
#if CAP_AI_FACE_COMPARE
Inference_NS::CImageFeature *CFaceDetectWorker::getFeatureHandle()
{
    return m_pFaceFeatureHandle;
}
#endif
