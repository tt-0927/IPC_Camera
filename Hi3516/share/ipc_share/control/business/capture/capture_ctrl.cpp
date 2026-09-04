/**
 * @FilePath     : capture_ctrl.cpp
 * @Author       : 梁浩尧 lianghaoyao@kfb.cn
 * @Date         : 2025-07-17 17:44:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 15:03:25
 * @Description  : 抓图计划管理
 */

#include <unistd.h>
#include <pthread.h>
#include <thread>
#include <sys/time.h>
#include <ctime>
#include <filesystem>
#include "capture_ctrl.h"
#include "capture_configure.h"
#include "capture_database.h"
#include "storage_manage.h"
#include "time_utils.h"
#include <regex>

namespace fs = std::filesystem;

// #define VENC_CHN_JPEG 2
#define DELETE_FILE_TIME_THRESHOLD 60 /* 10分钟 */

/**
 * @brief   : 时间单位转换
 * @param    {TimeUnit_E} eTimeUnit 时间单位
 * @param    {unsigned int} uInterval 时间
 * @return   {unsigned long long} 转换结果 单位毫秒
 */
static unsigned long long time_unit_conversion(Capture_NS::TimeUnit_E eTimeUnit, unsigned int uInterval);

CCaptureCtrl::CCaptureCtrl()
    : m_taskQueue(CAPTURE_QUEUE_MAX_SIZE)
{
}

CCaptureCtrl::CCaptureTaskQueue::CCaptureTaskQueue(int nMaxSize)
    : m_nMaxSize(nMaxSize)
{
}

bool CCaptureCtrl::CCaptureTaskQueue::push(const CaptureTask_S &stTask)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bExit)
    {
        return false;
    }
    if (static_cast<int>(m_queueData.size()) >= m_nMaxSize)
    {
        /* 队列已满，丢弃最旧任务，保证取流线程始终可投递最新图片 */
        m_queueData.pop();
    }
    m_queueData.push(stTask);
    m_condition.notify_one();
    return true;
}

bool CCaptureCtrl::CCaptureTaskQueue::pop(CaptureTask_S &stTask, int nTimeoutMs)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_bExit)
    {
        return false;
    }
    m_condition.wait_for(lock,
                         std::chrono::milliseconds(nTimeoutMs),
                         [this]()
                         {
                             return !m_queueData.empty() || m_bExit;
                         });
    if (m_bExit)
    {
        return false;
    }
    if (m_queueData.empty())
    {
        return false;
    }
    stTask = std::move(m_queueData.front());
    m_queueData.pop();
    return true;
}

void CCaptureCtrl::CCaptureTaskQueue::exit()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bExit = true;
    m_condition.notify_all();
}

void CCaptureCtrl::CCaptureTaskQueue::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bExit = false;
}

CCaptureCtrl::~CCaptureCtrl()
{
    /* 兜底：进程异常退出未走deinit()时，停止工作线程避免可join线程析构触发terminate */
    m_bWorkerRun.store(false, std::memory_order_release);
    m_taskQueue.exit();
    if (m_captureWorkerThread.joinable())
    {
        m_captureWorkerThread.join();
    }
}

IpcRet_E CCaptureCtrl::init()
{
    /* 更新抓图计划 */
    update_capturePlan();
    /* 更新抓图参数 */
    update_captureParam();

    m_mapEventCaptureStates.clear();
    m_bRun.store(true, std::memory_order_release);
    std::thread tid;
    tid = std::thread(&CCaptureCtrl::run, this);
    tid.detach();

    /* 启动抓图落盘工作线程，文件I/O与数据库操作均在该线程串行执行 */
    m_taskQueue.reset();
    m_bWorkerRun.store(true, std::memory_order_release);
    if (m_captureWorkerThread.joinable())
    {
        /* 重复初始化时复用已有工作线程 */
        dlog_warn("抓图工作线程已在运行，复用现有线程");
    }
    else
    {
        m_captureWorkerThread = std::thread(&CCaptureCtrl::capture_worker_run, this);
    }

    return OK;
}

IpcRet_E CCaptureCtrl::deinit()
{
    m_bRun.store(false, std::memory_order_release);

    /* 停止抓图落盘工作线程：唤醒阻塞的pop并等待线程退出 */
    m_bWorkerRun.store(false, std::memory_order_release);
    m_taskQueue.exit();
    if (m_captureWorkerThread.joinable())
    {
        m_captureWorkerThread.join();
    }

    return OK;
}

int CCaptureCtrl::get_jpegVencParamCallback(const GetjpegVencParamCallback &callback)
{
    m_GetJpegVencParamCallback = callback;
    return OK;
}

void CCaptureCtrl::update_capturePlan()
{
    dlog_info("更新抓图计划");
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = false;
    m_infos.clear();

    /* 取出所有抓图计划 */
    Capture_NS::CapturePlan_S stWeeklyCapturePlan;
    CCaptureConfigure::instance()->get_configure(stWeeklyCapturePlan);

    Info_S stInfo;

    for (auto &daySchedule : stWeeklyCapturePlan.vstDaySchedules)
    {
        stInfo.nDayOfWeek = (int) daySchedule.enDayOfWeek;
        stInfo.captureTimes = daySchedule.captureTimes;
        m_infos.push_back(stInfo);
    }
    return;
}

void CCaptureCtrl::update_captureParam()
{
    dlog_info("更新抓图参数");
    std::lock_guard<std::mutex> lock(m_mutex);
    CCaptureConfigure::instance()->get_configure(m_captureParams);
    return;
}

void CCaptureCtrl::get_captureParam(Capture_NS::CaptureParam_S &stCaptureParams)
{
    dlog_info("获取抓图参数");
    std::lock_guard<std::mutex> lock(m_mutex);
    CCaptureConfigure::instance()->get_configure(stCaptureParams);
    return;
}

void CCaptureCtrl::start_capture()
{
}

void CCaptureCtrl::stop_capture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapEventCaptureStates.clear();
    m_TimingCaptureFlag = false;
    return;
}

bool CCaptureCtrl::get_event_capture_status()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 只要有一个事件在抓图就返回true */
    for (auto &pair : m_mapEventCaptureStates)
    {
        if (pair.second.bCaptureFlag)
        {
            return true;
        }
    }

    return false;
}

bool CCaptureCtrl::get_event_first_capture_status(const Event::Type_E enType, std::string &strFirstPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 当前抓图数量大于0，即完成首张图片 */
    if (m_mapEventCaptureStates[enType].unCaptureCount > 0)
    {
        strFirstPath = m_mapEventCaptureStates[enType].stEventInfo.strVideoPath;
        return true;
    }
    strFirstPath = std::string();
    return false;
}

bool CCaptureCtrl::get_timing_capture_status()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_TimingCaptureFlag;
}

int CCaptureCtrl::set_event_capture(bool bEventEnded, const Event::Info_S &stEventInfo)
{
    /*
     * 锁优化：先快速拷贝启用标志到局部变量（避免在 m_mutex 外读 m_captureParams），
     * SD 卡状态检查不涉及 m_mutex 保护的共享数据，在锁外执行以减少竞争。
     */
    bool bEventCaptureEnabled = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bEventCaptureEnabled = m_captureParams.stCaptureEventConfig.bEnable;
    }

    /* 没有启用事件抓图 */
    if (!bEventCaptureEnabled)
    {
        return ERR;
    }

    if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
    {
        dlog_error("sd卡状态异常，不能进行抓图");
        return ERR;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    Event::Type_E enEventType = stEventInfo.enType;

    /* 事件结束 */
    if (bEventEnded)
    {
        auto it = m_mapEventCaptureStates.find(enEventType);
        if (it != m_mapEventCaptureStates.end())
        {
            /* 停止该事件的抓图 */
            it->second.bCaptureFlag = false;
            it->second.unCaptureCount = 0;
            dlog_info("事件[%d]结束，停止抓图", (int) enEventType);
        }
        /* 置空人脸抓拍文件名 */
        if (enEventType == Event::Type_E::FACE_CAPTURE || enEventType == Event::Type_E::FACE_COMPARE)
        {
            /* 使用独立的锁来清空人脸抓拍文件名 */
            std::lock_guard<std::mutex> faceLock(m_faceMutex);
            m_strFaceCaptureFile = std::string();
        }
        return OK;
    }

    /* 事件开始 */
    /* 时间单位统一转换为ms */
    unsigned long long ullInterval = time_unit_conversion(m_captureParams.stCaptureEventConfig.stTimeInterval.enTimeUnit,
                                                          m_captureParams.stCaptureEventConfig.stTimeInterval.unInterval);

    auto it = m_mapEventCaptureStates.find(enEventType);

    /* 第一次出现该类型事件 */
    if (it == m_mapEventCaptureStates.end())
    {
        EventCaptureState_S stState;
        stState.enType = enEventType;
        stState.bCaptureFlag = true;
        stState.unCaptureCount = 0;
        stState.ullLastTriggerTime = TimeUtils_NS::get_currentTimestampMs();
        /* 置0表示尚未抓过图，工作线程收到首张JPEG时立即抓图 */
        stState.ullLastCaptureTime = 0;
        stState.stEventInfo = stEventInfo;

        m_mapEventCaptureStates[enEventType] = stState;
        dlog_info("新事件[%d]触发抓图", (int) enEventType);
    }
    else
    {
        /* 判断当前发生该事件类型比上一次发生该事件类型的时间间隔是否大于等于用户设定的抓图时间间隔 */
        unsigned long long ullCurrentTime = TimeUtils_NS::get_currentTimestampMs();
        if (ullCurrentTime - it->second.ullLastTriggerTime >= ullInterval || enEventType == Event::Type_E::FACE_CAPTURE|| enEventType == Event::Type_E::FACE_COMPARE)
        {
            /* 重新开始抓图 */
            it->second.bCaptureFlag = true;
            it->second.unCaptureCount = 0;
            it->second.ullLastTriggerTime = ullCurrentTime;
            /* 置0表示尚未抓过图，工作线程收到首张JPEG时立即抓图 */
            it->second.ullLastCaptureTime = 0;
            it->second.stEventInfo = stEventInfo;
            dlog_info("事件[%d]再次触发抓图", (int) enEventType);
        }
        else
        {
            dlog_debug("事件[%d]时间间隔未达到设定值，不触发抓图", (int) enEventType);
        }
    }

    return OK;
}

int CCaptureCtrl::send_frameData(unsigned char *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        dlog_error("jpeg数据异常");
        return ERR_PARAM_NULL;
    }

    /* 抓图模块未初始化或已退出，直接丢弃 */
    if (!m_bWorkerRun.load(std::memory_order_acquire))
    {
        return ERR_UNINIT;
    }

    /*
     * perf: 本接口运行在JPEG VENC取流线程等延迟敏感线程上，
     * 仅做一次内存拷贝并投递到有界队列，文件写入与数据库操作全部交给工作线程，
     * 避免fsync/SQLite阻塞流媒体取流链路。
     */
    CaptureTask_S stTask;
    try
    {
        stTask.vecJpegData.assign(pData, pData + nDataLen);
    }
    catch (const std::bad_alloc &)
    {
        dlog_error("抓图任务内存分配失败，丢弃本帧JPEG数据");
        return ERR;
    }

    if (!m_taskQueue.push(stTask))
    {
        dlog_debug("抓图任务队列已退出，丢弃本帧JPEG数据");
        return ERR;
    }

    return OK;
}

void CCaptureCtrl::capture_worker_run()
{
    pthread_setname_np(pthread_self(), "CapWorker");

    CaptureTask_S stTask;
    while (m_bWorkerRun.load(std::memory_order_acquire))
    {
        if (!m_taskQueue.pop(stTask, CAPTURE_QUEUE_POP_TIMEOUT_MS))
        {
            /* 超时未取到任务，继续轮询退出标志 */
            continue;
        }

        process_capture_task(stTask.vecJpegData.data(), static_cast<int>(stTask.vecJpegData.size()));

        /* 清空任务数据但保留容量，避免每帧重复分配大块缓冲 */
        stTask.vecJpegData.clear();
    }

    dlog_info("抓图落盘工作线程退出");
}

void CCaptureCtrl::process_capture_task(unsigned char *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        return;
    }

    /*
     * ! 锁优化：全部 I/O 操作（文件写入、fsync、SQLite 操作）在 m_mutex 外执行。
     * 本函数仅由抓图工作线程单线程调用，事件快照、写入、状态回写天然串行。
     *
     * 分三阶段执行：
     *   Phase 1 [加锁]：快照活跃事件状态 + 预分配序号，判断定时抓图
     *   Phase 2 [无锁]：执行图片文件写入（仅 m_storageMutex 保护存储串行化）
     *   Phase 3 [加锁]：回写事件状态（首图路径、人脸文件名、完成检测）
     */

    /* ---- Phase 1: 快照活跃事件状态 ---- */

    /* 事件快照：{事件类型, 事件信息, 预留的序号, 是否为首张} */
    struct EventCaptureSnapshot
    {
        Event::Type_E enType = Event::Type::MOTION_DETECT;
        Event::Info_S stEventInfo;
        unsigned int unCaptureCount = 0;
        bool bIsFirstCapture = false;
    };
    std::vector<EventCaptureSnapshot> vecActiveEvents;

    bool bDoTimingCapture = false;
    unsigned long long ullTimingInterval = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        /* 收集需要抓图的事件，在锁内递增序号以预留唯一文件名 */
        if (m_captureParams.stCaptureEventConfig.bEnable)
        {
            const unsigned long long ullEventInterval = time_unit_conversion(
                m_captureParams.stCaptureEventConfig.stTimeInterval.enTimeUnit,
                m_captureParams.stCaptureEventConfig.stTimeInterval.unInterval);
            const unsigned long long ullNow = TimeUtils_NS::get_currentTimestampMs();

            for (auto &pair : m_mapEventCaptureStates)
            {
                EventCaptureState_S &stState = pair.second;
                if (!stState.bCaptureFlag)
                {
                    continue;
                }

                /*
                 * 抓图节拍控制：首张立即抓（ullLastCaptureTime为0），
                 * 后续按配置的抓图时间间隔节拍抓图，避免每张到达的JPEG都落盘。
                 * 人脸抓拍/人脸比对由算法层自行控制节奏，不参与节拍限制。
                 */
                if (stState.unCaptureCount > 0 && ullNow - stState.ullLastCaptureTime < ullEventInterval &&
                    pair.first != Event::Type_E::FACE_CAPTURE && pair.first != Event::Type_E::FACE_COMPARE)
                {
                    continue;
                }

                EventCaptureSnapshot snap;
                snap.enType = pair.first;
                snap.stEventInfo = stState.stEventInfo;
                snap.unCaptureCount = stState.unCaptureCount;
                snap.bIsFirstCapture = (stState.unCaptureCount == 0);

                /* 在锁内递增计数，确保并发调用不会生成重复序号 */
                stState.unCaptureCount++;
                stState.ullLastCaptureTime = ullNow;

                /* 达到上限则标记停止（后续 Phase 3 中会再次校验） */
                if (stState.unCaptureCount >= m_captureParams.stCaptureEventConfig.unNumber)
                {
                    stState.bCaptureFlag = false;
                    stState.unCaptureCount = 0;
                    dlog_info("事件[%d]抓图数量达到设定值[%u]，停止抓图",
                              (int) snap.enType,
                              m_captureParams.stCaptureEventConfig.unNumber);
                }

                vecActiveEvents.push_back(snap);
            }
        }

        /* 判断定时抓图 */
        if (m_TimingCaptureFlag)
        {
            ullTimingInterval = time_unit_conversion(
                m_captureParams.stCaptureTimingConfig.stTimeInterval.enTimeUnit,
                m_captureParams.stCaptureTimingConfig.stTimeInterval.unInterval);

            // note 将间隔时间降低50ms，允许误差50ms，避免1999 >= 2000这种情况
            if (ullTimingInterval >= 50)
            {
                ullTimingInterval -= 50;
            }

            long long llCurrentTime = TimeUtils_NS::get_currentTimestampMs();
            if (llCurrentTime - m_lastTimingCaptrueTime >= (long long) ullTimingInterval)
            {
                m_lastTimingCaptrueTime = llCurrentTime;
                bDoTimingCapture = true;
            }
        }
    } /* m_mutex 释放 */

    /* ---- Phase 2: 无锁执行图片写入（仅 m_storageMutex 保护存储操作） ---- */

    std::vector<std::pair<EventCaptureSnapshot, std::string>> vecEventResults;
    for (const auto &snap : vecActiveEvents)
    {
        std::string strFilePath = capture_image(
            Capture_NS::CaptureType_E::EVENT_CAPTURE, pData, nDataLen,
            snap.stEventInfo, snap.unCaptureCount, snap.enType);
        vecEventResults.push_back({snap, strFilePath});
    }

    if (bDoTimingCapture)
    {
        Event::Info_S stTimingInfo;
        capture_image(Capture_NS::CaptureType_E::TIMING_CAPTURE, pData, nDataLen, stTimingInfo);
    }

    /* ---- Phase 3: 回写事件状态（首图路径、人脸抓拍文件名） ---- */

    if (!vecEventResults.empty())
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (const auto &result : vecEventResults)
        {
            const auto &snap = result.first;
            const std::string &strFilePath = result.second;

            if (strFilePath.empty())
            {
                continue;
            }

            auto it = m_mapEventCaptureStates.find(snap.enType);
            if (it == m_mapEventCaptureStates.end())
            {
                continue; /* 事件已被 set_event_capture(false) 清除 */
            }

            EventCaptureState_S &stState = it->second;

            /* 事件在 I/O 期间被结束，不再更新其状态 */
            if (!stState.bCaptureFlag && stState.unCaptureCount == 0)
            {
                continue;
            }

            /* 首张图片路径回写 */
            if (snap.bIsFirstCapture)
            {
                stState.stEventInfo.strVideoPath = strFilePath;

                /* 人脸抓拍：更新文件名并通知等待线程 */
                if (snap.enType == Event::Type_E::FACE_CAPTURE ||
                    snap.enType == Event::Type_E::FACE_COMPARE)
                {
                    {
                        std::lock_guard<std::mutex> faceLock(m_faceMutex);
                        m_strFaceCaptureFile = strFilePath;
                    }
                    m_faceCv.notify_one();
                }
            }
        }
    }
}

int CCaptureCtrl::write_to_file(std::string filePath, unsigned char *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR;
    }

    std::ofstream file(filePath, std::ios::binary | std::ios::app);
    if (!file.is_open())
    {
        dlog_error("打开抓图[%s]文件时出错", filePath.c_str());
        return ERR;
    }

    /* 写入图片数据 */
    file.write(reinterpret_cast<const char *>(pData), nDataLen);

    if (!file.good())
    {
        dlog_error("写入抓图文件失败: %s", filePath.c_str());
        file.close();
        remove(filePath.c_str());
        return ERR;
    }

    /* 刷新缓冲区 */
    file.flush();

    if (!file.good())
    {
        dlog_error("刷新抓图文件失败: %s", filePath.c_str());
        file.close();
        remove(filePath.c_str());
        return ERR;
    }
    /* 获取当前文件大小（写指针位置） */
    std::streampos fileSize = file.tellp();

    file.close();

    /* 强制将文件数据同步到物理磁盘 */
    int fd = ::open(filePath.c_str(), O_WRONLY);
    if (fd != -1)
    {
        // ::fsync(fd);
        if (::fsync(fd) != 0)
        {
            dlog_error("同步抓图文件失败: %s, error: %s", filePath.c_str(), strerror(errno));
            ::close(fd);
            remove(filePath.c_str());
            return ERR;
        }
        ::close(fd);
    }

    return static_cast<int>(fileSize); /* 返回文件大小（字节） */
}

std::string CCaptureCtrl::get_date_storage_path()
{
    /* 获取当前日期，格式：20251103 */
    std::string strDate = TimeUtils_NS::get_currentDate(); /* 假设返回格式为"2025-11-03" */

    /* 去掉日期中的'-' */
    std::string strDateFolder;
    for (char c : strDate)
    {
        if (c != '-')
        {
            strDateFolder += c;
        }
    }

    /* 构建完整路径 */
    std::string strPath = std::string(CAPTURE_PATH) + "/" + strDateFolder;

    return strPath;
}

bool CCaptureCtrl::ensure_directory_exists(const std::string &path)
{
    struct stat info;

    /* 检查目录是否存在 */
    if (stat(path.c_str(), &info) != 0)
    {
        /* 目录不存在，创建目录 */
        if (mkdir(path.c_str(), 0755) != 0)
        {
            dlog_error("创建目录失败: %s", path.c_str());
            return false;
        }
        dlog_info("创建目录成功: %s", path.c_str());
    }
    else if (!(info.st_mode & S_IFDIR))
    {
        /* 路径存在但不是目录 */
        dlog_error("路径存在但不是目录: %s", path.c_str());
        return false;
    }

    return true;
}

std::string CCaptureCtrl::get_face_capture_file()
{
    std::unique_lock<std::mutex> lock(m_faceMutex);

    /* 等待指定时间，如果超时返回空字符串 */
    if (m_faceCv.wait_for(lock,
                          std::chrono::seconds(3),
                          [this]()
                          {
                              return !m_strFaceCaptureFile.empty();
                          }))
    {
        return m_strFaceCaptureFile;
    }

    return "";
}

std::string CCaptureCtrl::capture_image(Capture_NS::CaptureType_E eCaptureType,
                                        unsigned char *pData,
                                        int nDataLen,
                                        const Event::Info_S &stEventInfo,
                                        unsigned int unCaptureCount,
                                        Event::Type_E enEventType)
{
    std::string strFilePath;
    if (!pData || nDataLen <= 0)
    {
        // dlog_error("图片数据为空");
        return strFilePath;
    }

    /*
     * send_frameData() 之外也有直接调用抓图的入口。在格式化或SD卡异常时
     * 必须在创建目录和访问抓图数据库之前退出。
     */
     if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
     {
         dlog_warn("SD card is not ready, skip capture");
         return strFilePath;
     }

    /* 防止多个抓拍线程同时通过配额检查后一起写入。 */
    std::lock_guard<std::mutex> storageLock(m_storageMutex);

    /*
     * 写入前把当前图片大小计入配额，并为人脸图片目录保留物理空间。
     * 如果空间不足，分批删除最旧普通抓图，直到满足条件或无法继续释放。
     */
    int nCleanupBatches = 0;
    while (!CStorageManage::instance()->has_capture_write_space(nDataLen))
    {
        Capture_NS::CaptureDirInfo_S stBeforeCleanup;
        stBeforeCleanup.nChnId = 0;
        if (CCaptureDatabase::instance()->get_itemInfo(stBeforeCleanup) < 0)
        {
            dlog_error("读取抓图目录统计失败，无法执行循环覆盖");
            return strFilePath;
        }

        if (delete_old_images() != OK)
        {
            dlog_error("删除最旧图片失败，无法为新图片释放空间");
            return strFilePath;
        }

        Capture_NS::CaptureDirInfo_S stAfterCleanup;
        stAfterCleanup.nChnId = 0;
        if (CCaptureDatabase::instance()->get_itemInfo(stAfterCleanup) < 0)
        {
            dlog_error("清理后读取抓图目录统计失败");
            return strFilePath;
        }
        ++nCleanupBatches;

        /* 没有旧图可删或删除没有释放空间，避免无效循环。 */
        if (stAfterCleanup.nTotalSize >= stBeforeCleanup.nTotalSize)
        {
            dlog_error("旧图片清理没有释放空间，停止循环以避免死循环");
            return strFilePath;
        }
    }
    dlog_info("循环覆盖清理完成，共执行 %d 批，当前图片大小:%d", nCleanupBatches, nDataLen);


    /* 获取按日期分类的存储路径 */
    std::string strStoragePath = get_date_storage_path();

    /* 确保目录存在 */
    if (!ensure_directory_exists(strStoragePath))
    {
        dlog_error("确保目录存在失败");
        return strFilePath;
    }

    /*
     * send_frameData() 之外也有直接调用抓图的入口。在格式化或SD卡异常时
     * 必须在创建目录和访问抓图数据库之前退出。
     */
    //  if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
    //  {
    //      dlog_warn("SD card is not ready, skip capture");
    //      return strFilePath;
    //  }

    int nSize = 0;

    /* 事件抓图 */
    if (eCaptureType == Capture_NS::CaptureType_E::EVENT_CAPTURE)
    {
        /* 构建文件路径 — 使用调用者传入的 stEventInfo 和 unCaptureCount，
         * 避免在无 m_mutex 保护下访问 m_mapEventCaptureStates */
        strFilePath = strStoragePath + "/" + stEventInfo.strDate + "_" + stEventInfo.strTime + "_" +
                      std::to_string(static_cast<int>(enEventType)) + "_" + std::to_string(unCaptureCount + 1) + ".jpg";

        /* 保存图片 */
        nSize = write_to_file(strFilePath, pData, nDataLen);
        if (nSize < 0)
        {
            dlog_error("写入图片文件失败");
            return strFilePath;
        }

        /* 记录事件图片到数据库 */
        Capture_NS::CaptureInfo_S stInfo;
        stInfo.nChnId = 0;
        stInfo.strImagePath = strFilePath;
        stInfo.nImageSize = nSize;
        stInfo.strStartTime = stEventInfo.strStartTime;
        stInfo.strEndTime = stEventInfo.strEndTime;
        stInfo.enType = enEventType;
        CCaptureDatabase::instance()->add(stInfo);

        /* 更新抓图目录信息 */
        Capture_NS::CaptureDirInfo_S stDirInfo;
        stDirInfo.nChnId = 0;
        int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);

        stDirInfo.nTotalSize += (long long) nSize;
        stDirInfo.nCount++;

        if (nRet < 0)
        {
            CCaptureDatabase::instance()->add(stDirInfo);
        }
        else
        {
            CCaptureDatabase::instance()->update(stDirInfo);
        }
    }
    else /* 定时抓图 */
    {
        std::string strCurrentDate = TimeUtils_NS::get_currentDate();
        std::string strCurrentTimeMs = TimeUtils_NS::get_currentTimeMs();
        std::string strStartTime = TimeUtils_NS::get_currentDateWithDash() + " " + TimeUtils_NS::get_currentTimeWithColon();
        /* 构建文件路径 */
        strFilePath = strStoragePath + "/" + strCurrentDate + "_" + strCurrentTimeMs + ".jpg";

        /* 保存图片 */
        nSize = write_to_file(strFilePath, pData, nDataLen);
        if (nSize < 0)
        {
            dlog_error("写入图片文件失败");
            return strFilePath;
        }

        /* 记录定时抓图信息进数据库 */
        Capture_NS::CaptureInfo_S stInfo;
        stInfo.nImageSize = nSize;
        stInfo.nChnId = 0;
        stInfo.enType = Event::Type_E::SCREENSHOT;
        stInfo.strStartTime = strStartTime;
        stInfo.strEndTime = strStartTime;
        stInfo.strImagePath = strFilePath;

        CCaptureDatabase::instance()->add(stInfo);

        /* 更新抓图目录信息 */
        Capture_NS::CaptureDirInfo_S stDirInfo;
        stDirInfo.nChnId = 0;

        int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
        stDirInfo.nTotalSize += (long long) nSize;
        stDirInfo.nCount++;

        if (nRet < 0)
        {
            CCaptureDatabase::instance()->add(stDirInfo);
        }
        else
        {
            CCaptureDatabase::instance()->update(stDirInfo);
        }
    }
    return strFilePath;
}

int CCaptureCtrl::delete_old_images()
{
    dlog_info("开始批量删除旧图片");

    /* 检查TF卡是否存在 */
    if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
    {
        dlog_error("TF卡状态异常，无法删除图片");
        return ERR;
    }

    /* 获取抓图目录信息 */
    Capture_NS::CaptureDirInfo_S stDirInfo;
    stDirInfo.nChnId = 0;
    int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
    if (nRet < 0)
    {
        dlog_info("抓图目录信息不存在，无需删除");
        return OK;
    }

    /* 构建查询条件：通道ID = 0，按时间升序排序，限制查询数量 */
    Db::MatchMethods methods;

    /* 查询条件：通道ID = 0 */
    methods.push_back(Db::MatchMethod(Db::Element(Db::INFO_CHNL_ID, 0), Db::FIND_CRITERION_EQ, Db::FIND_CRITERION_NONE));

    /* 按开始时间升序排序（从旧到新） */
    std::string strOrderKey = "order by " + std::string(Db::INFO_CAPTURE_STATRTIME);
    std::string strOrderValue = "asc";
    methods.push_back(Db::MatchMethod(Db::Element(strOrderKey, strOrderValue), Db::FIND_CRITERION_NONE, Db::FIND_CRITERION_NONE));

    /* 限制查询数量为 BATCH_DELETE_COUNT */
    std::string strLimitKey = "limit";
    methods.push_back(
        Db::MatchMethod(Db::Element(strLimitKey, BATCH_DELETE_COUNT), Db::FIND_CRITERION_NONE, Db::FIND_CRITERION_NONE));

    /* 查询最旧的图片记录（已在数据库层面排序和限制数量） */
    std::vector<Capture_NS::CaptureInfo_S> vOldImages;
    nRet = CCaptureDatabase::instance()->find(methods, vOldImages);
    if (nRet < 0)
    {
        dlog_error("查询旧图片记录失败");
        return ERR;
    }

    if (vOldImages.empty())
    {
        dlog_info("没有找到可删除的图片记录");
        return OK;
    }

    dlog_info("查询到 %zu 条旧图片记录，准备删除", vOldImages.size());

    /* 批量删除 */
    int nDeleteCount = 0;
    long long llDeletedSize = 0;
    for (const auto &stInfo : vOldImages)
    {
        errno = 0;
        if (remove(stInfo.strImagePath.c_str()) != 0 && errno != ENOENT)
        {
            dlog_error("删除图片失败: %s, error: %s", stInfo.strImagePath.c_str(), strerror(errno));
            continue;
        }

        /* 按路径精确删除对应记录，不能按时间范围误删未成功删除的图片。 */
        MatchMethods delMethods;
        delMethods.push_back(
            MatchMethod(Element(INFO_CAPTURE_PATH, stInfo.strImagePath),
                        FIND_CRITERION_EQ,
                        FIND_CRITERION_NONE));
        nRet = CCaptureDatabase::instance()->del(delMethods, CAPTURE_TABLE_NAME);
        if (nRet < 0)
        {
            dlog_error("删除图片数据库记录失败: %s", stInfo.strImagePath.c_str());
            continue;
        }

        llDeletedSize += stInfo.nImageSize;
        ++nDeleteCount;
    }

    /* 更新抓图目录信息 */
    if (nDeleteCount > 0)
    {
        stDirInfo.nTotalSize -= llDeletedSize;
        stDirInfo.nCount -= nDeleteCount;

        /* 确保不为负数 */
        if (stDirInfo.nTotalSize < 0)
            stDirInfo.nTotalSize = 0;
        if (stDirInfo.nCount < 0)
            stDirInfo.nCount = 0;

        if (CCaptureDatabase::instance()->update(stDirInfo) < 0)
        {
            dlog_error("更新抓图目录统计失败");
            return ERR;
        }
        dlog_info("批量删除完成，共删除 %d 张图片，释放空间 %lld Bytes 更新后数据表空间应为： %lld",
                  nDeleteCount,
                  llDeletedSize,
                  stDirInfo.nTotalSize);
    }
    else
    {
        dlog_error("没有旧图片能够被成功删除");
        return ERR;
    }

    /* 清空vector释放内存 */
    vOldImages.clear();
    vOldImages.shrink_to_fit();

    return OK;
}

static unsigned long long time_unit_conversion(Capture_NS::TimeUnit_E eTimeUnit, unsigned int uInterval)
{
    unsigned int uTimeInterval = 1000;
    switch (eTimeUnit)
    {
    case Capture_NS::TimeUnit_E::SECONDS:
        uTimeInterval = uInterval * 1000;
        break;
    case Capture_NS::TimeUnit_E::MINUTES:
        uTimeInterval = uInterval * 60 * 1000;
        break;
    case Capture_NS::TimeUnit_E::HOURS:
        uTimeInterval = uInterval * 60 * 60 * 1000;
        break;
    case Capture_NS::TimeUnit_E::DAYS:
        uTimeInterval = uInterval * 60 * 60 * 24 * 1000;
        break;
    case Capture_NS::TimeUnit_E::MILLISECONDS:
    default:
        uTimeInterval = uInterval;
        break;
    }
    return static_cast<unsigned long long>(uTimeInterval);
}

/* 时间字符串转时间戳 */
time_t strToTimestamp(const std::string &timeStr)
{
    std::tm tm{};
    strptime(timeStr.c_str(), "%Y%m%d_%H%M%S", &tm);
    return mktime(&tm);
}

/* 遍历目录并比较时间 */
static void checkFilesBySuffix(const std::string &strDirPath, const std::string &strSuffix)
{
    if (!std::filesystem::exists(strDirPath))
    {
        return; // 不存在
    }

    if (!std::filesystem::is_directory(strDirPath))
    {
        return; // 存在但不是目录（是文件等）
    }

    std::regex pattern(R"((\d{8}_\d{6}))");

    time_t now = time(nullptr);

    for (const auto &entry : fs::directory_iterator(strDirPath))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string filename = entry.path().filename().string();

        /* 判断后缀 */
        if (entry.path().extension() != strSuffix)
        {
            continue;
        }

        std::smatch match;

        if (std::regex_search(filename, match, pattern))
        {
            std::string timeStr = match[1]; // 20260312_112817

            time_t fileTime = strToTimestamp(timeStr);

            long lDiff = now - fileTime;

            if (lDiff >= DELETE_FILE_TIME_THRESHOLD)
            {
                /* 安全修复：使用 std::filesystem::remove 替代 system("rm -rf ...")，
                 * 避免命令注入风险，同时减少嵌入式环境下创建子进程的开销 */
                std::error_code ec;
                fs::remove(strDirPath + "/" + filename, ec);
                if (ec)
                {
                    dlog_error("删除过期文件失败: %s, error: %s", filename.c_str(), ec.message().c_str());
                }
                else
                {
                    dlog_info("删除过期文件: %s", filename.c_str());
                }
            }
        }
    }
}

void CCaptureCtrl::run()
{
    pthread_setname_np(pthread_self(), "CapCtrlRun");

    m_lastTimingCaptrueTime = TimeUtils_NS::get_currentTimestampMs();

    /* 循环抓图检查计数器（避免频繁检查） */
    int nLoopCheckCounter = 0;
    const int LOOP_CHECK_INTERVAL = 50; /* 每50次循环检查一次（约10秒） */

    while (m_bRun.load(std::memory_order_acquire))
    {
        /* 避免长时间占用cpu */
        usleep(200 * 1000);

        /* 检查sd卡状态 */
        if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
        {
            continue;
        }

        /* 循环抓图功能：定期检查TF卡配额 */
        nLoopCheckCounter++;
        if (nLoopCheckCounter >= LOOP_CHECK_INTERVAL)
        {
            nLoopCheckCounter = 0;

            /* 检查是否达到配额空间大小 */
            // if (CStorageManage::instance()->get_captureDirUseStatus() < 0)
            // {
            //     dlog_info("达到配额空间，执行循环抓图删除操作");
            //     /* 删除旧图片 */
            //     delete_old_images();
            // }
             {
                std::lock_guard<std::mutex> storageLock(m_storageMutex);
                if (CStorageManage::instance()->get_captureDirUseStatus() < 0)
                {
                    dlog_info("达到配额空间，执行循环抓图删除操作");
                    /* 删除旧图片 */
                    delete_old_images();
                }
            }

            /* 检查下载压缩包文件 */
            fs::path strDirPath = std::string(CAPTURE_PATH) + "/tmp";
            checkFilesBySuffix(strDirPath, ".tgz");
        }

        /*
         * ! 锁优化：run() 线程原来在 m_mutex 保护下遍历整个抓图计划，
         * 改为只在锁内快速拷贝配置，在锁外做时间段匹配计算，减少与 send_frameData 的竞争。
         */

        /*当前星期几*/
        int nDayOfWeek = TimeUtils_NS::getTodayDayOfWeek();
        /*自当天开始的秒数*/
        int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();

        /* 在锁内快速拷贝配置到局部变量 */
        bool bTimingEnabled = false;
        std::vector<Info_S> localInfos;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            bTimingEnabled = m_captureParams.stCaptureTimingConfig.bEnable;
            localInfos = m_infos; /* 浅拷贝，vector + 内部的 vector<CaptureTime_S> */
        }

        /* 在锁外进行时间段匹配计算 */
        bool bNewTimingFlag = false;
        if (bTimingEnabled)
        {
            for (const auto &stInfo : localInfos)
            {
                if (stInfo.nDayOfWeek != nDayOfWeek)
                {
                    continue;
                }

                for (const auto &captureTime : stInfo.captureTimes)
                {
                    if (nCurrentTime >= captureTime.nStartTime && nCurrentTime <= captureTime.nEndTime)
                    {
                        bNewTimingFlag = true;
                        break;
                    }
                }

                if (bNewTimingFlag)
                {
                    break;
                }
            }
        }

        /* 仅在需要写入时加锁 */
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_TimingCaptureFlag = bNewTimingFlag;
        }
    }
}
